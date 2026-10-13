/*
 * Apple Soft Gradient — procedural animated soft mesh / gradient.
 * MIT License
 */

#include "apple-soft-gradient-source.h"

#include <graphics/vec2.h>
#include <graphics/vec4.h>

#include "plugin-support.h"

enum apple_gradient_palette {
	APPLE_GRAD_SILVER_MIST = 0,
	APPLE_GRAD_WARM_PAPER = 1,
	APPLE_GRAD_COOL_MIST = 2,
	APPLE_GRAD_SOFT_SKY = 3,
};

struct apple_soft_gradient_data {
	obs_source_t *source;
	gs_effect_t *effect;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_time;
	gs_eparam_t *param_speed;
	gs_eparam_t *param_intensity;
	gs_eparam_t *param_color_a;
	gs_eparam_t *param_color_b;
	gs_eparam_t *param_color_c;
	gs_eparam_t *param_color_d;
	gs_eparam_t *param_color_e;

	uint32_t cx;
	uint32_t cy;
	float speed;
	float intensity;
	enum apple_gradient_palette palette;
	float time_sec;
};

static void palette_colors(enum apple_gradient_palette p, struct vec4 *a,
			   struct vec4 *b, struct vec4 *c, struct vec4 *d,
			   struct vec4 *e)
{
	switch (p) {
	case APPLE_GRAD_WARM_PAPER:
		/* Cream, peach, soft rose, warm sand — distinct warm hues */
		vec4_set(a, 0.99f, 0.94f, 0.82f, 1.0f); /* cream */
		vec4_set(b, 0.98f, 0.78f, 0.62f, 1.0f); /* peach */
		vec4_set(c, 0.96f, 0.72f, 0.74f, 1.0f); /* soft rose */
		vec4_set(d, 0.92f, 0.80f, 0.58f, 1.0f); /* warm sand */
		vec4_set(e, 0.94f, 0.68f, 0.48f, 1.0f); /* apricot */
		break;
	case APPLE_GRAD_COOL_MIST:
		/* Icy blue, periwinkle, pale teal, soft lavender */
		vec4_set(a, 0.72f, 0.88f, 0.98f, 1.0f); /* icy blue */
		vec4_set(b, 0.70f, 0.74f, 0.96f, 1.0f); /* periwinkle */
		vec4_set(c, 0.68f, 0.92f, 0.90f, 1.0f); /* pale teal */
		vec4_set(d, 0.82f, 0.76f, 0.94f, 1.0f); /* soft lavender */
		vec4_set(e, 0.55f, 0.70f, 0.92f, 1.0f); /* deeper cool */
		break;
	case APPLE_GRAD_SOFT_SKY:
		/* Sky blue, aqua, soft coral dawn, pale gold */
		vec4_set(a, 0.55f, 0.82f, 0.98f, 1.0f); /* sky blue */
		vec4_set(b, 0.45f, 0.90f, 0.92f, 1.0f); /* aqua */
		vec4_set(c, 0.98f, 0.72f, 0.68f, 1.0f); /* soft coral dawn */
		vec4_set(d, 0.96f, 0.88f, 0.58f, 1.0f); /* pale gold */
		vec4_set(e, 0.62f, 0.78f, 0.96f, 1.0f); /* light azure */
		break;
	case APPLE_GRAD_SILVER_MIST:
	default:
		/* Cool silver, soft blue-gray, pearl, slate — elegant but contrasty */
		vec4_set(a, 0.90f, 0.92f, 0.96f, 1.0f); /* cool silver */
		vec4_set(b, 0.68f, 0.76f, 0.88f, 1.0f); /* soft blue-gray */
		vec4_set(c, 0.94f, 0.94f, 0.96f, 1.0f); /* pearl */
		vec4_set(d, 0.52f, 0.58f, 0.68f, 1.0f); /* slate */
		vec4_set(e, 0.78f, 0.84f, 0.92f, 1.0f); /* mist blue */
		break;
	}
}

static const char *apple_sg_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleSoftGradientSource");
}

static void apple_sg_update(void *data, obs_data_t *settings)
{
	struct apple_soft_gradient_data *ctx = data;

	ctx->cx = (uint32_t)obs_data_get_int(settings, "width");
	ctx->cy = (uint32_t)obs_data_get_int(settings, "height");
	if (ctx->cx < 64)
		ctx->cx = 64;
	if (ctx->cy < 64)
		ctx->cy = 64;

	ctx->speed = (float)obs_data_get_double(settings, "speed");
	ctx->intensity = (float)obs_data_get_double(settings, "intensity");
	ctx->palette =
		(enum apple_gradient_palette)obs_data_get_int(settings,
							      "palette");
}

static void apple_sg_destroy(void *data)
{
	struct apple_soft_gradient_data *ctx = data;

	obs_enter_graphics();
	gs_effect_destroy(ctx->effect);
	obs_leave_graphics();
	bfree(ctx);
}

