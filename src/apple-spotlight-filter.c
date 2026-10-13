/*
 * Apple Spotlight filter — soft radial brightening (center or offset).
 * MIT License
 */

#include "apple-spotlight-filter.h"

#include <graphics/vec2.h>

#include "plugin-support.h"

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

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();
	bfree(filter);
}

static void *apple_spotlight_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_spotlight_data *filter = bzalloc(sizeof(*filter));
	char *path;

	filter->context = context;

	path = obs_module_file("effects/apple_spotlight.effect");
	if (!path) {
		apple_borders_log(LOG_ERROR,
				  "could not resolve effects/apple_spotlight.effect");
		bfree(filter);
		return NULL;
	}

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(path, NULL);
	obs_leave_graphics();
	bfree(path);

	if (!filter->effect) {
		apple_borders_log(LOG_ERROR,
				  "failed to load apple_spotlight.effect");
		bfree(filter);
		return NULL;
	}

	filter->param_image =
		gs_effect_get_param_by_name(filter->effect, "image");
	filter->param_resolution =
		gs_effect_get_param_by_name(filter->effect, "resolution");
	filter->param_intensity =
		gs_effect_get_param_by_name(filter->effect, "intensity");
	filter->param_radius =
		gs_effect_get_param_by_name(filter->effect, "radius");
	filter->param_softness =
		gs_effect_get_param_by_name(filter->effect, "softness");
	filter->param_offset =
		gs_effect_get_param_by_name(filter->effect, "offset");

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

	obs_properties_add_text(props, "info",
				obs_module_text("AppleSpotlight.Info"),
				OBS_TEXT_INFO);

	obs_properties_add_float_slider(props, "intensity",
					obs_module_text("SpotlightIntensity"),
					0.0, 1.5, 0.01);
	obs_properties_add_float_slider(props, "radius",
					obs_module_text("SpotlightRadius"),
					0.05, 1.0, 0.01);
	obs_properties_add_float_slider(props, "softness",
					obs_module_text("SpotlightSoftness"),
					0.05, 1.0, 0.01);

	p = obs_properties_add_float_slider(props, "offset_x",
					    obs_module_text("SpotlightOffsetX"),
					    -0.5, 0.5, 0.01);
	obs_property_float_set_suffix(p, "");
	p = obs_properties_add_float_slider(props, "offset_y",
					    obs_module_text("SpotlightOffsetY"),
					    -0.5, 0.5, 0.01);
	obs_property_float_set_suffix(p, "");

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_spotlight_render(void *data, gs_effect_t *effect)
{
	struct apple_spotlight_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	uint32_t width;
	uint32_t height;
	struct vec2 resolution;
	struct vec2 offset;

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

	obs_source_process_filter_end(filter->context, filter->effect, width,
				      height);

	gs_blend_state_pop();
}

static enum gs_color_space
apple_spotlight_get_color_space(void *data, size_t count,
				const enum gs_color_space *preferred_spaces)
{
	struct apple_spotlight_data *filter = data;
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
