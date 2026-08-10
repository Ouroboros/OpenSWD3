#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldFlaggedRoleBit = 0x20000000U;
inline constexpr compat::u32 kLegacyWorldDrawableRoleBits = 0x00008400U;
inline constexpr compat::u32 kLegacyWorldDrawableRoleValue = 0x00008000U;

struct LegacyWorldRenderCamera {
  compat::i32 left{};
  compat::i32 top{};
};

enum class LegacyWorldFlaggedRoleDrawStatus : compat::u8 {
  completed,
  frame_load_failed,
};

struct LegacyWorldFlaggedRoleDrawResult {
  LegacyWorldFlaggedRoleDrawStatus status{
      LegacyWorldFlaggedRoleDrawStatus::completed};
  bool drawable{};
  bool horizontally_visible{};
  bool frame_requested{};
  bool drawn{};
  compat::u16 resource_id{};
  compat::u16 frame_index{};
  compat::i32 destination_x{};
  compat::i32 destination_y{};
  compat::u32 flags{};
  compat::i32 opacity_step{};
  compat::u32 blit_failure_count{};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
};

// 0x00413F00: draw one bit-29 spatial role with the fixed translucent mode.
// This path does not update the embedded action record before resolving its
// current TSW frame.
[[nodiscard]] LegacyWorldFlaggedRoleDrawResult
draw_legacy_world_flagged_role(const LegacyWorldRoleRecord &role,
                               LegacyWorldRenderCamera camera,
                               asset_runtime::LegacyActionDrawPorts &ports);

enum class LegacyWorldFlaggedRolesStatus : compat::u8 {
  completed,
  invalid_spatial_index,
  invalid_role_link,
  frame_load_failed,
};

struct LegacyWorldFlaggedRolesResult {
  LegacyWorldFlaggedRolesStatus status{
      LegacyWorldFlaggedRolesStatus::invalid_spatial_index};
  compat::u32 visited_rows{};
  compat::u32 visited_roles{};
  compat::u32 flagged_roles{};
  compat::u32 frame_requests{};
  compat::u32 draw_count{};
  compat::u32 blit_failure_count{};
};

// 0x00413EA0: scan group 0 from trunc(camera_y / 16) - 5 for at most forty
// rows, preserving each row's linked-list order and selecting bit-29 roles.
[[nodiscard]] LegacyWorldFlaggedRolesResult
draw_legacy_world_flagged_roles(const LegacyRoleSpatialIndex &spatial_index,
                                std::span<const LegacyWorldRoleRecord> roles,
                                LegacyWorldRenderCamera camera,
                                asset_runtime::LegacyActionDrawPorts &ports);

} // namespace openswd3::world_map
