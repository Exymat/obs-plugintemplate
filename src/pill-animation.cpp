#include "pill-animation.hpp"

#include <algorithm>

float pill_ease_out_cubic(float t)
{
	const float inv = 1.f - t;
	return 1.f - inv * inv * inv;
}

float pill_ease_in_cubic(float t)
{
	return t * t * t;
}

void PillAnimation::reset_cycle()
{
	state = PillAnimState::Hidden;
	opacity = 0.f;
	slide_offset = -40.f;
	state_elapsed_ms = 0.f;
	hidden_elapsed_ms = 0.f;
}

bool PillAnimation::is_active() const
{
	return debug_visible || state != PillAnimState::Hidden;
}

void PillAnimation::tick(float delta_ms)
{
	if (debug_visible) {
		state = PillAnimState::Visible;
		opacity = 1.f;
		slide_offset = 0.f;
		return;
	}

	switch (state) {
	case PillAnimState::Hidden:
		hidden_elapsed_ms += delta_ms;
		opacity = 0.f;
		slide_offset = -40.f;
		if (hidden_elapsed_ms >= static_cast<float>(interval_ms)) {
			state = PillAnimState::Entering;
			state_elapsed_ms = 0.f;
		}
		break;

	case PillAnimState::Entering: {
		state_elapsed_ms += delta_ms;
		const float duration = static_cast<float>(std::max(enter_ms, 1u));
		const float t = std::min(state_elapsed_ms / duration, 1.f);
		const float eased = pill_ease_out_cubic(t);
		opacity = eased;
		slide_offset = -40.f * (1.f - eased);
		if (t >= 1.f) {
			state = PillAnimState::Visible;
			state_elapsed_ms = 0.f;
			opacity = 1.f;
			slide_offset = 0.f;
		}
		break;
	}

	case PillAnimState::Visible:
		state_elapsed_ms += delta_ms;
		opacity = 1.f;
		slide_offset = 0.f;
		if (state_elapsed_ms >= static_cast<float>(show_ms)) {
			state = PillAnimState::Exiting;
			state_elapsed_ms = 0.f;
		}
		break;

	case PillAnimState::Exiting: {
		state_elapsed_ms += delta_ms;
		const float duration = static_cast<float>(std::max(exit_ms, 1u));
		const float t = std::min(state_elapsed_ms / duration, 1.f);
		const float eased = pill_ease_in_cubic(t);
		opacity = 1.f - eased;
		slide_offset = -60.f * eased;
		if (t >= 1.f) {
			state = PillAnimState::Hidden;
			state_elapsed_ms = 0.f;
			hidden_elapsed_ms = 0.f;
			opacity = 0.f;
			slide_offset = -60.f;
		}
		break;
	}
	}
}
