#include "pill-layout.hpp"

#include <algorithm>

PillLayout compute_pill_layout(uint32_t pill_width, uint32_t pill_height, const SvgTextLayer &icon,
			       const SvgTextLayer &time, const SvgTextLayer &day,
			       const SvgTextLayer &weather)
{
	PillLayout layout{};
	const float pad = 16.f;
	const float gap = 12.f;
	const float divider_w = 1.f;

	const bool has_icon = icon.texture != nullptr && icon.logical_width > 0;
	layout.has_icon = has_icon;

	const float icon_w = has_icon ? static_cast<float>(icon.logical_width) : 0.f;
	const float time_w = time.texture ? static_cast<float>(time.logical_width) : 0.f;
	const float day_w = day.texture ? static_cast<float>(day.logical_width) : 0.f;
	const float weather_w = weather.texture ? static_cast<float>(weather.logical_width) : 0.f;

	const float content_h = static_cast<float>(pill_height);
	const float text_h = static_cast<float>(
		std::max({time.logical_height, day.logical_height, weather.logical_height, icon.logical_height, 1u}));
	layout.content_y = (content_h - text_h) * 0.5f;

	float x = pad;
	if (has_icon) {
		layout.icon_x = x;
		x += icon_w + gap;
	}

	layout.time_x = x;
	x += time_w + gap;

	layout.divider1_x = x;
	x += divider_w + gap;

	layout.day_x = x;
	x += day_w + gap;

	layout.divider2_x = x;
	x += divider_w + gap;

	layout.weather_x = x;

	const float used = x + weather_w + pad;
	if (used > static_cast<float>(pill_width)) {
		const float scale = (static_cast<float>(pill_width) - pad * 2.f) / std::max(used - pad * 2.f, 1.f);
		const float offset = pad;
		auto scale_x = [&](float value) { return offset + (value - offset) * scale; };
		layout.icon_x = scale_x(layout.icon_x);
		layout.time_x = scale_x(layout.time_x);
		layout.divider1_x = scale_x(layout.divider1_x);
		layout.day_x = scale_x(layout.day_x);
		layout.divider2_x = scale_x(layout.divider2_x);
		layout.weather_x = scale_x(layout.weather_x);
	}

	return layout;
}
