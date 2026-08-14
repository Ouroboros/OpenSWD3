#include "openswd3/world_map/legacy_world_party_role_actions.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kRoleIndexOffset = 0x00U;
constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathBytesOffset = 0x1CU;

constexpr u16 kNoRole = 0xFFFFU;
constexpr u16 kInactivePathCursor = 0x7FFFU;
constexpr u16 kPathCursorFrameGate = 0x8000U;

constexpr std::array<i32, 8U> kCellStepX{
    1,
    0,
    -1,
    -1,
    -1,
    0,
    1,
    1,
};
constexpr std::array<i32, 8U> kCellStepY{
    1,
    1,
    1,
    0,
    -1,
    -1,
    -1,
    0,
};
constexpr std::array<u32, 8U> kDirectionToVariant{
    5U,
    1U,
    6U,
    2U,
    4U,
    0U,
    7U,
    3U,
};

[[nodiscard]] u16 read_u16_le(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16_le(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u32 read_i16_bits(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(read_u16_le(slot, offset)))
    );
}

[[nodiscard]] bool remove_aligned_party_role(
    LegacyWorldPartyRoleActionsResult& result,
    LegacyWorldRoleRecord& role,
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index
) noexcept {
    const auto spatial_result = relocate_legacy_role_spatially_by_guid(
        spatial_index, roles, role.guid, role.flags & 3U, 0, false
    );
    result.spatial_status = spatial_result.status;
    if (result.spatial_status == LegacyRoleSpatialRelocationStatus::ready) {
        return true;
    }
    result.status = LegacyWorldPartyRoleActionsStatus::spatial_removal_failed;
    return false;
}

[[nodiscard]] bool move_surface_occupancy(
    LegacyWorldPartyRoleActionsResult& result,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u8 direction
) noexcept {
    const auto cleared =
        clear_legacy_world_role_surface_occupancy(role, surface_context);
    result.surface_status = cleared.status;
    if (cleared.status != LegacyWorldRoleSurfaceStatus::ready) {
        result.status = LegacyWorldPartyRoleActionsStatus::surface_clear_failed;
        return false;
    }

    role.map_cell_pointer_32 +=
        std::bit_cast<u32>(kCellStepY[direction]) * surface_context.map_width +
        std::bit_cast<u32>(kCellStepX[direction]);

    const auto marked =
        mark_legacy_world_role_surface_occupancy(role, surface_context);
    result.surface_status = marked.status;
    if (marked.status != LegacyWorldRoleSurfaceStatus::ready) {
        result.status = LegacyWorldPartyRoleActionsStatus::surface_mark_failed;
        return false;
    }
    return true;
}

[[nodiscard]] bool refresh_cell_flags(
    LegacyWorldPartyRoleActionsResult& result,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context
) noexcept {
    if ((role.flags & 0x00000100U) == 0U) {
        return true;
    }
    if (refresh_legacy_world_role_cell_flags(
            role, surface_context.surface_grid
        ) == LegacyWorldRoleCellFlagRefreshStatus::ready) {
        return true;
    }
    result.status = LegacyWorldPartyRoleActionsStatus::cell_flag_refresh_failed;
    return false;
}

void update_action(
    LegacyWorldPartyRoleActionsResult& result,
    LegacyWorldRoleRecord& role,
    asset_runtime::LegacyActionDrawPorts& action_ports
) {
    ++result.action_update_count;
    if (action_ports.update_action_record(role.action) !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
        ++result.action_update_failure_count;
    }
}

}  // namespace

LegacyWorldPartyRoleActionsResult advance_legacy_world_party_role_actions(
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 party_role_count,
    const std::span<LegacyWorldObjectSlot> party_object_slots,
    asset_runtime::LegacyActionDrawPorts& action_ports
) {
    LegacyWorldPartyRoleActionsResult result;
    if (party_role_count <= 1U) {
        return result;
    }
    if (party_role_count > party_object_slots.size()) {
        result.status =
            LegacyWorldPartyRoleActionsStatus::invalid_party_role_count;
        return result;
    }

    for (u32 party_index = 1U; party_index < party_role_count; ++party_index) {
        ++result.slots_scanned;
        LegacyWorldObjectSlot& slot = party_object_slots[party_index];
        const u16 role_index_word = read_u16_le(slot, kRoleIndexOffset);
        if (role_index_word == kNoRole) {
            continue;
        }
        ++result.populated_slots;
        const u32 role_index = role_index_word;
        if (role_index >= roles.size()) {
            result.status =
                LegacyWorldPartyRoleActionsStatus::invalid_role_index;
            return result;
        }
        LegacyWorldRoleRecord& role = roles[role_index];

        const u16 path_cursor = read_u16_le(slot, kPathCursorOffset);
        if (path_cursor >= kInactivePathCursor) {
            update_action(result, role, action_ports);
            continue;
        }
        ++result.active_path_slots;

        const std::size_t direction_offset = kPathBytesOffset + path_cursor;
        if (direction_offset >= slot.bytes.size()) {
            result.status =
                LegacyWorldPartyRoleActionsStatus::path_byte_out_of_range;
            return result;
        }
        const u8 direction = slot.bytes[direction_offset];

        if (role.action.wait_remaining == 0U) {
            if (direction >= kDirectionToVariant.size()) {
                result.status =
                    LegacyWorldPartyRoleActionsStatus::direction_out_of_range;
                return result;
            }
            role.action.variant_delta = kDirectionToVariant[direction];
            role.world_x += read_i16_bits(slot, kStepXOffset);
            role.world_y += read_i16_bits(slot, kStepYOffset);
            ++result.roles_moved;

            if (((role.world_x | role.world_y) & 0x0FU) == 0U) {
                ++result.aligned_updates;
                if (!remove_aligned_party_role(
                        result, role, roles, spatial_index
                    ) ||
                    !move_surface_occupancy(
                        result, role, surface_context, direction
                    )) {
                    return result;
                }

                const u16 current_cursor = read_u16_le(slot, kPathCursorOffset);
                write_u16_le(
                    slot,
                    kPathCursorOffset,
                    static_cast<u16>(current_cursor + 1U) | kPathCursorFrameGate
                );
                ++result.cursor_advances;
                if (!refresh_cell_flags(result, role, surface_context)) {
                    return result;
                }
            }
        }

        update_action(result, role, action_ports);
    }
    return result;
}

}  // namespace openswd3::world_map
