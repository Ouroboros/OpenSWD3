#include "test.hpp"

#include "openswd3/world_map/legacy_world_cursor.hpp"

#include <limits>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::world_map::kLegacyWorldCursorActionId;
using openswd3::world_map::LegacyWorldCursorFrameInput;
using openswd3::world_map::LegacyWorldCursorState;
using openswd3::world_map::LegacyWorldCursorStatus;
using openswd3::world_map::prime_legacy_world_cursor_state;
using openswd3::world_map::update_draw_legacy_world_cursor;

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity{};

  [[nodiscard]] bool operator==(const DrawCall &) const = default;
};

class RecordingPorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    updated_variants.push_back(record.base_variant);
    if (write_frame_fields) {
      record.field_4a = static_cast<u16>(0x1200U + record.base_variant);
      record.field_4c = static_cast<u16>(0x40U + record.base_variant);
    }
    return record.base_variant == failed_variant
               ? LegacyActionUpdateStatus::stream_load_failed
               : LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 16U;
    piece.height = 16U;
    return frame_index != unavailable_frame;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step) noexcept override {
    draws.push_back(DrawCall{destination_x, destination_y, flags,
                             opacity_step});
    return draw_status;
  }

  std::vector<u32> updated_variants;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
  u32 failed_variant{0xFFFFFFFFU};
  u16 unavailable_frame{0xFFFFU};
  LegacyBlitExecutionStatus draw_status{LegacyBlitExecutionStatus::completed};
  bool write_frame_fields{true};
};

void test_initialization_and_prime(openswd3::test::Context &test) {
  LegacyWorldCursorState state;
  RecordingPorts ports;

  const auto status = prime_legacy_world_cursor_state(state, ports);
  test.expect_true(
      status == LegacyActionUpdateStatus::completed &&
          state.cursor_action.action_id == kLegacyWorldCursorActionId &&
          state.cursor_action.base_variant == 0U &&
          state.edge_action.action_id == kLegacyWorldCursorActionId &&
          state.edge_action.base_variant == 8U && state.edge_x == 2 &&
          state.edge_idle_frames == 0 &&
          state.previous_cursor_base_variant == 0U &&
          ports.updated_variants == std::vector<u32>{0U},
      "0x0040E0B0 creates both 0x2329 records and primes only the cursor");
}

void test_idle_edge_click_requests_special_mode(
    openswd3::test::Context &test) {
  LegacyWorldCursorState state;
  state.edge_idle_frames = 16;
  RecordingPorts ports;
  u32 special_mode{};

  const auto result = update_draw_legacy_world_cursor(
      state,
      LegacyWorldCursorFrameInput{
          .mouse_x = 611,
          .mouse_y = 23,
          .left_press_multiplicity = 1U,
      },
      special_mode, ports);

  test.expect_true(
      result.status == LegacyWorldCursorStatus::completed &&
          result.edge_draw_requested && result.special_mode_requested &&
          special_mode == 0x80000001U && state.edge_idle_frames == 17 &&
          state.edge_x == 1 && result.edge_action.action_update_count == 1U &&
          result.edge_action.draw_count == 1U &&
          result.cursor_update_count == 1U && result.cursor_draw_count == 1U,
      "the seventeenth idle frame exposes the edge and accepts its hot corner");
  test.expect_true(
      ports.updated_variants == std::vector<u32>{8U, 0U} &&
          ports.draws ==
              std::vector<DrawCall>{
                  DrawCall{643, 0, 0U, 0},
                  DrawCall{611, 23, 0U, 0},
              },
      "the edge draw precedes the software cursor at the assembly coordinates");
}

