/*
 * Shared video-filter render helpers
 * MIT License
 */

#pragma once

#include <obs-module.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct apple_filter_frame {
	obs_source_t *target;
	uint32_t width;
	uint32_t height;
	enum gs_color_space source_space;
	enum gs_color_format format;
};

/* Begin a standard filter pass; returns false if the frame should be skipped. */
bool apple_filter_begin(obs_source_t *filter_context, struct apple_filter_frame *out_frame);

enum gs_color_space apple_filter_get_color_space(obs_source_t *filter_context, size_t count,
						 const enum gs_color_space *preferred_spaces);

#ifdef __cplusplus
}
#endif
