/*
 * Apple Lower Third — frosted rounded bar + left accent + title/subtitle text.
 * MIT License
 */

#include "apple-lower-third-source.h"

#include <math.h>

#include <graphics/vec2.h>
#include <graphics/vec4.h>

#include "apple-chrome.h"
#include "apple-effect.h"
#include <plugin-support.h>

enum apple_lt_position {
	APPLE_LT_BOTTOM_LEFT = 0,
	APPLE_LT_BOTTOM_RIGHT = 1,
	APPLE_LT_BOTTOM_CENTER = 2,
};

enum apple_lt_style {
	APPLE_LT_STYLE_RESET = 0, /* Reset (Classic Frost) */
	APPLE_LT_STYLE_CLASSIC_FROST = 1,
	APPLE_LT_STYLE_MIDNIGHT = 2,
	APPLE_LT_STYLE_CLEAN_LIGHT = 3,
	APPLE_LT_STYLE_BOLD_BROADCAST = 4,
	APPLE_LT_STYLE_SOFT_VIOLET = 5,
};

#define APPLE_LT_TEXT_PAD_X 22.0f
#define APPLE_LT_TEXT_PAD_Y 22.0f
#define APPLE_LT_TITLE_SIZE 42
#define APPLE_LT_SUBTITLE_SIZE 24
#define APPLE_LT_ACCENT_DEFAULT 12.0f

/* Classic Frost defaults (OBS ABGR packing via vec4_from_rgba) */
#define APPLE_LT_DEFAULT_BAR_COLOR 0xF0FFFFFF    /* near-white frosted */
#define APPLE_LT_DEFAULT_ACCENT_COLOR 0xFFFF840A /* #0A84FF vivid Apple blue */
#define APPLE_LT_DEFAULT_CORNER 18.0
#define APPLE_LT_DEFAULT_HEIGHT 120.0
#define APPLE_LT_DEFAULT_ACCENT_W 12.0

/* Dark near-black text (OBS ABGR) */
#define APPLE_LT_TEXT_COLOR 0xFF1D1D1F

struct apple_lower_third_data {
	obs_source_t *source;
	gs_effect_t *effect;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_bar_color;
	gs_eparam_t *param_accent_color;
	gs_eparam_t *param_bar_origin;
	gs_eparam_t *param_bar_size;
	gs_eparam_t *param_corner_radius;
	gs_eparam_t *param_accent_width;

	obs_source_t *title_source;
	obs_source_t *subtitle_source;
	bool text_failed;
	bool text_fail_logged;

	uint32_t cx;
	uint32_t cy;

	float width_percent;
	float height_px;
	float margin_x;
	float margin_y;
	float corner_radius;
	float accent_width;
	struct vec4 bar_color;
	struct vec4 accent_color;
	bool enable_accent;
	enum apple_lt_position position;

	char *title_text;
	char *subtitle_text;
};

