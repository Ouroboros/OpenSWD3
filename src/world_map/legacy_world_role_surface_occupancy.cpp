#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <cstddef>
#include <limits>

namespace openswd3::world_map {
namespace {

using compat::u32;
using compat::u8;

constexpr u32 kOrdinaryClearMask = 0xCF7FFFFFU;
constexpr u32 kSelectedClearMask = 0xCF7FFEFFU;
constexpr u32 kOrdinaryMarkMask = 0x10000000U;
constexpr u32 kBlockingMarkMask = 0x30000000U;
constexpr u32 kMarkedRoleFlag = 0x00004000U;
constexpr u32 kMarkedOverlayFlag = 0x00000010U;
constexpr u32 kMarkedOverlayMask = 0x00800000U;
constexpr u32 kSelectedMarkMask = 0x00000100U;
constexpr u32 kRoleMapFlagMask = 0xDF0FFFFFU;
constexpr u32 kCellBit11 = 0x00000800U;
constexpr u32 kMappedBit11 = 0x20000000U;
constexpr u32 kCellNibbleMask = 0x0000F000U;
constexpr u32 kMappedNibbleClearMask = 0xFF0FFFFFU;

[[nodiscard]] u32 read_u32_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
           (static_cast<u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_u32_le(const std::span<u8> bytes, const std::size_t offset,
                  const u32 value) noexcept {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

template <typename Operation>
[[nodiscard]] LegacyWorldRoleSurfaceResult apply_footprint(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& context, const u32 mask,
    Operation operation) noexcept {
    LegacyWorldRoleSurfaceResult result{
        .status = LegacyWorldRoleSurfaceStatus::ready,
        .mask = mask,
    };
    if (context.map_width == 0U || context.surface_grid.empty() ||
        context.surface_grid.size() % sizeof(u32) != 0U) {
        result.status = LegacyWorldRoleSurfaceStatus::invalid_surface_grid;
        return result;
    }

    const auto apply_cell = [&](const std::size_t cell_index) -> bool {
        if (cell_index >
            (std::numeric_limits<std::size_t>::max() - 3U) / 4U) {
            return false;
        }
        const std::size_t offset = cell_index * 4U;
        if (offset > context.surface_grid.size() ||
            context.surface_grid.size() - offset < sizeof(u32)) {
            return false;
        }
        write_u32_le(context.surface_grid, offset,
                     operation(read_u32_le(context.surface_grid, offset),
                               mask));
        ++result.touched_cells;
        return true;
    };

    const std::size_t anchor = role.map_cell_pointer_32;
    if (!apply_cell(anchor)) {
        result.status = LegacyWorldRoleSurfaceStatus::footprint_out_of_range;
        return result;
    }

    const u32 width = role.action.field_2c;
    const u32 height = role.action.field_30;
    if (height == 1U && width == 2U) {
        if (!apply_cell(anchor + 1U)) {
            result.status =
                LegacyWorldRoleSurfaceStatus::footprint_out_of_range;
        }
        return result;
    }
    if (height == 1U && width == 1U) {
        return result;
    }
    if (height == 2U && width == 1U) {
        if (!apply_cell(anchor + context.map_width)) {
            result.status =
                LegacyWorldRoleSurfaceStatus::footprint_out_of_range;
        }
        return result;
    }

    for (u32 row = 0U; row < height; ++row) {
        if (row != 0U &&
            row > (std::numeric_limits<std::size_t>::max() - anchor) /
                      context.map_width) {
            result.status =
                LegacyWorldRoleSurfaceStatus::footprint_out_of_range;
            return result;
        }
        const std::size_t row_anchor =
            anchor + static_cast<std::size_t>(row) * context.map_width;
        for (u32 column = 0U; column < width; ++column) {
            if (column >
                    std::numeric_limits<std::size_t>::max() - row_anchor ||
                !apply_cell(row_anchor + column)) {
                result.status =
                    LegacyWorldRoleSurfaceStatus::footprint_out_of_range;
                return result;
            }
        }
    }
    return result;
}

}  // namespace

LegacyWorldRoleSurfaceResult clear_legacy_world_role_surface_occupancy(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& context) noexcept {
    const u32 mask = static_cast<u32>(role.guid) == context.selected_guid
                         ? kSelectedClearMask
                         : kOrdinaryClearMask;
    return apply_footprint(
        role, context, mask,
        [](const u32 value, const u32 selected_mask) {
            return value & selected_mask;
        });
}

LegacyWorldRoleSurfaceResult mark_legacy_world_role_surface_occupancy(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& context) noexcept {
    u32 mask = (role.flags & kMarkedRoleFlag) != 0U ? kBlockingMarkMask
                                                    : kOrdinaryMarkMask;
    if ((role.flags & kMarkedOverlayFlag) != 0U) {
        mask |= kMarkedOverlayMask;
    }
    if (static_cast<u32>(role.guid) == context.selected_guid) {
        mask |= kSelectedMarkMask;
    }
    return apply_footprint(
        role, context, mask,
        [](const u32 value, const u32 selected_mask) {
            return value | selected_mask;
        });
}

LegacyWorldRoleCellFlagRefreshStatus refresh_legacy_world_role_cell_flags(
    LegacyWorldRoleRecord& role,
    const std::span<const u8> surface_grid) noexcept {
    const std::size_t cell_index = role.map_cell_pointer_32;
    if (cell_index >
        (std::numeric_limits<std::size_t>::max() - 3U) / 4U) {
        return LegacyWorldRoleCellFlagRefreshStatus::cell_out_of_range;
    }
    const std::size_t offset = cell_index * 4U;
    if (offset > surface_grid.size() ||
        surface_grid.size() - offset < sizeof(u32)) {
        return LegacyWorldRoleCellFlagRefreshStatus::cell_out_of_range;
    }

    role.flags &= kRoleMapFlagMask;
    const u32 cell = read_u32_le(surface_grid, offset);
    if ((cell & kCellBit11) != 0U) {
        role.flags |= kMappedBit11;
    }
    const u32 nibble = cell & kCellNibbleMask;
    if (nibble != 0U) {
        role.flags = (role.flags & kMappedNibbleClearMask) | (nibble << 8U);
    }
    return LegacyWorldRoleCellFlagRefreshStatus::ready;
}

}  // namespace openswd3::world_map
