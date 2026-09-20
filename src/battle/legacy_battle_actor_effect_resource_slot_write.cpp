#include "openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

inline constexpr u32 kSlotsOffset = 0x000029C4U;
inline constexpr u32 kCursorOffset = 0x00002A7CU;
inline constexpr u32 kCursorReadInstruction = 0x004787D7U;
inline constexpr u32 kTargetWriteInstruction = 0x004787DEU;
inline constexpr u32 kReturnInstruction = 0x004787E6U;
inline constexpr u16 kMaximumCursor = 34U;

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

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
xor_zero_flags() noexcept {
    return {
        .carry = false,
        .parity = true,
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = true,
        .sign = false,
        .overflow = false,
    };
}

[[nodiscard]] bool execute_call(
    const LegacyBattleActorEffectResourceSlotWriteView actor,
    LegacyBattleActorEffectResourceSlotWriteCallTrace& trace,
    const LegacyBattleActorEffectResourceSlotWriteCallRequests& requests,
    const u32 actor_token,
    const u16 value,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const bool entry_overflow_defined
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
    request.entry_overflow_defined = entry_overflow_defined;
    trace.last = write_legacy_battle_actor_effect_resource_slot(actor, request);
    ++trace.calls;
    return trace.last.status ==
        LegacyBattleActorEffectResourceSlotWriteStatus::completed;
}

}  // namespace

LegacyBattleActorEffectResourceSlotWriteView
resolve_legacy_battle_actor_effect_resource_slot_write(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
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
        return {
            .slots = &actor.effect_resource_slots,
            .cursor = &actor.effect_resource_cursor,
            .execution_complete = &actor.execution_complete,
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
        return {
            .slots = &actor.effect_resource_slots,
            .cursor = &actor.effect_resource_cursor,
            .execution_complete = &actor.execution_complete,
        };
    }

    return {};
}

LegacyBattleActorEffectResourceSlotWriteView
resolve_legacy_battle_actor_effect_resource_slot_write(
    LegacyBattleGroupAActionExecutionState* const actor, const u32 actor_token
) noexcept {
    static_cast<void>(actor_token);
    if (actor == nullptr) {
        return {};
    }
    return {
        .slots = &actor->effect_resource_slots,
        .cursor = &actor->effect_resource_cursor,
        .execution_complete = &actor->execution_complete,
    };
}

void append_legacy_battle_actor_effect_resource_slot_write_trace(
    LegacyBattleActorEffectResourceSlotWriteCallTrace& destination,
    const LegacyBattleActorEffectResourceSlotWriteCallTrace& nested
) noexcept {
    const std::size_t available = destination.call_addresses.size() -
        std::min<std::size_t>(destination.calls,
                              destination.call_addresses.size());
    const std::size_t count = std::min<std::size_t>(nested.calls, available);
    for (std::size_t index = 0U; index < count; ++index) {
        destination.call_addresses[destination.calls + index] =
            nested.call_addresses[index];
        destination.return_addresses[destination.calls + index] =
            nested.return_addresses[index];
    }
    destination.calls += static_cast<u32>(count);
    if (count != 0U) {
        destination.last = nested.last;
    }
}

LegacyBattleActorEffectResourceSlotWriteResult
write_legacy_battle_actor_effect_resource_slot(
    const LegacyBattleActorEffectResourceSlotWriteView actor,
    const LegacyBattleActorEffectResourceSlotWriteRequest& request
) noexcept {
    LegacyBattleActorEffectResourceSlotWriteResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .argument_token = request.entry_esp + 4U,
        .cursor_token = request.actor_token + kCursorOffset,
        .return_address_token = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .overflow_defined = request.entry_overflow_defined,
        .flags = request.entry_flags,
    };

    if (!request.access.argument_readable) {
        result.status = LegacyBattleActorEffectResourceSlotWriteStatus::
            argument_read_typed_stop;
        return result;
    }
    result.argument_value = request.value;
    result.return_edx = (result.return_edx & 0xFFFF0000U) | request.value;
    result.stack_read_tokens[result.stack_read_count] = result.argument_token;
    result.stack_reads[result.stack_read_count] = request.value;
    ++result.stack_read_count;
    ++result.argument_reads;

    result.return_eax = 0U;
    result.flags = xor_zero_flags();
    result.flags_known = true;
    result.overflow_defined = true;
    result.return_eip = kCursorReadInstruction;

    if (actor.cursor == nullptr || !request.access.cursor_readable) {
        result.status = LegacyBattleActorEffectResourceSlotWriteStatus::
            cursor_read_typed_stop;
        return result;
    }
    result.cursor_value = *actor.cursor;
    result.return_eax = result.cursor_value;
    result.actor_accesses[result.actor_access_count] =
        LegacyBattleActorEffectResourceSlotWriteAccess::cursor_read;
    ++result.actor_access_count;
    ++result.cursor_reads;

    result.target_token = request.actor_token + kSlotsOffset +
        static_cast<u32>(result.cursor_value) * 2U;
    result.return_eip = kTargetWriteInstruction;
    if (actor.slots == nullptr || result.cursor_value >= actor.slots->size() ||
        !request.access.target_writable) {
        result.status = LegacyBattleActorEffectResourceSlotWriteStatus::
            target_write_typed_stop;
        return result;
    }
    (*actor.slots)[result.cursor_value] = request.value;
    result.target_write_value = request.value;
    result.actor_accesses[result.actor_access_count] =
        LegacyBattleActorEffectResourceSlotWriteAccess::target_write;
    ++result.actor_access_count;
    ++result.target_writes;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorEffectResourceSlotWriteStatus::
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

