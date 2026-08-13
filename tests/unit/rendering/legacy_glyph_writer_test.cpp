#include "test.hpp"

#include "openswd3/rendering/legacy_glyph_writer.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyGlyphBackgroundRequest;
using openswd3::rendering::LegacyGlyphBackgroundStatus;
using openswd3::rendering::LegacyGlyphClipRectangle;
using openswd3::rendering::LegacyGlyphDrawRequest;
using openswd3::rendering::LegacyGlyphStyle;
using openswd3::rendering::LegacyGlyphWriterState;
using openswd3::rendering::LegacyGlyphWriteStatus;
using openswd3::rendering::LegacySurfaceGeometry;

[[nodiscard]] LegacyFramebuffer
make_framebuffer(const int width = 12, const int height = 8) {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = width * 2,
        .width = width,
        .height = height,
    }};
}

[[nodiscard]] LegacyGlyphWriterState
make_state(const int height = 1, const std::size_t row_bytes = 1U) {
    return LegacyGlyphWriterState{
        .glyph_height = height,
        .mask_row_bytes = row_bytes,
        .secondary_color = 0x2222U,
        .clip = LegacyGlyphClipRectangle{
            .left = 0,
            .top = 0,
            .width = 12,
            .height = 8,
        },
    };
}

void clear(LegacyFramebuffer& framebuffer, const u16 color = 0U) {
    std::ranges::fill(framebuffer.physical_pixels(), color);
}

void expect_pixel(
    openswd3::test::Context& test,
    const LegacyFramebuffer& framebuffer,
    const int x,
    const int y,
    const u16 expected,
    const std::string_view message
) {
    test.expect_equal(
        framebuffer.row_pixels(
            static_cast<unsigned int>(y)
        )[static_cast<std::size_t>(x)],
        expected,
        message
    );
}

void test_style_priority(openswd3::test::Context& test) {
    test.expect_equal(
        openswd3::rendering::select_legacy_glyph_style(0U),
        LegacyGlyphStyle::none,
        "no low selector bit"
    );
    test.expect_equal(
        openswd3::rendering::select_legacy_glyph_style(0x10U),
        LegacyGlyphStyle::outlined_double,
        "0x10 selector"
    );
    test.expect_equal(
        openswd3::rendering::select_legacy_glyph_style(0x18U),
        LegacyGlyphStyle::outlined_single,
        "0x08 wins over 0x10"
    );
    test.expect_equal(
        openswd3::rendering::select_legacy_glyph_style(0x15U),
        LegacyGlyphStyle::single,
        "0x01 wins over all later selector bits"
    );
    test.expect_equal(
        openswd3::rendering::select_legacy_glyph_style(0x84U),
        LegacyGlyphStyle::doubled_shadow,
        "row color flags do not change selector priority"
    );
}

void test_direct_footprints(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kMask{0x80U};
    const LegacyGlyphWriterState state = make_state();

    LegacyFramebuffer single = make_framebuffer();
    auto result = openswd3::rendering::draw_legacy_glyph(
        single,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 3,
            .destination_y = 2,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(result.status, LegacyGlyphWriteStatus::completed, "0x01");
    expect_pixel(test, single, 3, 2, 0x1111U, "0x01 foreground");
    expect_pixel(
        test, single, 4, 2, 0U, "0x01 reserves but does not write x+1"
    );

    LegacyFramebuffer shadow = make_framebuffer();
    result = openswd3::rendering::draw_legacy_glyph(
        shadow,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 3,
            .destination_y = 2,
            .foreground_color = 0x1111U,
            .flags = 0x02U,
        }
    );
    test.expect_equal(result.status, LegacyGlyphWriteStatus::completed, "0x02");
    expect_pixel(test, shadow, 3, 2, 0x1111U, "0x02 foreground");
    expect_pixel(test, shadow, 3, 3, 0x2222U, "0x02 shadow x");
    expect_pixel(test, shadow, 4, 3, 0x2222U, "0x02 shadow x+1");

    LegacyFramebuffer doubled = make_framebuffer();
    result = openswd3::rendering::draw_legacy_glyph(
        doubled,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 3,
            .destination_y = 2,
            .foreground_color = 0x1111U,
            .flags = 0x04U,
        }
    );
    test.expect_equal(result.status, LegacyGlyphWriteStatus::completed, "0x04");
    expect_pixel(test, doubled, 3, 2, 0x1111U, "0x04 foreground x");
    expect_pixel(test, doubled, 4, 2, 0x1111U, "0x04 foreground x+1");
    expect_pixel(test, doubled, 4, 3, 0x2222U, "0x04 shadow x+1");
    expect_pixel(test, doubled, 5, 3, 0x2222U, "0x04 shadow x+2");
}

