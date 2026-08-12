#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_path_requests.hpp"

#include <span>

namespace openswd3::world_map {

enum class LegacyWorldPathScriptStatus : compat::u8 {
  completed,
  invalid_role_index,
  path_directory_entry_out_of_range,
  path_command_out_of_range,
  path_command_truncated,
  insufficient_object_slots,
  path_request_failed,
  directional_probe_failed,
  direction_out_of_range,
  unsupported_opcode,
};

struct LegacyWorldPathScriptResult {
  LegacyWorldPathScriptStatus status{LegacyWorldPathScriptStatus::completed};
  LegacyWorldRolePathRequestStatus path_request_status{
      LegacyWorldRolePathRequestStatus::completed};
  LegacyWorldPathfindingStatus pathfinding_status{
      LegacyWorldPathfindingStatus::completed};
  LegacyWorldDirectionProbeStatus directional_probe_status{
      LegacyWorldDirectionProbeStatus::completed};
  compat::u32 opcodes_dispatched{};
  compat::u32 path_requests{};
  compat::u32 movement_slots_advanced{};
  compat::u32 movement_slots_completed{};
  compat::u32 cursor_words_advanced{};
  compat::u32 waits_set{};
  compat::u32 waits_decremented{};
  compat::u16 last_opcode{};
};

// The currently restored, directly executable portion of sub_405500. This
// owner implements opcodes 0, 4, 5, 7 and 8, which form the ordinary role
// wait/request/move loop. Other opcodes stop at an explicit boundary until
// their assembly cases are restored.
[[nodiscard]] LegacyWorldPathScriptResult run_legacy_world_path_script(
    compat::u32 role_index, std::span<const compat::u8> path_database,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 map_height, std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool);

struct LegacyWorldPathScriptScanResult {
  LegacyWorldPathScriptStatus status{LegacyWorldPathScriptStatus::completed};
  LegacyWorldPathScriptResult last_role_result;
  compat::u32 roles_scanned{};
  compat::u32 eligible_roles{};
  compat::u32 scripts_completed{};
  compat::u32 unsupported_scripts{};
};

// Relevant ordinary-role branches of sub_405430. Party roles remain owned by
// prepare_legacy_world_party_paths at the same application stage.
[[nodiscard]] LegacyWorldPathScriptScanResult run_legacy_world_path_scripts(
    std::span<const compat::u8> path_database,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 map_height, std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool);

} // namespace openswd3::world_map