bool execute_legacy_battle_actor_effect_resource_slot_write_call(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    LegacyBattleActorEffectResourceSlotWriteCallTrace& trace,
    const LegacyBattleActorEffectResourceSlotWriteCallRequests& requests,
    const u32 actor_token,
    const u16 value,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const bool entry_overflow_defined
) noexcept {
    return execute_call(
        resolve_legacy_battle_actor_effect_resource_slot_write(
            owners, actor_token
        ),
        trace,
        requests,
        actor_token,
        value,
        entry_eax,
        entry_edx,
        call_address,
        return_address,
        entry_flags,
        entry_flags_known,
        entry_overflow_defined
    );
}

bool execute_legacy_battle_actor_effect_resource_slot_write_call(
    LegacyBattleGroupAActionExecutionState* const actor,
    LegacyBattleActorEffectResourceSlotWriteCallTrace& trace,
    const LegacyBattleActorEffectResourceSlotWriteCallRequests& requests,
    const u32 actor_token,
    const u16 value,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const bool entry_overflow_defined
) noexcept {
    return execute_call(
        resolve_legacy_battle_actor_effect_resource_slot_write(
            actor, actor_token
        ),
        trace,
        requests,
        actor_token,
        value,
        entry_eax,
        entry_edx,
        call_address,
        return_address,
        entry_flags,
        entry_flags_known,
        entry_overflow_defined
    );
}

void reset_legacy_battle_actor_effect_resource_slots(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    const u32 actor_token
) noexcept {
    auto actor = resolve_legacy_battle_actor_effect_resource_slot_write(
        owners, actor_token
    );
    if (actor.slots != nullptr) {
        actor.slots->fill(0U);
    }
    if (actor.cursor != nullptr) {
        *actor.cursor = 0U;
    }
}

void reset_legacy_battle_actor_effect_resource_slots(
    LegacyBattleGroupAActionExecutionState* const actor
) noexcept {
    if (actor == nullptr) {
        return;
    }
    actor->effect_resource_slots.fill(0U);
    actor->effect_resource_cursor = 0U;
}

namespace {

void synchronize_cursor_update(
    const LegacyBattleActorEffectResourceSlotWriteView actor, const u32 mode
) noexcept {
    if (actor.cursor != nullptr) {
        if (mode == 1U) {
            *actor.cursor = static_cast<u16>(*actor.cursor + 1U);
            if (*actor.cursor > kMaximumCursor) {
                *actor.cursor = kMaximumCursor;
            }
        } else {
            *actor.cursor = static_cast<u16>(*actor.cursor - 1U);
        }
    }
    if (actor.execution_complete != nullptr) {
        *actor.execution_complete = mode;
    }
}

}  // namespace

void synchronize_legacy_battle_actor_effect_resource_cursor_update(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    const u32 actor_token,
    const u32 mode
) noexcept {
    synchronize_cursor_update(
        resolve_legacy_battle_actor_effect_resource_slot_write(
            owners, actor_token
        ),
        mode
    );
}

void synchronize_legacy_battle_actor_effect_resource_cursor_update(
    LegacyBattleGroupAActionExecutionState* const actor, const u32 mode
) noexcept {
    synchronize_cursor_update(
        resolve_legacy_battle_actor_effect_resource_slot_write(actor, 0U), mode
    );
}

}  // namespace openswd3::battle
