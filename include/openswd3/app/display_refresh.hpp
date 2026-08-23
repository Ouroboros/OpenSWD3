#pragma once

#include <cstdint>

namespace openswd3::app {

inline constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;

struct DisplayRefreshClockState {
    std::uint64_t previous_refresh_nanoseconds{};
    std::uint64_t refresh_interval_nanoseconds{};
};

void configure_display_refresh_clock(
    DisplayRefreshClockState& state,
    std::uint32_t frames_per_second,
    std::uint64_t now_nanoseconds
) noexcept;

[[nodiscard]] bool independent_display_refresh_enabled(
    const DisplayRefreshClockState& state
) noexcept;

[[nodiscard]] bool try_accept_display_refresh(
    DisplayRefreshClockState& state, std::uint64_t now_nanoseconds
) noexcept;

}  // namespace openswd3::app
