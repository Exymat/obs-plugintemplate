#include "pill-source.hpp"

#include "time-format.hpp"

#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <cstring>

namespace {

constexpr int kTimeFontSize = 30;
constexpr int kDayFontSize = 22;
constexpr int kWeatherFontSize = 22;
constexpr int kIconSize = 24;

void bind_effect_param(gs_effect_t *effect, gs_eparam_t **param, const char *name)
{
	if (effect && param) {
		*param = gs_effect_get_param_by_name(effect, name);
	}
}

void draw_texture(gs_texture_t *texture, float x, float y, uint32_t width, uint32_t height, float opacity)
{
	if (!texture || width == 0 || height == 0 || opacity <= 0.001f) {
		return;
	}

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_technique_t *tech = gs_effect_get_technique(effect, "Draw");

	gs_effect_set_texture(image, texture);

	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.f);
	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_draw_sprite(texture, 0, width, height);
	gs_technique_end_pass(tech);
	gs_technique_end(tech);
	gs_matrix_pop();
}

void draw_divider(float x, float y, float height, float opacity)
{
	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	struct vec4 c;
	vec4_set(&c, 1.f, 1.f, 1.f, 0.25f * opacity);

	gs_effect_set_vec4(color, &c);

	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.f);
	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_draw_sprite(nullptr, 0, 1, static_cast<uint32_t>(height));
	gs_technique_end_pass(tech);
	gs_technique_end(tech);
	gs_matrix_pop();
}

void configure_text_layer(SvgTextLayer &layer, int font_size, int render_scale, const std::string &font_family)
{
	layer.font_size = font_size;
	layer.render_scale = render_scale;
	layer.font_family = font_family;
}

void load_pill_effect(pill_source *context)
{
	char *path = obs_module_file("effects/pill_glass.effect");
	if (!path) {
		obs_log(LOG_ERROR, "pill_glass.effect not found");
		return;
	}

	context->pill_effect = gs_effect_create_from_file(path, nullptr);
	bfree(path);

	if (!context->pill_effect) {
		obs_log(LOG_ERROR, "failed to load pill_glass.effect");
		return;
	}

	bind_effect_param(context->pill_effect, &context->pill_size_param, "pill_size");
	bind_effect_param(context->pill_effect, &context->corner_radius_param, "corner_radius");
	bind_effect_param(context->pill_effect, &context->opacity_param, "opacity");
	bind_effect_param(context->pill_effect, &context->caustic_time_param, "caustic_time");
	bind_effect_param(context->pill_effect, &context->caustic_intensity_param, "caustic_intensity");
}

void destroy_graphics(pill_source *context)
{
	if (!context) {
		return;
	}

	context->time_layer.destroy_texture();
	context->day_layer.destroy_texture();
	context->weather_layer.destroy_texture();
	context->icon_layer.destroy_texture();

	if (context->pill_effect) {
		gs_effect_destroy(context->pill_effect);
		context->pill_effect = nullptr;
	}
}

void reload_icon_layer(pill_source *context)
{
	context->icon_layer.destroy_texture();
	context->icon_layer.content.clear();

	if (context->weather_icon_path.empty()) {
		context->icon_layer.mark_dirty();
		return;
	}

	FILE *file = os_fopen(context->weather_icon_path.c_str(), "rb");
	if (!file) {
		context->icon_layer.mark_dirty();
		return;
	}

	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0) {
		fclose(file);
		return;
	}

	std::string data(static_cast<size_t>(size), '\0');
	const size_t read = fread(data.data(), 1, static_cast<size_t>(size), file);
	fclose(file);
	if (read == 0) {
		return;
	}
	data.resize(read);

	const int tex_size = std::max(kIconSize * context->render_scale, 8);
	context->icon_layer.texture = texture_from_svg_string(data, tex_size, tex_size);
	if (context->icon_layer.texture) {
		context->icon_layer.logical_width = kIconSize;
		context->icon_layer.logical_height = kIconSize;
	}
	context->layout_dirty = true;
}

