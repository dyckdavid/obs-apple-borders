/*
 * Shared Apple chrome helpers
 * MIT License
 */

#include "apple-chrome.h"

#include <graphics/vec2.h>
#include <stdio.h>
#include <string.h>
#include <util/bmem.h>
#include <util/dstr.h>

#include "apple-effect.h"
#include <plugin-support.h>

static void make_key(struct dstr *out, const char *prefix, const char *name)
{
	dstr_copy(out, prefix ? prefix : "");
	if (prefix && *prefix)
		dstr_cat(out, ".");
	dstr_cat(out, name);
}

void apple_chrome_rgba_from_obs_color(struct vec4 *out, uint32_t color)
{
	/* Same path as stock OBS filters (color-source, mask-filter, etc.) */
	vec4_from_rgba(out, color);
}

void apple_chrome_settings_set_defaults(obs_data_t *settings, const char *prefix)
{
	struct dstr key = {0};

	make_key(&key, prefix, "corner_radius");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);

	make_key(&key, prefix, "border_thickness");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_THICKNESS);

	make_key(&key, prefix, "border_color");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_COLOR);

	make_key(&key, prefix, "enable_border");
	obs_data_set_default_bool(settings, key.array, true);

	make_key(&key, prefix, "border_style");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_STYLE);

	make_key(&key, prefix, "border_softness");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_SOFTNESS);

	make_key(&key, prefix, "enable_shadow");
	obs_data_set_default_bool(settings, key.array, true);

	make_key(&key, prefix, "shadow_blur");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_BLUR);

	make_key(&key, prefix, "shadow_opacity");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OPACITY);

	make_key(&key, prefix, "shadow_offset_x");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OFFSET_X);

	make_key(&key, prefix, "shadow_offset_y");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OFFSET_Y);

	make_key(&key, prefix, "content_padding");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_PADDING);

	dstr_free(&key);
}

void apple_chrome_settings_load(struct apple_chrome_settings *out, obs_data_t *settings, const char *prefix)
{
	struct dstr key = {0};
	uint32_t color;

	make_key(&key, prefix, "corner_radius");
	out->corner_radius = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "border_thickness");
	out->border_thickness = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "border_color");
	color = (uint32_t)obs_data_get_int(settings, key.array);
	apple_chrome_rgba_from_obs_color(&out->border_color, color);

	make_key(&key, prefix, "enable_border");
	out->enable_border = obs_data_get_bool(settings, key.array);

	make_key(&key, prefix, "border_style");
	out->border_style = (int)obs_data_get_int(settings, key.array);

	make_key(&key, prefix, "border_softness");
	out->border_softness = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "enable_shadow");
	out->enable_shadow = obs_data_get_bool(settings, key.array);

	/* Softness nudge for glow only — never overwrite border_color (user picker wins). */
	if (out->border_style == APPLE_BORDER_SOFT_GLOW && out->border_softness < 0.5f)
		out->border_softness = 8.0f;

	make_key(&key, prefix, "shadow_blur");
	out->shadow_blur = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_opacity");
	out->shadow_opacity = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_offset_x");
	out->shadow_offset_x = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_offset_y");
	out->shadow_offset_y = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "content_padding");
	out->content_padding = (float)obs_data_get_double(settings, key.array);

	dstr_free(&key);
}

static uint32_t apple_chrome_preset_color(int style)
{
	switch (style) {
	case APPLE_BORDER_HAIRLINE_LIGHT:
		return 0xD9FFFFFF;
	case APPLE_BORDER_HAIRLINE_DARK:
		return 0xD9000000;
	case APPLE_BORDER_SOFT_GLOW:
		return 0xB3FFFFFF;
	case APPLE_BORDER_DOUBLE:
		return 0xE6FFFFFF;
	case APPLE_BORDER_INSET:
		return 0x99FFFFFF;
	case APPLE_BORDER_SOLID_LIGHT:
		return 0xFFFFFFFF;
	case APPLE_BORDER_SOLID_DARK:
		return 0xFF1E1C1C; /* ABGR: #1C1C1E */
	case APPLE_BORDER_ACCENT_BLUE:
		return 0xFFFF840A; /* ABGR: #0A84FF */
	case APPLE_BORDER_CUSTOM:
	default:
		return 0;
	}
}

