#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_countdown.hpp"
#include "openswd3/world_map/legacy_world_camera_pan.hpp"
#include "openswd3/world_map/legacy_world_debug_overlay.hpp"
#include "openswd3/world_map/legacy_world_frame_runtime.hpp"
#include "openswd3/world_map/legacy_world_frame_tail.hpp"
#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_party_role_actions.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_player_post_frame.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"

#include <array>
#include <span>

namespace openswd3::world_map {

class LegacyWorldOuterFramePorts : public LegacyWorldMapRolePathPorts,
                                   public LegacyWorldDebugOverlayPorts {
public:
  virtual ~LegacyWorldOuterFramePorts() = default;

  virtual void maintain_audio() noexcept = 0;
  virtual void request_world_presentation() noexcept = 0;
};

struct LegacyWorldFrameCoordinatorState {
  compat::u32 map_id{};
  compat::u32 player_role_index{};
  compat::u32 party_role_count{1U};
  compat::u32 developer_tools_enabled{};
  std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount>
      party_object_slots;
  LegacyWorldMovementRuntimeState movement;
  LegacyWorldCameraPanState camera_pan;
  LegacyWorldDebugOverlayState debug_overlay;
  rendering::LegacyCountdownState countdown;
  asset_runtime::LegacyActionRecord countdown_action{};
  LegacyWorldSelectionScrollState selection_scroll;
  LegacyWorldTileLayerAnimationState tile_animation;
  LegacyWorldFrameRuntimeState frame_runtime;
  LegacyWorldHeadSignActionsState head_sign_actions;
  LegacyWorldMapRolePathState map_role_paths;
  LegacyWorldPlayerPostFrameState player_post_frame;
};

enum class LegacyWorldFrameCoordinatorStatus : compat::u8 {
  completed,
  invalid_player_index,
  invalid_selection_window,
  map_role_paths_failed,
  party_role_actions_failed,
  countdown_failed,
  debug_overlay_failed,
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
  LegacyWorldMapRolePathResult map_role_paths;
  LegacyWorldPartyRoleActionsResult party_role_actions;
  rendering::LegacyCountdownDisplayResult countdown;
  LegacyWorldDebugOverlayResult debug_overlay;
  LegacyWorldPlayerPostFrameResult player_post_frame;
  compat::u32 audio_service_count{};
  compat::i32 composition_camera_left{};
  compat::i32 composition_camera_top{};
  bool player_motion_applied{};
  bool camera_pan_advanced{};
  bool countdown_stage_executed{};
  bool debug_overlay_executed{};
  bool presentation_requested{};
  bool post_present_player_aligned{};
  bool movement_transitions_cleared{};
  bool tile_animation_advanced{};
  bool viewport_restored{};
};

// Ordinary-world outer frame at 0x004120B0. The framebuffer is the modern
// equivalent of the source-surface bind at 0x004126A2..0x004126B3. All calls
// on outer_ports stay at their original assembly slots; recovered stateful
// stages execute directly against the same role/camera/framebuffer objects.
[[nodiscard]] LegacyWorldFrameCoordinatorResult
run_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                       rendering::LegacyRasterGeometryState &raster,
                       const LegacyWorldBackgroundSource &background_source,
                       std::span<const LegacyWorldMapEvent> map_events,
                       LegacyRoleSpatialIndex &spatial_index,
                       std::span<LegacyWorldRoleRecord> roles,
                       LegacyWorldRoleSurfaceContext role_surface,
                       std::span<const compat::i16> selection_words,
                       LegacyWorldCameraRect &camera,
                       LegacyWorldFrameCoordinatorState &state,
                       rendering::LegacyRleRowJitterState &jitter,
                       const rendering::LegacyBlitEffectState &effects,
                       LegacyWorldFrameRuntimePorts frame_ports,
                       LegacyWorldOuterFramePorts &outer_ports) noexcept;

} // namespace openswd3::world_map
