/*
 * Shared Apple chrome helpers
 * MIT License
 */

#include "apple-chrome.h"

#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <stdio.h>
#include <string.h>
#include <util/bmem.h>
#include <util/dstr.h>

#include "apple-effect.h"
#include <plugin-support.h>

static void make_key(struct dstr *out, const char *prefix, const char *name)
{
	dstr_copy(out, prefix ? prefix : "");
	if (prefix && *prefix)
		dstr_cat(out, ".");
	dstr_cat(out, name);
}

void apple_chrome_rgba_from_obs_color(struct vec4 *out, uint32_t color)
{
	/* Same path as stock OBS filters (color-source, mask-filter, etc.) */
	vec4_from_rgba(out, color);
}

static void apple_chrome_apply_shadow_preset(obs_data_t *settings, const char *prefix, int preset)
{
	struct dstr enable_key = {0};
	struct dstr blur_key = {0};
	struct dstr opacity_key = {0};
	struct dstr ox_key = {0};
	struct dstr oy_key = {0};

	if (preset == APPLE_SHADOW_CUSTOM)
		return;

	make_key(&enable_key, prefix, "enable_shadow");
	obs_data_set_bool(settings, enable_key.array, preset != APPLE_SHADOW_NONE);

	make_key(&blur_key, prefix, "shadow_blur");
	make_key(&opacity_key, prefix, "shadow_opacity");
	make_key(&ox_key, prefix, "shadow_offset_x");
	make_key(&oy_key, prefix, "shadow_offset_y");

	switch (preset) {
	case APPLE_SHADOW_NONE:
		obs_data_set_double(settings, blur_key.array, 0.0);
		obs_data_set_double(settings, opacity_key.array, 0.0);
		obs_data_set_double(settings, ox_key.array, 0.0);
		obs_data_set_double(settings, oy_key.array, 0.0);
		break;
	case APPLE_SHADOW_SOFT:
		obs_data_set_double(settings, blur_key.array, 20.0);
		obs_data_set_double(settings, opacity_key.array, 0.40);
		obs_data_set_double(settings, ox_key.array, 0.0);
		obs_data_set_double(settings, oy_key.array, 10.0);
		break;
	case APPLE_SHADOW_MEDIUM:
		obs_data_set_double(settings, blur_key.array, 28.0);
		obs_data_set_double(settings, opacity_key.array, 0.50);
		obs_data_set_double(settings, ox_key.array, 0.0);
		obs_data_set_double(settings, oy_key.array, 14.0);
		break;
	case APPLE_SHADOW_DEEP:
		obs_data_set_double(settings, blur_key.array, 40.0);
		obs_data_set_double(settings, opacity_key.array, 0.62);
		obs_data_set_double(settings, ox_key.array, 0.0);
		obs_data_set_double(settings, oy_key.array, 20.0);
		break;
	case APPLE_SHADOW_FLOATING:
		obs_data_set_double(settings, blur_key.array, 48.0);
		obs_data_set_double(settings, opacity_key.array, 0.35);
		obs_data_set_double(settings, ox_key.array, 0.0);
		obs_data_set_double(settings, oy_key.array, 28.0);
		break;
	default:
		break;
	}

	dstr_free(&enable_key);
	dstr_free(&blur_key);
	dstr_free(&opacity_key);
	dstr_free(&ox_key);
	dstr_free(&oy_key);
}

