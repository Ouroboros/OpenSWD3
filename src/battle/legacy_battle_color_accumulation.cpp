#include "openswd3/battle/legacy_battle_color_accumulation.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace openswd3::battle {
namespace {

[[nodiscard]] bool x87_equal_zero_or_unordered(const float value) noexcept {
    return value == 0.0F || std::isnan(value);
}

[[nodiscard]] compat::i32
truncate_x87_float_to_low_i32(const float value) noexcept {
    if (!std::isfinite(value)) {
        return 0;
    }
    const double truncated = std::trunc(static_cast<double>(value));
    constexpr double kMinimum =
        static_cast<double>(std::numeric_limits<std::int64_t>::min());
    constexpr double kMaximum =
        static_cast<double>(std::numeric_limits<std::int64_t>::max());
    if (truncated < kMinimum || truncated >= kMaximum) {
        return 0;
    }
    const auto integer = static_cast<std::int64_t>(truncated);
    return std::bit_cast<compat::i32>(static_cast<compat::u32>(integer));
}

}  // namespace

rendering::LegacyFrameColorTransitionResult
update_legacy_battle_color_accumulation(
    rendering::LegacyFrameColorTransitionState& state,
    const bool decrement_countdown,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& format
) noexcept {
    rendering::LegacyFrameColorTransitionResult result;
    if (x87_equal_zero_or_unordered(state.step_red) &&
        x87_equal_zero_or_unordered(state.step_green) &&
        x87_equal_zero_or_unordered(state.step_blue)) {
        return result;
    }

    if (decrement_countdown && state.countdown >= 0) {
        --state.countdown;
        result.countdown_decremented = true;
    }

    if (state.countdown >= 0) {
        state.current_red = state.current_red + state.step_red;
        state.current_green = state.current_green + state.step_green;
        state.current_blue = state.current_blue + state.step_blue;
        result.current_values_advanced = true;
    } else {
        state.step_red = state.target_red;
        state.step_green = state.target_green;
        state.step_blue = state.target_blue;
        result.steps_replaced_by_targets = true;
    }

    result.applied_blue = truncate_x87_float_to_low_i32(state.current_blue);
    result.applied_green = truncate_x87_float_to_low_i32(state.current_green);
    result.applied_red = truncate_x87_float_to_low_i32(state.current_red);
    result.framebuffer_status = rendering::adjust_legacy_rgb_channels(
        framebuffer.physical_pixels_with_read_guard(),
        kLegacyBattleColorAccumulationPixelCount,
        result.applied_red,
        result.applied_green,
        result.applied_blue,
        format
    );
    if (result.framebuffer_status !=
        rendering::LegacyFrameColorStatus::completed) {
        result.status =
            rendering::LegacyFrameColorTransitionStatus::framebuffer_failed;
        return result;
    }
    result.status = rendering::LegacyFrameColorTransitionStatus::completed;
    return result;
}

}  // namespace openswd3::battle
