#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_render_session.hpp"
#include "openswd3/world_map/legacy_world_role_preload.hpp"

#include <filesystem>
#include <span>

namespace openswd3::world_map {

class LegacyWorldRoleActionInitializer {
public:
    virtual ~LegacyWorldRoleActionInitializer() = default;

    [[nodiscard]] virtual compat::u32 initialize_action(
        asset_runtime::LegacyActionRecord& action
    ) = 0;
};

class LegacyWorldActionUpdaterInitializer final
    : public LegacyWorldRoleActionInitializer {
public:
    explicit LegacyWorldActionUpdaterInitializer(
        asset_runtime::LegacyActionUpdater& updater
    ) noexcept;

    [[nodiscard]] compat::u32 initialize_action(
        asset_runtime::LegacyActionRecord& action
    ) override;

private:
    asset_runtime::LegacyActionUpdater& updater_;
};

enum class LegacyWorldRuntimeSessionStatus {
    ready,
    maps_database_failed,
    map_descriptor_not_found,
    preload_coordinate_stage_required,
    preload_role_synchronization_failed,
    render_session_failed,
    maps_load_apply_failed,
    role_capacity_exceeded,
    role_spatial_insertion_failed,
    role_source_write_failed,
    selected_role_not_materialized,
    allocation_failed,
};

struct LegacyWorldRuntimeSessionRequest {
    std::filesystem::path archive_path;
    std::filesystem::path cache_directory;
    LegacyWorldLoadRequest load;
    compat::u32 cache_limit_megabytes{60U};
    rendering::LegacyPixelConversionState pixel_conversion;
    const LegacyWorldRolePreloadContext* preload_context{};
};

struct LegacyWorldRuntimeSession {
    LegacyMapsWorldDatabase maps_database;
    LegacyMapsWorldLoadApplyResult maps_load_apply;
    LegacyMapsMapDescriptor map_descriptor;
    LegacyWorldRenderSession render;
    LegacyWorldCameraRect camera;
    compat::u32 logical_map_id{};
    compat::u32 selected_role_index{};
    compat::u32 maps_role_count{};
    compat::u32 duplicate_role_count{};
    compat::u32 out_of_bounds_role_count{};
    compat::u32 action_update_failure_count{};
    LegacyWorldRolePreloadResult role_preload;
    bool role_preload_applied{};
};

struct LegacyWorldRuntimeSessionResult {
    LegacyWorldRuntimeSessionStatus status{
        LegacyWorldRuntimeSessionStatus::maps_database_failed
    };
    LegacyMapsWorldDatabaseStatus maps_database_status{
        LegacyMapsWorldDatabaseStatus::payload_header_truncated
    };
    LegacyWorldRenderSessionStatus render_status{
        LegacyWorldRenderSessionStatus::map_load_failed
    };
    LegacyWorldRuntimeSession session;
};

[[nodiscard]] LegacyWorldRuntimeSessionResult
load_legacy_world_runtime_session(
    std::span<compat::u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer,
    LegacyWorldMapSource& map_source,
    LegacyWorldCmCacheSource& cm_cache_source
);

[[nodiscard]] LegacyWorldRuntimeSessionResult
load_legacy_world_runtime_session(
    std::span<compat::u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer
);

}  // namespace openswd3::world_map
