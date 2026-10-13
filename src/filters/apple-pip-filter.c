/*
 * Apple PiP filter — composites a secondary source into a corner with
 * Apple-styled chrome (rounded corners, hairline border, soft shadow).
 * MIT License
 */

#include "apple-pip-filter.h"

#include <graphics/matrix4.h>
#include <string.h>
#include <util/dstr.h>

#include "apple-chrome.h"
#include "apple-filter-util.h"
#include <plugin-support.h>

#define SETTING_PIP_SOURCE "pip_source"
#define SETTING_PIP_CORNER "pip_corner"
#define SETTING_PIP_SIZE "pip_size_percent"
#define SETTING_PIP_MARGIN_X "pip_margin_x"
#define SETTING_PIP_MARGIN_Y "pip_margin_y"
#define SETTING_STYLE_MAIN "style_main"
#define SETTING_STYLE_PIP "style_pip"

struct apple_pip_data {
	obs_source_t *context;

	struct apple_chrome_effect chrome;
	struct apple_chrome_settings main_settings;
	struct apple_chrome_settings pip_settings;

	obs_weak_source_t *pip_weak;
	char *pip_name;

	gs_texrender_t *pip_texrender;

	enum apple_pip_corner corner;
	float size_percent;
	float margin_x;
	float margin_y;
	bool style_main;
	bool style_pip;

	bool pip_showing;
	bool pip_active;
	uint32_t pip_tex_cx;
	uint32_t pip_tex_cy;
};

/* ---------- source enumeration ---------- */

static bool enum_video_sources(void *data, obs_source_t *source)
{
	obs_property_t *list = data;
	const char *name = obs_source_get_name(source);
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_VIDEO) == 0)
		return true;
	if (!name || !*name)
		return true;

	obs_property_list_add_string(list, name, name);
	return true;
}

static bool enum_scenes(void *data, obs_source_t *source)
{
	return enum_video_sources(data, source);
}

/* ---------- pip source lifetime ---------- */

static void apple_pip_release_source(struct apple_pip_data *filter)
{
	obs_source_t *pip;

	if (!filter->pip_weak)
		return;

	pip = obs_weak_source_get_source(filter->pip_weak);
	if (pip) {
		if (filter->pip_active) {
			obs_source_dec_active(pip);
			filter->pip_active = false;
		}
		if (filter->pip_showing) {
			obs_source_dec_showing(pip);
			filter->pip_showing = false;
		}
		obs_source_release(pip);
	}

	obs_weak_source_release(filter->pip_weak);
	filter->pip_weak = NULL;
}

static void apple_pip_acquire_source(struct apple_pip_data *filter, const char *name)
{
	obs_source_t *pip;
	obs_source_t *parent;

	apple_pip_release_source(filter);

	bfree(filter->pip_name);
	filter->pip_name = NULL;

	if (!name || !*name)
		return;

	filter->pip_name = bstrdup(name);
	pip = obs_get_source_by_name(name);
	if (!pip)
		return;

	parent = obs_filter_get_parent(filter->context);

	/* Avoid feedback loops: never PiP the filtered source itself. */
	if (pip == parent || pip == filter->context) {
		obs_source_release(pip);
		obs_log(LOG_WARNING, "PiP source '%s' cannot be the filtered source", name);
		return;
	}

	filter->pip_weak = obs_source_get_weak_source(pip);

	/* Match the filter's current visibility/activation. */
	if (obs_source_showing(filter->context)) {
		obs_source_inc_showing(pip);
		filter->pip_showing = true;
	}
	if (obs_source_active(filter->context)) {
		obs_source_inc_active(pip);
		filter->pip_active = true;
	}

	obs_source_release(pip);
}

/* ---------- OBS filter callbacks ---------- */

static const char *apple_pip_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ApplePipFilter");
}

