#include "test.hpp"

#include "openswd3/world_map/legacy_picture_actions.hpp"

#include <bit>
#include <list>
#include <tuple>
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
using openswd3::world_map::LegacyPictureActionAudioPorts;
using openswd3::world_map::LegacyPictureActionNode;
using openswd3::world_map::update_draw_legacy_picture_actions;

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity{};
};

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    updated_ids.push_back(record.action_id);
    if (record.action_id == 2U) {
      return LegacyActionUpdateStatus::malformed_stream;
    }
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 16U;
    piece.height = 20U;
    return resource_id != 33U;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step) noexcept override {
    draws.push_back(DrawCall{destination_x, destination_y, flags,
                             opacity_step});
    return draws.size() == 2U ? LegacyBlitExecutionStatus::malformed_source
                             : LegacyBlitExecutionStatus::completed;
  }

  std::vector<u32> updated_ids;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
};

class RecordingAudioPorts final : public LegacyPictureActionAudioPorts {
public:
  void play_positional_sample(const u16 sound_id, const i32 world_x,
                              const i32 world_y) noexcept override {
    samples.emplace_back(sound_id, world_x, world_y);
  }

  std::vector<std::tuple<u16, i32, i32>> samples;
};

[[nodiscard]] LegacyPictureActionNode make_node(
    const u32 id, const u16 x, const u16 y, const u16 resource_id,
    const u16 frame_index, const u16 sound_id, const u32 completion) {
  LegacyPictureActionNode node{};
  node.screen_x = x;
  node.screen_y = y;
  node.action.action_id = id;
  node.action.draw_offset_x = id;
  node.action.draw_offset_y = id + 1U;
  node.action.mode_flags = 0x100U + id;
  node.action.field_4a = resource_id;
  node.action.field_4c = frame_index;
  node.action.field_58 = sound_id;
  node.action.field_8a = static_cast<openswd3::compat::u8>(0x20U + id);
  node.action.field_8c = completion;
  return node;
}

void test_exact_update_draw_audio_and_removal(
    openswd3::test::Context &test) {
  std::list<LegacyPictureActionNode> nodes;
  nodes.push_back(make_node(1U, 10U, 20U, 11U, 12U, 5U, 0U));
  nodes.push_back(make_node(2U, 30U, 40U, 22U, 23U, 0U, 1U));
  nodes.push_back(make_node(3U, 50U, 60U, 33U, 34U, 7U, 1U));
  RecordingActionPorts action_ports;
  RecordingAudioPorts audio_ports;

  const auto result = update_draw_legacy_picture_actions(
      nodes, 100, 200, action_ports, audio_ports);

  test.expect_true(
      result.visited_count == 3U &&
          result.action_update_failure_count == 1U &&
          result.frame_request_count == 3U && result.frame_failure_count == 1U &&
          result.draw_count == 2U && result.blit_failure_count == 1U &&
          result.positional_sample_count == 2U && result.removed_count == 2U,
      "0x004147E0 retains every update, draw, sound and removal boundary");
  test.expect_true(
      action_ports.updated_ids == std::vector<u32>{1U, 2U, 3U} &&
          action_ports.loads ==
              std::vector<std::pair<u16, u16>>{{11U, 12U}, {22U, 23U},
                                                {33U, 34U}} &&
          action_ports.draws.size() == 2U,
      "an action-update diagnostic does not skip the original frame request");
  test.expect_true(
      action_ports.draws[0].x == 9 && action_ports.draws[0].y == 18 &&
          action_ports.draws[0].flags == 0x101U &&
          action_ports.draws[0].opacity == 0x21 &&
          action_ports.draws[1].x == 28 &&
          action_ports.draws[1].y == 37,
      "draw coordinates and action fields use their exact post-update widths");
  test.expect_true(
      audio_ports.samples ==
              std::vector<std::tuple<u16, i32, i32>>{{5U, 110, 220},
                                                      {7U, 150, 260}} &&
          nodes.size() == 1U && nodes.front().action.action_id == 1U &&
          nodes.front().action.field_58 == 0U,
      "positional sounds use world coordinates, clear once, and completed nodes erase");
}

void test_completion_must_equal_one(openswd3::test::Context &test) {
  std::list<LegacyPictureActionNode> nodes;
  nodes.push_back(make_node(4U, 0xFFFFU, 0U, 44U, 45U, 0U, 2U));
  RecordingActionPorts action_ports;
  RecordingAudioPorts audio_ports;

  const auto result = update_draw_legacy_picture_actions(
      nodes, std::bit_cast<i32>(0x7FFFFFFFU), 0, action_ports, audio_ports);

  test.expect_true(result.removed_count == 0U && nodes.size() == 1U,
                   "completion values other than exact one remain linked");
  test.expect_equal(action_ports.draws.front().x, i32{65531},
                    "u16 screen coordinates are zero extended before subtraction");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_exact_update_draw_audio_and_removal(test);
  test_completion_must_equal_one(test);
  return test.exit_code();
}
