#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u32;
using compat::u8;

constexpr u32 kSurfaceFlagMask = 0x60000000U;
constexpr u32 kClearanceCenter = 0x00000080U;
constexpr u32 kClearanceNegativeOne = 0x00000040U;
constexpr u32 kClearancePositiveOne = 0x00000100U;
constexpr u32 kClearanceNegativeFar = 0x0000003FU;
constexpr u32 kClearancePositiveFar = 0x0000FE00U;

struct DirectionRule {
  i32 offset{};
  u32 side_far{};
  u32 side_negative{};
  u32 center{};
  u32 side_positive{};
  u32 opposite_far{};
};

class SurfaceReader {
public:
  SurfaceReader(const std::span<const u8> surface_grid, const u32 map_width,
                const u32 map_height) noexcept
      : surface_grid_{surface_grid}, map_width_{map_width},
        map_height_{map_height} {}

  [[nodiscard]] LegacyWorldDirectionProbeStatus validate() const noexcept {
    if (map_width_ == 0U || map_height_ == 0U) {
      return LegacyWorldDirectionProbeStatus::invalid_map_dimensions;
    }
    constexpr std::size_t bytes_per_cell = 4U;
    if (static_cast<std::size_t>(map_width_) >
        std::numeric_limits<std::size_t>::max() /
            static_cast<std::size_t>(map_height_)) {
      return LegacyWorldDirectionProbeStatus::invalid_surface_grid;
    }
    const std::size_t cell_count = static_cast<std::size_t>(map_width_) *
                                   static_cast<std::size_t>(map_height_);
    if (cell_count > std::numeric_limits<std::size_t>::max() / bytes_per_cell ||
        surface_grid_.size() < cell_count * bytes_per_cell) {
      return LegacyWorldDirectionProbeStatus::invalid_surface_grid;
    }
    return LegacyWorldDirectionProbeStatus::completed;
  }

  [[nodiscard]] bool read(const std::int64_t cell_index,
                          u32 &value) const noexcept {
    const std::uint64_t cell_count =
        static_cast<std::uint64_t>(map_width_) * map_height_;
    if (cell_index < 0 ||
        static_cast<std::uint64_t>(cell_index) >= cell_count) {
      return false;
    }
    const std::size_t offset = static_cast<std::size_t>(cell_index) * 4U;
    value = static_cast<u32>(surface_grid_[offset]) |
            (static_cast<u32>(surface_grid_[offset + 1U]) << 8U) |
            (static_cast<u32>(surface_grid_[offset + 2U]) << 16U) |
            (static_cast<u32>(surface_grid_[offset + 3U]) << 24U);
    return true;
  }

private:
  std::span<const u8> surface_grid_;
  u32 map_width_{};
  u32 map_height_{};
};

[[nodiscard]] bool merge_masked_cell(const SurfaceReader &reader,
                                     const std::int64_t cell_index,
                                     const u32 cell_mask,
                                     u32 &destination) noexcept {
  u32 value{};
  if (!reader.read(cell_index, value)) {
    return false;
  }
  destination |= value & cell_mask;
  return true;
}

[[nodiscard]] bool scan_clear_stripe(const SurfaceReader &reader,
                                     const std::int64_t start_index,
                                     const std::int64_t cell_step,
                                     const u32 span_length,
                                     bool &clear) noexcept {
  clear = true;
  std::int64_t cell_index = start_index;
  for (u32 index = 0U; index < span_length; ++index) {
    u32 value{};
    if (!reader.read(cell_index, value)) {
      return false;
    }
    if ((value & kSurfaceFlagMask) != 0U) {
      clear = false;
    }
    cell_index += cell_step;
  }
  return true;
}

[[nodiscard]] i32 wrapping_add(const i32 left, const i32 right) noexcept {
  return std::bit_cast<i32>(std::bit_cast<u32>(left) +
                            std::bit_cast<u32>(right));
}

[[nodiscard]] i32 wrapping_add_unsigned(const u32 left,
                                        const i32 right) noexcept {
  return std::bit_cast<i32>(left + std::bit_cast<u32>(right));
}

void apply_horizontal_rule(const DirectionRule &rule, const u8 occupancy_mask,
                           const u32 clearance_mask, i32 &delta_x,
                           i32 &delta_y) noexcept {
  if (clearance_mask == 0U) {
    delta_x = 0;
    return;
  }
  if ((clearance_mask & kClearanceCenter) != 0U &&
      (occupancy_mask & rule.center) == 0U) {
    return;
  }
  if ((clearance_mask & kClearanceNegativeOne) != 0U &&
      (occupancy_mask & rule.side_negative) == 0U) {
    delta_y = -1;
    return;
  }
  if ((clearance_mask & kClearancePositiveOne) != 0U &&
      (occupancy_mask & rule.side_positive) == 0U) {
    delta_y = 1;
    return;
  }
  if ((clearance_mask & kClearanceNegativeFar) != 0U &&
      (occupancy_mask & rule.side_far) == 0U) {
    delta_x = 0;
    delta_y = -1;
    return;
  }
  if ((clearance_mask & kClearancePositiveFar) != 0U &&
      (occupancy_mask & rule.opposite_far) == 0U) {
    delta_x = 0;
    delta_y = 1;
    return;
  }
  delta_x = 0;
  delta_y = 0;
}