void update_text_content(pill_source *context, bool force_day)
{
	const std::string time_str = format_time_string(context->use_24h, context->locale);
	if (time_str != context->cached_time) {
		context->cached_time = time_str;
		context->time_layer.content = time_str;
		context->time_layer.mark_dirty();
		context->layout_dirty = true;
	}

	const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm local_tm{};
#ifdef _WIN32
	localtime_s(&local_tm, &now);
#else
	localtime_r(&now, &local_tm);
#endif

	if (force_day || local_tm.tm_yday != context->cached_yday) {
		context->cached_yday = local_tm.tm_yday;
		const std::string day_str = format_day_string(context->locale);
		if (day_str != context->cached_day) {
			context->cached_day = day_str;
			context->day_layer.content = day_str;
			context->day_layer.mark_dirty();
			context->layout_dirty = true;
		}
	}

	if (context->weather_layer.content != context->weather_text) {
		context->weather_layer.content = context->weather_text;
		context->weather_layer.mark_dirty();
		context->layout_dirty = true;
	}
}

void upload_text_layers(pill_source *context)
{
	context->time_layer.upload_if_dirty();
	context->day_layer.upload_if_dirty();
	context->weather_layer.upload_if_dirty();

	if (context->layout_dirty) {
		context->layout = compute_pill_layout(context->width, context->height, context->icon_layer,
						      context->time_layer, context->day_layer,
						      context->weather_layer);
		context->layout_dirty = false;
	}
}

} // namespace

static const char *pill_source_get_name(void *)
{
	return obs_module_text("PillWidget");
}

static void pill_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "width", 480);
	obs_data_set_default_int(settings, "height", 72);
	obs_data_set_default_int(settings, "corner_radius", 36);
	obs_data_set_default_int(settings, "interval_sec", 300);
	obs_data_set_default_int(settings, "show_sec", 15);
	obs_data_set_default_int(settings, "enter_ms", 1200);
	obs_data_set_default_int(settings, "exit_ms", 800);
	obs_data_set_default_int(settings, "render_scale", 3);
	obs_data_set_default_bool(settings, "use_24h", true);
	obs_data_set_default_bool(settings, "debug_visible", false);
	obs_data_set_default_double(settings, "caustic_intensity", 0.6);
	obs_data_set_default_string(settings, "locale", "fr-FR");
	obs_data_set_default_string(settings, "font_family", "Segoe UI");
	obs_data_set_default_string(settings, "weather_text", "22°C");
	obs_data_set_default_string(settings, "weather_icon", "");
}

