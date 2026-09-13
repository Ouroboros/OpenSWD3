#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

inline constexpr u32 kField26c0Offset = 0x000026C0U;
inline constexpr u32 kField26b8Offset = 0x000026B8U;
inline constexpr u32 kSummonCompletionWordOffset = 0x00002A78U;
inline constexpr u32 kSpecialTargetCommandCursorOffset = 0x00000542U;
inline constexpr u32 kGateMask = 0x02000000U;
inline constexpr u32 kHighBitMask = 0x80000000U;
inline constexpr u32 kField26b8ReadInstruction = 0x0047878CU;
inline constexpr u32 kSummonCompletionWordWriteInstruction = 0x0047879BU;
inline constexpr u32 kSpecialTargetCommandCursorWriteInstruction = 0x004787A2U;
inline constexpr u32 kField26b8WriteInstruction = 0x004787AEU;
inline constexpr u32 kReturnInstruction = 0x004787B4U;

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
logical_flags(const u32 value) noexcept {
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
    LegacyBattleActorField26b8HighBitSetResult& result,
    const LegacyBattleActorField26b8HighBitSetAccess access
) noexcept {
    result.actor_accesses[result.actor_access_count] = access;
    ++result.actor_access_count;
}

}  // namespace

LegacyBattleActorField26b8HighBitSetView
resolve_legacy_battle_actor_field_26b8_high_bit_set(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
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
        owners.action != nullptr) {
        auto& actor = owners.action->group_a_action_execution[index];
        if (owners.startup != nullptr) {
            owners.startup->party[index].progress.field_26c0.alias(
                actor.field_26c0
            );
        }
        return {
            .field_26c0 = actor.field_26c0.data(),
            .field_26b8 = &actor.field_26b8,
            .summon_completion_word = &actor.summon_completion_word,
            .special_target_command_cursor =
                &actor.special_target_action_record.command_cursor,
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
        auto& actor =
            (*owners.startup->group_b_lifecycle)[index].action_execution;
        owners.startup->enemies[index].progress.field_26c0.alias(
            actor.field_26c0
        );
        return {
            .field_26c0 = actor.field_26c0.data(),
            .field_26b8 = &actor.field_26b8,
            .summon_completion_word = &actor.summon_completion_word,
            .special_target_command_cursor =
                &actor.special_target_action_record.command_cursor,
        };
    }

    return {};
}

LegacyBattleActorField26b8HighBitSetView
resolve_legacy_battle_actor_field_26b8_high_bit_set(
    LegacyBattleGroupAActionExecutionState* const actor, const u32 actor_token
) noexcept {
    if (actor == nullptr || actor_token == 0U) {
        return {};
    }
    return {
        .field_26c0 = actor->field_26c0.data(),
        .field_26b8 = &actor->field_26b8,
        .summon_completion_word = &actor->summon_completion_word,
        .special_target_command_cursor =
            &actor->special_target_action_record.command_cursor,
    };
}

