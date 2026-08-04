#include "test.hpp"

#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitClipRectangle;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitRequest;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyBlitSourceLayout;
using openswd3::rendering::LegacyBlitterRoutine;
using openswd3::rendering::LegacyBlitterSelectionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

void append_u16(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value & 0xFFU));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

[[nodiscard]] std::vector<u8> direct_pixels(
    const std::span<const u16> pixels
) {
    std::vector<u8> bytes;
    bytes.reserve(pixels.size() * 2U);
    for (const u16 pixel : pixels) {
        append_u16(bytes, pixel);
    }

    return bytes;
}

[[nodiscard]] std::vector<u8> make_rle(
    const u16 width,
    const u16 height,
    const std::span<const std::vector<u16>> rows,
    const u16 source_flags = 0x10U
) {
    std::vector<u8> bytes;
    append_u16(bytes, 0xFFFFU);
    append_u16(bytes, width);
    append_u16(bytes, height);
    append_u16(bytes, source_flags);
    for (const std::vector<u16>& row : rows) {
        append_u16(
            bytes,
            static_cast<u16>(2U + row.size() * sizeof(u16))
        );
        for (const u16 word : row) {
            append_u16(bytes, word);
        }
    }

    append_u16(bytes, 0U);
    return bytes;
}

[[nodiscard]] LegacyBlitClipRectangle full_clip(
    const LegacyFramebuffer& framebuffer
) {
    const auto& geometry = framebuffer.geometry();
    return LegacyBlitClipRectangle{
        .left = geometry.clip_left,
        .top = geometry.clip_top,
        .width = geometry.clip_width,
        .height = geometry.clip_height,
    };
}

[[nodiscard]] LegacyBlitExecutionStatus blit(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) {
    return openswd3::rendering::blit_legacy_copy_paths(
        framebuffer,
        full_clip(framebuffer),
        source,
        request,
        effects,
        jitter
    ).status;
}

[[nodiscard]] LegacyBlitExecutionStatus blit(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    LegacyRleRowJitterState& jitter
) {
    return blit(
        framebuffer,
        source,
        request,
        LegacyBlitEffectState{},
        jitter
    );
}

void test_sparse_dispatch_table(openswd3::test::Context& test) {
    using Assignment = std::pair<u32, LegacyBlitterRoutine>;
    constexpr std::array<Assignment, 43> kAssignments{
        Assignment{0x00U, LegacyBlitterRoutine::rle_copy_forward},
        Assignment{0x01U, LegacyBlitterRoutine::rle_copy_reverse},
        Assignment{0x02U, LegacyBlitterRoutine::rle_copy_forward},
        Assignment{0x03U, LegacyBlitterRoutine::rle_copy_reverse},
        Assignment{0x04U, LegacyBlitterRoutine::rle_saturated_add_forward},
        Assignment{0x05U, LegacyBlitterRoutine::rle_saturated_add_reverse},
        Assignment{0x08U, LegacyBlitterRoutine::rle_coverage_forward},
        Assignment{0x09U, LegacyBlitterRoutine::rle_coverage_reverse},
        Assignment{0x0CU, LegacyBlitterRoutine::rle_shifted_resample_forward},
        Assignment{0x0DU, LegacyBlitterRoutine::rle_shifted_resample_reverse},
        Assignment{0x0EU, LegacyBlitterRoutine::rle_shifted_resample_forward},
        Assignment{0x0FU, LegacyBlitterRoutine::rle_shifted_resample_reverse},
        Assignment{0x10U, LegacyBlitterRoutine::rle_destination_offset_forward},
        Assignment{0x11U, LegacyBlitterRoutine::rle_destination_offset_reverse},
        Assignment{0x14U, LegacyBlitterRoutine::rle_opacity_forward},
        Assignment{0x15U, LegacyBlitterRoutine::rle_opacity_reverse},
        Assignment{0x16U, LegacyBlitterRoutine::rle_opacity_forward},
        Assignment{0x17U, LegacyBlitterRoutine::rle_opacity_reverse},
        Assignment{0x18U, LegacyBlitterRoutine::rle_copy_with_edges_forward},
        Assignment{0x19U, LegacyBlitterRoutine::rle_copy_with_edges_reverse},
        Assignment{0x1CU, LegacyBlitterRoutine::rle_vertical_opacity_fade},
        Assignment{0x20U, LegacyBlitterRoutine::rle_saturated_resample_forward},
        Assignment{0x21U, LegacyBlitterRoutine::rle_saturated_resample_reverse},
        Assignment{0x24U, LegacyBlitterRoutine::rle_constant_fill_forward},
        Assignment{0x25U, LegacyBlitterRoutine::rle_constant_fill_reverse},
        Assignment{0x26U, LegacyBlitterRoutine::rle_constant_fill_forward},
        Assignment{0x27U, LegacyBlitterRoutine::rle_constant_fill_reverse},
        Assignment{0x28U, LegacyBlitterRoutine::rle_grayscale_forward},
        Assignment{0x29U, LegacyBlitterRoutine::rle_grayscale_reverse},
        Assignment{0x2AU, LegacyBlitterRoutine::rle_grayscale_forward},
        Assignment{0x2BU, LegacyBlitterRoutine::rle_grayscale_reverse},
        Assignment{0x2CU, LegacyBlitterRoutine::rle_saturated_subtract_forward},
        Assignment{0x2DU, LegacyBlitterRoutine::rle_saturated_subtract_reverse},
        Assignment{0x30U, LegacyBlitterRoutine::rle_smear_forward},
        Assignment{0x31U, LegacyBlitterRoutine::rle_smear_reverse},
        Assignment{0x32U, LegacyBlitterRoutine::rle_smear_forward},
        Assignment{0x33U, LegacyBlitterRoutine::rle_smear_reverse},
        Assignment{0x80U, LegacyBlitterRoutine::raw_copy_forward},
        Assignment{0x81U, LegacyBlitterRoutine::raw_copy_reverse},
        Assignment{0x84U, LegacyBlitterRoutine::raw_color_key_copy_forward},
        Assignment{0x85U, LegacyBlitterRoutine::raw_color_key_copy_reverse},
        Assignment{0x88U, LegacyBlitterRoutine::raw_constant_vertical_fade},
        Assignment{0x94U, LegacyBlitterRoutine::raw_opacity_forward},
    };
    for (const auto& [slot, expected] : kAssignments) {
        test.expect_equal(
            openswd3::rendering::legacy_blitter_routine(slot),
            expected,
            "exact sparse slot assignment"
        );
    }

    std::size_t assigned_count{};
    std::set<u32> unique_routines;
    for (u32 slot = 0U; slot < 256U; ++slot) {
        const LegacyBlitterRoutine routine =
            openswd3::rendering::legacy_blitter_routine(slot);
        if (routine == LegacyBlitterRoutine::unassigned) {
            continue;
        }

        ++assigned_count;
        unique_routines.insert(static_cast<u32>(routine));
    }

    test.expect_equal(
        assigned_count,
        static_cast<std::size_t>(43U),
        "exact assigned slot count"
    );
    test.expect_equal(
        unique_routines.size(),
        static_cast<std::size_t>(31U),
        "exact unique routine count"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x00U),
        LegacyBlitterRoutine::rle_copy_forward,
        "RLE forward copy slot"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x02U),
        LegacyBlitterRoutine::rle_copy_forward,
        "vertical flip shares the RLE forward routine"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x80U),
        LegacyBlitterRoutine::raw_copy_forward,
        "raw forward copy slot"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x06U),
        LegacyBlitterRoutine::unassigned,
        "sparse low-family hole"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x82U),
        LegacyBlitterRoutine::unassigned,
        "raw vertical-flip hole"
    );
    test.expect_equal(
        openswd3::rendering::legacy_blitter_routine(0x100U),
        LegacyBlitterRoutine::unassigned,
        "out-of-table slot has no fallback"
    );
}

