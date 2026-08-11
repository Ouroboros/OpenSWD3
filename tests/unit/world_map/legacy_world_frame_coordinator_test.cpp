#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_frame_coordinator.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::world_map::initialize_legacy_world_player_position_history;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldFrameCoordinatorResult;
using openswd3::world_map::LegacyWorldFrameCoordinatorState;
using openswd3::world_map::LegacyWorldFrameCoordinatorStatus;
using openswd3::world_map::LegacyWorldFramePorts;
using openswd3::world_map::LegacyWorldFrameRuntimePorts;
using openswd3::world_map::LegacyWorldFrameRuntimeStatus;
using openswd3::world_map::LegacyWorldFrameStage;
using openswd3::world_map::LegacyWorldHeadSignActionsStatus;
using openswd3::world_map::LegacyWorldOuterFramePorts;
using openswd3::world_map::LegacyWorldOuterFrameStage;
using openswd3::world_map::LegacyWorldOuterFrameStageRequest;
using openswd3::world_map::LegacyWorldRoleBlitRequest;
using openswd3::world_map::LegacyWorldRoleFrame;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleRenderPorts;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldSelectionScrollStatus;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;
using openswd3::world_map::run_legacy_world_frame;

constexpr u32 kOuterEventBase = 0x100U;
constexpr u32 kFrameEventBase = 0x200U;
constexpr u32 kAudioEvent = 0x300U;
constexpr u32 kPresentEvent = 0x301U;
constexpr u32 kHeadSignEventBase = 0x400U;

[[nodiscard]] constexpr u32
outer_event(const LegacyWorldOuterFrameStage stage) noexcept {
  return kOuterEventBase + static_cast<u32>(stage);
}

[[nodiscard]] constexpr u32
frame_event(const LegacyWorldFrameStage stage) noexcept {
  return kFrameEventBase + static_cast<u32>(stage);
}

class RecordingOuterPorts final : public LegacyWorldOuterFramePorts {
public:
  explicit RecordingOuterPorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  [[nodiscard]] bool execute_stage(
      const LegacyWorldOuterFrameStageRequest &request) noexcept override {
    requests.push_back(request);
    events_.push_back(outer_event(request.stage));
    return !fail_stage || request.stage != failed_stage;
  }

  [[nodiscard]] bool
  complete_role_path(const u32 role_index) noexcept override {
    ++path_completion_calls;
    last_completed_role_index = role_index;
    return path_completion_succeeds;
  }

  void maintain_audio() noexcept override { events_.push_back(kAudioEvent); }

  void request_world_presentation() noexcept override {
    events_.push_back(kPresentEvent);
  }

  bool fail_stage{};
  LegacyWorldOuterFrameStage failed_stage{
      LegacyWorldOuterFrameStage::fixed_ui_004308c0};
  bool path_completion_succeeds{true};
  u32 path_completion_calls{};
  u32 last_completed_role_index{0xFFFFFFFFU};
  std::vector<LegacyWorldOuterFrameStageRequest> requests;

private:
  std::vector<u32> &events_;
};

class RecordingFramePorts final : public LegacyWorldFramePorts {
public:
  explicit RecordingFramePorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  [[nodiscard]] bool query_service(const u32) noexcept override {
    return false;
  }

  [[nodiscard]] bool query_control(const u32) noexcept override {
    return false;
  }

  [[nodiscard]] bool
  execute_stage(const LegacyWorldFrameStage stage) noexcept override {
    events_.push_back(frame_event(stage));
    return !fail_stage || stage != failed_stage;
  }

  void draw_decorated_number(const i32, const i32, const u32,
                             const u32) noexcept override {}

  bool fail_stage{};
  LegacyWorldFrameStage failed_stage{
      LegacyWorldFrameStage::pre_background_records_004151f0};

private:
  std::vector<u32> &events_;
};

class EmptyActionPorts final : public LegacyActionDrawPorts {
public:
  explicit EmptyActionPorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    events_.push_back(kHeadSignEventBase + record.base_variant);
    if (record.base_variant == failed_variant) {
      return LegacyActionUpdateStatus::stream_load_failed;
    }
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16, const u16,
                                      LegacyFramePiece &) override {
    return false;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32, const i32, const u32,
                   const i32) noexcept override {
    return LegacyBlitExecutionStatus::completed;
  }

  u32 failed_variant{0xFFFFFFFFU};

