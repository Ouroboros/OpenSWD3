#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::input_time_rng {

struct LegacyFrameClockState {
    compat::u32 sampled_seconds{};
    compat::u32 sampled_milliseconds{};
    compat::u32 previous_accepted_frame_milliseconds{};
    compat::u32 frame_interval_milliseconds{};
    compat::u32 current_frame_milliseconds{};
    compat::u32 frame_delta_milliseconds{};
    compat::u32 previous_input_milliseconds{};
};

[[nodiscard]] compat::i32 set_frame_interval(
    compat::u32& interval,
    compat::u32 milliseconds
) noexcept;

[[nodiscard]] compat::i32 clear_frame_interval(
    compat::u32& interval
) noexcept;

[[nodiscard]] bool try_accept_frame_milliseconds(
    LegacyFrameClockState& state,
    compat::u32 now
) noexcept;

void finish_accepted_frame_time(LegacyFrameClockState& state) noexcept;

}  // namespace openswd3::input_time_rng
