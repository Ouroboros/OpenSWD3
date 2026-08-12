#include "test.hpp"

#include "openswd3/world_map/legacy_world_facing_role_query.hpp"

#include <array>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::world_map::find_legacy_world_facing_role;
using openswd3::world_map::kLegacyWorldFacingRoleNotFound;
using openswd3::world_map::LegacyWorldFacingRoleQueryStatus;
using openswd3::world_map::LegacyWorldRoleRecord;

void make_role(LegacyWorldRoleRecord &role, const u32 tile_x, const u32 tile_y,
               const u32 width = 1U, const u32 height = 1U) {
  role.world_x = tile_x << 4U;
  role.world_y = tile_y << 4U;
  role.flags = 0x00008000U;
  role.talk_script_id = 1U;
  role.action.field_2c = width;
  role.action.field_30 = height;
}

void test_invalid_player(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 1> roles{};
  const auto result =
      find_legacy_world_facing_role(roles, 1U, 1U, 0, -1, 40U, 30U);
  test.expect_equal(result.status,
                    LegacyWorldFacingRoleQueryStatus::invalid_player_index,
                    "player index is checked at the modern span boundary");
  test.expect_equal(result.tile_query_count, u32{0U},
                    "invalid player does not probe tiles");
}

void test_cardinal_scan_order(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 6> roles{};
  make_role(roles[0], 10U, 10U, 2U, 2U);
  make_role(roles[1], 10U, 8U);
  make_role(roles[2], 9U, 9U);
  make_role(roles[3], 7U, 10U);
  make_role(roles[4], 12U, 10U);
  make_role(roles[5], 10U, 12U);

  const auto north =
      find_legacy_world_facing_role(roles, 6U, 0U, 0, -1, 40U, 30U);
  test.expect_equal(north.role_index, u32{2U},
                    "north scans the near row from player left edge first");

  roles[2].talk_script_id = 0U;
  const auto west =
      find_legacy_world_facing_role(roles, 6U, 0U, -1, 0, 40U, 30U);
  test.expect_equal(west.role_index, u32{3U},
                    "west scans outward from the adjacent column");
  const auto east =
      find_legacy_world_facing_role(roles, 6U, 0U, 1, 0, 40U, 30U);
  test.expect_equal(east.role_index, u32{4U},
                    "east begins at player tile plus player width");
  const auto south =
      find_legacy_world_facing_role(roles, 6U, 0U, 0, 1, 40U, 30U);
  test.expect_equal(south.role_index, u32{5U},
                    "south begins at player tile plus player height");
}

void test_diagonal_endpoint_rules(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 5> roles{};
  make_role(roles[0], 10U, 10U);
  make_role(roles[1], 9U, 9U);
  make_role(roles[2], 8U, 8U);
  make_role(roles[3], 6U, 6U);
  make_role(roles[4], 11U, 5U);

  const auto north_west =
      find_legacy_world_facing_role(roles, 5U, 0U, -1, -1, 40U, 30U);
  test.expect_equal(north_west.role_index, u32{1U},
                    "north-west includes the nearest diagonal tile");
  roles[1].talk_script_id = 0U;
  const auto next =
      find_legacy_world_facing_role(roles, 5U, 0U, -1, -1, 40U, 30U);
  test.expect_equal(next.role_index, u32{2U},
                    "north-west preserves row-major outward scan order");
  roles[2].talk_script_id = 0U;
  const auto fifth_edge =
      find_legacy_world_facing_role(roles, 5U, 0U, -1, -1, 40U, 30U);
  test.expect_equal(fifth_edge.role_index, u32{3U},
                    "north-west includes the fifth extent endpoint");

  roles[1].talk_script_id = 1U;
  const auto north_east =
      find_legacy_world_facing_role(roles, 5U, 0U, 1, -1, 40U, 30U);
  test.expect_equal(north_east.role_index, u32{4U},
                    "north-east includes the fifth vertical endpoint");
}

void test_map_edge_and_fallback(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 3> roles{};
  make_role(roles[0], 1U, 1U, 2U, 2U);
  make_role(roles[1], 0U, 0U);
  make_role(roles[2], 2U, 2U);

  const auto north_west =
      find_legacy_world_facing_role(roles, 3U, 0U, -1, -1, 3U, 3U);
  test.expect_equal(north_west.role_index, u32{1U},
                    "edge-limited diagonal scan still probes coordinate zero");

  roles[1].talk_script_id = 0U;
  const auto fallback =
      find_legacy_world_facing_role(roles, 3U, 0U, 2, 2, 3U, 3U);
  test.expect_equal(fallback.role_index, u32{2U},
                    "unsupported direction falls back to the player footprint");
}

void test_no_match_query_count(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 1> roles{};
  make_role(roles[0], 10U, 10U, 1U, 1U);
  const auto result =
      find_legacy_world_facing_role(roles, 1U, 0U, -1, 0, 40U, 30U);
  test.expect_equal(result.role_index, kLegacyWorldFacingRoleNotFound,
                    "no candidate returns the legacy sentinel");
  test.expect_equal(
      result.tile_query_count, u32{16U},
      "west probes five three-tile columns then one footprint tile");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_invalid_player(test);
  test_cardinal_scan_order(test);
  test_diagonal_endpoint_rules(test);
  test_map_edge_and_fallback(test);
  test_no_match_query_count(test);
  return test.exit_code();
}