void apple_chrome_settings_set_defaults(obs_data_t *settings, const char *prefix)
{
	struct dstr key = {0};

	make_key(&key, prefix, "corner_radius");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);

	make_key(&key, prefix, "use_per_corner_radius");
	obs_data_set_default_bool(settings, key.array, false);

	make_key(&key, prefix, "corner_radius_tl");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);
	make_key(&key, prefix, "corner_radius_tr");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);
	make_key(&key, prefix, "corner_radius_br");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);
	make_key(&key, prefix, "corner_radius_bl");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_RADIUS);

	make_key(&key, prefix, "border_thickness");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_THICKNESS);

	make_key(&key, prefix, "border_color");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_COLOR);

	make_key(&key, prefix, "border_color2");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_COLOR2);

	make_key(&key, prefix, "enable_border_gradient");
	obs_data_set_default_bool(settings, key.array, false);

	make_key(&key, prefix, "border_gradient_angle");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_GRADIENT_ANGLE);

	make_key(&key, prefix, "enable_border");
	obs_data_set_default_bool(settings, key.array, true);

	make_key(&key, prefix, "border_style");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_STYLE);

	make_key(&key, prefix, "border_softness");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_BORDER_SOFTNESS);

	make_key(&key, prefix, "shadow_preset");
	obs_data_set_default_int(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_PRESET);

	make_key(&key, prefix, "enable_shadow");
	obs_data_set_default_bool(settings, key.array, true);

	make_key(&key, prefix, "shadow_blur");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_BLUR);

	make_key(&key, prefix, "shadow_opacity");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OPACITY);

	make_key(&key, prefix, "shadow_offset_x");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OFFSET_X);

	make_key(&key, prefix, "shadow_offset_y");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_SHADOW_OFFSET_Y);

	make_key(&key, prefix, "content_padding");
	obs_data_set_default_double(settings, key.array, APPLE_CHROME_DEFAULT_PADDING);

	dstr_free(&key);
}

void apple_chrome_settings_load(struct apple_chrome_settings *out, obs_data_t *settings, const char *prefix)
{
	struct dstr key = {0};
	uint32_t color;
	float uniform_radius;

	make_key(&key, prefix, "corner_radius");
	uniform_radius = (float)obs_data_get_double(settings, key.array);
	out->corner_radius = uniform_radius;

	make_key(&key, prefix, "use_per_corner_radius");
	out->use_per_corner_radius = obs_data_get_bool(settings, key.array);

	make_key(&key, prefix, "corner_radius_tl");
	out->corner_radius_tl = (float)obs_data_get_double(settings, key.array);
	make_key(&key, prefix, "corner_radius_tr");
	out->corner_radius_tr = (float)obs_data_get_double(settings, key.array);
	make_key(&key, prefix, "corner_radius_br");
	out->corner_radius_br = (float)obs_data_get_double(settings, key.array);
	make_key(&key, prefix, "corner_radius_bl");
	out->corner_radius_bl = (float)obs_data_get_double(settings, key.array);

	if (!out->use_per_corner_radius) {
		out->corner_radius_tl = uniform_radius;
		out->corner_radius_tr = uniform_radius;
		out->corner_radius_br = uniform_radius;
		out->corner_radius_bl = uniform_radius;
	}

	make_key(&key, prefix, "border_thickness");
	out->border_thickness = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "border_color");
	color = (uint32_t)obs_data_get_int(settings, key.array);
	apple_chrome_rgba_from_obs_color(&out->border_color, color);

	make_key(&key, prefix, "border_color2");
	color = (uint32_t)obs_data_get_int(settings, key.array);
	apple_chrome_rgba_from_obs_color(&out->border_color2, color);

	make_key(&key, prefix, "enable_border_gradient");
	out->enable_border_gradient = obs_data_get_bool(settings, key.array);

	make_key(&key, prefix, "border_gradient_angle");
	out->border_gradient_angle = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "enable_border");
	out->enable_border = obs_data_get_bool(settings, key.array);

	make_key(&key, prefix, "border_style");
	out->border_style = (int)obs_data_get_int(settings, key.array);

	make_key(&key, prefix, "border_softness");
	out->border_softness = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_preset");
	out->shadow_preset = (int)obs_data_get_int(settings, key.array);

	make_key(&key, prefix, "enable_shadow");
	out->enable_shadow = obs_data_get_bool(settings, key.array);

	/* Softness nudge for glow/glass/inner-glow only — never overwrite border_color. */
	if ((out->border_style == APPLE_BORDER_SOFT_GLOW || out->border_style == APPLE_BORDER_GLASS ||
	     out->border_style == APPLE_BORDER_INNER_GLOW) &&
	    out->border_softness < 0.5f)
		out->border_softness = 8.0f;

	if (out->border_style == APPLE_BORDER_THICK && out->border_thickness < 4.0f)
		out->border_thickness = 6.0f;

	if (out->border_style == APPLE_BORDER_NONE)
		out->enable_border = false;

	make_key(&key, prefix, "shadow_blur");
	out->shadow_blur = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_opacity");
	out->shadow_opacity = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_offset_x");
	out->shadow_offset_x = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "shadow_offset_y");
	out->shadow_offset_y = (float)obs_data_get_double(settings, key.array);

	make_key(&key, prefix, "content_padding");
	out->content_padding = (float)obs_data_get_double(settings, key.array);

	dstr_free(&key);
}

