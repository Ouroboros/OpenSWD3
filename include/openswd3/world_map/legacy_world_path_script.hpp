#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_path_requests.hpp"

#include <span>
#include <array>
#include <vector>

namespace openswd3::input_time_rng {
class LegacyCrtRng;
class LegacySecondaryRng;
}

namespace openswd3::world_map {

struct LegacyRoleSpatialIndex;
struct LegacyWorldStoryVmState;

enum class LegacyWorldPathScriptStatus : compat::u8 {
  completed,
  invalid_role_index,
  path_directory_entry_out_of_range,
  path_command_out_of_range,
  path_command_truncated,
  runtime_unavailable,
  insufficient_object_slots,
  path_request_failed,
  directional_probe_failed,
  direction_out_of_range,
  indeterminate_legacy_stack_state,
  unsupported_opcode,
};

struct LegacyWorldPathScriptState {
  std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>
      role_label_payloads;
  compat::u16 camera_target_x{};
  compat::u16 camera_target_y{};
};

enum class LegacyWorldPathRoleFrameAction : compat::u8 {
  skip,
  run_party_path,
  mark_surface,
  update_action_then_mark_surface,
  run_path_script,
};

// The per-role branch tree at sub_405430+0x1A..+0xB6. The selected action is
// executed by the caller so party movement can retain its separate owner.
[[nodiscard]] LegacyWorldPathRoleFrameAction
select_legacy_world_path_role_frame_action(
    const LegacyWorldRoleRecord &role, compat::u32 role_index,
    compat::u32 controlled_role_index) noexcept;

struct LegacyWorldPathScriptRuntime {
  LegacyWorldStoryVmState *shared_script_state{};
  LegacyRoleSpatialIndex *spatial_index{};
  input_time_rng::LegacyCrtRng *crt_rng{};
  input_time_rng::LegacySecondaryRng *secondary_rng{};
  compat::u32 controlled_role_index{};
};

class LegacyWorldPathScriptPorts {
public:
  virtual ~LegacyWorldPathScriptPorts() = default;

  [[nodiscard]] virtual compat::u32
  update_action(asset_runtime::LegacyActionRecord &action) = 0;
  virtual void play_positional_sample(compat::u16 sound_id,
                                      compat::i32 world_x,
                                      compat::i32 world_y) noexcept = 0;
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
  compat::u32 action_updates{};
  compat::u32 action_update_failures{};
  compat::u32 conditional_transfers{};
  compat::u32 invalid_variable_indices{};
  compat::u16 last_opcode{};
};

// sub_405500 PATH interpreter. Each legal opcode is restored as an explicit
// branch; invalid opcodes stop without advancing the legacy cursor.
[[nodiscard]] LegacyWorldPathScriptResult run_legacy_world_path_script(
    compat::u32 role_index, std::span<const compat::u8> path_database,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context,
    compat::u32 map_height, std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool, LegacyWorldPathScriptState &state,
    const LegacyWorldPathScriptRuntime &runtime,
    LegacyWorldPathScriptPorts &ports);

[[nodiscard]] std::span<const compat::u8> resolve_legacy_world_path_label(
    const LegacyWorldPathScriptState &state, compat::u32 token) noexcept;

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
    LegacyWorldPathNodePool &node_pool, LegacyWorldPathScriptState &state,
    const LegacyWorldPathScriptRuntime &runtime,
    LegacyWorldPathScriptPorts &ports);

} // namespace openswd3::world_map
