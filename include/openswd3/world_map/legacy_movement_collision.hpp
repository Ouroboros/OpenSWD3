#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <optional>
#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyMovementCollisionNoRole = 0xFFFFFFFFU;

enum class LegacyMovementCollisionStatus {
    completed,
    invalid_role,
    invalid_map_cell,
};

struct LegacyMovementCollisionResult {
    LegacyMovementCollisionStatus status{
        LegacyMovementCollisionStatus::completed
    };
    compat::u32 event_code{0xFFFFFFFFU};
    compat::u32 hit_role_index{kLegacyMovementCollisionNoRole};
};

class LegacyMovementCollisionPorts {
public:
    virtual ~LegacyMovementCollisionPorts() = default;

    [[nodiscard]] virtual std::optional<compat::u32> read_map_cell(
        compat::u32 cell_index
    ) = 0;

    [[nodiscard]] virtual compat::u32 find_collision_role_at_tile(
        compat::u32 tile_x,
        compat::u32 tile_y
    ) = 0;
};

[[nodiscard]] LegacyMovementCollisionResult
check_legacy_movement_collision(
    const LegacyWorldRoleRecord& role,
    compat::i32 delta_x,
    compat::i32 delta_y,
    compat::u32 map_row_stride,
    LegacyMovementCollisionPorts& ports
);

[[nodiscard]] LegacyMovementCollisionResult
check_legacy_movement_collision(
    std::span<const LegacyWorldRoleRecord> roles,
    compat::u32 role_count,
    compat::u32 role_index,
    compat::i32 delta_x,
    compat::i32 delta_y,
    std::span<const compat::u8> map_cells,
    compat::u32 map_row_stride
);

}  // namespace openswd3::world_map
