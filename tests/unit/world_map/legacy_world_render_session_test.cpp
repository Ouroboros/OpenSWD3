#include "test.hpp"

#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_render_session.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::legacy_convert_pixels_forward;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::select_legacy_pixel_conversion;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectory;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectoryStatus;
using openswd3::resource_io::LegacyLmfMapHeader;
using openswd3::resource_io::LegacyLmfMapHeaderStatus;
using openswd3::resource_io::LegacyLmfMapLookupResult;
using openswd3::resource_io::LegacyLmfMapLookupStatus;
using openswd3::resource_io::LegacyLmfOffset14Directory;
using openswd3::resource_io::LegacyLmfOffset14DirectoryStatus;
using openswd3::resource_io::LegacyLmfOffset1cDirectory;
using openswd3::resource_io::LegacyLmfOffset1cDirectoryStatus;
using openswd3::resource_io::LegacyLmfPostSurfaceRecords;
using openswd3::resource_io::LegacyLmfPostSurfaceRecordsStatus;
using openswd3::resource_io::LegacyLmfReferencedRecordDirectory;
using openswd3::resource_io::LegacyLmfReferencedRecordDirectoryStatus;
using openswd3::resource_io::LegacyLmfSurfaceGrid;
using openswd3::resource_io::LegacyLmfSurfaceGridStatus;
using openswd3::world_map::load_legacy_world_render_session;
using openswd3::world_map::render_legacy_world_background;
using openswd3::world_map::LegacyCmCacheLoadResult;
using openswd3::world_map::LegacyCmCacheLoadStatus;
using openswd3::world_map::LegacyCmCacheRequest;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundRenderStatus;
using openswd3::world_map::LegacyWorldBackgroundView;
using openswd3::world_map::LegacyFileWorldCmCacheSource;
using openswd3::world_map::LegacyLmfWorldMapSource;
using openswd3::world_map::LegacyWorldCmCacheSource;
using openswd3::world_map::LegacyWorldMapLoadStatus;
using openswd3::world_map::LegacyWorldMapSource;
using openswd3::world_map::LegacyWorldRenderSessionRequest;
using openswd3::world_map::LegacyWorldRenderSessionStatus;
using openswd3::world_map::draw_legacy_world_indexed_objects;
using openswd3::world_map::LegacyWorldIndexedObjectDrawStatus;
using openswd3::world_map::LegacyWorldIndexedObjectRuntimeDrawPorts;
using openswd3::world_map::LegacyWorldIndexedObjectViewport;

enum class Stage {
    lookup,
    header,
    cm,
    surface,
    post_surface,
    referenced,
    offset14,
    indexed,
    offset1c,
};

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-render-session-" + std::to_string(unique_value));
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

class FakeMapSource final : public LegacyWorldMapSource {
public:
    explicit FakeMapSource(std::vector<Stage>& stages) : stages_{stages} {
        make_ready(16U);
    }

    void make_ready(const u32 pixel_bits) {
        lookup.status = LegacyLmfMapLookupStatus::ready;
        lookup.map_offset = 0x11223344U;
        header.status = LegacyLmfMapHeaderStatus::ready;
        header.offset_20 = 0x55667788U;
        header.width = 40U;
        header.height = 30U;
        header.field_88 = pixel_bits;
        header.layers = 1U;
        surface.status = LegacyLmfSurfaceGridStatus::ready;
        surface.raw_table_values.assign(40U * 30U, 0U);
        surface.surface_grid.assign(40U * 30U * 4U, 0U);
        post_surface.status = LegacyLmfPostSurfaceRecordsStatus::ready;
        referenced.status = LegacyLmfReferencedRecordDirectoryStatus::ready;
        offset14.status = LegacyLmfOffset14DirectoryStatus::ready;
        indexed.status = LegacyLmfIndexedObjectDirectoryStatus::ready;
        offset1c.status = LegacyLmfOffset1cDirectoryStatus::ready;
    }

    LegacyLmfMapLookupResult lookup_map(const u32 map_id) override {
        stages_.push_back(Stage::lookup);
        seen_map_id = map_id;
        return lookup;
    }

    LegacyLmfMapHeader read_map_header(const u32) override {
        stages_.push_back(Stage::header);
        return header;
    }

