#include "openswd3/world_map/legacy_world_role_map_update.hpp"

#include <array>
#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u16 kObjectPathCursorMask = 0x7FFFU;
constexpr std::size_t kObjectPathCursorOffset = 0x02U;
constexpr std::size_t kObjectPathDataOffset = 0x1CU;

constexpr std::array<i32, 8U> kDirectionStepX{
    4,
    0,
    -4,
    -4,
    -4,
    0,
    4,
    4,
};
constexpr std::array<i32, 8U> kDirectionStepY{
    4,
    4,
    4,
    0,
    -4,
    -4,
    -4,
    0,
};

[[nodiscard]] u16 read_u16_le(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] bool align_axis(u32& value, const i32 step) noexcept {
    if ((value & 0x0FU) == 0U) {
        return true;
    }
    if (step == 0 || (value & 3U) != 0U) {
        return false;
    }
    while ((value & 0x0FU) != 0U) {
        value += static_cast<u32>(step);
    }
    return true;
}

[[nodiscard]] LegacyWorldRoleMapUpdateStatus align_party_role(
    LegacyWorldRoleRecord& role,
    const LegacyWorldObjectSlot& slot,
    const LegacyWorldRoleMapUpdateContext& context,
    LegacyWorldRoleMapUpdateResult& result
) noexcept {
    const auto cleared =
        clear_legacy_world_role_surface_occupancy(role, context.role_surface);
    if (cleared.status != LegacyWorldRoleSurfaceStatus::ready) {
        return LegacyWorldRoleMapUpdateStatus::role_surface_failed;
    }
    result.role_surface_cleared = true;

    const u16 cursor = static_cast<u16>(
        read_u16_le(slot.bytes, kObjectPathCursorOffset) & kObjectPathCursorMask
    );
    const std::size_t direction_offset = kObjectPathDataOffset + cursor;
    if (direction_offset >= slot.bytes.size()) {
        return LegacyWorldRoleMapUpdateStatus::path_direction_out_of_range;
    }
    const u8 direction = slot.bytes[direction_offset];
    if (direction != 0xFFU) {
        if (direction >= kDirectionStepX.size()) {
            return LegacyWorldRoleMapUpdateStatus::path_direction_out_of_range;
        }
        if (!align_axis(role.world_x, kDirectionStepX[direction]) ||
            !align_axis(role.world_y, kDirectionStepY[direction])) {
            return LegacyWorldRoleMapUpdateStatus::path_direction_cannot_align;
        }
        result.coordinates_aligned = true;
    }
    if (context.spatial_index == nullptr) {
        return LegacyWorldRoleMapUpdateStatus::role_spatial_relocation_failed;
    }
    const u32 first_row_bits = (role.world_y >> 4U) - 1U;
    const auto spatial_status = relocate_legacy_role_spatially_by_guid(
        *context.spatial_index,
        context.roles,
        role.guid,
        role.flags & 3U,
        std::bit_cast<i32>(first_row_bits),
        true
    );
    if (spatial_status != LegacyRoleSpatialRelocationStatus::ready) {
        return LegacyWorldRoleMapUpdateStatus::role_spatial_relocation_failed;
    }
    result.spatial_role_relocated = true;
    return LegacyWorldRoleMapUpdateStatus::ready;
}

[[nodiscard]] LegacyMapsRolePatchStatus patch_maps_source(
    const LegacyWorldRoleMapUpdateRequest& request,
    const LegacyWorldRoleMapUpdateContext& context,
    const u16 guid
) noexcept {
    return patch_legacy_maps_role_source_record(
        context.maps_payload,
        *context.maps_database,
        LegacyMapsRolePatchRequest{
            .guid = guid,
            .talk_script_id = request.talk_script_id,
            .path_data_id = request.path_data_id,
            .flags_or_mask = 0U,
            .flags_and_mask = 0xFF7FU,
        }
    );
}

}  // namespace

