/*
 * Apple Backdrop — bundled streaming / church backgrounds
 * MIT License
 */

#include "apple-backdrop-source.h"

#include <graphics/image-file.h>
#include <util/dstr.h>

#include "plugin-support.h"

struct backdrop_preset {
	const char *id;
	const char *file;
	const char *locale_key;
};

static const struct backdrop_preset PRESETS[] = {
	{"apple_white_cards", "backgrounds/apple-white-cards.jpg",
	 "Backdrop.AppleWhiteCards"},
	{"apple_scattered_boxes", "backgrounds/apple-scattered-boxes.jpg",
	 "Backdrop.AppleScatteredBoxes"},
	{"apple_frost_grid", "backgrounds/apple-frost-grid.jpg",
	 "Backdrop.AppleFrostGrid"},
	{"apple_window_stack", "backgrounds/apple-window-stack.jpg",
	 "Backdrop.AppleWindowStack"},
	{"apple_mist_panels", "backgrounds/apple-mist-panels.jpg",
	 "Backdrop.AppleMistPanels"},
	{"apple_soft_bands", "backgrounds/apple-soft-bands.jpg",
	 "Backdrop.AppleSoftBands"},
};

#define PRESET_COUNT (sizeof(PRESETS) / sizeof(PRESETS[0]))

struct apple_backdrop_data {
	obs_source_t *source;
	gs_image_file4_t image;
	char *preset_id;
	char *loaded_path;
	uint32_t cx;
	uint32_t cy;
	bool image_loaded;
};

static const struct backdrop_preset *find_preset(const char *id)
{
	size_t i;

	if (!id || !*id)
		return &PRESETS[0];

	for (i = 0; i < PRESET_COUNT; i++) {
		if (strcmp(PRESETS[i].id, id) == 0)
			return &PRESETS[i];
	}
	return &PRESETS[0];
}

static void apple_backdrop_unload_image(struct apple_backdrop_data *ctx)
{
	obs_enter_graphics();
	gs_image_file4_free(&ctx->image);
	obs_leave_graphics();
	bfree(ctx->loaded_path);
	ctx->loaded_path = NULL;
	ctx->image_loaded = false;
	ctx->cx = 1920;
	ctx->cy = 1080;
}

static void apple_backdrop_load_image(struct apple_backdrop_data *ctx,
				      const char *preset_id)
{
	const struct backdrop_preset *preset = find_preset(preset_id);
	char *path;

	path = obs_module_file(preset->file);
	if (!path) {
		apple_borders_log(LOG_ERROR, "backdrop missing: %s",
				  preset->file);
		return;
	}

	if (ctx->loaded_path && strcmp(ctx->loaded_path, path) == 0) {
		bfree(path);
		return;
	}

	apple_backdrop_unload_image(ctx);

	gs_image_file4_init(&ctx->image, path, GS_IMAGE_ALPHA_PREMULTIPLY);

	obs_enter_graphics();
	gs_image_file4_init_texture(&ctx->image);
	obs_leave_graphics();

	if (!ctx->image.image3.image2.image.loaded) {
		apple_borders_log(LOG_ERROR, "failed to load backdrop %s", path);
		gs_image_file4_free(&ctx->image);
		bfree(path);
		return;
	}

	ctx->cx = ctx->image.image3.image2.image.cx;
	ctx->cy = ctx->image.image3.image2.image.cy;
	if (ctx->cx == 0)
		ctx->cx = 1920;
	if (ctx->cy == 0)
		ctx->cy = 1080;
	ctx->loaded_path = path;
	ctx->image_loaded = true;
}

static const char *apple_backdrop_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AppleBackdropSource");
}

static void apple_backdrop_update(void *data, obs_data_t *settings)
{
	struct apple_backdrop_data *ctx = data;
	const char *preset = obs_data_get_string(settings, "preset");

	bfree(ctx->preset_id);
	ctx->preset_id = bstrdup(preset ? preset : PRESETS[0].id);
	apple_backdrop_load_image(ctx, ctx->preset_id);
}

static void *apple_backdrop_create(obs_data_t *settings, obs_source_t *source)
{
	struct apple_backdrop_data *ctx = bzalloc(sizeof(*ctx));
	ctx->source = source;
	ctx->cx = 1920;
	ctx->cy = 1080;
	apple_backdrop_update(ctx, settings);
	return ctx;
}

static void apple_backdrop_destroy(void *data)
{
	struct apple_backdrop_data *ctx = data;
	apple_backdrop_unload_image(ctx);
	bfree(ctx->preset_id);
	bfree(ctx);
}

static uint32_t apple_backdrop_width(void *data)
{
	struct apple_backdrop_data *ctx = data;
	return ctx->cx;
}

static uint32_t apple_backdrop_height(void *data)
{
	struct apple_backdrop_data *ctx = data;
	return ctx->cy;
}

static void apple_backdrop_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "preset", PRESETS[0].id);
}

static obs_properties_t *apple_backdrop_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;
	size_t i;

	obs_properties_add_text(props, "info",
				obs_module_text("AppleBackdrop.Info"),
				OBS_TEXT_INFO);

	list = obs_properties_add_list(props, "preset",
				       obs_module_text("BackdropPreset"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	for (i = 0; i < PRESET_COUNT; i++) {
		obs_property_list_add_string(list,
					     obs_module_text(PRESETS[i].locale_key),
					     PRESETS[i].id);
	}

	UNUSED_PARAMETER(data);
	return props;
}

static void apple_backdrop_render(void *data, gs_effect_t *effect)
{
	struct apple_backdrop_data *ctx = data;
	gs_effect_t *eff = effect ? effect : obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_technique_t *tech;
	gs_eparam_t *image_param;
	gs_texture_t *tex;

	if (!ctx->image_loaded)
		return;

	tex = ctx->image.image3.image2.image.texture;
	if (!tex)
		return;

	image_param = gs_effect_get_param_by_name(eff, "image");
	gs_effect_set_texture_srgb(image_param, tex);

	tech = gs_effect_get_technique(eff, "Draw");
	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_draw_sprite(tex, 0, ctx->cx, ctx->cy);
	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

static enum gs_color_space
apple_backdrop_get_color_space(void *data, size_t count,
			       const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);
	return GS_CS_SRGB;
}

struct obs_source_info apple_backdrop_source = {
	.id = "apple_backdrop_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = apple_backdrop_get_name,
	.create = apple_backdrop_create,
	.destroy = apple_backdrop_destroy,
	.update = apple_backdrop_update,
	.video_render = apple_backdrop_render,
	.get_width = apple_backdrop_width,
	.get_height = apple_backdrop_height,
	.get_defaults = apple_backdrop_defaults,
	.get_properties = apple_backdrop_properties,
	.video_get_color_space = apple_backdrop_get_color_space,
	.icon_type = OBS_ICON_TYPE_IMAGE,
};