void test_selection_rules(openswd3::test::Context& test) {
    const auto rle = openswd3::rendering::select_legacy_blitter(
        0xFFFFU,
        false,
        0U,
        0
    );
    test.expect_equal(
        rle.status,
        LegacyBlitterSelectionStatus::selected,
        "FFFF direct source selects a routine"
    );
    test.expect_true(rle.rle_family, "FFFF direct source selects RLE family");
    test.expect_equal(rle.table_slot, 0U, "RLE copy slot");

    const auto indexed = openswd3::rendering::select_legacy_blitter(
        0xFFFFU,
        true,
        0U,
        0
    );
    test.expect_false(
        indexed.rle_family,
        "nonzero palette pointer suppresses automatic RLE marker"
    );
    test.expect_equal(indexed.table_slot, 0x80U, "indexed raw copy slot");

    const auto disabled = openswd3::rendering::select_legacy_blitter(
        0xFFFFU,
        false,
        0x14U,
        0
    );
    test.expect_equal(
        disabled.status,
        LegacyBlitterSelectionStatus::opacity_disabled,
        "nonpositive opacity skips the complete draw"
    );

    const auto copy = openswd3::rendering::select_legacy_blitter(
        0xFFFFU,
        false,
        0x16U,
        16
    );
    test.expect_equal(copy.table_slot, 0x02U, "opacity above 15 keeps flips");
    test.expect_equal(
        copy.routine,
        LegacyBlitterRoutine::rle_copy_forward,
        "opacity above 15 degrades to copy"
    );

    const auto hole = openswd3::rendering::select_legacy_blitter(
        0xFFFFU,
        false,
        0x06U,
        0
    );
    test.expect_equal(
        hole.status,
        LegacyBlitterSelectionStatus::unassigned,
        "unassigned combination remains a hole"
    );
}

void test_raw_direct_copy_and_clipping(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 6,
        .height = 5,
    });
    std::ranges::fill(framebuffer.physical_pixels(), 0xEEEEU);

    std::array<u16, 56> pixels{};
    for (std::size_t index = 0U; index < pixels.size(); ++index) {
        pixels[index] = static_cast<u16>(index + 1U);
    }

    const std::vector<u8> source_bytes = direct_pixels(pixels);
    LegacyRleRowJitterState jitter{};
    const LegacyBlitExecutionStatus status = blit(
        framebuffer,
        LegacyBlitSource{.bytes = source_bytes},
        LegacyBlitRequest{
            .destination_x = -1,
            .destination_y = -1,
            .source_width = 8,
            .source_height = 7,
        },
        jitter
    );
    test.expect_equal(status, LegacyBlitExecutionStatus::completed, "raw copy");

    const std::span<const u16> physical = framebuffer.physical_pixels();
    for (std::size_t row = 0U; row < 5U; ++row) {
        for (std::size_t column = 0U; column < 6U; ++column) {
            const u16 expected = static_cast<u16>(row * 8U + column + 10U);
            test.expect_equal(
                physical[row * 8U + column],
                expected,
                "raw source window after four-edge clipping"
            );
        }

        test.expect_equal(
            physical[row * 8U + 6U],
            static_cast<u16>(0xEEEEU),
            "first padding pixel is untouched"
        );
        test.expect_equal(
            physical[row * 8U + 7U],
            static_cast<u16>(0xEEEEU),
            "second padding pixel is untouched"
        );
    }
}

