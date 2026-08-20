#include "openswd3/world_map/legacy_world_role_lifecycle.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kRoleIndexOffset = 0x00U;
constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kPathBytesOffset = 0x1CU;
constexpr u16 kPathCursorMask = 0x7FFFU;
constexpr u32 kPathOwnershipFlag = 0x80000000U;
constexpr u32 kRestorePendingActionFlag = 0x00000800U;
constexpr u32 kMatchingSlotFlagClearMask = 0xBBFFFFFFU;
constexpr u32 kNoActionOverride = 0xFFFFFFFFU;

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
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void record_transition_failure(
    LegacyWorldRoleTransitionResult& result,
    const LegacyWorldRoleTransitionStatus status
) noexcept {
    if (result.status == LegacyWorldRoleTransitionStatus::ready) {
        result.status = status;
    }
}

void align_axis(compat::u32& coordinate, const i32 step) noexcept {
    const u32 step_bits = std::bit_cast<u32>(step);
    while ((coordinate & 0x0FU) != 0U) {
        coordinate -= step_bits;
    }
}

}  // namespace

LegacyWorldRoleTableClearResult clear_legacy_world_role_table(
    const std::span<LegacyWorldRoleRecord> roles,
    std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>&
        role_label_payloads
) noexcept {
    LegacyWorldRoleTableClearResult result;
    if (roles.size() > kLegacyWorldRoleCapacity) {
        result.status =
            LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity;
        return result;
    }

    for (std::size_t index = 0U; index < roles.size(); ++index) {
        ++result.payload_slots_scanned;
        LegacyWorldRoleRecord& role = roles[index];
        if (role.path_payload_pointer_32 != 0U) {
            std::vector<compat::u8>{}.swap(role_label_payloads[index]);
            ++result.payload_owners_released;
        }
        role = {};
        ++result.roles_zeroed;
    }

    result.status = LegacyWorldRoleTableResetStatus::ready;
    return result;
}

LegacyWorldRoleTableResetResult reset_legacy_world_role_table(
    const std::span<LegacyWorldRoleRecord> roles,
    std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>&
        role_label_payloads,
    const compat::i32 highest_role_index
) noexcept {
    LegacyWorldRoleTableResetResult result;
    if (roles.size() > kLegacyWorldRoleCapacity) {
        result.status =
            LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity;
        return result;
    }
    if (highest_role_index >=
        static_cast<compat::i32>(kLegacyWorldRoleCapacity)) {
        result.status =
            LegacyWorldRoleTableResetStatus::highest_role_index_out_of_range;
        return result;
    }
    if (highest_role_index >= 0 &&
        static_cast<std::size_t>(highest_role_index) >= roles.size()) {
        result.status =
            LegacyWorldRoleTableResetStatus::active_role_range_out_of_bounds;
        return result;
    }

    for (compat::i32 index = 0; index <= highest_role_index; ++index) {
        ++result.payload_slots_scanned;
        LegacyWorldRoleRecord& role = roles[static_cast<std::size_t>(index)];
        if (role.path_payload_pointer_32 != 0U) {
            std::vector<compat::u8>{}.swap(
                role_label_payloads[static_cast<std::size_t>(index)]
            );
            ++result.payload_owners_released;
        }
    }

    for (LegacyWorldRoleRecord& role : roles) {
        role = {};
        ++result.roles_zeroed;
    }
    for (LegacyWorldRoleRecord& role : roles) {
        asset_runtime::initialize_legacy_action_record(role.action);
        ++result.action_records_initialized;
    }

    result.status = LegacyWorldRoleTableResetStatus::ready;
    return result;
}

LegacyWorldRoleTransitionResult release_legacy_world_role_transition(
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const u32 role_index,
    const u32 selected_role_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    LegacyWorldMapRolePathPorts& path_ports,
    asset_runtime::LegacyActionDrawPorts& action_ports
) {
    LegacyWorldRoleTransitionResult result;
    if (role_index >= roles.size()) {
        result.status = LegacyWorldRoleTransitionStatus::invalid_role_index;
        return result;
    }

    LegacyWorldRoleRecord& role = roles[role_index];
    result.ownership_flag_set = (role.flags & kPathOwnershipFlag) != 0U;
    if (!result.ownership_flag_set) {
        role.interaction_gate = 0U;
        return result;
    }

    for (LegacyWorldObjectSlot& slot : active_object_slots) {
        ++result.slots_scanned;
        if (read_u16_le(slot, kRoleIndexOffset) !=
            static_cast<u16>(role_index)) {
            continue;
        }
        ++result.matching_slots;

        if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
            ++result.surface_clear_calls;
            const auto cleared = clear_legacy_world_role_surface_occupancy(
                role, surface_context
            );
            result.surface_status = cleared.status;
            if (cleared.status != LegacyWorldRoleSurfaceStatus::ready) {
                record_transition_failure(
                    result,
                    LegacyWorldRoleTransitionStatus::surface_clear_failed
                );
            }

            if (role_index != selected_role_index) {
                const u16 cursor = static_cast<u16>(
                    read_u16_le(slot, kPathCursorOffset) & kPathCursorMask
                );
                const std::size_t direction_offset = kPathBytesOffset + cursor;
                if (direction_offset >= slot.bytes.size()) {
                    record_transition_failure(
                        result,
                        LegacyWorldRoleTransitionStatus::
                            path_cursor_out_of_range
                    );
                } else {
                    const u8 direction = slot.bytes[direction_offset];
                    if (direction >= kDirectionStepX.size()) {
                        record_transition_failure(
                            result,
                            LegacyWorldRoleTransitionStatus::
                                direction_out_of_range
                        );
                    } else {
                        align_axis(role.world_x, kDirectionStepX[direction]);
                        align_axis(role.world_y, kDirectionStepY[direction]);
                        ++result.coordinate_alignment_slots;
                    }
                }
            }
        }

        role.flags &= kMatchingSlotFlagClearMask;
        role.action.wait_override = 0U;
    }

    ++result.path_completion_calls;
    if (!path_ports.complete_role_path(role_index)) {
        record_transition_failure(
            result, LegacyWorldRoleTransitionStatus::path_completion_failed
        );
    }

    if ((role.flags & kRestorePendingActionFlag) != 0U) {
        if (role.action.one_shot_base_variant != kNoActionOverride) {
            role.action.base_variant = role.action.one_shot_base_variant;
        }
        if (role.action.one_shot_variant_delta != kNoActionOverride) {
            role.action.variant_delta = role.action.one_shot_variant_delta;
        }
    }
    role.action.one_shot_base_variant = kNoActionOverride;
    role.action.one_shot_variant_delta = kNoActionOverride;

    ++result.action_update_calls;
    result.action_update_status =
        action_ports.update_action_record(role.action);
    if (result.action_update_status !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
        record_transition_failure(
            result, LegacyWorldRoleTransitionStatus::action_update_failed
        );
    }

    role.action.one_shot_base_variant = kNoActionOverride;
    role.action.one_shot_variant_delta = kNoActionOverride;
    role.flags &= ~kPathOwnershipFlag;
    role.action.wait_remaining = 0U;
    role.interaction_gate = 0U;
    return result;
}

}  // namespace openswd3::world_map
