#include "openswd3/battle/legacy_battle_actor_presentation_activation.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

inline constexpr u32 kSpecialReadyOffset = 0x00002AB8U;
inline constexpr u32 kMarkerOffset = 0x00002A94U;
inline constexpr u32 kPresentationEnabledOffset = 0x00002ABCU;
inline constexpr u32 kSourceRuntimeValueOffset = 0x00002AA0U;
inline constexpr u32 kLiveRecordTokenOffset = 0x00000004U;
inline constexpr u32 kLiveRecordValueOffset = 0x00000004U;
inline constexpr u32 kSpecialReadyWriteInstruction = 0x004787F9U;
inline constexpr u32 kMarkerReadInstruction = 0x004787FFU;
inline constexpr u32 kPresentationEnabledWriteInstruction = 0x00478807U;
inline constexpr u32 kMarkerWriteInstruction = 0x0047880FU;
inline constexpr u32 kSourceRuntimeValueReadInstruction = 0x00478816U;
inline constexpr u32 kLiveRecordTokenReadInstruction = 0x0047881EU;
inline constexpr u32 kLiveRecordValueWriteInstruction = 0x00478825U;
inline constexpr u32 kReturnInstruction = 0x0047882BU;

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    u8 bits = value;
    bool parity = true;
    while (bits != 0U) {
        parity = !parity;
        bits = static_cast<u8>(bits & static_cast<u8>(bits - 1U));
    }
    return parity;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