private:
  std::vector<u32> &events_;
};

class EmptyRolePorts final : public LegacyWorldRoleRenderPorts {
public:
  [[nodiscard]] bool query_service(const u32) noexcept override {
    return false;
  }

  void play_positional_sample(const u16, const i32,
                              const i32) noexcept override {}

  [[nodiscard]] bool load_frame(const u16, const u16,
                                LegacyWorldRoleFrame &) override {
    return false;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame(const LegacyWorldRoleFrame &, const LegacyWorldRoleBlitRequest &,
             LegacyRleRowJitterState &) noexcept override {
    return LegacyBlitExecutionStatus::completed;
  }

  [[nodiscard]] const LegacyActionRecord *
  resolve_overlay_action(const u32) noexcept override {
    return nullptr;
  }

  void emit_role_particles(const i32, const i32, const u16) noexcept override {}

  [[nodiscard]] std::span<const u8>
  resolve_label_bytes(const u32) noexcept override {
    return {};
  }

  [[nodiscard]] u16 label_color(const u32) noexcept override { return 0U; }

  void draw_label(std::span<const u8>, const i32, const i32, const u16,
                  const u32) noexcept override {}
};

class EmptyAudioPorts final : public LegacyWorldSpatialAudioPorts {
public:
  void play_sample(const u16, const i32, const i32,
                   const i32) noexcept override {}
  void stop_sample(const u16) noexcept override {}
  void set_sample_volume(const u16, const i32) noexcept override {}
  void set_sample_pan(const u16, const i32) noexcept override {}
};

struct Fixture {
  std::vector<u32> events;
  u32 map_width{45U};
  u32 map_height{40U};
  std::vector<u16> tile_indices =
      std::vector<u16>(static_cast<std::size_t>(map_width) * map_height, 0U);
  std::vector<u8> cell_flags = std::vector<u8>(
      static_cast<std::size_t>(map_width) * map_height * 4U, 0U);
  std::vector<u8> tile_bytes = std::vector<u8>(0x200U, 0x11U);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  LegacyRoleSpatialIndex spatial;
  std::array<i16, 2U> distances{};
  std::array<i16, 2U> vertical_offsets{};
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster{framebuffer.geometry()};
  LegacyRleRowJitterState jitter;
  LegacyWorldCameraRect camera{10U, 20U, 650U, 500U};
  LegacyWorldFrameCoordinatorState state;
  RecordingFramePorts frame_ports{events};
  RecordingOuterPorts outer_ports{events};
  EmptyActionPorts action_ports{events};
  EmptyRolePorts role_ports;
  EmptyAudioPorts audio_ports;

  Fixture() {
    roles[1].world_x = 124U;
    roles[1].world_y = 196U;
    roles[1].map_cell_pointer_32 = 13U * map_width + 7U;
    roles[1].guid = 1U;
    spatial.map_height = map_height;
    const std::size_t spatial_rows =
        static_cast<std::size_t>(map_height) + 2U * kLegacySpatialRowPadding;
    for (auto &rows : spatial.row_heads) {
      rows.assign(spatial_rows, kLegacySpatialNoRole);
    }
    spatial.row_heads[0][13U + kLegacySpatialRowPadding] = 1U;
    state.map_id = 24U;
    state.player_role_index = 1U;
    state.party_role_count = 2U;
    state.map_marker_state = 1U;
    state.movement = {
        .camera_x_transition = 1,
        .player_x_transition = 0,
        .camera_y_transition = -1,
        .player_y_transition = 0,
        .movement_step = 4U,
    };
    state.selection_scroll = {
        .frames_remaining = 2,
        .frame_interval = 7,
    };
    state.camera_pan = {
        .remaining_x = 6,
        .remaining_y = -3,
        .step_x = 2,
        .step_y = -1,
    };
    state.tile_animation = {
        .cycle_counter = 0,
        .cycle_interval = 1,
        .frame_count = 3U,
        .frame_index = 0U,
        .frame_direction = 1,
        .tile_layer_stride = 0x200U,
        .tile_layer_offset = 0U,
    };
    state.frame_runtime.spatial_audio = LegacyWorldSpatialAudioState{
        .controlled_role_index = 1U,
        .distance_by_role = distances,
        .vertical_offset_by_role = vertical_offsets,
    };
    initialize_legacy_world_player_position_history(state.player_post_frame,
                                                    roles[1]);
  }

