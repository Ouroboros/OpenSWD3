#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldGuidOneArrivalByteCount = 0x200U;

class LegacyWorldMapRolePathPorts {
public:
  virtual ~LegacyWorldMapRolePathPorts() = default;

  // sub_42D920 owns chained path completion in story_scene. The original
  // caller ignores its machine return value; false here means that the
  // modern owner is not connected and must stop instead of faking it.
  [[nodiscard]] virtual bool
  complete_role_path(compat::u32 role_index) noexcept = 0;
};

struct LegacyWorldMapRolePathState {
  std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
      active_object_slots;
  LegacyWorldTalkContext talk_context;
  std::array<compat::u8, kLegacyWorldGuidOneArrivalByteCount>
      guid_one_arrival_bytes{};
};

enum class LegacyWorldMapRolePathStatus : compat::u8 {
  completed,
  invalid_selected_role_index,
  invalid_role_index,
  path_byte_out_of_range,
  direction_out_of_range,
  spatial_relocation_failed,
  surface_clear_failed,
  surface_mark_failed,
  cell_flag_refresh_failed,
  directional_probe_failed,
  path_completion_port_failed,
};

struct LegacyWorldMapRolePathResult {
  LegacyWorldMapRolePathStatus status{LegacyWorldMapRolePathStatus::completed};
  LegacyRoleSpatialRelocationStatus spatial_status{
      LegacyRoleSpatialRelocationStatus::ready};
  LegacyWorldRoleSurfaceStatus surface_status{
      LegacyWorldRoleSurfaceStatus::ready};
  LegacyWorldDirectionProbeStatus directional_probe_status{
      LegacyWorldDirectionProbeStatus::completed};
  compat::u32 slots_scanned{};
  compat::u32 active_slots{};
  compat::u32 roles_moved{};
  compat::u32 aligned_updates{};
  compat::u32 arrivals{};
  compat::u32 path_completion_calls{};
  compat::u32 cursor_advances{};
  compat::u32 action_update_count{};
  compat::u32 action_update_failure_count{};
  compat::u32 camera_recenter_count{};
  bool talk_context_created{};
};

// 0x004121A1..0x004124D1: scan exactly 72 active-object slots and maintain
// map-role path motion, spatial/surface ownership, arrival state, automatic
// Talk context, action records and selected-role camera recentering.
[[nodiscard]] LegacyWorldMapRolePathResult advance_legacy_world_map_role_paths(
    std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex &spatial_index,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 selected_role_index, compat::u8 runtime_flags,
    LegacyWorldMovementRuntimeState &movement, LegacyWorldCameraRect &camera,
    LegacyWorldMapRolePathState &state,
    asset_runtime::LegacyActionDrawPorts &action_ports,
    LegacyWorldMapRolePathPorts &path_ports);

} // namespace openswd3::world_map