static bool apple_chrome_border_style_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	char color_key[160];
	char soft_key[160];
	const char *style_name = obs_property_name(property);
	const char *dot;
	int style;
	uint32_t preset;

	UNUSED_PARAMETER(props);

	if (!style_name)
		return false;

	style = (int)obs_data_get_int(settings, style_name);
	preset = apple_chrome_preset_color(style);
	if (preset == 0)
		return true;

	dot = strrchr(style_name, '.');
	if (dot && dot != style_name) {
		int prefix_len = (int)(dot - style_name);
		snprintf(color_key, sizeof(color_key), "%.*s.border_color", prefix_len, style_name);
		snprintf(soft_key, sizeof(soft_key), "%.*s.border_softness", prefix_len, style_name);
	} else {
		snprintf(color_key, sizeof(color_key), "border_color");
		snprintf(soft_key, sizeof(soft_key), "border_softness");
	}

	obs_data_set_int(settings, color_key, (long long)preset);
	if (style == APPLE_BORDER_SOFT_GLOW)
		obs_data_set_double(settings, soft_key, 8.0);
	else
		obs_data_set_double(settings, soft_key, 0.0);

	return true;
}

void apple_chrome_add_properties(obs_properties_t *props, const char *prefix, const char *group_label)
{
	obs_properties_t *group = obs_properties_create();
	struct dstr key = {0};
	obs_property_t *p;

	make_key(&key, prefix, "corner_radius");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("CornerRadius"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "content_padding");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("ContentPadding"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "enable_border");
	obs_properties_add_bool(group, key.array, obs_module_text("EnableBorder"));

	make_key(&key, prefix, "border_style");
	p = obs_properties_add_list(group, key.array, obs_module_text("BorderStyle"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.HairlineLight"), APPLE_BORDER_HAIRLINE_LIGHT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.HairlineDark"), APPLE_BORDER_HAIRLINE_DARK);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SoftGlow"), APPLE_BORDER_SOFT_GLOW);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Double"), APPLE_BORDER_DOUBLE);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Inset"), APPLE_BORDER_INSET);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SolidLight"), APPLE_BORDER_SOLID_LIGHT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SolidDark"), APPLE_BORDER_SOLID_DARK);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.AccentBlue"), APPLE_BORDER_ACCENT_BLUE);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Custom"), APPLE_BORDER_CUSTOM);
	obs_property_set_modified_callback(p, apple_chrome_border_style_modified);

	make_key(&key, prefix, "border_thickness");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("BorderThickness"), 0.0, 24.0, 0.25);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "border_color");
	obs_properties_add_color_alpha(group, key.array, obs_module_text("BorderColor"));

	make_key(&key, prefix, "border_softness");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("BorderSoftness"), 0.0, 24.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "enable_shadow");
	obs_properties_add_bool(group, key.array, obs_module_text("EnableShadow"));

	make_key(&key, prefix, "shadow_blur");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("ShadowBlur"), 0.0, 60.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shadow_opacity");
	obs_properties_add_float_slider(group, key.array, obs_module_text("ShadowOpacity"), 0.0, 1.0, 0.01);

	make_key(&key, prefix, "shadow_offset_x");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("ShadowOffsetX"), -40.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shadow_offset_y");
	p = obs_properties_add_float_slider(group, key.array, obs_module_text("ShadowOffsetY"), -40.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	obs_properties_add_group(props, prefix && *prefix ? prefix : "chrome", group_label, OBS_GROUP_NORMAL, group);

	dstr_free(&key);
}

