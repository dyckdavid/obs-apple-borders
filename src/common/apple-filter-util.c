/*
 * Shared video-filter render helpers
 * MIT License
 */

#include "apple-filter-util.h"

bool apple_filter_begin(obs_source_t *filter_context, struct apple_filter_frame *out_frame)
{
	static const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	if (!filter_context || !out_frame)
		return false;

	out_frame->target = obs_filter_get_target(filter_context);
	if (!out_frame->target) {
		obs_source_skip_video_filter(filter_context);
		return false;
	}

	out_frame->width = obs_source_get_base_width(out_frame->target);
	out_frame->height = obs_source_get_base_height(out_frame->target);
	if (out_frame->width == 0 || out_frame->height == 0) {
		obs_source_skip_video_filter(filter_context);
		return false;
	}

	out_frame->source_space =
		obs_source_get_color_space(out_frame->target, OBS_COUNTOF(preferred_spaces), preferred_spaces);
	out_frame->format = gs_get_format_from_space(out_frame->source_space);

	if (!obs_source_process_filter_begin_with_color_space(filter_context, out_frame->format,
							      out_frame->source_space, OBS_ALLOW_DIRECT_RENDERING)) {
		return false;
	}

	return true;
}

enum gs_color_space apple_filter_get_color_space(obs_source_t *filter_context, size_t count,
						 const enum gs_color_space *preferred_spaces)
{
	static const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	return obs_source_get_color_space(obs_filter_get_target(filter_context), OBS_COUNTOF(potential_spaces),
					  potential_spaces);
}
