#include "openswd3/world_map/legacy_world_facing_role_query.hpp"

#include "openswd3/world_map/legacy_role_spatial_query.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <limits>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] u32
bounded_role_count(const std::span<const LegacyWorldRoleRecord> roles,
                   const u32 role_count) noexcept {
  constexpr auto kU32Maximum =
      static_cast<std::size_t>(std::numeric_limits<u32>::max());
  const auto representable_size = std::min(roles.size(), kU32Maximum);
  return std::min(role_count, static_cast<u32>(representable_size));
}

[[nodiscard]] bool
query_tile(LegacyWorldFacingRoleQueryResult &result,
           const std::span<const LegacyWorldRoleRecord> roles,
           const u32 role_count, const u32 tile_x, const u32 tile_y) noexcept {
  ++result.tile_query_count;
  result.role_index =
      find_legacy_role_at_tile(roles, role_count, tile_x, tile_y);
  return result.role_index != kLegacyRoleNotFound;
}

[[nodiscard]] u32 limited_scan_extent(const u32 coordinate,
                                      const u32 map_extent) noexcept {
  u32 extent = 5U;
  if (coordinate <= 5U) {
    extent = coordinate;
  }
  if (map_extent - coordinate <= 5U) {
    extent = map_extent - coordinate;
  }
  return extent;
}

