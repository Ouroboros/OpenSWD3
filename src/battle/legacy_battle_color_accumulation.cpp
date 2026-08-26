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

struct X87IntegerResult {
    compat::u32 low{};
    compat::u32 high{};
};

[[nodiscard]] X87IntegerResult
truncate_x87_float_to_qword(const float value) noexcept {
    if (!std::isfinite(value)) {
        return {.low = 0U, .high = 0x80000000U};
    }
    const double truncated = std::trunc(static_cast<double>(value));
    constexpr double kMinimum =
        static_cast<double>(std::numeric_limits<std::int64_t>::min());
    constexpr double kMaximum =
        static_cast<double>(std::numeric_limits<std::int64_t>::max());
    if (truncated < kMinimum || truncated >= kMaximum) {
        return {.low = 0U, .high = 0x80000000U};
    }
    const auto integer = static_cast<std::int64_t>(truncated);
    const auto bits = std::bit_cast<std::uint64_t>(integer);
    return {
        .low = static_cast<compat::u32>(bits),
        .high = static_cast<compat::u32>(bits >> 32U),
    };
}

[[nodiscard]] compat::i32
truncate_x87_float_to_low_i32(const float value) noexcept {
    return std::bit_cast<compat::i32>(truncate_x87_float_to_qword(value).low);
}

[[nodiscard]] constexpr compat::i32 wrapping_subtract_i32(
    const compat::i32 left, const compat::u32 right
) noexcept {
    return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(left) - right);
}

[[nodiscard]] float legacy_x87_interpolation_step(
    const compat::i32 delta, const compat::i32 countdown
) noexcept {
    if (countdown == 0) {
        if (delta > 0) {
            return std::bit_cast<float>(compat::u32{0x7F800000U});
        }
        if (delta < 0) {
            return std::bit_cast<float>(compat::u32{0xFF800000U});
        }
        return std::bit_cast<float>(compat::u32{0xFFC00000U});
    }
    return static_cast<float>(
        static_cast<long double>(delta) / static_cast<long double>(countdown)
    );
}

}  // namespace

LegacyBattleColorInitializationResult
initialize_legacy_battle_color_accumulation(
    rendering::LegacyFrameColorTransitionState& state,
    const LegacyBattleColorInitializationRequest& request
) noexcept {
    state.countdown = request.countdown;
    state.current_red = static_cast<float>(request.current_red);
    state.current_green = static_cast<float>(request.current_green);
    state.current_blue = static_cast<float>(request.current_blue);
    state.target_red = static_cast<float>(request.target_red);
    state.target_green = static_cast<float>(request.target_green);
    state.target_blue = static_cast<float>(request.target_blue);

    const X87IntegerResult red = truncate_x87_float_to_qword(state.current_red);
    const compat::i32 red_delta =
        wrapping_subtract_i32(request.target_red, red.low);
    state.step_red =
        legacy_x87_interpolation_step(red_delta, request.countdown);

    const X87IntegerResult green =
        truncate_x87_float_to_qword(state.current_green);
    const compat::i32 green_delta =
        wrapping_subtract_i32(request.target_green, green.low);
    state.step_green =
        legacy_x87_interpolation_step(green_delta, request.countdown);

    const X87IntegerResult blue =
        truncate_x87_float_to_qword(state.current_blue);
    const compat::i32 blue_delta =
        wrapping_subtract_i32(request.target_blue, blue.low);
    state.step_blue =
        legacy_x87_interpolation_step(blue_delta, request.countdown);

    return {
        .return_eax = blue.low,
        .return_ecx = std::bit_cast<compat::u32>(blue_delta),
        .return_edx = blue.high,
    };
}

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