static uint32_t apple_chrome_preset_color(int style)
{
	switch (style) {
	case APPLE_BORDER_HAIRLINE_LIGHT:
		return 0xD9FFFFFF;
	case APPLE_BORDER_HAIRLINE_DARK:
		return 0xD9000000;
	case APPLE_BORDER_SOFT_GLOW:
	case APPLE_BORDER_GLASS:
		return 0xB3FFFFFF;
	case APPLE_BORDER_DOUBLE:
		return 0xE6FFFFFF;
	case APPLE_BORDER_INSET:
		return 0x99FFFFFF;
	case APPLE_BORDER_SOLID_LIGHT:
		return 0xFFFFFFFF;
	case APPLE_BORDER_SOLID_DARK:
		return 0xFF1E1C1C; /* ABGR: #1C1C1E */
	case APPLE_BORDER_ACCENT_BLUE:
		return 0xFFFF840A; /* ABGR: #0A84FF */
	case APPLE_BORDER_THICK:
		return 0xE6FFFFFF;
	case APPLE_BORDER_INNER_GLOW:
		return 0x99FFFFFF;
	case APPLE_BORDER_NONE:
	case APPLE_BORDER_CUSTOM:
	default:
		return 0;
	}
}

static bool apple_chrome_border_style_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	char color_key[160];
	char soft_key[160];
	char thick_key[160];
	char enable_key[160];
	const char *style_name = obs_property_name(property);
	const char *dot;
	int style;
	uint32_t preset;
	int prefix_len = 0;
	bool has_prefix = false;

	UNUSED_PARAMETER(props);

	if (!style_name)
		return false;

	style = (int)obs_data_get_int(settings, style_name);
	dot = strrchr(style_name, '.');
	if (dot && dot != style_name) {
		has_prefix = true;
		prefix_len = (int)(dot - style_name);
		snprintf(color_key, sizeof(color_key), "%.*s.border_color", prefix_len, style_name);
		snprintf(soft_key, sizeof(soft_key), "%.*s.border_softness", prefix_len, style_name);
		snprintf(thick_key, sizeof(thick_key), "%.*s.border_thickness", prefix_len, style_name);
		snprintf(enable_key, sizeof(enable_key), "%.*s.enable_border", prefix_len, style_name);
	} else {
		snprintf(color_key, sizeof(color_key), "border_color");
		snprintf(soft_key, sizeof(soft_key), "border_softness");
		snprintf(thick_key, sizeof(thick_key), "border_thickness");
		snprintf(enable_key, sizeof(enable_key), "enable_border");
	}

	UNUSED_PARAMETER(has_prefix);

	if (style == APPLE_BORDER_NONE) {
		obs_data_set_bool(settings, enable_key, false);
		return true;
	}

	obs_data_set_bool(settings, enable_key, true);

	preset = apple_chrome_preset_color(style);
	if (preset != 0)
		obs_data_set_int(settings, color_key, (long long)preset);

	if (style == APPLE_BORDER_SOFT_GLOW || style == APPLE_BORDER_GLASS || style == APPLE_BORDER_INNER_GLOW)
		obs_data_set_double(settings, soft_key, 8.0);
	else
		obs_data_set_double(settings, soft_key, 0.0);

	if (style == APPLE_BORDER_THICK)
		obs_data_set_double(settings, thick_key, 6.0);
	else if (style == APPLE_BORDER_HAIRLINE_LIGHT || style == APPLE_BORDER_HAIRLINE_DARK)
		obs_data_set_double(settings, thick_key, 1.5);
	else if (style == APPLE_BORDER_GLASS || style == APPLE_BORDER_SOFT_GLOW)
		obs_data_set_double(settings, thick_key, 2.5);
	else if (style == APPLE_BORDER_INNER_GLOW)
		obs_data_set_double(settings, thick_key, 8.0);
	else if (style == APPLE_BORDER_DOUBLE)
		obs_data_set_double(settings, thick_key, 4.0);

	return true;
}

