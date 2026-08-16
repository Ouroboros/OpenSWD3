#include "test.hpp"

#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_cm_cache_loader.hpp"
#include "openswd3/world_map/legacy_world_background.hpp"
#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::legacy_convert_pixels_forward;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::select_legacy_pixel_conversion;
using openswd3::world_map::kLegacyWorldCellHidden;
using openswd3::world_map::kLegacyWorldCellTransparent;
using openswd3::world_map::LegacyCmCacheLoadStatus;
using openswd3::world_map::LegacyCmCacheRequest;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundRenderStatus;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldBackgroundView;
using openswd3::world_map::LegacyWorldMapLoadStatus;
using openswd3::world_map::load_legacy_cm_cache;
using openswd3::world_map::load_legacy_world_map;
using openswd3::world_map::render_legacy_world_background;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-background-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

private:
    std::filesystem::path root_;
};

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> make_direct_tile_bytes(const u32 tile_count) {
    std::vector<u8> bytes(static_cast<std::size_t>(tile_count) * 0x200U);
    for (u32 tile = 0U; tile < tile_count; ++tile) {
        for (u32 y = 0U; y < 16U; ++y) {
            for (u32 x = 0U; x < 16U; ++x) {
                const u16 pixel =
                    static_cast<u16>(tile * 0x1000U + y * 16U + x + 1U);
                const std::size_t offset =
                    static_cast<std::size_t>(tile) * 0x200U +
                    static_cast<std::size_t>(y * 16U + x) * 2U;
                bytes[offset] = static_cast<u8>(pixel);
                bytes[offset + 1U] = static_cast<u8>(pixel >> 8U);
            }
        }
    }
    return bytes;
}

struct SyntheticMap {
    u32 width{45U};
    u32 height{35U};
    std::vector<u16> tiles;
    std::vector<u8> flags;
    std::vector<u8> cache;

    SyntheticMap()
        : tiles(static_cast<std::size_t>(width) * height, 0U),
          flags(static_cast<std::size_t>(width) * height * 4U),
          cache(make_direct_tile_bytes(2U)) {}

    [[nodiscard]] LegacyWorldBackgroundSource source() const {
        return LegacyWorldBackgroundSource{
            .map_width = width,
            .map_height = height,
            .tile_layer_offset = 0U,
            .tile_indices = tiles,
            .cell_flags = flags,
            .tile_bytes = cache,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
            .palette = {},
            .transparent_pixel = 1U,
        };
    }
};

struct SyntheticIndexedMap {
    u32 width{};
    u32 height{};
    std::vector<u16> tiles;
    std::vector<u8> flags;
    std::vector<u8> cache;
    std::vector<u16> palette;

    explicit SyntheticIndexedMap(
        const u32 map_width = 64U, const u32 map_height = 64U
    )
        : width(map_width), height(map_height),
          tiles(static_cast<std::size_t>(width) * height, 0U),
          flags(static_cast<std::size_t>(width) * height * 4U),
          cache(0x400U, 2U), palette(256U) {
        std::fill(cache.begin() + 0x300, cache.end(), 1U);
        for (u32 index = 0U; index < 256U; ++index) {
            palette[index] = static_cast<u16>(0x2000U + index);
        }
    }

    [[nodiscard]] LegacyWorldBackgroundSource
    source(const u32 tile_layer_offset = 0U) const {
        return LegacyWorldBackgroundSource{
            .map_width = width,
            .map_height = height,
            .tile_layer_offset = tile_layer_offset,
            .tile_indices = tiles,
            .cell_flags = flags,
            .tile_bytes = cache,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::indexed_8,
            .palette = palette,
        };
    }
};

void fill_framebuffer(LegacyFramebuffer& framebuffer, const u16 value) {
    std::ranges::fill(framebuffer.physical_pixels(), value);
}

void test_aligned_direct_tiles(openswd3::test::Context& test) {
    SyntheticMap map;
    map.tiles[2U * map.width + 1U] = 1U;
    LegacyFramebuffer framebuffer;

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{.camera_left = 16, .camera_top = 32}
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 40U * 30U &&
            result.opaque_cells == 40U * 30U &&
            result.written_pixels == 640U * 480U,
        "aligned 16-bit path visits the original 40 by 30 tile viewport"
    );
    test.expect_true(
        framebuffer.row_pixels(0U)[0U] == 0x1001U &&
            framebuffer.row_pixels(0U)[15U] == 0x1010U &&
            framebuffer.row_pixels(15U)[0U] == 0x10F1U &&
            framebuffer.row_pixels(16U)[0U] == 1U,
        "aligned viewport selects the layer tile and advances map rows"
    );
}

