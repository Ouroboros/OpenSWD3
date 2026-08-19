#include "test.hpp"

#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyFrameColorStatus;
using openswd3::rendering::LegacyFrameColorTransitionState;
using openswd3::rendering::LegacyFrameColorTransitionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;

struct FormatVector {
    LegacyPixelMasks reported_masks;
    u16 plus_input;
    u16 plus_output;
    u16 minus_input;
    u16 minus_output;
    u16 mixed_input;
    u16 mixed_output;
    u16 combine_source;
    u16 combine_destination;
    u16 combine_output;
    u16 overflow_source;
    u16 overflow_destination;
    u16 grayscale_input;
    u16 grayscale_output;
};

constexpr std::array<FormatVector, 4> kFormatVectors{{
    {
        {0x7C00U, 0x03E0U, 0x001FU},
        0x1154U,
        0x1DB7U,
        0x1154U,
        0x006DU,
        0x783FU,
        0x7C1FU,
        0x14EBU,
        0x0DB1U,
        0x229CU,
        0x50FFU,
        0x3321U,
        0x7DE0U,
        0x2D6BU,
    },
    {
        {0xF800U, 0x07C0U, 0x003FU},
        0x22A8U,
        0x3B6EU,
        0x22A8U,
        0x00DAU,
        0xF07EU,
        0xF83EU,
        0x29D6U,
        0x1B62U,
        0x4538U,
        0xA1FEU,
        0x6642U,
        0xFBC0U,
        0x5AD6U,
    },
    {
        {0xF800U, 0x07E0U, 0x001FU},
        0x2294U,
        0x3B57U,
        0x2294U,
        0x00CDU,
        0xF05FU,
        0xF81FU,
        0x29CBU,
        0x1B51U,
        0x451CU,
        0xA1DFU,
        0x6641U,
        0xFBC0U,
        0x5ACBU,
    },
    {
        {0xFC00U, 0x03E0U, 0x001FU},
        0x2154U,
        0x39B7U,
        0x2154U,
        0x006DU,
        0xF03FU,
        0xF81FU,
        0x28EBU,
        0x19B1U,
        0x429CU,
        0xA0FFU,
        0x6321U,
        0xF9E0U,
        0x596BU,
    },
}};

void expect_rgb_adjustment(
    openswd3::test::Context& test,
    LegacyPixelConversionState& format,
    const u16 input,
    const i32 red,
    const i32 green,
    const i32 blue,
    const u16 expected,
    const char* const message
) {
    std::array<u16, 2> pixels{input, 0xA55AU};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_rgb_channels(
            pixels, 1, red, green, blue, format
        ),
        LegacyFrameColorStatus::completed,
        "RGB adjustment completes"
    );
    test.expect_equal(pixels[0], expected, message);
    test.expect_equal(
        pixels[1],
        static_cast<u16>(0xA55AU),
        "dword look-ahead remains unmodified"
    );
}

void test_closed_format_vectors(openswd3::test::Context& test) {
    for (const FormatVector& vector : kFormatVectors) {
        LegacyPixelConversionState format;
        openswd3::rendering::select_legacy_pixel_conversion(
            format, vector.reported_masks
        );

        expect_rgb_adjustment(
            test,
            format,
            vector.plus_input,
            3,
            3,
            3,
            vector.plus_output,
            "three-channel positive vector"
        );
        expect_rgb_adjustment(
            test,
            format,
            vector.minus_input,
            -7,
            -7,
            -7,
            vector.minus_output,
            "three-channel negative vector"
        );
        expect_rgb_adjustment(
            test,
            format,
            vector.mixed_input,
            5,
            -5,
            1,
            vector.mixed_output,
            "three-channel mixed saturation vector"
        );

        std::array<u16, 1> source{vector.combine_source};
        std::array<u16, 1> destination{vector.combine_destination};
        test.expect_equal(
            openswd3::rendering::combine_legacy_channels_overflow_to_zero(
                source, destination, 1, format
            ),
            LegacyFrameColorStatus::completed,
            "two-input combine completes"
        );
        test.expect_equal(
            destination[0], vector.combine_output, "two-input below-32 vector"
        );

        source[0] = vector.overflow_source;
        destination[0] = vector.overflow_destination;
        (void)openswd3::rendering::combine_legacy_channels_overflow_to_zero(
            source, destination, 1, format
        );
        test.expect_equal(
            destination[0],
            static_cast<u16>(0U),
            "channel sums of 32 map to zero instead of saturation"
        );

        std::array<u16, 1> grayscale{vector.grayscale_input};
        test.expect_equal(
            openswd3::rendering::convert_legacy_quarter_sum_grayscale(
                grayscale, 1, format
            ),
            LegacyFrameColorStatus::completed,
            "quarter-sum grayscale completes"
        );
        test.expect_equal(
            grayscale[0],
            vector.grayscale_output,
            "quarter-sum grayscale vector"
        );
    }
}

