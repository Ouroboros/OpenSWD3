#include "test.hpp"

#include "openswd3/world_map/legacy_world_facing_talk.hpp"

#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::coordinate_legacy_world_facing_talk;
using openswd3::world_map::kLegacyWorldTalkIdleSource;
using openswd3::world_map::kLegacyWorldTalkTurningRoleFlag;
using openswd3::world_map::LegacyWorldFacingTalkPorts;
using openswd3::world_map::LegacyWorldFacingTalkStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldTalkContext;

class RecordingPorts final : public LegacyWorldFacingTalkPorts {
public:
  u32 update_action(LegacyActionRecord &action) override {
    updated.push_back(&action);
    const std::size_t index = updated.size() - 1U;
    return index < returns.size() ? returns[index] : 1U;
  }

  std::vector<LegacyActionRecord *> updated;
  std::vector<u32> returns;
};

void make_role(LegacyWorldRoleRecord &role, const u32 tile_x,
               const u32 tile_y) {
  role.world_x = tile_x << 4U;
  role.world_y = tile_y << 4U;
  role.flags = 0x00008000U;
  role.talk_script_id = 1U;
  role.action.field_2c = 1U;
  role.action.field_30 = 1U;
}

void test_role_talk(openswd3::test::Context &test) {
  std::vector<LegacyWorldRoleRecord> roles(2U);
  make_role(roles[0], 10U, 10U);
  make_role(roles[1], 10U, 9U);
  roles[1].flags |= kLegacyWorldTalkTurningRoleFlag;
  roles[1].talk_data_offset = 0x11223344U;
  roles[1].talk_script_id = 0x5678U;
  roles[1].talk_initial_offset = 0x9ABCU;
  roles[1].guid = 0x2468U;
  roles[1].action.base_variant = 9U;
  roles[1].action.variant_delta = 6U;
  roles[1].action.wait_remaining = 7U;
  roles[0].action.base_variant = 8U;
  roles[0].action.variant_delta = 1U;
  roles[0].action.wait_remaining = 5U;

  LegacyWorldTalkContext talk{};
  talk.source_guid = kLegacyWorldTalkIdleSource;
  talk.world_x = 0xAAAAAAAAU;
  talk.world_y = 0xBBBBBBBBU;
  u32 one_shot = 3U;
  RecordingPorts ports;
  ports.returns = {0U, 0U};
  const auto result = coordinate_legacy_world_facing_talk(
      {0U, 0, -1, 40U, 30U}, roles, talk, one_shot, ports);

  test.expect_true(result.talk_created, "north-facing query creates Talk");
  test.expect_equal(result.role_index, u32{1U}, "the north target is selected");
  test.expect_equal(
      result.facing, u32{1U},
      "facing value turns the northern target south toward the player");
  test.expect_equal(result.action_update_count, u32{2U},
                    "target then player are refreshed");
  test.expect_equal(result.action_update_failure_count, u32{2U},
                    "update failures are diagnostic only");
  test.expect_equal(ports.updated[0], &roles[1].action,
                    "first refresh targets NPC");
  test.expect_equal(ports.updated[1], &roles[0].action,
                    "second refresh targets player");
  test.expect_equal(roles[1].action.one_shot_base_variant, u32{9U},
                    "target saves prior action");
  test.expect_equal(roles[1].action.one_shot_variant_delta, u32{6U},
                    "target saves prior facing");
  test.expect_equal(roles[1].action.base_variant, u32{0U},
                    "target becomes idle while talking");
  test.expect_equal(roles[0].action.variant_delta, u32{0U},
                    "player receives the opposite north-facing direction");
  test.expect_equal(talk.talk_data_offset, 0x11223344U,
                    "Talk data offset is copied");
  test.expect_equal(talk.talk_script_id, u16{0x5678U}, "Talk script is copied");
  test.expect_equal(talk.instruction_offset, u16{0x9ABCU},
                    "Talk cursor is copied");
  test.expect_equal(talk.source_guid, u16{0x2468U}, "Talk GUID is copied");
  test.expect_equal(talk.world_x, 0xAAAAAAAAU,
                    "facing role path leaves Talk X stale");
  test.expect_equal(talk.world_y, 0xBBBBBBBBU,
                    "facing role path leaves Talk Y stale");
  test.expect_equal(one_shot, u32{0U},
                    "successful role Talk clears one-shot state");
}

void test_gates(openswd3::test::Context &test) {
  std::vector<LegacyWorldRoleRecord> roles(2U);
  make_role(roles[0], 10U, 10U);
  make_role(roles[1], 10U, 9U);
  LegacyWorldTalkContext talk{};
  talk.source_guid = 7U;
  u32 one_shot = 8U;
  RecordingPorts ports;
  auto busy = coordinate_legacy_world_facing_talk({0U, 0, -1, 40U, 30U}, roles,
                                                  talk, one_shot, ports);
  test.expect_false(busy.talk_created, "occupied Talk skips the facing query");
  test.expect_equal(busy.tile_query_count, u32{0U},
                    "occupied Talk probes no tiles");

  talk.source_guid = kLegacyWorldTalkIdleSource;
  roles[1].interaction_gate = 1U;
  auto gated = coordinate_legacy_world_facing_talk({0U, 0, -1, 40U, 30U}, roles,
                                                   talk, one_shot, ports);
  test.expect_false(gated.talk_created, "target interaction gate blocks Talk");
  test.expect_true(gated.tile_query_count != 0U,
                   "target gate is checked after query");
  test.expect_equal(one_shot, u32{8U},
                    "failed Talk leaves one-shot state intact");

  auto invalid = coordinate_legacy_world_facing_talk(
      {3U, 0, -1, 40U, 30U}, roles, talk, one_shot, ports);
  test.expect_equal(invalid.status,
                    LegacyWorldFacingTalkStatus::invalid_player_index,
                    "player index is checked at the modern boundary");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_role_talk(test);
  test_gates(test);
  return test.exit_code();
}
