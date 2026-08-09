#include "test.hpp"

#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_scaled_rle_writer.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitClipRectangle;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyScaledRleRequest;
using openswd3::rendering::LegacyScaledRleSource;
using openswd3::rendering::LegacyScaledRleTransform;
using openswd3::rendering::LegacyScaledRleWriteStatus;
using openswd3::rendering::LegacySurfaceGeometry;

void append_u16(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

void append_literal_row(
    std::vector<u8>& bytes,
    const std::initializer_list<u16> pixels
) {
    const auto row_bytes = static_cast<u16>(6U + pixels.size() * 2U);
    append_u16(bytes, row_bytes);
    append_u16(bytes, static_cast<u16>(pixels.size()));
    for (const u16 pixel : pixels) {
        append_u16(bytes, pixel);
    }
    append_u16(bytes, 0U);
}

[[nodiscard]] std::vector<u8> three_row_source() {
    std::vector<u8> bytes(8U, 0U);
    append_literal_row(bytes, {1U, 2U, 3U, 4U});
    append_literal_row(bytes, {5U, 6U, 7U, 8U});
    append_literal_row(bytes, {9U, 10U, 11U, 12U});
    append_u16(bytes, 0U);
    return bytes;
}

[[nodiscard]] LegacyFramebuffer make_framebuffer() {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 24,
        .width = 12,
        .height = 8,
    }};
}

constexpr LegacyBlitClipRectangle kFullClip{
    .left = 0,
    .top = 0,
    .width = 12,
    .height = 8,
};

void expect_row(
    openswd3::test::Context& test,
    const LegacyFramebuffer& framebuffer,
    const std::size_t row,
    const std::span<const u16> expected,
    const char* const message
) {
    test.expect_true(
        std::ranges::equal(
            framebuffer.row_pixels(static_cast<openswd3::compat::u32>(row))
                .first(expected.size()),
            expected
        ),
        message
    );
}

void test_forward_and_reverse(openswd3::test::Context& test) {
    const std::vector<u8> bytes = three_row_source();
    const LegacyScaledRleSource source{bytes};
    const LegacyScaledRleTransform transform{
        .anchor_x = 0,
        .anchor_y = 0,
        .horizontal_step_10_10 = 0x400,
        .vertical_step_10_10 = 0x400,
    };
    const LegacyScaledRleRequest request{
        .destination_x = 1,
        .destination_y = 1,
        .source_width = 4,
        .source_height = 3,
    };

    LegacyFramebuffer forward = make_framebuffer();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            forward,
            kFullClip,
            source,
            request,
            transform
        ),
        LegacyScaledRleWriteStatus::completed,
        "forward scaled RLE completes"
    );
    constexpr std::array<u16, 7> kForwardRow{
        0U, 1U, 2U, 3U, 4U, 0U, 0U,
    };
    expect_row(test, forward, 1U, kForwardRow, "forward row order");

    LegacyFramebuffer reverse = make_framebuffer();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_reverse(
            reverse,
            kFullClip,
            source,
            request,
            transform
        ),
        LegacyScaledRleWriteStatus::completed,
        "reverse scaled RLE completes"
    );
    constexpr std::array<u16, 7> kReverseRow{
        0U, 0U, 4U, 3U, 2U, 1U, 0U,
    };
    expect_row(
        test,
        reverse,
        1U,
        kReverseRow,
        "reverse path starts at transformed right coordinate"
    );
}

