/*
 * Apple Spotlight filter — soft radial brightening (center or offset).
 * MIT License
 */

#include "apple-spotlight-filter.h"

#include <graphics/vec2.h>

#include "apple-effect.h"
#include "apple-filter-util.h"

struct apple_spotlight_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *param_image;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_intensity;
	gs_eparam_t *param_radius;
	gs_eparam_t *param_softness;
	gs_eparam_t *param_offset;

	float intensity;
	float radius;
	float softness;
	float offset_x;
	float offset_y;
};

static const char *apple_spotlight_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleSpotlightFilter");
}

static void apple_spotlight_update(void *data, obs_data_t *settings)
{
	struct apple_spotlight_data *filter = data;

	filter->intensity = (float)obs_data_get_double(settings, "intensity");
	filter->radius = (float)obs_data_get_double(settings, "radius");
	filter->softness = (float)obs_data_get_double(settings, "softness");
	filter->offset_x = (float)obs_data_get_double(settings, "offset_x");
	filter->offset_y = (float)obs_data_get_double(settings, "offset_y");
}

static void apple_spotlight_destroy(void *data)
{
	struct apple_spotlight_data *filter = data;

	apple_effect_destroy(&filter->effect);
	bfree(filter);
}

static void *apple_spotlight_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_spotlight_data *filter = bzalloc(sizeof(*filter));

	filter->context = context;

	if (!apple_effect_load(&filter->effect, "effects/apple_spotlight.effect")) {
		apple_spotlight_destroy(filter);
		return NULL;
	}

	filter->param_image = gs_effect_get_param_by_name(filter->effect, "image");
	filter->param_resolution = gs_effect_get_param_by_name(filter->effect, "resolution");
	filter->param_intensity = gs_effect_get_param_by_name(filter->effect, "intensity");
	filter->param_radius = gs_effect_get_param_by_name(filter->effect, "radius");
	filter->param_softness = gs_effect_get_param_by_name(filter->effect, "softness");
	filter->param_offset = gs_effect_get_param_by_name(filter->effect, "offset");

	apple_spotlight_update(filter, settings);
	return filter;
}

static void apple_spotlight_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "intensity", 0.35);
	obs_data_set_default_double(settings, "radius", 0.35);
	obs_data_set_default_double(settings, "softness", 0.40);
	obs_data_set_default_double(settings, "offset_x", 0.0);
	obs_data_set_default_double(settings, "offset_y", 0.0);
}

static obs_properties_t *apple_spotlight_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(props, "info", obs_module_text("AppleSpotlight.Info"), OBS_TEXT_INFO);

	obs_properties_add_float_slider(props, "intensity", obs_module_text("SpotlightIntensity"), 0.0, 1.5, 0.01);
	obs_properties_add_float_slider(props, "radius", obs_module_text("SpotlightRadius"), 0.05, 1.0, 0.01);
	obs_properties_add_float_slider(props, "softness", obs_module_text("SpotlightSoftness"), 0.05, 1.0, 0.01);

	p = obs_properties_add_float_slider(props, "offset_x", obs_module_text("SpotlightOffsetX"), -0.5, 0.5, 0.01);
	obs_property_float_set_suffix(p, "");
	p = obs_properties_add_float_slider(props, "offset_y", obs_module_text("SpotlightOffsetY"), -0.5, 0.5, 0.01);
	obs_property_float_set_suffix(p, "");

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_spotlight_render(void *data, gs_effect_t *effect)
{
	struct apple_spotlight_data *filter = data;
	struct apple_filter_frame frame;
	struct vec2 resolution;
	struct vec2 offset;

	UNUSED_PARAMETER(effect);

	if (!filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!apple_filter_begin(filter->context, &frame))
		return;

	vec2_set(&resolution, (float)frame.width, (float)frame.height);
	vec2_set(&offset, filter->offset_x, filter->offset_y);

	if (filter->param_resolution)
		gs_effect_set_vec2(filter->param_resolution, &resolution);
	if (filter->param_intensity)
		gs_effect_set_float(filter->param_intensity, filter->intensity);
	if (filter->param_radius)
		gs_effect_set_float(filter->param_radius, filter->radius);
	if (filter->param_softness)
		gs_effect_set_float(filter->param_softness, filter->softness);
	if (filter->param_offset)
		gs_effect_set_vec2(filter->param_offset, &offset);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->effect, frame.width, frame.height);

	gs_blend_state_pop();
}

static enum gs_color_space apple_spotlight_get_color_space(void *data, size_t count,
							   const enum gs_color_space *preferred_spaces)
{
	struct apple_spotlight_data *filter = data;
	return apple_filter_get_color_space(filter->context, count, preferred_spaces);
}

struct obs_source_info apple_spotlight_filter = {
	.id = "apple_spotlight_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_spotlight_get_name,
	.create = apple_spotlight_create,
	.destroy = apple_spotlight_destroy,
	.update = apple_spotlight_update,
	.video_render = apple_spotlight_render,
	.get_properties = apple_spotlight_properties,
	.get_defaults = apple_spotlight_defaults,
	.video_get_color_space = apple_spotlight_get_color_space,
};