    LegacyLmfSurfaceGrid
    read_surface_grid(const u32, const LegacyLmfMapHeader&) override {
        stages_.push_back(Stage::surface);
        return surface;
    }

    LegacyLmfPostSurfaceRecords
    read_post_surface_records(const u32, const LegacyLmfSurfaceGrid&) override {
        stages_.push_back(Stage::post_surface);
        return post_surface;
    }

    LegacyLmfReferencedRecordDirectory read_referenced_record_directory(
        const u32, const LegacyLmfPostSurfaceRecords&
    ) override {
        stages_.push_back(Stage::referenced);
        return referenced;
    }

    LegacyLmfOffset14Directory
    read_offset14_directory(const u32, const LegacyLmfMapHeader&) override {
        stages_.push_back(Stage::offset14);
        return offset14;
    }

    LegacyLmfIndexedObjectDirectory read_indexed_object_directory(
        const u32, const LegacyLmfMapHeader&
    ) override {
        stages_.push_back(Stage::indexed);
        return indexed;
    }

    LegacyLmfOffset1cDirectory
    read_offset1c_directory(const u32, const LegacyLmfMapHeader&) override {
        stages_.push_back(Stage::offset1c);
        return offset1c;
    }

    LegacyLmfMapLookupResult lookup;
    LegacyLmfMapHeader header;
    LegacyLmfSurfaceGrid surface;
    LegacyLmfPostSurfaceRecords post_surface;
    LegacyLmfReferencedRecordDirectory referenced;
    LegacyLmfOffset14Directory offset14;
    LegacyLmfIndexedObjectDirectory indexed;
    LegacyLmfOffset1cDirectory offset1c;
    u32 seen_map_id{};

private:
    std::vector<Stage>& stages_;
};

class FakeCmCacheSource final : public LegacyWorldCmCacheSource {
public:
    explicit FakeCmCacheSource(std::vector<Stage>& stages) : stages_{stages} {
        result.status = LegacyCmCacheLoadStatus::ready_hit;
        result.cache_bytes.resize(0x400U);
    }

    LegacyCmCacheLoadResult
    load_cm_cache(const LegacyCmCacheRequest& request) override {
        stages_.push_back(Stage::cm);
        seen_request = request;
        ++call_count;
        return result;
    }

    LegacyCmCacheLoadResult result;
    LegacyCmCacheRequest seen_request;
    u32 call_count{};

private:
    std::vector<Stage>& stages_;
};

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
    LegacyPixelConversionState state;
    select_legacy_pixel_conversion(
        state,
        openswd3::rendering::LegacyPixelMasks{
            .red = 0xF800U,
            .green = 0x07E0U,
            .blue = 0x001FU,
        }
    );
    return state;
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        hash ^= byte;
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

void test_original_load_slot_and_direct_source(openswd3::test::Context& test) {
    TestTree tree;
    std::vector<Stage> stages;
    FakeMapSource map_source{stages};
    FakeCmCacheSource cm_source{stages};
    const LegacyPixelConversionState conversion = rgb565_conversion();
    const auto result = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "cache" / "maps",
            .map_id = 24U,
            .cache_limit_megabytes = 73U,
            .pixel_conversion = conversion,
        },
        map_source,
        cm_source
    );

    test.expect_equal(
        result.status,
        LegacyWorldRenderSessionStatus::ready,
        "direct map produces a complete render session"
    );
    test.expect_equal(
        stages,
        std::vector<Stage>{
            Stage::lookup,
            Stage::header,
            Stage::cm,
            Stage::surface,
            Stage::post_surface,
            Stage::referenced,
            Stage::offset14,
            Stage::indexed,
            Stage::offset1c,
        },
        "CM loading occupies the header-to-surface assembly slot"
    );
    test.expect_true(
        map_source.seen_map_id == 24U && cm_source.seen_request.map_id == 24U &&
            cm_source.seen_request.map_offset == 0x11223344U &&
            cm_source.seen_request.cm_relative_offset == 0x55667788U &&
            cm_source.seen_request.cache_limit_megabytes == 73U &&
            cm_source.seen_request.map_pixel_bits == 16U,
        "CM request is derived from the loaded map header"
    );

    u16 expected_transparent = 0x026BU;
    legacy_convert_pixels_forward(conversion, &expected_transparent, 1);
    const auto background = result.session.background_source(17U);
    test.expect_true(
        background.map_width == 40U && background.map_height == 30U &&
            background.tile_layer_offset == 17U &&
            background.tile_indices.data() ==
                result.session.map_load.session.surface_grid.raw_table_values
                    .data() &&
            background.cell_flags.data() ==
                result.session.map_load.session.surface_grid.surface_grid
                    .data() &&
            background.tile_bytes.data() ==
                result.session.cm_cache.cache_bytes.data() &&
            background.pixel_layout ==
                LegacyWorldBackgroundPixelLayout::direct_16 &&
            background.palette.empty() &&
            background.transparent_pixel == expected_transparent,
        "background view borrows all buffers from its owning session"
    );
}