  [[nodiscard]] LegacyWorldBackgroundSource background() const noexcept {
    return {
        .map_width = map_width,
        .map_height = map_height,
        .tile_indices = tile_indices,
        .cell_flags = cell_flags,
        .tile_bytes = tile_bytes,
        .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
    };
  }

  [[nodiscard]] LegacyWorldFrameCoordinatorResult
  run(const std::span<const i16> selection) noexcept {
    return run_legacy_world_frame(framebuffer, raster, background(), spatial,
                                  roles,
                                  LegacyWorldRoleSurfaceContext{
                                      .map_width = map_width,
                                      .selected_guid = roles[1].guid,
                                      .surface_grid = cell_flags,
                                  },
                                  selection, camera, state, jitter,
                                  LegacyWorldFrameRuntimePorts{
                                      .remaining_stages = frame_ports,
                                      .flagged_roles = action_ports,
                                      .world_roles = role_ports,
                                      .spatial_audio = audio_ports,
                                  },
                                  outer_ports);
  }
};

[[nodiscard]] std::vector<u32> expected_normal_events() {
  using Outer = LegacyWorldOuterFrameStage;
  using Inner = LegacyWorldFrameStage;
  return {
      kHeadSignEventBase + 3U,
      kHeadSignEventBase + 2U,
      kHeadSignEventBase + 1U,
      kHeadSignEventBase + 0U,
      kAudioEvent,
      frame_event(Inner::pre_background_records_004151f0),
      frame_event(Inner::primary_picture_actions_004147e0),
      frame_event(Inner::moving_action_sprites_00414b60),
      frame_event(Inner::ani_drift_004161c0),
      frame_event(Inner::ani_streak_00416590),
      frame_event(Inner::ani_spark_004167b0),
      frame_event(Inner::ani_directional_00415b70),
      frame_event(Inner::ani_row_copy_004163c0),
      frame_event(Inner::framebuffer_deformation_00416cc0),
      frame_event(Inner::ani_follower_00416b30),
      frame_event(Inner::secondary_picture_actions_004147e0),
      frame_event(Inner::packed_row_effects_00414e50),
      frame_event(Inner::timed_ui_update_0042ed40),
      frame_event(Inner::role_head_sprites_00414ce0),
      frame_event(Inner::world_indicator_004149b0),
      frame_event(Inner::frame_color_update_004146f0),
      frame_event(Inner::timed_messages_004153d0),
      outer_event(Outer::fixed_ui_004308c0),
      outer_event(Outer::optional_map_marker_00413fe0),
      kAudioEvent,
      kPresentEvent,
  };
}

void test_complete_frame_exact_order_and_state(openswd3::test::Context &test) {
  Fixture fixture;
  const std::array<i16, 3U> selection{3, -2, -12337};
  const auto result = fixture.run(selection);

  test.expect_true(
      result.status == LegacyWorldFrameCoordinatorStatus::completed &&
          result.frame.status == LegacyWorldFrameRuntimeStatus::completed &&
          result.selection_scroll ==
              LegacyWorldSelectionScrollStatus::completed &&
          result.outer_stage_call_count == 2U &&
          result.map_role_paths.status ==
              openswd3::world_map::LegacyWorldMapRolePathStatus::completed &&
          result.map_role_paths.slots_scanned == 72U &&
          result.map_role_paths.active_slots == 0U &&
          result.party_role_actions.status ==
              openswd3::world_map::LegacyWorldPartyRoleActionsStatus::
                  completed &&
          result.party_role_actions.slots_scanned == 1U &&
          result.party_role_actions.populated_slots == 0U &&
          result.audio_service_count == 2U && result.player_motion_applied &&
          result.camera_pan_advanced && result.presentation_requested &&
          result.post_present_player_aligned &&
          result.movement_transitions_cleared &&
          result.tile_animation_advanced && result.viewport_restored,
      "the recovered outer frame reaches every stateful assembly slot");
  test.expect_equal(fixture.events, expected_normal_events(),
                    "outer, composition, audio and presentation order");
  test.expect_true(
      fixture.roles[1].world_x == 128U && fixture.roles[1].world_y == 192U &&
          result.composition_camera_left == 19 &&
          result.composition_camera_top == 13 && fixture.camera.left == 16U &&
          fixture.camera.top == 15U && fixture.camera.right == 656U &&
          fixture.camera.bottom == 495U,
      "motion, script pan and temporary selection scroll share one camera");
  test.expect_true(
      fixture.state.camera_pan.remaining_x == 4 &&
          fixture.state.camera_pan.remaining_y == -2 &&
          fixture.state.camera_pan.step_x == 2 &&
          fixture.state.camera_pan.step_y == -1 &&
          fixture.state.selection_scroll.saved_left == 16U &&
          fixture.state.selection_scroll.saved_top == 15U &&
          fixture.state.selection_scroll.frames_remaining == 1 &&
          fixture.state.tile_animation.frame_index == 1U &&
          fixture.state.tile_animation.tile_layer_offset == 0x200U &&
          fixture.state.frame_runtime.frame.camera_left == 16 &&
          fixture.state.frame_runtime.frame.camera_top == 15,
      "camera pan, animation and viewport restoration persist final state");
  test.expect_true(
      result.player_post_frame.status ==
              openswd3::world_map::LegacyWorldPlayerPostFrameStatus::
                  completed &&
          result.player_post_frame.spatially_relocated &&
          result.player_post_frame.old_occupancy_cleared &&
          result.player_post_frame.new_occupancy_marked &&
          result.player_post_frame.history_shifted &&
          result.player_post_frame.map_cell_delta == 0xFFFFFFD4U &&
          fixture.roles[1].map_cell_pointer_32 == 548U &&
          fixture.state.player_post_frame.world_x_history[0] == 128U &&
          fixture.state.player_post_frame.world_y_history[0] == 192U,
      "post-present player bookkeeping owns the moved cell and histories");
  test.expect_true(
      fixture.outer_ports.requests[0].stage ==
              LegacyWorldOuterFrameStage::fixed_ui_004308c0 &&
          fixture.outer_ports.requests[0].argument_0 == 400 &&
          fixture.outer_ports.requests[0].argument_1 == 8 &&
          fixture.outer_ports.requests[0].argument_2 == 0U &&
          fixture.outer_ports.requests[1].stage ==
              LegacyWorldOuterFrameStage::optional_map_marker_00413fe0 &&
          fixture.outer_ports.requests[1].argument_0 == 19 &&
          fixture.outer_ports.requests[1].argument_1 == 13 &&
          fixture.outer_ports.requests[1].argument_2 == 2U,
      "fixed UI and marker retain the exact stack arguments from assembly");
  test.expect_true(fixture.state.movement.camera_x_transition == 0 &&
                       fixture.state.movement.player_x_transition == 0 &&
                       fixture.state.movement.camera_y_transition == 0 &&
                       fixture.state.movement.player_y_transition == 0,
                   "the four movement transitions clear only after "
                   "post-present bookkeeping");
}

void test_marker_requires_exact_one(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.state.map_marker_state = 2U;
  const std::array<i16, 1U> selection{-12337};
  const auto result = fixture.run(selection);

  test.expect_true(
      result.status == LegacyWorldFrameCoordinatorStatus::completed &&
          result.outer_stage_call_count == 1U,
      "noncanonical marker state two does not alias equality with one");
  test.expect_true(
      std::ranges::find(
          fixture.events,
          outer_event(
              LegacyWorldOuterFrameStage::optional_map_marker_00413fe0)) ==
          fixture.events.end(),
      "optional 00413FE0 is skipped unless the state equals one exactly");
}

void test_party_count_and_alignment_gates(openswd3::test::Context &test) {
  for (const u32 party_count : std::array<u32, 2U>{0U, 1U}) {
    Fixture fixture;
    fixture.state.party_role_count = party_count;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status == LegacyWorldFrameCoordinatorStatus::completed &&
            result.outer_stage_call_count == 2U &&
            result.party_role_actions.slots_scanned == 0U,
        "unsigned party count zero or one jumps directly to 0041268C");
  }

