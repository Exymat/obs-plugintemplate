#pragma once

#include <cstdint>

enum class PillAnimState { Hidden, Entering, Visible, Exiting };

struct PillAnimation {
	PillAnimState state = PillAnimState::Hidden;
	float opacity = 0.f;
	float slide_offset = -40.f;
	float state_elapsed_ms = 0.f;
	float hidden_elapsed_ms = 0.f;

	uint32_t interval_ms = 300000;
	uint32_t show_ms = 15000;
	uint32_t enter_ms = 1200;
	uint32_t exit_ms = 800;
	bool debug_visible = false;

	void reset_cycle();
	void tick(float delta_ms);
	bool is_active() const;
};

float pill_ease_out_cubic(float t);
float pill_ease_in_cubic(float t);