static void apple_lt_apply_style(obs_data_t *settings, int style)
{
	switch (style) {
	case APPLE_LT_STYLE_MIDNIGHT:
		/* Dark translucent bar + cyan/blue accent */
		obs_data_set_int(settings, "bar_color", 0xB31C1C1E);
		obs_data_set_int(settings, "accent_color", 0xFFFFD264); /* #64D2FF */
		obs_data_set_double(settings, "accent_width", 10.0);
		obs_data_set_double(settings, "corner_radius", 16.0);
		obs_data_set_double(settings, "bar_height_px", 120.0);
		obs_data_set_bool(settings, "enable_accent", true);
		break;
	case APPLE_LT_STYLE_CLEAN_LIGHT:
		/* Very light bar + thin bright blue */
		obs_data_set_int(settings, "bar_color", 0xE8FAFAFC);
		obs_data_set_int(settings, "accent_color", 0xFFFF9F0A); /* #0A9FFF */
		obs_data_set_double(settings, "accent_width", 6.0);
		obs_data_set_double(settings, "corner_radius", 20.0);
		obs_data_set_double(settings, "bar_height_px", 110.0);
		obs_data_set_bool(settings, "enable_accent", true);
		break;
	case APPLE_LT_STYLE_BOLD_BROADCAST:
		/* White bar + thick royal blue */
		obs_data_set_int(settings, "bar_color", 0xFFFFFFFF);
		obs_data_set_int(settings, "accent_color", 0xFFD45500); /* #0055D4 */
		obs_data_set_double(settings, "accent_width", 18.0);
		obs_data_set_double(settings, "corner_radius", 12.0);
		obs_data_set_double(settings, "bar_height_px", 130.0);
		obs_data_set_bool(settings, "enable_accent", true);
		break;
	case APPLE_LT_STYLE_SOFT_VIOLET:
		/* Light bar + purple accent */
		obs_data_set_int(settings, "bar_color", 0xF0F8F0F5);
		obs_data_set_int(settings, "accent_color", 0xFFF25ABF); /* #BF5AF2 */
		obs_data_set_double(settings, "accent_width", 10.0);
		obs_data_set_double(settings, "corner_radius", 20.0);
		obs_data_set_double(settings, "bar_height_px", 120.0);
		obs_data_set_bool(settings, "enable_accent", true);
		break;
	case APPLE_LT_STYLE_RESET:
	case APPLE_LT_STYLE_CLASSIC_FROST:
	default:
		/* Classic Frost / Reset */
		obs_data_set_int(settings, "bar_color", APPLE_LT_DEFAULT_BAR_COLOR);
		obs_data_set_int(settings, "accent_color", APPLE_LT_DEFAULT_ACCENT_COLOR);
		obs_data_set_double(settings, "accent_width", APPLE_LT_DEFAULT_ACCENT_W);
		obs_data_set_double(settings, "corner_radius", APPLE_LT_DEFAULT_CORNER);
		obs_data_set_double(settings, "bar_height_px", APPLE_LT_DEFAULT_HEIGHT);
		obs_data_set_bool(settings, "enable_accent", true);
		break;
	}
}

static bool apple_lt_style_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	int style;

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	style = (int)obs_data_get_int(settings, "style_preset");
	apple_lt_apply_style(settings, style);
	return true;
}

static const char *apple_lt_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleLowerThirdSource");
}

static obs_data_t *apple_lt_make_text_settings(const char *text, int font_size, bool bold)
{
	obs_data_t *settings = obs_data_create();
	obs_data_t *font = obs_data_create();

#if defined(__APPLE__)
	obs_data_set_string(font, "face", "Helvetica Neue");
#elif defined(_WIN32)
	obs_data_set_string(font, "face", "Segoe UI");
#else
	obs_data_set_string(font, "face", "Sans Serif");
#endif
	obs_data_set_string(font, "style", bold ? "Bold" : "Regular");
	obs_data_set_int(font, "size", font_size);
	obs_data_set_int(font, "flags", bold ? 1 : 0); /* OBS_FONT_BOLD = 1 */

	obs_data_set_obj(settings, "font", font);
	obs_data_release(font);

	obs_data_set_string(settings, "text", text ? text : "");
	obs_data_set_bool(settings, "antialiasing", true);
	obs_data_set_bool(settings, "outline", false);
	obs_data_set_bool(settings, "drop_shadow", false);
	obs_data_set_bool(settings, "word_wrap", false);
	obs_data_set_bool(settings, "from_file", false);
	obs_data_set_int(settings, "color1", APPLE_LT_TEXT_COLOR);
	obs_data_set_int(settings, "color2", APPLE_LT_TEXT_COLOR);

	return settings;
}

static obs_source_t *apple_lt_create_text_source(const char *name, const char *text, int font_size, bool bold)
{
	obs_data_t *settings = apple_lt_make_text_settings(text, font_size, bold);
	obs_source_t *src = obs_source_create_private("text_ft2_source_v2", name, settings);
	if (!src)
		src = obs_source_create_private("text_ft2_source", name, settings);
	obs_data_release(settings);
	return src;
}