void test_vertical_phase_and_top_clip(openswd3::test::Context& test) {
    const std::vector<u8> bytes = three_row_source();
    const LegacyScaledRleSource source{bytes};
    LegacyScaledRleTransform transform{
        .anchor_x = 0,
        .anchor_y = 0,
        .horizontal_step_10_10 = 0x400,
        .vertical_step_10_10 = 0x600,
    };
    LegacyScaledRleRequest request{
        .destination_x = 0,
        .destination_y = 0,
        .source_width = 4,
        .source_height = 3,
    };

    LegacyFramebuffer enlarged = make_framebuffer();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            enlarged,
            kFullClip,
            source,
            request,
            transform
        ),
        LegacyScaledRleWriteStatus::completed,
        "vertical enlargement completes"
    );
    constexpr std::array<u16, 4> kRow0{1U, 2U, 3U, 4U};
    constexpr std::array<u16, 4> kRow1{5U, 6U, 7U, 8U};
    constexpr std::array<u16, 4> kRow2{9U, 10U, 11U, 12U};
    expect_row(test, enlarged, 0U, kRow0, "first row enlargement copy 1");
    expect_row(test, enlarged, 1U, kRow0, "first row enlargement copy 2");
    expect_row(test, enlarged, 2U, kRow1, "second row single copy");
    expect_row(test, enlarged, 3U, kRow2, "third row enlargement copy 1");
    expect_row(test, enlarged, 4U, kRow2, "third row enlargement copy 2");

    transform.vertical_step_10_10 = 0x400;
    request.destination_y = -1;
    LegacyFramebuffer top_clipped = make_framebuffer();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            top_clipped,
            kFullClip,
            source,
            request,
            transform
        ),
        LegacyScaledRleWriteStatus::completed,
        "top-clipped scaled RLE completes"
    );
    expect_row(
        test,
        top_clipped,
        0U,
        kRow1,
        "prepass consumes the source row above the clip"
    );
}

void test_horizontal_phase_and_boundary_bug(
    openswd3::test::Context& test
) {
    std::vector<u8> bytes(8U, 0U);
    append_u16(bytes, 14U);
    append_u16(bytes, 1U);
    append_u16(bytes, 0x1111U);
    append_u16(bytes, 0x8002U);
    append_u16(bytes, 1U);
    append_u16(bytes, 0x2222U);
    append_u16(bytes, 0U);
    append_u16(bytes, 0U);

    const LegacyScaledRleTransform transform{
        .anchor_x = 0,
        .anchor_y = 0,
        .horizontal_step_10_10 = 0x600,
        .vertical_step_10_10 = 0x400,
    };
    const LegacyScaledRleRequest request{
        .destination_x = 0,
        .destination_y = 0,
        .source_width = 4,
        .source_height = 1,
    };
    LegacyFramebuffer framebuffer = make_framebuffer();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{bytes},
            request,
            transform
        ),
        LegacyScaledRleWriteStatus::completed,
        "transparent phase vector completes"
    );
    constexpr std::array<u16, 7> kExpected{
        0x1111U, 0U, 0U, 0U, 0x2222U, 0U, 0U,
    };
    expect_row(
        test,
        framebuffer,
        0U,
        kExpected,
        "transparent run advances phase by step times scaled length"
    );

    std::vector<u8> one_run(8U, 0U);
    append_u16(one_run, 16U);
    append_u16(one_run, 1U);
    append_u16(one_run, 1U);
    append_u16(one_run, 3U);
    append_u16(one_run, 2U);
    append_u16(one_run, 3U);
    append_u16(one_run, 4U);
    append_u16(one_run, 0U);
    append_u16(one_run, 0U);
    LegacyFramebuffer one_past = make_framebuffer();
    const LegacyBlitClipRectangle exact_clip{
        .left = 0,
        .top = 0,
        .width = 4,
        .height = 2,
    };
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            one_past,
            exact_clip,
            LegacyScaledRleSource{one_run},
            LegacyScaledRleRequest{0, 0, 3, 1},
            LegacyScaledRleTransform{0, 0, 0x555, 0x400}
        ),
        LegacyScaledRleWriteStatus::completed,
        "one-past boundary vector completes"
    );
    test.expect_equal(
        one_past.row_pixels(0U)[4U],
        static_cast<u16>(4U),
        "exact-right comparison preserves the one-past write"
    );

    std::vector<u8> clipped_run(8U, 0U);
    append_u16(clipped_run, 14U);
    append_u16(clipped_run, 0x8001U);
    append_u16(clipped_run, 3U);
    append_u16(clipped_run, 5U);
    append_u16(clipped_run, 6U);
    append_u16(clipped_run, 7U);
    append_u16(clipped_run, 0U);
    append_u16(clipped_run, 0U);
    LegacyFramebuffer clipped_phase = make_framebuffer();
    const LegacyBlitClipRectangle left_clip{
        .left = 2,
        .top = 0,
        .width = 3,
        .height = 2,
    };
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            clipped_phase,
            left_clip,
            LegacyScaledRleSource{clipped_run},
            LegacyScaledRleRequest{0, 0, 4, 1},
            LegacyScaledRleTransform{0, 0, 0x555, 0x400}
        ),
        LegacyScaledRleWriteStatus::completed,
        "wholly clipped command vector completes"
    );
    constexpr std::array<u16, 5> kClippedPhaseExpected{
        0U, 0U, 6U, 7U, 0U,
    };
    expect_row(
        test,
        clipped_phase,
        0U,
        kClippedPhaseExpected,
        "wholly clipped command does not advance horizontal phase"
    );
}

