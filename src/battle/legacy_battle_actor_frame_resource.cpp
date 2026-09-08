#include "openswd3/battle/legacy_battle_actor_frame_resource.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstring>

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kPushEbxInstruction = 0x00478620U;
inline constexpr u32 kPushEsiInstruction = 0x00478623U;
inline constexpr u32 kPushEdiInstruction = 0x00478624U;
inline constexpr u32 kRepMovsdInstruction = 0x00478638U;
inline constexpr u32 kActionArgumentPushInstruction = 0x0047863AU;
inline constexpr u32 kActionCallInstruction = 0x0047863BU;
inline constexpr u32 kActionUpdateReturnAddress = 0x00478640U;
inline constexpr u32 kEarlyPopEdiInstruction = 0x00478647U;
inline constexpr u32 kEarlyPopEsiInstruction = 0x00478648U;
inline constexpr u32 kEarlyPopEbxInstruction = 0x00478649U;
inline constexpr u32 kEarlyReturnInstruction = 0x0047864AU;
inline constexpr u32 kPreparedFrameWordInstruction = 0x0047864BU;
inline constexpr u32 kPreparedResourceWordInstruction = 0x00478652U;
inline constexpr u32 kFrameArgumentPushInstruction = 0x00478659U;
inline constexpr u32 kResourceArgumentPushInstruction = 0x0047865AU;
inline constexpr u32 kFrameLookupCallInstruction = 0x0047865BU;
inline constexpr u32 kFrameLookupReturnAddress = 0x00478660U;
inline constexpr u32 kFrameTokenWriteInstruction = 0x00478663U;
inline constexpr u32 kSuccessPopEdiInstruction = 0x00478669U;
inline constexpr u32 kSuccessPopEsiInstruction = 0x0047866AU;
inline constexpr u32 kSuccessPopEbxInstruction = 0x0047866BU;
inline constexpr u32 kSuccessReturnInstruction = 0x0047866CU;
inline constexpr u32 kSourceActionOffset = 0x02A0U;
inline constexpr u32 kPreparedActionOffset = 0x0CB8U;

[[nodiscard]] constexpr bool even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logic_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left + right;
    return {
        .carry = value < left,
        .parity = even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr bool resolve_index(
    const u32 token,
    const u32 base,
    const u32 stride,
    const std::size_t count,
    std::size_t& index
) noexcept {
    if (token < base) {
        return false;
    }
    const u32 delta = token - base;
    if (delta % stride != 0U) {
        return false;
    }
    index = delta / stride;
    return index < count;
}

}  // namespace

LegacyBattleActorFrameResourceView resolve_legacy_battle_actor_frame_resource(
    const LegacyBattleActorFrameResourceOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            10U,
            index
        ) &&
        owners.action != nullptr) {
        auto& actor = owners.action->group_a_action_execution[index];
        return {
            .source_action = &actor.frame_source_action_record,
            .prepared_action = &actor.frame_prepared_action_record,
            .source_action_bytes = reinterpret_cast<const std::byte*>(
                &actor.frame_source_action_record
            ),
            .prepared_action_bytes = reinterpret_cast<std::byte*>(
                &actor.frame_prepared_action_record
            ),
            .frame_token = &actor.turn_frame_token,
            .frame_token_write_accessible =
                &actor.turn_frame_token_write_accessible,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            8U,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        auto& actor =
            (*owners.startup->group_b_lifecycle)[index].action_execution;
        return {
            .source_action = &actor.frame_source_action_record,
            .prepared_action = &actor.frame_prepared_action_record,
            .source_action_bytes = reinterpret_cast<const std::byte*>(
                &actor.frame_source_action_record
            ),
            .prepared_action_bytes = reinterpret_cast<std::byte*>(
                &actor.frame_prepared_action_record
            ),
            .frame_token = &actor.turn_frame_token,
            .frame_token_write_accessible =
                &actor.turn_frame_token_write_accessible,
        };
    }
    return {};
}