test_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 lhs, const u32 rhs) noexcept {
    const u32 value = lhs - rhs;
    return {
        .carry = lhs < rhs,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((lhs ^ rhs ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((lhs ^ rhs) & (lhs ^ value) & 0x80000000U) != 0U,
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

void record_access(
    LegacyBattleActorPresentationActivationResult& result,
    const LegacyBattleActorPresentationActivationAccess access
) noexcept {
    result.actor_accesses[result.actor_access_count] = access;
    ++result.actor_access_count;
}

}  // namespace

LegacyBattleActorPresentationActivationView
resolve_legacy_battle_actor_presentation_activation(
    const LegacyBattleActorPresentationActivationOwners& owners,
    const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            kLegacyBattleActorGroupAElementCount,
            index
        ) &&
        owners.startup != nullptr) {
        auto& actor = owners.startup->party[index];
        return {
            .special_ready = &actor.progress.special_ready,
            .marker = &actor.base_initialization.field_2a94,
            .presentation_enabled = &actor.progress.presentation_enabled,
            .source_runtime_value = &actor.configuration.source_runtime_value,
            .live_record_token = &actor.configuration.actor_record_token,
            .live_record_value_04 = &actor.configuration.actor_record[2U],
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        auto& actor = (*owners.startup->group_b_lifecycle)[index];
        return {
            .special_ready = &actor.action_configuration.special_ready,
            .marker = &actor.base_initialization.field_2a94,
            .presentation_enabled =
                &actor.action_configuration.presentation_enabled,
            .source_runtime_value =
                &actor.action_configuration.source_runtime_value,
            .live_record_token = &actor.live_record_token,
            .live_record_value_04 = &actor.live_record_value_04,
        };
    }

    return {};
}

LegacyBattleActorPresentationActivationResult
activate_legacy_battle_actor_presentation(
    const LegacyBattleActorPresentationActivationView actor,
    const LegacyBattleActorPresentationActivationRequest& request
) noexcept {
    LegacyBattleActorPresentationActivationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .argument_token = request.entry_esp + 4U,
        .special_ready_token = request.actor_token + kSpecialReadyOffset,
        .marker_token = request.actor_token + kMarkerOffset,
        .presentation_enabled_token =
            request.actor_token + kPresentationEnabledOffset,
        .source_runtime_value_token =
            request.actor_token + kSourceRuntimeValueOffset,
        .live_record_token_token = request.actor_token + kLiveRecordTokenOffset,
        .return_address_token = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (!request.access.argument_readable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            argument_read_typed_stop;
        return result;
    }
    result.argument_value = request.value;
    result.return_edx = request.value;
    result.stack_read_tokens[result.stack_read_count] = result.argument_token;
    result.stack_reads[result.stack_read_count] = request.value;
    ++result.stack_read_count;
    ++result.argument_reads;

    result.return_eax = 1U;
    result.return_eip = kSpecialReadyWriteInstruction;
    if (actor.special_ready == nullptr ||
        !request.access.special_ready_writable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            special_ready_write_typed_stop;
        return result;
    }
    *actor.special_ready = request.value;
    record_access(
        result,
        LegacyBattleActorPresentationActivationAccess::special_ready_write
    );
    ++result.special_ready_writes;

    result.return_eip = kMarkerReadInstruction;
    if (actor.marker == nullptr || !request.access.marker_readable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            marker_read_typed_stop;
        return result;
    }
    result.original_marker = *actor.marker;
    result.return_edx = (result.return_edx & 0xFFFFFF00U) |
        static_cast<u32>(result.original_marker);
    record_access(
        result, LegacyBattleActorPresentationActivationAccess::marker_read
    );
    ++result.marker_reads;
    result.flags = test_flags(result.original_marker);
    result.flags_known = true;

    result.return_eip = kPresentationEnabledWriteInstruction;
    if (actor.presentation_enabled == nullptr ||
        !request.access.presentation_enabled_writable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            presentation_enabled_write_typed_stop;
        return result;
    }
    *actor.presentation_enabled = 1U;
    record_access(
        result,
        LegacyBattleActorPresentationActivationAccess::
            presentation_enabled_write
    );
    ++result.presentation_enabled_writes;

    if (result.original_marker == 0U) {
        result.return_eip = kMarkerWriteInstruction;
        if (!request.access.marker_writable) {
            result.status = LegacyBattleActorPresentationActivationStatus::
                marker_write_typed_stop;
            return result;
        }
        *actor.marker = 6U;
        record_access(
            result, LegacyBattleActorPresentationActivationAccess::marker_write
        );
        ++result.marker_writes;
    }

    result.return_eip = kSourceRuntimeValueReadInstruction;
    if (actor.source_runtime_value == nullptr ||
        !request.access.source_runtime_value_readable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            source_runtime_value_read_typed_stop;
        return result;
    }
    result.source_runtime_value = *actor.source_runtime_value;
    record_access(
        result,
        LegacyBattleActorPresentationActivationAccess::source_runtime_value_read
    );
    ++result.source_runtime_value_reads;
    result.flags = subtract_flags(result.source_runtime_value, 1U);
    result.flags_known = true;

    if (result.source_runtime_value == 1U) {
        result.return_eip = kLiveRecordTokenReadInstruction;
        if (actor.live_record_token == nullptr ||
            !request.access.live_record_token_readable) {
            result.status = LegacyBattleActorPresentationActivationStatus::
                live_record_token_read_typed_stop;
            return result;
        }
        result.resolved_live_record_token = *actor.live_record_token;
        result.return_ecx = result.resolved_live_record_token;
        result.live_record_value_token =
            result.resolved_live_record_token + kLiveRecordValueOffset;
        record_access(
            result,
            LegacyBattleActorPresentationActivationAccess::
                live_record_token_read
        );
        ++result.live_record_token_reads;
        result.flags = test_flags(result.return_ecx);
        result.flags_known = true;

        if (result.resolved_live_record_token != 0U) {
            result.return_eip = kLiveRecordValueWriteInstruction;
            if (actor.live_record_value_04 == nullptr ||
                !request.access.live_record_value_writable) {
                result.status = LegacyBattleActorPresentationActivationStatus::
                    live_record_value_write_typed_stop;
                return result;
            }
            *actor.live_record_value_04 &= 0xFFFF0000U;
            record_access(
                result,
                LegacyBattleActorPresentationActivationAccess::
                    live_record_value_write
            );
            ++result.live_record_value_writes;
        }
    }

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorPresentationActivationStatus::
            return_address_read_typed_stop;
        return result;
    }
    result.stack_read_tokens[result.stack_read_count] = request.entry_esp;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    ++result.return_address_reads;
    result.return_esp += 8U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

bool execute_legacy_battle_actor_presentation_activation_call(
    const LegacyBattleActorPresentationActivationOwners& owners,
    LegacyBattleActorPresentationActivationCallTrace& trace,
    const LegacyBattleActorPresentationActivationCallRequests& requests,
    const u32 actor_token,
    const u32 value,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    const std::size_t index = trace.calls;
    if (index >= trace.call_addresses.size()) {
        return false;
    }
    trace.call_addresses[index] = call_address;
    trace.return_addresses[index] = return_address;
    auto request = requests.calls[index];
    request.actor_token = actor_token;
    request.value = value;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;
    trace.last = activate_legacy_battle_actor_presentation(
        resolve_legacy_battle_actor_presentation_activation(
            owners, actor_token
        ),
        request
    );
    ++trace.calls;
    return trace.last.status ==
        LegacyBattleActorPresentationActivationStatus::completed;
}

}  // namespace openswd3::battle