static void apple_lt_update_text_source(obs_source_t *src, const char *text, int font_size, bool bold)
{
	obs_data_t *settings;

	if (!src)
		return;

	settings = apple_lt_make_text_settings(text, font_size, bold);
	obs_source_update(src, settings);
	obs_data_release(settings);
}

static void apple_lt_ensure_text_sources(struct apple_lower_third_data *ctx)
{
	if (ctx->text_failed)
		return;

	if (!ctx->title_source) {
		ctx->title_source = apple_lt_create_text_source(
			"apple_lt_title", ctx->title_text ? ctx->title_text : "Guest Name", APPLE_LT_TITLE_SIZE, true);
	}
	if (!ctx->subtitle_source) {
		ctx->subtitle_source = apple_lt_create_text_source(
			"apple_lt_subtitle", ctx->subtitle_text ? ctx->subtitle_text : "Title / Role",
			APPLE_LT_SUBTITLE_SIZE, false);
	}

	if (!ctx->title_source || !ctx->subtitle_source) {
		ctx->text_failed = true;
		if (!ctx->text_fail_logged) {
			obs_log(LOG_WARNING, "Apple Lower Third: FreeType text source unavailable; drawing bar only");
			ctx->text_fail_logged = true;
		}
		if (ctx->title_source) {
			obs_source_release(ctx->title_source);
			ctx->title_source = NULL;
		}
		if (ctx->subtitle_source) {
			obs_source_release(ctx->subtitle_source);
			ctx->subtitle_source = NULL;
		}
	}
}

static void apple_lt_update(void *data, obs_data_t *settings)
{
	struct apple_lower_third_data *ctx = data;
	uint32_t bar_c;
	uint32_t accent_c;
	const char *title;
	const char *subtitle;

	ctx->cx = (uint32_t)obs_data_get_int(settings, "width");
	ctx->cy = (uint32_t)obs_data_get_int(settings, "height");
	if (ctx->cx < 64)
		ctx->cx = 64;
	if (ctx->cy < 64)
		ctx->cy = 64;

	ctx->width_percent = (float)obs_data_get_double(settings, "bar_width_percent");
	ctx->height_px = (float)obs_data_get_double(settings, "bar_height_px");
	ctx->margin_x = (float)obs_data_get_double(settings, "margin_x");
	ctx->margin_y = (float)obs_data_get_double(settings, "margin_y");
	ctx->corner_radius = (float)obs_data_get_double(settings, "corner_radius");
	ctx->position = (enum apple_lt_position)obs_data_get_int(settings, "position");
	ctx->enable_accent = obs_data_get_bool(settings, "enable_accent");
	ctx->accent_width = (float)obs_data_get_double(settings, "accent_width");
	if (ctx->accent_width < 1.0f)
		ctx->accent_width = APPLE_LT_ACCENT_DEFAULT;
	if (!ctx->enable_accent)
		ctx->accent_width = 0.0f;

	bar_c = (uint32_t)obs_data_get_int(settings, "bar_color");
	accent_c = (uint32_t)obs_data_get_int(settings, "accent_color");
	apple_chrome_rgba_from_obs_color(&ctx->bar_color, bar_c);
	apple_chrome_rgba_from_obs_color(&ctx->accent_color, accent_c);

	title = obs_data_get_string(settings, "title");
	subtitle = obs_data_get_string(settings, "subtitle");

	bfree(ctx->title_text);
	bfree(ctx->subtitle_text);
	ctx->title_text = bstrdup(title ? title : "");
	ctx->subtitle_text = bstrdup(subtitle ? subtitle : "");

	apple_lt_ensure_text_sources(ctx);
	apple_lt_update_text_source(ctx->title_source, ctx->title_text, APPLE_LT_TITLE_SIZE, true);
	apple_lt_update_text_source(ctx->subtitle_source, ctx->subtitle_text, APPLE_LT_SUBTITLE_SIZE, false);
}