LegacyBattleActorFrameResourceResult prepare_legacy_battle_actor_frame_resource(
    const LegacyBattleActorFrameResourceView& actor,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleActorFrameResourceRequest& request
) {
    LegacyBattleActorFrameResourceResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
        .return_ebx = request.entry_ebx,
        .return_esi = request.entry_esi,
        .return_edi = request.entry_edi,
        .return_esp = request.entry_esp,
        .return_eip = kPushEbxInstruction,
        .source_token = request.actor_token + kSourceActionOffset,
        .destination_token = request.actor_token + kPreparedActionOffset,
        .flags_known = request.entry_flags_known,
        .direction_flag = request.direction_flag,
        .flags = request.entry_flags,
    };

    const auto stop = [&](const LegacyBattleActorFrameResourceStatus status) {
        result.status = status;
        return result;
    };
    const auto push = [&](const bool writable,
                          const u32 value,
                          const u32 fault_eip,
                          const LegacyBattleActorFrameResourceStatus status) {
        result.return_eip = fault_eip;
        if (!writable) {
            result.status = status;
            return false;
        }
        result.return_esp -= 4U;
        result.stack_writes[result.stack_write_count] = value;
        ++result.stack_write_count;
        return true;
    };
    const auto pop = [&](const bool readable,
                         u32& destination,
                         const u32 value,
                         const u32 fault_eip,
                         const LegacyBattleActorFrameResourceStatus status) {
        result.return_eip = fault_eip;
        if (!readable) {
            result.status = status;
            return false;
        }
        destination = value;
        result.return_esp += 4U;
        result.stack_reads[result.stack_read_count] = value;
        ++result.stack_read_count;
        return true;
    };
    const auto restore_registers = [&](const u32 pop_edi_eip,
                                       const u32 pop_esi_eip,
                                       const u32 pop_ebx_eip) {
        if (!pop(
                request.stack_access.pop_edi_readable,
                result.return_edi,
                request.entry_edi,
                pop_edi_eip,
                LegacyBattleActorFrameResourceStatus::pop_edi_read_typed_stop
            )) {
            return false;
        }
        if (!pop(
                request.stack_access.pop_esi_readable,
                result.return_esi,
                request.entry_esi,
                pop_esi_eip,
                LegacyBattleActorFrameResourceStatus::pop_esi_read_typed_stop
            )) {
            return false;
        }
        return pop(
            request.stack_access.pop_ebx_readable,
            result.return_ebx,
            request.entry_ebx,
            pop_ebx_eip,
            LegacyBattleActorFrameResourceStatus::pop_ebx_read_typed_stop
        );
    };

    if (!push(
            request.stack_access.push_ebx_writable,
            request.entry_ebx,
            kPushEbxInstruction,
            LegacyBattleActorFrameResourceStatus::push_ebx_write_typed_stop
        )) {
        return result;
    }
    result.return_ebx = request.entry_ecx;
    if (!push(
            request.stack_access.push_esi_writable,
            request.entry_esi,
            kPushEsiInstruction,
            LegacyBattleActorFrameResourceStatus::push_esi_write_typed_stop
        )) {
        return result;
    }
    if (!push(
            request.stack_access.push_edi_writable,
            request.entry_edi,
            kPushEdiInstruction,
            LegacyBattleActorFrameResourceStatus::push_edi_write_typed_stop
        )) {
        return result;
    }

    result.return_eax = result.destination_token;
    result.return_esi = result.source_token;
    result.return_ecx = static_cast<u32>(kLegacyBattleActorFrameResourceDwords);
    result.return_edi = result.destination_token;
    result.return_eip = kRepMovsdInstruction;
    if (request.direction_flag) {
        return stop(
            LegacyBattleActorFrameResourceStatus::
                direction_flag_contract_typed_stop
        );
    }

    const auto* source_bytes = actor.source_action_bytes != nullptr
        ? actor.source_action_bytes
        : reinterpret_cast<const std::byte*>(actor.source_action);
    auto* prepared_bytes = actor.prepared_action_bytes != nullptr
        ? actor.prepared_action_bytes
        : reinterpret_cast<std::byte*>(actor.prepared_action);
    for (std::size_t index = 0U; index < kLegacyBattleActorFrameResourceDwords;
         ++index) {
        result.fault_dword_index = index;
        if (source_bytes == nullptr || !request.source_dword_readable[index]) {
            return stop(
                LegacyBattleActorFrameResourceStatus::
                    source_dword_read_typed_stop
            );
        }
        u32 value{};
        std::memcpy(&value, source_bytes + index * sizeof(u32), sizeof(value));
        ++result.source_reads;

        if (prepared_bytes == nullptr ||
            !request.destination_dword_writable[index]) {
            return stop(
                LegacyBattleActorFrameResourceStatus::
                    destination_dword_write_typed_stop
            );
        }
        std::memcpy(
            prepared_bytes + index * sizeof(u32), &value, sizeof(value)
        );
        ++result.destination_writes;
        ++result.copied_dwords;
        result.return_esi += 4U;
        result.return_edi += 4U;
        --result.return_ecx;
    }
    result.fault_dword_index = kLegacyBattleActorFrameResourceDwords;

    if (!push(
            request.stack_access.action_argument_push_writable,
            result.destination_token,
            kActionArgumentPushInstruction,
            LegacyBattleActorFrameResourceStatus::
                action_argument_push_typed_stop
        )) {
        return result;
    }
    if (!push(
            request.stack_access.action_call_return_push_writable,
            kActionUpdateReturnAddress,
            kActionCallInstruction,
            LegacyBattleActorFrameResourceStatus::
                action_call_return_push_typed_stop
        )) {
        return result;
    }

    if (actor.prepared_action == nullptr) {
        return stop(
            LegacyBattleActorFrameResourceStatus::
                destination_dword_write_typed_stop
        );
    }
    result.action_updater_entry_eax = result.return_eax;
    result.action_updater_entry_ecx = result.return_ecx;
    result.action_updater_entry_edx = result.return_edx;
    ++result.action_update_calls;
    result.action_update = action_updater.update(*actor.prepared_action);
    result.return_eax = request.override_action_updater_return_eax
        ? request.action_updater_return_eax
        : result.action_update.return_value;
    result.return_ecx = request.action_updater_return_ecx;
    result.return_edx = request.action_updater_return_edx;
    result.flags = request.action_updater_flags;
    result.flags_known = request.action_updater_flags_known;
    result.return_esp += 4U;
    result.stack_reads[result.stack_read_count] = kActionUpdateReturnAddress;
    ++result.stack_read_count;

    const u32 action_argument_esp = result.return_esp;
    result.return_esp += 4U;
    result.flags = add_flags(action_argument_esp, 4U);
    result.flags_known = true;
    result.flags = logic_flags(result.return_eax);
    if (result.return_eax == 0U) {
        result.returned_early = true;
        if (!restore_registers(
                kEarlyPopEdiInstruction,
                kEarlyPopEsiInstruction,
                kEarlyPopEbxInstruction
            )) {
            return result;
        }
        result.return_eip = kEarlyReturnInstruction;
        if (!request.stack_access.early_return_address_readable) {
            return stop(
                LegacyBattleActorFrameResourceStatus::
                    early_return_address_read_typed_stop
            );
        }
        result.return_esp += 4U;
        result.stack_reads[result.stack_read_count] =
            request.entry_return_address;
        ++result.stack_read_count;
        result.return_eip = request.entry_return_address;
        result.returned = true;
        return result;
    }

    result.return_eip = kPreparedFrameWordInstruction;
    if (prepared_bytes == nullptr || !request.prepared_frame_word_readable) {
        return stop(
            LegacyBattleActorFrameResourceStatus::
                prepared_frame_word_read_typed_stop
        );
    }
    compat::u16 frame_id{};
    std::memcpy(
        &frame_id,
        prepared_bytes + offsetof(asset_runtime::LegacyActionRecord, field_4c),
        sizeof(frame_id)
    );
    result.return_eax = (result.return_eax & 0xFFFF0000U) | frame_id;
    result.frame_id = result.return_eax & 0xFFFFU;
    ++result.source_reads;

    result.return_eip = kPreparedResourceWordInstruction;
    if (!request.prepared_resource_word_readable) {
        return stop(
            LegacyBattleActorFrameResourceStatus::
                prepared_resource_word_read_typed_stop
        );
    }
    compat::u16 resource_id{};
    std::memcpy(
        &resource_id,
        prepared_bytes + offsetof(asset_runtime::LegacyActionRecord, field_4a),
        sizeof(resource_id)
    );
    result.return_ecx = (result.return_ecx & 0xFFFF0000U) | resource_id;
    result.resource_id = result.return_ecx & 0xFFFFU;
    ++result.source_reads;

    if (!push(
            request.stack_access.frame_argument_push_writable,
            result.return_eax,
            kFrameArgumentPushInstruction,
            LegacyBattleActorFrameResourceStatus::frame_argument_push_typed_stop
        )) {
        return result;
    }
    if (!push(
            request.stack_access.resource_argument_push_writable,
            result.return_ecx,
            kResourceArgumentPushInstruction,
            LegacyBattleActorFrameResourceStatus::
                resource_argument_push_typed_stop
        )) {
        return result;
    }
    if (!push(
            request.stack_access.frame_call_return_push_writable,
            kFrameLookupReturnAddress,
            kFrameLookupCallInstruction,
            LegacyBattleActorFrameResourceStatus::
                frame_call_return_push_typed_stop
        )) {
        return result;
    }

    result.frame_provider_entry_eax = result.return_eax;
    result.frame_provider_entry_ecx = result.return_ecx;
    result.frame_provider_entry_edx = result.return_edx;
    ++result.frame_lookup_calls;
    result.frame_available = frame_provider.load_frame_piece(
        result.resource_id, result.frame_id, result.frame
    );
    result.return_eax = request.frame_provider_return_eax;
    result.return_ecx = request.frame_provider_return_ecx;
    result.return_edx = request.frame_provider_return_edx;
    result.flags = request.frame_provider_flags;
    result.flags_known = request.frame_provider_flags_known;
    result.return_esp += 4U;
    result.stack_reads[result.stack_read_count] = kFrameLookupReturnAddress;
    ++result.stack_read_count;

    const u32 frame_arguments_esp = result.return_esp;
    result.return_esp += 8U;
    result.flags = add_flags(frame_arguments_esp, 8U);
    result.flags_known = true;

    result.return_eip = kFrameTokenWriteInstruction;
    if (actor.frame_token == nullptr ||
        (actor.frame_token_write_accessible != nullptr &&
         !*actor.frame_token_write_accessible)) {
        return stop(
            LegacyBattleActorFrameResourceStatus::frame_token_write_typed_stop
        );
    }
    *actor.frame_token = result.return_eax;
    ++result.actor_writes;
    result.frame_token_committed = true;

    if (!restore_registers(
            kSuccessPopEdiInstruction,
            kSuccessPopEsiInstruction,
            kSuccessPopEbxInstruction
        )) {
        return result;
    }
    result.return_eip = kSuccessReturnInstruction;
    if (!request.stack_access.success_return_address_readable) {
        return stop(
            LegacyBattleActorFrameResourceStatus::
                success_return_address_read_typed_stop
        );
    }
    result.return_esp += 4U;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

}  // namespace openswd3::battle