LegacyBattleActorField26b8HighBitSetResult
set_legacy_battle_actor_field_26b8_high_bit(
    const LegacyBattleActorField26b8HighBitSetView actor,
    const LegacyBattleActorField26b8HighBitSetRequest& request
) noexcept {
    LegacyBattleActorField26b8HighBitSetResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .field_26c0_token = request.actor_token + kField26c0Offset,
        .field_26b8_token = request.actor_token + kField26b8Offset,
        .summon_completion_word_token =
            request.actor_token + kSummonCompletionWordOffset,
        .special_target_command_cursor_token =
            request.actor_token + kSpecialTargetCommandCursorOffset,
        .return_address_token = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.field_26c0 == nullptr || !request.access.field_26c0_readable) {
        result.status = LegacyBattleActorField26b8HighBitSetStatus::
            field_26c0_read_typed_stop;
        return result;
    }
    result.field_26c0_read_value = *actor.field_26c0;
    ++result.field_26c0_reads;
    record_access(
        result, LegacyBattleActorField26b8HighBitSetAccess::field_26c0_read
    );
    result.flags = logical_flags(result.field_26c0_read_value & kGateMask);
    result.flags_known = true;
    if ((result.field_26c0_read_value & kGateMask) != 0U) {
        result.gate_blocked = true;
        result.return_eip = kReturnInstruction;
    } else {
        result.return_eip = kField26b8ReadInstruction;
        if (actor.field_26b8 == nullptr ||
            !request.access.field_26b8_readable) {
            result.status = LegacyBattleActorField26b8HighBitSetStatus::
                field_26b8_read_typed_stop;
            return result;
        }
        result.field_26b8_read_value = *actor.field_26b8;
        result.return_eax = result.field_26b8_read_value;
        ++result.field_26b8_reads;
        record_access(
            result, LegacyBattleActorField26b8HighBitSetAccess::field_26b8_read
        );
        result.flags =
            logical_flags(result.field_26b8_read_value & kHighBitMask);
        result.high_bit_was_set =
            (result.field_26b8_read_value & kHighBitMask) != 0U;

        if (result.high_bit_was_set) {
            result.return_edx = 0U;
            result.flags = logical_flags(0U);
            result.return_eip = kSummonCompletionWordWriteInstruction;
            if (actor.summon_completion_word == nullptr ||
                !request.access.summon_completion_word_writable) {
                result.status = LegacyBattleActorField26b8HighBitSetStatus::
                    summon_completion_word_write_typed_stop;
                return result;
            }
            *actor.summon_completion_word = 0U;
            ++result.summon_completion_word_writes;
            record_access(
                result,
                LegacyBattleActorField26b8HighBitSetAccess::
                    summon_completion_word_write
            );

            result.return_eip = kSpecialTargetCommandCursorWriteInstruction;
            if (actor.special_target_command_cursor == nullptr ||
                !request.access.special_target_command_cursor_writable) {
                result.status = LegacyBattleActorField26b8HighBitSetStatus::
                    special_target_command_cursor_write_typed_stop;
                return result;
            }
            *actor.special_target_command_cursor = 0U;
            ++result.special_target_command_cursor_writes;
            record_access(
                result,
                LegacyBattleActorField26b8HighBitSetAccess::
                    special_target_command_cursor_write
            );
        }

        result.return_eax |= kHighBitMask;
        result.flags = logical_flags(result.return_eax);
        result.return_eip = kField26b8WriteInstruction;
        if (!request.access.field_26b8_writable) {
            result.status = LegacyBattleActorField26b8HighBitSetStatus::
                field_26b8_write_typed_stop;
            return result;
        }
        *actor.field_26b8 = result.return_eax;
        result.field_26b8_write_value = result.return_eax;
        ++result.field_26b8_writes;
        record_access(
            result, LegacyBattleActorField26b8HighBitSetAccess::field_26b8_write
        );
        result.return_eip = kReturnInstruction;
    }

    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorField26b8HighBitSetStatus::
            return_address_read_typed_stop;
        return result;
    }
    result.stack_read_tokens[result.stack_read_count] = request.entry_esp;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    ++result.return_address_reads;
    result.return_esp += 4U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

void append_legacy_battle_actor_field_26b8_high_bit_set_trace(
    LegacyBattleActorField26b8HighBitSetCallTrace& destination,
    const LegacyBattleActorField26b8HighBitSetCallTrace& nested
) noexcept {
    for (u32 index = 0U; index < nested.calls; ++index) {
        destination.return_addresses[destination.calls] =
            nested.return_addresses[index];
        ++destination.calls;
    }
    if (nested.calls != 0U) {
        destination.last = nested.last;
    }
}

bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    auto request = requests.calls[trace.calls];
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;
    trace.return_addresses[trace.calls] = return_address;
    trace.last = set_legacy_battle_actor_field_26b8_high_bit(
        resolve_legacy_battle_actor_field_26b8_high_bit_set(
            owners, actor_token
        ),
        request
    );
    ++trace.calls;
    return trace.last.status ==
        LegacyBattleActorField26b8HighBitSetStatus::completed;
}

bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    const u32 actor_token,
    const u32 return_address
) noexcept {
    auto request = requests.calls[trace.calls];
    request.actor_token = actor_token;
    request.entry_return_address = return_address;
    trace.return_addresses[trace.calls] = return_address;
    trace.last = set_legacy_battle_actor_field_26b8_high_bit(
        resolve_legacy_battle_actor_field_26b8_high_bit_set(
            owners, actor_token
        ),
        request
    );
    ++trace.calls;
    return trace.last.status ==
        LegacyBattleActorField26b8HighBitSetStatus::completed;
}

bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    LegacyBattleGroupAActionExecutionState* const actor,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    auto request = requests.calls[trace.calls];
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;
    trace.return_addresses[trace.calls] = return_address;
    trace.last = set_legacy_battle_actor_field_26b8_high_bit(
        resolve_legacy_battle_actor_field_26b8_high_bit_set(actor, actor_token),
        request
    );
    ++trace.calls;
    return trace.last.status ==
        LegacyBattleActorField26b8HighBitSetStatus::completed;
}

}  // namespace openswd3::battle