void test_indexed_palette_is_owned_and_converted(
    openswd3::test::Context& test
) {
    TestTree tree;
    std::vector<Stage> stages;
    FakeMapSource map_source{stages};
    map_source.make_ready(8U);
    FakeCmCacheSource cm_source{stages};
    cm_source.result.cache_bytes.assign(0x300U, 0U);
    cm_source.result.cache_bytes[14U] = 0xE0U;
    cm_source.result.cache_bytes[15U] = 0x03U;

    const auto result = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "cache",
            .map_id = 4U,
            .pixel_conversion = rgb565_conversion(),
        },
        map_source,
        cm_source
    );

    test.expect_equal(
        result.status,
        LegacyWorldRenderSessionStatus::ready,
        "indexed map produces a complete render session"
    );
    const auto background = result.session.background_source();
    test.expect_true(
        result.session.indexed_palette.size() == 256U &&
            result.session.indexed_palette[7U] == 0x07C0U &&
            background.pixel_layout ==
                LegacyWorldBackgroundPixelLayout::indexed_8 &&
            background.palette.data() ==
                result.session.indexed_palette.data() &&
            background.tile_bytes.data() ==
                result.session.cm_cache.cache_bytes.data(),
        "first CM 0x200 bytes become the converted owned palette"
    );

    std::vector<Stage> short_stages;
    FakeMapSource short_map{short_stages};
    short_map.make_ready(8U);
    FakeCmCacheSource short_cm{short_stages};
    short_cm.result.cache_bytes.resize(0x1FFU);
    const auto short_result = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "short-cache",
            .map_id = 4U,
            .pixel_conversion = rgb565_conversion(),
        },
        short_map,
        short_cm
    );
    test.expect_equal(
        short_result.status,
        LegacyWorldRenderSessionStatus::indexed_palette_too_short,
        "short indexed CM palette is rejected after physical loading"
    );
}

