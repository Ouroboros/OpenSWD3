#include "test.hpp"

#include "openswd3/world_map/legacy_role_head_actions.hpp"

#include <bit>
#include <cstddef>
#include <iterator>
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
using openswd3::world_map::LegacyRoleHeadActionList;
using openswd3::world_map::LegacyRoleHeadActionNode;
using openswd3::world_map::update_draw_legacy_role_head_actions;

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity{};
};

class RecordingPorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    updated_ids.push_back(record.action_id);
    record.field_4a = static_cast<u16>(0x2200U + record.action_id);
    record.field_4c = static_cast<u16>(0x30U + record.action_id);
    return record.action_id == 2U ? LegacyActionUpdateStatus::malformed_stream
                                  : LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 16U;
    piece.height = 16U;
    return resource_id != 0x2204U;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step) noexcept override {
    draws.push_back(
        DrawCall{destination_x, destination_y, flags, opacity_step});
    return draws.size() == 3U ? LegacyBlitExecutionStatus::malformed_source
                              : LegacyBlitExecutionStatus::completed;
  }

  std::vector<u32> updated_ids;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
};

[[nodiscard]] LegacyRoleHeadActionNode
make_node(const u32 id, const i32 x, const i32 target_x, const i32 y) {
  LegacyRoleHeadActionNode node{};
  node.action.action_id = id;
  node.action.draw_offset_x = id;
  node.action.draw_offset_y = id + 1U;
  node.action.mode_flags = 0x200U + id;
  node.action.field_8a = static_cast<openswd3::compat::u8>(0x40U + id);
  node.current_x = static_cast<openswd3::compat::i16>(x);
  node.target_x = static_cast<openswd3::compat::i16>(target_x);
  node.y = static_cast<openswd3::compat::i16>(y);
  return node;
}

void test_exact_layout_and_frame_order(openswd3::test::Context &test) {
  test.expect_equal(sizeof(LegacyRoleHeadActionNode), std::size_t{0xB4U},
                    "role-head node retains its full physical size");
  test.expect_equal(offsetof(LegacyRoleHeadActionNode, current_x),
                    std::size_t{0x98U}, "current X keeps the producer offset");
  test.expect_equal(offsetof(LegacyRoleHeadActionNode, horizontal_motion),
                    std::size_t{0x9AU}, "motion word keeps its exact offset");
  test.expect_equal(offsetof(LegacyRoleHeadActionNode, y), std::size_t{0x9EU},
                    "fixed Y keeps its exact offset");
  test.expect_equal(offsetof(LegacyRoleHeadActionNode, next_pointer_32),
                    std::size_t{0xB0U}, "legacy next remains the final dword");

  LegacyRoleHeadActionList nodes;
  nodes.push_back(make_node(1U, 0, 2, 20));
  auto ballistic = make_node(2U, 759, 759, 21);
  ballistic.horizontal_motion = 2;
  nodes.push_back(ballistic);
  auto special = make_node(3U, 10, 13, 22);
  special.horizontal_motion =
      std::bit_cast<openswd3::compat::i16>(static_cast<u16>(0x8000U));
  nodes.push_back(special);
  nodes.push_back(make_node(4U, 30, 30, 23));
  nodes.push_back(make_node(5U, 40, 40, 24));
  auto retained_ballistic = make_node(6U, 100, 100, 25);
  retained_ballistic.horizontal_motion = 2;
  nodes.push_back(retained_ballistic);

  RecordingPorts ports;
  const auto result = update_draw_legacy_role_head_actions(nodes, ports);

  test.expect_true(
      result.visited_count == 6U && result.action_update_failure_count == 1U &&
          result.frame_request_count == 6U &&
          result.frame_failure_count == 1U && result.draw_count == 5U &&
          result.blit_failure_count == 1U && result.removed_count == 1U,
      "0x00414CE0 preserves update, draw, movement and retirement order");
  test.expect_true(
      ports.updated_ids == std::vector<u32>{1U, 2U, 3U, 4U, 5U, 6U} &&
          ports.loads == std::vector<std::pair<u16, u16>>{{0x2201U, 0x31U},
                                                          {0x2202U, 0x32U},
                                                          {0x2203U, 0x33U},
                                                          {0x2204U, 0x34U},
                                                          {0x2205U, 0x35U},
                                                          {0x2206U, 0x36U}} &&
          ports.draws.size() == 5U && ports.draws[0].x == -1 &&
          ports.draws[0].y == 18 && ports.draws[0].flags == 0x201U &&
          ports.draws[0].opacity == 0x41 && ports.draws[1].x == 757 &&
          ports.draws[1].y == 18,
      "drawing uses pre-movement coordinates and post-update action fields");
  test.expect_true(
      nodes.size() == 5U && nodes.front().current_x == 2 &&
          nodes.front().action.action_id == 1U &&
          std::next(nodes.begin())->current_x == 12 &&
          nodes.back().current_x == 102 && nodes.back().horizontal_motion == 6,
      "easing, bit15 selection and retained threefold motion are exact");
}

void test_ballistic_boundaries_are_inclusive(openswd3::test::Context &test) {
  LegacyRoleHeadActionList nodes;
  auto left = make_node(6U, -119, -119, 0);
  left.horizontal_motion = -1;
  nodes.push_back(left);
  auto right = make_node(7U, 759, 759, 0);
  right.horizontal_motion = 1;
  nodes.push_back(right);
  RecordingPorts ports;

  const auto result = update_draw_legacy_role_head_actions(nodes, ports);

  test.expect_true(result.removed_count == 2U && nodes.empty(),
                   "-120 and 760 retire on the exact assembly boundaries");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_exact_layout_and_frame_order(test);
  test_ballistic_boundaries_are_inclusive(test);
  return test.exit_code();
}