static bool apple_chrome_shadow_preset_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	const char *name = obs_property_name(property);
	const char *dot;
	char prefix_buf[128];
	const char *prefix = NULL;
	int preset;

	UNUSED_PARAMETER(props);

	if (!name)
		return false;

	preset = (int)obs_data_get_int(settings, name);
	dot = strrchr(name, '.');
	if (dot && dot != name) {
		int n = (int)(dot - name);
		if (n > 0 && n < (int)sizeof(prefix_buf)) {
			memcpy(prefix_buf, name, (size_t)n);
			prefix_buf[n] = '\0';
			prefix = prefix_buf;
		}
	}

	apple_chrome_apply_shadow_preset(settings, prefix, preset);
	return true;
}

static bool apple_chrome_per_corner_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	const char *name = obs_property_name(property);
	const char *dot;
	char key[160];
	bool per_corner;
	bool visible;

	if (!name)
		return false;

	per_corner = obs_data_get_bool(settings, name);
	dot = strrchr(name, '.');

	const char *names[] = {"corner_radius", "corner_radius_tl", "corner_radius_tr", "corner_radius_br",
			       "corner_radius_bl"};
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		obs_property_t *p;
		if (dot && dot != name) {
			int prefix_len = (int)(dot - name);
			snprintf(key, sizeof(key), "%.*s.%s", prefix_len, name, names[i]);
		} else {
			snprintf(key, sizeof(key), "%s", names[i]);
		}
		p = obs_properties_get(props, key);
		if (!p)
			continue;
		if (i == 0)
			visible = !per_corner;
		else
			visible = per_corner;
		obs_property_set_visible(p, visible);
	}

	return true;
}

