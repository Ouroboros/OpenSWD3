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

void clear_movement_transitions(
    LegacyWorldMovementRuntimeState &movement) noexcept {
  movement.camera_x_transition = 0;
  movement.player_x_transition = 0;
  movement.camera_y_transition = 0;
  movement.player_y_transition = 0;
}

} // namespace

LegacyWorldFrameCoordinatorResult
run_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                       rendering::LegacyRasterGeometryState &raster,
                       const LegacyWorldBackgroundSource &background_source,
                       const LegacyRoleSpatialIndex &spatial_index,
                       const std::span<LegacyWorldRoleRecord> roles,
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

  if (!execute_outer_stage(
          outer_ports, result,
          {LegacyWorldOuterFrameStage::map_role_actions_004121a1})) {
    return result;
  }
  if (state.company_role_count > 1U &&
      !execute_outer_stage(
          outer_ports, result,
          {LegacyWorldOuterFrameStage::company_role_actions_004124ef})) {
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

  const LegacyWorldRoleRecord &player = roles[state.player_role_index];
  result.post_present_player_aligned =
      ((player.world_x | player.world_y) & 0x0FU) == 0U;
  if (result.post_present_player_aligned) {
    if (!execute_outer_stage(outer_ports, result,
                             {LegacyWorldOuterFrameStage::
                                  post_present_player_action_0041272e})) {
      return result;
    }

    clear_movement_transitions(state.movement);
    result.movement_transitions_cleared = true;

    if (!execute_outer_stage(outer_ports, result,
                             {LegacyWorldOuterFrameStage::
                                  post_present_player_snapshot_004127a0})) {
      return result;
    }
  }
  if (!execute_outer_stage(outer_ports, result,
                           {LegacyWorldOuterFrameStage::
                                post_present_player_validation_0041283c})) {
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