void test_reverse_vertical_rejection(openswd3::test::Context& test) {
    const std::vector<u8> bytes = three_row_source();
    LegacyFramebuffer framebuffer = make_framebuffer();
    const LegacyScaledRleTransform transform{
        .anchor_x = 0,
        .anchor_y = 0,
        .horizontal_step_10_10 = 0x400,
        .vertical_step_10_10 = 0x400,
    };
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_reverse(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{bytes},
            LegacyScaledRleRequest{1, 0, 4, 8},
            transform
        ),
        LegacyScaledRleWriteStatus::clipped_out,
        "reverse path rejects an exact bottom boundary"
    );
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_reverse(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{bytes},
            LegacyScaledRleRequest{1, -1, 4, 3},
            transform
        ),
        LegacyScaledRleWriteStatus::clipped_out,
        "reverse path rejects a top-clipped sprite"
    );
}

void test_malformed_and_clipped(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    constexpr std::array<u8, 10> kMalformed{};
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{kMalformed},
            LegacyScaledRleRequest{0, 0, 1, 1},
            LegacyScaledRleTransform{0, 0, 0x400, 0x400}
        ),
        LegacyScaledRleWriteStatus::completed,
        "zero row marker is a normal empty stream"
    );

    std::array<u8, 10> bad_length{};
    bad_length[8] = 1U;
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{bad_length},
            LegacyScaledRleRequest{0, 0, 1, 1},
            LegacyScaledRleTransform{0, 0, 0x400, 0x400}
        ),
        LegacyScaledRleWriteStatus::malformed_source,
        "short nonzero row is isolated"
    );

    std::vector<u8> flagged_zero_run(8U, 0U);
    append_u16(flagged_zero_run, 10U);
    append_u16(flagged_zero_run, 0x8000U);
    append_u16(flagged_zero_run, 1U);
    append_u16(flagged_zero_run, 0x3456U);
    append_u16(flagged_zero_run, 0U);
    append_u16(flagged_zero_run, 0U);
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{flagged_zero_run},
            LegacyScaledRleRequest{0, 0, 1, 1},
            LegacyScaledRleTransform{0, 0, 0x400, 0x400}
        ),
        LegacyScaledRleWriteStatus::completed,
        "flagged zero-length command is not the row terminator"
    );
    test.expect_equal(
        framebuffer.row_pixels(0U)[0U],
        static_cast<u16>(0x3456U),
        "literal after flagged zero-length command is rendered"
    );

    const std::vector<u8> bytes = three_row_source();
    test.expect_equal(
        openswd3::rendering::write_legacy_scaled_rle_forward(
            framebuffer,
            kFullClip,
            LegacyScaledRleSource{bytes},
            LegacyScaledRleRequest{20, 0, 4, 3},
            LegacyScaledRleTransform{0, 0, 0x400, 0x400}
        ),
        LegacyScaledRleWriteStatus::clipped_out,
        "fully offscreen scaled sprite is rejected before parsing"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_forward_and_reverse(test);
    test_vertical_phase_and_top_clip(test);
    test_horizontal_phase_and_boundary_bug(test);
    test_reverse_vertical_rejection(test);
    test_malformed_and_clipped(test);
    return test.exit_code();
}