void test_aligned_direct_layer_and_cell_flags(openswd3::test::Context& test) {
    SyntheticMap map;
    const std::size_t cell_count =
        static_cast<std::size_t>(map.width) * map.height;
    map.tiles.assign(cell_count * 2U, 1U);
    std::fill(
        map.tiles.begin() + static_cast<std::ptrdiff_t>(cell_count),
        map.tiles.end(),
        0U
    );
    write_u32(map.flags, 0U, kLegacyWorldCellTransparent);
    write_u32(map.flags, 4U, kLegacyWorldCellHidden);

    LegacyWorldBackgroundSource source = map.source();
    source.tile_layer_offset = static_cast<u32>(cell_count);
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer, source, LegacyWorldBackgroundView{}
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 40U * 30U &&
            result.transparent_cells == 1U && result.hidden_cells == 1U &&
            result.opaque_cells == 40U * 30U - 2U &&
            result.written_pixels == 40U * 30U * 16U * 16U - 257U,
        "aligned direct path applies the animated layer only to tile indices"
    );
    test.expect_true(
        framebuffer.row_pixels(0U)[0U] == 0x7777U &&
            framebuffer.row_pixels(0U)[1U] == 2U &&
            framebuffer.row_pixels(0U)[16U] == 0x7777U &&
            framebuffer.row_pixels(0U)[32U] == 1U,
        "aligned direct path preserves transparent, hidden, and opaque dispatch"
    );
}

void test_unaligned_edges_and_flags(openswd3::test::Context& test) {
    SyntheticMap map;
    write_u32(map.flags, 0U, kLegacyWorldCellTransparent);
    write_u32(map.flags, 4U, kLegacyWorldCellHidden);
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{.camera_left = 5, .camera_top = 7}
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 41U * 31U &&
            result.transparent_cells == 1U && result.hidden_cells == 1U,
        "unaligned path includes both clipped edge rows and columns"
    );
    test.expect_true(
        framebuffer.row_pixels(0U)[0U] == 118U &&
            framebuffer.row_pixels(8U)[10U] == 256U &&
            framebuffer.row_pixels(9U)[11U] == 1U,
        "unaligned camera maps destination pixels to exact tile offsets"
    );
    test.expect_true(
        framebuffer.row_pixels(0U)[11U] == 0x7777U &&
            framebuffer.row_pixels(0U)[27U] == 113U,
        "hidden cell keeps the prior framebuffer while its neighbor draws"
    );

    fill_framebuffer(framebuffer, 0x7777U);
    const auto negative_cell = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{.camera_left = -21, .camera_top = 7}
    );
    test.expect_true(
        negative_cell.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(0U)[4U] == 0x7777U &&
            framebuffer.row_pixels(0U)[5U] == 113U,
        "direct normal path clamps negative cell X once at 00412D74..00412DC4"
    );

    fill_framebuffer(framebuffer, 0x7777U);
    map.flags.assign(map.flags.size(), 0U);
    write_u32(map.flags, 0U, kLegacyWorldCellTransparent);
    const auto transparent = render_legacy_world_background(
        framebuffer, map.source(), LegacyWorldBackgroundView{}
    );
    test.expect_true(
        transparent.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(0U)[0U] == 0x7777U &&
            framebuffer.row_pixels(0U)[1U] == 2U,
        "16-bit transparent tiles skip only the configured color-key pixel"
    );
}