void test_outline_footprints(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kMask{0x80U};
    const LegacyGlyphWriterState state = make_state();

    LegacyFramebuffer single = make_framebuffer();
    auto result = openswd3::rendering::draw_legacy_glyph(
        single,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 4,
            .destination_y = 3,
            .foreground_color = 0x1111U,
            .flags = 0x08U,
        }
    );
    test.expect_equal(result.status, LegacyGlyphWriteStatus::completed, "0x08");
    expect_pixel(test, single, 4, 2, 0x2222U, "0x08 upper outline");
    expect_pixel(test, single, 3, 3, 0x2222U, "0x08 left outline");
    expect_pixel(test, single, 4, 3, 0x1111U, "0x08 overlay replaces center");
    expect_pixel(test, single, 3, 4, 0x2222U, "0x08 lower-left outline");
    expect_pixel(test, single, 4, 4, 0x2222U, "0x08 lower-center outline");
    expect_pixel(test, single, 5, 4, 0x2222U, "0x08 lower-right outline");

    LegacyFramebuffer doubled = make_framebuffer();
    result = openswd3::rendering::draw_legacy_glyph(
        doubled,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 4,
            .destination_y = 3,
            .foreground_color = 0x1111U,
            .flags = 0x10U,
        }
    );
    test.expect_equal(result.status, LegacyGlyphWriteStatus::completed, "0x10");
    expect_pixel(test, doubled, 4, 2, 0x2222U, "0x10 upper outline");
    expect_pixel(test, doubled, 3, 3, 0x2222U, "0x10 left outline");
    expect_pixel(test, doubled, 4, 3, 0x1111U, "0x10 overlay x");
    expect_pixel(test, doubled, 5, 3, 0x1111U, "0x10 overlay x+1");
    expect_pixel(test, doubled, 6, 3, 0x2222U, "0x10 right outline");
    for (int x = 3; x <= 6; ++x) {
        expect_pixel(test, doubled, x, 4, 0x2222U, "0x10 lower outline");
    }
}

void test_packed_row_color(openswd3::test::Context& test) {
    constexpr std::array<u8, 2> kTwoRows{0x80U, 0x80U};
    LegacyGlyphWriterState state = make_state(2);
    state.clip.height = 6;

    LegacyFramebuffer incremented = make_framebuffer(10, 6);
    state.clip.width = 10;
    auto result = openswd3::rendering::draw_legacy_glyph(
        incremented,
        kTwoRows,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 2,
            .destination_y = 1,
            .foreground_color = 0x10FFU,
            .flags = 0x81U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "+1 rows"
    );
    expect_pixel(test, incremented, 2, 1, 0x10FFU, "first packed +1 row");
    expect_pixel(
        test, incremented, 2, 2, 0x1100U, "packed u16 carries as one value"
    );

    LegacyFramebuffer decremented = make_framebuffer(10, 6);
    clear(decremented, 0x7777U);
    result = openswd3::rendering::draw_legacy_glyph(
        decremented,
        kTwoRows,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 2,
            .destination_y = 1,
            .foreground_color = 0U,
            .flags = 0x181U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "-1 rows"
    );
    expect_pixel(
        test, decremented, 2, 1, 0U, "0x100 takes precedence over 0x80"
    );
    expect_pixel(
        test, decremented, 2, 2, 0xFFFFU, "packed u16 decrements with wrap"
    );

    LegacyFramebuffer outlined = make_framebuffer(10, 6);
    result = openswd3::rendering::draw_legacy_glyph(
        outlined,
        kTwoRows,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 3,
            .destination_y = 1,
            .foreground_color = 0x1000U,
            .flags = 0x88U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "outline +1"
    );
    expect_pixel(
        test, outlined, 3, 1, 0x1002U, "overlay starts after prepass rows"
    );
    expect_pixel(
        test, outlined, 3, 2, 0x1003U, "overlay advances foreground again"
    );
}