static void apple_lt_destroy(void *data)
{
	struct apple_lower_third_data *ctx = data;

	if (ctx->title_source) {
		obs_source_release(ctx->title_source);
		ctx->title_source = NULL;
	}
	if (ctx->subtitle_source) {
		obs_source_release(ctx->subtitle_source);
		ctx->subtitle_source = NULL;
	}

	bfree(ctx->title_text);
	bfree(ctx->subtitle_text);

	apple_effect_destroy(&ctx->effect);
	bfree(ctx);
}

static void *apple_lt_create(obs_data_t *settings, obs_source_t *source)
{
	struct apple_lower_third_data *ctx = bzalloc(sizeof(*ctx));

	ctx->source = source;
	ctx->cx = 1920;
	ctx->cy = 1080;
	ctx->accent_width = APPLE_LT_ACCENT_DEFAULT;

	if (!apple_effect_load(&ctx->effect, "effects/apple_lower_third.effect")) {
		apple_lt_destroy(ctx);
		return NULL;
	}

	ctx->param_resolution = gs_effect_get_param_by_name(ctx->effect, "resolution");
	ctx->param_bar_color = gs_effect_get_param_by_name(ctx->effect, "bar_color");
	ctx->param_accent_color = gs_effect_get_param_by_name(ctx->effect, "accent_color");
	ctx->param_bar_origin = gs_effect_get_param_by_name(ctx->effect, "bar_origin");
	ctx->param_bar_size = gs_effect_get_param_by_name(ctx->effect, "bar_size");
	ctx->param_corner_radius = gs_effect_get_param_by_name(ctx->effect, "corner_radius");
	ctx->param_accent_width = gs_effect_get_param_by_name(ctx->effect, "accent_width");

	apple_lt_update(ctx, settings);
	return ctx;
}

static uint32_t apple_lt_width(void *data)
{
	struct apple_lower_third_data *ctx = data;
	return ctx->cx;
}

static uint32_t apple_lt_height(void *data)
{
	struct apple_lower_third_data *ctx = data;
	return ctx->cy;
}

static void apple_lt_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "width", 1920);
	obs_data_set_default_int(settings, "height", 1080);
	obs_data_set_default_double(settings, "bar_width_percent", 50.0);
	obs_data_set_default_double(settings, "bar_height_px", APPLE_LT_DEFAULT_HEIGHT);
	obs_data_set_default_double(settings, "margin_x", 56.0);
	obs_data_set_default_double(settings, "margin_y", 72.0);
	obs_data_set_default_double(settings, "corner_radius", APPLE_LT_DEFAULT_CORNER);
	obs_data_set_default_int(settings, "position", APPLE_LT_BOTTOM_LEFT);
	obs_data_set_default_int(settings, "style_preset", APPLE_LT_STYLE_RESET);
	/* Near-white opaque frosted (ABGR) */
	obs_data_set_default_int(settings, "bar_color", APPLE_LT_DEFAULT_BAR_COLOR);
	obs_data_set_default_bool(settings, "enable_accent", true);
	obs_data_set_default_double(settings, "accent_width", APPLE_LT_DEFAULT_ACCENT_W);
	/* Accent blue #0A84FF — full alpha, verified ABGR for vec4_from_rgba */
	obs_data_set_default_int(settings, "accent_color", APPLE_LT_DEFAULT_ACCENT_COLOR);
	obs_data_set_default_string(settings, "title", "Guest Name");
	obs_data_set_default_string(settings, "subtitle", "Title / Role");
}