void test_partial_refresh(openswd3::test::Context& test) {
    SyntheticMap map;
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .partial_refresh = true,
            .partial_focus_x = 320,
            .partial_focus_y = 240,
        }
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 24U * 24U &&
            result.written_pixels == 384U * 384U,
        "service-13 refresh rounds around the focus to a 384-square region"
    );
    test.expect_true(
        framebuffer.row_pixels(47U)[320U] == 0x7777U &&
            framebuffer.row_pixels(48U)[128U] != 0x7777U &&
            framebuffer.row_pixels(431U)[511U] != 0x7777U &&
            framebuffer.row_pixels(432U)[320U] == 0x7777U,
        "partial refresh does not modify pixels outside its aligned bounds"
    );

    fill_framebuffer(framebuffer, 0x7777U);
    const auto unaligned = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .partial_refresh = true,
            .partial_focus_x = 321,
            .partial_focus_y = 241,
        }
    );
    test.expect_true(
        unaligned.status == LegacyWorldBackgroundRenderStatus::completed &&
            unaligned.written_pixels == 384U * 384U &&
            framebuffer.row_pixels(63U)[320U] == 0x7777U &&
            framebuffer.row_pixels(64U)[144U] != 0x7777U &&
            framebuffer.row_pixels(447U)[527U] != 0x7777U &&
            framebuffer.row_pixels(448U)[320U] == 0x7777U,
        "background refresh rounds focus upward at 00412C26 before tile bounds"
    );
}

void test_partial_refresh_zero_left_stride(openswd3::test::Context& test) {
    SyntheticMap map;
    map.tiles[3U * map.width + 29U] = 1U;
    LegacyFramebuffer framebuffer;

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .partial_refresh = true,
            .partial_focus_x = 192,
            .partial_focus_y = 240,
        }
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(64U)[0U] == 0x1001U,
        "zero-left partial refresh preserves the original row-stride quirk"
    );
}

void test_unaligned_partial_refresh_interior_only(
    openswd3::test::Context& test
) {
    {
        SyntheticMap map;
        LegacyFramebuffer framebuffer;
        fill_framebuffer(framebuffer, 0x7777U);

        const auto result = render_legacy_world_background(
            framebuffer,
            map.source(),
            LegacyWorldBackgroundView{
                .camera_left = 0,
                .camera_top = 7,
                .partial_refresh = true,
                .partial_focus_x = 320,
                .partial_focus_y = 240,
            }
        );
        test.expect_true(
            result.status == LegacyWorldBackgroundRenderStatus::completed &&
                framebuffer.row_pixels(57U)[128U] == 0x7777U &&
                framebuffer.row_pixels(48U)[144U] == 0x7777U &&
                framebuffer.row_pixels(57U)[144U] == 1U,
            "unaligned partial refresh leaves its aligned-axis border intact"
        );
    }

    {
        SyntheticMap map;
        map.tiles[4U * map.width + 9U] = 1U;
        LegacyFramebuffer framebuffer;
        fill_framebuffer(framebuffer, 0x7777U);

        const auto result = render_legacy_world_background(
            framebuffer,
            map.source(),
            LegacyWorldBackgroundView{
                .camera_left = -5,
                .camera_top = 7,
                .partial_refresh = true,
                .partial_focus_x = 320,
                .partial_focus_y = 240,
            }
        );
        test.expect_true(
            result.status == LegacyWorldBackgroundRenderStatus::completed &&
                framebuffer.row_pixels(57U)[133U] == 0x7777U &&
                framebuffer.row_pixels(57U)[149U] == 0x1001U,
            "negative camera clamp retains the original partial tile origin"
        );
    }
}

