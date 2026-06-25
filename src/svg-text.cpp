#include "svg-text.hpp"

#include <lunasvg.h>

#include <obs-module.h>
#include <util/platform.h>

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <vector>

std::string build_text_svg(const std::string &text, int font_size, const std::string &font_family,
			   uint32_t color_rgba, int padding)
{
	const uint8_t r = (color_rgba >> 16) & 0xFF;
	const uint8_t g = (color_rgba >> 8) & 0xFF;
	const uint8_t b = color_rgba & 0xFF;
	const float alpha = ((color_rgba >> 24) & 0xFF) / 255.f;

	std::ostringstream escaped;
	for (char c : text) {
		switch (c) {
		case '&':
			escaped << "&amp;";
			break;
		case '<':
			escaped << "&lt;";
			break;
		case '>':
			escaped << "&gt;";
			break;
		case '"':
			escaped << "&quot;";
			break;
		case '\'':
			escaped << "&apos;";
			break;
		default:
			escaped << c;
			break;
		}
	}

	const int est_width = std::max(static_cast<int>(text.size()) * font_size * 3 / 5 + padding * 2, font_size * 2);
	const int est_height = font_size + padding * 2;

	std::ostringstream svg;
	svg << R"(<?xml version="1.0" encoding="UTF-8"?>)"
	    << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << est_width << R"(" height=")" << est_height
	    << R"(" viewBox="0 0 )" << est_width << " " << est_height << R"(">)"
	    << R"(<text x=")" << padding << R"(" y=")" << (padding + static_cast<int>(font_size * 0.82f))
	    << R"(" font-family=")" << font_family << R"(, sans-serif" font-size=")" << font_size
	    << "\" font-weight=\"600\" fill=\"rgb(" << static_cast<int>(r) << "," << static_cast<int>(g) << ","
	    << static_cast<int>(b) << ")\" fill-opacity=\"" << alpha << "\">" << escaped.str() << "</text></svg>";
	return svg.str();
}

gs_texture_t *texture_from_svg_string(const std::string &svg, int target_width, int target_height)
{
	auto document = lunasvg::Document::loadFromData(svg);
	if (!document) {
		return nullptr;
	}

	lunasvg::Bitmap bitmap = document->renderToBitmap(target_width, target_height, 0x00000000);
	if (bitmap.isNull()) {
		return nullptr;
	}

	bitmap.convertToRGBA();

	const uint32_t width = static_cast<uint32_t>(bitmap.width());
	const uint32_t height = static_cast<uint32_t>(bitmap.height());
	const uint8_t *data = bitmap.data();
	const uint32_t stride = static_cast<uint32_t>(bitmap.stride());

	std::vector<uint8_t> rows(height * width * 4);
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src = data + y * stride;
		uint8_t *dst = rows.data() + y * width * 4;
		memcpy(dst, src, width * 4);
	}

	const uint8_t *ptr = rows.data();
	return gs_texture_create(width, height, GS_RGBA, 1, &ptr, GS_DYNAMIC);
}

void SvgTextLayer::destroy_texture()
{
	if (texture) {
		gs_texture_destroy(texture);
		texture = nullptr;
	}
	logical_width = 0;
	logical_height = 0;
}

bool SvgTextLayer::rasterize_to_texture()
{
	if (content.empty()) {
		destroy_texture();
		dirty_ = false;
		return false;
	}

	const int scale = std::clamp(render_scale, 1, 4);
	svg_cache = build_text_svg(content, font_size, font_family, color);

	auto document = lunasvg::Document::loadFromData(svg_cache);
	if (!document) {
		destroy_texture();
		dirty_ = false;
		return false;
	}

	const float intrinsic_w = std::max(document->width(), 1.f);
	const float intrinsic_h = std::max(document->height(), 1.f);
	logical_width = static_cast<uint32_t>(intrinsic_w);
	logical_height = static_cast<uint32_t>(intrinsic_h);

	const int tex_w = std::max(static_cast<int>(logical_width * static_cast<uint32_t>(scale)), 1);
	const int tex_h = std::max(static_cast<int>(logical_height * static_cast<uint32_t>(scale)), 1);

	lunasvg::Bitmap bitmap = document->renderToBitmap(tex_w, tex_h, 0x00000000);
	if (bitmap.isNull()) {
		destroy_texture();
		dirty_ = false;
		return false;
	}

	bitmap.convertToRGBA();

	const uint32_t width = static_cast<uint32_t>(bitmap.width());
	const uint32_t height = static_cast<uint32_t>(bitmap.height());
	const uint8_t *data = bitmap.data();
	const uint32_t stride = static_cast<uint32_t>(bitmap.stride());

	std::vector<uint8_t> rows(height * width * 4);
	for (uint32_t y = 0; y < height; y++) {
		memcpy(rows.data() + y * width * 4, data + y * stride, width * 4);
	}

	destroy_texture();
	const uint8_t *ptr = rows.data();
	texture = gs_texture_create(width, height, GS_RGBA, 1, &ptr, GS_DYNAMIC);
	dirty_ = false;
	return texture != nullptr;
}

bool SvgTextLayer::upload_if_dirty()
{
	if (!dirty_) {
		return texture != nullptr;
	}
	return rasterize_to_texture();
}
