#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyRoleNotFound = 0xFFFFFFFFU;
inline constexpr compat::u32 kLegacyRoleSpatiallyActiveFlag = 0x00008000U;
inline constexpr compat::u32 kLegacyRoleCollisionFlag = 0x00002000U;

[[nodiscard]] compat::u32 find_legacy_role_at_tile(
    std::span<const LegacyWorldRoleRecord> roles,
    compat::u32 role_count,
    compat::u32 tile_x,
    compat::u32 tile_y
) noexcept;

[[nodiscard]] compat::u32 find_legacy_collision_role_at_tile(
    std::span<const LegacyWorldRoleRecord> roles,
    compat::u32 role_count,
    compat::u32 tile_x,
    compat::u32 tile_y
) noexcept;

}  // namespace openswd3::world_map