void test_failures_stop_at_their_physical_stage(openswd3::test::Context& test) {
    TestTree tree;
    std::vector<Stage> stages;
    FakeMapSource map_source{stages};
    FakeCmCacheSource cm_source{stages};
    cm_source.result.status = LegacyCmCacheLoadStatus::generation_failed;
    const auto cm_failed = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "cache",
            .map_id = 24U,
            .pixel_conversion = {},
        },
        map_source,
        cm_source
    );
    test.expect_true(
        cm_failed.status ==
                LegacyWorldRenderSessionStatus::cm_cache_load_failed &&
            cm_failed.session.map_load.status ==
                LegacyWorldMapLoadStatus::pre_surface_stage_failed &&
            stages ==
                std::vector<Stage>{
                    Stage::lookup,
                    Stage::header,
                    Stage::cm,
                },
        "CM failure stops before the surface stream"
    );

    std::vector<Stage> map_stages;
    FakeMapSource bad_map{map_stages};
    bad_map.header.status = LegacyLmfMapHeaderStatus::unsupported_signature;
    FakeCmCacheSource unused_cm{map_stages};
    const auto map_failed = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "unused",
            .map_id = 24U,
            .pixel_conversion = {},
        },
        bad_map,
        unused_cm
    );
    test.expect_true(
        map_failed.status == LegacyWorldRenderSessionStatus::map_load_failed &&
            unused_cm.call_count == 0U &&
            map_stages == std::vector<Stage>{Stage::lookup, Stage::header},
        "header failure never attempts CM loading"
    );

    const std::filesystem::path blocking_file = tree.root() / "not-a-dir";
    {
        std::ofstream output{blocking_file};
        output << "x";
    }
    std::vector<Stage> directory_stages;
    FakeMapSource directory_map{directory_stages};
    FakeCmCacheSource directory_cm{directory_stages};
    const auto directory_failed = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = blocking_file / "maps",
            .map_id = 24U,
            .pixel_conversion = {},
        },
        directory_map,
        directory_cm
    );
    test.expect_true(
        directory_failed.status ==
                LegacyWorldRenderSessionStatus::cache_directory_failed &&
            directory_cm.call_count == 0U &&
            directory_stages ==
                std::vector<Stage>{Stage::lookup, Stage::header},
        "cache-directory failure stops in the pre-surface slot"
    );

    std::vector<Stage> unsupported_stages;
    FakeMapSource unsupported_map{unsupported_stages};
    unsupported_map.make_ready(12U);
    FakeCmCacheSource unsupported_cm{unsupported_stages};
    const auto unsupported = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "unsupported",
            .map_id = 24U,
            .pixel_conversion = {},
        },
        unsupported_map,
        unsupported_cm
    );
    test.expect_true(
        unsupported.status ==
                LegacyWorldRenderSessionStatus::unsupported_pixel_bits &&
            unsupported.session.map_load.status ==
                LegacyWorldMapLoadStatus::ready,
        "non-8/16-bit map is not silently assigned a pixel layout"
    );

    std::vector<Stage> object_stages;
    FakeMapSource object_map{object_stages};
    object_map.indexed.objects.emplace_back();
    object_map.indexed.objects.front().decompressed_payload.assign(8U, 0U);
    object_map.indexed.objects.front().actual_decompressed_size = 8U;
    FakeCmCacheSource object_cm{object_stages};
    std::vector<i32> object_progress;
    const auto object_failed = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "object-cache",
            .map_id = 24U,
            .pixel_conversion = {},
        },
        object_map,
        object_cm,
        {},
        [&](const i32 progress, const auto&) {
            object_progress.push_back(progress);
        }
    );
    test.expect_true(
        object_failed.status ==
                LegacyWorldRenderSessionStatus::indexed_object_prepare_failed &&
            object_failed.session.map_load.status ==
                LegacyWorldMapLoadStatus::indexed_object_stage_failed,
        "invalid indexed-object images fail in the original consumer slot"
    );
    test.expect_equal(
        object_stages,
        std::vector<Stage>{
            Stage::lookup,
            Stage::header,
            Stage::cm,
            Stage::surface,
            Stage::post_surface,
            Stage::referenced,
            Stage::offset14,
            Stage::indexed,
        },
        "indexed-object prepare failure stops before the +0x1C directory"
    );
    test.expect_equal(
        object_progress,
        std::vector<i32>{15, 60, 65, 70, 75},
        "indexed-object prepare failure stops before progress 80"
    );
}

