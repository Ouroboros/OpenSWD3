#pragma once

#include "openswd3/compat/types.hpp"

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

}  // namespace openswd3::world_map
