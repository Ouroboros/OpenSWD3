#include "openswd3/world_map/legacy_movement_collision.hpp"

#include "openswd3/world_map/legacy_role_spatial_query.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

[[nodiscard]] bool probe_collision_cell(
    const compat::u32 cell_index,
    const compat::u32 tile_x,
    const compat::u32 tile_y,
    LegacyMovementCollisionPorts& ports,
    LegacyMovementCollisionResult& result
) {
    const std::optional<compat::u32> cell = ports.read_map_cell(cell_index);
    if (!cell.has_value()) {
        result.status = LegacyMovementCollisionStatus::invalid_map_cell;
        return false;
    }

    result.event_code = *cell & 0xFFU;
    if (result.event_code != 0U) {
        return false;
    }

    result.hit_role_index = ports.find_collision_role_at_tile(tile_x, tile_y);
    return result.hit_role_index == kLegacyMovementCollisionNoRole;
}

[[nodiscard]] bool scan_horizontal_edge(
    compat::u32 cell_index,
    const compat::u32 tile_x,
    const compat::u32 tile_y,
    const compat::u32 count,
    LegacyMovementCollisionPorts& ports,
    LegacyMovementCollisionResult& result
) {
    for (compat::u32 offset = 0U; offset < count; ++offset) {
        if (!probe_collision_cell(
                cell_index,
                tile_x + offset,
                tile_y,
                ports,
                result
            )) {
            return false;
        }
        ++cell_index;
    }
    return true;
}

[[nodiscard]] bool scan_vertical_edge(
    compat::u32 cell_index,
    const compat::u32 tile_x,
    const compat::u32 tile_y,
    const compat::u32 count,
    const compat::u32 legacy_cell_step,
    LegacyMovementCollisionPorts& ports,
    LegacyMovementCollisionResult& result
) {
    for (compat::u32 offset = 0U; offset < count; ++offset) {
        if (!probe_collision_cell(
                cell_index,
                tile_x,
                tile_y + offset,
                ports,
                result
            )) {
            return false;
        }
        cell_index += legacy_cell_step;
    }
    return true;
}

class SessionMovementCollisionPorts final
    : public LegacyMovementCollisionPorts {
public:
    SessionMovementCollisionPorts(
        const std::span<const LegacyWorldRoleRecord> roles,
        const compat::u32 role_count,
        const std::span<const compat::u8> map_cells
    ) noexcept
        : roles_(roles), role_count_(role_count), map_cells_(map_cells) {}

    std::optional<compat::u32> read_map_cell(
        const compat::u32 cell_index
    ) override {
        const std::uint64_t byte_offset =
            static_cast<std::uint64_t>(cell_index) * 4U;
        if (byte_offset > map_cells_.size() ||
            map_cells_.size() - static_cast<std::size_t>(byte_offset) < 4U) {
            return std::nullopt;
        }

        const std::size_t offset = static_cast<std::size_t>(byte_offset);
        return static_cast<compat::u32>(map_cells_[offset]) |
            (static_cast<compat::u32>(map_cells_[offset + 1U]) << 8U) |
            (static_cast<compat::u32>(map_cells_[offset + 2U]) << 16U) |
            (static_cast<compat::u32>(map_cells_[offset + 3U]) << 24U);
    }

    compat::u32 find_collision_role_at_tile(
        const compat::u32 tile_x,
        const compat::u32 tile_y
    ) override {
        return find_legacy_collision_role_at_tile(
            roles_,
            role_count_,
            tile_x,
            tile_y
        );
    }

private:
    std::span<const LegacyWorldRoleRecord> roles_;
    compat::u32 role_count_{};
    std::span<const compat::u8> map_cells_;
};

}  // namespace

