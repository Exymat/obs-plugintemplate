#pragma once

#include "pill-animation.hpp"
#include "pill-layout.hpp"
#include "svg-text.hpp"

#include <obs-module.h>

#include <cstdint>
#include <string>

struct pill_source {
	obs_source_t *source = nullptr;

	uint32_t width = 480;
	uint32_t height = 72;
	uint32_t corner_radius = 36;
	int render_scale = 3;
	bool use_24h = true;
	bool debug_visible = false;
	float caustic_intensity = 0.6f;
	std::string locale = "fr-FR";
	std::string font_family = "Segoe UI";
	std::string weather_text = "22°C";
	std::string weather_icon_path;

	PillAnimation animation;
	SvgTextLayer icon_layer;
	SvgTextLayer time_layer;
	SvgTextLayer day_layer;
	SvgTextLayer weather_layer;
	PillLayout layout{};

	std::string cached_time;
	std::string cached_day;
	int cached_yday = -1;

	gs_effect_t *pill_effect = nullptr;
	gs_eparam_t *pill_size_param = nullptr;
	gs_eparam_t *corner_radius_param = nullptr;
	gs_eparam_t *opacity_param = nullptr;
	gs_eparam_t *caustic_time_param = nullptr;
	gs_eparam_t *caustic_intensity_param = nullptr;

	bool layout_dirty = true;
	uint64_t last_tick_ns = 0;
};

extern obs_source_info pill_widget_source_info;