LegacyWorldRoleMapUpdateResult apply_legacy_world_role_map_update(
    const LegacyWorldRoleMapUpdateRequest& request,
    const LegacyWorldRoleMapUpdateContext& context
) noexcept {
    LegacyWorldRoleMapUpdateResult result;
    if (context.maps_database == nullptr ||
        context.party_role_count == nullptr) {
        result.status = LegacyWorldRoleMapUpdateStatus::maps_runtime_required;
        return result;
    }
    u32 role_index = 0U;
    const bool role_found = resolve_legacy_world_role_selector(
        context.roles,
        request.role_selector,
        context.controlled_role_index,
        role_index
    );
    result.resolved_role_index = role_index;
    result.runtime_role_found = role_found;

    if (!role_found) {
        const auto patch_status = patch_legacy_maps_role_source_record(
            context.maps_payload,
            *context.maps_database,
            LegacyMapsRolePatchRequest{
                .guid = request.role_selector,
                .flags_and_mask = 0xFF7FU,
            }
        );
        if (patch_status != LegacyMapsRolePatchStatus::ready) {
            result.status = LegacyWorldRoleMapUpdateStatus::maps_patch_failed;
            return result;
        }
        result.maps_source_patched = true;
        return result;
    }
    if (role_index >= context.roles.size()) {
        result.status =
            LegacyWorldRoleMapUpdateStatus::controlled_role_out_of_range;
        return result;
    }
    if (*context.party_role_count == 0U ||
        *context.party_role_count > kLegacyWorldPartySlotCount) {
        result.status =
            LegacyWorldRoleMapUpdateStatus::party_count_out_of_range;
        return result;
    }

    u32 party_index = 0U;
    while (party_index < kLegacyWorldPartySlotCount &&
           context.party_role_indices[party_index] != role_index) {
        ++party_index;
    }
    result.physical_party_index = party_index;
    if (party_index == kLegacyWorldPartySlotCount) {
        result.status =
            LegacyWorldRoleMapUpdateStatus::active_role_not_in_physical_party;
        return result;
    }

    LegacyWorldRoleRecord& role = context.roles[role_index];
    LegacyWorldObjectSlot& party_slot = context.party_object_slots[party_index];
    const u16 path_cursor =
        read_u16_le(party_slot.bytes, kObjectPathCursorOffset);
    if (path_cursor < kObjectPathCursorMask &&
        ((role.world_x | role.world_y) & 0x0FU) != 0U) {
        result.status = align_party_role(role, party_slot, context, result);
        if (result.status != LegacyWorldRoleMapUpdateStatus::ready &&
            result.status !=
                LegacyWorldRoleMapUpdateStatus::
                    role_spatial_relocation_failed) {
            return result;
        }
    }

    static_cast<void>(reset_legacy_world_object_slot(party_slot));
    role.action.action_id = request.action_id;
    role.action.base_variant = request.base_variant;
    role.action.variant_delta = request.variant_delta;
    role.talk_script_id = request.talk_script_id;
    role.path_data_id = request.path_data_id;
    role.path_word_index = 0U;
    role.flags = request.flags;

    const auto patch_status = patch_maps_source(request, context, role.guid);
    result.maps_source_patched =
        patch_status == LegacyMapsRolePatchStatus::ready;

    const auto marked =
        mark_legacy_world_role_surface_occupancy(role, context.role_surface);
    if (marked.status != LegacyWorldRoleSurfaceStatus::ready) {
        result.status = LegacyWorldRoleMapUpdateStatus::role_surface_failed;
        return result;
    }
    for (u32 index = party_index; index + 1U < kLegacyWorldPartySlotCount;
         ++index) {
        context.party_role_indices[index] =
            context.party_role_indices[index + 1U];
    }
    for (u32 index = party_index; index + 1U < kLegacyWorldPartySlotCount;
         ++index) {
        context.party_object_slots[index] =
            context.party_object_slots[index + 1U];
    }
    --*context.party_role_count;
    result.party_role_removed = true;
    if (!result.maps_source_patched &&
        result.status == LegacyWorldRoleMapUpdateStatus::ready) {
        result.status = LegacyWorldRoleMapUpdateStatus::maps_patch_failed;
    }
    return result;
}

}  // namespace openswd3::world_map
