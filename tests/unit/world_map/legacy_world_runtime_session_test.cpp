#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_world_frame_coordinator.hpp"
#include "openswd3/world_map/legacy_world_runtime_session.hpp"
#include "openswd3/world_map/legacy_world_special_frame_loader.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
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
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::load_legacy_world_runtime_session;
using openswd3::world_map::LegacyCmCacheLoadResult;
using openswd3::world_map::LegacyCmCacheLoadStatus;
using openswd3::world_map::LegacyCmCacheRequest;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyWorldActionUpdaterInitializer;
using openswd3::world_map::LegacyWorldCmCacheSource;
using openswd3::world_map::LegacyWorldMapSource;
using openswd3::world_map::LegacyWorldRoleActionInitializer;
using openswd3::world_map::LegacyWorldRuntimeSessionRequest;
using openswd3::world_map::LegacyWorldRuntimeSessionStatus;

void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_i16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const i16 value
) {
    write_u16(bytes, offset, std::bit_cast<u16>(value));
}

void write_u32(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void write_role(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 map_id,
    const u16 guid,
    const u16 action_id,
    const u16 x,
    const u16 y,
    const u16 flags
) {
    write_u16(bytes, offset + 0x00U, map_id);
    write_u16(bytes, offset + 0x02U, guid);
    write_u16(bytes, offset + 0x04U, action_id);
    write_u16(bytes, offset + 0x06U, 3U);
    write_u16(bytes, offset + 0x08U, 4U);
    write_u16(bytes, offset + 0x0AU, x);
    write_u16(bytes, offset + 0x0CU, y);
    write_u16(bytes, offset + 0x0EU, 7U);
    write_u16(bytes, offset + 0x10U, 8U);
    write_i16(bytes, offset + 0x12U, -9);
    write_u16(bytes, offset + 0x14U, flags);
}

std::vector<u8> make_maps_payload() {
    std::vector<u8> bytes(0xC0U, 0U);
    write_u32(bytes, 0x04U, 0x80U);
    write_u32(bytes, 0x0CU, 0x70U);
    write_u32(bytes, 0x10U, 0x60U);
    write_u32(bytes, 0x54U, 0xB0U);

    write_u16(bytes, 0x60U, 5U);
    write_u16(bytes, 0x62U, 11U);
    write_u16(bytes, 0x64U, 12U);
    write_u16(bytes, 0x66U, 13U);
    write_u16(bytes, 0x68U, 14U);
    write_u16(bytes, 0x6AU, 15U);
    write_u16(bytes, 0x6CU, 7U);

    write_u16(bytes, 0x70U, 5U);
    write_u16(bytes, 0x72U, 9U);
    write_u16(bytes, 0x7EU, 0xFFFFU);
    write_role(bytes, 0x80U, 1U, 10000U, 2U, 5U, 6U, 0xA100U);
    write_role(bytes, 0x96U, 2U, 7U, 20U, 21U, 22U, 0xA100U);
    write_u16(bytes, 0xACU, 0xFFFFU);
    write_u16(bytes, 0xB0U, 7U);
    write_u16(bytes, 0xB2U, 0x2468U);
    write_u16(bytes, 0xB4U, 0xACE0U);
    write_u16(bytes, 0xB6U, 0U);
    return bytes;
}

class TestTree {
public:
    TestTree() {
        const auto unique =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-runtime-session-" + std::to_string(unique));
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
    explicit FakeMapSource(std::vector<std::string>& stages)
        : stages_{stages} {
        lookup.status = LegacyLmfMapLookupStatus::ready;
        lookup.map_offset = 0x1234U;
        header.status = LegacyLmfMapHeaderStatus::ready;
        header.width = 40U;
        header.height = 30U;
        header.field_88 = 16U;
        header.layers = 1U;
        surface.status = LegacyLmfSurfaceGridStatus::ready;
        surface.raw_table_values.assign(40U * 30U, 0U);
        surface.surface_grid.assign(40U * 30U * 4U, 0U);
        post.status = LegacyLmfPostSurfaceRecordsStatus::ready;
        referenced.status =
            LegacyLmfReferencedRecordDirectoryStatus::ready;
        offset14.status = LegacyLmfOffset14DirectoryStatus::ready;
        indexed.status = LegacyLmfIndexedObjectDirectoryStatus::ready;
        offset1c.status = LegacyLmfOffset1cDirectoryStatus::ready;
    }

    LegacyLmfMapLookupResult lookup_map(const u32 map_id) override {
        stages_.push_back("lookup");
        seen_map_id = map_id;
        return lookup;
    }

    LegacyLmfMapHeader read_map_header(const u32) override {
        stages_.push_back("header");
        return header;
    }

    LegacyLmfSurfaceGrid read_surface_grid(
        const u32,
        const LegacyLmfMapHeader&
    ) override {
        stages_.push_back("surface");
        return surface;
    }

    LegacyLmfPostSurfaceRecords read_post_surface_records(
        const u32,
        const LegacyLmfSurfaceGrid&
    ) override {
        stages_.push_back("post");
        return post;
    }

    LegacyLmfReferencedRecordDirectory read_referenced_record_directory(
        const u32,
        const LegacyLmfPostSurfaceRecords&
    ) override {
        stages_.push_back("referenced");
        return referenced;
    }

    LegacyLmfOffset14Directory read_offset14_directory(
        const u32,
        const LegacyLmfMapHeader&
    ) override {
        stages_.push_back("offset14");
        return offset14;
    }

    LegacyLmfIndexedObjectDirectory read_indexed_object_directory(
        const u32,
        const LegacyLmfMapHeader&
    ) override {
        stages_.push_back("indexed");
        return indexed;
    }

    LegacyLmfOffset1cDirectory read_offset1c_directory(
        const u32,
        const LegacyLmfMapHeader&
    ) override {
        stages_.push_back("offset1c");
        return offset1c;
    }

    LegacyLmfMapLookupResult lookup;
    LegacyLmfMapHeader header;
    LegacyLmfSurfaceGrid surface;
    LegacyLmfPostSurfaceRecords post;
    LegacyLmfReferencedRecordDirectory referenced;
    LegacyLmfOffset14Directory offset14;
    LegacyLmfIndexedObjectDirectory indexed;
    LegacyLmfOffset1cDirectory offset1c;
    u32 seen_map_id{};

private:
    std::vector<std::string>& stages_;
};

class FakeCmSource final : public LegacyWorldCmCacheSource {
public:
    explicit FakeCmSource(std::vector<std::string>& stages)
        : stages_{stages} {}

    LegacyCmCacheLoadResult load_cm_cache(
        const LegacyCmCacheRequest& request
    ) override {
        stages_.push_back("cm");
        seen_map_id = request.map_id;
        LegacyCmCacheLoadResult result;
        result.status = LegacyCmCacheLoadStatus::ready_hit;
        return result;
    }

    u32 seen_map_id{};

private:
    std::vector<std::string>& stages_;
};

class RecordingActionInitializer final
    : public LegacyWorldRoleActionInitializer {
public:
    explicit RecordingActionInitializer(std::vector<std::string>& stages)
        : stages_{stages} {}

    u32 initialize_action(
        openswd3::asset_runtime::LegacyActionRecord& action
    ) override {
        action_ids.push_back(action.action_id);
        stages_.push_back("action-" + std::to_string(action.action_id));
        action.mode_flags = 99U;
        return action.action_id == 2U ? 0U : 1U;
    }

    std::vector<u32> action_ids;

private:
    std::vector<std::string>& stages_;
};

class RealInitialFramePorts final
    : public openswd3::world_map::LegacyWorldFramePorts,
      public openswd3::world_map::LegacyWorldRoleExternalPorts,
      public openswd3::world_map::LegacyWorldSpatialAudioPorts,
      public openswd3::world_map::LegacyWorldOuterFramePorts {
public:
    bool complete_role_path(u32) noexcept override {
        return false;
    }

    bool query_service(u32) noexcept override {
        return false;
    }

    bool query_control(u32) noexcept override {
        return false;
    }

    bool execute_stage(
        openswd3::world_map::LegacyWorldFrameStage
    ) noexcept override {
        ++deferred_frame_stages;
        return true;
    }

    void draw_decorated_number(
        openswd3::compat::i32,
        openswd3::compat::i32,
        u32,
        u32
    ) noexcept override {}

    void play_positional_sample(
        u16,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) noexcept override {}

    const openswd3::asset_runtime::LegacyActionRecord*
    resolve_overlay_action(u32) noexcept override {
        return nullptr;
    }

    void emit_role_particles(
        openswd3::compat::i32,
        openswd3::compat::i32,
        u16
    ) noexcept override {}

    std::span<const u8> resolve_label_bytes(u32) noexcept override {
        return {};
    }

    u16 label_color(u32) noexcept override {
        return 0U;
    }

    void draw_label(
        std::span<const u8>,
        openswd3::compat::i32,
        openswd3::compat::i32,
        u16,
        u32
    ) noexcept override {}

    void play_sample(
        u16,
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) noexcept override {}

    void stop_sample(u16) noexcept override {}

    void set_sample_volume(
        u16,
        openswd3::compat::i32
    ) noexcept override {}

    void set_sample_pan(u16, openswd3::compat::i32) noexcept override {}

    bool execute_stage(
        const openswd3::world_map::LegacyWorldOuterFrameStageRequest&
    ) noexcept override {
        ++deferred_outer_stages;
        return true;
    }

    void maintain_audio() noexcept override {
        ++audio_services;
    }

    void request_world_presentation() noexcept override {
        ++presentations;
    }

    u32 deferred_frame_stages{};
    u32 deferred_outer_stages{};
    u32 audio_services{};
    u32 presentations{};
};

openswd3::rendering::LegacyPixelConversionState rgb565_conversion() {
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

void test_world_assembly_slot(openswd3::test::Context& test) {
    TestTree tree;
    std::vector<u8> payload = make_maps_payload();
    const auto decoded = decode_legacy_maps_world_database(payload);
    std::vector<std::string> stages;
    FakeMapSource map_source{stages};
    FakeCmSource cm_source{stages};
    RecordingActionInitializer action_initializer{stages};
    auto result = load_legacy_world_runtime_session(
        payload,
        LegacyWorldRuntimeSessionRequest{
            .archive_path = "huge.lmf",
            .cache_directory = tree.root() / "cache" / "maps",
            .load = decoded.database.initial_load,
            .cache_limit_megabytes = 60U,
            .pixel_conversion = rgb565_conversion(),
        },
        action_initializer,
        map_source,
        cm_source
    );

    test.expect_equal(
        result.status,
        LegacyWorldRuntimeSessionStatus::ready,
        "a complete MAPS plus LMF request creates a world owner"
    );
    test.expect_true(
        map_source.seen_map_id == 9U && cm_source.seen_map_id == 9U &&
            result.session.logical_map_id == 5U &&
            result.session.map_descriptor.archive_map_id == 9U,
        "logical map five selects archive map nine without conflating IDs"
    );
    test.expect_equal(
        stages,
        std::vector<std::string>{
            "lookup", "header", "cm", "surface", "post", "referenced",
            "offset14", "indexed", "offset1c", "action-2", "action-13",
        },
        "MAPS roles and action initialization occupy the pre-binding slot"
    );
    const auto& post_state = result.session.role_post_materialization;
    test.expect_true(
        result.session.maps_role_count == 2U &&
            result.session.selected_role_index == 2U &&
            result.session.action_update_failure_count == 1U &&
            result.session.role_post_materialization_status ==
                openswd3::world_map::
                    LegacyWorldRolePostMaterializationStatus::ready &&
            post_state.party_role_count == 1U &&
            post_state.party_role_indices[0] == 2U,
        "migrated roles retain action and selected-party post-load state"
    );

    auto& map_session = result.session.render.map_load.session;
    auto& roles = map_session.business.state.roles;
    test.expect_true(
        roles.size() == 3U && roles[1].guid == 10000U &&
            roles[1].map_cell_pointer_32 == 245U && roles[2].guid == 7U &&
            roles[2].world_x == 176U && roles[2].world_y == 192U &&
            roles[2].action.action_id == 13U &&
            roles[2].action.base_variant == 14U &&
            roles[2].action.variant_delta == 15U &&
            roles[2].field_2c == 0x2468U &&
            roles[2].field_30 == 0xACE0ACE0U &&
            roles[2].action.mode_flags == 0U,
        "source fields/defaults are materialized before final cell binding"
    );
    test.expect_true(
        result.session.camera.left == 0U && result.session.camera.top == 0U &&
            result.session.camera.right == 640U &&
            result.session.camera.bottom == 480U,
        "sub_40D0C0 centers then clamps the selected role camera"
    );
}

void test_explicit_failure_boundaries(openswd3::test::Context& test) {
    TestTree tree;
    std::vector<std::string> stages;
    FakeMapSource map_source{stages};
    FakeCmSource cm_source{stages};
    RecordingActionInitializer action_initializer{stages};

    std::vector<u8> payload = make_maps_payload();
    auto decoded = decode_legacy_maps_world_database(payload);
    auto request = LegacyWorldRuntimeSessionRequest{
        .archive_path = "huge.lmf",
        .cache_directory = tree.root() / "cache" / "maps",
        .load = decoded.database.initial_load,
        .cache_limit_megabytes = 60U,
        .pixel_conversion = rgb565_conversion(),
    };
    request.load.logical_map_id = 6U;
    test.expect_equal(
        load_legacy_world_runtime_session(
            payload,
            request,
            action_initializer,
            map_source,
            cm_source
        ).status,
        LegacyWorldRuntimeSessionStatus::map_descriptor_not_found,
        "an absent logical descriptor stops before LMF lookup"
    );

    request.load = decoded.database.initial_load;
    request.load.load_flags = 1U;
    test.expect_equal(
        load_legacy_world_runtime_session(
            payload,
            request,
            action_initializer,
            map_source,
            cm_source
        ).status,
        LegacyWorldRuntimeSessionStatus::preload_coordinate_stage_required,
        "bit-zero load requests cannot silently skip sub_40D200"
    );

    const std::array<openswd3::world_map::LegacyWorldRoleRecord, 1U>
        prior_roles{};
    const openswd3::world_map::LegacyWorldRolePreloadContext
        preload_context{
            .path_database = {},
            .roles = prior_roles,
            .object_slots = {},
            .controlled_role_index = 0U,
            .current_map_width = 40U,
            .current_map_height = 30U,
        };
    request.preload_context = &preload_context;
    const auto preloaded = load_legacy_world_runtime_session(
        payload,
        request,
        action_initializer,
        map_source,
        cm_source
    );
    test.expect_true(
        preloaded.status == LegacyWorldRuntimeSessionStatus::ready &&
            preloaded.session.role_preload_applied &&
            preloaded.session.role_preload.roles_visited == 0U,
        "bit-zero load requests run sub_40D200 when prior world state exists"
    );

    request.load = decoded.database.initial_load;
    request.load.selected_guid = 99U;
    const auto missing = load_legacy_world_runtime_session(
        payload,
        request,
        action_initializer,
        map_source,
        cm_source
    );
    test.expect_equal(
        missing.status,
        LegacyWorldRuntimeSessionStatus::maps_load_apply_failed,
        "a missing selected GUID is exposed at the original post-LMF slot"
    );
}

void test_real_initial_world(
    openswd3::test::Context& test,
    const std::filesystem::path& data_root
) {
    std::ifstream input(data_root / "MAPS.DAT", std::ios::binary);
    std::vector<u8> file_bytes{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
    test.expect_true(
        input.is_open() && file_bytes.size() > 0x200U,
        "real initial-world test owns the complete MAPS payload"
    );
    if (!input.is_open() || file_bytes.size() <= 0x200U) {
        return;
    }

    std::vector<u8> payload(
        file_bytes.begin() + 0x200,
        file_bytes.end()
    );
    const auto decoded = decode_legacy_maps_world_database(payload);
    test.expect_equal(
        decoded.status,
        LegacyMapsWorldDatabaseStatus::ready,
        "real MAPS database decodes before world creation"
    );
    if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
        return;
    }

    TestTree tree;
    openswd3::asset_runtime::LegacyActRuntime act_runtime{data_root};
    openswd3::asset_runtime::LegacyActActionStreamProvider provider{
        act_runtime
    };
    openswd3::asset_runtime::LegacyActionUpdater updater{provider};
    LegacyWorldActionUpdaterInitializer action_initializer{updater};
    auto result = load_legacy_world_runtime_session(
        payload,
        LegacyWorldRuntimeSessionRequest{
            .archive_path = data_root / "huge.lmf",
            .cache_directory = tree.root() / "cache" / "maps",
            .load = decoded.database.initial_load,
            .cache_limit_megabytes = 60U,
            .pixel_conversion = rgb565_conversion(),
        },
        action_initializer
    );
    test.expect_equal(
        result.status,
        LegacyWorldRuntimeSessionStatus::ready,
        "current game data creates the first real world session"
    );
    if (result.status != LegacyWorldRuntimeSessionStatus::ready) {
        return;
    }

    auto& map_session = result.session.render.map_load.session;
    auto& roles = map_session.business.state.roles;
    const auto& selected = roles[result.session.selected_role_index];
    const auto& post_state = result.session.role_post_materialization;
    test.expect_true(
        result.session.logical_map_id == 81U &&
            result.session.map_descriptor.archive_map_id == 81U &&
            result.session.maps_role_count == 12U &&
            result.session.maps_load_apply.reserved_records_moved == 2U &&
            result.session.duplicate_role_count == 0U &&
            result.session.out_of_bounds_role_count == 0U &&
            result.session.action_update_failure_count == 0U &&
            post_state.party_role_count == 1U &&
            post_state.party_role_indices[0] ==
                result.session.selected_role_index &&
            post_state.gated_roles_scanned == 0U &&
            post_state.flagged_role_record_count == 0U &&
            post_state.guid_one_roles_overridden == 0U,
        "real new game loads the exact twelve MAPS roles without repair"
    );
    test.expect_true(
        selected.guid == 1U && selected.world_x == 13U * 16U &&
            selected.world_y == 28U * 16U &&
            selected.action.action_id == 1U &&
            selected.action.base_variant == 0U &&
            selected.action.variant_delta == 3U,
        "selected GUID one materializes the seven-word initial record"
    );
    test.expect_true(
        map_session.role_cell_binding.roles_bound == roles.size() - 1U &&
            result.session.camera.right - result.session.camera.left == 640U &&
            result.session.camera.bottom - result.session.camera.top == 480U,
        "all physical and MAPS roles bind before the 640x480 camera is fixed"
    );

    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster =
        framebuffer.geometry();
    openswd3::rendering::LegacyRleRowJitterState jitter;
    const openswd3::rendering::LegacyBlitEffectState effects{
        .pixel_conversion = rgb565_conversion(),
    };
    openswd3::world_map::LegacyWorldSpecialFrameLoader special_frame_loader{
        data_root / "huge.lmf",
        map_session.lookup.map_offset,
        map_session.referenced_records.records,
        rgb565_conversion(),
    };
    openswd3::asset_runtime::LegacyTswRuntime tsw_runtime{
        data_root,
        rgb565_conversion(),
        &special_frame_loader
    };
    tsw_runtime.set_cache_limit(0x01000000U);
    openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
        updater,
        tsw_runtime,
        framebuffer,
        raster,
        effects,
        jitter,
    };
    RealInitialFramePorts deferred_ports;
    openswd3::world_map::LegacyWorldRoleRenderRuntimePorts role_ports{
        tsw_runtime,
        framebuffer,
        raster,
        effects,
        deferred_ports,
    };
    std::vector<i16> distances(roles.size());
    std::vector<i16> vertical_offsets(roles.size());
    std::array<i16, 1U> selection_words{
        std::bit_cast<i16>(
            openswd3::world_map::kLegacyWorldSelectionSentinel
        )
    };
    openswd3::world_map::LegacyWorldFrameCoordinatorState frame_state;
    frame_state.map_id = result.session.logical_map_id;
    frame_state.player_role_index = result.session.selected_role_index;
    frame_state.company_role_count = 1U;
    frame_state.tile_animation = {
        .cycle_counter = 1,
        .cycle_interval = std::max(
            static_cast<openswd3::compat::i32>(
                result.session.map_descriptor.field_08
            ),
            1
        ),
        .frame_count = map_session.header.layers,
        .frame_index = 0U,
        .frame_direction = 1,
        .tile_layer_stride =
            map_session.header.width * map_session.header.height,
        .tile_layer_offset = 0U,
    };
    frame_state.frame_runtime.spatial_audio = {
        .controlled_role_index = result.session.selected_role_index,
        .mix_level = 0,
        .distance_by_role = distances,
        .vertical_offset_by_role = vertical_offsets,
    };
    frame_state.selection_scroll.saved_left = result.session.camera.left;
    frame_state.selection_scroll.saved_top = result.session.camera.top;
    openswd3::world_map::initialize_legacy_world_player_position_history(
        frame_state.player_post_frame,
        roles[result.session.selected_role_index]
    );

    const auto first_frame = openswd3::world_map::run_legacy_world_frame(
        framebuffer,
        raster,
        result.session.render.background_source(),
        map_session.business.state.spatial_index,
        roles,
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = map_session.header.width,
            .selected_guid = roles[result.session.selected_role_index].guid,
            .surface_grid = map_session.surface_grid.surface_grid,
        },
        selection_words,
        result.session.camera,
        frame_state,
        jitter,
        {
            deferred_ports,
            action_ports,
            role_ports,
            deferred_ports,
        },
        deferred_ports
    );
    test.expect_equal(
        first_frame.status,
        openswd3::world_map::LegacyWorldFrameCoordinatorStatus::completed,
        "the exact initial MAPS/LMF/ACT owner completes one ordinary world frame"
    );
    test.expect_equal(
        first_frame.frame.status,
        openswd3::world_map::LegacyWorldFrameRuntimeStatus::completed,
        "the exact initial owner completes the inner composition"
    );
    test.expect_true(
        first_frame.head_sign_actions.status ==
                openswd3::world_map::LegacyWorldHeadSignActionsStatus::
                    completed &&
            first_frame.head_sign_actions.update_count == 4U &&
            first_frame.head_sign_actions.update_failure_count == 0U,
        "the exact initial ACT owner advances all four HeadSgn variants"
    );
    test.expect_true(
        first_frame.player_post_frame.status ==
                openswd3::world_map::LegacyWorldPlayerPostFrameStatus::
                    completed &&
            first_frame.player_post_frame.aligned &&
            first_frame.player_post_frame.spatially_relocated &&
            first_frame.player_post_frame.old_occupancy_cleared &&
            first_frame.player_post_frame.new_occupancy_marked &&
            first_frame.player_post_frame.transitions_cleared &&
            !first_frame.player_post_frame.history_shifted &&
            first_frame.player_post_frame.map_cell_delta == 0U,
        "the exact initial owner completes post-present player bookkeeping"
    );
    test.expect_equal(
        deferred_ports.presentations,
        1U,
        "the exact initial owner reaches the original presentation slot"
    );
    test.expect_equal(
        deferred_ports.audio_services,
        2U,
        "the exact initial owner services audio at both original slots"
    );
}

}  // namespace

int main(const int argc, char** argv) {
    openswd3::test::Context test;
    test_world_assembly_slot(test);
    test_explicit_failure_boundaries(test);
    if (argc == 2) {
        test_real_initial_world(test, argv[1]);
    }
    return test.exit_code();
}
