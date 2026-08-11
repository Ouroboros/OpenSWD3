#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
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
using openswd3::rendering::LegacyBlitEffectState;
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
using openswd3::world_map::LegacyPictureActionLists;
using openswd3::world_map::LegacyMovingActionList;
using openswd3::world_map::LegacyWorldHeadSignActionsStatus;
using openswd3::world_map::LegacyWorldOuterFramePorts;
using openswd3::world_map::LegacyWorldRoleBlitRequest;
using openswd3::world_map::LegacyWorldRoleFrame;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleRenderPorts;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldSelectionScrollStatus;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;
using openswd3::world_map::run_legacy_world_frame;

constexpr u32 kDebugOverlayEvent = 0x100U;
constexpr u32 kFrameEventBase = 0x200U;
constexpr u32 kAudioEvent = 0x300U;
constexpr u32 kPresentEvent = 0x301U;
constexpr u32 kHeadSignEventBase = 0x400U;
constexpr u32 kCountdownEventBase = 0x500U;
constexpr u32 kCursorEventBase = 0x600U;

[[nodiscard]] constexpr u32
frame_event(const LegacyWorldFrameStage stage) noexcept {
  return kFrameEventBase + static_cast<u32>(stage);
}

class RecordingOuterPorts final : public LegacyWorldOuterFramePorts {
public:
  explicit RecordingOuterPorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  void configure_debug_text(const u16 background_color,
                            const u16 secondary_color) noexcept override {
    ++text_configuration_calls;
    configured_background = background_color;
    configured_secondary = secondary_color;
    events_.push_back(kDebugOverlayEvent);
  }

  [[nodiscard]] openswd3::rendering::LegacyTextDrawResult draw_debug_text(
      const openswd3::rendering::LegacyTextDrawRequest &) noexcept override {
    ++text_draw_calls;
    return {};
  }

  [[nodiscard]] bool query_debug_flag(const u32) noexcept override {
    return false;
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

  bool path_completion_succeeds{true};
  u32 path_completion_calls{};
  u32 last_completed_role_index{0xFFFFFFFFU};
  u32 text_configuration_calls{};
  u32 text_draw_calls{};
  u16 configured_background{};
  u16 configured_secondary{};

private:
  std::vector<u32> &events_;
};

class RecordingFramePorts final
    : public LegacyWorldFramePorts,
      public openswd3::rendering::LegacyTimedMessageRuntimePorts {
public:
  explicit RecordingFramePorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  [[nodiscard]] bool query_service(const u32 service_id) noexcept override {
    return service_id < service_flags.size() && service_flags[service_id];
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

  [[nodiscard]] openswd3::rendering::LegacyTimedMessageResult update_and_draw(
      std::list<openswd3::rendering::LegacyTimedMessage> &messages,
      const u16) noexcept override {
    return {
        .visited_count = static_cast<u32>(messages.size()),
    };
  }

  bool fail_stage{};
  std::array<bool, 128U> service_flags{};
  LegacyWorldFrameStage failed_stage{
      LegacyWorldFrameStage::timed_ui_update_0042ed40};

private:
  std::vector<u32> &events_;
};

class EmptyActionPorts final
    : public LegacyActionDrawPorts,
      public openswd3::asset_runtime::LegacyAniDriftPorts,
      public openswd3::asset_runtime::LegacyAniDirectionalPorts,
      public openswd3::asset_runtime::LegacyAniFollowerPorts {
public:
  explicit EmptyActionPorts(std::vector<u32> &events) noexcept
      : events_(events) {}

  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    if (record.action_id == 0x232CU) {
      events_.push_back(kCountdownEventBase + record.base_variant);
      record.field_4a = 0x232CU;
      record.field_4c = static_cast<u16>(record.base_variant);
      return LegacyActionUpdateStatus::completed;
    }
    if (record.action_id == openswd3::world_map::kLegacyWorldCursorActionId) {
      events_.push_back(kCursorEventBase + record.base_variant);
      record.field_4a = static_cast<u16>(record.action_id);
      record.field_4c = static_cast<u16>(record.base_variant);
      return LegacyActionUpdateStatus::completed;
    }
    events_.push_back(kHeadSignEventBase + record.base_variant);
    if (record.base_variant == failed_variant) {
      return LegacyActionUpdateStatus::stream_load_failed;
    }
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16, const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    if (frame_index == unavailable_frame_index) {
      return false;
    }
    piece = LegacyFramePiece{
        .source =
            openswd3::rendering::LegacyBlitSource{
                .bytes = countdown_pixel,
            },
        .width = 1U,
        .height = 1U,
    };
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32, const i32, const u32,
                   const i32) noexcept override {
    return LegacyBlitExecutionStatus::completed;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32, const i32,
                   const u32) noexcept override {
    return LegacyBlitExecutionStatus::completed;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32, const i32, const u32,
                   const i32, const i32) noexcept override {
    return LegacyBlitExecutionStatus::completed;
  }

  void set_clip_rectangle(const i32, const i32, const i32,
                          const i32) noexcept override {}

  u32 failed_variant{0xFFFFFFFFU};
  u16 unavailable_frame_index{0xFFFFU};
  std::array<u8, 2U> countdown_pixel{0x34U, 0x12U};

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
  LegacyBlitEffectState effects;
  openswd3::world_map::LegacyWorldFrameEffectState frame_effects;
  openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
  openswd3::rendering::LegacyPixelConversionState pixel_conversion;
  LegacyWorldCameraRect camera{10U, 20U, 650U, 500U};
  LegacyWorldFrameCoordinatorState state;
  RecordingFramePorts frame_ports{events};
  RecordingOuterPorts outer_ports{events};
  EmptyActionPorts action_ports{events};
  EmptyRolePorts role_ports;
  EmptyAudioPorts audio_ports;
  u32 special_mode_state{};

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
    state.developer_tools_enabled = 1U;
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
    state.countdown.primary_ticks = 754U * 30U;
    frame_ports.service_flags[0x10U] = true;
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
    return run_legacy_world_frame(framebuffer, raster, background(), {},
                                  spatial, roles,
                                  LegacyWorldRoleSurfaceContext{
                                      .map_width = map_width,
                                      .selected_guid = roles[1].guid,
                                      .surface_grid = cell_flags,
                                  },
                                  selection, camera, state, jitter, effects,
                                  LegacyWorldFrameRuntimePorts{
                                      .remaining_stages = frame_ports,
                                      .indexed_objects = {},
                                      .picture_actions = picture_actions,
                                      .moving_actions = moving_actions,
                                      .role_head_actions = role_head_actions,
                                      .environment_effects = frame_effects,
                                      .secondary_rng = secondary_rng,
                                      .pixel_conversion = pixel_conversion,
                                      .blit_effects = &effects,
                                      .cursor_delete_key_pressed = false,
                                      .cursor_mouse_x = 0,
                                      .cursor_mouse_y = 0,
                                      .cursor_left_press_multiplicity = 0U,
                                      .special_mode_state =
                                          &special_mode_state,
                                      .ani_drift = action_ports,
                                      .ani_directional = action_ports,
                                      .ani_follower = action_ports,
                                      .timed_message_runtime = frame_ports,
                                      .flagged_roles = action_ports,
                                      .world_roles = role_ports,
                                      .spatial_audio = audio_ports,
                                  },
                                  outer_ports);
  }

