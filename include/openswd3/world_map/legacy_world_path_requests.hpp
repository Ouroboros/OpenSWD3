#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_pathfinding.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_player_post_frame.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <span>

namespace openswd3::world_map {

enum class LegacyWorldRolePathRequestStatus : compat::u8 {
  completed,
  invalid_role_index,
  command_truncated,
  insufficient_object_slots,
  pathfinding_boundary_failed,
  path_does_not_fit_slot,
};

struct LegacyWorldRolePathRequestResult {
  LegacyWorldRolePathRequestStatus status{
      LegacyWorldRolePathRequestStatus::completed};
  LegacyWorldPathfindingStatus pathfinding_status{
      LegacyWorldPathfindingStatus::completed};
  compat::u32 slot_index{};
  compat::i32 legacy_return_value{};
  bool free_slot_found{};
  bool target_in_legacy_bounds{};
  bool path_found{};
  bool role_relocated_after_path_failure{};
};

// 0x00406390: consume one path command containing destination tile X/Y at
// +2/+4, use the first free one of 72 ordinary object slots, and generate a
// route for the selected map role. Its machine return is the historical
// "slot index != 32" predicate, not a path-success predicate.
[[nodiscard]] LegacyWorldRolePathRequestResult request_legacy_world_role_path(
    compat::u32 role_index, std::span<const compat::u8> path_command,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 map_height, std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool);

enum class LegacyWorldPartyPathPreparationStatus : compat::u8 {
  completed,
  invalid_selected_role_index,
  invalid_party_role_count,
  invalid_party_role_index,
  history_index_out_of_range,
  pathfinding_boundary_failed,
  path_does_not_fit_slot,
  path_cursor_out_of_range,
  direction_out_of_range,
  surface_clear_failed,
  surface_mark_failed,
  directional_probe_failed,
};

struct LegacyWorldPartyPathPreparationResult {
  LegacyWorldPartyPathPreparationStatus status{
      LegacyWorldPartyPathPreparationStatus::completed};
  LegacyWorldPathfindingStatus pathfinding_status{
      LegacyWorldPathfindingStatus::completed};
  LegacyWorldRoleSurfaceStatus surface_status{
      LegacyWorldRoleSurfaceStatus::ready};
  LegacyWorldDirectionProbeStatus directional_probe_status{
      LegacyWorldDirectionProbeStatus::completed};
  LegacyRoleSpatialRelocationStatus spatial_removal_status{
      LegacyRoleSpatialRelocationStatus::ready};
  compat::u32 roles_scanned{};
  compat::u32 eligible_roles{};
  compat::u32 unaligned_roles{};
  compat::u32 missing_party_roles{};
  compat::u32 paths_reused{};
  compat::u32 paths_generated{};
  compat::u32 pathfinding_failures{};
  compat::u32 preadvanced_steps{};
  compat::u32 spatial_removal_failures{};
  compat::u32 movement_slots_enabled{};
  compat::u32 movement_slots_blocked{};
  compat::u32 terminal_paths{};
  compat::u32 collision_service_queries{};
};

class LegacyWorldPartyPathPorts {
public:
  virtual ~LegacyWorldPartyPathPorts() = default;

  [[nodiscard]] virtual bool query_collision_disabled() noexcept = 0;
};

// Relevant 0x00405430 gate plus 0x00406960: before the ordinary-world outer
// frame, prepare/reuse follower routes for roles carrying both legacy party
// and spatial flags, preadvance off-screen followers, and arm their 0x21C
// slots for the existing 0x004124DC consumer.
[[nodiscard]] LegacyWorldPartyPathPreparationResult
prepare_legacy_world_party_paths(
    std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex &spatial_index,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 selected_role_index, compat::u32 party_role_count,
    std::span<const compat::u32> party_role_indices,
    std::span<LegacyWorldObjectSlot> party_object_slots,
    const LegacyWorldPlayerPostFrameState &player_history,
    const LegacyWorldCameraRect &camera, LegacyWorldPathNodePool &node_pool,
    LegacyWorldPartyPathPorts &ports);

} // namespace openswd3::world_map
