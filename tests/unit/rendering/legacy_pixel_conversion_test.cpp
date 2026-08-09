#include "test.hpp"

#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::rendering::LegacyPixelTransform;

[[nodiscard]] constexpr u16 expected_transform(
    const LegacyPixelTransform transform,
    const u16 pixel
) noexcept {
    switch (transform) {
    case LegacyPixelTransform::identity:
        return pixel;

    case LegacyPixelTransform::shift_whole_word_left:
        return static_cast<u16>(static_cast<u32>(pixel) << 1U);

    case LegacyPixelTransform::rgb555_to_rgb565:
        return static_cast<u16>(
            ((static_cast<u32>(pixel) & 0xFFE0U) << 1U) +
            (static_cast<u32>(pixel) & 0x001FU)
        );

    case LegacyPixelTransform::rgb565_to_rgb555:
        return static_cast<u16>(
            ((static_cast<u32>(pixel) >> 1U) & 0xFFE0U) +
            (static_cast<u32>(pixel) & 0x001FU)
        );

    case LegacyPixelTransform::shift_red_field_left:
        return static_cast<u16>(
            ((static_cast<u32>(pixel) & 0x7C00U) << 1U) |
            (static_cast<u32>(pixel) & 0x03FFU)
        );
    }

    return pixel;
}

void test_all_pixel_values(openswd3::test::Context& test) {
    constexpr std::array<LegacyPixelTransform, 4> kTransforms{
        LegacyPixelTransform::shift_whole_word_left,
        LegacyPixelTransform::rgb555_to_rgb565,
        LegacyPixelTransform::rgb565_to_rgb555,
        LegacyPixelTransform::shift_red_field_left,
    };

    for (const LegacyPixelTransform transform : kTransforms) {
        u32 mismatches{};
        for (u32 value = 0U; value <= 0xFFFFU; ++value) {
            u16 actual = static_cast<u16>(value);
            openswd3::rendering::apply_legacy_pixel_transform(
                transform,
                &actual,
                1
            );
            if (actual != expected_transform(
                    transform,
                    static_cast<u16>(value)
                )) {
                ++mismatches;
            }
        }

        test.expect_equal(
            mismatches,
            0U,
            "every possible packed u16 follows the assembly formula"
        );
    }
}

void test_nonpositive_count_bug(openswd3::test::Context& test) {
    std::array<u16, 3> zero_count{0x1234U, 0x2345U, 0x3456U};
    openswd3::rendering::apply_legacy_pixel_transform(
        LegacyPixelTransform::shift_whole_word_left,
        zero_count.data(),
        0
    );
    constexpr std::array<u16, 3> kZeroExpected{0x2468U, 0x2345U, 0x3456U};
    test.expect_equal(
        zero_count,
        kZeroExpected,
        "zero count still converts the first pixel"
    );

    std::array<u16, 3> negative_count{0x7C1FU, 0x03E0U, 0x001FU};
    openswd3::rendering::apply_legacy_pixel_transform(
        LegacyPixelTransform::rgb555_to_rgb565,
        negative_count.data(),
        -1
    );
    constexpr std::array<u16, 3> kNegativeExpected{
        0xF81FU,
        0x03E0U,
        0x001FU,
    };
    test.expect_equal(
        negative_count,
        kNegativeExpected,
        "negative count still converts exactly the first pixel"
    );

    openswd3::rendering::apply_legacy_pixel_transform(
        LegacyPixelTransform::identity,
        nullptr,
        0
    );
}