void test_strict_clip_and_padding(openswd3::test::Context& test) {
    LegacyGlyphWriterState state = make_state();
    state.clip = LegacyGlyphClipRectangle{
        .left = 2,
        .top = 2,
        .width = 5,
        .height = 5,
    };

    LegacyFramebuffer right = make_framebuffer();
    constexpr std::array<u8, 1> kTwoPixels{0xC0U};
    auto result = openswd3::rendering::draw_legacy_glyph(
        right,
        kTwoPixels,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 5,
            .destination_y = 2,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "right clip"
    );
    expect_pixel(test, right, 5, 2, 0x1111U, "x+1 below right is accepted");
    expect_pixel(test, right, 6, 2, 0U, "x+1 equal right is rejected");

    LegacyFramebuffer bottom = make_framebuffer();
    constexpr std::array<u8, 2> kTwoRows{0x80U, 0x80U};
    state.glyph_height = 2;
    result = openswd3::rendering::draw_legacy_glyph(
        bottom,
        kTwoRows,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 3,
            .destination_y = 5,
            .foreground_color = 0x1111U,
            .flags = 0x02U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "bottom clip"
    );
    expect_pixel(
        test, bottom, 3, 5, 0x1111U, "shadow row below bottom-1 is accepted"
    );
    expect_pixel(test, bottom, 3, 6, 0x2222U, "accepted shadow row");
    expect_pixel(
        test, bottom, 3, 7, 0U, "glyph at bottom-1 is rejected as a whole"
    );

    LegacyFramebuffer edge = make_framebuffer();
    state.glyph_height = 1;
    constexpr std::array<u8, 1> kOnePixel{0x80U};
    result = openswd3::rendering::draw_legacy_glyph(
        edge,
        kOnePixel,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = 1,
            .destination_y = 3,
            .foreground_color = 0x1111U,
            .flags = 0x08U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "edge outline"
    );
    expect_pixel(
        test, edge, 1, 3, 0x2222U, "wider prepass leaves secondary center"
    );
    expect_pixel(
        test, edge, 1, 2, 0x2222U, "prepass reaches left-1 glyph position"
    );

    LegacyFramebuffer padding = make_framebuffer(20, 4);
    LegacyGlyphWriterState padding_state = make_state(1, 2U);
    padding_state.clip.width = 20;
    padding_state.clip.height = 4;
    constexpr std::array<u8, 2> kPaddingBit{0U, 0x01U};
    result = openswd3::rendering::draw_legacy_glyph(
        padding,
        kPaddingBit,
        padding_state,
        LegacyGlyphDrawRequest{
            .destination_x = 0,
            .destination_y = 1,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(
        result.status, LegacyGlyphWriteStatus::completed, "padding bit"
    );
    expect_pixel(
        test, padding, 15, 1, 0x1111U, "writer consumes all physical row bits"
    );
}

void test_writer_safety_boundaries(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyGlyphWriterState state = make_state(2);
    constexpr std::array<u8, 1> kShortMask{0x80U};
    auto result = openswd3::rendering::draw_legacy_glyph(
        framebuffer, kShortMask, state, LegacyGlyphDrawRequest{.flags = 0x01U}
    );
    test.expect_equal(
        result.status,
        LegacyGlyphWriteStatus::mask_out_of_bounds,
        "short physical mask is rejected before writes"
    );

    result = openswd3::rendering::draw_legacy_glyph(
        framebuffer, {}, state, LegacyGlyphDrawRequest{.flags = 0U}
    );
    test.expect_equal(
        result.status,
        LegacyGlyphWriteStatus::no_style,
        "no selector does not consume a mask"
    );

    state = make_state();
    state.clip.left = -2;
    state.clip.width = 10;
    constexpr std::array<u8, 1> kMask{0x80U};
    result = openswd3::rendering::draw_legacy_glyph(
        framebuffer,
        kMask,
        state,
        LegacyGlyphDrawRequest{
            .destination_x = -1,
            .destination_y = 2,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(
        result.status,
        LegacyGlyphWriteStatus::destination_out_of_bounds,
        "an original out-of-surface write is isolated"
    );
}

void test_background_fill(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(6, 5);
    auto status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = -1,
            .destination_y = -1,
            .width = 4,
            .height = 3,
            .color = 0x3456U,
        }
    );
    test.expect_equal(
        status, LegacyGlyphBackgroundStatus::completed, "background fill"
    );
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            expect_pixel(
                test, framebuffer, x, y, 0x3456U, "clamped background pixel"
            );
        }
        expect_pixel(
            test, framebuffer, 3, y, 0U, "background right is exclusive"
        );
    }
    expect_pixel(test, framebuffer, 0, 2, 0U, "background bottom is exclusive");

    clear(framebuffer);
    status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = 1,
            .destination_y = 3,
            .width = 2,
            .height = 0,
            .color = 0x4567U,
        }
    );
    test.expect_equal(
        status, LegacyGlyphBackgroundStatus::completed, "zero-height background"
    );
    expect_pixel(
        test, framebuffer, 1, 3, 0x4567U, "zero height still writes first row"
    );
    expect_pixel(
        test, framebuffer, 2, 3, 0x4567U, "zero-height first-row width"
    );

    clear(framebuffer);
    status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = 1,
            .destination_y = -3,
            .width = 2,
            .height = 2,
            .color = 0x5678U,
        }
    );
    test.expect_equal(
        status, LegacyGlyphBackgroundStatus::completed, "above background"
    );
    expect_pixel(
        test,
        framebuffer,
        1,
        0,
        0x5678U,
        "fully above rectangle still writes row zero"
    );
    expect_pixel(
        test, framebuffer, 1, 1, 0U, "negative bottom prevents copied rows"
    );

    clear(framebuffer, 0x7777U);
    status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = 1,
            .destination_y = 1,
            .width = 2,
            .height = 2,
            .color = 0xFFFEU,
        }
    );
    test.expect_equal(
        status, LegacyGlyphBackgroundStatus::disabled, "background sentinel"
    );
    test.expect_true(
        std::ranges::all_of(
            framebuffer.physical_pixels(),
            [](const u16 pixel) { return pixel == 0x7777U; }
        ),
        "disabled background leaves framebuffer unchanged"
    );

    status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = 0,
            .destination_y = 5,
            .width = 1,
            .height = 1,
            .color = 0x1234U,
        }
    );
    test.expect_equal(
        status,
        LegacyGlyphBackgroundStatus::destination_out_of_bounds,
        "first-row out-of-surface access is isolated"
    );

    status = openswd3::rendering::fill_legacy_glyph_background(
        framebuffer,
        LegacyGlyphBackgroundRequest{
            .destination_x = 2,
            .destination_y = 99,
            .width = 0,
            .height = 1,
            .color = 0x1234U,
        }
    );
    test.expect_equal(
        status,
        LegacyGlyphBackgroundStatus::completed,
        "empty horizontal range exits before touching an invalid row"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_style_priority(test);
    test_direct_footprints(test);
    test_outline_footprints(test);
    test_packed_row_color(test);
    test_strict_clip_and_padding(test);
    test_writer_safety_boundaries(test);
    test_background_fill(test);
    return test.exit_code();
}