[[nodiscard]] bool
scan_north_west(LegacyWorldFacingRoleQueryResult &result,
                const std::span<const LegacyWorldRoleRecord> roles,
                const u32 role_count, const u32 tile_x, const u32 tile_y,
                const u32 x_extent, const u32 y_extent) noexcept {
  for (u32 y = 1U; y <= y_extent; ++y) {
    for (u32 x = 1U; x <= x_extent; ++x) {
      if (query_tile(result, roles, role_count, tile_x - x, tile_y - y)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool
scan_north(LegacyWorldFacingRoleQueryResult &result,
           const std::span<const LegacyWorldRoleRecord> roles,
           const u32 role_count, const u32 tile_x, const u32 tile_y,
           const u32 player_width, const u32 y_extent) noexcept {
  const u32 width = player_width + 2U;
  for (u32 y = 1U; y <= y_extent; ++y) {
    for (u32 x = 0U; x < width; ++x) {
      if (query_tile(result, roles, role_count, tile_x + x - 1U, tile_y - y)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool
scan_north_east(LegacyWorldFacingRoleQueryResult &result,
                const std::span<const LegacyWorldRoleRecord> roles,
                const u32 role_count, const u32 tile_x, const u32 tile_y,
                const u32 x_extent, const u32 y_extent) noexcept {
  for (u32 y = 1U; y <= y_extent; ++y) {
    for (u32 x = 1U; x < x_extent; ++x) {
      if (query_tile(result, roles, role_count, tile_x + x, tile_y - y)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool scan_west(LegacyWorldFacingRoleQueryResult &result,
                             const std::span<const LegacyWorldRoleRecord> roles,
                             const u32 role_count, const u32 tile_x,
                             const u32 tile_y, const u32 player_height,
                             const u32 x_extent) noexcept {
  const u32 height = player_height + 2U;
  for (u32 x = 1U; x <= x_extent; ++x) {
    for (u32 y = 0U; y < height; ++y) {
      if (query_tile(result, roles, role_count, tile_x - x, tile_y + y - 1U)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool scan_east(LegacyWorldFacingRoleQueryResult &result,
                             const std::span<const LegacyWorldRoleRecord> roles,
                             const u32 role_count, const u32 tile_x,
                             const u32 tile_y, const u32 player_width,
                             const u32 player_height,
                             const u32 x_extent) noexcept {
  const u32 height = player_height + 2U;
  u32 candidate_x = tile_x + player_width;
  for (u32 distance = 0U; distance < x_extent; ++distance, ++candidate_x) {
    for (u32 y = 0U; y < height; ++y) {
      if (query_tile(result, roles, role_count, candidate_x, tile_y + y - 1U)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool
scan_south_west(LegacyWorldFacingRoleQueryResult &result,
                const std::span<const LegacyWorldRoleRecord> roles,
                const u32 role_count, const u32 tile_x, const u32 tile_y,
                const u32 x_extent, const u32 y_extent) noexcept {
  for (u32 y = 1U; y < y_extent; ++y) {
    for (u32 x = 1U; x <= x_extent; ++x) {
      if (query_tile(result, roles, role_count, tile_x - x, tile_y + y)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool
scan_south(LegacyWorldFacingRoleQueryResult &result,
           const std::span<const LegacyWorldRoleRecord> roles,
           const u32 role_count, const u32 tile_x, const u32 tile_y,
           const u32 player_width, const u32 player_height,
           const u32 y_extent) noexcept {
  const u32 width = player_width + 2U;
  u32 candidate_y = tile_y + player_height;
  for (u32 distance = 0U; distance < y_extent; ++distance, ++candidate_y) {
    for (u32 x = 0U; x < width; ++x) {
      if (query_tile(result, roles, role_count, tile_x + x - 1U, candidate_y)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool
scan_south_east(LegacyWorldFacingRoleQueryResult &result,
                const std::span<const LegacyWorldRoleRecord> roles,
                const u32 role_count, const u32 tile_x, const u32 tile_y,
                const u32 x_extent, const u32 y_extent) noexcept {
  for (u32 y = 1U; y < y_extent; ++y) {
    for (u32 x = 1U; x < x_extent; ++x) {
      if (query_tile(result, roles, role_count, tile_x + x, tile_y + y)) {
        return true;
      }
    }
  }
  return false;
}

void scan_player_footprint(LegacyWorldFacingRoleQueryResult &result,
                           const std::span<const LegacyWorldRoleRecord> roles,
                           const u32 role_count, const u32 tile_x,
                           const u32 tile_y, const u32 player_width,
                           const u32 player_height) noexcept {
  for (u32 y = 0U; y < player_height; ++y) {
    for (u32 x = 0U; x < player_width; ++x) {
      if (query_tile(result, roles, role_count, tile_x + x, tile_y + y)) {
        return;
      }
    }
  }
  result.role_index = kLegacyWorldFacingRoleNotFound;
}

} // namespace

LegacyWorldFacingRoleQueryResult find_legacy_world_facing_role(
    const std::span<const LegacyWorldRoleRecord> roles, const u32 role_count,
    const u32 player_index, const i32 delta_x, const i32 delta_y,
    const u32 map_width, const u32 map_height) noexcept {
  LegacyWorldFacingRoleQueryResult result;
  const u32 scan_role_count = bounded_role_count(roles, role_count);
  if (player_index >= scan_role_count) {
    result.status = LegacyWorldFacingRoleQueryStatus::invalid_player_index;
    return result;
  }

  const LegacyWorldRoleRecord &player = roles[player_index];
  const u32 tile_x = player.world_x >> 4U;
  const u32 tile_y = player.world_y >> 4U;
  const u32 player_width = player.action.field_2c;
  const u32 player_height = player.action.field_30;
  const u32 x_extent = limited_scan_extent(tile_x, map_width);
  const u32 y_extent = limited_scan_extent(tile_y, map_height);
  const u32 selector =
      std::bit_cast<u32>(delta_x) + std::bit_cast<u32>(delta_y) * 3U + 4U;

  bool found = false;
  switch (selector) {
  case 0U:
    found = scan_north_west(result, roles, scan_role_count, tile_x, tile_y,
                            x_extent, y_extent);
    break;
  case 1U:
    found = scan_north(result, roles, scan_role_count, tile_x, tile_y,
                       player_width, y_extent);
    break;
  case 2U:
    found = scan_north_east(result, roles, scan_role_count, tile_x, tile_y,
                            x_extent, y_extent);
    break;
  case 3U:
    found = scan_west(result, roles, scan_role_count, tile_x, tile_y,
                      player_height, x_extent);
    break;
  case 5U:
    found = scan_east(result, roles, scan_role_count, tile_x, tile_y,
                      player_width, player_height, x_extent);
    break;
  case 6U:
    found = scan_south_west(result, roles, scan_role_count, tile_x, tile_y,
                            x_extent, y_extent);
    break;
  case 7U:
    found = scan_south(result, roles, scan_role_count, tile_x, tile_y,
                       player_width, player_height, y_extent);
    break;
  case 8U:
    found = scan_south_east(result, roles, scan_role_count, tile_x, tile_y,
                            x_extent, y_extent);
    break;
  default:
    break;
  }

  if (!found) {
    scan_player_footprint(result, roles, scan_role_count, tile_x, tile_y,
                          player_width, player_height);
  }
  return result;
}

} // namespace openswd3::world_map
