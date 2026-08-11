#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_spark_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_streak_effect.hpp"
#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_timed_messages.hpp"
#include "openswd3/world_map/legacy_moving_actions.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"
#include "openswd3/world_map/legacy_world_frame_composition.hpp"
#include "openswd3/world_map/legacy_world_roles.hpp"

#include <list>
#include <span>

namespace openswd3::world_map {

struct LegacyWorldFrameEffectState {
  LegacyWorldFrameEffectState() noexcept;

  asset_runtime::LegacyAniDriftEffect drift;
  asset_runtime::LegacyAniStreakEffect streak;
  asset_runtime::LegacyAniSparkEffect spark;
  asset_runtime::LegacyAniDirectionalEffect directional;
  asset_runtime::LegacyAniDirectionalConfiguration directional_configuration{
      .base_variant = 0U,
      .variant_count = 4U,
      .spawn_direction = 0U,
  };
  asset_runtime::LegacyActionRecord directional_action{};
  asset_runtime::LegacyAniRowCopyEffect row_copy;
  asset_runtime::LegacyDeformationList deformation;
  asset_runtime::LegacyAniFollowerState follower;
  asset_runtime::LegacyActionRecord follower_action{};
  std::list<rendering::LegacyPackedRowEffect> packed_rows;
  rendering::LegacyFrameColorTransitionState frame_color;
  std::list<rendering::LegacyTimedMessage> timed_messages;
};

struct LegacyWorldFrameRuntimeState {
  LegacyWorldFrameState frame;
  compat::u32 role_frame_counter{};
  compat::i32 flash_red_offset{};
  compat::i32 flash_green_offset{};
  compat::i32 flash_blue_offset{};
  LegacyWorldSpatialAudioState spatial_audio;
  compat::i32 directional_movement_scale{};
  compat::i32 directional_player_delta_x{};
  compat::i32 directional_player_delta_y{};
};

struct LegacyWorldFrameRuntimePorts {
  // Stages without a concrete owner remain explicit here until their owning
  // module supplies a runtime implementation.
  LegacyWorldFramePorts &remaining_stages;
  LegacyPictureActionLists &picture_actions;
  LegacyMovingActionList &moving_actions;
  LegacyRoleHeadActionList &role_head_actions;
  LegacyWorldFrameEffectState &environment_effects;
  input_time_rng::LegacySecondaryRng &secondary_rng;
  const rendering::LegacyPixelConversionState &pixel_conversion;
  asset_runtime::LegacyAniDriftPorts &ani_drift;
  asset_runtime::LegacyAniDirectionalPorts &ani_directional;
  asset_runtime::LegacyAniFollowerPorts &ani_follower;
  rendering::LegacyTimedMessageRuntimePorts &timed_message_runtime;
  asset_runtime::LegacyActionDrawPorts &flagged_roles;
  LegacyWorldRoleRenderPorts &world_roles;
  LegacyWorldSpatialAudioPorts &spatial_audio;
};

enum class LegacyWorldFrameRuntimeStatus : compat::u8 {
  completed,
  composition_failed,
  delegated_stage_failed,
  environment_effect_failed,
  frame_color_failed,
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
  LegacyRoleHeadActionResult role_head_actions;
  asset_runtime::LegacyAniDriftResult ani_drift;
  asset_runtime::LegacyAniStreakResult ani_streak;
  asset_runtime::LegacyAniSparkResult ani_spark;
  asset_runtime::LegacyAniDirectionalResult ani_directional;
  asset_runtime::LegacyAniRowCopyResult ani_row_copy;
  asset_runtime::LegacyDeformationListUpdateResult framebuffer_deformation;
  asset_runtime::LegacyAniFollowerResult ani_follower;
  rendering::LegacyPackedRowEffectResult packed_rows;
  rendering::LegacyFrameColorTransitionResult frame_color;
  rendering::LegacyTimedMessageResult timed_messages;
  compat::u32 delegated_stage_count{};
  bool flagged_stage_executed{};
  bool world_roles_stage_executed{};
  bool primary_picture_actions_executed{};
  bool moving_actions_executed{};
  bool secondary_picture_actions_executed{};
  bool role_head_actions_executed{};
  bool ani_drift_executed{};
  bool ani_streak_executed{};
  bool ani_spark_executed{};
  bool ani_directional_executed{};
  bool ani_row_copy_executed{};
  bool framebuffer_deformation_executed{};
  bool ani_follower_executed{};
  bool packed_rows_executed{};
  bool frame_color_executed{};
  bool timed_messages_executed{};
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
