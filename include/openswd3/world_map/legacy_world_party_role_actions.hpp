#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <span>

namespace openswd3::world_map {

enum class LegacyWorldPartyRoleActionsStatus : compat::u8 {
  completed,
  invalid_party_role_count,
  invalid_role_index,
  path_byte_out_of_range,
  direction_out_of_range,
  spatial_removal_failed,
  surface_clear_failed,
  surface_mark_failed,
  cell_flag_refresh_failed,
};

struct LegacyWorldPartyRoleActionsResult {
  LegacyWorldPartyRoleActionsStatus status{
      LegacyWorldPartyRoleActionsStatus::completed};
  LegacyRoleSpatialRelocationStatus spatial_status{
      LegacyRoleSpatialRelocationStatus::ready};
  LegacyWorldRoleSurfaceStatus surface_status{
      LegacyWorldRoleSurfaceStatus::ready};
  compat::u32 slots_scanned{};
  compat::u32 populated_slots{};
  compat::u32 active_path_slots{};
  compat::u32 roles_moved{};
  compat::u32 aligned_updates{};
  compat::u32 cursor_advances{};
  compat::u32 action_update_count{};
  compat::u32 action_update_failure_count{};
};

// 0x004124DC..0x00412681: update party slots 1..count-1. Unlike ordinary
// map-role paths, an aligned party follower is removed from the map spatial
// chain without reinsertion, and every populated slot still updates action
// state when its path cursor is inactive or its action wait is nonzero.
[[nodiscard]] LegacyWorldPartyRoleActionsResult
advance_legacy_world_party_role_actions(
    std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex &spatial_index,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 party_role_count,
    std::span<LegacyWorldObjectSlot> party_object_slots,
    asset_runtime::LegacyActionDrawPorts &action_ports);

} // namespace openswd3::world_map