void test_selector_state(openswd3::test::Context& test) {
    LegacyPixelConversionState state;
    const LegacyPixelMasks rgb565{0xF800U, 0x07E0U, 0x001FU};
    openswd3::rendering::select_legacy_pixel_conversion(state, rgb565);
    test.expect_equal(
        state.forward,
        LegacyPixelTransform::rgb555_to_rgb565,
        "RGB565 selects the forward converter"
    );
    test.expect_equal(
        state.reverse,
        LegacyPixelTransform::rgb565_to_rgb555,
        "RGB565 selects the reverse converter"
    );
    test.expect_equal(
        state.effective_masks.green,
        0x07C0U,
        "RGB565 narrows the six-bit green field to five effective bits"
    );
    test.expect_equal(state.red_shift, 11U, "RGB565 red shift");
    test.expect_equal(state.green_shift, 6U, "RGB565 green shift");
    test.expect_equal(state.blue_shift, 0U, "RGB565 blue shift");

    u16 forward_pixel = 0x7C1FU;
    openswd3::rendering::legacy_convert_pixels_forward(
        state,
        &forward_pixel,
        1
    );
    test.expect_equal(
        forward_pixel,
        static_cast<u16>(0xF81FU),
        "forward wrapper dispatches through the selected RGB565 converter"
    );

    u16 reverse_pixel = 0x07E0U;
    openswd3::rendering::legacy_convert_pixels_reverse(
        state,
        &reverse_pixel,
        1
    );
    test.expect_equal(
        reverse_pixel,
        static_cast<u16>(0x03E0U),
        "reverse wrapper dispatches through the selected RGB565 converter"
    );

    const LegacyPixelMasks whole_shift{0xF800U, 0x07C0U, 0x003FU};
    openswd3::rendering::select_legacy_pixel_conversion(state, whole_shift);
    test.expect_equal(
        state.forward,
        LegacyPixelTransform::shift_whole_word_left,
        "six-bit-blue format replaces only the forward converter"
    );
    test.expect_equal(
        state.reverse,
        LegacyPixelTransform::rgb565_to_rgb555,
        "six-bit-blue format inherits the previous reverse converter"
    );
    test.expect_equal(
        state.effective_masks.blue,
        0x003EU,
        "six-bit-blue format drops the low blue bit"
    );
    test.expect_equal(state.red_shift, 11U, "six-bit-blue red shift");
    test.expect_equal(state.green_shift, 6U, "six-bit-blue green shift");
    test.expect_equal(state.blue_shift, 1U, "six-bit-blue blue shift");

    const LegacyPixelMasks unsupported{0x1111U, 0x2222U, 0x3333U};
    openswd3::rendering::select_legacy_pixel_conversion(state, unsupported);
    test.expect_equal(
        state.forward,
        LegacyPixelTransform::shift_whole_word_left,
        "unsupported masks preserve the previous forward converter"
    );
    test.expect_equal(
        state.reverse,
        LegacyPixelTransform::rgb565_to_rgb555,
        "unsupported masks preserve the previous reverse converter"
    );
    test.expect_equal(
        state.reported_masks.red,
        0x1111U,
        "unsupported masks are still published as the reported format"
    );
    test.expect_equal(
        state.effective_masks.green,
        0x2222U,
        "unsupported masks become the active arithmetic masks"
    );
    test.expect_equal(
        state.red_shift,
        11U,
        "unsupported masks inherit the previous red shift"
    );
    test.expect_equal(
        state.green_shift,
        6U,
        "unsupported masks inherit the previous green shift"
    );
    test.expect_equal(
        state.blue_shift,
        1U,
        "unsupported masks inherit the previous blue shift"
    );

    const LegacyPixelMasks shifted_red{0xFC00U, 0x03E0U, 0x001FU};
    openswd3::rendering::select_legacy_pixel_conversion(state, shifted_red);
    test.expect_equal(
        state.forward,
        LegacyPixelTransform::shift_red_field_left,
        "six-bit-red format replaces only the forward converter"
    );
    test.expect_equal(
        state.reverse,
        LegacyPixelTransform::rgb565_to_rgb555,
        "six-bit-red format inherits the previous reverse converter"
    );
    test.expect_equal(
        state.effective_masks.red,
        0xF800U,
        "six-bit-red format drops the low red bit"
    );
    test.expect_equal(state.red_shift, 11U, "six-bit-red red shift");
    test.expect_equal(state.green_shift, 5U, "six-bit-red green shift");
    test.expect_equal(state.blue_shift, 0U, "six-bit-red blue shift");

    const LegacyPixelMasks rgb555{0x7C00U, 0x03E0U, 0x001FU};
    openswd3::rendering::select_legacy_pixel_conversion(state, rgb555);
    test.expect_equal(
        state.forward,
        LegacyPixelTransform::identity,
        "RGB555 resets the forward converter"
    );
    test.expect_equal(
        state.reverse,
        LegacyPixelTransform::identity,
        "RGB555 resets the reverse converter"
    );
    test.expect_equal(
        state.effective_masks.red,
        0x7C00U,
        "RGB555 restores the exact red mask"
    );
    test.expect_equal(state.red_shift, 10U, "RGB555 red shift");
    test.expect_equal(state.green_shift, 5U, "RGB555 green shift");
    test.expect_equal(state.blue_shift, 0U, "RGB555 blue shift");

    std::array<u16, 2> pixels{0x1234U, 0x5678U};
    openswd3::rendering::legacy_convert_pixels_forward(
        state,
        pixels.data(),
        2
    );
    constexpr std::array<u16, 2> kUnchanged{0x1234U, 0x5678U};
    test.expect_equal(
        pixels,
        kUnchanged,
        "forward wrapper dispatches through the selected identity transform"
    );
}

void test_color_pair_packing(openswd3::test::Context& test) {
    LegacyPixelConversionState state;
    test.expect_equal(
        openswd3::rendering::legacy_pack_color_pair(state, 25, 23, 17),
        0x66F166F1U,
        "sub_4239D0 RGB555 pair"
    );
    test.expect_equal(
        openswd3::rendering::legacy_pack_color_pair(state, -1, 32, 33),
        0x7C017C01U,
        "sub_4239D0 masks every input to five bits"
    );

    openswd3::rendering::select_legacy_pixel_conversion(
        state,
        LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    test.expect_equal(
        openswd3::rendering::legacy_pack_color_pair(state, 25, 23, 17),
        0xCDD1CDD1U,
        "sub_4239D0 applies the selected forward conversion to both lanes"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_all_pixel_values(test);
    test_nonpositive_count_bug(test);
    test_selector_state(test);
    test_color_pair_packing(test);
    return test.exit_code();
}