void test_raw_reverse_and_indexed_paths(openswd3::test::Context& test) {
    constexpr std::array<u16, 8> kDirectPixels{
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,
    };
    const std::vector<u8> direct_source = direct_pixels(kDirectPixels);
    LegacyFramebuffer reverse_framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 2,
    });
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            reverse_framebuffer,
            LegacyBlitSource{.bytes = direct_source},
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 2,
                .flags = 1U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw reverse copy"
    );
    constexpr std::array<u16, 8> kExpectedReverse{
        4U, 3U, 2U, 1U, 8U, 7U, 6U, 5U,
    };
    test.expect_true(
        std::ranges::equal(
            reverse_framebuffer.physical_pixels(),
            kExpectedReverse
        ),
        "raw reverse reads each 16-bit row backwards"
    );

    std::array<u16, 256> palette{};
    palette[1] = 0x1111U;
    palette[2] = 0x2222U;
    palette[3] = 0x3333U;
    constexpr std::array<u8, 3> kIndices{1U, 2U, 3U};
    LegacyFramebuffer indexed_framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 6,
        .width = 3,
        .height = 1,
    });
    test.expect_equal(
        blit(
            indexed_framebuffer,
            LegacyBlitSource{
                .bytes = kIndices,
                .layout = LegacyBlitSourceLayout::indexed_8,
                .palette = palette,
            },
            LegacyBlitRequest{
                .source_width = 3,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "indexed raw forward copy"
    );
    constexpr std::array<u16, 3> kExpectedIndexed{
        0x1111U, 0x2222U, 0x3333U,
    };
    test.expect_true(
        std::ranges::equal(
            indexed_framebuffer.physical_pixels(),
            kExpectedIndexed
        ),
        "indexed forward path performs palette lookup"
    );

    constexpr std::array<u8, 8> kIndexedReverseBytes{
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,
    };
    LegacyFramebuffer indexed_reverse_framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    test.expect_equal(
        blit(
            indexed_reverse_framebuffer,
            LegacyBlitSource{
                .bytes = kIndexedReverseBytes,
                .layout = LegacyBlitSourceLayout::indexed_8,
                .palette = palette,
            },
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 1,
                .flags = 1U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "indexed reverse legacy path"
    );
    test.expect_equal(
        indexed_reverse_framebuffer.physical_pixels().front(),
        static_cast<u16>(0x0504U),
        "indexed reverse still performs an unaligned 16-bit source read"
    );
}

void test_raw_color_key_copy(openswd3::test::Context& test) {
    constexpr std::array<u16, 4> kDirectPixels{
        0x026BU, 0x1234U, 0x026BU, 0x7FFFU,
    };
    const std::vector<u8> direct_source = direct_pixels(kDirectPixels);
    LegacyFramebuffer forward(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    std::ranges::fill(forward.physical_pixels(), 0xAAAAU);
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = direct_source},
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 1,
                .flags = 4U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw direct color-key copy"
    );
    constexpr std::array<u16, 4> kExpectedForward{
        0xAAAAU, 0x1234U, 0xAAAAU, 0x7FFFU,
    };
    test.expect_true(
        std::ranges::equal(forward.physical_pixels(), kExpectedForward),
        "forward direct path compares against the converted low-word key"
    );

    std::array<u16, 4> palette{};
    palette[1] = 0x1111U;
    palette[2] = 0x2222U;
    palette[3] = 0x3333U;
    constexpr std::array<u8, 4> kIndices{1U, 2U, 1U, 3U};
    LegacyFramebuffer indexed(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    std::ranges::fill(indexed.physical_pixels(), 0xBBBBU);
    test.expect_equal(
        blit(
            indexed,
            LegacyBlitSource{
                .bytes = kIndices,
                .layout = LegacyBlitSourceLayout::indexed_8,
                .palette = palette,
            },
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 1,
                .flags = 4U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw indexed color-key copy"
    );
    constexpr std::array<u16, 4> kExpectedIndexed{
        0xBBBBU, 0x2222U, 0xBBBBU, 0x3333U,
    };
    test.expect_true(
        std::ranges::equal(indexed.physical_pixels(), kExpectedIndexed),
        "indexed color-key path treats palette index one as transparent"
    );

    constexpr std::array<u8, 2> kTransparentIndices{1U, 1U};
    LegacyFramebuffer transparent_without_palette(LegacySurfaceGeometry{
        .pitch_bytes = 4,
        .width = 2,
        .height = 1,
    });
    std::ranges::fill(
        transparent_without_palette.physical_pixels(),
        0xCAFEU
    );
    test.expect_equal(
        blit(
            transparent_without_palette,
            LegacyBlitSource{
                .bytes = kTransparentIndices,
                .layout = LegacyBlitSourceLayout::indexed_8,
            },
            LegacyBlitRequest{
                .source_width = 2,
                .source_height = 1,
                .flags = 4U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw transparent index does not read the palette"
    );
    constexpr std::array<u16, 2> kExpectedTransparent{0xCAFEU, 0xCAFEU};
    test.expect_true(
        std::ranges::equal(
            transparent_without_palette.physical_pixels(),
            kExpectedTransparent
        ),
        "index one is discarded before palette lookup"
    );

    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    std::ranges::fill(reverse.physical_pixels(), 0xCCCCU);
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = direct_source},
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 1,
                .flags = 5U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw reverse color-key copy"
    );
    constexpr std::array<u16, 4> kExpectedReverse{
        0x7FFFU, 0x026BU, 0x1234U, 0x026BU,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "reverse path preserves the original 16-bit versus duplicated 32-bit key comparison bug"
    );

    constexpr std::array<u16, 2> kRgb565Pixels{0x04CBU, 0x026BU};
    const std::vector<u8> rgb565_source = direct_pixels(kRgb565Pixels);
    LegacyFramebuffer rgb565(LegacySurfaceGeometry{
        .pitch_bytes = 4,
        .width = 2,
        .height = 1,
    });
    std::ranges::fill(rgb565.physical_pixels(), 0xDDDDU);
    LegacyBlitEffectState effects{};
    openswd3::rendering::select_legacy_pixel_conversion(
        effects.pixel_conversion,
        LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    test.expect_equal(
        blit(
            rgb565,
            LegacyBlitSource{.bytes = rgb565_source},
            LegacyBlitRequest{
                .source_width = 2,
                .source_height = 1,
                .flags = 4U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "raw converted color key"
    );
    constexpr std::array<u16, 2> kExpectedRgb565{0xDDDDU, 0x026BU};
    test.expect_true(
        std::ranges::equal(rgb565.physical_pixels(), kExpectedRgb565),
        "color key is converted from RGB555 before the low-word comparison"
    );
}

void test_sparse_and_unsupported_execution(openswd3::test::Context& test) {
    constexpr std::array<u16, 4> kPixels{1U, 2U, 3U, 4U};
    const std::vector<u8> source_bytes = direct_pixels(kPixels);
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 4,
        .width = 2,
        .height = 2,
    });
    std::ranges::fill(framebuffer.physical_pixels(), 0x7777U);
    LegacyRleRowJitterState jitter{};

    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source_bytes},
            LegacyBlitRequest{
                .source_width = 2,
                .source_height = 2,
                .flags = 2U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::unassigned_routine,
        "raw vertical flip remains unassigned"
    );
    test.expect_true(
        std::ranges::all_of(
            framebuffer.physical_pixels(),
            [](const u16 pixel) { return pixel == 0x7777U; }
        ),
        "unassigned slot does not fall back to copy"
    );

    const auto clipped_hole = openswd3::rendering::blit_legacy_copy_paths(
        framebuffer,
        full_clip(framebuffer),
        LegacyBlitSource{.bytes = source_bytes},
        LegacyBlitRequest{
            .destination_x = -10,
            .source_width = 2,
            .source_height = 2,
            .flags = 6U,
        },
        LegacyBlitEffectState{},
        jitter
    );
    test.expect_equal(
        clipped_hole.status,
        LegacyBlitExecutionStatus::clipped_out,
        "fully clipped hole never reaches the legacy null call"
    );
    test.expect_equal(
        clipped_hole.selection.status,
        LegacyBlitterSelectionStatus::unassigned,
        "clipped result still reports the selected sparse hole"
    );

    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source_bytes},
            LegacyBlitRequest{
                .source_width = 2,
                .source_height = 2,
                .flags = 8U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::unsupported_routine,
        "a still-unimplemented assigned effect remains explicit"
    );
}

[[nodiscard]] std::vector<u8> synthetic_rle_copy_source() {
    const std::vector<u16> row{
        0x4001U,
        0x0003U, 0x0000U, 0xFFFFU, 0x1234U,
        0x8001U,
        0xC001U,
        0x0002U, 0x5678U, 0x9ABCU,
        0x0000U,
    };
    const std::array<std::vector<u16>, 1> rows{row};
    return make_rle(8U, 1U, rows);
}

void test_rle_spans_and_horizontal_direction(
    openswd3::test::Context& test
) {
    const std::vector<u8> source = synthetic_rle_copy_source();
    LegacyRleRowJitterState jitter{};
    LegacyFramebuffer forward(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    std::ranges::fill(forward.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE forward copy"
    );
    constexpr std::array<u16, 8> kExpectedForward{
        0xA55AU, 0x0000U, 0xFFFFU, 0x1234U,
        0xA55AU, 0xA55AU, 0x5678U, 0x9ABCU,
    };
    test.expect_true(
        std::ranges::equal(forward.physical_pixels(), kExpectedForward),
        "all three high command forms skip while literal zero and FFFF copy"
    );

    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    std::ranges::fill(reverse.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 1U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE reverse copy"
    );
    constexpr std::array<u16, 8> kExpectedReverse{
        0x9ABCU, 0x5678U, 0xA55AU, 0xA55AU,
        0x1234U, 0xFFFFU, 0x0000U, 0xA55AU,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "RLE source stays forward while destination runs backwards"
    );

    LegacyFramebuffer clipped(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    std::ranges::fill(clipped.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            clipped,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = -2,
                .source_width = 8,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "clipped RLE copy"
    );
    constexpr std::array<u16, 4> kExpectedClipped{
        0xFFFFU, 0x1234U, 0xA55AU, 0xA55AU,
    };
    test.expect_true(
        std::ranges::equal(clipped.physical_pixels(), kExpectedClipped),
        "RLE clipping consumes source prefixes and span coverage exactly"
    );
}

void test_rle_run_edge_copy(openswd3::test::Context& test) {
    constexpr u16 kRgb555Edge = 0x03E0U;
    const std::array<std::vector<u16>, 1> split_rows{
        std::vector<u16>{
            0x0004U,
            0x1111U,
            0x2222U,
            0x3333U,
            0x4444U,
            0x4002U,
            0x0003U,
            0x5555U,
            0x6666U,
            0x7777U,
            0x0000U,
        },
    };
    const std::vector<u8> split_source = make_rle(9U, 1U, split_rows);
    LegacyFramebuffer forward(LegacySurfaceGeometry{
        .pitch_bytes = 18,
        .width = 9,
        .height = 1,
    });
    std::array<i32, 33> zero_offsets{};
    LegacyRleRowJitterState jitter{
        .group = 1,
        .offsets = zero_offsets,
    };
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = split_source},
            LegacyBlitRequest{
                .source_width = 9,
                .source_height = 1,
                .flags = 0x18U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE run-edge copy forward"
    );
    constexpr std::array<u16, 9> kExpectedForward{
        kRgb555Edge,
        0x2222U,
        0x3333U,
        kRgb555Edge,
        0x0000U,
        0x0000U,
        kRgb555Edge,
        0x6666U,
        kRgb555Edge,
    };
    test.expect_true(
        std::ranges::equal(forward.physical_pixels(), kExpectedForward),
        "forward mode replaces both visible ends of every literal run"
    );
    test.expect_equal(
        jitter.phase_bytes,
        4U,
        "forward run-edge copy advances the 0x84-byte jitter phase"
    );

    const std::array<std::vector<u16>, 1> clipped_rows{
        std::vector<u16>{
            0x0004U,
            0x1111U,
            0x2222U,
            0x3333U,
            0x4444U,
            0x0000U,
        },
    };
    const std::vector<u8> clipped_source = make_rle(
        4U,
        1U,
        clipped_rows
    );
    LegacyFramebuffer clipped(LegacySurfaceGeometry{
        .pitch_bytes = 6,
        .width = 3,
        .height = 1,
    });
    test.expect_equal(
        blit(
            clipped,
            LegacyBlitSource{.bytes = clipped_source},
            LegacyBlitRequest{
                .destination_x = -1,
                .source_width = 4,
                .source_height = 1,
                .flags = 0x18U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "clipped RLE run-edge copy"
    );
    constexpr std::array<u16, 3> kExpectedClipped{
        kRgb555Edge, 0x3333U, kRgb555Edge,
    };
    test.expect_true(
        std::ranges::equal(clipped.physical_pixels(), kExpectedClipped),
        "a clipped literal prefix makes the first actually written pixel the edge"
    );

    const std::array<std::vector<u16>, 1> reverse_rows{
        std::vector<u16>{
            0x0003U,
            0x1111U,
            0x2222U,
            0x3333U,
            0x0000U,
        },
    };
    const std::vector<u8> reverse_source = make_rle(
        3U,
        1U,
        reverse_rows
    );
    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 10,
        .width = 5,
        .height = 1,
    });
    std::ranges::fill(reverse.physical_pixels(), 0xA55AU);
    jitter.phase_bytes = 0U;
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = reverse_source},
            LegacyBlitRequest{
                .destination_x = 1,
                .source_width = 3,
                .source_height = 1,
                .flags = 0x19U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE run-edge copy reverse"
    );
    constexpr std::array<u16, 5> kExpectedReverse{
        kRgb555Edge, 0x3333U, 0x2222U, kRgb555Edge, 0xA55AU,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "reverse mode preserves the original off-by-one edge write to the pixel left of the literal run"
    );
    test.expect_equal(
        jitter.phase_bytes,
        0U,
        "reverse run-edge copy preserves the jitter phase"
    );

    LegacyBlitEffectState rgb565_effects{};
    openswd3::rendering::select_legacy_pixel_conversion(
        rgb565_effects.pixel_conversion,
        LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    LegacyFramebuffer rgb565(LegacySurfaceGeometry{
        .pitch_bytes = 6,
        .width = 3,
        .height = 1,
    });
    test.expect_equal(
        blit(
            rgb565,
            LegacyBlitSource{.bytes = reverse_source},
            LegacyBlitRequest{
                .source_width = 3,
                .source_height = 1,
                .flags = 0x18U,
            },
            rgb565_effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RGB565 RLE run-edge copy"
    );
    constexpr std::array<u16, 3> kExpectedRgb565{
        0x0F80U, 0x2222U, 0x0F80U,
    };
    test.expect_true(
        std::ranges::equal(rgb565.physical_pixels(), kExpectedRgb565),
        "run-edge color applies the selected forward conversion to the effective green mask"
    );
}

void test_rle_vertical_flip_row_base(openswd3::test::Context& test) {
    const std::array<std::vector<u16>, 2> rows{
        std::vector<u16>{0x0002U, 0x1111U, 0x2222U, 0x0000U},
        std::vector<u16>{0x0002U, 0x3333U, 0x4444U, 0x0000U},
    };
    const std::vector<u8> source = make_rle(2U, 2U, rows);
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 4,
    });
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = 1,
                .source_width = 2,
                .source_height = 2,
                .flags = 2U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE vertical flip"
    );

    test.expect_equal(
        framebuffer.row_pixels(0U)[1],
        static_cast<u16>(0xA55AU),
        "vertical flip preserves the legacy one-row destination offset"
    );
    test.expect_equal(
        framebuffer.row_pixels(2U)[1],
        static_cast<u16>(0x1111U),
        "first source row starts at row_offsets[visible_height]"
    );
    test.expect_equal(
        framebuffer.row_pixels(1U)[1],
        static_cast<u16>(0x3333U),
        "second source row steps upward by pitch"
    );
}

void test_rle_jitter_and_header_gate(openswd3::test::Context& test) {
    const std::array<std::vector<u16>, 2> rows{
        std::vector<u16>{0x0001U, 0x1111U, 0x0000U},
        std::vector<u16>{0x0001U, 0x2222U, 0x0000U},
    };
    const std::vector<u8> source = make_rle(1U, 2U, rows);
    std::array<i32, 33> offsets{};
    offsets[1] = 1;
    offsets[2] = -1;
    LegacyRleRowJitterState jitter{
        .group = 1,
        .phase_bytes = 0U,
        .offsets = offsets,
    };
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 3,
    });
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = 1,
                .source_width = 1,
                .source_height = 2,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "jittered RLE forward copy"
    );
    test.expect_equal(
        framebuffer.row_pixels(0U)[2],
        static_cast<u16>(0x1111U),
        "first row uses phase plus four bytes"
    );
    test.expect_equal(
        framebuffer.row_pixels(1U)[0],
        static_cast<u16>(0x2222U),
        "second row advances to the next signed offset"
    );
    test.expect_equal(jitter.phase_bytes, 4U, "forward routine advances phase");

    std::ranges::fill(framebuffer.physical_pixels(), 0U);
    jitter.phase_bytes = 0U;
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = 1,
                .source_width = 1,
                .source_height = 2,
                .flags = 1U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "jittered RLE reverse copy"
    );
    test.expect_equal(
        jitter.phase_bytes,
        0U,
        "reverse routine does not advance the global phase"
    );

    const std::array<std::vector<u16>, 0> no_rows{};
    const std::vector<u8> gated_source = make_rle(
        1U,
        1U,
        no_rows,
        0U
    );
    std::ranges::fill(framebuffer.physical_pixels(), 0xBEEFU);
    jitter.phase_bytes = 0U;
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = gated_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE header gate returns normally"
    );
    test.expect_true(
        std::ranges::all_of(
            framebuffer.physical_pixels(),
            [](const u16 pixel) { return pixel == 0xBEEFU; }
        ),
        "header without bit 0x10 writes no pixels"
    );
    test.expect_equal(
        jitter.phase_bytes,
        4U,
        "forward gate exit still advances jitter phase"
    );
}