void apply_vertical_rule(const DirectionRule &rule, const u8 occupancy_mask,
                         const u32 clearance_mask, i32 &delta_x,
                         i32 &delta_y) noexcept {
  if (clearance_mask == 0U) {
    delta_y = 0;
    return;
  }
  if ((clearance_mask & kClearanceCenter) != 0U &&
      (occupancy_mask & rule.center) == 0U) {
    return;
  }
  if ((clearance_mask & kClearanceNegativeOne) != 0U &&
      (occupancy_mask & rule.side_negative) == 0U) {
    delta_x = -1;
    return;
  }
  if ((clearance_mask & kClearancePositiveOne) != 0U &&
      (occupancy_mask & rule.side_positive) == 0U) {
    delta_x = 1;
    return;
  }
  if ((clearance_mask & kClearanceNegativeFar) != 0U &&
      (occupancy_mask & rule.side_far) == 0U) {
    delta_x = -1;
    delta_y = 0;
    return;
  }
  if ((clearance_mask & kClearancePositiveFar) != 0U &&
      (occupancy_mask & rule.opposite_far) == 0U) {
    delta_x = 1;
    delta_y = 0;
    return;
  }
  delta_x = 0;
  delta_y = 0;
}

void apply_diagonal_rule(const DirectionRule &rule, const u8 occupancy_mask,
                         const bool same_sign, i32 &delta_x,
                         i32 &delta_y) noexcept {
  if ((occupancy_mask & rule.side_far) == 0U) {
    return;
  }
  if ((occupancy_mask & rule.side_negative) == 0U) {
    delta_x = rule.offset;
    delta_y = 0;
    return;
  }
  if ((occupancy_mask & rule.center) == 0U) {
    delta_x = 0;
    delta_y = same_sign ? rule.offset : -rule.offset;
    return;
  }
  delta_x = 0;
  delta_y = 0;
}

} // namespace

