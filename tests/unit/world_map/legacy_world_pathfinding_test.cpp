#include "test.hpp"

#include "openswd3/world_map/legacy_world_pathfinding.hpp"

#include <cstddef>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::kLegacyWorldPathDefaultCollisionMask;
using openswd3::world_map::LegacyWorldPathfinder;
using openswd3::world_map::LegacyWorldPathfindingRequest;
using openswd3::world_map::LegacyWorldPathfindingStatus;
using openswd3::world_map::LegacyWorldPathNodePool;

constexpr u32 kMapWidth = 16U;
constexpr u32 kMapHeight = 16U;

std::vector<u8> make_grid(const u32 width = kMapWidth,
                          const u32 height = kMapHeight) {
  return std::vector<u8>(static_cast<std::size_t>(width) * height * 4U, 0U);
}

void set_cell(std::vector<u8> &grid, const u32 map_width, const u32 x,
              const u32 y, const u32 value) {
  const std::size_t offset = (static_cast<std::size_t>(y) * map_width + x) * 4U;
  grid[offset] = static_cast<u8>(value);
  grid[offset + 1U] = static_cast<u8>(value >> 8U);
  grid[offset + 2U] = static_cast<u8>(value >> 16U);
  grid[offset + 3U] = static_cast<u8>(value >> 24U);
}

LegacyWorldPathfindingRequest make_request(const std::vector<u8> &grid,
                                           const i32 start_x, const i32 start_y,
                                           const i32 target_x,
                                           const i32 target_y) {
  return {
      .start_x = start_x << 4U,
      .start_y = start_y << 4U,
      .target_x = target_x << 4U,
      .target_y = target_y << 4U,
      .footprint_width = 1U,
      .footprint_height = 1U,
      .map_width = kMapWidth,
      .map_height = kMapHeight,
      .surface_grid = grid,
  };
}

void test_direct_and_diagonal_routes(openswd3::test::Context &test) {
  const auto grid = make_grid();
  LegacyWorldPathNodePool pool;

  LegacyWorldPathfinder horizontal{pool};
  const auto straight = horizontal.find_path(make_request(grid, 5, 5, 7, 5));
  test.expect_true(straight.status == LegacyWorldPathfindingStatus::completed &&
                       straight.legacy_return_value == 1 &&
                       straight.path_length == 2U && straight.path[0] == 7U &&
                       straight.path[1] == 7U && straight.path[2] == 0xFFU,
                   "0x00402230 writes two east steps and the FF terminator");

  LegacyWorldPathfinder diagonal{pool};
  const auto route = diagonal.find_path(make_request(grid, 5, 5, 7, 7));
  test.expect_true(route.legacy_return_value == 1 && route.path_length == 2U &&
                       route.path[0] == 0U && route.path[1] == 0U &&
                       route.path[2] == 0xFFU,
                   "the reverse A* parent chain becomes forward SE directions");

  const auto statistics = pool.statistics();
  test.expect_true(statistics.available_nodes == statistics.allocated_nodes &&
                       statistics.allocated_nodes >= 8U &&
                       statistics.high_water_nodes == 8U,
                   "0x00402410 returns every active search node to the pool");
}

void test_obstacle_detour_and_collision_mask(openswd3::test::Context &test) {
  auto grid = make_grid();
  set_cell(grid, kMapWidth, 6U, 5U, kLegacyWorldPathDefaultCollisionMask);

  LegacyWorldPathNodePool pool;
  LegacyWorldPathfinder detour{pool};
  const auto route = detour.find_path(make_request(grid, 5, 5, 7, 5));
  test.expect_true(
      route.legacy_return_value == 1 && route.path_length == 2U &&
          route.path[0] == 0U && route.path[1] == 6U,
      "a blocked west probe preserves the equal-cost SE/NE detour");

  set_cell(grid, kMapWidth, 7U, 5U, kLegacyWorldPathDefaultCollisionMask);
  LegacyWorldPathfinder blocked{pool};
  const auto failure = blocked.find_path(make_request(grid, 5, 5, 7, 5));
  test.expect_true(failure.status == LegacyWorldPathfindingStatus::completed &&
                       failure.legacy_return_value == 0 &&
                       failure.path[0] == 0xFFU,
                   "0x004023E0 rejects a masked destination before searching");

  LegacyWorldPathfinder mask_override{pool};
  mask_override.set_collision_mask(0U);
  const auto allowed = mask_override.find_path(make_request(grid, 5, 5, 7, 5));
  test.expect_true(
      mask_override.collision_mask() == 0U &&
          allowed.legacy_return_value == 1 && allowed.path_length == 2U,
      "0x00402F70 changes the mask used by target and edge probes");
}