void test_current_maps(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
) {
    const LegacyPixelConversionState conversion = rgb565_conversion();

    TestTree direct_tree;
    LegacyLmfWorldMapSource direct_map_source{archive_path};
    LegacyFileWorldCmCacheSource direct_cm_source;
    std::vector<i32> direct_progress;
    std::size_t direct_audio_maintenance_count{};
    const auto direct = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = archive_path,
            .cache_directory = direct_tree.root() / "maps",
            .map_id = 24U,
            .pixel_conversion = conversion,
        },
        direct_map_source,
        direct_cm_source,
        {},
        [&](const i32 progress, const auto&) {
            direct_progress.push_back(progress);
        },
        [&] { ++direct_audio_maintenance_count; }
    );
    test.expect_equal(
        direct.status,
        LegacyWorldRenderSessionStatus::ready,
        "current map 24 owns LMF and generated CM data"
    );
    if (direct.status == LegacyWorldRenderSessionStatus::ready) {
        test.expect_equal(
            direct_audio_maintenance_count,
            std::size_t{48U} +
                direct.session.map_load.session.referenced_records.records
                    .size() +
                direct.session.map_load.session.indexed_objects.objects.size() *
                    5U,
            "real render composition includes loader, generator, and mapping services"
        );
        test.expect_equal(
            direct_progress,
            std::vector<i32>{15, 15, 26, 37, 48, 60, 65, 70, 75, 80, 85},
            "real map 24 exposes nested CM progress before outer progress 60"
        );

        LegacyFramebuffer framebuffer;
        const auto rendered = render_legacy_world_background(
            framebuffer,
            direct.session.background_source(),
            LegacyWorldBackgroundView{}
        );
        test.expect_true(
            rendered.status == LegacyWorldBackgroundRenderStatus::completed &&
                legacy_framebuffer_logical_fnv1a64(framebuffer) ==
                    0x947C15A53487BF9AULL,
            "owned map 24 source preserves the fixed full viewport hash"
        );
    }

    TestTree indexed_tree;
    const auto indexed = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = archive_path,
            .cache_directory = indexed_tree.root() / "maps",
            .map_id = 4U,
            .pixel_conversion = conversion,
        }
    );
    test.expect_equal(
        indexed.status,
        LegacyWorldRenderSessionStatus::ready,
        "current map 4 owns its indexed CM palette and tiles"
    );
    if (indexed.status == LegacyWorldRenderSessionStatus::ready) {
        LegacyFramebuffer framebuffer;
        const auto rendered = render_legacy_world_background(
            framebuffer,
            indexed.session.background_source(),
            LegacyWorldBackgroundView{}
        );
        test.expect_true(
            rendered.status == LegacyWorldBackgroundRenderStatus::completed &&
                rendered.visited_cells == 40U * 30U &&
                indexed.session.indexed_palette.size() == 256U &&
                legacy_framebuffer_logical_fnv1a64(framebuffer) ==
                    0xF00691829E9FE2D5ULL,
            "owned indexed map preserves the fixed 40x30 viewport hash"
        );
    }

    TestTree object_tree;
    const auto object_map = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = archive_path,
            .cache_directory = object_tree.root() / "maps",
            .map_id = 72U,
            .pixel_conversion = conversion,
        }
    );
    test.expect_equal(
        object_map.status,
        LegacyWorldRenderSessionStatus::ready,
        "current map 72 normalizes its embedded pre-background image"
    );
    if (object_map.status == LegacyWorldRenderSessionStatus::ready) {
        test.expect_false(
            object_map.session.prepared_indexed_objects.objects.empty(),
            "current map 72 exposes a prepared indexed object"
        );
        if (!object_map.session.prepared_indexed_objects.objects.empty()) {
            const auto& object =
                object_map.session.prepared_indexed_objects.objects.front();
            test.expect_true(
                object.source_width == 1072U && object.source_height == 1024U &&
                    object.command_stream.size() == 1'790'338U,
                "the render-session owner keeps the real sub_401B70 dimensions"
            );
            test.expect_equal(
                fnv1a64(object.command_stream),
                std::uint64_t{0xA70AE50B232B53DEULL},
                "the render-session owner keeps the converted real stream bytes"
            );

            LegacyFramebuffer framebuffer;
            LegacyRasterGeometryState raster = framebuffer.geometry();
            LegacyBlitEffectState effects{.pixel_conversion = conversion};
            LegacyRleRowJitterState jitter;
            LegacyWorldIndexedObjectRuntimeDrawPorts draw_ports{
                framebuffer,
                raster,
                effects,
                jitter,
            };
            const i32 viewport_left = object.world_left;
            const i32 viewport_top = object.world_top;
            const auto drawn = draw_legacy_world_indexed_objects(
                object_map.session.prepared_indexed_objects.objects,
                LegacyWorldIndexedObjectViewport{
                    .left = viewport_left,
                    .top = viewport_top,
                    .right = viewport_left + 640,
                    .bottom = viewport_top + 480,
                },
                draw_ports
            );
            test.expect_true(
                drawn.status == LegacyWorldIndexedObjectDrawStatus::completed &&
                    drawn.draw_count >= 1U,
                "the real normalized stream reaches the 0x004151F0 blitter path"
            );
        }
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_original_load_slot_and_direct_source(test);
    test_indexed_palette_is_owned_and_converted(test);
    test_failures_stop_at_their_physical_stage(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_maps(test, arguments[1]);
    }
    return test.exit_code();
}
