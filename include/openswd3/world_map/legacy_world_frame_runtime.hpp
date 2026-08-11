#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_moving_actions.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_world_frame_composition.hpp"
#include "openswd3/world_map/legacy_world_roles.hpp"

#include <span>

namespace openswd3::world_map {

struct LegacyWorldFrameRuntimeState {
  LegacyWorldFrameState frame;
  compat::u32 role_frame_counter{};
  compat::i32 flash_red_offset{};
  compat::i32 flash_green_offset{};
  compat::i32 flash_blue_offset{};
  LegacyWorldSpatialAudioState spatial_audio;
};

struct LegacyWorldFrameRuntimePorts {
  // Stages without a concrete owner remain explicit here until their owning
  // module supplies a runtime implementation.
  LegacyWorldFramePorts &remaining_stages;
  LegacyPictureActionLists &picture_actions;
  LegacyMovingActionList &moving_actions;
  asset_runtime::LegacyActionDrawPorts &flagged_roles;
  LegacyWorldRoleRenderPorts &world_roles;
  LegacyWorldSpatialAudioPorts &spatial_audio;
};

enum class LegacyWorldFrameRuntimeStatus : compat::u8 {
  completed,
  composition_failed,
  delegated_stage_failed,
  flagged_roles_failed,
  world_roles_failed,
  stage_exception,
};

struct LegacyWorldFrameRuntimeResult {
  LegacyWorldFrameRuntimeStatus status{
      LegacyWorldFrameRuntimeStatus::composition_failed};
  LegacyWorldFrameCompositionResult composition;
  LegacyWorldFlaggedRolesResult flagged_roles;
  LegacyWorldRolesResult world_roles;
  LegacyPictureActionResult primary_picture_actions;
  LegacyMovingActionResult moving_actions;
  LegacyPictureActionResult secondary_picture_actions;
  compat::u32 delegated_stage_count{};
  bool flagged_stage_executed{};
  bool world_roles_stage_executed{};
  bool primary_picture_actions_executed{};
  bool moving_actions_executed{};
  bool secondary_picture_actions_executed{};
  bool failed_stage_recorded{};
  LegacyWorldFrameStage failed_stage{
      LegacyWorldFrameStage::ani_activity_004154a0};
};

// Concrete 0x00412930 vertical slice. The frame coordinator retains the exact
// assembly order while recovered stages execute against the same mutable
// frame state, role array, raster clip, framebuffer and row-jitter state.
[[nodiscard]] LegacyWorldFrameRuntimeResult compose_legacy_world_runtime_frame(
    rendering::LegacyFramebuffer &framebuffer,
    rendering::LegacyRasterGeometryState &raster,
    const LegacyWorldBackgroundSource &background_source,
    const LegacyRoleSpatialIndex &spatial_index,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldFrameRuntimeState &state,
    rendering::LegacyRleRowJitterState &jitter,
    LegacyWorldFrameRuntimePorts ports) noexcept;

} // namespace openswd3::world_map