bool apple_chrome_effect_load(struct apple_chrome_effect *fx)
{
	if (!fx)
		return false;

	memset(fx, 0, sizeof(*fx));

	if (!apple_effect_load(&fx->effect, "effects/apple_chrome.effect"))
		return false;

	fx->param_image = gs_effect_get_param_by_name(fx->effect, "image");
	fx->param_resolution = gs_effect_get_param_by_name(fx->effect, "resolution");
	fx->param_corner_radius = gs_effect_get_param_by_name(fx->effect, "corner_radius");
	fx->param_border_thickness = gs_effect_get_param_by_name(fx->effect, "border_thickness");
	fx->param_border_color = gs_effect_get_param_by_name(fx->effect, "border_color");
	fx->param_shadow_blur = gs_effect_get_param_by_name(fx->effect, "shadow_blur");
	fx->param_shadow_opacity = gs_effect_get_param_by_name(fx->effect, "shadow_opacity");
	fx->param_shadow_offset = gs_effect_get_param_by_name(fx->effect, "shadow_offset");
	fx->param_content_padding = gs_effect_get_param_by_name(fx->effect, "content_padding");
	fx->param_enable_border = gs_effect_get_param_by_name(fx->effect, "enable_border");
	fx->param_enable_shadow = gs_effect_get_param_by_name(fx->effect, "enable_shadow");
	fx->param_border_style = gs_effect_get_param_by_name(fx->effect, "border_style");
	fx->param_border_softness = gs_effect_get_param_by_name(fx->effect, "border_softness");

	return true;
}

void apple_chrome_effect_destroy(struct apple_chrome_effect *fx)
{
	if (!fx)
		return;

	apple_effect_destroy(&fx->effect);
	memset(fx, 0, sizeof(*fx));
}

void apple_chrome_effect_bind(struct apple_chrome_effect *fx, const struct apple_chrome_settings *s, float width,
			      float height)
{
	struct vec2 resolution;
	struct vec2 shadow_offset;

	vec2_set(&resolution, width, height);
	vec2_set(&shadow_offset, s->shadow_offset_x, s->shadow_offset_y);

	if (fx->param_resolution)
		gs_effect_set_vec2(fx->param_resolution, &resolution);
	if (fx->param_corner_radius)
		gs_effect_set_float(fx->param_corner_radius, s->corner_radius);
	if (fx->param_border_thickness)
		gs_effect_set_float(fx->param_border_thickness, s->border_thickness);
	if (fx->param_border_color)
		gs_effect_set_vec4(fx->param_border_color, &s->border_color);
	if (fx->param_shadow_blur)
		gs_effect_set_float(fx->param_shadow_blur, s->shadow_blur);
	if (fx->param_shadow_opacity)
		gs_effect_set_float(fx->param_shadow_opacity, s->shadow_opacity);
	if (fx->param_shadow_offset)
		gs_effect_set_vec2(fx->param_shadow_offset, &shadow_offset);
	if (fx->param_content_padding)
		gs_effect_set_float(fx->param_content_padding, s->content_padding);
	if (fx->param_enable_border)
		gs_effect_set_bool(fx->param_enable_border, s->enable_border);
	if (fx->param_enable_shadow)
		gs_effect_set_bool(fx->param_enable_shadow, s->enable_shadow);
	if (fx->param_border_style)
		gs_effect_set_float(fx->param_border_style, (float)s->border_style);
	if (fx->param_border_softness)
		gs_effect_set_float(fx->param_border_softness, s->border_softness);
}

void apple_pip_corner_position(enum apple_pip_corner corner, float parent_w, float parent_h, float pip_w, float pip_h,
			       float margin_x, float margin_y, float *out_x, float *out_y)
{
	float x = margin_x;
	float y = margin_y;

	switch (corner) {
	case APPLE_PIP_CORNER_TOP_LEFT:
		x = margin_x;
		y = margin_y;
		break;
	case APPLE_PIP_CORNER_TOP_RIGHT:
		x = parent_w - pip_w - margin_x;
		y = margin_y;
		break;
	case APPLE_PIP_CORNER_BOTTOM_LEFT:
		x = margin_x;
		y = parent_h - pip_h - margin_y;
		break;
	case APPLE_PIP_CORNER_BOTTOM_RIGHT:
	default:
		x = parent_w - pip_w - margin_x;
		y = parent_h - pip_h - margin_y;
		break;
	}

	if (out_x)
		*out_x = x;
	if (out_y)
		*out_y = y;
}