static obs_properties_t *pill_source_properties(void *)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, "width", obs_module_text("PillWidget.Width"), 120, 4096, 1);
	obs_properties_add_int(props, "height", obs_module_text("PillWidget.Height"), 32, 512, 1);
	obs_properties_add_int(props, "corner_radius", obs_module_text("PillWidget.CornerRadius"), 0, 256, 1);

	obs_properties_add_int(props, "interval_sec", obs_module_text("PillWidget.IntervalSec"), 10, 3600, 1);
	obs_properties_add_int(props, "show_sec", obs_module_text("PillWidget.ShowSec"), 1, 300, 1);
	obs_properties_add_int(props, "enter_ms", obs_module_text("PillWidget.EnterMs"), 100, 10000, 50);
	obs_properties_add_int(props, "exit_ms", obs_module_text("PillWidget.ExitMs"), 100, 10000, 50);

	obs_properties_add_int(props, "render_scale", obs_module_text("PillWidget.RenderScale"), 1, 4, 1);
	obs_properties_add_bool(props, "use_24h", obs_module_text("PillWidget.Use24h"));
	obs_properties_add_bool(props, "debug_visible", obs_module_text("PillWidget.DebugVisible"));

	obs_properties_add_float(props, "caustic_intensity", obs_module_text("PillWidget.CausticIntensity"), 0.0,
				 2.0, 0.05);

	obs_properties_add_text(props, "locale", obs_module_text("PillWidget.Locale"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "font_family", obs_module_text("PillWidget.FontFamily"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "weather_text", obs_module_text("PillWidget.WeatherText"), OBS_TEXT_DEFAULT);
	obs_properties_add_path(props, "weather_icon", obs_module_text("PillWidget.WeatherIcon"), OBS_PATH_FILE,
				"SVG (*.svg)", nullptr);

	return props;
}

static void pill_source_update(void *data, obs_data_t *settings)
{
	auto *context = static_cast<pill_source *>(data);

	const uint32_t new_width = static_cast<uint32_t>(obs_data_get_int(settings, "width"));
	const uint32_t new_height = static_cast<uint32_t>(obs_data_get_int(settings, "height"));
	const uint32_t new_radius = static_cast<uint32_t>(obs_data_get_int(settings, "corner_radius"));

	if (new_width != context->width || new_height != context->height) {
		context->layout_dirty = true;
	}
	context->width = std::max(new_width, 1u);
	context->height = std::max(new_height, 1u);
	context->corner_radius = new_radius;

	context->animation.interval_ms =
		static_cast<uint32_t>(obs_data_get_int(settings, "interval_sec")) * 1000u;
	context->animation.show_ms = static_cast<uint32_t>(obs_data_get_int(settings, "show_sec")) * 1000u;
	context->animation.enter_ms = static_cast<uint32_t>(obs_data_get_int(settings, "enter_ms"));
	context->animation.exit_ms = static_cast<uint32_t>(obs_data_get_int(settings, "exit_ms"));

	const int new_scale = static_cast<int>(obs_data_get_int(settings, "render_scale"));
	if (new_scale != context->render_scale) {
		context->time_layer.mark_dirty();
		context->day_layer.mark_dirty();
		context->weather_layer.mark_dirty();
	}
	context->render_scale = std::clamp(new_scale, 1, 4);

	context->use_24h = obs_data_get_bool(settings, "use_24h");
	const bool new_debug = obs_data_get_bool(settings, "debug_visible");
	if (new_debug != context->debug_visible) {
		context->animation.reset_cycle();
	}
	context->debug_visible = new_debug;
	context->animation.debug_visible = new_debug;

	context->caustic_intensity = static_cast<float>(obs_data_get_double(settings, "caustic_intensity"));

	const char *locale = obs_data_get_string(settings, "locale");
	const char *font = obs_data_get_string(settings, "font_family");
	const char *weather = obs_data_get_string(settings, "weather_text");
	const char *icon = obs_data_get_string(settings, "weather_icon");

	if (context->locale != locale) {
		context->locale = locale;
		context->day_layer.mark_dirty();
		context->layout_dirty = true;
	}
	if (context->font_family != font) {
		context->font_family = font;
		context->time_layer.mark_dirty();
		context->day_layer.mark_dirty();
		context->weather_layer.mark_dirty();
		context->layout_dirty = true;
	}
	if (context->weather_text != weather) {
		context->weather_text = weather;
		context->weather_layer.mark_dirty();
		context->layout_dirty = true;
	}
	if (context->weather_icon_path != icon) {
		context->weather_icon_path = icon;
		obs_enter_graphics();
		reload_icon_layer(context);
		obs_leave_graphics();
	}

	configure_text_layer(context->time_layer, kTimeFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->day_layer, kDayFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->weather_layer, kWeatherFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->icon_layer, kIconSize, context->render_scale, context->font_family);

	update_text_content(context, true);
}

static void *pill_source_create(obs_data_t *settings, obs_source_t *source)
{
	auto *context = new pill_source();
	context->source = source;

	configure_text_layer(context->time_layer, kTimeFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->day_layer, kDayFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->weather_layer, kWeatherFontSize, context->render_scale, context->font_family);
	configure_text_layer(context->icon_layer, kIconSize, context->render_scale, context->font_family);

	obs_enter_graphics();
	load_pill_effect(context);
	obs_leave_graphics();

	pill_source_update(context, settings);
	context->last_tick_ns = os_gettime_ns();

	if (context->animation.is_active()) {
		obs_enter_graphics();
		upload_text_layers(context);
		obs_leave_graphics();
	}

	return context;
}

static void pill_source_destroy(void *data)
{
	auto *context = static_cast<pill_source *>(data);
	obs_enter_graphics();
	destroy_graphics(context);
	obs_leave_graphics();
	delete context;
}

static uint32_t pill_source_get_width(void *data)
{
	return static_cast<pill_source *>(data)->width;
}

static uint32_t pill_source_get_height(void *data)
{
	return static_cast<pill_source *>(data)->height;
}

static void pill_source_video_tick(void *data, float seconds)
{
	auto *context = static_cast<pill_source *>(data);
	const uint64_t now = os_gettime_ns();
	const float delta_ms = context->last_tick_ns == 0
				       ? seconds * 1000.f
				       : static_cast<float>((now - context->last_tick_ns) / 1000000.0);
	context->last_tick_ns = now;

	const auto prev_state = context->animation.state;
	context->animation.tick(delta_ms);

	const bool entering_visible =
		prev_state == PillAnimState::Hidden && context->animation.state == PillAnimState::Entering;
	const bool active = context->animation.is_active();

	if (active) {
		update_text_content(context, entering_visible);
		obs_enter_graphics();
		upload_text_layers(context);
		obs_leave_graphics();
	}
}

static void pill_source_video_render(void *data, gs_effect_t *)
{
	auto *context = static_cast<pill_source *>(data);
	const float opacity = context->animation.is_active() ? context->animation.opacity : 0.f;

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_matrix_push();
	gs_matrix_translate3f(context->animation.slide_offset, 0.f, 0.f);

	if (context->pill_effect) {
		gs_technique_t *tech = gs_effect_get_technique(context->pill_effect, "Draw");
		struct vec2 pill_size;
		vec2_set(&pill_size, static_cast<float>(context->width), static_cast<float>(context->height));

		const float t = static_cast<float>(os_gettime_ns() & 0xFFFFFFFF) / 1e9f;

		gs_effect_set_vec2(context->pill_size_param, &pill_size);
		gs_effect_set_float(context->corner_radius_param, static_cast<float>(context->corner_radius));
		gs_effect_set_float(context->opacity_param, opacity);
		gs_effect_set_float(context->caustic_time_param, t);
		gs_effect_set_float(context->caustic_intensity_param, context->caustic_intensity);

		gs_technique_begin(tech);
		gs_technique_begin_pass(tech, 0);
		gs_draw_sprite(nullptr, 0, context->width, context->height);
		gs_technique_end_pass(tech);
		gs_technique_end(tech);
	}

	if (opacity > 0.001f) {
		const float divider_h = static_cast<float>(context->height) * 0.5f;
		const float divider_y = (static_cast<float>(context->height) - divider_h) * 0.5f;

		if (context->layout.has_icon) {
			draw_texture(context->icon_layer.texture, context->layout.icon_x, context->layout.content_y,
				     context->icon_layer.logical_width, context->icon_layer.logical_height, opacity);
		}

		draw_texture(context->time_layer.texture, context->layout.time_x, context->layout.content_y,
			     context->time_layer.logical_width, context->time_layer.logical_height, opacity);

		draw_divider(context->layout.divider1_x, divider_y, divider_h, opacity);

		draw_texture(context->day_layer.texture, context->layout.day_x, context->layout.content_y,
			     context->day_layer.logical_width, context->day_layer.logical_height, opacity);

		draw_divider(context->layout.divider2_x, divider_y, divider_h, opacity);

		draw_texture(context->weather_layer.texture, context->layout.weather_x, context->layout.content_y,
			     context->weather_layer.logical_width, context->weather_layer.logical_height, opacity);
	}

	gs_matrix_pop();
	gs_blend_state_pop();
	gs_enable_framebuffer_srgb(previous);
}

obs_source_info pill_widget_source_info = {
	.id = "pill_widget_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = pill_source_get_name,
	.create = pill_source_create,
	.destroy = pill_source_destroy,
	.get_width = pill_source_get_width,
	.get_height = pill_source_get_height,
	.get_defaults = pill_source_defaults,
	.get_properties = pill_source_properties,
	.update = pill_source_update,
	.video_tick = pill_source_video_tick,
	.video_render = pill_source_video_render,
	.icon_type = OBS_ICON_TYPE_TEXT,
};