void test_indexed_tiles(openswd3::test::Context& test) {
    constexpr u32 width = 45U;
    constexpr u32 height = 35U;
    std::vector<u16> tiles(static_cast<std::size_t>(width) * height, 0U);
    std::vector<u8> flags(static_cast<std::size_t>(width) * height * 4U);
    write_u32(flags, 0U, kLegacyWorldCellTransparent);
    std::vector<u8> cache(0x300U);
    for (u32 index = 0U; index < 0x100U; ++index) {
        cache[0x200U + index] = static_cast<u8>(index);
    }
    std::vector<u16> palette(256U);
    for (u32 index = 0U; index < 256U; ++index) {
        palette[index] = static_cast<u16>(0x2000U + index);
    }
    const LegacyWorldBackgroundSource source{
        .map_width = width,
        .map_height = height,
        .tile_indices = tiles,
        .cell_flags = flags,
        .tile_bytes = cache,
        .pixel_layout = LegacyWorldBackgroundPixelLayout::indexed_8,
        .palette = palette,
    };
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer, source, LegacyWorldBackgroundView{}
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(0U)[0U] == 0x2000U &&
            framebuffer.row_pixels(0U)[1U] == 0x7777U &&
            framebuffer.row_pixels(0U)[2U] == 0x2002U,
        "8-bit transparent tiles skip palette index one, not a color value"
    );

    fill_framebuffer(framebuffer, 0x7777U);
    const auto unaligned = render_legacy_world_background(
        framebuffer,
        source,
        LegacyWorldBackgroundView{.camera_left = 5, .camera_top = 7}
    );
    test.expect_true(
        unaligned.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(0U)[0U] == 0x2075U,
        "8-bit unaligned path maps clipped destination pixels through palette"
    );

    fill_framebuffer(framebuffer, 0x7777U);
    const auto negative_cell = render_legacy_world_background(
        framebuffer,
        source,
        LegacyWorldBackgroundView{.camera_left = -21, .camera_top = 7}
    );
    test.expect_true(
        negative_cell.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(0U)[4U] == 0x7777U &&
            framebuffer.row_pixels(0U)[5U] == 0x2070U,
        "indexed normal path clamps negative cell X once at 004133B4..00413404"
    );
}

void test_unaligned_indexed_partial_interior(openswd3::test::Context& test) {
    SyntheticIndexedMap map;
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .camera_left = 0,
            .camera_top = 7,
            .partial_refresh = true,
            .partial_focus_x = 320,
            .partial_focus_y = 240,
        }
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 22U * 23U &&
            result.opaque_cells == 22U * 23U &&
            result.written_pixels == 22U * 23U * 16U * 16U,
        "indexed service-13 reports 506 modern unique cells and 129536 writes"
    );
    test.expect_true(
        framebuffer.row_pixels(56U)[144U] == 0x7777U &&
            framebuffer.row_pixels(57U)[143U] == 0x7777U &&
            framebuffer.row_pixels(57U)[144U] == 0x2002U &&
            framebuffer.row_pixels(424U)[495U] == 0x2002U &&
            framebuffer.row_pixels(425U)[495U] == 0x7777U &&
            framebuffer.row_pixels(424U)[496U] == 0x7777U,
        "indexed service-13 keeps all four strict borders and starts at 144,57"
    );

    map.tiles[4U * map.width + 9U] = 1U;
    fill_framebuffer(framebuffer, 0x7777U);
    const auto negative = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .camera_left = -5,
            .camera_top = 7,
            .partial_refresh = true,
            .partial_focus_x = 320,
            .partial_focus_y = 240,
        }
    );
    test.expect_true(
        negative.status == LegacyWorldBackgroundRenderStatus::completed &&
            negative.visited_cells == 22U * 23U &&
            negative.written_pixels == 22U * 23U * 16U * 16U &&
            framebuffer.row_pixels(57U)[148U] == 0x7777U &&
            framebuffer.row_pixels(57U)[149U] == 0x2001U,
        "indexed negative camera clamp preserves first destination x 149"
    );
}

void test_unaligned_indexed_normal_edge_clip(openswd3::test::Context& test) {
    SyntheticIndexedMap map;
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{
            .camera_left = 5,
            .camera_top = 7,
            .edge_clip_left = 300,
            .edge_clip_top = 200,
            .edge_clip_right = 340,
            .edge_clip_bottom = 240,
        }
    );
    // These result counters are the modern API's unique-cell observations.
    // They do not reproduce the original four edge calls' repeated corners.
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 41U * 31U &&
            result.opaque_cells == 41U * 31U &&
            result.written_pixels == 39U * 29U * 16U * 16U,
        "indexed normal traversal reports unique cells while clipping its ring"
    );
    test.expect_true(
        framebuffer.row_pixels(0U)[0U] == 0x7777U &&
            framebuffer.row_pixels(9U)[11U] == 0x2002U,
        "indexed outer ring obeys a narrow clip while interior bypasses it"
    );
}

