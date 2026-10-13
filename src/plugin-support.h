/*
 * OBS Apple Borders — shared logging helpers
 * MIT License
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char *PLUGIN_NAME;
extern const char *PLUGIN_VERSION;

void apple_borders_log(int log_level, const char *format, ...);

#ifdef __cplusplus
}
#endif