void test_rle_destination_offset(openswd3::test::Context& test) {
    const std::vector<u8> source = synthetic_rle_copy_source();
    LegacyBlitEffectState effects{
        .red_offset = 2,
        .green_offset = -3,
        .blue_offset = 4,
    };
    LegacyRleRowJitterState jitter{};
    LegacyFramebuffer forward(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    constexpr std::array<u16, 8> kInitial{
        0xA55AU, 0x0000U, 0x7FFFU, 0x1441U,
        0xA55AU, 0xA55AU, 0x4210U, 0x001EU,
    };
    std::ranges::copy(kInitial, forward.physical_pixels().begin());
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x10U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE destination offset forward"
    );
    constexpr std::array<u16, 8> kExpectedForward{
        0xA55AU, 0x0804U, 0x7F9FU, 0x1C05U,
        0xA55AU, 0xA55AU, 0x49B4U, 0x081FU,
    };
    test.expect_true(
        std::ranges::equal(forward.physical_pixels(), kExpectedForward),
        "destination offset clamps each five-bit channel and ignores source colors"
    );

    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    effects.red_offset = 1;
    effects.green_offset = 1;
    effects.blue_offset = 1;
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x11U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE destination offset reverse"
    );
    constexpr std::array<u16, 8> kExpectedReverse{
        0x0421U, 0x0421U, 0x0000U, 0x0000U,
        0x0421U, 0x0421U, 0x0421U, 0x0000U,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "reverse offset follows source coverage while walking the target backwards"
    );

    LegacyFramebuffer clipped_forward(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    test.expect_equal(
        blit(
            clipped_forward,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = -2,
                .source_width = 8,
                .source_height = 1,
                .flags = 0x10U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "clipped forward destination offset"
    );
    constexpr std::array<u16, 4> kExpectedClippedForward{
        0x0421U, 0x0421U, 0x0000U, 0x0000U,
    };
    test.expect_true(
        std::ranges::equal(
            clipped_forward.physical_pixels(),
            kExpectedClippedForward
        ),
        "forward effect clipping consumes the literal source prefix"
    );

    LegacyFramebuffer clipped_reverse(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    test.expect_equal(
        blit(
            clipped_reverse,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = -2,
                .source_width = 8,
                .source_height = 1,
                .flags = 0x11U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "clipped reverse destination offset"
    );
    constexpr std::array<u16, 4> kExpectedClippedReverse{
        0x0000U, 0x0000U, 0x0421U, 0x0421U,
    };
    test.expect_true(
        std::ranges::equal(
            clipped_reverse.physical_pixels(),
            kExpectedClippedReverse
        ),
        "reverse effect clipping applies the source window backwards"
    );

    constexpr std::array<u8, 2> kMarkerOnly{0xFFU, 0xFFU};
    LegacyFramebuffer zero_offset(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    zero_offset.physical_pixels().front() = 0xBEEFU;
    LegacyRleRowJitterState inaccessible_jitter{
        .group = 1,
        .phase_bytes = 20U,
    };
    test.expect_equal(
        blit(
            zero_offset,
            LegacyBlitSource{.bytes = kMarkerOnly},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 0x10U,
            },
            LegacyBlitEffectState{},
            inaccessible_jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "all-zero destination offset exits before the RLE header"
    );
    test.expect_equal(
        zero_offset.physical_pixels().front(),
        static_cast<u16>(0xBEEFU),
        "all-zero destination offset touches no target pixel"
    );
    test.expect_equal(
        inaccessible_jitter.phase_bytes,
        20U,
        "all-zero destination offset does not advance jitter phase"
    );
}

void test_rle_constant_fill(openswd3::test::Context& test) {
    const std::vector<u8> source = synthetic_rle_copy_source();
    constexpr std::array<u8, 4> kFillBytes{0x34U, 0x12U, 0x78U, 0x56U};
    LegacyBlitEffectState effects{};
    LegacyRleRowJitterState jitter{};
    LegacyFramebuffer forward(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    std::ranges::fill(forward.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x24U,
                .auxiliary = kFillBytes,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE constant fill forward"
    );
    constexpr std::array<u16, 8> kExpectedForward{
        0xA55AU, 0x1234U, 0x1234U, 0x1234U,
        0xA55AU, 0xA55AU, 0x1234U, 0x1234U,
    };
    test.expect_true(
        std::ranges::equal(forward.physical_pixels(), kExpectedForward),
        "constant fill uses the low word of the four-byte auxiliary value"
    );

    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    std::ranges::fill(reverse.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x25U,
                .auxiliary = kFillBytes,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE constant fill reverse"
    );
    constexpr std::array<u16, 8> kExpectedReverse{
        0x1234U, 0x1234U, 0xA55AU, 0xA55AU,
        0x1234U, 0x1234U, 0x1234U, 0xA55AU,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "reverse constant fill preserves sparse source coverage"
    );

    constexpr std::array<u8, 2> kShortAuxiliary{0x34U, 0x12U};
    test.expect_equal(
        blit(
            forward,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x24U,
                .auxiliary = kShortAuxiliary,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::auxiliary_out_of_bounds,
        "constant fill exposes the original four-byte auxiliary read boundary"
    );
}

void test_rle_grayscale(openswd3::test::Context& test) {
    const std::array<std::vector<u16>, 1> rows{
        std::vector<u16>{
            0x0005U,
            0x1111U,
            0x2222U,
            0x3333U,
            0x4444U,
            0x5555U,
            0x0000U,
        },
    };
    const std::vector<u8> source = make_rle(5U, 1U, rows);
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 10,
        .width = 5,
        .height = 1,
    });
    constexpr std::array<u16, 5> kInitial{
        0x0000U, 0x7FFFU, 0x7C00U, 0x03E0U, 0x001FU,
    };
    std::ranges::copy(kInitial, framebuffer.physical_pixels().begin());
    LegacyBlitEffectState effects{};
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .source_width = 5,
                .source_height = 1,
                .flags = 0x28U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE grayscale forward"
    );
    constexpr std::array<u16, 5> kExpected{
        0x0000U, 0x5EF7U, 0x1CE7U, 0x1CE7U, 0x1CE7U,
    };
    test.expect_true(
        std::ranges::equal(framebuffer.physical_pixels(), kExpected),
        "grayscale sums three five-bit target channels and divides by four"
    );

    const std::vector<u8> sparse_source = synthetic_rle_copy_source();
    LegacyFramebuffer reverse(LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 8,
        .height = 1,
    });
    std::ranges::fill(reverse.physical_pixels(), 0x7FFFU);
    test.expect_equal(
        blit(
            reverse,
            LegacyBlitSource{.bytes = sparse_source},
            LegacyBlitRequest{
                .source_width = 8,
                .source_height = 1,
                .flags = 0x29U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE grayscale reverse"
    );
    constexpr std::array<u16, 8> kExpectedReverse{
        0x5EF7U, 0x5EF7U, 0x7FFFU, 0x7FFFU,
        0x5EF7U, 0x5EF7U, 0x5EF7U, 0x7FFFU,
    };
    test.expect_true(
        std::ranges::equal(reverse.physical_pixels(), kExpectedReverse),
        "reverse grayscale changes only the reversed literal footprint"
    );

    const std::array<std::vector<u16>, 1> one_row{
        std::vector<u16>{0x0001U, 0x0000U, 0x0000U},
    };
    const std::vector<u8> one_pixel_source = make_rle(1U, 1U, one_row);
    struct FormatCase {
        LegacyPixelMasks masks;
        u16 expected;
    };
    constexpr std::array<FormatCase, 3> kFormats{
        FormatCase{
            LegacyPixelMasks{0xF800U, 0x07C0U, 0x003FU},
            0xBDEEU,
        },
        FormatCase{
            LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU},
            0xBDD7U,
        },
        FormatCase{
            LegacyPixelMasks{0xFC00U, 0x03E0U, 0x001FU},
            0xBAF7U,
        },
    };
    for (const FormatCase& format : kFormats) {
        LegacyFramebuffer formatted(LegacySurfaceGeometry{
            .pitch_bytes = 2,
            .width = 1,
            .height = 1,
        });
        formatted.physical_pixels().front() = 0xFFFFU;
        openswd3::rendering::select_legacy_pixel_conversion(
            effects.pixel_conversion,
            format.masks
        );
        test.expect_equal(
            blit(
                formatted,
                LegacyBlitSource{.bytes = one_pixel_source},
                LegacyBlitRequest{
                    .source_width = 1,
                    .source_height = 1,
                    .flags = 0x28U,
                },
                effects,
                jitter
            ),
            LegacyBlitExecutionStatus::completed,
            "grayscale supported pixel format"
        );
        test.expect_equal(
            formatted.physical_pixels().front(),
            format.expected,
            "grayscale uses the selected effective masks and shifts"
        );
    }
}

void test_rle_saturated_add_and_subtract(
    openswd3::test::Context& test
) {
    const std::array<std::vector<u16>, 1> add_rows{
        std::vector<u16>{
            0x0004U,
            0x7C00U,
            0x03E0U,
            0x001FU,
            0x0421U,
            0x0000U,
        },
    };
    const std::vector<u8> add_source = make_rle(4U, 1U, add_rows);
    LegacyFramebuffer added(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 1,
    });
    constexpr std::array<u16, 4> kAddInitial{
        0x0400U, 0x0020U, 0x0001U, 0x0842U,
    };
    std::ranges::copy(kAddInitial, added.physical_pixels().begin());
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            added,
            LegacyBlitSource{.bytes = add_source},
            LegacyBlitRequest{
                .source_width = 4,
                .source_height = 1,
                .flags = 4U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE saturated add forward"
    );
    constexpr std::array<u16, 4> kExpectedAdded{
        0x7C00U, 0x03E0U, 0x001FU, 0x0C63U,
    };
    test.expect_true(
        std::ranges::equal(added.physical_pixels(), kExpectedAdded),
        "saturated add clamps each RGB555 channel independently"
    );

    const std::array<std::vector<u16>, 1> one_pixel_rows{
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
    };
    const std::vector<u8> one_pixel_source = make_rle(
        1U,
        1U,
        one_pixel_rows
    );
    LegacyBlitEffectState effects{
        .red_offset = 1,
        .green_offset = -1,
        .blue_offset = 30,
    };
    LegacyFramebuffer offset_add(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    test.expect_equal(
        blit(
            offset_add,
            LegacyBlitSource{.bytes = one_pixel_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 4U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE source-offset saturated add"
    );
    test.expect_equal(
        offset_add.physical_pixels().front(),
        static_cast<u16>(0x081FU),
        "add clamps the offset-adjusted source before adding the destination"
    );

    const std::array<std::vector<u16>, 1> reverse_rows{
        std::vector<u16>{
            0x0003U,
            0x0421U,
            0x0842U,
            0x0C63U,
            0x0000U,
        },
    };
    const std::vector<u8> reverse_source = make_rle(3U, 1U, reverse_rows);
    LegacyFramebuffer reverse_add(LegacySurfaceGeometry{
        .pitch_bytes = 6,
        .width = 3,
        .height = 1,
    });
    test.expect_equal(
        blit(
            reverse_add,
            LegacyBlitSource{.bytes = reverse_source},
            LegacyBlitRequest{
                .source_width = 3,
                .source_height = 1,
                .flags = 5U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE saturated add reverse"
    );
    constexpr std::array<u16, 3> kExpectedReverseAdd{
        0x0C63U, 0x0842U, 0x0421U,
    };
    test.expect_true(
        std::ranges::equal(
            reverse_add.physical_pixels(),
            kExpectedReverseAdd
        ),
        "reverse add reads the RLE payload forward and writes the target backward"
    );

    LegacyFramebuffer forward_subtract(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    forward_subtract.physical_pixels().front() = 0x0C63U;
    test.expect_equal(
        blit(
            forward_subtract,
            LegacyBlitSource{.bytes = one_pixel_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 0x2CU,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE saturated subtract forward"
    );
    test.expect_equal(
        forward_subtract.physical_pixels().front(),
        static_cast<u16>(0x0460U),
        "forward subtract adjusts the source then clamps target minus source"
    );

    LegacyFramebuffer reverse_subtract(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    reverse_subtract.physical_pixels().front() = 0x0C63U;
    test.expect_equal(
        blit(
            reverse_subtract,
            LegacyBlitSource{.bytes = one_pixel_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 0x2DU,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE saturated subtract reverse"
    );
    test.expect_equal(
        reverse_subtract.physical_pixels().front(),
        static_cast<u16>(0x0C3EU),
        "reverse subtract preserves the original bug that offsets the destination rather than the source"
    );

    LegacyFramebuffer underflow(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 1,
    });
    test.expect_equal(
        blit(
            underflow,
            LegacyBlitSource{.bytes = one_pixel_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 0x2CU,
            },
            LegacyBlitEffectState{},
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE saturated subtract underflow"
    );
    test.expect_equal(
        underflow.physical_pixels().front(),
        static_cast<u16>(0U),
        "negative per-channel subtract results clamp to zero"
    );
}

void test_rle_every_third_row_skip(openswd3::test::Context& test) {
    const std::array<std::vector<u16>, 3> three_rows{
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
    };
    const std::vector<u8> three_row_source = make_rle(
        1U,
        3U,
        three_rows
    );
    LegacyFramebuffer added(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 4,
    });
    LegacyBlitEffectState effects{.skip_every_third_row = true};
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            added,
            LegacyBlitSource{.bytes = three_row_source},
            LegacyBlitRequest{
                .destination_y = 1,
                .source_width = 1,
                .source_height = 3,
                .flags = 4U,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE add every-third-row gate"
    );
    constexpr std::array<u16, 4> kExpectedAdded{
        0x0000U, 0x0421U, 0x0000U, 0x0421U,
    };
    test.expect_true(
        std::ranges::equal(added.physical_pixels(), kExpectedAdded),
        "row gate phase starts from the clipped destination y plus 480"
    );

    const std::array<std::vector<u16>, 4> four_rows{
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
        std::vector<u16>{0x0001U, 0x0421U, 0x0000U},
    };
    const std::vector<u8> four_row_source = make_rle(
        1U,
        4U,
        four_rows
    );
    LegacyFramebuffer subtracted(LegacySurfaceGeometry{
        .pitch_bytes = 2,
        .width = 1,
        .height = 4,
    });
    std::ranges::fill(subtracted.physical_pixels(), 0x7FFFU);
    test.expect_equal(
        blit(
            subtracted,
            LegacyBlitSource{.bytes = four_row_source},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 4,
                .flags = 0x2DU,
            },
            effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "RLE reverse subtract every-third-row gate"
    );
    constexpr std::array<u16, 4> kExpectedSubtracted{
        0x7BDEU, 0x7BDEU, 0x7FFFU, 0x7BDEU,
    };
    test.expect_true(
        std::ranges::equal(
            subtracted.physical_pixels(),
            kExpectedSubtracted
        ),
        "third physical RLE row is consumed without touching the target"
    );
}

void test_effect_jitter_policies(openswd3::test::Context& test) {
    const std::array<std::vector<u16>, 1> rows{
        std::vector<u16>{0x0001U, 0xCAFEU, 0x0000U},
    };
    const std::vector<u8> source = make_rle(1U, 1U, rows);
    std::array<i32, 400> offsets{};
    offsets[34] = -1;
    offsets[331] = 1;
    constexpr std::array<u8, 4> kFillBytes{0x34U, 0x12U, 0x00U, 0x00U};
    LegacyBlitEffectState effects{.red_offset = 1};

    struct JitterRun {
        LegacyBlitExecutionStatus status;
        std::array<u16, 4> pixels;
        u32 phase_bytes;
    };
    const auto run = [&](const u32 flags, const u16 initial) {
        LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
            .pitch_bytes = 8,
            .width = 4,
            .height = 1,
        });
        std::ranges::fill(framebuffer.physical_pixels(), initial);
        LegacyRleRowJitterState jitter{
            .group = 2,
            .phase_bytes = 0U,
            .offsets = offsets,
        };
        const LegacyBlitExecutionStatus status = blit(
            framebuffer,
            LegacyBlitSource{.bytes = source},
            LegacyBlitRequest{
                .destination_x = 1,
                .source_width = 1,
                .source_height = 1,
                .flags = flags,
                .auxiliary = kFillBytes,
            },
            effects,
            jitter
        );
        std::array<u16, 4> pixels{};
        std::ranges::copy(framebuffer.physical_pixels(), pixels.begin());
        return JitterRun{
            .status = status,
            .pixels = pixels,
            .phase_bytes = jitter.phase_bytes,
        };
    };

    const JitterRun offset_forward = run(0x10U, 0U);
    test.expect_equal(
        offset_forward.status,
        LegacyBlitExecutionStatus::completed,
        "jittered forward destination offset"
    );
    test.expect_equal(
        offset_forward.pixels[2],
        static_cast<u16>(0x0400U),
        "forward destination offset uses the 0x528-byte jitter group stride"
    );
    test.expect_equal(
        offset_forward.phase_bytes,
        4U,
        "forward destination offset advances jitter phase"
    );

    const JitterRun offset_reverse = run(0x11U, 0U);
    test.expect_equal(
        offset_reverse.pixels[0],
        static_cast<u16>(0x0400U),
        "reverse destination offset uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        offset_reverse.phase_bytes,
        4U,
        "reverse destination offset advances jitter phase"
    );

    const JitterRun fill_forward = run(0x24U, 0U);
    test.expect_equal(
        fill_forward.pixels[0],
        static_cast<u16>(0x1234U),
        "forward constant fill uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        fill_forward.phase_bytes,
        4U,
        "forward constant fill advances jitter phase"
    );

    const JitterRun fill_reverse = run(0x25U, 0U);
    test.expect_equal(
        fill_reverse.pixels[0],
        static_cast<u16>(0x1234U),
        "reverse constant fill uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        fill_reverse.phase_bytes,
        0U,
        "reverse constant fill preserves jitter phase"
    );

    const JitterRun gray_forward = run(0x28U, 0x7FFFU);
    test.expect_equal(
        gray_forward.pixels[2],
        static_cast<u16>(0x5EF7U),
        "forward grayscale uses the 0x528-byte jitter group stride"
    );
    test.expect_equal(
        gray_forward.phase_bytes,
        4U,
        "forward grayscale advances jitter phase"
    );

    const JitterRun gray_reverse = run(0x29U, 0x7FFFU);
    test.expect_equal(
        gray_reverse.pixels[0],
        static_cast<u16>(0x5EF7U),
        "reverse grayscale uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        gray_reverse.phase_bytes,
        4U,
        "reverse grayscale advances jitter phase"
    );

    const JitterRun add_forward = run(0x04U, 0U);
    test.expect_true(
        add_forward.pixels[0] != 0U,
        "forward saturated add uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        add_forward.phase_bytes,
        4U,
        "forward saturated add advances jitter phase"
    );

    const JitterRun add_reverse = run(0x05U, 0U);
    test.expect_true(
        add_reverse.pixels[0] != 0U,
        "reverse saturated add uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        add_reverse.phase_bytes,
        4U,
        "reverse saturated add advances jitter phase"
    );

    const JitterRun subtract_forward = run(0x2CU, 0x7FFFU);
    test.expect_true(
        subtract_forward.pixels[0] != 0x7FFFU,
        "forward saturated subtract uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        subtract_forward.phase_bytes,
        4U,
        "forward saturated subtract advances jitter phase"
    );

    const JitterRun subtract_reverse = run(0x2DU, 0x7FFFU);
    test.expect_true(
        subtract_reverse.pixels[0] != 0x7FFFU,
        "reverse saturated subtract uses the 0x84-byte jitter group stride"
    );
    test.expect_equal(
        subtract_reverse.phase_bytes,
        4U,
        "reverse saturated subtract advances jitter phase"
    );
}

void test_safe_abnormal_boundaries(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 4,
        .width = 2,
        .height = 2,
    });
    LegacyRleRowJitterState jitter{};
    constexpr std::array<u8, 1> kShortMarker{0xFFU};
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = kShortMarker},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::malformed_source,
        "short first word is an explicit source boundary"
    );

    constexpr std::array<u8, 12> kTruncatedLiteral{
        0xFFU, 0xFFU, 0x01U, 0x00U, 0x01U, 0x00U,
        0x10U, 0x00U, 0x08U, 0x00U, 0x01U, 0x00U,
    };
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = kTruncatedLiteral},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::malformed_source,
        "truncated literal payload is not replaced with transparent pixels"
    );

    constexpr std::array<u8, 2> kIndexed{1U, 1U};
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{
                .bytes = kIndexed,
                .layout = LegacyBlitSourceLayout::indexed_8,
            },
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::palette_out_of_bounds,
        "indexed copy exposes a short palette"
    );

    const std::array<std::vector<u16>, 1> rows{
        std::vector<u16>{0x0001U, 0x1234U, 0x0000U},
    };
    const std::vector<u8> rle = make_rle(1U, 1U, rows);
    LegacyRleRowJitterState short_jitter{
        .group = 1,
        .phase_bytes = 0U,
    };
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = rle},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
            },
            short_jitter
        ),
        LegacyBlitExecutionStatus::jitter_table_out_of_bounds,
        "active jitter requires its exact offset table"
    );

    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = rle},
            LegacyBlitRequest{
                .source_width = 1,
                .source_height = 1,
                .flags = 0x19U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::destination_out_of_bounds,
        "reverse run-edge BUG is isolated when its extra write precedes the framebuffer"
    );
    test.expect_equal(
        framebuffer.physical_pixels().front(),
        static_cast<u16>(0x03E0U),
        "reverse run-edge boundary preserves the in-range write before rejecting the extra write"
    );

    constexpr std::array<u16, 2> kPixels{1U, 2U};
    const std::vector<u8> direct = direct_pixels(kPixels);
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = direct},
            LegacyBlitRequest{
                .destination_x = 0x7FFFFFFF,
                .source_width = 2,
                .source_height = 1,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::destination_out_of_bounds,
        "32-bit coordinate addition wraps before the target safety check"
    );
}

