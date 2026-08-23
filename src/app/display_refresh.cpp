#include "openswd3/app/display_refresh.hpp"

namespace openswd3::app {

void configure_display_refresh_clock(
    DisplayRefreshClockState& state,
    const std::uint32_t frames_per_second,
    const std::uint64_t now_nanoseconds
) noexcept {
    state.previous_refresh_nanoseconds = now_nanoseconds;
    state.refresh_interval_nanoseconds = frames_per_second == 0U
        ? 0U
        : kNanosecondsPerSecond / frames_per_second;
}

bool independent_display_refresh_enabled(
    const DisplayRefreshClockState& state
) noexcept {
    return state.refresh_interval_nanoseconds != 0U;
}

bool try_accept_display_refresh(
    DisplayRefreshClockState& state, const std::uint64_t now_nanoseconds
) noexcept {
    if (!independent_display_refresh_enabled(state)) {
        return false;
    }

    const std::uint64_t elapsed =
        now_nanoseconds - state.previous_refresh_nanoseconds;
    if (elapsed < state.refresh_interval_nanoseconds) {
        return false;
    }

    state.previous_refresh_nanoseconds = now_nanoseconds;
    return true;
}

}  // namespace openswd3::app
