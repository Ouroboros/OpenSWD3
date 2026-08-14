#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::world_map {

enum class LegacyWorldRoleTableResetStatus {
    ready,
    role_span_exceeds_capacity,
    highest_role_index_out_of_range,
    active_role_range_out_of_bounds,
};

struct LegacyWorldRoleTableResetResult {
    LegacyWorldRoleTableResetStatus status{
        LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity
    };
    compat::u32 payload_slots_scanned{};
    compat::u32 payload_owners_released{};
    compat::u32 roles_zeroed{};
    compat::u32 action_records_initialized{};
};

// sub_40F3B0 (0x0040F3B0..0x0040F40D): release +0x38 for inclusive
// indices 0..highest, zero the complete physical role table, then apply
// sub_40DC00 to every +0x40 action record. A negative highest index skips
// only the release loop.
[[nodiscard]] LegacyWorldRoleTableResetResult reset_legacy_world_role_table(
    std::span<LegacyWorldRoleRecord> roles,
    std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>&
        role_label_payloads,
    compat::i32 highest_role_index
) noexcept;

enum class LegacyWorldRoleTransitionStatus : compat::u8 {
    ready,
    invalid_role_index,
    surface_clear_failed,
    path_cursor_out_of_range,
    direction_out_of_range,
    path_completion_failed,
    action_update_failed,
};

struct LegacyWorldRoleTransitionResult {
    LegacyWorldRoleTransitionStatus status{
        LegacyWorldRoleTransitionStatus::ready
    };
    LegacyWorldRoleSurfaceStatus surface_status{
        LegacyWorldRoleSurfaceStatus::ready
    };
    asset_runtime::LegacyActionUpdateStatus action_update_status{
        asset_runtime::LegacyActionUpdateStatus::completed
    };
    compat::u32 slots_scanned{};
    compat::u32 matching_slots{};
    compat::u32 surface_clear_calls{};
    compat::u32 coordinate_alignment_slots{};
    compat::u32 path_completion_calls{};
    compat::u32 action_update_calls{};
    bool ownership_flag_set{};
};

// sub_40F6D0 plus its sole caller's following role+0x26 word clear. For a
// role carrying bit 31, scan all 72 object slots, reconcile any sub-cell
// coordinates, finish the story path, restore pending action overrides and
// clear the ownership/wait state. The caller clears interaction_gate even
// when sub_40F6D0 takes its bit-31 early return.
[[nodiscard]] LegacyWorldRoleTransitionResult
release_legacy_world_role_transition(
    std::span<LegacyWorldRoleRecord> roles,
    std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    compat::u32 role_index,
    compat::u32 selected_role_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    LegacyWorldMapRolePathPorts& path_ports,
    asset_runtime::LegacyActionDrawPorts& action_ports
);

}  // namespace openswd3::world_map
