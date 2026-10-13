/*
 * Apple Border filter — rounded corners, hairline stroke, soft drop shadow.
 * MIT License
 */

#include "apple-border-filter.h"

#include "apple-chrome-util.h"
#include "plugin-support.h"

struct apple_border_data {
	obs_source_t *context;
	struct apple_chrome_effect chrome;
	struct apple_chrome_settings settings;
};

static const char *apple_border_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleBorderFilter");
}

static void apple_border_update(void *data, obs_data_t *settings)
{
	struct apple_border_data *filter = data;
	apple_chrome_settings_load(&filter->settings, settings, NULL);
}

static void apple_border_destroy(void *data)
{
	struct apple_border_data *filter = data;
	apple_chrome_effect_destroy(&filter->chrome);
	bfree(filter);
}

static void *apple_border_create(obs_data_t *settings, obs_source_t *context)
{
	struct apple_border_data *filter = bzalloc(sizeof(*filter));
	filter->context = context;

	if (!apple_chrome_effect_load(&filter->chrome)) {
		apple_border_destroy(filter);
		return NULL;
	}

	apple_border_update(filter, settings);
	return filter;
}

static void apple_border_defaults(obs_data_t *settings)
{
	apple_chrome_settings_set_defaults(settings, NULL);
}

static obs_properties_t *apple_border_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "info",
				obs_module_text("AppleBorder.Info"),
				OBS_TEXT_INFO);

	apple_chrome_add_properties(props, NULL,
				    obs_module_text("AppleChromeGroup"));

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_border_render(void *data, gs_effect_t *effect)
{
	struct apple_border_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	uint32_t width;
	uint32_t height;

	UNUSED_PARAMETER(effect);

	if (!target || !filter->chrome.effect) {
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

	apple_chrome_effect_bind(&filter->chrome, &filter->settings,
				 (float)width, (float)height);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->chrome.effect,
				      width, height);

	gs_blend_state_pop();
}

static enum gs_color_space
apple_border_get_color_space(void *data, size_t count,
			     const enum gs_color_space *preferred_spaces)
{
	struct apple_border_data *filter = data;
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

struct obs_source_info apple_border_filter = {
	.id = "apple_border_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_border_get_name,
	.create = apple_border_create,
	.destroy = apple_border_destroy,
	.update = apple_border_update,
	.video_render = apple_border_render,
	.get_properties = apple_border_properties,
	.get_defaults = apple_border_defaults,
	.video_get_color_space = apple_border_get_color_space,
};
