/*
 * Shared GS effect load/destroy helpers
 * MIT License
 */

#include "apple-effect.h"

#include <obs-module.h>
#include <util/bmem.h>

#include <plugin-support.h>

bool apple_effect_load(gs_effect_t **out_effect, const char *relative_path)
{
	char *path;
	gs_effect_t *effect = NULL;

	if (!out_effect || !relative_path || !*relative_path)
		return false;

	*out_effect = NULL;

	path = obs_module_file(relative_path);
	if (!path) {
		obs_log(LOG_ERROR, "could not resolve %s", relative_path);
		return false;
	}

	obs_enter_graphics();
	effect = gs_effect_create_from_file(path, NULL);
	obs_leave_graphics();
	bfree(path);

	if (!effect) {
		obs_log(LOG_ERROR, "failed to load %s", relative_path);
		return false;
	}

	*out_effect = effect;
	return true;
}

void apple_effect_destroy(gs_effect_t **effect)
{
	if (!effect || !*effect)
		return;

	obs_enter_graphics();
	gs_effect_destroy(*effect);
	obs_leave_graphics();
	*effect = NULL;
}
