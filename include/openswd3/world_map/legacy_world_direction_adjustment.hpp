#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldDirectionOccupancyCellMask =
    0x60800000U;
inline constexpr compat::u32 kLegacyWorldDirectionMaxFootprint = 6U;

enum class LegacyWorldClearanceAxis {
  horizontal,
  vertical,
};

enum class LegacyWorldDirectionProbeStatus {
  completed,
  invalid_map_dimensions,
  invalid_surface_grid,
  footprint_out_of_range,
  surface_access_out_of_bounds,
};

struct LegacyWorldDirectionalOccupancyResult {
  LegacyWorldDirectionProbeStatus status{
      LegacyWorldDirectionProbeStatus::invalid_map_dimensions};
  compat::u8 mask{};
};

struct LegacyWorldSurfaceClearanceResult {
  LegacyWorldDirectionProbeStatus status{
      LegacyWorldDirectionProbeStatus::invalid_map_dimensions};
  compat::u32 mask{};
};

struct LegacyWorldDirectionAdjustmentResult {
  LegacyWorldDirectionProbeStatus status{
      LegacyWorldDirectionProbeStatus::invalid_map_dimensions};
  compat::i32 delta_x{};
  compat::i32 delta_y{};
  compat::u8 occupancy_mask{};
  compat::u32 surface_clearance_mask{};
  bool used_surface_clearance{};
};

[[nodiscard]] LegacyWorldDirectionalOccupancyResult
compute_legacy_world_directional_occupancy_mask(
    std::span<const compat::u8> surface_grid, compat::u32 map_width,
    compat::u32 map_height, compat::u32 anchor_cell_index,
    compat::u32 footprint_width, compat::u32 footprint_height,
    compat::u32 cell_mask = kLegacyWorldDirectionOccupancyCellMask) noexcept;

[[nodiscard]] LegacyWorldSurfaceClearanceResult
compute_legacy_world_surface_clearance_mask(
    std::span<const compat::u8> surface_grid, compat::u32 map_width,
    compat::u32 map_height, compat::i32 start_x, compat::i32 start_y,
    compat::u32 span_length, LegacyWorldClearanceAxis axis) noexcept;

[[nodiscard]] LegacyWorldDirectionAdjustmentResult
adjust_legacy_world_direction_for_obstacles(
    const LegacyWorldRoleRecord &role, compat::i32 delta_x, compat::i32 delta_y,
    compat::u32 map_width, compat::u32 map_height,
    std::span<const compat::u8> surface_grid) noexcept;

} // namespace openswd3::world_map
