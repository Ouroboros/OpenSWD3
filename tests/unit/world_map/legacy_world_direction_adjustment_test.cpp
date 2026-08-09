#include "test.hpp"

#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::adjust_legacy_world_direction_for_obstacles;
using openswd3::world_map::compute_legacy_world_directional_occupancy_mask;
using openswd3::world_map::compute_legacy_world_surface_clearance_mask;
using openswd3::world_map::kLegacyWorldDirectionOccupancyCellMask;
using openswd3::world_map::LegacyWorldClearanceAxis;
using openswd3::world_map::LegacyWorldDirectionProbeStatus;
using openswd3::world_map::LegacyWorldRoleRecord;

constexpr u32 kMapWidth = 16U;
constexpr u32 kMapHeight = 16U;
constexpr u32 kAnchorX = 7U;
constexpr u32 kAnchorY = 7U;

std::vector<u8> make_grid() {
  return std::vector<u8>(static_cast<std::size_t>(kMapWidth) * kMapHeight * 4U,
                         0U);
}

void set_cell(std::vector<u8> &grid, const u32 x, const u32 y,
              const u32 value) {
  const std::size_t offset = (static_cast<std::size_t>(y) * kMapWidth + x) * 4U;
  grid[offset] = static_cast<u8>(value);
  grid[offset + 1U] = static_cast<u8>(value >> 8U);
  grid[offset + 2U] = static_cast<u8>(value >> 16U);
  grid[offset + 3U] = static_cast<u8>(value >> 24U);
}

LegacyWorldRoleRecord make_role(const u32 width = 1U, const u32 height = 1U) {
  LegacyWorldRoleRecord role{};
  role.world_x = kAnchorX << 4U;
  role.world_y = kAnchorY << 4U;
  role.map_cell_pointer_32 = kAnchorY * kMapWidth + kAnchorX;
  role.action.field_2c = width;
  role.action.field_30 = height;
  return role;
}

u8 expected_direction_bits_for_cell(const i32 x, const i32 y, const i32 width,
                                    const i32 height) {
  const auto between = [](const i32 value, const i32 first, const i32 last) {
    return value >= first && value <= last;
  };
  u8 mask{};
  if ((y == -1 && between(x, -1, width - 2)) ||
      (x == -1 && between(y, -1, height - 2))) {
    mask |= 0x01U;
  }
  if (y == -1 && between(x, 0, width - 1)) {
    mask |= 0x02U;
  }
  if ((y == -1 && between(x, 1, width)) ||
      (x == width && between(y, -1, height - 2))) {
    mask |= 0x04U;
  }
  if (x == width && between(y, 0, height - 1)) {
    mask |= 0x08U;
  }
  if ((y == height && between(x, 1, width)) ||
      (x == width && between(y, 1, height))) {
    mask |= 0x10U;
  }
  if (y == height && between(x, 0, width - 1)) {
    mask |= 0x20U;
  }
  if ((y == height && between(x, -1, width - 2)) ||
      (x == -1 && between(y, 1, height))) {
    mask |= 0x40U;
  }
  if (x == -1 && between(y, 0, height - 1)) {
    mask |= 0x80U;
  }
  return mask;
}

void test_eight_direction_occupancy_bits(openswd3::test::Context &test) {
  struct Probe {
    i32 x;
    i32 y;
    u8 bit;
  };
  constexpr std::array probes{
      Probe{-1, -1, 0x01U}, Probe{0, -1, 0x02U}, Probe{1, -1, 0x04U},
      Probe{1, 0, 0x08U},   Probe{1, 1, 0x10U},  Probe{0, 1, 0x20U},
      Probe{-1, 1, 0x40U},  Probe{-1, 0, 0x80U},
  };

  for (const auto &probe : probes) {
    auto grid = make_grid();
    set_cell(grid, static_cast<u32>(static_cast<i32>(kAnchorX) + probe.x),
             static_cast<u32>(static_cast<i32>(kAnchorY) + probe.y),
             kLegacyWorldDirectionOccupancyCellMask);
    const auto result = compute_legacy_world_directional_occupancy_mask(
        grid, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX, 1U, 1U);
    test.expect_true(result.status ==
                             LegacyWorldDirectionProbeStatus::completed &&
                         result.mask == probe.bit,
                     "0x0040BB50 maps the eight surrounding cells clockwise");
  }

  auto filtered = make_grid();
  set_cell(filtered, kAnchorX, kAnchorY - 1U, 0x00800000U);
  const auto ignored = compute_legacy_world_directional_occupancy_mask(
      filtered, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX, 1U, 1U,
      0x60000000U);
  test.expect_true(
      ignored.status == LegacyWorldDirectionProbeStatus::completed &&
          ignored.mask == 0U,
      "direction occupancy only observes the caller-supplied cell mask");
}

