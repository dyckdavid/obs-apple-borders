/*
 * Shared Apple chrome helpers (defaults, color conversion, effect binding)
 * MIT License
 */

#pragma once

#include <graphics/vec4.h>
#include <obs-module.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default look: Continuity Camera / modern macOS window chrome */
#define APPLE_CHROME_DEFAULT_RADIUS 24.0
#define APPLE_CHROME_DEFAULT_BORDER_THICKNESS 2.0
#define APPLE_CHROME_DEFAULT_BORDER_COLOR 0xB3FFFFFF /* ABGR visible white hairline (~70% a) */
#define APPLE_CHROME_DEFAULT_BORDER_STYLE 0 /* hairline light */
#define APPLE_CHROME_DEFAULT_BORDER_SOFTNESS 6.0
#define APPLE_CHROME_DEFAULT_SHADOW_BLUR 20.0
#define APPLE_CHROME_DEFAULT_SHADOW_OPACITY 0.40
#define APPLE_CHROME_DEFAULT_SHADOW_OFFSET_X 0.0
#define APPLE_CHROME_DEFAULT_SHADOW_OFFSET_Y 10.0
#define APPLE_CHROME_DEFAULT_PADDING 32.0

enum apple_pip_corner {
	APPLE_PIP_CORNER_TOP_LEFT = 0,
	APPLE_PIP_CORNER_TOP_RIGHT = 1,
	APPLE_PIP_CORNER_BOTTOM_LEFT = 2,
	APPLE_PIP_CORNER_BOTTOM_RIGHT = 3,
};

enum apple_border_style {
	APPLE_BORDER_HAIRLINE_LIGHT = 0,
	APPLE_BORDER_HAIRLINE_DARK = 1,
	APPLE_BORDER_SOFT_GLOW = 2,
	APPLE_BORDER_DOUBLE = 3,
	APPLE_BORDER_INSET = 4,
	APPLE_BORDER_SOLID_LIGHT = 5,
	APPLE_BORDER_SOLID_DARK = 6,
	APPLE_BORDER_ACCENT_BLUE = 7,
	APPLE_BORDER_CUSTOM = 8,
};

struct apple_chrome_settings {
	float corner_radius;
	float border_thickness;
	struct vec4 border_color;
	float shadow_blur;
	float shadow_opacity;
	float shadow_offset_x;
	float shadow_offset_y;
	float content_padding;
	float border_softness;
	int border_style;
	bool enable_border;
	bool enable_shadow;
};

struct apple_chrome_effect {
	gs_effect_t *effect;
	gs_eparam_t *param_image;
	gs_eparam_t *param_resolution;
	gs_eparam_t *param_corner_radius;
	gs_eparam_t *param_border_thickness;
	gs_eparam_t *param_border_color;
	gs_eparam_t *param_shadow_blur;
	gs_eparam_t *param_shadow_opacity;
	gs_eparam_t *param_shadow_offset;
	gs_eparam_t *param_content_padding;
	gs_eparam_t *param_enable_border;
	gs_eparam_t *param_enable_shadow;
	gs_eparam_t *param_border_style;
	gs_eparam_t *param_border_softness;
};

void apple_chrome_settings_set_defaults(obs_data_t *settings,
					const char *prefix);
void apple_chrome_settings_load(struct apple_chrome_settings *out,
				obs_data_t *settings, const char *prefix);
void apple_chrome_add_properties(obs_properties_t *props, const char *prefix,
				 const char *group_label);

bool apple_chrome_effect_load(struct apple_chrome_effect *fx);
void apple_chrome_effect_destroy(struct apple_chrome_effect *fx);
void apple_chrome_effect_bind(struct apple_chrome_effect *fx,
			      const struct apple_chrome_settings *s,
			      float width, float height);

void apple_chrome_rgba_from_obs_color(struct vec4 *out, uint32_t color);
void apple_pip_corner_position(enum apple_pip_corner corner, float parent_w,
			       float parent_h, float pip_w, float pip_h,
			       float margin_x, float margin_y, float *out_x,
			       float *out_y);

#ifdef __cplusplus
}
#endif