  {
    Fixture fixture;
    fixture.roles[1].world_x = 125U;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status == LegacyWorldFrameCoordinatorStatus::completed &&
            !result.post_present_player_aligned &&
            !result.movement_transitions_cleared &&
            result.outer_stage_call_count == 2U &&
            fixture.state.movement.camera_x_transition == 1 &&
            fixture.state.movement.player_x_transition == 0 &&
            fixture.state.movement.camera_y_transition == -1,
        "unaligned player skips action, snapshots and transition clears");
    test.expect_true(!result.player_post_frame.spatially_relocated &&
                         !result.player_post_frame.history_shifted &&
                         !result.player_post_frame.action_validation_requested,
                     "unaligned player skips aligned bookkeeping while the "
                     "unconditional validation gate still executes");
  }
}

void test_checked_failures_stop_at_the_original_slot(
    openswd3::test::Context &test) {
  {
    Fixture fixture;
    fixture.state.player_role_index = 2U;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::invalid_player_index &&
            fixture.events.empty() && !result.player_motion_applied,
        "invalid modern role ownership is isolated before the first stage");
  }

  {
    Fixture fixture;
    fixture.state.party_role_count = 9U;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::party_role_actions_failed &&
            result.party_role_actions.status ==
                openswd3::world_map::LegacyWorldPartyRoleActionsStatus::
                    invalid_party_role_count &&
            result.outer_stage_call_count == 0U &&
            result.audio_service_count == 0U &&
            !result.presentation_requested && fixture.roles[1].world_x == 128U,
        "invalid party-slot ownership stops after player motion and before "
        "00414570");
  }

  {
    Fixture fixture;
    fixture.outer_ports.fail_stage = true;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::outer_stage_failed &&
            result.failed_outer_stage_recorded &&
            result.failed_outer_stage ==
                LegacyWorldOuterFrameStage::fixed_ui_004308c0 &&
            result.outer_stage_call_count == 1U &&
            result.player_motion_applied && result.camera_pan_advanced &&
            result.audio_service_count == 1U &&
            !result.presentation_requested && fixture.roles[1].world_x == 128U,
        "outer stage failure preserves all earlier state and stops in place");
  }

  {
    Fixture fixture;
    const std::array<i16, 1U> missing_y{1};
    const auto result = fixture.run(missing_y);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::invalid_selection_window &&
            result.outer_stage_call_count == 0U &&
            result.audio_service_count == 0U && !result.presentation_requested,
        "invalid selection pair stops after 00414570 and before audio");
  }

  {
    Fixture fixture;
    fixture.frame_ports.fail_stage = true;
    const std::array<i16, 3U> selection{3, -2, -12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::composition_failed &&
            result.frame.status ==
                LegacyWorldFrameRuntimeStatus::delegated_stage_failed &&
            result.audio_service_count == 1U &&
            result.outer_stage_call_count == 0U &&
            !result.presentation_requested && !result.tile_animation_advanced &&
            fixture.camera.left == 19U && fixture.camera.top == 13U,
        "composition failure is visible at 00412930 and cannot fake a frame");
  }
}

void test_head_sign_update_failure_remains_nonfatal(
    openswd3::test::Context &test) {
  Fixture fixture;
  fixture.action_ports.failed_variant = 2U;
  const std::array<i16, 1U> selection{-12337};

  const auto result = fixture.run(selection);
  test.expect_true(
      result.status == LegacyWorldFrameCoordinatorStatus::completed &&
          result.head_sign_actions.status ==
              LegacyWorldHeadSignActionsStatus::
                  completed_with_update_failures &&
          result.head_sign_actions.update_failure_count == 1U &&
          fixture.events == expected_normal_events(),
      "the original HeadSgn diagnostic branch does not stop the frame");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_complete_frame_exact_order_and_state(test);
  test_marker_requires_exact_one(test);
  test_party_count_and_alignment_gates(test);
  test_checked_failures_stop_at_the_original_slot(test);
  test_head_sign_update_failure_remains_nonfatal(test);
  return test.exit_code();
}