void test_unaligned_indexed_flags_and_layer(openswd3::test::Context& test) {
    SyntheticIndexedMap map;
    const std::size_t cell_count =
        static_cast<std::size_t>(map.width) * map.height;
    map.tiles.resize(cell_count * 2U, 0U);
    map.tiles[cell_count + map.width + 1U] = 1U;
    map.tiles[cell_count + map.width + 2U] = 1U;
    write_u32(map.flags, (map.width + 1U) * 4U, kLegacyWorldCellTransparent);
    write_u32(map.flags, (map.width + 3U) * 4U, kLegacyWorldCellHidden);

    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);
    const auto result = render_legacy_world_background(
        framebuffer,
        map.source(static_cast<u32>(cell_count)),
        LegacyWorldBackgroundView{.camera_left = 5, .camera_top = 7}
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            result.visited_cells == 41U * 31U &&
            result.transparent_cells == 1U && result.hidden_cells == 1U &&
            result.opaque_cells == 41U * 31U - 2U &&
            result.written_pixels == 640U * 480U - 2U * 16U * 16U,
        "indexed unaligned flags stay on the base grid while tile layer moves"
    );
    test.expect_true(
        framebuffer.row_pixels(9U)[11U] == 0x7777U &&
            framebuffer.row_pixels(9U)[27U] == 0x2001U &&
            framebuffer.row_pixels(9U)[43U] == 0x7777U,
        "indexed transparent index one skips, opaque index one writes, hidden skips"
    );
}

void test_unaligned_indexed_map_and_source_bounds(
    openswd3::test::Context& test
) {
    SyntheticIndexedMap map(42U, 32U);
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);

    const auto truncated = render_legacy_world_background(
        framebuffer,
        map.source(),
        LegacyWorldBackgroundView{.camera_left = 37, .camera_top = 39}
    );
    test.expect_true(
        truncated.status == LegacyWorldBackgroundRenderStatus::completed &&
            truncated.visited_cells == 40U * 30U &&
            truncated.written_pixels == 635U * 473U &&
            framebuffer.row_pixels(472U)[634U] == 0x2002U &&
            framebuffer.row_pixels(472U)[635U] == 0x7777U &&
            framebuffer.row_pixels(473U)[634U] == 0x7777U,
        "indexed right and down map edges truncate the unaligned viewport"
    );

    LegacyWorldBackgroundSource short_palette = map.source();
    short_palette.palette = std::span<const u16>{map.palette}.first(255U);
    test.expect_equal(
        render_legacy_world_background(
            framebuffer,
            short_palette,
            LegacyWorldBackgroundView{.camera_left = 5, .camera_top = 7}
        )
            .status,
        LegacyWorldBackgroundRenderStatus::palette_out_of_bounds,
        "indexed unaligned rendering rejects a short palette before access"
    );

    LegacyWorldBackgroundSource short_tiles = map.source();
    short_tiles.tile_bytes = std::span<const u8>{map.cache}.first(0x200U);
    test.expect_equal(
        render_legacy_world_background(
            framebuffer,
            short_tiles,
            LegacyWorldBackgroundView{.camera_left = 5, .camera_top = 7}
        )
            .status,
        LegacyWorldBackgroundRenderStatus::tile_source_out_of_bounds,
        "indexed unaligned rendering isolates a short tile source"
    );
}

void test_indexed_partial_zero_left_stride(openswd3::test::Context& test) {
    constexpr u32 width = 45U;
    constexpr u32 height = 35U;
    std::vector<u16> tiles(static_cast<std::size_t>(width) * height, 0U);
    tiles[3U * width + 29U] = 1U;
    std::vector<u8> flags(static_cast<std::size_t>(width) * height * 4U);
    std::vector<u8> cache(0x400U, 0U);
    std::fill(cache.begin() + 0x300, cache.end(), 2U);
    std::vector<u16> palette(256U);
    for (u32 index = 0U; index < 256U; ++index) {
        palette[index] = static_cast<u16>(0x2000U + index);
    }
    const LegacyWorldBackgroundSource source{
        .map_width = width,
        .map_height = height,
        .tile_indices = tiles,
        .cell_flags = flags,
        .tile_bytes = cache,
        .pixel_layout = LegacyWorldBackgroundPixelLayout::indexed_8,
        .palette = palette,
    };
    LegacyFramebuffer framebuffer;

    const auto result = render_legacy_world_background(
        framebuffer,
        source,
        LegacyWorldBackgroundView{
            .partial_refresh = true,
            .partial_focus_x = 192,
            .partial_focus_y = 240,
        }
    );
    test.expect_true(
        result.status == LegacyWorldBackgroundRenderStatus::completed &&
            framebuffer.row_pixels(64U)[0U] == 0x2002U,
        "indexed aligned path preserves the zero-left row-stride quirk"
    );
}