void test_edge_motion_talk_gate_and_clamp(openswd3::test::Context &test) {
  LegacyWorldCursorState state;
  state.edge_x = -32;
  state.edge_idle_frames = 99;
  RecordingPorts ports;
  u32 special_mode{};

  const auto moving = update_draw_legacy_world_cursor(
      state,
      LegacyWorldCursorFrameInput{
          .movement_x = 1,
          .talk_target = 7U,
          .talk_phase = 8U,
      },
      special_mode, ports);
  test.expect_true(
      moving.status == LegacyWorldCursorStatus::completed &&
          state.edge_x == -31 && state.edge_idle_frames == 99 &&
          !moving.edge_draw_requested && ports.updated_variants.size() == 1U &&
          ports.draws.size() == 1U,
      "movement reveals the edge without resetting idle, while talk phase 8 "
      "suppresses only the edge draw");

  state.edge_x = -32;
  state.edge_idle_frames = 16;
  ports = RecordingPorts{};
  const auto clamped = update_draw_legacy_world_cursor(
      state, LegacyWorldCursorFrameInput{}, special_mode, ports);
  test.expect_true(clamped.edge_draw_requested && state.edge_x == -32 &&
                       state.edge_idle_frames == 17 &&
                       ports.draws.front().x == 610,
                   "the hidden edge clamps at -32 and remains drawable");
}

void test_delete_update_failure_continues_with_current_frame(
    openswd3::test::Context &test) {
  LegacyWorldCursorState state;
  state.cursor_action.wait_remaining = 7U;
  state.cursor_action.field_4a = 0x3456U;
  state.cursor_action.field_4c = 0x0078U;
  state.cursor_action.draw_offset_x = 1U;
  state.cursor_action.draw_offset_y = 2U;
  state.cursor_action.mode_flags = 0x80000013U;
  state.cursor_action.field_8a = 9U;
  RecordingPorts ports;
  ports.failed_variant = 15U;
  ports.write_frame_fields = false;
  ports.draw_status = LegacyBlitExecutionStatus::malformed_source;
  u32 special_mode{1U};

  const auto result = update_draw_legacy_world_cursor(
      state,
      LegacyWorldCursorFrameInput{
          .delete_key_pressed = true,
          .mouse_x = std::numeric_limits<i32>::min(),
          .mouse_y = 1,
      },
      special_mode, ports);

  test.expect_true(
      result.status == LegacyWorldCursorStatus::completed &&
          result.delete_variant_selected &&
          state.cursor_action.base_variant == 15U &&
          state.cursor_action.wait_remaining == 0U &&
          state.previous_cursor_base_variant == 15U &&
          result.cursor_update_failure_count == 1U &&
          result.cursor_frame_request_count == 1U &&
          result.cursor_draw_count == 1U &&
          result.cursor_blit_failure_count == 1U,
      "Delete changes variant and the diagnostic update failure stays nonfatal");
  test.expect_true(
      ports.loads == std::vector<std::pair<u16, u16>>{{0x3456U, 0x0078U}} &&
          ports.draws == std::vector<DrawCall>{DrawCall{
                             std::numeric_limits<i32>::max(), -1,
                             0x80000013U, 9}},
      "the failed update still resolves and draws its current frame with "
      "32-bit wrapping offsets");
}

void test_missing_main_frame_is_checked(openswd3::test::Context &test) {
  LegacyWorldCursorState state;
  RecordingPorts ports;
  ports.unavailable_frame = 0x40U;
  u32 special_mode{};

  const auto result = update_draw_legacy_world_cursor(
      state, LegacyWorldCursorFrameInput{}, special_mode, ports);
  test.expect_true(
      result.status == LegacyWorldCursorStatus::cursor_frame_unavailable &&
          result.edge_draw_requested && result.edge_action.draw_count == 1U &&
          result.cursor_frame_request_count == 1U &&
          result.cursor_draw_count == 0U && ports.draws.size() == 1U,
      "an unavailable main cursor frame stops at the modern checked boundary");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_initialization_and_prime(test);
  test_idle_edge_click_requests_special_mode(test);
  test_edge_motion_talk_gate_and_clamp(test);
  test_delete_update_failure_continues_with_current_frame(test);
  test_missing_main_frame_is_checked(test);
  return test.exit_code();
}
