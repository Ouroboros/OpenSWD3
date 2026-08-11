#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_frame_runtime.hpp"
#include "openswd3/world_map/legacy_world_frame_tail.hpp"
#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_player_post_frame.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"

#include <span>

namespace openswd3::world_map {

enum class LegacyWorldOuterFrameStage : compat::u8 {
  map_role_actions_004121a1,
  company_role_actions_004124ef,
  precompose_00414570,
  fixed_ui_004308c0,
  optional_map_marker_00413fe0,
};

struct LegacyWorldOuterFrameStageRequest {
  LegacyWorldOuterFrameStage stage{
      LegacyWorldOuterFrameStage::map_role_actions_004121a1};
  compat::i32 argument_0{};
  compat::i32 argument_1{};
  compat::u32 argument_2{};
};

class LegacyWorldOuterFramePorts {
public:
  virtual ~LegacyWorldOuterFramePorts() = default;

  [[nodiscard]] virtual bool
  execute_stage(const LegacyWorldOuterFrameStageRequest &request) noexcept = 0;
  virtual void maintain_audio() noexcept = 0;
  virtual void request_world_presentation() noexcept = 0;
};

struct LegacyWorldFrameCoordinatorState {
  compat::u32 map_id{};
  compat::u32 player_role_index{};
  compat::u32 company_role_count{};
  compat::u32 map_marker_state{};
  LegacyWorldMovementRuntimeState movement;
  LegacyWorldSelectionScrollState selection_scroll;
  LegacyWorldTileLayerAnimationState tile_animation;
  LegacyWorldFrameRuntimeState frame_runtime;
  LegacyWorldHeadSignActionsState head_sign_actions;
  LegacyWorldPlayerPostFrameState player_post_frame;
};

enum class LegacyWorldFrameCoordinatorStatus : compat::u8 {
  completed,
  invalid_player_index,
  invalid_selection_window,
  outer_stage_failed,
  composition_failed,
  player_post_frame_failed,
};

struct LegacyWorldFrameCoordinatorResult {
  LegacyWorldFrameCoordinatorStatus status{
      LegacyWorldFrameCoordinatorStatus::invalid_player_index};
  LegacyWorldSelectionScrollStatus selection_scroll{
      LegacyWorldSelectionScrollStatus::invalid_selection_window};
  LegacyWorldFrameRuntimeResult frame;
  LegacyWorldHeadSignActionsResult head_sign_actions;
  LegacyWorldPlayerPostFrameResult player_post_frame;
  compat::u32 outer_stage_call_count{};
  compat::u32 audio_service_count{};
  compat::i32 composition_camera_left{};
  compat::i32 composition_camera_top{};
  bool player_motion_applied{};
  bool presentation_requested{};
  bool post_present_player_aligned{};
  bool movement_transitions_cleared{};
  bool tile_animation_advanced{};
  bool viewport_restored{};
  bool failed_outer_stage_recorded{};
  LegacyWorldOuterFrameStage failed_outer_stage{
      LegacyWorldOuterFrameStage::map_role_actions_004121a1};
};

// Ordinary-world outer frame at 0x004120B0. The framebuffer is the modern
// equivalent of the source-surface bind at 0x004126A2..0x004126B3. All calls
// on outer_ports stay at their original assembly slots; recovered stateful
// stages execute directly against the same role/camera/framebuffer objects.
[[nodiscard]] LegacyWorldFrameCoordinatorResult
run_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                       rendering::LegacyRasterGeometryState &raster,
                       const LegacyWorldBackgroundSource &background_source,
                       LegacyRoleSpatialIndex &spatial_index,
                       std::span<LegacyWorldRoleRecord> roles,
                       LegacyWorldRoleSurfaceContext role_surface,
                       std::span<const compat::i16> selection_words,
                       LegacyWorldCameraRect &camera,
                       LegacyWorldFrameCoordinatorState &state,
                       rendering::LegacyRleRowJitterState &jitter,
                       LegacyWorldFrameRuntimePorts frame_ports,
                       LegacyWorldOuterFramePorts &outer_ports) noexcept;

} // namespace openswd3::world_map