void test_checked_source_failures(openswd3::test::Context& test) {
    SyntheticMap map;
    LegacyFramebuffer framebuffer;

    LegacyWorldBackgroundSource source = map.source();
    source.tile_indices = std::span<const u16>{source.tile_indices}.first(1U);
    test.expect_equal(
        render_legacy_world_background(framebuffer, source, {}).status,
        LegacyWorldBackgroundRenderStatus::tile_grid_out_of_bounds,
        "short tile grid is isolated before rendering"
    );

    source = map.source();
    source.tile_bytes = std::span<const u8>{source.tile_bytes}.first(1U);
    test.expect_equal(
        render_legacy_world_background(framebuffer, source, {}).status,
        LegacyWorldBackgroundRenderStatus::tile_source_out_of_bounds,
        "short CM tile source is reported at its first accessed pixel"
    );
}

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
    LegacyPixelConversionState state;
    select_legacy_pixel_conversion(state, {0xF800U, 0x07E0U, 0x001FU});
    return state;
}

void test_current_map_24(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
) {
    const auto map = load_legacy_world_map(archive_path, 24U);
    test.expect_equal(
        map.status,
        LegacyWorldMapLoadStatus::ready,
        "current map 24 loads before background rendering"
    );
    if (map.status != LegacyWorldMapLoadStatus::ready) {
        return;
    }

    const TestTree tree;
    const LegacyPixelConversionState conversion = rgb565_conversion();
    const auto cache = load_legacy_cm_cache(
        LegacyCmCacheRequest{
            .archive_path = archive_path,
            .cache_directory = tree.root(),
            .map_id = 24U,
            .map_offset = map.session.lookup.map_offset,
            .cm_relative_offset = map.session.header.offset_20,
            .cache_limit_megabytes = 60U,
            .map_pixel_bits = map.session.header.field_88,
            .pixel_conversion = conversion,
        }
    );
    test.expect_equal(
        cache.status,
        LegacyCmCacheLoadStatus::ready_generated,
        "current map 24 produces its real CM tile source"
    );
    if (cache.status != LegacyCmCacheLoadStatus::ready_generated) {
        return;
    }

    u16 transparent_pixel = 0x026BU;
    legacy_convert_pixels_forward(conversion, &transparent_pixel, 1);
    LegacyFramebuffer framebuffer;
    const auto rendered = render_legacy_world_background(
        framebuffer,
        LegacyWorldBackgroundSource{
            .map_width = map.session.header.width,
            .map_height = map.session.header.height,
            .tile_layer_offset = 0U,
            .tile_indices = map.session.surface_grid.raw_table_values,
            .cell_flags = map.session.surface_grid.surface_grid,
            .tile_bytes = cache.cache_bytes,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
            .palette = {},
            .transparent_pixel = transparent_pixel,
        },
        LegacyWorldBackgroundView{}
    );
    test.expect_true(
        map.session.header.field_88 == 16U &&
            rendered.status == LegacyWorldBackgroundRenderStatus::completed &&
            rendered.visited_cells == 40U * 30U &&
            legacy_framebuffer_logical_fnv1a64(framebuffer) ==
                0x947C15A53487BF9AULL,
        "current 16-bit map reaches the aligned real-asset viewport"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_aligned_direct_tiles(test);
    test_aligned_direct_layer_and_cell_flags(test);
    test_unaligned_edges_and_flags(test);
    test_partial_refresh(test);
    test_partial_refresh_zero_left_stride(test);
    test_unaligned_partial_refresh_interior_only(test);
    test_indexed_tiles(test);
    test_unaligned_indexed_partial_interior(test);
    test_unaligned_indexed_normal_edge_clip(test);
    test_unaligned_indexed_flags_and_layer(test);
    test_unaligned_indexed_map_and_source_bounds(test);
    test_indexed_partial_zero_left_stride(test);
    test_checked_source_failures(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_map_24(test, arguments[1]);
    }
    return test.exit_code();
}