void test_single_channel_variants(openswd3::test::Context& test) {
    const LegacyPixelConversionState format;

    std::array<u16, 3> red{0x1154U, 0x783FU, 0xA55AU};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_red_channel(red, 2, 3, format),
        LegacyFrameColorStatus::completed,
        "red-only adjustment completes"
    );
    constexpr std::array<u16, 3> kRedExpected{
        0x1D54U,
        0x7C3FU,
        0xA55AU,
    };
    test.expect_equal(red, kRedExpected, "red-only saturation and look-ahead");

    std::array<u16, 2> green{0x1154U, 0x783FU};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_green_channel_pairs(
            green, 2, -5, format
        ),
        LegacyFrameColorStatus::completed,
        "green packed-pair adjustment completes"
    );
    constexpr std::array<u16, 2> kGreenExpected{0x10B4U, 0x781FU};
    test.expect_equal(green, kGreenExpected, "green packed-pair underflow");

    std::array<u16, 2> blue{0x1154U, 0x783FU};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_blue_channel_pairs(
            blue, 2, 1, format
        ),
        LegacyFrameColorStatus::completed,
        "blue packed-pair adjustment completes"
    );
    constexpr std::array<u16, 2> kBlueExpected{0x1155U, 0x783FU};
    test.expect_equal(blue, kBlueExpected, "blue packed-pair saturation");
}

void test_wrapped_delta_and_lane_carry(openswd3::test::Context& test) {
    const LegacyPixelConversionState format;
    std::array<u16, 2> rgb{0x1154U, 0xA55AU};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_rgb_channels(
            rgb,
            1,
            std::numeric_limits<i32>::min(),
            std::numeric_limits<i32>::min(),
            std::numeric_limits<i32>::min(),
            format
        ),
        LegacyFrameColorStatus::completed,
        "extreme wrapped RGB adjustment completes"
    );
    test.expect_equal(
        rgb[0],
        static_cast<u16>(0x1140U),
        "imul wrap keeps red/green delta zero and underflows blue"
    );

    std::array<u16, 2> green{};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_green_channel_pairs(
            green, 2, 2048, format
        ),
        LegacyFrameColorStatus::completed,
        "packed lane-carry vector completes"
    );
    constexpr std::array<u16, 2> kLaneCarryExpected{0U, 0x03E0U};
    test.expect_equal(
        green,
        kLaneCarryExpected,
        "dword delta carry leaves low lane and saturates high lane"
    );
}

void test_safety_and_zero_count_side_effect(openswd3::test::Context& test) {
    LegacyPixelConversionState format;
    std::array<u16, 1> one_pixel{0x1234U};
    test.expect_equal(
        openswd3::rendering::adjust_legacy_rgb_channels(
            one_pixel, 1, 1, 1, 1, format
        ),
        LegacyFrameColorStatus::buffer_out_of_bounds,
        "RGB dword look-ahead is checked"
    );
    test.expect_equal(
        openswd3::rendering::adjust_legacy_green_channel_pairs(
            one_pixel, 1, 1, format
        ),
        LegacyFrameColorStatus::invalid_count,
        "packed green rejects the unreachable odd-count domain"
    );
    test.expect_equal(
        openswd3::rendering::convert_legacy_quarter_sum_grayscale(
            one_pixel, 0, format
        ),
        LegacyFrameColorStatus::invalid_count,
        "tail-tested grayscale zero-count runaway is isolated"
    );

    format.effective_masks = {
        0x12347C00U,
        0x567803E0U,
        0x9ABC001FU,
    };
    test.expect_equal(
        openswd3::rendering::combine_legacy_channels_overflow_to_zero(
            {}, {}, 0, format
        ),
        LegacyFrameColorStatus::completed,
        "combine zero count returns normally"
    );
    test.expect_equal(
        format.effective_masks.red,
        0x7C00U,
        "combine truncates red mask before zero-count return"
    );
    test.expect_equal(
        format.effective_masks.green,
        0x03E0U,
        "combine truncates green mask before zero-count return"
    );
    test.expect_equal(
        format.effective_masks.blue,
        0x001FU,
        "combine truncates blue mask before zero-count return"
    );
}