void test_legacy_early_returns_and_object_state(openswd3::test::Context &test) {
  const auto grid = make_grid();
  LegacyWorldPathNodePool pool;
  LegacyWorldPathfinder pathfinder{pool};

  const auto same_position =
      pathfinder.find_path(make_request(grid, 5, 5, 5, 5));
  test.expect_true(same_position.legacy_return_value == 1 &&
                       same_position.path_length == 0U &&
                       same_position.path[0] == 0xFFU &&
                       !pathfinder.legacy_success_flag(),
                   "identical coordinates return one without setting +0x10");

  auto unchecked_same_position = make_request(grid, 5, 5, 5, 5);
  unchecked_same_position.map_width = 0U;
  unchecked_same_position.map_height = 0U;
  unchecked_same_position.surface_grid = {};
  const auto unchecked = pathfinder.find_path(unchecked_same_position);
  test.expect_true(unchecked.status ==
                           LegacyWorldPathfindingStatus::completed &&
                       unchecked.legacy_return_value == 1,
                   "the identical-coordinate return precedes every map access");

  const auto first = pathfinder.find_path(make_request(grid, 5, 5, 7, 5));
  const auto second = pathfinder.find_path(make_request(grid, 5, 5, 7, 5));
  test.expect_true(first.legacy_return_value == 1 &&
                       pathfinder.legacy_success_flag() &&
                       second.legacy_return_value == 0,
                   "the original reusable object keeps +0x10 after success");

  LegacyWorldPathfinder same_cell{pool};
  auto request = make_request(grid, 5, 5, 5, 5);
  request.target_x += 8;
  const auto same_tile = same_cell.find_path(request);
  test.expect_true(
      same_tile.legacy_return_value == 0 &&
          same_tile.status == LegacyWorldPathfindingStatus::completed,
      "different coordinates in one cell take the legacy failure path");
}

void test_boundaries_and_legacy_path_limit(openswd3::test::Context &test) {
  const auto grid = make_grid();
  LegacyWorldPathNodePool pool;

  LegacyWorldPathfinder bad_dimensions{pool};
  auto request = make_request(grid, 5, 5, 7, 5);
  request.map_width = 0U;
  const auto invalid_dimensions = bad_dimensions.find_path(request);
  test.expect_true(
      invalid_dimensions.status ==
              LegacyWorldPathfindingStatus::invalid_map_dimensions &&
          invalid_dimensions.legacy_return_value == 0,
      "modern validation rejects a zero map dimension");

  LegacyWorldPathfinder bad_footprint{pool};
  request = make_request(grid, 5, 5, 7, 5);
  request.footprint_width = 7U;
  const auto invalid_footprint = bad_footprint.find_path(request);
  test.expect_true(
      invalid_footprint.status ==
              LegacyWorldPathfindingStatus::footprint_out_of_range &&
          invalid_footprint.legacy_return_value == 0,
      "modern validation protects the original six-slot arrays");

  constexpr u32 long_width = 520U;
  constexpr u32 long_height = 3U;
  auto long_grid = make_grid(long_width, long_height);
  for (u32 x = 0U; x < long_width; ++x) {
    set_cell(long_grid, long_width, x, 0U,
             kLegacyWorldPathDefaultCollisionMask);
    set_cell(long_grid, long_width, x, 2U,
             kLegacyWorldPathDefaultCollisionMask);
  }
  set_cell(long_grid, long_width, 0U, 1U, kLegacyWorldPathDefaultCollisionMask);
  set_cell(long_grid, long_width, long_width - 1U, 1U,
           kLegacyWorldPathDefaultCollisionMask);
  LegacyWorldPathfinder too_long{pool};
  const auto long_route = too_long.find_path({
      .start_x = 1 << 4U,
      .start_y = 1 << 4U,
      .target_x = 518 << 4U,
      .target_y = 1 << 4U,
      .footprint_width = 1U,
      .footprint_height = 1U,
      .map_width = long_width,
      .map_height = long_height,
      .surface_grid = long_grid,
  });
  test.expect_true(
      long_route.status == LegacyWorldPathfindingStatus::completed &&
          long_route.legacy_return_value == 1 &&
          long_route.path_length > 0x1FEU &&
          long_route.path.size() == long_route.path_length + 1U &&
          long_route.path.back() == 0xFFU &&
          long_route.legacy_path_limit_exceeded,
      "the 510-step warning does not stop the original path write");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_direct_and_diagonal_routes(test);
  test_obstacle_detour_and_collision_mask(test);
  test_legacy_early_returns_and_object_state(test);
  test_boundaries_and_legacy_path_limit(test);
  return test.exit_code();
}
