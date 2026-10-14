/*
 * Apple Border filter — rounded corners, hairline stroke, soft drop shadow.
 * MIT License
 */

#include "apple-border-filter.h"

#include "apple-chrome.h"
#include "apple-filter-util.h"

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

	obs_properties_add_text(props, "info", obs_module_text("AppleBorder.Info"), OBS_TEXT_INFO);

	apple_chrome_add_properties(props, NULL, obs_module_text("AppleChromeGroup"));

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_border_render(void *data, gs_effect_t *effect)
{
	struct apple_border_data *filter = data;
	struct apple_filter_frame frame;

	UNUSED_PARAMETER(effect);

	if (!filter->chrome.effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!apple_filter_begin(filter->context, &frame))
		return;

	apple_chrome_effect_bind(&filter->chrome, &filter->settings, (float)frame.width, (float)frame.height);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->chrome.effect, frame.width, frame.height);

	gs_blend_state_pop();
}

static enum gs_color_space apple_border_get_color_space(void *data, size_t count,
							const enum gs_color_space *preferred_spaces)
{
	struct apple_border_data *filter = data;
	return apple_filter_get_color_space(filter->context, count, preferred_spaces);
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
