#include "openswd3/world_map/legacy_world_role_post_materialization.hpp"

namespace openswd3::world_map {
namespace {

using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kGateDirectoryOffsetField = 0x64U;
constexpr std::size_t kGateRecordSize = 0x12U;
constexpr std::size_t kGateStateCount = 8U;
constexpr u16 kGateDirectoryTerminator = 0xFFFFU;
constexpr u32 kRoleTransferFlag = 0x00000080U;
constexpr u32 kFlaggedRoleRecordFlag = 0x00000200U;
constexpr u32 kRoleMapMarkerFlag = 0x00008000U;

struct GateEvaluation {
    LegacyWorldRolePostMaterializationStatus status{
        LegacyWorldRolePostMaterializationStatus::ready
    };
    bool transfer{};
};

[[nodiscard]] bool range_available(
    const std::span<const u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16 read_u16_le(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_u32_le(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] GateEvaluation evaluate_role_gate(
    const std::span<const u8> payload,
    const u16 role_guid,
    const u16 map_state
) noexcept {
    GateEvaluation result;
    if (!range_available(
            payload,
            kGateDirectoryOffsetField,
            sizeof(u32)
        )) {
        result.status = LegacyWorldRolePostMaterializationStatus::
            gate_offset_field_out_of_range;
        return result;
    }

    std::size_t offset = read_u32_le(payload, kGateDirectoryOffsetField);
    if (!range_available(payload, offset, sizeof(u16))) {
        result.status = LegacyWorldRolePostMaterializationStatus::
            gate_directory_offset_out_of_range;
        return result;
    }

    u32 decision = 1U;
    while (true) {
        const u16 row_guid = read_u16_le(payload, offset);
        if (row_guid == kGateDirectoryTerminator) {
            result.transfer = decision != 0U;
            return result;
        }
        if (!range_available(payload, offset, kGateRecordSize)) {
            result.status = LegacyWorldRolePostMaterializationStatus::
                gate_record_truncated;
            return result;
        }

        if (row_guid == role_guid) {
            decision &= ~1U;
            for (std::size_t index = 0U;
                 index < kGateStateCount;
                 ++index) {
                if (read_u16_le(
                        payload,
                        offset + sizeof(u16) + index * sizeof(u16)
                    ) == map_state) {
                    decision |= 2U;
                    break;
                }
            }
        }

        offset += kGateRecordSize;
        if (!range_available(payload, offset, sizeof(u16))) {
            result.status = LegacyWorldRolePostMaterializationStatus::
                gate_directory_unterminated;
            return result;
        }
    }
}

void apply_guid_one_action_override(
    LegacyWorldRoleRecord& role,
    const LegacyMapsMapDescriptor& descriptor,
    const LegacyWorldLoadRequest& request,
    const LegacyWorldRolePostMaterializationContext* const context,
    LegacyWorldRolePostMaterializationState& state
) noexcept {
    if (role.guid != 1U) {
        return;
    }

    const u32 previous_map = context == nullptr ?
        0U : context->previous_logical_map_id;
    const u32 action_override = context == nullptr ?
        0U : context->guid_one_action_override;
    bool overridden = false;
    if (previous_map == 22U && request.logical_map_id != 22U &&
        action_override != 0U) {
        role.action.action_id = action_override;
        overridden = true;
    }

    if ((descriptor.field_0c & 0x8000U) != 0U) {
        role.action.action_id = 0x60U;
        overridden = true;
        const bool map_uses_story_override =
            request.logical_map_id == 6U ||
            request.logical_map_id == 8U ||
            request.logical_map_id == 200U;
        if (map_uses_story_override && context != nullptr &&
            context->has_story_state_0x0192) {
            role.action.action_id = 0x5FU;
        }
    }

    if (overridden) {
        ++state.guid_one_roles_overridden;
    }
}

[[nodiscard]] LegacyWorldRolePostMaterializationStatus transfer_role(
    const std::span<u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const u16 selected_guid,
    const LegacyWorldRolePostMaterializationContext* const context,
    LegacyWorldRolePostMaterializationState& state
) noexcept {
    LegacyWorldRoleTransferContext transfer_context;
    const LegacyWorldRoleTransferContext* transfer_context_pointer = nullptr;
    if (context != nullptr) {
        transfer_context.active_object_slots = context->active_object_slots;
        transfer_context.spatial_index = context->spatial_index;
        transfer_context.surface_grid = context->surface_grid;
        transfer_context.map_width = context->map_width;
        transfer_context.selected_guid = selected_guid;
        transfer_context_pointer = &transfer_context;
    }

    state.last_transfer_status = transfer_legacy_world_role(
        maps_payload,
        maps_database,
        roles,
        role_index,
        transfer_context_pointer,
        state
    );
    switch (state.last_transfer_status) {
    case LegacyWorldRoleTransferStatus::ready:
        return LegacyWorldRolePostMaterializationStatus::ready;
    case LegacyWorldRoleTransferStatus::party_capacity_exceeded:
        return LegacyWorldRolePostMaterializationStatus::
            party_capacity_exceeded;
    case LegacyWorldRoleTransferStatus::active_object_slots_required:
        return LegacyWorldRolePostMaterializationStatus::
            active_object_slots_required;
    case LegacyWorldRoleTransferStatus::role_source_patch_failed:
        return LegacyWorldRolePostMaterializationStatus::
            role_source_patch_failed;
    default:
        return LegacyWorldRolePostMaterializationStatus::role_transfer_failed;
    }
}

void append_flagged_role_record(
    const LegacyWorldRoleRecord& role,
    LegacyWorldRolePostMaterializationState& state
) noexcept {
    if ((role.flags & kFlaggedRoleRecordFlag) == 0U) {
        return;
    }

    if (state.flagged_role_record_count >=
        kLegacyWorldFlaggedRoleRecordCount) {
        ++state.flagged_role_overflow_count;
        return;
    }

    auto& record = state.flagged_role_records[
        state.flagged_role_record_count
    ];
    record.world_x = static_cast<u16>(role.world_x);
    record.world_y = static_cast<u16>(role.world_y);
    record.field_04 = 0U;
    record.field_06 = 0U;
    record.guid = role.guid;
    record.field_0a = 0U;
    ++state.flagged_role_record_count;
}

}  // namespace

LegacyWorldRolePostMaterializationStatus
post_materialize_legacy_world_role(
    const std::span<u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const LegacyMapsMapDescriptor& map_descriptor,
    const LegacyWorldLoadRequest& request,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const LegacyWorldRolePostMaterializationContext* const context,
    LegacyWorldRolePostMaterializationState& state
) noexcept {
    if (role_index == 0U || role_index >= roles.size()) {
        return LegacyWorldRolePostMaterializationStatus::
            role_index_out_of_range;
    }

    LegacyWorldRoleRecord& role = roles[role_index];
    if (role.guid == request.selected_guid) {
        state.party_role_indices[0] = role_index;
    }

    apply_guid_one_action_override(
        role,
        map_descriptor,
        request,
        context,
        state
    );

    if ((role.flags & kRoleTransferFlag) != 0U) {
        ++state.gated_roles_scanned;
        const auto gate = evaluate_role_gate(
            maps_payload,
            role.guid,
            static_cast<u16>(map_descriptor.field_0c & 0x7FFFU)
        );
        if (gate.status != LegacyWorldRolePostMaterializationStatus::ready) {
            return gate.status;
        }

        if (gate.transfer) {
            const auto transfer_status = transfer_role(
                maps_payload,
                maps_database,
                roles,
                role_index,
                request.selected_guid,
                context,
                state
            );
            if (transfer_status !=
                LegacyWorldRolePostMaterializationStatus::ready) {
                return transfer_status;
            }

            role.flags |= kRoleMapMarkerFlag;
            if (request.logical_map_id == 22U ||
                (map_descriptor.field_0c & 0x8000U) != 0U) {
                role.flags &= ~kRoleMapMarkerFlag;
            }
        } else {
            ++state.roles_suppressed;
        }
    }

    append_flagged_role_record(role, state);
    return LegacyWorldRolePostMaterializationStatus::ready;
}

}  // namespace openswd3::world_map