static void *apple_sg_create(obs_data_t *settings, obs_source_t *source)
{
	struct apple_soft_gradient_data *ctx = bzalloc(sizeof(*ctx));
	char *path;

	ctx->source = source;
	ctx->cx = 1920;
	ctx->cy = 1080;
	ctx->time_sec = 0.0f;

	path = obs_module_file("effects/apple_soft_gradient.effect");
	if (!path) {
		apple_borders_log(
			LOG_ERROR,
			"could not resolve effects/apple_soft_gradient.effect");
		bfree(ctx);
		return NULL;
	}

	obs_enter_graphics();
	ctx->effect = gs_effect_create_from_file(path, NULL);
	obs_leave_graphics();
	bfree(path);

	if (!ctx->effect) {
		apple_borders_log(LOG_ERROR,
				  "failed to load apple_soft_gradient.effect");
		bfree(ctx);
		return NULL;
	}

	ctx->param_resolution =
		gs_effect_get_param_by_name(ctx->effect, "resolution");
	ctx->param_time = gs_effect_get_param_by_name(ctx->effect, "time");
	ctx->param_speed = gs_effect_get_param_by_name(ctx->effect, "speed");
	ctx->param_intensity =
		gs_effect_get_param_by_name(ctx->effect, "intensity");
	ctx->param_color_a =
		gs_effect_get_param_by_name(ctx->effect, "color_a");
	ctx->param_color_b =
		gs_effect_get_param_by_name(ctx->effect, "color_b");
	ctx->param_color_c =
		gs_effect_get_param_by_name(ctx->effect, "color_c");
	ctx->param_color_d =
		gs_effect_get_param_by_name(ctx->effect, "color_d");
	ctx->param_color_e =
		gs_effect_get_param_by_name(ctx->effect, "color_e");

	apple_sg_update(ctx, settings);
	return ctx;
}

static uint32_t apple_sg_width(void *data)
{
	struct apple_soft_gradient_data *ctx = data;
	return ctx->cx;
}

static uint32_t apple_sg_height(void *data)
{
	struct apple_soft_gradient_data *ctx = data;
	return ctx->cy;
}

static void apple_sg_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "width", 1920);
	obs_data_set_default_int(settings, "height", 1080);
	obs_data_set_default_int(settings, "palette", APPLE_GRAD_SILVER_MIST);
	obs_data_set_default_double(settings, "speed", 0.55);
	obs_data_set_default_double(settings, "intensity", 1.0);
}

static obs_properties_t *apple_sg_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *list;

	obs_properties_add_text(props, "info",
				obs_module_text("AppleSoftGradient.Info"),
				OBS_TEXT_INFO);

	p = obs_properties_add_int(props, "width",
				   obs_module_text("CanvasWidth"), 64, 7680, 1);
	obs_property_int_set_suffix(p, " px");
	p = obs_properties_add_int(props, "height",
				   obs_module_text("CanvasHeight"), 64, 4320, 1);
	obs_property_int_set_suffix(p, " px");

	list = obs_properties_add_list(props, "palette",
				       obs_module_text("GradientPalette"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list,
				  obs_module_text("GradientPalette.SilverMist"),
				  APPLE_GRAD_SILVER_MIST);
	obs_property_list_add_int(list,
				  obs_module_text("GradientPalette.WarmPaper"),
				  APPLE_GRAD_WARM_PAPER);
	obs_property_list_add_int(list,
				  obs_module_text("GradientPalette.CoolMist"),
				  APPLE_GRAD_COOL_MIST);
	obs_property_list_add_int(list,
				  obs_module_text("GradientPalette.SoftSky"),
				  APPLE_GRAD_SOFT_SKY);

	obs_properties_add_float_slider(props, "speed",
					obs_module_text("GradientSpeed"), 0.0,
					2.0, 0.01);
	obs_properties_add_float_slider(props, "intensity",
					obs_module_text("GradientIntensity"),
					0.0, 1.0, 0.01);

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_sg_tick(void *data, float seconds)
{
	struct apple_soft_gradient_data *ctx = data;
	ctx->time_sec += seconds;
}

static void apple_sg_render(void *data, gs_effect_t *effect)
{
	struct apple_soft_gradient_data *ctx = data;
	struct vec2 resolution;
	struct vec4 ca, cb, cc, cd, ce;

	UNUSED_PARAMETER(effect);

	if (!ctx->effect)
		return;

	vec2_set(&resolution, (float)ctx->cx, (float)ctx->cy);
	palette_colors(ctx->palette, &ca, &cb, &cc, &cd, &ce);

	if (ctx->param_resolution)
		gs_effect_set_vec2(ctx->param_resolution, &resolution);
	if (ctx->param_time)
		gs_effect_set_float(ctx->param_time, ctx->time_sec);
	if (ctx->param_speed)
		gs_effect_set_float(ctx->param_speed, ctx->speed);
	if (ctx->param_intensity)
		gs_effect_set_float(ctx->param_intensity, ctx->intensity);
	if (ctx->param_color_a)
		gs_effect_set_vec4(ctx->param_color_a, &ca);
	if (ctx->param_color_b)
		gs_effect_set_vec4(ctx->param_color_b, &cb);
	if (ctx->param_color_c)
		gs_effect_set_vec4(ctx->param_color_c, &cc);
	if (ctx->param_color_d)
		gs_effect_set_vec4(ctx->param_color_d, &cd);
	if (ctx->param_color_e)
		gs_effect_set_vec4(ctx->param_color_e, &ce);

	while (gs_effect_loop(ctx->effect, "Draw"))
		gs_draw_sprite(NULL, 0, ctx->cx, ctx->cy);
}

static enum gs_color_space
apple_sg_get_color_space(void *data, size_t count,
			 const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);
	return GS_CS_SRGB;
}

struct obs_source_info apple_soft_gradient_source = {
	.id = "apple_soft_gradient_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_SRGB,
	.get_name = apple_sg_get_name,
	.create = apple_sg_create,
	.destroy = apple_sg_destroy,
	.update = apple_sg_update,
	.video_tick = apple_sg_tick,
	.video_render = apple_sg_render,
	.get_width = apple_sg_width,
	.get_height = apple_sg_height,
	.get_defaults = apple_sg_defaults,
	.get_properties = apple_sg_properties,
	.video_get_color_space = apple_sg_get_color_space,
	.icon_type = OBS_ICON_TYPE_IMAGE,
};
