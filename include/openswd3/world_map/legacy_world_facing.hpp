#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

namespace openswd3::world_map {

struct LegacyWorldFacingResult {
    compat::u32 distance{};
    compat::u32 angle_degrees{};
    compat::u32 direction{};

    bool operator==(const LegacyWorldFacingResult&) const = default;
};

[[nodiscard]] LegacyWorldFacingResult measure_legacy_world_facing(
    compat::u32 source_x,
    compat::u32 source_y,
    compat::u32 target_x,
    compat::u32 target_y
) noexcept;

// sub_40E030 (0x0040E030..0x0040E07B): derive the controlled role's centre
// from its world position and embedded action extents, then return the legacy
// eight-direction value toward the supplied world point.
[[nodiscard]] compat::u32 measure_legacy_world_controlled_role_direction(
    const LegacyWorldRoleRecord& controlled_role,
    compat::u32 target_x,
    compat::u32 target_y
) noexcept;

}  // namespace openswd3::world_map