[[nodiscard]] std::uint64_t framebuffer_fnv1a(
    const std::span<const u16> pixels
) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const u16 pixel : pixels) {
        hash ^= static_cast<u8>(pixel & 0xFFU);
        hash *= 0x100000001B3ULL;
        hash ^= static_cast<u8>(pixel >> 8U);
        hash *= 0x100000001B3ULL;
    }

    return hash;
}

void test_real_tsw_frame(openswd3::test::Context& test) {
    constexpr std::array<u8, 76> kCompressed{
        0x0AU, 0xFFU, 0xFFU, 0x10U, 0x00U, 0x10U, 0x00U, 0x10U,
        0x00U, 0x06U, 0x00U, 0x10U, 0xC0U, 0x00U, 0x37U, 0x14U,
        0x00U, 0x03U, 0x18U, 0x00U, 0x05U, 0xC0U, 0x07U, 0x00U,
        0x2CU, 0x00U, 0x00U, 0x06U, 0x04U, 0xC0U, 0x00U, 0x00U,
        0x1CU, 0x00U, 0x04U, 0xC0U, 0x09U, 0x2CU, 0x58U, 0x00U,
        0x94U, 0x01U, 0x06U, 0x03U, 0xC0U, 0x00U, 0x00U, 0x20U,
        0x00U, 0x03U, 0xC0U, 0x0BU, 0x94U, 0x01U, 0x30U, 0x11U,
        0x00U, 0x02U, 0x20U, 0x02U, 0x7CU, 0x00U, 0x3AU, 0x6CU,
        0x01U, 0x36U, 0x3CU, 0x02U, 0x3CU, 0x16U, 0x03U, 0x00U,
        0x00U, 0x11U, 0x00U, 0x00U,
    };
    std::vector<u8> decompressed(238U);
    u32 actual_size{};
    test.expect_equal(
        openswd3::resource_io::decompress_legacy_resource_block(
            kCompressed,
            decompressed,
            actual_size
        ),
        openswd3::resource_io::LegacyLzo1xStatus::success,
        "real TSW frame decompresses"
    );
    test.expect_equal(actual_size, 238U, "real TSW decompressed byte count");

    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(framebuffer.physical_pixels(), 0xA55AU);
    LegacyRleRowJitterState jitter{};
    test.expect_equal(
        blit(
            framebuffer,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE copy"
    );
    test.expect_equal(
        framebuffer_fnv1a(framebuffer.physical_pixels()),
        0x7C38744AC87B8BE1ULL,
        "real all_sys.tsw frame produces the fixed framebuffer hash"
    );
    test.expect_equal(
        std::ranges::count(framebuffer.physical_pixels(), 0x0000U),
        static_cast<std::ptrdiff_t>(54),
        "literal zeroes in the real frame are copied rather than keyed"
    );

    LegacyFramebuffer edged(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(edged.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            edged,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x18U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE run-edge copy"
    );
    test.expect_equal(
        framebuffer_fnv1a(edged.physical_pixels()),
        0x28FED51BD6E4E461ULL,
        "real TSW run-edge copy produces the independently parsed hash"
    );
    test.expect_equal(
        std::ranges::count(edged.physical_pixels(), 0x03E0U),
        static_cast<std::ptrdiff_t>(12),
        "six real literal runs receive two RGB555 edge pixels each"
    );

    constexpr std::array<u8, 4> kFillBytes{0x34U, 0x12U, 0x00U, 0x00U};
    LegacyFramebuffer filled(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(filled.physical_pixels(), 0xA55AU);
    test.expect_equal(
        blit(
            filled,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x24U,
                .auxiliary = kFillBytes,
            },
            LegacyBlitEffectState{},
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE constant fill"
    );
    test.expect_equal(
        framebuffer_fnv1a(filled.physical_pixels()),
        0x96240FB8764F3C39ULL,
        "real TSW literal footprint produces the fixed fill hash"
    );
    test.expect_equal(
        std::ranges::count(filled.physical_pixels(), 0x1234U),
        static_cast<std::ptrdiff_t>(54),
        "real TSW fill changes exactly the independently parsed literal pixels"
    );

    LegacyFramebuffer grayscale(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(grayscale.physical_pixels(), 0x7FFFU);
    test.expect_equal(
        blit(
            grayscale,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x28U,
            },
            LegacyBlitEffectState{},
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE grayscale"
    );
    test.expect_equal(
        framebuffer_fnv1a(grayscale.physical_pixels()),
        0xCE3B416A93211135ULL,
        "real TSW literal footprint produces the fixed grayscale hash"
    );
    test.expect_equal(
        std::ranges::count(grayscale.physical_pixels(), 0x5EF7U),
        static_cast<std::ptrdiff_t>(54),
        "real TSW grayscale changes exactly the independently parsed literal pixels"
    );

    LegacyBlitEffectState color_effects{
        .red_offset = 1,
        .green_offset = -2,
        .blue_offset = 3,
    };
    LegacyFramebuffer added(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(added.physical_pixels(), 0x4210U);
    test.expect_equal(
        blit(
            added,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x04U,
            },
            color_effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE saturated add"
    );

    LegacyFramebuffer subtracted(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(subtracted.physical_pixels(), 0x4210U);
    test.expect_equal(
        blit(
            subtracted,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x2CU,
            },
            color_effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE saturated subtract"
    );

    LegacyFramebuffer reverse_subtracted(LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 16,
    });
    std::ranges::fill(reverse_subtracted.physical_pixels(), 0x4210U);
    test.expect_equal(
        blit(
            reverse_subtracted,
            LegacyBlitSource{.bytes = decompressed},
            LegacyBlitRequest{
                .source_width = 16,
                .source_height = 16,
                .flags = 0x2DU,
            },
            color_effects,
            jitter
        ),
        LegacyBlitExecutionStatus::completed,
        "real TSW RLE reverse saturated subtract"
    );

    test.expect_equal(
        framebuffer_fnv1a(added.physical_pixels()),
        0x870AB3FD82D197EDULL,
        "real TSW saturated add produces the fixed framebuffer hash"
    );
    test.expect_equal(
        std::ranges::count(added.physical_pixels(), 0x4613U),
        static_cast<std::ptrdiff_t>(54),
        "real TSW add changes exactly the literal footprint"
    );
    test.expect_equal(
        framebuffer_fnv1a(subtracted.physical_pixels()),
        0x44AABC486DCBAD05ULL,
        "real TSW saturated subtract produces the fixed framebuffer hash"
    );
    test.expect_equal(
        std::ranges::count(subtracted.physical_pixels(), 0x3E0DU),
        static_cast<std::ptrdiff_t>(54),
        "real TSW forward subtract changes exactly the literal footprint"
    );
    test.expect_equal(
        framebuffer_fnv1a(reverse_subtracted.physical_pixels()),
        0xBAF98799C8AA17B9ULL,
        "real TSW reverse subtract produces the fixed framebuffer hash"
    );
    test.expect_equal(
        std::ranges::count(reverse_subtracted.physical_pixels(), 0x45D3U),
        static_cast<std::ptrdiff_t>(54),
        "real TSW reverse subtract changes exactly the reversed literal footprint"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_sparse_dispatch_table(test);
    test_selection_rules(test);
    test_raw_direct_copy_and_clipping(test);
    test_raw_reverse_and_indexed_paths(test);
    test_raw_color_key_copy(test);
    test_sparse_and_unsupported_execution(test);
    test_rle_spans_and_horizontal_direction(test);
    test_rle_run_edge_copy(test);
    test_rle_vertical_flip_row_base(test);
    test_rle_jitter_and_header_gate(test);
    test_rle_destination_offset(test);
    test_rle_constant_fill(test);
    test_rle_grayscale(test);
    test_rle_saturated_add_and_subtract(test);
    test_rle_every_third_row_skip(test);
    test_effect_jitter_policies(test);
    test_safe_abnormal_boundaries(test);
    test_real_tsw_frame(test);
    return test.exit_code();
}