void test_full_frame_transition_wrapper(openswd3::test::Context& test) {
    const LegacyPixelConversionState format;
    LegacyFramebuffer framebuffer;
    LegacyFrameColorTransitionState transition{
        .countdown = 1,
        .step_red = 1.75F,
        .step_green = 2.9F,
        .step_blue = 3.1F,
    };

    const auto active =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, true, framebuffer, format
        );
    test.expect_true(
        active.status == LegacyFrameColorTransitionStatus::completed &&
            active.framebuffer_status == LegacyFrameColorStatus::completed &&
            active.countdown_decremented && active.current_values_advanced &&
            !active.steps_replaced_by_targets && active.applied_red == 1 &&
            active.applied_green == 2 && active.applied_blue == 3 &&
            transition.countdown == 0,
        "0x004146F0 decrements, accumulates and truncates before sub_420490"
    );
    test.expect_equal(
        framebuffer.physical_pixels().front(),
        static_cast<u16>(0x0443U),
        "first full-frame pixel receives the three channel deltas"
    );
    test.expect_equal(
        framebuffer.physical_pixels().back(),
        static_cast<u16>(0x0443U),
        "the final full-frame pixel is processed through the read guard"
    );
    test.expect_equal(
        framebuffer.physical_pixels_with_read_guard().back(),
        static_cast<u16>(0U),
        "the look-ahead guard remains read-only"
    );

    std::ranges::fill(framebuffer.physical_pixels(), 0U);
    transition = LegacyFrameColorTransitionState{
        .countdown = 0,
        .current_red = 4.9F,
        .current_green = 5.9F,
        .current_blue = 6.9F,
        .target_red = 9.0F,
        .target_green = 10.0F,
        .target_blue = 11.0F,
        .step_red = 7.0F,
        .step_green = 8.0F,
        .step_blue = 9.0F,
    };
    const auto terminal =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, true, framebuffer, format
        );
    test.expect_true(
        terminal.status == LegacyFrameColorTransitionStatus::completed &&
            terminal.countdown_decremented &&
            !terminal.current_values_advanced &&
            terminal.steps_replaced_by_targets && transition.countdown == -1 &&
            transition.current_red == 4.9F &&
            transition.current_green == 5.9F &&
            transition.current_blue == 6.9F && transition.step_red == 9.0F &&
            transition.step_green == 10.0F && transition.step_blue == 11.0F &&
            terminal.applied_red == 4 && terminal.applied_green == 5 &&
            terminal.applied_blue == 6,
        "negative countdown replaces steps without advancing current values"
    );

    const float nan = std::numeric_limits<float>::quiet_NaN();
    transition = LegacyFrameColorTransitionState{
        .countdown = 7,
        .step_red = nan,
        .step_green = nan,
        .step_blue = nan,
    };
    const auto unordered_idle =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, true, framebuffer, format
        );
    test.expect_true(
        unordered_idle.status == LegacyFrameColorTransitionStatus::idle &&
            transition.countdown == 7 && !unordered_idle.countdown_decremented,
        "x87 unordered zero comparisons preserve the original early return"
    );
}

void test_transition_unordered_and_conversion_boundaries(
    openswd3::test::Context& test
) {
    const LegacyPixelConversionState format;
    LegacyFramebuffer framebuffer;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    LegacyFrameColorTransitionState transition{
        .countdown = 0,
        .current_red = 4.0F,
        .step_red = nan,
        .step_green = 1.9F,
    };

    const auto mixed_unordered =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, false, framebuffer, format
        );
    test.expect_true(
        mixed_unordered.status == LegacyFrameColorTransitionStatus::completed &&
            !mixed_unordered.countdown_decremented &&
            mixed_unordered.current_values_advanced &&
            transition.countdown == 0 && std::isnan(transition.current_red) &&
            mixed_unordered.applied_red == 0 &&
            mixed_unordered.applied_green == 1 &&
            mixed_unordered.applied_blue == 0,
        "one unordered step does not trigger the all-three early return"
    );

    transition = LegacyFrameColorTransitionState{
        .countdown = 0,
        .step_red = 4294967040.0F,
        .step_green = -4294967296.0F,
        .step_blue = std::numeric_limits<float>::infinity(),
    };
    const auto conversion =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, false, framebuffer, format
        );
    test.expect_true(
        conversion.status == LegacyFrameColorTransitionStatus::completed &&
            conversion.applied_red == -256 && conversion.applied_green == 0 &&
            conversion.applied_blue == 0,
        "sub_489654 truncates to i64 then exposes the low i32"
    );

    transition = LegacyFrameColorTransitionState{
        .countdown = -2,
        .current_red = 1.9F,
        .current_green = 2.9F,
        .current_blue = 3.9F,
        .target_red = 7.0F,
        .target_green = 8.0F,
        .target_blue = 9.0F,
        .step_red = 4.0F,
        .step_green = 5.0F,
        .step_blue = 6.0F,
    };
    const auto already_negative =
        openswd3::rendering::update_legacy_frame_color_transition(
            transition, true, framebuffer, format
        );
    test.expect_true(
        already_negative.status ==
                LegacyFrameColorTransitionStatus::completed &&
            !already_negative.countdown_decremented &&
            !already_negative.current_values_advanced &&
            already_negative.steps_replaced_by_targets &&
            transition.countdown == -2 && transition.step_red == 7.0F &&
            transition.step_green == 8.0F && transition.step_blue == 9.0F &&
            already_negative.applied_red == 1 &&
            already_negative.applied_green == 2 &&
            already_negative.applied_blue == 3,
        "an initially negative countdown skips decrement and copies targets"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_closed_format_vectors(test);
    test_single_channel_variants(test);
    test_wrapped_delta_and_lane_carry(test);
    test_safety_and_zero_count_side_effect(test);
    test_full_frame_transition_wrapper(test);
    test_transition_unordered_and_conversion_boundaries(test);
    return test.exit_code();
}