static obs_properties_t *apple_lt_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *list;

	obs_properties_add_text(props, "info", obs_module_text("AppleLowerThird.Info"), OBS_TEXT_INFO);

	obs_properties_add_text(props, "title", obs_module_text("LowerThirdTitle"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "subtitle", obs_module_text("LowerThirdSubtitle"), OBS_TEXT_DEFAULT);

	list = obs_properties_add_list(props, "style_preset", obs_module_text("LowerThirdStylePreset"),
				       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.ResetClassicFrost"), APPLE_LT_STYLE_RESET);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.ClassicFrost"), APPLE_LT_STYLE_CLASSIC_FROST);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.Midnight"), APPLE_LT_STYLE_MIDNIGHT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.CleanLight"), APPLE_LT_STYLE_CLEAN_LIGHT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.BoldBroadcast"),
				  APPLE_LT_STYLE_BOLD_BROADCAST);
	obs_property_list_add_int(list, obs_module_text("LowerThirdStyle.SoftViolet"), APPLE_LT_STYLE_SOFT_VIOLET);
	obs_property_set_modified_callback(list, apple_lt_style_modified);

	p = obs_properties_add_int(props, "width", obs_module_text("CanvasWidth"), 64, 7680, 1);
	obs_property_int_set_suffix(p, " px");
	p = obs_properties_add_int(props, "height", obs_module_text("CanvasHeight"), 64, 4320, 1);
	obs_property_int_set_suffix(p, " px");

	p = obs_properties_add_float_slider(props, "bar_width_percent", obs_module_text("LowerThirdWidth"), 10.0, 90.0,
					    0.5);
	obs_property_float_set_suffix(p, " %");

	p = obs_properties_add_float_slider(props, "bar_height_px", obs_module_text("LowerThirdHeight"), 40.0, 280.0,
					    1.0);
	obs_property_float_set_suffix(p, " px");

	p = obs_properties_add_float_slider(props, "margin_x", obs_module_text("LowerThirdMarginX"), 0.0, 200.0, 1.0);
	obs_property_float_set_suffix(p, " px");
	p = obs_properties_add_float_slider(props, "margin_y", obs_module_text("LowerThirdMarginY"), 0.0, 200.0, 1.0);
	obs_property_float_set_suffix(p, " px");

	p = obs_properties_add_float_slider(props, "corner_radius", obs_module_text("CornerRadius"), 0.0, 48.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	list = obs_properties_add_list(props, "position", obs_module_text("LowerThirdPosition"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdPosition.BottomLeft"), APPLE_LT_BOTTOM_LEFT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdPosition.BottomRight"), APPLE_LT_BOTTOM_RIGHT);
	obs_property_list_add_int(list, obs_module_text("LowerThirdPosition.BottomCenter"), APPLE_LT_BOTTOM_CENTER);

	obs_properties_add_color_alpha(props, "bar_color", obs_module_text("LowerThirdBarColor"));
	obs_properties_add_bool(props, "enable_accent", obs_module_text("LowerThirdEnableAccent"));
	p = obs_properties_add_float_slider(props, "accent_width", obs_module_text("LowerThirdAccentWidth"), 4.0, 24.0,
					    0.5);
	obs_property_float_set_suffix(p, " px");
	obs_properties_add_color_alpha(props, "accent_color", obs_module_text("LowerThirdAccentColor"));

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_lt_compute_bar(struct apple_lower_third_data *ctx, float *out_x, float *out_y, float *out_w,
				 float *out_h)
{
	float w = (float)ctx->cx * (ctx->width_percent / 100.0f);
	float h = ctx->height_px;
	float x;
	float y;

	w = fmaxf(w, 40.0f);
	h = fmaxf(h, 24.0f);

	y = (float)ctx->cy - h - ctx->margin_y;

	switch (ctx->position) {
	case APPLE_LT_BOTTOM_RIGHT:
		x = (float)ctx->cx - w - ctx->margin_x;
		break;
	case APPLE_LT_BOTTOM_CENTER:
		x = ((float)ctx->cx - w) * 0.5f;
		break;
	case APPLE_LT_BOTTOM_LEFT:
	default:
		x = ctx->margin_x;
		break;
	}

	*out_x = x;
	*out_y = y;
	*out_w = w;
	*out_h = h;
}

static void apple_lt_render_text(obs_source_t *text_src, float x, float y)
{
	uint32_t tw;
	uint32_t th;

	if (!text_src)
		return;

	tw = obs_source_get_width(text_src);
	th = obs_source_get_height(text_src);
	if (tw == 0 || th == 0)
		return;

	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);
	obs_source_video_render(text_src);
	gs_matrix_pop();
}

