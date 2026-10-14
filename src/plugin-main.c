/*
 * OBS Apple Borders
 * macOS-styled rounded borders, PiP, backdrops, and polish filters/sources.
 *
 * MIT License
 */

#include <obs-module.h>
#include <plugin-support.h>

#include "apple-backdrop-source.h"
#include "apple-border-filter.h"
#include "apple-frost-filter.h"
#include "apple-lower-third-source.h"
#include "apple-pip-filter.h"
#include "apple-soft-gradient-source.h"
#include "apple-spotlight-filter.h"
#include "apple-vignette-filter.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Apple-styled borders, PiP, backdrops, vignette, spotlight, frost, lower thirds, and soft gradients";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "OBS Apple Borders";
}

bool obs_module_load(void)
{
	obs_register_source(&apple_border_filter);
	obs_register_source(&apple_pip_filter);
	obs_register_source(&apple_backdrop_source);
	obs_register_source(&apple_vignette_filter);
	obs_register_source(&apple_spotlight_filter);
	obs_register_source(&apple_frost_filter);
	obs_register_source(&apple_lower_third_source);
	obs_register_source(&apple_soft_gradient_source);

	obs_log(LOG_INFO,
		"loaded successfully (version %s) — Border, PiP, Backdrop, Vignette, Spotlight, Frost, Lower Third, Soft Gradient ready",
		PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "unloaded");
}
