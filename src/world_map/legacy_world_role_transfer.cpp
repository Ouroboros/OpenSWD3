#include "openswd3/world_map/legacy_world_role_transfer.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kObjectRoleIndexOffset = 0x00U;
constexpr std::size_t kObjectPathCursorOffset = 0x02U;
constexpr std::size_t kObjectPathDataOffset = 0x1CU;
constexpr u16 kObjectPathCursorMask = 0x7FFFU;
constexpr u32 kRoleTransferFlag = 0x00000080U;
constexpr u32 kRolePartyClearFlag = 0x00004000U;

constexpr std::array<i32, 8U> kDirectionStepX{
    4, 0, -4, -4, -4, 0, 4, 4,
};
constexpr std::array<i32, 8U> kDirectionStepY{
    4, 4, 4, 0, -4, -4, -4, 0,
};

[[nodiscard]] u16 read_u16_le(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] LegacyWorldObjectSlot* find_active_object_slot(
    const std::span<LegacyWorldObjectSlot> slots,
    const u32 role_index
) noexcept {
    const auto exact_slots = slots.first(kLegacyWorldActiveObjectSlotCount);
    const u16 target = static_cast<u16>(role_index);
    const auto found = std::ranges::find_if(
        exact_slots,
        [target](const LegacyWorldObjectSlot& slot) {
            return read_u16_le(slot.bytes, kObjectRoleIndexOffset) == target;
        }
    );
    return found == exact_slots.end() ? nullptr : &*found;
}

[[nodiscard]] LegacyWorldRoleTransferStatus clear_role_surface_occupancy(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleTransferContext& context,
    LegacyWorldRoleTransferState& state
) noexcept {
    if (context.surface_grid.empty()) {
        return LegacyWorldRoleTransferStatus::surface_grid_required;
    }
    const auto result = clear_legacy_world_role_surface_occupancy(
        role,
        LegacyWorldRoleSurfaceContext{
            .map_width = context.map_width,
            .selected_guid = context.selected_guid,
            .surface_grid = context.surface_grid,
        }
    );
    state.cleared_surface_cells += result.touched_cells;
    if (result.status == LegacyWorldRoleSurfaceStatus::invalid_surface_grid) {
        return LegacyWorldRoleTransferStatus::surface_grid_invalid;
    }
    if (result.status ==
        LegacyWorldRoleSurfaceStatus::footprint_out_of_range) {
        return LegacyWorldRoleTransferStatus::surface_footprint_out_of_range;
    }
    return LegacyWorldRoleTransferStatus::ready;
}

[[nodiscard]] LegacyWorldRoleTransferStatus align_role_to_tile(
    LegacyWorldRoleRecord& role,
    const LegacyWorldObjectSlot& slot,
    const LegacyWorldRoleTransferContext& context,
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyWorldRoleTransferState& state
) noexcept {
    const u16 cursor = static_cast<u16>(
        read_u16_le(slot.bytes, kObjectPathCursorOffset) &
        kObjectPathCursorMask
    );
    const std::size_t direction_offset = kObjectPathDataOffset + cursor;
    if (direction_offset >= slot.bytes.size()) {
        return LegacyWorldRoleTransferStatus::path_cursor_out_of_range;
    }
    const u8 direction = slot.bytes[direction_offset];
    if (direction >= kDirectionStepX.size()) {
        return LegacyWorldRoleTransferStatus::direction_out_of_range;
    }

    const i32 step_x = kDirectionStepX[direction];
    if ((role.world_x & 0x0FU) != 0U &&
        (step_x == 0 || (role.world_x & 3U) != 0U)) {
        return LegacyWorldRoleTransferStatus::direction_cannot_align;
    }
    while ((role.world_x & 0x0FU) != 0U) {
        role.world_x -= static_cast<u32>(step_x);
    }

    const i32 step_y = kDirectionStepY[direction];
    if ((role.world_y & 0x0FU) != 0U &&
        (step_y == 0 || (role.world_y & 3U) != 0U)) {
        return LegacyWorldRoleTransferStatus::direction_cannot_align;
    }
    while ((role.world_y & 0x0FU) != 0U) {
        role.world_y -= static_cast<u32>(step_y);
    }

    if (context.spatial_index == nullptr) {
        return LegacyWorldRoleTransferStatus::spatial_index_required;
    }
    const u32 first_row_bits = (role.world_y >> 4U) - 1U;
    const auto relocation_status = relocate_legacy_role_spatially_by_guid(
        *context.spatial_index,
        roles,
        role.guid,
        role.flags & 3U,
        std::bit_cast<i32>(first_row_bits),
        true
    );
    if (relocation_status != LegacyRoleSpatialRelocationStatus::ready) {
        return LegacyWorldRoleTransferStatus::spatial_relocation_failed;
    }

    ++state.aligned_roles;
    ++state.spatial_roles_relocated;
    return LegacyWorldRoleTransferStatus::ready;
}

}  // namespace

LegacyWorldRoleTransferStatus transfer_legacy_world_role(
    const std::span<u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const LegacyWorldRoleTransferContext* const context,
    LegacyWorldRoleTransferState& state
) noexcept {
    if (role_index == 0U || role_index >= roles.size()) {
        return LegacyWorldRoleTransferStatus::role_index_out_of_range;
    }
    LegacyWorldRoleRecord& role = roles[role_index];
    if (role.path_data_id != 0U) {
        if (context == nullptr ||
            context->active_object_slots.size() <
                kLegacyWorldActiveObjectSlotCount) {
            return LegacyWorldRoleTransferStatus::active_object_slots_required;
        }

        LegacyWorldObjectSlot* const slot = find_active_object_slot(
            context->active_object_slots,
            role_index
        );
        if (slot != nullptr) {
            if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
                auto status = clear_role_surface_occupancy(
                    role,
                    *context,
                    state
                );
                if (status != LegacyWorldRoleTransferStatus::ready) {
                    return status;
                }
                status = align_role_to_tile(
                    role,
                    *slot,
                    *context,
                    roles,
                    state
                );
                if (status != LegacyWorldRoleTransferStatus::ready) {
                    return status;
                }
            }

            const auto patch_status = patch_legacy_maps_role_source_record(
                maps_payload,
                maps_database,
                LegacyMapsRolePatchRequest{
                    .guid = role.guid,
                    .flags_or_mask = 0x0080U,
                    .flags_and_mask = 0xFFFFU,
                }
            );
            if (patch_status != LegacyMapsRolePatchStatus::ready) {
                return LegacyWorldRoleTransferStatus::role_source_patch_failed;
            }

            slot->bytes.fill(0xFFU);
            ++state.active_object_slots_reset;
        }
    }

    if (state.party_role_count >= kLegacyWorldPartySlotCount) {
        return LegacyWorldRoleTransferStatus::party_capacity_exceeded;
    }

    const std::size_t party_index = state.party_role_count;
    state.party_role_indices[party_index] = role_index;
    state.party_object_slots[party_index].bytes.fill(0xFFU);
    role.talk_script_id = 0U;
    role.flags = (role.flags & ~kRolePartyClearFlag) | kRoleTransferFlag;
    ++state.party_role_count;
    ++state.roles_transferred;
    return LegacyWorldRoleTransferStatus::ready;
}

}  // namespace openswd3::world_map