LegacyWorldDirectionalOccupancyResult
compute_legacy_world_directional_occupancy_mask(
    const std::span<const u8> surface_grid, const u32 map_width,
    const u32 map_height, const u32 anchor_cell_index,
    const u32 footprint_width, const u32 footprint_height,
    const u32 cell_mask) noexcept {
  LegacyWorldDirectionalOccupancyResult result;
  const SurfaceReader reader{surface_grid, map_width, map_height};
  result.status = reader.validate();
  if (result.status != LegacyWorldDirectionProbeStatus::completed) {
    return result;
  }
  if (footprint_width > kLegacyWorldDirectionMaxFootprint ||
      footprint_height > kLegacyWorldDirectionMaxFootprint) {
    result.status = LegacyWorldDirectionProbeStatus::footprint_out_of_range;
    return result;
  }

  std::array<u32, 32U> samples{};
  const auto anchor = static_cast<std::int64_t>(anchor_cell_index);
  const auto row_stride = static_cast<std::int64_t>(map_width);
  const auto width = static_cast<std::int64_t>(footprint_width);
  const auto height = static_cast<std::int64_t>(footprint_height);
  const std::int64_t upper_left = anchor - row_stride - 1;
  const std::int64_t lower_left = anchor + height * row_stride - 1;

  u32 north_west{};
  u32 north{};
  u32 north_east{};
  u32 south_west{};
  u32 south{};
  u32 south_east{};

  if (footprint_width <= 2U) {
    for (std::size_t offset = 0U; offset < 3U; ++offset) {
      if (!merge_masked_cell(reader,
                             upper_left + static_cast<std::int64_t>(offset),
                             cell_mask, samples[offset]) ||
          !merge_masked_cell(reader,
                             lower_left + static_cast<std::int64_t>(offset),
                             cell_mask, samples[8U + offset])) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
    }
    if (footprint_width == 2U &&
        (!merge_masked_cell(reader, upper_left + 3, cell_mask, samples[3U]) ||
         !merge_masked_cell(reader, lower_left + 3, cell_mask, samples[11U]))) {
      result.status =
          LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
      return result;
    }

    north_west = samples[0U];
    north = samples[1U];
    north_east = samples[2U];
    south_west = samples[8U];
    south = samples[9U];
    south_east = samples[10U];
    if (footprint_width == 2U) {
      north_west |= samples[1U];
      north |= samples[2U];
      north_east |= samples[3U];
      south_west |= samples[9U];
      south |= samples[10U];
      south_east |= samples[11U];
    }
  } else {
    for (u32 offset = 0U; offset < footprint_width + 2U; ++offset) {
      if (!merge_masked_cell(reader, upper_left + offset, cell_mask,
                             samples[offset]) ||
          !merge_masked_cell(reader, lower_left + offset, cell_mask,
                             samples[8U + offset])) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
    }
    for (u32 offset = 0U; offset < footprint_width; ++offset) {
      north_west |= samples[offset];
      north |= samples[offset + 1U];
      north_east |= samples[offset + 2U];
      south_west |= samples[8U + offset];
      south |= samples[9U + offset];
      south_east |= samples[10U + offset];
    }
  }

  const std::int64_t upper_right = anchor + width - row_stride;
  u32 west{};
  u32 east{};
  if (footprint_height <= 2U) {
    for (std::size_t offset = 0U; offset < 3U; ++offset) {
      const auto row_offset = static_cast<std::int64_t>(offset) * row_stride;
      if (!merge_masked_cell(reader, upper_left + row_offset, cell_mask,
                             samples[16U + offset]) ||
          !merge_masked_cell(reader, upper_right + row_offset, cell_mask,
                             samples[24U + offset])) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
    }
    if (footprint_height == 2U &&
        (!merge_masked_cell(reader, upper_left + 3 * row_stride, cell_mask,
                            samples[19U]) ||
         !merge_masked_cell(reader, upper_right + 3 * row_stride, cell_mask,
                            samples[27U]))) {
      result.status =
          LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
      return result;
    }

    north_west |= samples[16U];
    west = samples[17U];
    south_west |= samples[18U];
    north_east |= samples[24U];
    east = samples[25U];
    south_east |= samples[26U];
    if (footprint_height == 2U) {
      north_west |= samples[17U];
      west |= samples[18U];
      south_west |= samples[19U];
      north_east |= samples[25U];
      east |= samples[26U];
      south_east |= samples[27U];
    }
  } else {
    for (u32 offset = 0U; offset < footprint_height + 2U; ++offset) {
      const auto row_offset = static_cast<std::int64_t>(offset) * row_stride;
      if (!merge_masked_cell(reader, upper_left + row_offset, cell_mask,
                             samples[16U + offset]) ||
          !merge_masked_cell(reader, upper_right + row_offset, cell_mask,
                             samples[24U + offset])) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
    }
    for (u32 offset = 0U; offset < footprint_height; ++offset) {
      north_west |= samples[16U + offset];
      west |= samples[17U + offset];
      south_west |= samples[18U + offset];
      north_east |= samples[24U + offset];
      east |= samples[25U + offset];
      south_east |= samples[26U + offset];
    }
  }

  u8 mask{};
  if (north_west != 0U) {
    mask |= 0x01U;
  }
  if (north != 0U) {
    mask |= 0x02U;
  }
  if (north_east != 0U) {
    mask |= 0x04U;
  }
  if (east != 0U) {
    mask |= 0x08U;
  }
  if (south_east != 0U) {
    mask |= 0x10U;
  }
  if (south != 0U) {
    mask |= 0x20U;
  }
  if (south_west != 0U) {
    mask |= 0x40U;
  }
  if (west != 0U) {
    mask |= 0x80U;
  }
  result.status = LegacyWorldDirectionProbeStatus::completed;
  result.mask = mask;
  return result;
}

LegacyWorldSurfaceClearanceResult compute_legacy_world_surface_clearance_mask(
    const std::span<const u8> surface_grid, const u32 map_width,
    const u32 map_height, const i32 start_x, const i32 start_y,
    const u32 span_length, const LegacyWorldClearanceAxis axis) noexcept {
  LegacyWorldSurfaceClearanceResult result;
  const SurfaceReader reader{surface_grid, map_width, map_height};
  result.status = reader.validate();
  if (result.status != LegacyWorldDirectionProbeStatus::completed) {
    return result;
  }

  const std::int64_t row_stride = map_width;
  const std::int64_t cell_step =
      axis == LegacyWorldClearanceAxis::vertical ? row_stride : 1;
  const i32 selected_start =
      axis == LegacyWorldClearanceAxis::vertical ? start_y : start_x;
  const i32 selected_bound = static_cast<i32>(
      axis == LegacyWorldClearanceAxis::vertical ? map_height : map_width);
  const std::int64_t base_index =
      static_cast<std::int64_t>(start_y) * row_stride + start_x;
  std::int64_t negative_start = base_index;
  std::int64_t positive_start = base_index;
  i32 current_selected = selected_start;
  u32 mask{};

  for (u32 distance = 0U; distance <= 2U; ++distance) {
    if (current_selected >= 0) {
      bool clear{};
      if (!scan_clear_stripe(reader, negative_start, cell_step, span_length,
                             clear)) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
      if (clear) {
        mask |= 0x80U >> distance;
      }
    }

    const std::int64_t positive_bound_test =
        static_cast<std::int64_t>(current_selected) +
        static_cast<std::int64_t>(distance) + span_length;
    if (positive_bound_test < selected_bound) {
      bool clear{};
      if (!scan_clear_stripe(reader, positive_start, cell_step, span_length,
                             clear)) {
        result.status =
            LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds;
        return result;
      }
      if (clear) {
        mask |= 0x80U << distance;
      }
    }

    negative_start -= cell_step;
    positive_start += cell_step;
    --current_selected;
  }

  result.status = LegacyWorldDirectionProbeStatus::completed;
  result.mask = mask;
  return result;
}

