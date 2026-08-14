#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_random_encounter.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_render_session.hpp"
#include "openswd3/world_map/legacy_world_role_post_materialization.hpp"
#include "openswd3/world_map/legacy_world_role_preload.hpp"

#include <array>
#include <filesystem>
#include <span>

namespace openswd3::world_map {

class LegacyWorldRoleActionInitializer {
public:
    virtual ~LegacyWorldRoleActionInitializer() = default;

    [[nodiscard]] virtual compat::u32
    initialize_action(asset_runtime::LegacyActionRecord& action) = 0;
};

class LegacyWorldActionUpdaterInitializer final
    : public LegacyWorldRoleActionInitializer {
public:
    explicit LegacyWorldActionUpdaterInitializer(
        asset_runtime::LegacyActionUpdater& updater
    ) noexcept;

    [[nodiscard]] compat::u32
    initialize_action(asset_runtime::LegacyActionRecord& action) override;

private:
    asset_runtime::LegacyActionUpdater& updater_;
};

class LegacyWorldRuntimeRandom {
public:
    virtual ~LegacyWorldRuntimeRandom() = default;

    [[nodiscard]] virtual compat::u32 next_bounded(compat::u32 upper_bound) = 0;
};

struct LegacyWorldDirectionalPoint {
    compat::u32 target_interval{};
    compat::u32 variant{};
    compat::u32 world_x{};
    compat::u32 world_y{};
    compat::i32 velocity_x{};
    compat::i32 velocity_y{};
};

enum class LegacyWorldRuntimeSessionStatus {
    ready,
    maps_database_failed,
    encounter_thresholds_failed,
    encounter_regions_failed,
    map_descriptor_not_found,
    preload_coordinate_stage_required,
    preload_role_synchronization_failed,
    render_session_failed,
    maps_load_apply_failed,
    role_capacity_exceeded,
    role_spatial_insertion_failed,
    role_post_materialization_failed,
    role_source_write_failed,
    role_cell_binding_failed,
    role_surface_occupancy_failed,
    selected_role_not_materialized,
    allocation_failed,
};

struct LegacyWorldMapDescriptorRuntimeState {
    compat::u32 behavior_index{};
    compat::u32 base_movement_step{4U};
    compat::u32 tile_animation_interval{1U};
    compat::u32 encounter_table_index{};
    compat::u32 map_state_flags{};
    compat::i32 role_red_offset{};
    compat::i32 role_green_offset{};
    compat::i32 role_blue_offset{};
    compat::u32 enabled_service_bits{};
    bool environment_enabled{true};
    bool directional_effect_enabled{};
    compat::u32 directional_base_variant{64U};
    compat::u32 directional_variant_count{4U};
    bool encounter_table_index_repaired{};
};

struct LegacyWorldRuntimeSessionRequest {
    std::filesystem::path archive_path;
    std::filesystem::path cache_directory;
    LegacyWorldLoadRequest load;
    compat::u32 cache_limit_megabytes{60U};
    rendering::LegacyPixelConversionState pixel_conversion;
    const LegacyWorldRolePreloadContext* preload_context{};
    const LegacyWorldRolePostMaterializationContext*
        post_materialization_context{};
    LegacyWorldRuntimeRandom* random{};
};

struct LegacyWorldRuntimeSession {
    LegacyMapsWorldDatabase maps_database;
    LegacyMapsWorldLoadApplyResult maps_load_apply;
    LegacyMapsMapDescriptor map_descriptor;
    LegacyWorldMapDescriptorRuntimeState map_descriptor_runtime;
    LegacyEncounterThresholdSourceResult encounter_thresholds;
    LegacyEncounterRegionSourceResult encounter_regions;
    std::array<LegacyWorldDirectionalPoint, 4U> directional_points;
    LegacyWorldRenderSession render;
    LegacyWorldCameraRect camera;
    std::array<compat::u8, kLegacyMapsCurrentMapNameCapacity> map_name{};
    LegacyMapsMapNameLookupResult map_name_lookup;
    compat::u32 logical_map_id{};
    compat::u32 selected_role_index{};
    compat::u32 maps_role_count{};
    compat::u32 duplicate_role_count{};
    compat::u32 out_of_bounds_role_count{};
    compat::u32 action_update_failure_count{};
    LegacyWorldRolePreloadResult role_preload;
    LegacyWorldRolePostMaterializationState role_post_materialization;
    LegacyWorldRolePostMaterializationStatus role_post_materialization_status{
        LegacyWorldRolePostMaterializationStatus::ready
    };
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

[[nodiscard]] LegacyWorldRuntimeSessionResult load_legacy_world_runtime_session(
    std::span<compat::u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer,
    LegacyWorldMapSource& map_source,
    LegacyWorldCmCacheSource& cm_cache_source
);

[[nodiscard]] LegacyWorldRuntimeSessionResult load_legacy_world_runtime_session(
    std::span<compat::u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer
);

}  // namespace openswd3::world_map