void apple_chrome_add_properties(obs_properties_t *props, const char *prefix, const char *group_label)
{
	obs_properties_t *shape = obs_properties_create();
	obs_properties_t *border = obs_properties_create();
	obs_properties_t *shadow = obs_properties_create();
	struct dstr key = {0};
	struct dstr shape_label = {0};
	struct dstr border_label = {0};
	struct dstr shadow_label = {0};
	obs_property_t *p;

	/* Keep the chrome group label as a section prefix when nested under PiP */
	dstr_printf(&shape_label, "%s — %s", group_label, obs_module_text("ChromeShapeGroup"));
	dstr_printf(&border_label, "%s — %s", group_label, obs_module_text("ChromeBorderGroup"));
	dstr_printf(&shadow_label, "%s — %s", group_label, obs_module_text("ChromeShadowGroup"));

	/* ---- Shape ---- */
	make_key(&key, prefix, "use_per_corner_radius");
	p = obs_properties_add_bool(shape, key.array, obs_module_text("UsePerCornerRadius"));
	obs_property_set_modified_callback(p, apple_chrome_per_corner_modified);

	make_key(&key, prefix, "corner_radius");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("CornerRadius"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "corner_radius_tl");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("CornerRadiusTL"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	obs_property_set_visible(p, false);

	make_key(&key, prefix, "corner_radius_tr");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("CornerRadiusTR"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	obs_property_set_visible(p, false);

	make_key(&key, prefix, "corner_radius_br");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("CornerRadiusBR"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	obs_property_set_visible(p, false);

	make_key(&key, prefix, "corner_radius_bl");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("CornerRadiusBL"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	obs_property_set_visible(p, false);

	make_key(&key, prefix, "content_padding");
	p = obs_properties_add_float_slider(shape, key.array, obs_module_text("ContentPadding"), 0.0, 80.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shape_group");
	obs_properties_add_group(props, key.array, shape_label.array, OBS_GROUP_NORMAL, shape);

	/* ---- Border ---- */
	make_key(&key, prefix, "enable_border");
	obs_properties_add_bool(border, key.array, obs_module_text("EnableBorder"));

	make_key(&key, prefix, "border_style");
	p = obs_properties_add_list(border, key.array, obs_module_text("BorderStyle"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.HairlineLight"), APPLE_BORDER_HAIRLINE_LIGHT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.HairlineDark"), APPLE_BORDER_HAIRLINE_DARK);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Thick"), APPLE_BORDER_THICK);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Glass"), APPLE_BORDER_GLASS);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SoftGlow"), APPLE_BORDER_SOFT_GLOW);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Double"), APPLE_BORDER_DOUBLE);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.InnerGlow"), APPLE_BORDER_INNER_GLOW);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Inset"), APPLE_BORDER_INSET);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SolidLight"), APPLE_BORDER_SOLID_LIGHT);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.SolidDark"), APPLE_BORDER_SOLID_DARK);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.AccentBlue"), APPLE_BORDER_ACCENT_BLUE);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.None"), APPLE_BORDER_NONE);
	obs_property_list_add_int(p, obs_module_text("BorderStyle.Custom"), APPLE_BORDER_CUSTOM);
	obs_property_set_modified_callback(p, apple_chrome_border_style_modified);

	make_key(&key, prefix, "border_thickness");
	p = obs_properties_add_float_slider(border, key.array, obs_module_text("BorderThickness"), 0.0, 24.0, 0.25);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "border_color");
	obs_properties_add_color_alpha(border, key.array, obs_module_text("BorderColor"));

	make_key(&key, prefix, "enable_border_gradient");
	obs_properties_add_bool(border, key.array, obs_module_text("EnableBorderGradient"));

	make_key(&key, prefix, "border_color2");
	obs_properties_add_color_alpha(border, key.array, obs_module_text("BorderColor2"));

	make_key(&key, prefix, "border_gradient_angle");
	p = obs_properties_add_float_slider(border, key.array, obs_module_text("BorderGradientAngle"), 0.0, 360.0, 1.0);
	obs_property_float_set_suffix(p, "°");

	make_key(&key, prefix, "border_softness");
	p = obs_properties_add_float_slider(border, key.array, obs_module_text("BorderSoftness"), 0.0, 24.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "border_group");
	obs_properties_add_group(props, key.array, border_label.array, OBS_GROUP_NORMAL, border);

	/* ---- Shadow ---- */
	make_key(&key, prefix, "shadow_preset");
	p = obs_properties_add_list(shadow, key.array, obs_module_text("ShadowPreset"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.None"), APPLE_SHADOW_NONE);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.Soft"), APPLE_SHADOW_SOFT);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.Medium"), APPLE_SHADOW_MEDIUM);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.Deep"), APPLE_SHADOW_DEEP);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.Floating"), APPLE_SHADOW_FLOATING);
	obs_property_list_add_int(p, obs_module_text("ShadowPreset.Custom"), APPLE_SHADOW_CUSTOM);
	obs_property_set_modified_callback(p, apple_chrome_shadow_preset_modified);

	make_key(&key, prefix, "enable_shadow");
	obs_properties_add_bool(shadow, key.array, obs_module_text("EnableShadow"));

	make_key(&key, prefix, "shadow_blur");
	p = obs_properties_add_float_slider(shadow, key.array, obs_module_text("ShadowBlur"), 0.0, 60.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shadow_opacity");
	obs_properties_add_float_slider(shadow, key.array, obs_module_text("ShadowOpacity"), 0.0, 1.0, 0.01);

	make_key(&key, prefix, "shadow_offset_x");
	p = obs_properties_add_float_slider(shadow, key.array, obs_module_text("ShadowOffsetX"), -40.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shadow_offset_y");
	p = obs_properties_add_float_slider(shadow, key.array, obs_module_text("ShadowOffsetY"), -40.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " px");

	make_key(&key, prefix, "shadow_group");
	obs_properties_add_group(props, key.array, shadow_label.array, OBS_GROUP_NORMAL, shadow);

	dstr_free(&key);
	dstr_free(&shape_label);
	dstr_free(&border_label);
	dstr_free(&shadow_label);
}

bool apple_chrome_effect_load(struct apple_chrome_effect *fx)
{
	if (!fx)
		return false;

	memset(fx, 0, sizeof(*fx));

	if (!apple_effect_load(&fx->effect, "effects/apple_chrome.effect"))
		return false;

	fx->param_image = gs_effect_get_param_by_name(fx->effect, "image");
	fx->param_resolution = gs_effect_get_param_by_name(fx->effect, "resolution");
	fx->param_corner_radii = gs_effect_get_param_by_name(fx->effect, "corner_radii");
	fx->param_border_thickness = gs_effect_get_param_by_name(fx->effect, "border_thickness");
	fx->param_border_color = gs_effect_get_param_by_name(fx->effect, "border_color");
	fx->param_border_color2 = gs_effect_get_param_by_name(fx->effect, "border_color2");
	fx->param_border_gradient_angle = gs_effect_get_param_by_name(fx->effect, "border_gradient_angle");
	fx->param_enable_border_gradient = gs_effect_get_param_by_name(fx->effect, "enable_border_gradient");
	fx->param_shadow_blur = gs_effect_get_param_by_name(fx->effect, "shadow_blur");
	fx->param_shadow_opacity = gs_effect_get_param_by_name(fx->effect, "shadow_opacity");
	fx->param_shadow_offset = gs_effect_get_param_by_name(fx->effect, "shadow_offset");
	fx->param_content_padding = gs_effect_get_param_by_name(fx->effect, "content_padding");
	fx->param_enable_border = gs_effect_get_param_by_name(fx->effect, "enable_border");
	fx->param_enable_shadow = gs_effect_get_param_by_name(fx->effect, "enable_shadow");
	fx->param_border_style = gs_effect_get_param_by_name(fx->effect, "border_style");
	fx->param_border_softness = gs_effect_get_param_by_name(fx->effect, "border_softness");

	return true;
}

void apple_chrome_effect_destroy(struct apple_chrome_effect *fx)
{
	if (!fx)
		return;

	apple_effect_destroy(&fx->effect);
	memset(fx, 0, sizeof(*fx));
}

void apple_chrome_effect_bind(struct apple_chrome_effect *fx, const struct apple_chrome_settings *s, float width,
			      float height)
{
	struct vec2 resolution;
	struct vec2 shadow_offset;
	struct vec4 radii;

	vec2_set(&resolution, width, height);
	vec2_set(&shadow_offset, s->shadow_offset_x, s->shadow_offset_y);
	vec4_set(&radii, s->corner_radius_tl, s->corner_radius_tr, s->corner_radius_br, s->corner_radius_bl);

	if (fx->param_resolution)
		gs_effect_set_vec2(fx->param_resolution, &resolution);
	if (fx->param_corner_radii)
		gs_effect_set_vec4(fx->param_corner_radii, &radii);
	if (fx->param_border_thickness)
		gs_effect_set_float(fx->param_border_thickness, s->border_thickness);
	if (fx->param_border_color)
		gs_effect_set_vec4(fx->param_border_color, &s->border_color);
	if (fx->param_border_color2)
		gs_effect_set_vec4(fx->param_border_color2, &s->border_color2);
	if (fx->param_border_gradient_angle)
		gs_effect_set_float(fx->param_border_gradient_angle, s->border_gradient_angle);
	if (fx->param_enable_border_gradient)
		gs_effect_set_bool(fx->param_enable_border_gradient, s->enable_border_gradient);
	if (fx->param_shadow_blur)
		gs_effect_set_float(fx->param_shadow_blur, s->shadow_blur);
	if (fx->param_shadow_opacity)
		gs_effect_set_float(fx->param_shadow_opacity, s->shadow_opacity);
	if (fx->param_shadow_offset)
		gs_effect_set_vec2(fx->param_shadow_offset, &shadow_offset);
	if (fx->param_content_padding)
		gs_effect_set_float(fx->param_content_padding, s->content_padding);
	if (fx->param_enable_border)
		gs_effect_set_bool(fx->param_enable_border, s->enable_border && s->border_style != APPLE_BORDER_NONE);
	if (fx->param_enable_shadow)
		gs_effect_set_bool(fx->param_enable_shadow, s->enable_shadow);
	if (fx->param_border_style)
		gs_effect_set_float(fx->param_border_style, (float)s->border_style);
	if (fx->param_border_softness)
		gs_effect_set_float(fx->param_border_softness, s->border_softness);
}

void apple_pip_corner_position(enum apple_pip_corner corner, float parent_w, float parent_h, float pip_w, float pip_h,
			       float margin_x, float margin_y, float *out_x, float *out_y)
{
	float x = margin_x;
	float y = margin_y;

	switch (corner) {
	case APPLE_PIP_CORNER_TOP_LEFT:
		x = margin_x;
		y = margin_y;
		break;
	case APPLE_PIP_CORNER_TOP_RIGHT:
		x = parent_w - pip_w - margin_x;
		y = margin_y;
		break;
	case APPLE_PIP_CORNER_BOTTOM_LEFT:
		x = margin_x;
		y = parent_h - pip_h - margin_y;
		break;
	case APPLE_PIP_CORNER_BOTTOM_RIGHT:
	default:
		x = parent_w - pip_w - margin_x;
		y = parent_h - pip_h - margin_y;
		break;
	}

	if (out_x)
		*out_x = x;
	if (out_y)
		*out_y = y;
}