static void apple_pip_update(void *data, obs_data_t *settings)
{
	struct apple_pip_data *filter = data;
	const char *pip_name = obs_data_get_string(settings, SETTING_PIP_SOURCE);
	const char *corner = obs_data_get_string(settings, SETTING_PIP_CORNER);

	filter->size_percent = (float)obs_data_get_double(settings, SETTING_PIP_SIZE);
	filter->margin_x = (float)obs_data_get_double(settings, SETTING_PIP_MARGIN_X);
	filter->margin_y = (float)obs_data_get_double(settings, SETTING_PIP_MARGIN_Y);
	filter->style_main = obs_data_get_bool(settings, SETTING_STYLE_MAIN);
	filter->style_pip = obs_data_get_bool(settings, SETTING_STYLE_PIP);

	if (corner && strcmp(corner, "top_left") == 0)
		filter->corner = APPLE_PIP_CORNER_TOP_LEFT;
	else if (corner && strcmp(corner, "top_right") == 0)
		filter->corner = APPLE_PIP_CORNER_TOP_RIGHT;
	else if (corner && strcmp(corner, "bottom_left") == 0)
		filter->corner = APPLE_PIP_CORNER_BOTTOM_LEFT;
	else
		filter->corner = APPLE_PIP_CORNER_BOTTOM_RIGHT;

	apple_chrome_settings_load(&filter->main_settings, settings, "main");
	apple_chrome_settings_load(&filter->pip_settings, settings, "pip");

	/* Scale PiP chrome defaults feel — radius stays absolute in px */
	if (!filter->pip_name || strcmp(filter->pip_name, pip_name) != 0)
		apple_pip_acquire_source(filter, pip_name);
}

static void apple_pip_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_PIP_SOURCE, "");
	obs_data_set_default_string(settings, SETTING_PIP_CORNER, "bottom_right");
	obs_data_set_default_double(settings, SETTING_PIP_SIZE, 28.0);
	obs_data_set_default_double(settings, SETTING_PIP_MARGIN_X, 24.0);
	obs_data_set_default_double(settings, SETTING_PIP_MARGIN_Y, 24.0);
	obs_data_set_default_bool(settings, SETTING_STYLE_MAIN, false);
	obs_data_set_default_bool(settings, SETTING_STYLE_PIP, true);

	apple_chrome_settings_set_defaults(settings, "main");
	apple_chrome_settings_set_defaults(settings, "pip");

	/* Slightly tighter chrome on the inset window */
	obs_data_set_default_double(settings, "pip.corner_radius", 14.0);
	obs_data_set_default_double(settings, "pip.content_padding", 18.0);
	obs_data_set_default_double(settings, "pip.shadow_blur", 14.0);
	obs_data_set_default_double(settings, "pip.shadow_offset_y", 6.0);
}

static obs_properties_t *apple_pip_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *list;

	UNUSED_PARAMETER(data);

	obs_properties_add_text(props, "info", obs_module_text("ApplePip.Info"), OBS_TEXT_INFO);

	list = obs_properties_add_list(props, SETTING_PIP_SOURCE, obs_module_text("PipSource"), OBS_COMBO_TYPE_EDITABLE,
				       OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, obs_module_text("PipSource.None"), "");
	obs_enum_sources(enum_video_sources, list);
	obs_enum_scenes(enum_scenes, list);

	p = obs_properties_add_list(props, SETTING_PIP_CORNER, obs_module_text("PipCorner"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, obs_module_text("PipCorner.TopLeft"), "top_left");
	obs_property_list_add_string(p, obs_module_text("PipCorner.TopRight"), "top_right");
	obs_property_list_add_string(p, obs_module_text("PipCorner.BottomLeft"), "bottom_left");
	obs_property_list_add_string(p, obs_module_text("PipCorner.BottomRight"), "bottom_right");

	p = obs_properties_add_float_slider(props, SETTING_PIP_SIZE, obs_module_text("PipSize"), 8.0, 60.0, 0.5);
	obs_property_float_set_suffix(p, " %");

	p = obs_properties_add_float_slider(props, SETTING_PIP_MARGIN_X, obs_module_text("PipMarginX"), 0.0, 200.0,
					    1.0);
	obs_property_float_set_suffix(p, " px");

	p = obs_properties_add_float_slider(props, SETTING_PIP_MARGIN_Y, obs_module_text("PipMarginY"), 0.0, 200.0,
					    1.0);
	obs_property_float_set_suffix(p, " px");

	obs_properties_add_bool(props, SETTING_STYLE_MAIN, obs_module_text("StyleMain"));
	obs_properties_add_bool(props, SETTING_STYLE_PIP, obs_module_text("StylePip"));

	apple_chrome_add_properties(props, "main", obs_module_text("MainChromeGroup"));
	apple_chrome_add_properties(props, "pip", obs_module_text("PipChromeGroup"));

	return props;
}

