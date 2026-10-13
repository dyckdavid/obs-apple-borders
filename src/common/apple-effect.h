/*
 * Shared GS effect load/destroy helpers
 * MIT License
 */

#pragma once

#include <graphics/graphics.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool apple_effect_load(gs_effect_t **out_effect, const char *relative_path);
void apple_effect_destroy(gs_effect_t **effect);

#ifdef __cplusplus
}
#endif