void test_footprint_folding_and_zero_quirk(openswd3::test::Context &test) {
  auto grid = make_grid();
  set_cell(grid, kAnchorX + 1U, kAnchorY - 1U, 0x20000000U);
  set_cell(grid, kAnchorX - 1U, kAnchorY + 1U, 0x40000000U);
  const auto folded = compute_legacy_world_directional_occupancy_mask(
      grid, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX, 2U, 2U);
  test.expect_true(
      folded.status == LegacyWorldDirectionProbeStatus::completed &&
          folded.mask == static_cast<u8>(0x02U | 0x04U | 0x40U | 0x80U),
      "two-cell footprints preserve the overlapping edge folds");

  auto zero_grid = make_grid();
  set_cell(zero_grid, kAnchorX, kAnchorY - 1U, 0x20000000U);
  const auto zero = compute_legacy_world_directional_occupancy_mask(
      zero_grid, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX, 0U,
      0U);
  test.expect_true(
      zero.status == LegacyWorldDirectionProbeStatus::completed &&
          (zero.mask & 0x02U) != 0U,
      "zero HW still executes the original fixed three-cell reads");

  const auto too_large = compute_legacy_world_directional_occupancy_mask(
      grid, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX, 7U, 1U);
  test.expect_true(too_large.status ==
                       LegacyWorldDirectionProbeStatus::footprint_out_of_range,
                   "the modern boundary rejects values that overflow the "
                   "original six-slot arrays");
}

void test_all_asset_footprint_perimeters(openswd3::test::Context &test) {
  for (u32 width = 1U; width <= 6U; ++width) {
    for (u32 height = 1U; height <= 6U; ++height) {
      for (i32 y = -1; y <= static_cast<i32>(height); ++y) {
        for (i32 x = -1; x <= static_cast<i32>(width); ++x) {
          const u8 expected = expected_direction_bits_for_cell(
              x, y, static_cast<i32>(width), static_cast<i32>(height));
          if (expected == 0U) {
            continue;
          }
          auto grid = make_grid();
          set_cell(grid, static_cast<u32>(static_cast<i32>(kAnchorX) + x),
                   static_cast<u32>(static_cast<i32>(kAnchorY) + y),
                   0x00800000U);
          const auto result = compute_legacy_world_directional_occupancy_mask(
              grid, kMapWidth, kMapHeight, kAnchorY * kMapWidth + kAnchorX,
              width, height);
          test.expect_true(
              result.status == LegacyWorldDirectionProbeStatus::completed &&
                  result.mask == expected,
              "all HW 1..6 perimeter cells match the folded compass sectors");
        }
      }
    }
  }
}

void test_surface_clearance_mask(openswd3::test::Context &test) {
  auto grid = make_grid();
  const auto open = compute_legacy_world_surface_clearance_mask(
      grid, kMapWidth, kMapHeight, static_cast<i32>(kAnchorX),
      static_cast<i32>(kAnchorY), 2U, LegacyWorldClearanceAxis::vertical);
  test.expect_true(
      open.status == LegacyWorldDirectionProbeStatus::completed &&
          open.mask == 0x3E0U,
      "three negative and positive clear stripes produce bits 0x20..0x200");

  set_cell(grid, kAnchorX, kAnchorY, 0x20000000U);
  const auto blocked_center = compute_legacy_world_surface_clearance_mask(
      grid, kMapWidth, kMapHeight, static_cast<i32>(kAnchorX),
      static_cast<i32>(kAnchorY), 2U, LegacyWorldClearanceAxis::vertical);
  test.expect_equal(
      blocked_center.mask, u32{0x320U},
      "overlapping spans clear the center and negative-one stripes");

  const auto top = compute_legacy_world_surface_clearance_mask(
      make_grid(), kMapWidth, kMapHeight, static_cast<i32>(kAnchorX), 0, 2U,
      LegacyWorldClearanceAxis::vertical);
  test.expect_equal(
      top.mask, u32{0x380U},
      "negative stripes use the selected-coordinate lower-bound checks");
}

