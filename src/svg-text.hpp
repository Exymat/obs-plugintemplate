#pragma once

#include <graphics/graphics.h>

#include <cstdint>
#include <string>

struct SvgTextLayer {
	std::string content;
	std::string svg_cache;
	int render_scale = 3;
	int font_size = 28;
	std::string font_family = "Segoe UI";
	uint32_t color = 0xFFFFFFFF;

	uint32_t logical_width = 0;
	uint32_t logical_height = 0;
	gs_texture_t *texture = nullptr;

	void destroy_texture();
	bool upload_if_dirty();
	void mark_dirty() { dirty_ = true; }
	bool is_dirty() const { return dirty_; }

private:
	bool dirty_ = true;

	bool rasterize_to_texture();
};

std::string build_text_svg(const std::string &text, int font_size, const std::string &font_family,
			   uint32_t color_rgba, int padding = 4);

gs_texture_t *texture_from_svg_string(const std::string &svg, int target_width, int target_height);
