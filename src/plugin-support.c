/*
 * OBS Apple Borders — shared logging helpers
 * MIT License
 */

#include "plugin-support.h"

#include <obs-module.h>

#include "plugin-version.h"

const char *PLUGIN_NAME = "obs-apple-borders";
const char *PLUGIN_VERSION = PLUGIN_VERSION_STRING;

void apple_borders_log(int log_level, const char *format, ...)
{
	char template_buf[512];
	snprintf(template_buf, sizeof(template_buf), "[%s] %s", PLUGIN_NAME,
		 format);

	va_list args;
	va_start(args, format);
	blogva(log_level, template_buf, args);
	va_end(args);
}
