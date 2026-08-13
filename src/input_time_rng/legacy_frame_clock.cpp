#include "openswd3/input_time_rng/legacy_frame_clock.hpp"

namespace openswd3::input_time_rng {

compat::i32 set_frame_interval(
    compat::u32& interval, const compat::u32 milliseconds
) noexcept {
    interval = milliseconds;
    return 1;
}

compat::i32 clear_frame_interval(compat::u32& interval) noexcept {
    interval = 0U;
    return 1;
}

bool try_accept_frame_milliseconds(
    LegacyFrameClockState& state, const compat::u32 now
) noexcept {
    state.sampled_milliseconds = now;
    const compat::u32 elapsed =
        now - state.previous_accepted_frame_milliseconds;
    if (elapsed < state.frame_interval_milliseconds) {
        return false;
    }

    state.previous_accepted_frame_milliseconds = now;
    return true;
}

void finish_accepted_frame_time(LegacyFrameClockState& state) noexcept {
    const compat::u32 now = state.sampled_milliseconds;
    state.current_frame_milliseconds = now;
    state.frame_delta_milliseconds = now - state.previous_input_milliseconds;
    state.previous_input_milliseconds = now;
}

}  // namespace openswd3::input_time_rng
