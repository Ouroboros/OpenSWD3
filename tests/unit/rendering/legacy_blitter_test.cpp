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
using openswd3::rendering::LegacyBlitRequest;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyBlitSourceLayout;
using openswd3::rendering::LegacyBlitterRoutine;
using openswd3::rendering::LegacyBlitterSelectionStatus;
using openswd3::rendering::LegacyFramebuffer;
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
    LegacyRleRowJitterState& jitter
) {
    return openswd3::rendering::blit_legacy_copy_paths(
        framebuffer,
        full_clip(framebuffer),
        source,
        request,
        jitter
    ).status;
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
        Assignment{0x84U, LegacyBlitterRoutine::raw_saturated_add_forward},
        Assignment{0x85U, LegacyBlitterRoutine::raw_saturated_add_reverse},
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
                .flags = 4U,
            },
            jitter
        ),
        LegacyBlitExecutionStatus::unsupported_routine,
        "assigned effect remains explicit until its B4.5 implementation"
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
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_sparse_dispatch_table(test);
    test_selection_rules(test);
    test_raw_direct_copy_and_clipping(test);
    test_raw_reverse_and_indexed_paths(test);
    test_sparse_and_unsupported_execution(test);
    test_rle_spans_and_horizontal_direction(test);
    test_rle_vertical_flip_row_base(test);
    test_rle_jitter_and_header_gate(test);
    test_safe_abnormal_boundaries(test);
    test_real_tsw_frame(test);
    return test.exit_code();
}
