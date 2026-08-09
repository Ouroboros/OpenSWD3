#include "openswd3/world_map/legacy_role_spatial_query.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::u32 bounded_role_count(
    const std::span<const LegacyWorldRoleRecord> roles,
    const compat::u32 role_count
) noexcept {
    constexpr auto kU32Maximum =
        static_cast<std::size_t>(std::numeric_limits<compat::u32>::max());
    const auto representable_size = std::min(roles.size(), kU32Maximum);
    return std::min(role_count, static_cast<compat::u32>(representable_size));
}

}  // namespace

compat::u32 find_legacy_role_at_tile(
    const std::span<const LegacyWorldRoleRecord> roles,
    const compat::u32 role_count,
    const compat::u32 tile_x,
    const compat::u32 tile_y
) noexcept {
    const compat::u32 scan_count = bounded_role_count(roles, role_count);
    if (scan_count <= 1U) {
        return kLegacyRoleNotFound;
    }

    for (compat::u32 role_index = 1U; role_index < scan_count; ++role_index) {
        const auto& role = roles[role_index];
        if (role.talk_script_id == 0U ||
            (role.flags & kLegacyRoleSpatiallyActiveFlag) == 0U) {
            continue;
        }

        const compat::u32 role_tile_x = role.world_x >> 4U;
        const compat::u32 role_tile_y = role.world_y >> 4U;
        if (role_tile_x > tile_x) {
            continue;
        }

        const compat::u32 end_tile_x = role_tile_x + role.action.field_2c;
        if (end_tile_x <= tile_x || role_tile_y > tile_y) {
            continue;
        }

        const compat::u32 end_tile_y = role_tile_y + role.action.field_30;
        if (end_tile_y > tile_y) {
            return role_index;
        }
    }

    return kLegacyRoleNotFound;
}

compat::u32 find_legacy_collision_role_at_tile(
    const std::span<const LegacyWorldRoleRecord> roles,
    const compat::u32 role_count,
    const compat::u32 tile_x,
    const compat::u32 tile_y
) noexcept {
    const compat::u32 role_index =
        find_legacy_role_at_tile(roles, role_count, tile_x, tile_y);
    if (role_index == kLegacyRoleNotFound ||
        (roles[role_index].flags & kLegacyRoleCollisionFlag) == 0U) {
        return kLegacyRoleNotFound;
    }
    return role_index;
}

}  // namespace openswd3::world_map