void test_cardinal_adjustment(openswd3::test::Context &test) {
  const LegacyWorldRoleRecord role = make_role();
  auto open_grid = make_grid();
  const auto unchanged = adjust_legacy_world_direction_for_obstacles(
      role, -1, 0, kMapWidth, kMapHeight, open_grid);
  test.expect_true(unchanged.status ==
                           LegacyWorldDirectionProbeStatus::completed &&
                       unchanged.delta_x == -1 && unchanged.delta_y == 0 &&
                       unchanged.occupancy_mask == 0U &&
                       unchanged.surface_clearance_mask == 0x3E0U &&
                       unchanged.used_surface_clearance,
                   "an open cardinal lane remains unchanged");

  auto center_blocked = make_grid();
  set_cell(center_blocked, kAnchorX - 1U, kAnchorY, 0x20000000U);
  const auto turn_up = adjust_legacy_world_direction_for_obstacles(
      role, -1, 0, kMapWidth, kMapHeight, center_blocked);
  test.expect_true(
      turn_up.delta_x == -1 && turn_up.delta_y == -1,
      "blocked left center first tries the negative adjacent stripe");

  set_cell(center_blocked, kAnchorX - 1U, kAnchorY - 1U, 0x00800000U);
  const auto turn_down = adjust_legacy_world_direction_for_obstacles(
      role, -1, 0, kMapWidth, kMapHeight, center_blocked);
  test.expect_true(
      turn_down.delta_x == -1 && turn_down.delta_y == 1,
      "occupied negative side makes the cardinal rule try the positive side");

  auto vertical_blocked = make_grid();
  set_cell(vertical_blocked, kAnchorX, kAnchorY - 1U, 0x20000000U);
  const auto turn_left = adjust_legacy_world_direction_for_obstacles(
      role, 0, -1, kMapWidth, kMapHeight, vertical_blocked);
  test.expect_true(
      turn_left.delta_x == -1 && turn_left.delta_y == -1,
      "blocked upward center first tries the negative horizontal stripe");

  auto right_blocked = make_grid();
  set_cell(right_blocked, kAnchorX + 1U, kAnchorY, 0x20000000U);
  const auto right_turn_up = adjust_legacy_world_direction_for_obstacles(
      role, 1, 0, kMapWidth, kMapHeight, right_blocked);
  test.expect_true(
      right_turn_up.delta_x == 1 && right_turn_up.delta_y == -1,
      "blocked right center uses the positive-width direction table");

  auto down_blocked = make_grid();
  set_cell(down_blocked, kAnchorX, kAnchorY + 1U, 0x20000000U);
  const auto down_turn_left = adjust_legacy_world_direction_for_obstacles(
      role, 0, 1, kMapWidth, kMapHeight, down_blocked);
  test.expect_true(
      down_turn_left.delta_x == -1 && down_turn_left.delta_y == 1,
      "blocked downward center uses the positive-height direction table");
}

void test_diagonal_adjustment(openswd3::test::Context &test) {
  const LegacyWorldRoleRecord role = make_role();
  auto grid = make_grid();
  set_cell(grid, kAnchorX - 1U, kAnchorY - 1U, 0x20000000U);
  const auto slide_left = adjust_legacy_world_direction_for_obstacles(
      role, -1, -1, kMapWidth, kMapHeight, grid);
  test.expect_true(
      slide_left.delta_x == -1 && slide_left.delta_y == 0 &&
          !slide_left.used_surface_clearance,
      "north-west occupancy first preserves the horizontal component");

  set_cell(grid, kAnchorX - 1U, kAnchorY, 0x20000000U);
  const auto slide_up = adjust_legacy_world_direction_for_obstacles(
      role, -1, -1, kMapWidth, kMapHeight, grid);
  test.expect_true(
      slide_up.delta_x == 0 && slide_up.delta_y == -1,
      "blocked horizontal component next preserves the vertical component");

  set_cell(grid, kAnchorX, kAnchorY - 1U, 0x20000000U);
  const auto stopped = adjust_legacy_world_direction_for_obstacles(
      role, -1, -1, kMapWidth, kMapHeight, grid);
  test.expect_true(stopped.delta_x == 0 && stopped.delta_y == 0,
                   "all three north-west sector bits stop the diagonal");

  auto opposite = make_grid();
  set_cell(opposite, kAnchorX + 1U, kAnchorY - 1U, 0x20000000U);
  set_cell(opposite, kAnchorX + 1U, kAnchorY, 0x20000000U);
  const auto slide_up_right = adjust_legacy_world_direction_for_obstacles(
      role, 1, -1, kMapWidth, kMapHeight, opposite);
  test.expect_true(
      slide_up_right.delta_x == 0 && slide_up_right.delta_y == -1,
      "opposite-sign diagonal negates the rule offset for its vertical slide");

  auto south_east = make_grid();
  set_cell(south_east, kAnchorX + 1U, kAnchorY + 1U, 0x20000000U);
  const auto slide_right = adjust_legacy_world_direction_for_obstacles(
      role, 1, 1, kMapWidth, kMapHeight, south_east);
  test.expect_true(slide_right.delta_x == 1 && slide_right.delta_y == 0,
                   "south-east occupancy uses the positive same-sign table");
}

void test_invalid_boundaries(openswd3::test::Context &test) {
  const auto invalid_size = compute_legacy_world_surface_clearance_mask(
      std::array<u8, 3U>{}, 1U, 1U, 0, 0, 1U,
      LegacyWorldClearanceAxis::horizontal);
  test.expect_true(invalid_size.status ==
                       LegacyWorldDirectionProbeStatus::invalid_surface_grid,
                   "truncated four-byte surface cells are rejected");

  auto grid = make_grid();
  const auto edge = compute_legacy_world_directional_occupancy_mask(
      grid, kMapWidth, kMapHeight, 0U, 1U, 1U);
  test.expect_true(
      edge.status ==
          LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds,
      "unavailable original padding is reported instead of dereferenced");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_eight_direction_occupancy_bits(test);
  test_footprint_folding_and_zero_quirk(test);
  test_all_asset_footprint_perimeters(test);
  test_surface_clearance_mask(test);
  test_cardinal_adjustment(test);
  test_diagonal_adjustment(test);
  test_invalid_boundaries(test);
  return test.exit_code();
}