LegacyMovementCollisionResult check_legacy_movement_collision(
    const LegacyWorldRoleRecord& role,
    const compat::i32 delta_x,
    const compat::i32 delta_y,
    const compat::u32 map_row_stride,
    LegacyMovementCollisionPorts& ports
) {
    LegacyMovementCollisionResult result;

    const compat::u32 tile_x = role.world_x >> 4U;
    const compat::u32 tile_y = role.world_y >> 4U;
    const compat::u32 width = role.action.field_2c;
    const compat::u32 height = role.action.field_30;
    const compat::u32 anchor = role.map_cell_pointer_32;
    const compat::u32 selector =
        std::bit_cast<compat::u32>(delta_x) +
        std::bit_cast<compat::u32>(delta_y) * 3U + 4U;

    switch (selector) {
    case 0U:
        if (!scan_horizontal_edge(
                anchor - map_row_stride - 1U,
                tile_x - 1U,
                tile_y - 1U,
                width,
                ports,
                result
            )) {
            return result;
        }
        static_cast<void>(scan_vertical_edge(
            anchor - map_row_stride - 1U,
            tile_x - 1U,
            tile_y - 1U,
            height,
            width,
            ports,
            result
        ));
        return result;

    case 1U:
        static_cast<void>(scan_horizontal_edge(
            anchor - map_row_stride,
            tile_x,
            tile_y - 1U,
            width,
            ports,
            result
        ));
        return result;

    case 2U:
        if (!scan_horizontal_edge(
                anchor - map_row_stride + 1U,
                tile_x + 1U,
                tile_y - 1U,
                width,
                ports,
                result
            )) {
            return result;
        }
        static_cast<void>(scan_vertical_edge(
            anchor + width - map_row_stride,
            tile_x + width + 1U,
            tile_y - 1U,
            height,
            width,
            ports,
            result
        ));
        return result;

    case 3U:
        static_cast<void>(scan_vertical_edge(
            anchor - 1U,
            tile_x - 1U,
            tile_y,
            height,
            width,
            ports,
            result
        ));
        return result;

    case 4U:
        return result;

    case 5U:
        static_cast<void>(scan_vertical_edge(
            anchor + width,
            tile_x + width,
            tile_y,
            height,
            width,
            ports,
            result
        ));
        return result;

    case 6U:
        if (!scan_horizontal_edge(
                anchor + map_row_stride * height - 1U,
                tile_x - 1U,
                tile_y + height,
                width,
                ports,
                result
            )) {
            return result;
        }
        static_cast<void>(scan_vertical_edge(
            anchor + map_row_stride - 1U,
            tile_x - 1U,
            tile_y + 1U,
            height,
            width,
            ports,
            result
        ));
        return result;

    case 7U:
        static_cast<void>(scan_horizontal_edge(
            anchor + map_row_stride * height,
            tile_x,
            tile_y + height,
            width,
            ports,
            result
        ));
        return result;

    case 8U:
        if (!scan_horizontal_edge(
                anchor + map_row_stride * height + 1U,
                tile_x + 1U,
                tile_y + height,
                width,
                ports,
                result
            )) {
            return result;
        }
        static_cast<void>(scan_vertical_edge(
            anchor + width,
            tile_x + width,
            tile_y + 1U,
            height,
            width,
            ports,
            result
        ));
        return result;

    default:
        return result;
    }
}

LegacyMovementCollisionResult check_legacy_movement_collision(
    const std::span<const LegacyWorldRoleRecord> roles,
    const compat::u32 role_count,
    const compat::u32 role_index,
    const compat::i32 delta_x,
    const compat::i32 delta_y,
    const std::span<const compat::u8> map_cells,
    const compat::u32 map_row_stride
) {
    const std::size_t bounded_count = std::min(
        roles.size(),
        static_cast<std::size_t>(role_count)
    );
    if (role_index >= bounded_count) {
        return LegacyMovementCollisionResult{
            .status = LegacyMovementCollisionStatus::invalid_role,
        };
    }

    SessionMovementCollisionPorts ports{roles, role_count, map_cells};
    return check_legacy_movement_collision(
        roles[role_index],
        delta_x,
        delta_y,
        map_row_stride,
        ports
    );
}

}  // namespace openswd3::world_map
