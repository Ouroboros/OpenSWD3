#include "test.hpp"

#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_fixed_tile_writer.hpp"
#include "openswd3/rendering/legacy_packed_row.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitClipRectangle;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyBlitRequest;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyFixedTileWriteStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPackedRowBlendStatus;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

[[nodiscard]] LegacyFramebuffer make_framebuffer() {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 48,
        .width = 24,
        .height = 24,
    }};
}

void append_u16(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

[[nodiscard]] std::array<u8, 512> direct_tile() {
    std::array<u8, 512> bytes{};
    for (std::size_t index = 0U; index < 256U; ++index) {
        const u16 value = static_cast<u16>(0x1000U + index);
        bytes[index * 2U] = static_cast<u8>(value);
        bytes[index * 2U + 1U] = static_cast<u8>(value >> 8U);
    }
    return bytes;
}

void test_direct_fixed_tiles(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    const std::array<u8, 512> source = direct_tile();

    auto status = openswd3::rendering::write_legacy_direct_16x16_tile(
        framebuffer, 3, 4, source
    );
    test.expect_equal(
        status,
        LegacyFixedTileWriteStatus::completed,
        "direct fixed tile completes"
    );
    test.expect_equal(
        framebuffer.row_pixels(4U)[3U],
        static_cast<u16>(0x1000U),
        "direct tile first pixel"
    );
    test.expect_equal(
        framebuffer.row_pixels(19U)[18U],
        static_cast<u16>(0x10FFU),
        "direct tile final pixel"
    );
    test.expect_equal(
        framebuffer.row_pixels(4U)[2U],
        static_cast<u16>(0xA55AU),
        "direct tile preserves outside pixels"
    );

    std::array<u8, 512> keyed{};
    for (std::size_t index = 0U; index < 256U; ++index) {
        keyed[index * 2U] = 0x22U;
        keyed[index * 2U + 1U] = 0x22U;
    }
    keyed[0] = 0x33U;
    keyed[1] = 0x33U;
    keyed[510] = 0x44U;
    keyed[511] = 0x44U;
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    status = openswd3::rendering::write_legacy_direct_keyed_16x16_tile(
        framebuffer, 2, 2, keyed, 0x2222U
    );
    test.expect_equal(
        status,
        LegacyFixedTileWriteStatus::completed,
        "direct keyed tile completes"
    );
    test.expect_equal(
        framebuffer.row_pixels(2U)[2U],
        static_cast<u16>(0x3333U),
        "direct keyed tile writes non-key first pixel"
    );
    test.expect_equal(
        framebuffer.row_pixels(2U)[3U],
        static_cast<u16>(0xA55AU),
        "direct keyed tile preserves key pixel"
    );
    test.expect_equal(
        framebuffer.row_pixels(17U)[17U],
        static_cast<u16>(0x4444U),
        "direct keyed tile writes non-key final pixel"
    );
}

void test_indexed_fixed_tiles(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    std::array<u8, 256> source{};
    std::array<u16, 256> palette{};
    for (std::size_t index = 0U; index < source.size(); ++index) {
        source[index] = static_cast<u8>(index);
        palette[index] = static_cast<u16>(0x2000U + index);
    }

    auto status = openswd3::rendering::write_legacy_indexed_16x16_tile(
        framebuffer, 1, 1, source, palette
    );
    test.expect_equal(
        status,
        LegacyFixedTileWriteStatus::completed,
        "indexed fixed tile completes"
    );
    test.expect_equal(
        framebuffer.row_pixels(1U)[1U],
        static_cast<u16>(0x2000U),
        "indexed tile resolves first palette entry"
    );
    test.expect_equal(
        framebuffer.row_pixels(16U)[16U],
        static_cast<u16>(0x20FFU),
        "indexed tile resolves final palette entry"
    );

    source.fill(1U);
    source.front() = 2U;
    source.back() = 3U;
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    status = openswd3::rendering::write_legacy_indexed_keyed_16x16_tile(
        framebuffer, 4, 3, source, palette
    );
    test.expect_equal(
        status,
        LegacyFixedTileWriteStatus::completed,
        "indexed keyed tile completes"
    );
    test.expect_equal(
        framebuffer.row_pixels(3U)[4U],
        palette[2U],
        "indexed keyed tile writes non-one index"
    );
    test.expect_equal(
        framebuffer.row_pixels(3U)[5U],
        static_cast<u16>(0xA55AU),
        "indexed keyed tile treats index one as transparent"
    );
    test.expect_equal(
        framebuffer.row_pixels(18U)[19U],
        palette[3U],
        "indexed keyed tile writes final non-one index"
    );
}

void test_fixed_tile_safety(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    constexpr std::array<u8, 1> kShortSource{};
    constexpr std::array<u8, 256> kIndexed{};
    constexpr std::array<u16, 1> kShortPalette{};

    test.expect_equal(
        openswd3::rendering::write_legacy_direct_16x16_tile(
            framebuffer, 0, 0, kShortSource
        ),
        LegacyFixedTileWriteStatus::source_out_of_bounds,
        "short direct source is isolated"
    );
    test.expect_equal(
        openswd3::rendering::write_legacy_indexed_16x16_tile(
            framebuffer, 0, 0, kIndexed, kShortPalette
        ),
        LegacyFixedTileWriteStatus::palette_out_of_bounds,
        "short indexed palette is isolated"
    );
    test.expect_equal(
        openswd3::rendering::write_legacy_indexed_16x16_tile(
            framebuffer, 9, 0, kIndexed, std::array<u16, 256>{}
        ),
        LegacyFixedTileWriteStatus::destination_out_of_bounds,
        "unclipped tile destination is isolated"
    );
}

void test_packed_row_formula(openswd3::test::Context& test) {
    const LegacyPixelConversionState format;
    std::array<u16, 3> pixels{0x7FFFU, 0x001FU, 0x1234U};
    const u32 color =
        openswd3::rendering::legacy_pack_color_pair(format, 31, 0, 0);
    auto status =
        openswd3::rendering::blend_legacy_packed_row(pixels, color, 3, format);
    test.expect_equal(
        status, LegacyPackedRowBlendStatus::completed, "packed row completes"
    );
    constexpr std::array<u16, 3> kExpected{0x6A73U, 0x1C13U, 0x1234U};
    test.expect_true(
        std::ranges::equal(pixels, kExpected),
        "packed row follows four-term dword formula and keeps odd tail"
    );

    pixels = {1U, 2U, 3U};
    status =
        openswd3::rendering::blend_legacy_packed_row(pixels, color, 1, format);
    test.expect_equal(
        status,
        LegacyPackedRowBlendStatus::invalid_geometry,
        "one-pixel nonterminating legacy path is isolated"
    );
    test.expect_equal(
        pixels[0], static_cast<u16>(1U), "invalid row does not write"
    );

    status = openswd3::rendering::blend_legacy_packed_row(
        std::span<u16>{pixels}.first(2U), color, 4, format
    );
    test.expect_equal(
        status,
        LegacyPackedRowBlendStatus::destination_out_of_bounds,
        "short packed destination is isolated"
    );
}

[[nodiscard]] std::vector<u8> one_pixel_rle() {
    std::vector<u8> bytes;
    append_u16(bytes, 0xFFFFU);
    append_u16(bytes, 1U);
    append_u16(bytes, 1U);
    append_u16(bytes, 0x10U);
    append_u16(bytes, 8U);
    append_u16(bytes, 1U);
    append_u16(bytes, 0x7777U);
    append_u16(bytes, 0U);
    append_u16(bytes, 0U);
    return bytes;
}

void test_outline_wrapper(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 8,
    }};
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    const std::vector<u8> source = one_pixel_rle();
    constexpr std::array<u8, 4> kColor{0x34U, 0x12U, 0x34U, 0x12U};
    const LegacyBlitClipRectangle clip{
        .width = 8,
        .height = 8,
    };
    LegacyRleRowJitterState jitter;
    LegacyBlitRequest shared_request{
        .destination_x = 3,
        .destination_y = 3,
        .source_width = 1,
        .source_height = 1,
        .target_height = 1,
        .vertical_resample_enlarge_state = 1U,
        .vertical_resample_phase_10_10 = 0x25U,
        .opacity_step = 7,
        .auxiliary = kColor,
    };
    LegacyBlitEffectState shared_effects;

    const auto result = openswd3::rendering::blit_legacy_outline_copy_paths(
        framebuffer,
        clip,
        LegacyBlitSource{.bytes = source},
        shared_request,
        shared_effects,
        jitter
    );

    test.expect_equal(
        result.pass_count, u32{4U}, "all four outline passes execute"
    );
    for (const auto& pass : result.passes) {
        test.expect_equal(
            pass.status,
            LegacyBlitExecutionStatus::completed,
            "each outline pass completes"
        );
    }
    test.expect_equal(
        result.passes[0].selection.effective_flags,
        0x80000024U,
        "outline forces RLE constant-fill flags"
    );
    for (const unsigned int y : {2U, 4U}) {
        for (const std::size_t x : {2U, 4U}) {
            test.expect_equal(
                framebuffer.row_pixels(y)[x],
                static_cast<u16>(0x1234U),
                "outline writes four diagonal offsets"
            );
        }
    }
    test.expect_equal(
        framebuffer.row_pixels(3U)[3U],
        static_cast<u16>(0xA55AU),
        "outline wrapper does not draw the center"
    );
    test.expect_true(
        shared_request.target_height == 0 &&
            shared_request.vertical_resample_phase_10_10 == 0U &&
            shared_request.opacity_step == 0 &&
            shared_request.vertical_resample_enlarge_state == 1U,
        "outline propagates each accepted blit common epilogue"
    );

    LegacyBlitRequest stopped_request{
        .destination_x = 3,
        .destination_y = 3,
        .source_width = 1,
        .source_height = 1,
        .target_height = 2,
        .vertical_resample_phase_10_10 = 0x44U,
        .opacity_step = 9,
        .auxiliary = kColor,
    };
    LegacyBlitEffectState stopped_effects{.red_offset = 3};
    const auto stopped = openswd3::rendering::blit_legacy_outline_copy_paths(
        framebuffer,
        clip,
        LegacyBlitSource{},
        stopped_request,
        stopped_effects,
        jitter
    );
    test.expect_true(
        stopped.pass_count == 1U &&
            stopped.passes[0].status ==
                LegacyBlitExecutionStatus::malformed_source &&
            stopped_request.target_height == 2 &&
            stopped_request.vertical_resample_phase_10_10 == 0x44U &&
            stopped_request.opacity_step == 9 &&
            stopped_effects.red_offset == 3,
        "outline typed stop preserves entry state and skips remaining passes"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_direct_fixed_tiles(test);
    test_indexed_fixed_tiles(test);
    test_fixed_tile_safety(test);
    test_packed_row_formula(test);
    test_outline_wrapper(test);
    return test.exit_code();
}
