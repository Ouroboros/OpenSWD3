#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_pathfinding.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <span>

namespace openswd3::world_map {

struct LegacyWorldStoryPathRuntime {
    std::span<LegacyWorldRoleRecord> roles;
    std::span<LegacyWorldObjectSlot> active_object_slots;
    LegacyRoleSpatialIndex* spatial_index{};
    LegacyWorldRoleSurfaceContext role_surface{};
    LegacyWorldPathNodePool* node_pool{};
    LegacyWorldMovementRuntimeState* movement{};
    LegacyWorldCameraRect* camera{};
    std::span<compat::u8> selected_arrival_bytes;
    compat::u32 selected_role_index{};
    compat::u32 map_height{};
    compat::u8* scene_render_flags{};
};

enum class LegacyWorldStoryPathStatus : compat::u8 {
    completed,
    runtime_unavailable,
    invalid_role_index,
    invalid_selected_role_index,
    invalid_map_dimensions,
    pathfinding_failed,
    path_does_not_fit_slot,
    path_cursor_out_of_range,
    direction_out_of_range,
    surface_clear_failed,
    surface_mark_failed,
    spatial_relocation_failed,
    directional_probe_failed,
    allocation_failed,
};

struct LegacyWorldStoryPathResult {
    LegacyWorldStoryPathStatus status{LegacyWorldStoryPathStatus::completed};
    LegacyWorldPathfindingStatus pathfinding_status{
        LegacyWorldPathfindingStatus::completed
    };
    LegacyWorldRoleSurfaceStatus surface_status{
        LegacyWorldRoleSurfaceStatus::ready
    };
    LegacyRoleSpatialRelocationStatus spatial_status{
        LegacyRoleSpatialRelocationStatus::ready
    };
    LegacyWorldDirectionProbeStatus directional_probe_status{
        LegacyWorldDirectionProbeStatus::completed
    };
    compat::u32 slot_index{kLegacyWorldActiveObjectSlotCount};
    compat::i32 legacy_return_value{};
    compat::u32 preadvanced_steps{};
    bool existing_slot_found{};
    bool free_slot_allocated{};
    bool path_found{};
    bool direct_move{};
    bool slot_cleared{};
};

struct LegacyWorldStoryPathRequest {
    compat::u32 role_index{};
    compat::u16 destination_x{};
    compat::u16 destination_y{};
    compat::u32 flags{};
    compat::i16 action_id{-1};
    compat::i16 base_variant{-1};
    compat::i16 variant_delta{-1};
};

// sub_42E5A0: suspend one story-controlled role, preserving an ordinary
// path for later restoration and reconciling sub-cell movement first.
[[nodiscard]] LegacyWorldStoryPathResult suspend_legacy_world_story_role(
    LegacyWorldStoryPathRuntime& runtime, compat::u32 role_index
) noexcept;

// sub_42DAF0: schedule one story-controlled role path, preserving the
// ordinary 72-slot format consumed by advance_legacy_world_map_role_paths.
[[nodiscard]] LegacyWorldStoryPathResult schedule_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime,
    const LegacyWorldStoryPathRequest& request
) noexcept;

// sub_42E280: arm the next four-pixel path segment. The historical return is
// 0=no matching type-2 slot, 1=still moving, 2=at destination.
[[nodiscard]] LegacyWorldStoryPathResult query_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime, compat::u32 role_index
) noexcept;

// sub_42D920: complete a type>1 slot, restoring a chained path when present.
// Its historical return is one unless no matching slot was found.
[[nodiscard]] LegacyWorldStoryPathResult complete_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime, compat::u32 role_index
) noexcept;

}  // namespace openswd3::world_map
