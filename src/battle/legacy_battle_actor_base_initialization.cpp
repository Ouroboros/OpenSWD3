#include "openswd3/battle/legacy_battle_actor_base_initialization.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

[[nodiscard]] bool can_write(
    const LegacyBattleActorBaseInitializationRequest request,
    const compat::u32 offset,
    const compat::u32 size,
    LegacyBattleActorBaseInitializationResult& result,
    const compat::u32 eax,
    const compat::u32 ecx
) noexcept {
    if (request.object_token != 0U && request.writable_bytes >= offset + size) {
        return true;
    }

    result.status =
        LegacyBattleActorBaseInitializationStatus::object_write_typed_stop;
    result.stopped_object_offset = offset;
    result.return_eax = eax;
    result.return_ecx = ecx;
    return false;
}

}  // namespace

LegacyBattleActorBaseInitializationResult initialize_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationFields& fields,
    LegacyBattleGroupAActionExecutionState& action_execution,
    const std::span<compat::u8> resource_definition,
    std::vector<compat::u8>& resource_definition_description,
    const std::span<compat::u8> action_text,
    compat::u16& action_kind,
    const LegacyBattleActorBaseInitializationRequest request
) noexcept {
    LegacyBattleActorBaseInitializationResult result{
        .return_eax = 0xFFFFFFFFU,
        .return_ecx = request.object_token + 0x2A56U,
        .return_edx = request.object_token,
    };
    compat::u32 eax = 0xFFFFFFFFU;
    compat::u32 ecx = request.object_token + 0x2A56U;

    for (std::size_t index = 0U; index < action_execution.target_indices.size();
         ++index) {
        const auto offset =
            0x2A56U + static_cast<compat::u32>(index * sizeof(compat::u32));
        if (!can_write(
                request, offset, sizeof(compat::u32), result, eax, ecx
            )) {
            return result;
        }
        action_execution.target_indices[index] = eax;
        ++result.dword_writes;
    }

    eax = 0U;
    ecx = request.object_token + 0x2630U;
    const auto write_dword = [&](compat::u32& destination,
                                 const compat::u32 offset,
                                 const compat::u32 value) {
        if (!can_write(
                request, offset, sizeof(compat::u32), result, eax, ecx
            )) {
            return false;
        }
        destination = value;
        ++result.dword_writes;
        return true;
    };
    const auto write_signed_dword = [&](compat::i32& destination,
                                        const compat::u32 offset,
                                        const compat::i32 value) {
        if (!can_write(
                request, offset, sizeof(compat::u32), result, eax, ecx
            )) {
            return false;
        }
        destination = value;
        ++result.dword_writes;
        return true;
    };
    const auto write_word = [&](compat::u16& destination,
                                const compat::u32 offset,
                                const compat::u16 value) {
        if (!can_write(
                request, offset, sizeof(compat::u16), result, eax, ecx
            )) {
            return false;
        }
        destination = value;
        ++result.word_writes;
        return true;
    };
    const auto write_byte = [&](compat::u8& destination,
                                const compat::u32 offset,
                                const compat::u8 value) {
        if (!can_write(request, offset, sizeof(compat::u8), result, eax, ecx)) {
            return false;
        }
        destination = value;
        ++result.byte_writes;
        return true;
    };

    if (!write_word(fields.field_29a2, 0x29A2U, 0xFFFFU) ||
        !write_signed_dword(action_execution.turn_countdown, 0x2668U, 0x0F) ||
        !write_dword(fields.field_266c, 0x266CU, 1U) ||
        !write_word(action_execution.turn_threshold, 0x2958U, 0U) ||
        !write_word(fields.field_2a0a, 0x2A0AU, 4U) ||
        !write_word(fields.field_2a68, 0x2A68U, 2U) ||
        !write_word(fields.field_2a6a, 0x2A6AU, 0x18U) ||
        !write_word(action_execution.motion_word, 0x2954U, 0U) ||
        !write_word(action_execution.motion_aux_word, 0x2956U, 0U) ||
        !write_word(action_execution.summon_phase, 0x2A66U, 0U) ||
        !write_word(action_execution.profile_variant_override, 0x2A0EU, 0U) ||
        !write_word(action_kind, 0x2A6CU, 0U) ||
        !write_byte(fields.field_2a94, 0x2A94U, 0U) ||
        !write_dword(fields.field_26bc, 0x26BCU, 0x062B062BU) ||
        !write_dword(fields.linked_action_head_token, 0x2584U, 0U)) {
        return result;
    }

    for (std::size_t index = 0U; index < 4U; ++index) {
        const auto byte_index = index * sizeof(compat::u32);
        const auto offset = 0x2630U + static_cast<compat::u32>(byte_index);
        if (!can_write(
                request, offset, sizeof(compat::u32), result, eax, ecx
            ) ||
            byte_index + sizeof(compat::u32) > action_text.size()) {
            result.status = LegacyBattleActorBaseInitializationStatus::
                object_write_typed_stop;
            result.stopped_object_offset = offset;
            result.return_eax = eax;
            result.return_ecx = ecx;
            return result;
        }
        for (std::size_t byte = 0U; byte < sizeof(compat::u32); ++byte) {
            action_text[byte_index + byte] = 0U;
        }
        ++result.dword_writes;
    }

    for (std::size_t index = 0U; index < 0x29U; ++index) {
        ecx = 0x29U - static_cast<compat::u32>(index);
        const auto byte_index = index * sizeof(compat::u32);
        const auto offset = 0x10U + static_cast<compat::u32>(byte_index);
        if (!can_write(
                request, offset, sizeof(compat::u32), result, eax, ecx
            ) ||
            byte_index + sizeof(compat::u32) > resource_definition.size()) {
            result.status = LegacyBattleActorBaseInitializationStatus::
                object_write_typed_stop;
            result.stopped_object_offset = offset;
            result.return_eax = eax;
            result.return_ecx = ecx;
            return result;
        }
        for (std::size_t byte = 0U; byte < sizeof(compat::u32); ++byte) {
            resource_definition[byte_index + byte] = 0U;
        }
        ++result.dword_writes;
        if (index == 0x28U) {
            resource_definition_description.clear();
        }
    }

    result.return_eax = request.object_token;
    result.return_ecx = 0U;
    return result;
}

LegacyBattleActorBaseInitializationResult initialize_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationOwner& owner,
    const LegacyBattleActorBaseInitializationRequest request
) noexcept {
    return initialize_legacy_battle_actor_base(
        owner.fields,
        owner.action_execution,
        owner.resource_definition,
        owner.resource_definition_description,
        owner.action_text,
        owner.action_kind,
        request
    );
}

}  // namespace openswd3::battle
