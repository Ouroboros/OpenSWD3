#include "openswd3/battle/legacy_battle_color_accumulation.hpp"

#include "test.hpp"

#include <bit>
#include <cmath>
#include <limits>

namespace {

[[nodiscard]] openswd3::rendering::LegacyPixelConversionState rgb565() {
    openswd3::rendering::LegacyPixelConversionState conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        conversion,
        openswd3::rendering::LegacyPixelMasks{
            .red = 0xF800U,
            .green = 0x07E0U,
            .blue = 0x001FU,
        }
    );
    return conversion;
}

}  // namespace

void test_battle_color_accumulation(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleColorAccumulationPixelCount;
    using openswd3::battle::update_legacy_battle_color_accumulation;
    using openswd3::rendering::LegacyFrameColorTransitionState;
    using openswd3::rendering::LegacyFrameColorTransitionStatus;
    using openswd3::rendering::LegacyFramebuffer;

    const auto conversion = rgb565();

    {
        LegacyFrameColorTransitionState state;
        const auto result =
            openswd3::battle::initialize_legacy_battle_color_accumulation(
                state,
                {
                    .current_red = 24,
                    .current_green = 24,
                    .current_blue = 24,
                    .target_red = 0,
                    .target_green = 0,
                    .target_blue = 0,
                    .countdown = 8,
                }
            );
        test.expect_true(
            state.countdown == 8 && state.current_red == 24.0F &&
                state.current_green == 24.0F && state.current_blue == 24.0F &&
                state.target_red == 0.0F && state.target_green == 0.0F &&
                state.target_blue == 0.0F && state.step_red == -3.0F &&
                state.step_green == -3.0F && state.step_blue == -3.0F &&
                result.return_eax == 24U && result.return_ecx == 0xFFFFFFE8U &&
                result.return_edx == 0U,
            "color initialization publishes signed integer arguments and final blue conversion registers"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        const auto result =
            openswd3::battle::initialize_legacy_battle_color_accumulation(
                state,
                {
                    .current_red = 1,
                    .current_green = 0,
                    .current_blue = -1,
                    .target_red = 2,
                    .target_green = 0,
                    .target_blue = -2,
                    .countdown = 0,
                }
            );
        test.expect_true(
            std::bit_cast<openswd3::compat::u32>(state.step_red) ==
                    0x7F800000U &&
                std::bit_cast<openswd3::compat::u32>(state.step_green) ==
                    0xFFC00000U &&
                std::bit_cast<openswd3::compat::u32>(state.step_blue) ==
                    0xFF800000U &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0xFFFFFFFFU &&
                result.return_edx == 0xFFFFFFFFU,
            "zero countdown preserves x87 positive infinity indefinite NaN negative infinity and signed blue registers"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        const auto result =
            openswd3::battle::initialize_legacy_battle_color_accumulation(
                state,
                {
                    .current_red =
                        std::numeric_limits<openswd3::compat::i32>::max(),
                    .current_green =
                        std::numeric_limits<openswd3::compat::i32>::min(),
                    .current_blue = -1,
                    .target_red =
                        std::numeric_limits<openswd3::compat::i32>::min(),
                    .target_green =
                        std::numeric_limits<openswd3::compat::i32>::max(),
                    .target_blue = 2,
                    .countdown = -2,
                }
            );
        test.expect_true(
            state.current_red == 2147483648.0F &&
                std::bit_cast<openswd3::compat::u32>(state.step_red) ==
                    0x80000000U &&
                state.step_green == 0.5F && state.step_blue == -1.5F &&
                result.return_eax == 0xFFFFFFFFU && result.return_ecx == 3U &&
                result.return_edx == 0xFFFFFFFFU,
            "initial float rounding precedes wrapped target subtraction and signed negative countdown division"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = 7;
        state.current_red = 4.0F;
        state.step_red = std::numeric_limits<float>::quiet_NaN();
        state.step_green = -0.0F;
        state.step_blue = 0.0F;
        LegacyFramebuffer framebuffer;
        framebuffer.physical_pixels()[0U] = 0x1234U;
        const auto result = update_legacy_battle_color_accumulation(
            state, true, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::idle &&
                !result.countdown_decremented &&
                !result.current_values_advanced && state.countdown == 7 &&
                state.current_red == 4.0F &&
                framebuffer.physical_pixels()[0U] == 0x1234U,
            "zero or unordered steps take the original x87 equality early return"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.current_red = 1.0F;
        state.step_red = std::numeric_limits<float>::quiet_NaN();
        state.step_green = 2.0F;
        LegacyFramebuffer framebuffer;
        const auto result = update_legacy_battle_color_accumulation(
            state, false, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                result.current_values_advanced &&
                std::isnan(state.current_red) && state.current_green == 2.0F &&
                result.applied_red == 0 && result.applied_green == 2,
            "one unordered step and another nonzero step continue the original C3 conjunction path"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = 2;
        state.current_red = 1.75F;
        state.current_green = -2.75F;
        state.current_blue = 3.0F;
        state.step_red = 0.5F;
        state.step_green = 0.5F;
        state.step_blue = 0.5F;
        LegacyFramebuffer framebuffer;
        const auto result = update_legacy_battle_color_accumulation(
            state, true, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                result.countdown_decremented &&
                result.current_values_advanced &&
                !result.steps_replaced_by_targets && state.countdown == 1 &&
                state.current_red == 2.25F && state.current_green == -2.25F &&
                state.current_blue == 3.5F && result.applied_red == 2 &&
                result.applied_green == -2 && result.applied_blue == 3,
            "nonnegative countdown decrements then accumulates floats and truncates each current value toward zero"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = 0;
        state.current_red = 4.0F;
        state.current_green = 5.0F;
        state.current_blue = 6.0F;
        state.step_red = 1.0F;
        state.step_green = 2.0F;
        state.step_blue = 3.0F;
        state.target_red = -1.0F;
        state.target_green = -2.0F;
        state.target_blue = -3.0F;
        LegacyFramebuffer framebuffer;
        const auto result = update_legacy_battle_color_accumulation(
            state, true, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                result.countdown_decremented && state.countdown == -1 &&
                !result.current_values_advanced &&
                result.steps_replaced_by_targets && state.current_red == 4.0F &&
                state.current_green == 5.0F && state.current_blue == 6.0F &&
                state.step_red == -1.0F && state.step_green == -2.0F &&
                state.step_blue == -3.0F && result.applied_red == 4 &&
                result.applied_green == 5 && result.applied_blue == 6,
            "zero countdown publishes minus one before replacing all steps without advancing currents"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = 0;
        state.step_red = 1.0F;
        state.current_red = 2.0F;
        LegacyFramebuffer framebuffer;
        const auto result = update_legacy_battle_color_accumulation(
            state, false, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                !result.countdown_decremented && state.countdown == 0 &&
                result.current_values_advanced && state.current_red == 3.0F,
            "zero argument skips the countdown decrement and keeps the nonnegative accumulation path"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = -1;
        state.step_red = 1.0F;
        state.target_red = 2.0F;
        state.target_green = 3.0F;
        state.target_blue = 4.0F;
        state.current_red = std::numeric_limits<float>::infinity();
        state.current_green = 4294967296.0F;
        state.current_blue = -1.0F;
        LegacyFramebuffer framebuffer;
        const auto result = update_legacy_battle_color_accumulation(
            state, true, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                !result.countdown_decremented &&
                result.steps_replaced_by_targets && state.step_red == 2.0F &&
                state.step_green == 3.0F && state.step_blue == 4.0F &&
                result.applied_red == 0 && result.applied_green == 0 &&
                result.applied_blue == -1,
            "negative countdown preserves its value and x87 qword conversion exposes low dword zero for invalid and 2^32"
        );
    }

    {
        LegacyFrameColorTransitionState state;
        state.countdown = 1;
        state.step_red = 1.0F;
        LegacyFramebuffer framebuffer;
        auto pixels = framebuffer.physical_pixels();
        pixels[0U] = 0U;
        pixels[static_cast<std::size_t>(
            kLegacyBattleColorAccumulationPixelCount - 1
        )] = 0U;
        pixels[static_cast<std::size_t>(
            kLegacyBattleColorAccumulationPixelCount
        )] = 0x1234U;
        const auto result = update_legacy_battle_color_accumulation(
            state, true, framebuffer, conversion
        );
        test.expect_true(
            result.status == LegacyFrameColorTransitionStatus::completed &&
                pixels[0U] == 0x0800U &&
                pixels[static_cast<std::size_t>(
                    kLegacyBattleColorAccumulationPixelCount - 1
                )] == 0x0800U &&
                pixels[static_cast<std::size_t>(
                    kLegacyBattleColorAccumulationPixelCount
                )] == 0x1234U,
            "battle color accumulation adjusts exactly the 0x3C000-pixel prefix and leaves the following pixel untouched"
        );
    }
}
