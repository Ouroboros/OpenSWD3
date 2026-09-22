#include "openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kStartGateLatchOffset = 0x00002AE0U;

}  // namespace

LegacyBattleActorStartGateLatchQueryResult
query_legacy_battle_actor_start_gate_latch(
    const LegacyBattleActorStartGateIncrementView actor,
    const LegacyBattleActorStartGateLatchQueryRequest& request
) noexcept {
    LegacyBattleActorStartGateLatchQueryResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .start_gate_latch_field_token =
            request.actor_token + kStartGateLatchOffset,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (actor.start_gate_latch == nullptr ||
        !request.access.start_gate_latch_readable) {
        result.status = LegacyBattleActorStartGateLatchQueryStatus::
            start_gate_latch_read_typed_stop;
        return result;
    }

    result.return_eax = *actor.start_gate_latch;
    ++result.start_gate_latch_reads;

    result.return_eip = kLegacyBattleActorStartGateLatchQueryReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorStartGateLatchQueryStatus::
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

bool execute_legacy_battle_actor_start_gate_latch_query_call(
    LegacyBattleActorStartGateLatchQueryTrace& trace,
    const LegacyBattleActorStartGateLatchQueryCallRequests& requests,
    const LegacyBattleActorStartGateIncrementOwners& owners,
    const u32 call_address,
    const u32 return_address,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const std::size_t request_offset
) noexcept {
    const std::size_t call_index = trace.calls;
    const std::size_t request_index = request_offset + call_index;
    auto request = request_index < requests.count
        ? requests.calls[request_index]
        : LegacyBattleActorStartGateLatchQueryRequest{};
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
    trace.last = query_legacy_battle_actor_start_gate_latch(
        resolve_legacy_battle_actor_start_gate_increment(owners, actor_token),
        request
    );
    return trace.last.status ==
        LegacyBattleActorStartGateLatchQueryStatus::completed;
}

}  // namespace openswd3::battle
