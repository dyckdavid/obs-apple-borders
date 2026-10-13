/*
 * Apple Vignette filter — soft circular / elliptical vignette.
 * MIT License
 */

#include "apple-vignette-filter.h"

#include <graphics/vec2.h>

#include "apple-effect.h"
#include "apple-filter-util.h"

struct apple_vignette_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *param_image;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_intensity;
	gs_eparam_t *param_softness;
	gs_eparam_t *param_roundness;
	gs_eparam_t *param_opacity;

	float intensity;
	float softness;
	float roundness;
	float opacity;
};

static const char *apple_vignette_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleVignetteFilter");
}

static void apple_vignette_update(void *data, obs_data_t *settings)
{
	struct apple_vignette_data *filter = data;

	filter->intensity = (float)obs_data_get_double(settings, "intensity");
	filter->softness = (float)obs_data_get_double(settings, "softness");
	filter->roundness = (float)obs_data_get_double(settings, "roundness");
	filter->opacity = (float)obs_data_get_double(settings, "opacity");
}

static void apple_vignette_destroy(void *data)
{
	struct apple_vignette_data *filter = data;

	apple_effect_destroy(&filter->effect);
	bfree(filter);
}

static void *apple_vignette_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_vignette_data *filter = bzalloc(sizeof(*filter));

	filter->context = context;

	if (!apple_effect_load(&filter->effect, "effects/apple_vignette.effect")) {
		apple_vignette_destroy(filter);
		return NULL;
	}

	filter->param_image = gs_effect_get_param_by_name(filter->effect, "image");
	filter->param_resolution = gs_effect_get_param_by_name(filter->effect, "resolution");
	filter->param_intensity = gs_effect_get_param_by_name(filter->effect, "intensity");
	filter->param_softness = gs_effect_get_param_by_name(filter->effect, "softness");
	filter->param_roundness = gs_effect_get_param_by_name(filter->effect, "roundness");
	filter->param_opacity = gs_effect_get_param_by_name(filter->effect, "opacity");

	apple_vignette_update(filter, settings);
	return filter;
}

static void apple_vignette_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "intensity", 0.55);
	obs_data_set_default_double(settings, "softness", 0.45);
	obs_data_set_default_double(settings, "roundness", 0.65);
	obs_data_set_default_double(settings, "opacity", 0.85);
}

static obs_properties_t *apple_vignette_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "info", obs_module_text("AppleVignette.Info"), OBS_TEXT_INFO);

	obs_properties_add_float_slider(props, "intensity", obs_module_text("VignetteIntensity"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "softness", obs_module_text("VignetteSoftness"), 0.05, 1.0, 0.01);
	obs_properties_add_float_slider(props, "roundness", obs_module_text("VignetteRoundness"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "opacity", obs_module_text("VignetteOpacity"), 0.0, 1.0, 0.01);

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_vignette_render(void *data, gs_effect_t *effect)
{
	struct apple_vignette_data *filter = data;
	struct apple_filter_frame frame;
	struct vec2 resolution;

	UNUSED_PARAMETER(effect);

	if (!filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!apple_filter_begin(filter->context, &frame))
		return;

	vec2_set(&resolution, (float)frame.width, (float)frame.height);
	if (filter->param_resolution)
		gs_effect_set_vec2(filter->param_resolution, &resolution);
	if (filter->param_intensity)
		gs_effect_set_float(filter->param_intensity, filter->intensity);
	if (filter->param_softness)
		gs_effect_set_float(filter->param_softness, filter->softness);
	if (filter->param_roundness)
		gs_effect_set_float(filter->param_roundness, filter->roundness);
	if (filter->param_opacity)
		gs_effect_set_float(filter->param_opacity, filter->opacity);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->effect, frame.width, frame.height);

	gs_blend_state_pop();
}

static enum gs_color_space apple_vignette_get_color_space(void *data, size_t count,
							  const enum gs_color_space *preferred_spaces)
{
	struct apple_vignette_data *filter = data;
	return apple_filter_get_color_space(filter->context, count, preferred_spaces);
}

struct obs_source_info apple_vignette_filter = {
	.id = "apple_vignette_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_vignette_get_name,
	.create = apple_vignette_create,
	.destroy = apple_vignette_destroy,
	.update = apple_vignette_update,
	.video_render = apple_vignette_render,
	.get_properties = apple_vignette_properties,
	.get_defaults = apple_vignette_defaults,
	.video_get_color_space = apple_vignette_get_color_space,
};
