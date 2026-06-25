#pragma once

#include "svg-text.hpp"

#include <cstdint>
#include <string>

struct PillLayout {
	float icon_x = 0.f;
	float time_x = 0.f;
	float day_x = 0.f;
	float weather_x = 0.f;
	float content_y = 0.f;
	float divider1_x = 0.f;
	float divider2_x = 0.f;
	bool has_icon = false;
};

PillLayout compute_pill_layout(uint32_t pill_width, uint32_t pill_height, const SvgTextLayer &icon,
			       const SvgTextLayer &time, const SvgTextLayer &day,
			       const SvgTextLayer &weather);