static void apple_lt_render(void *data, gs_effect_t *effect)
{
	struct apple_lower_third_data *ctx = data;
	struct vec2 resolution;
	struct vec2 origin;
	struct vec2 size;
	float x, y, w, h;
	float accent_w;
	float text_x;
	float title_y;
	float subtitle_y;
	uint32_t title_h = 0;
	uint32_t subtitle_h = 0;
	float content_h;
	float text_block_top;

	UNUSED_PARAMETER(effect);

	if (!ctx->effect)
		return;

	apple_lt_compute_bar(ctx, &x, &y, &w, &h);
	vec2_set(&resolution, (float)ctx->cx, (float)ctx->cy);
	vec2_set(&origin, x, y);
	vec2_set(&size, w, h);
	accent_w = ctx->enable_accent ? ctx->accent_width : 0.0f;

	if (ctx->param_resolution)
		gs_effect_set_vec2(ctx->param_resolution, &resolution);
	if (ctx->param_bar_color)
		gs_effect_set_vec4(ctx->param_bar_color, &ctx->bar_color);
	if (ctx->param_accent_color)
		gs_effect_set_vec4(ctx->param_accent_color, &ctx->accent_color);
	if (ctx->param_bar_origin)
		gs_effect_set_vec2(ctx->param_bar_origin, &origin);
	if (ctx->param_bar_size)
		gs_effect_set_vec2(ctx->param_bar_size, &size);
	if (ctx->param_corner_radius)
		gs_effect_set_float(ctx->param_corner_radius, ctx->corner_radius);
	if (ctx->param_accent_width)
		gs_effect_set_float(ctx->param_accent_width, accent_w);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	while (gs_effect_loop(ctx->effect, "Draw"))
		gs_draw_sprite(NULL, 0, ctx->cx, ctx->cy);

	/* Title + subtitle inside the bar (after chrome) */
	if (ctx->title_source || ctx->subtitle_source) {
		if (ctx->title_source)
			title_h = obs_source_get_height(ctx->title_source);
		if (ctx->subtitle_source)
			subtitle_h = obs_source_get_height(ctx->subtitle_source);

		content_h = (float)title_h;
		if (subtitle_h > 0) {
			if (title_h > 0)
				content_h += 6.0f;
			content_h += (float)subtitle_h;
		}

		text_block_top = y + (h - content_h) * 0.5f;
		if (text_block_top < y + APPLE_LT_TEXT_PAD_Y)
			text_block_top = y + APPLE_LT_TEXT_PAD_Y;

		text_x = x + accent_w + APPLE_LT_TEXT_PAD_X + 4.0f;
		title_y = text_block_top;
		subtitle_y = title_y + (float)title_h + (title_h > 0 && subtitle_h > 0 ? 6.0f : 0.0f);

		apple_lt_render_text(ctx->title_source, text_x, title_y);
		apple_lt_render_text(ctx->subtitle_source, text_x, subtitle_y);
	}

	gs_blend_state_pop();
}

static enum gs_color_space apple_lt_get_color_space(void *data, size_t count,
						    const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);
	return GS_CS_SRGB;
}

struct obs_source_info apple_lower_third_source = {
	.id = "apple_lower_third_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = apple_lt_get_name,
	.create = apple_lt_create,
	.destroy = apple_lt_destroy,
	.update = apple_lt_update,
	.video_render = apple_lt_render,
	.get_width = apple_lt_width,
	.get_height = apple_lt_height,
	.get_defaults = apple_lt_defaults,
	.get_properties = apple_lt_properties,
	.video_get_color_space = apple_lt_get_color_space,
	.icon_type = OBS_ICON_TYPE_IMAGE,
};
