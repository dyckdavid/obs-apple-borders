/*
 * Apple Frost filter — frosted-glass (multi-sample blur + wash).
 * MIT License
 */

#include "apple-frost-filter.h"

#include <graphics/vec2.h>

#include "plugin-support.h"

struct apple_frost_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *param_image;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_blur_amount;
	gs_eparam_t *param_frost_mix;
	gs_eparam_t *param_brightness;

	float blur_amount;
	float frost_mix;
	float brightness;
};

static const char *apple_frost_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleFrostFilter");
}

static void apple_frost_update(void *data, obs_data_t *settings)
{
	struct apple_frost_data *filter = data;

	filter->blur_amount = (float)obs_data_get_double(settings, "blur_amount");
	filter->frost_mix = (float)obs_data_get_double(settings, "frost_mix");
	filter->brightness = (float)obs_data_get_double(settings, "brightness");
}

static void apple_frost_destroy(void *data)
{
	struct apple_frost_data *filter = data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();
	bfree(filter);
}

static void *apple_frost_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_frost_data *filter = bzalloc(sizeof(*filter));
	char *path;

	filter->context = context;

	path = obs_module_file("effects/apple_frost.effect");
	if (!path) {
		apple_borders_log(LOG_ERROR,
				  "could not resolve effects/apple_frost.effect");
		bfree(filter);
		return NULL;
	}

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(path, NULL);
	obs_leave_graphics();
	bfree(path);

	if (!filter->effect) {
		apple_borders_log(LOG_ERROR, "failed to load apple_frost.effect");
		bfree(filter);
		return NULL;
	}

	filter->param_image =
		gs_effect_get_param_by_name(filter->effect, "image");
	filter->param_resolution =
		gs_effect_get_param_by_name(filter->effect, "resolution");
	filter->param_blur_amount =
		gs_effect_get_param_by_name(filter->effect, "blur_amount");
	filter->param_frost_mix =
		gs_effect_get_param_by_name(filter->effect, "frost_mix");
	filter->param_brightness =
		gs_effect_get_param_by_name(filter->effect, "brightness");

	apple_frost_update(filter, settings);
	return filter;
}

static void apple_frost_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "blur_amount", 6.0);
	obs_data_set_default_double(settings, "frost_mix", 0.75);
	obs_data_set_default_double(settings, "brightness", 0.06);
}

static obs_properties_t *apple_frost_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(props, "info",
				obs_module_text("AppleFrost.Info"),
				OBS_TEXT_INFO);

	p = obs_properties_add_float_slider(props, "blur_amount",
					    obs_module_text("FrostBlurAmount"),
					    0.0, 24.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	obs_properties_add_float_slider(props, "frost_mix",
					obs_module_text("FrostMix"), 0.0, 1.0,
					0.01);
	obs_properties_add_float_slider(props, "brightness",
					obs_module_text("FrostBrightness"),
					-0.2, 0.4, 0.01);

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_frost_render(void *data, gs_effect_t *effect)
{
	struct apple_frost_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	uint32_t width;
	uint32_t height;
	struct vec2 resolution;

	UNUSED_PARAMETER(effect);

	if (!target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	width = obs_source_get_base_width(target);
	height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};
	const enum gs_color_space source_space = obs_source_get_color_space(
		target, OBS_COUNTOF(preferred_spaces), preferred_spaces);
	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	if (!obs_source_process_filter_begin_with_color_space(
		    filter->context, format, source_space,
		    OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	vec2_set(&resolution, (float)width, (float)height);
	if (filter->param_resolution)
		gs_effect_set_vec2(filter->param_resolution, &resolution);
	if (filter->param_blur_amount)
		gs_effect_set_float(filter->param_blur_amount,
				    filter->blur_amount);
	if (filter->param_frost_mix)
		gs_effect_set_float(filter->param_frost_mix, filter->frost_mix);
	if (filter->param_brightness)
		gs_effect_set_float(filter->param_brightness,
				    filter->brightness);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->effect, width,
				      height);

	gs_blend_state_pop();
}

static enum gs_color_space
apple_frost_get_color_space(void *data, size_t count,
			    const enum gs_color_space *preferred_spaces)
{
	struct apple_frost_data *filter = data;
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	return obs_source_get_color_space(obs_filter_get_target(filter->context),
					  OBS_COUNTOF(potential_spaces),
					  potential_spaces);
}

struct obs_source_info apple_frost_filter = {
	.id = "apple_frost_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_frost_get_name,
	.create = apple_frost_create,
	.destroy = apple_frost_destroy,
	.update = apple_frost_update,
	.video_render = apple_frost_render,
	.get_properties = apple_frost_properties,
	.get_defaults = apple_frost_defaults,
	.video_get_color_space = apple_frost_get_color_space,
};