static void apple_pip_destroy(void *data)
{
	struct apple_pip_data *filter = data;

	apple_pip_release_source(filter);
	bfree(filter->pip_name);
	filter->pip_name = NULL;

	if (filter->pip_texrender) {
		obs_enter_graphics();
		gs_texrender_destroy(filter->pip_texrender);
		obs_leave_graphics();
		filter->pip_texrender = NULL;
	}

	apple_chrome_effect_destroy(&filter->chrome);
	bfree(filter);
}

static void *apple_pip_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_pip_data *filter = bzalloc(sizeof(*filter));
	filter->context = context;
	filter->style_pip = true;
	filter->corner = APPLE_PIP_CORNER_BOTTOM_RIGHT;
	filter->size_percent = 28.0f;
	filter->margin_x = 24.0f;
	filter->margin_y = 24.0f;

	if (!apple_chrome_effect_load(&filter->chrome)) {
		apple_pip_destroy(filter);
		return NULL;
	}

	obs_enter_graphics();
	filter->pip_texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	obs_leave_graphics();

	if (!filter->pip_texrender) {
		apple_pip_destroy(filter);
		return NULL;
	}

	apple_pip_update(filter, settings);
	return filter;
}

static bool apple_pip_render_source_to_tex(struct apple_pip_data *filter, obs_source_t *pip, uint32_t cx, uint32_t cy)
{
	gs_texrender_reset(filter->pip_texrender);

	if (!gs_texrender_begin(filter->pip_texrender, cx, cy))
		return false;

	struct vec4 clear_color;
	vec4_zero(&clear_color);
	gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
	gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

	obs_source_video_render(pip);

	gs_texrender_end(filter->pip_texrender);
	filter->pip_tex_cx = cx;
	filter->pip_tex_cy = cy;
	return true;
}

static void apple_pip_draw_textured_rect(struct apple_pip_data *filter, gs_texture_t *tex, float x, float y, float w,
					 float h, bool apply_chrome, const struct apple_chrome_settings *s)
{
	gs_effect_t *effect;
	gs_eparam_t *image_param;

	if (apply_chrome && filter->chrome.effect) {
		effect = filter->chrome.effect;
		apple_chrome_effect_bind(&filter->chrome, s, w, h);
		image_param = filter->chrome.param_image;
	} else {
		effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		image_param = gs_effect_get_param_by_name(effect, "image");
	}

	if (!effect || !tex)
		return;

	if (image_param)
		gs_effect_set_texture(image_param, tex);

	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);

	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(tex, 0, (uint32_t)w, (uint32_t)h);

	gs_matrix_pop();
}

