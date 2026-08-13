#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldFacingRoleNotFound = 0xFFFFFFFFU;

enum class LegacyWorldFacingRoleQueryStatus {
    completed,
    invalid_player_index,
};

struct LegacyWorldFacingRoleQueryResult {
    LegacyWorldFacingRoleQueryStatus status{
        LegacyWorldFacingRoleQueryStatus::completed
    };
    compat::u32 role_index{kLegacyWorldFacingRoleNotFound};
    compat::u32 tile_query_count{};
};

// sub_404C00 (0x00404C00..0x00404F9D): scan for a Talk-capable role in
// the player's requested direction, then fall back to the player's current
// footprint. The directional scan order and unsigned boundary arithmetic are
// part of the original behavior.
[[nodiscard]] LegacyWorldFacingRoleQueryResult find_legacy_world_facing_role(
    std::span<const LegacyWorldRoleRecord> roles,
    compat::u32 role_count,
    compat::u32 player_index,
    compat::i32 delta_x,
    compat::i32 delta_y,
    compat::u32 map_width,
    compat::u32 map_height
) noexcept;

}  // namespace openswd3::world_map
