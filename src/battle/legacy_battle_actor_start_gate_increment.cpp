#include "openswd3/battle/legacy_battle_actor_start_gate_increment.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

inline constexpr u32 kStartGateOffset = 0x00002A74U;
inline constexpr u32 kStartGateLatchOffset = 0x00002AE0U;
inline constexpr u32 kStartGateLatchWriteInstruction = 0x00478AC7U;
inline constexpr u32 kReturnInstruction = 0x00478AD1U;

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
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

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags increment_flags(
    const u16 previous,
    const u16 incremented,
    const LegacyBattleActorCoordinateFlags& entry
) noexcept {
    return {
        .carry = entry.carry,
        .parity = has_even_parity(incremented),
        .auxiliary_carry = ((previous ^ incremented) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = incremented == 0U,
        .sign = (incremented & 0x8000U) != 0U,
        .overflow = previous == 0x7FFFU,
    };
}

}  // namespace

LegacyBattleActorStartGateIncrementView
resolve_legacy_battle_actor_start_gate_increment(
    const LegacyBattleActorStartGateIncrementOwners& owners,
    const u32 actor_token
) noexcept {
    std::size_t index{};
    if (owners.action != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            owners.action->group_a_action_execution.size(),
            index
        )) {
        auto& actor = owners.action->group_a_action_execution[index];
        return {
            .start_gate = &actor.start_gate,
            .start_gate_latch = &actor.start_gate_latch,
        };
    }

    if (owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            owners.startup->group_b_lifecycle->size(),
            index
        )) {
        auto& actor =
            (*owners.startup->group_b_lifecycle)[index].action_execution;
        return {
            .start_gate = &actor.start_gate,
            .start_gate_latch = &actor.start_gate_latch,
        };
    }

    return {};
}

LegacyBattleActorStartGateIncrementResult
increment_legacy_battle_actor_start_gate(
    const LegacyBattleActorStartGateIncrementView actor,
    const LegacyBattleActorStartGateIncrementRequest& request
) noexcept {
    LegacyBattleActorStartGateIncrementResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .start_gate_field_token = request.actor_token + kStartGateOffset,
        .start_gate_latch_field_token =
            request.actor_token + kStartGateLatchOffset,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (actor.start_gate == nullptr || !request.access.start_gate_readable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_read_typed_stop;
        return result;
    }

    result.previous_start_gate = *actor.start_gate;
    ++result.start_gate_reads;
    result.incremented_start_gate =
        static_cast<u16>(result.previous_start_gate + 1U);
    if (!request.access.start_gate_writable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_write_typed_stop;
        return result;
    }

    *actor.start_gate = result.incremented_start_gate;
    ++result.start_gate_writes;
    result.flags = increment_flags(
        result.previous_start_gate,
        result.incremented_start_gate,
        request.entry_flags
    );

    result.return_eip = kStartGateLatchWriteInstruction;
    if (actor.start_gate_latch == nullptr ||
        !request.access.start_gate_latch_writable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_latch_write_typed_stop;
        return result;
    }

    *actor.start_gate_latch = 1U;
    ++result.start_gate_latch_writes;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            return_address_read_typed_stop;
        return result;
    }

    result.stack_reads[result.stack_read_count++] = request.return_address;
    ++result.return_address_reads;
    result.return_esp += 4U;
    result.return_eip = request.return_address;
    result.returned = true;
    return result;
}

bool execute_legacy_battle_actor_start_gate_increment_call(
    LegacyBattleActorStartGateIncrementTrace& trace,
    const LegacyBattleActorStartGateIncrementCallRequests& requests,
    const LegacyBattleActorStartGateIncrementOwners& owners,
    const u32 call_address,
    const u32 return_address,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    const std::size_t call_index = trace.calls;
    auto request = call_index < requests.count
        ? requests.calls[call_index]
        : LegacyBattleActorStartGateIncrementRequest{};
    request.call_address = call_address;
    request.return_address = return_address;
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;

    trace.call_addresses[call_index] = call_address;
    trace.return_addresses[call_index] = return_address;
    trace.actor_tokens[call_index] = actor_token;
    ++trace.calls;
    trace.last = increment_legacy_battle_actor_start_gate(
        resolve_legacy_battle_actor_start_gate_increment(owners, actor_token),
        request
    );
    return trace.last.status ==
        LegacyBattleActorStartGateIncrementStatus::completed;
}

}  // namespace openswd3::battle
