#include "openswd3/world_map/legacy_world_frame_coordinator.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] bool
execute_outer_stage(LegacyWorldOuterFramePorts &ports,
                    LegacyWorldFrameCoordinatorResult &result,
                    const LegacyWorldOuterFrameStageRequest request) noexcept {
  ++result.outer_stage_call_count;
  if (ports.execute_stage(request)) {
    return true;
  }
  result.status = LegacyWorldFrameCoordinatorStatus::outer_stage_failed;
  result.failed_outer_stage_recorded = true;
  result.failed_outer_stage = request.stage;
  return false;
}

void sync_frame_camera(LegacyWorldFrameCoordinatorState &state,
                       const LegacyWorldCameraRect &camera) noexcept {
  state.frame_runtime.frame.camera_left =
      std::bit_cast<compat::i32>(camera.left);
  state.frame_runtime.frame.camera_top = std::bit_cast<compat::i32>(camera.top);
}

} // namespace

LegacyWorldFrameCoordinatorResult
run_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                       rendering::LegacyRasterGeometryState &raster,
                       const LegacyWorldBackgroundSource &background_source,
                       LegacyRoleSpatialIndex &spatial_index,
                       const std::span<LegacyWorldRoleRecord> roles,
                       const LegacyWorldRoleSurfaceContext role_surface,
                       const std::span<const compat::i16> selection_words,
                       LegacyWorldCameraRect &camera,
                       LegacyWorldFrameCoordinatorState &state,
                       rendering::LegacyRleRowJitterState &jitter,
                       const LegacyWorldFrameRuntimePorts frame_ports,
                       LegacyWorldOuterFramePorts &outer_ports) noexcept {
  LegacyWorldFrameCoordinatorResult result;
  if (state.player_role_index >= roles.size()) {
    return result;
  }

  result.head_sign_actions = advance_legacy_world_head_sign_actions(
      state.head_sign_actions, frame_ports.flagged_roles);

  advance_legacy_world_player_and_camera(roles[state.player_role_index], camera,
                                         state.movement);
  result.player_motion_applied = true;

  result.map_role_paths = advance_legacy_world_map_role_paths(
      roles, spatial_index, role_surface, state.player_role_index,
      state.frame_runtime.frame.runtime_flags, state.movement, camera,
      state.map_role_paths, frame_ports.flagged_roles, outer_ports);
  state.frame_runtime.frame.talk_target =
      state.map_role_paths.talk_context.source_guid;
  if (result.map_role_paths.status !=
      LegacyWorldMapRolePathStatus::completed) {
    result.status = LegacyWorldFrameCoordinatorStatus::map_role_paths_failed;
    return result;
  }
  result.party_role_actions = advance_legacy_world_party_role_actions(
      roles, spatial_index, role_surface, state.party_role_count,
      state.party_object_slots, frame_ports.flagged_roles);
  if (result.party_role_actions.status !=
      LegacyWorldPartyRoleActionsStatus::completed) {
    result.status =
        LegacyWorldFrameCoordinatorStatus::party_role_actions_failed;
    return result;
  }
  if (!execute_outer_stage(outer_ports, result,
                           {LegacyWorldOuterFrameStage::precompose_00414570})) {
    return result;
  }

  result.selection_scroll = advance_legacy_world_selection_scroll(
      selection_words, state.map_id, camera, state.selection_scroll);
  if (result.selection_scroll ==
      LegacyWorldSelectionScrollStatus::invalid_selection_window) {
    result.status = LegacyWorldFrameCoordinatorStatus::invalid_selection_window;
    return result;
  }

  outer_ports.maintain_audio();
  ++result.audio_service_count;

  sync_frame_camera(state, camera);
  result.composition_camera_left = state.frame_runtime.frame.camera_left;
  result.composition_camera_top = state.frame_runtime.frame.camera_top;
  LegacyWorldBackgroundSource frame_background = background_source;
  frame_background.tile_layer_offset = state.tile_animation.tile_layer_offset;
  result.frame = compose_legacy_world_runtime_frame(
      framebuffer, raster, frame_background, spatial_index, roles,
      state.frame_runtime, jitter, frame_ports);
  if (result.frame.status != LegacyWorldFrameRuntimeStatus::completed) {
    result.status = LegacyWorldFrameCoordinatorStatus::composition_failed;
    return result;
  }

  if (!execute_outer_stage(
          outer_ports, result,
          {
              .stage = LegacyWorldOuterFrameStage::fixed_ui_004308c0,
              .argument_0 = 400,
              .argument_1 = 8,
              .argument_2 = 0U,
          })) {
    return result;
  }
  if (state.map_marker_state == 1U &&
      !execute_outer_stage(
          outer_ports, result,
          {
              .stage = LegacyWorldOuterFrameStage::optional_map_marker_00413fe0,
              .argument_0 = std::bit_cast<compat::i32>(camera.left),
              .argument_1 = std::bit_cast<compat::i32>(camera.top),
              .argument_2 = 2U,
          })) {
    return result;
  }

  outer_ports.maintain_audio();
  ++result.audio_service_count;
  outer_ports.request_world_presentation();
  result.presentation_requested = true;

  result.player_post_frame = advance_legacy_world_player_post_frame(
      roles[state.player_role_index], roles, spatial_index, state.movement,
      state.player_post_frame, role_surface, frame_ports.flagged_roles);
  result.post_present_player_aligned = result.player_post_frame.aligned;
  result.movement_transitions_cleared =
      result.player_post_frame.transitions_cleared;
  if (result.player_post_frame.status !=
      LegacyWorldPlayerPostFrameStatus::completed) {
    result.status = LegacyWorldFrameCoordinatorStatus::player_post_frame_failed;
    return result;
  }

  advance_legacy_world_tile_layer_animation(state.tile_animation);
  result.tile_animation_advanced = true;

  const bool should_restore =
      !selection_words.empty() &&
      std::bit_cast<compat::u16>(selection_words.front()) !=
          kLegacyWorldSelectionSentinel &&
      state.map_id != 0x16U;
  restore_legacy_world_viewport_after_selection_scroll(
      camera, LegacyWorldViewportRestoreState{
                  .first_selection_word =
                      selection_words.empty()
                          ? kLegacyWorldSelectionSentinel
                          : std::bit_cast<compat::u16>(selection_words.front()),
                  .map_id = state.map_id,
                  .saved_left = state.selection_scroll.saved_left,
                  .saved_top = state.selection_scroll.saved_top,
              });
  result.viewport_restored = should_restore;
  sync_frame_camera(state, camera);
  result.status = LegacyWorldFrameCoordinatorStatus::completed;
  return result;
}

} // namespace openswd3::world_map