  LegacyPictureActionLists picture_actions;
  LegacyMovingActionList moving_actions;
  openswd3::world_map::LegacyRoleHeadActionList role_head_actions;
};

[[nodiscard]] std::vector<u32> expected_normal_events() {
  using Inner = LegacyWorldFrameStage;
  return {
      kHeadSignEventBase + 3U,
      kHeadSignEventBase + 2U,
      kHeadSignEventBase + 1U,
      kHeadSignEventBase + 0U,
      kAudioEvent,
      frame_event(Inner::timed_ui_update_0042ed40),
      kCursorEventBase + 8U,
      kCursorEventBase,
      kCountdownEventBase + 1U,
      kCountdownEventBase + 2U,
      kCountdownEventBase + 10U,
      kCountdownEventBase + 3U,
      kCountdownEventBase + 4U,
      kDebugOverlayEvent,
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
          result.frame.primary_picture_actions_executed &&
          result.frame.secondary_picture_actions_executed &&
          result.frame.packed_rows_executed &&
          result.frame.frame_color_executed &&
          result.frame.timed_messages_executed &&
          result.frame.cursor_executed &&
          result.frame.cursor.cursor_draw_count == 1U &&
          result.selection_scroll ==
              LegacyWorldSelectionScrollStatus::completed &&
          result.debug_overlay_executed &&
          result.debug_overlay.text_style_configured &&
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
          result.countdown_stage_executed &&
          result.countdown.status ==
              openswd3::rendering::LegacyCountdownDisplayStatus::completed &&
          result.countdown.displayed_seconds == 754 &&
          result.countdown.draw_call_count == 5U &&
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
          fixture.state.countdown_action.action_id == 0x232CU &&
          fixture.state.countdown_action.variant_delta == 0U &&
          fixture.state.countdown_action.base_variant == 4U &&
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
  test.expect_true(fixture.outer_ports.text_configuration_calls == 1U &&
                       fixture.outer_ports.configured_background == 0xFFFEU &&
                       fixture.outer_ports.configured_secondary == 0U,
                   "the debug overlay configures the 16-point renderer at "
                   "the exact assembly slot");
  test.expect_true(fixture.state.movement.camera_x_transition == 0 &&
                       fixture.state.movement.player_x_transition == 0 &&
                       fixture.state.movement.camera_y_transition == 0 &&
                       fixture.state.movement.player_y_transition == 0,
                   "the four movement transitions clear only after "
                   "post-present bookkeeping");
  test.expect_true(fixture.framebuffer.row_pixels(8U)[400U] == 0x1234U &&
                       fixture.framebuffer.row_pixels(8U)[404U] == 0x1234U,
                   "the countdown draws its first and last pieces at 400,8");
}

void test_hidden_countdown_preserves_action_state(
    openswd3::test::Context &test) {
  Fixture fixture;
  fixture.frame_ports.service_flags[0x10U] = false;
  fixture.state.countdown_action.action_id = 0x11223344U;
  fixture.state.countdown_action.base_variant = 0x55667788U;
  fixture.state.countdown_action.variant_delta = 0x99AABBCCU;
  const std::array<i16, 1U> selection{-12337};

  const auto result = fixture.run(selection);
  test.expect_true(
      result.status == LegacyWorldFrameCoordinatorStatus::completed &&
          result.countdown_stage_executed &&
          result.countdown.status ==
              openswd3::rendering::LegacyCountdownDisplayStatus::
                  hidden_inactive &&
          result.countdown.piece_request_count == 0U &&
          fixture.state.countdown_action.action_id == 0x11223344U &&
          fixture.state.countdown_action.base_variant == 0x55667788U &&
          fixture.state.countdown_action.variant_delta == 0x99AABBCCU,
      "an inactive primary timer returns before touching the static action");
  test.expect_true(std::ranges::none_of(fixture.events,
                                        [](const u32 event) {
                                          return event >= kCountdownEventBase &&
                                                 event < kCountdownEventBase +
                                                             0x100U;
                                        }),
                   "the inactive gate performs no countdown action update");
}

void test_developer_tools_require_exact_one(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.state.developer_tools_enabled = 2U;
  const std::array<i16, 1U> selection{-12337};
  const auto result = fixture.run(selection);

  test.expect_true(result.status ==
                           LegacyWorldFrameCoordinatorStatus::completed &&
                       !result.debug_overlay_executed,
                   "noncanonical developer-tools state two does not alias one");
  test.expect_true(
      std::ranges::find(fixture.events, kDebugOverlayEvent) ==
          fixture.events.end(),
      "00413FE0 is skipped unless the developer gate equals one exactly");
}

void test_party_count_and_alignment_gates(openswd3::test::Context &test) {
  for (const u32 party_count : std::array<u32, 2U>{0U, 1U}) {
    Fixture fixture;
    fixture.state.party_role_count = party_count;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status == LegacyWorldFrameCoordinatorStatus::completed &&
            result.debug_overlay_executed &&
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
            result.debug_overlay_executed &&
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
            !result.debug_overlay_executed &&
            result.audio_service_count == 0U &&
            !result.presentation_requested && fixture.roles[1].world_x == 128U,
        "invalid party-slot ownership stops after player motion and before "
        "00414570");
  }

  {
    Fixture fixture;
    fixture.state.debug_overlay.diagnostic_text_visible = 1U;
    fixture.state.debug_overlay.frame_interval_milliseconds = 0U;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::debug_overlay_failed &&
            result.debug_overlay.status ==
                openswd3::world_map::LegacyWorldDebugOverlayStatus::
                    zero_frame_interval &&
            result.debug_overlay_executed && result.player_motion_applied &&
            result.camera_pan_advanced && result.countdown_stage_executed &&
            result.audio_service_count == 1U &&
            !result.presentation_requested && fixture.roles[1].world_x == 128U,
        "debug-overlay failure preserves earlier state and stops in place");
  }

  {
    Fixture fixture;
    fixture.action_ports.unavailable_frame_index = 10U;
    const std::array<i16, 1U> selection{-12337};
    const auto result = fixture.run(selection);
    test.expect_true(
        result.status == LegacyWorldFrameCoordinatorStatus::countdown_failed &&
            result.countdown.status ==
                openswd3::rendering::LegacyCountdownDisplayStatus::
                    piece_unavailable &&
            result.countdown.piece_request_count == 3U &&
            result.countdown.draw_call_count == 2U &&
            result.countdown_stage_executed && !result.debug_overlay_executed &&
            result.audio_service_count == 1U && !result.presentation_requested,
        "a missing countdown piece stops at 004308C0 after two prior draws");
  }

  {
    Fixture fixture;
    const std::array<i16, 1U> missing_y{1};
    const auto result = fixture.run(missing_y);
    test.expect_true(
        result.status ==
                LegacyWorldFrameCoordinatorStatus::invalid_selection_window &&
            !result.debug_overlay_executed &&
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
            !result.debug_overlay_executed && !result.presentation_requested &&
            !result.tile_animation_advanced && fixture.camera.left == 19U &&
            fixture.camera.top == 13U,
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
  test_hidden_countdown_preserves_action_state(test);
  test_developer_tools_require_exact_one(test);
  test_party_count_and_alignment_gates(test);
  test_checked_failures_stop_at_the_original_slot(test);
  test_head_sign_update_failure_remains_nonfatal(test);
  return test.exit_code();
}