LegacyWorldDirectionAdjustmentResult
adjust_legacy_world_direction_for_obstacles(
    const LegacyWorldRoleRecord &role, const i32 delta_x, const i32 delta_y,
    const u32 map_width, const u32 map_height,
    const std::span<const u8> surface_grid) noexcept {
  LegacyWorldDirectionAdjustmentResult result{
      .delta_x = delta_x,
      .delta_y = delta_y,
  };
  const auto occupancy = compute_legacy_world_directional_occupancy_mask(
      surface_grid, map_width, map_height, role.map_cell_pointer_32,
      role.action.field_2c, role.action.field_30);
  result.status = occupancy.status;
  result.occupancy_mask = occupancy.mask;
  if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
    return result;
  }

  if (result.delta_x != 0 && result.delta_y == 0) {
    const DirectionRule rule =
        result.delta_x == -1 ? DirectionRule{-1, 2U, 1U, 0x80U, 0x40U, 0x20U}
                             : DirectionRule{
                                   static_cast<i32>(role.action.field_2c),
                                   2U,
                                   4U,
                                   8U,
                                   0x10U,
                                   0x20U,
                               };
    const auto clearance = compute_legacy_world_surface_clearance_mask(
        surface_grid, map_width, map_height,
        wrapping_add_unsigned(role.world_x >> 4U, rule.offset),
        std::bit_cast<i32>(role.world_y >> 4U), role.action.field_30,
        LegacyWorldClearanceAxis::vertical);
    result.status = clearance.status;
    result.surface_clearance_mask = clearance.mask;
    result.used_surface_clearance = true;
    if (clearance.status != LegacyWorldDirectionProbeStatus::completed) {
      return result;
    }
    apply_horizontal_rule(rule, occupancy.mask, clearance.mask, result.delta_x,
                          result.delta_y);
    return result;
  }

  if (result.delta_x == 0 && result.delta_y != 0) {
    const DirectionRule rule = result.delta_y == -1
                                   ? DirectionRule{-1, 0x80U, 1U, 2U, 4U, 8U}
                                   : DirectionRule{
                                         static_cast<i32>(role.action.field_30),
                                         0x80U,
                                         0x40U,
                                         0x20U,
                                         0x10U,
                                         8U,
                                     };
    const auto clearance = compute_legacy_world_surface_clearance_mask(
        surface_grid, map_width, map_height,
        std::bit_cast<i32>(role.world_x >> 4U),
        wrapping_add_unsigned(role.world_y >> 4U, rule.offset),
        role.action.field_2c, LegacyWorldClearanceAxis::horizontal);
    result.status = clearance.status;
    result.surface_clearance_mask = clearance.mask;
    result.used_surface_clearance = true;
    if (clearance.status != LegacyWorldDirectionProbeStatus::completed) {
      return result;
    }
    apply_vertical_rule(rule, occupancy.mask, clearance.mask, result.delta_x,
                        result.delta_y);
    return result;
  }

  const bool same_sign = wrapping_add(result.delta_x, result.delta_y) != 0;
  const DirectionRule rule =
      same_sign
          ? (result.delta_x == -1 ? DirectionRule{-1, 1U, 0x80U, 2U, 0U, 0U}
                                  : DirectionRule{1, 0x10U, 8U, 0x20U, 0U, 0U})
          : (result.delta_x == -1
                 ? DirectionRule{-1, 0x40U, 0x80U, 0x20U, 0U, 0U}
                 : DirectionRule{1, 4U, 8U, 2U, 0U, 0U});
  apply_diagonal_rule(rule, occupancy.mask, same_sign, result.delta_x,
                      result.delta_y);
  result.status = LegacyWorldDirectionProbeStatus::completed;
  return result;
}

} // namespace openswd3::world_map