static void apple_pip_render(void *data, gs_effect_t *effect)
{
	struct apple_pip_data *filter = data;
	struct apple_filter_frame frame;
	obs_source_t *pip = NULL;
	float pip_w;
	float pip_h;
	float pip_x;
	float pip_y;
	gs_texture_t *pip_tex;
	uint32_t src_cx;
	uint32_t src_cy;

	UNUSED_PARAMETER(effect);

	if (!apple_filter_begin(filter->context, &frame))
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	/* Draw the main (filtered) source, optionally with Apple chrome. */
	if (filter->style_main && filter->chrome.effect) {
		apple_chrome_effect_bind(&filter->chrome, &filter->main_settings, (float)frame.width,
					 (float)frame.height);
		obs_source_process_filter_end(filter->context, filter->chrome.effect, frame.width, frame.height);
	} else {
		obs_source_process_filter_end(filter->context, obs_get_base_effect(OBS_EFFECT_DEFAULT), frame.width,
					      frame.height);
	}

	/* Overlay PiP */
	if (filter->pip_weak)
		pip = obs_weak_source_get_source(filter->pip_weak);

	if (pip) {
		src_cx = obs_source_get_base_width(pip);
		src_cy = obs_source_get_base_height(pip);

		if (src_cx > 0 && src_cy > 0) {
			pip_w = (float)frame.width * (filter->size_percent / 100.0f);
			pip_h = pip_w * ((float)src_cy / (float)src_cx);

			if (pip_h > (float)frame.height * 0.9f) {
				pip_h = (float)frame.height * 0.9f;
				pip_w = pip_h * ((float)src_cx / (float)src_cy);
			}

			apple_pip_corner_position(filter->corner, (float)frame.width, (float)frame.height, pip_w, pip_h,
						  filter->margin_x, filter->margin_y, &pip_x, &pip_y);

			if (apple_pip_render_source_to_tex(filter, pip, src_cx, src_cy)) {
				pip_tex = gs_texrender_get_texture(filter->pip_texrender);
				apple_pip_draw_textured_rect(filter, pip_tex, pip_x, pip_y, pip_w, pip_h,
							     filter->style_pip, &filter->pip_settings);
			}
		}

		obs_source_release(pip);
	}

	gs_blend_state_pop();
}

static enum gs_color_space apple_pip_get_color_space(void *data, size_t count,
						     const enum gs_color_space *preferred_spaces)
{
	struct apple_pip_data *filter = data;
	return apple_filter_get_color_space(filter->context, count, preferred_spaces);
}

static void apple_pip_show(void *data)
{
	struct apple_pip_data *filter = data;
	obs_source_t *pip;

	if (!filter->pip_weak || filter->pip_showing)
		return;

	pip = obs_weak_source_get_source(filter->pip_weak);
	if (pip) {
		obs_source_inc_showing(pip);
		filter->pip_showing = true;
		obs_source_release(pip);
	}
}

static void apple_pip_hide(void *data)
{
	struct apple_pip_data *filter = data;
	obs_source_t *pip;

	if (!filter->pip_weak || !filter->pip_showing)
		return;

	pip = obs_weak_source_get_source(filter->pip_weak);
	if (pip) {
		obs_source_dec_showing(pip);
		filter->pip_showing = false;
		obs_source_release(pip);
	}
}

static void apple_pip_activate(void *data)
{
	struct apple_pip_data *filter = data;
	obs_source_t *pip;

	if (!filter->pip_weak || filter->pip_active)
		return;

	pip = obs_weak_source_get_source(filter->pip_weak);
	if (pip) {
		obs_source_inc_active(pip);
		filter->pip_active = true;
		obs_source_release(pip);
	}
}

static void apple_pip_deactivate(void *data)
{
	struct apple_pip_data *filter = data;
	obs_source_t *pip;

	if (!filter->pip_weak || !filter->pip_active)
		return;

	pip = obs_weak_source_get_source(filter->pip_weak);
	if (pip) {
		obs_source_dec_active(pip);
		filter->pip_active = false;
		obs_source_release(pip);
	}
}

struct obs_source_info apple_pip_filter = {
	.id = "apple_pip_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_pip_get_name,
	.create = apple_pip_create,
	.destroy = apple_pip_destroy,
	.update = apple_pip_update,
	.video_render = apple_pip_render,
	.get_properties = apple_pip_properties,
	.get_defaults = apple_pip_defaults,
	.video_get_color_space = apple_pip_get_color_space,
	.show = apple_pip_show,
	.hide = apple_pip_hide,
	.activate = apple_pip_activate,
	.deactivate = apple_pip_deactivate,
};
