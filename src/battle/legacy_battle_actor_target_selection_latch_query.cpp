#include "openswd3/battle/legacy_battle_actor_target_selection_latch_query.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kTargetSelectionLatchOffset = 0x00002AA8U;

}  // namespace

LegacyBattleActorTargetSelectionLatchQueryResult
query_legacy_battle_actor_target_selection_latch(
    const LegacyBattleActorTargetSelectionLatchView actor,
    const LegacyBattleActorTargetSelectionLatchQueryRequest& request
) noexcept {
    LegacyBattleActorTargetSelectionLatchQueryResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .target_selection_latch_field_token =
            request.actor_token + kTargetSelectionLatchOffset,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (actor.target_selection_latch == nullptr ||
        !request.access.target_selection_latch_readable) {
        result.status = LegacyBattleActorTargetSelectionLatchQueryStatus::
            target_selection_latch_read_typed_stop;
        return result;
    }

    result.return_eax = *actor.target_selection_latch;
    ++result.target_selection_latch_reads;

    result.return_eip =
        kLegacyBattleActorTargetSelectionLatchQueryReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionLatchQueryStatus::
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

bool execute_legacy_battle_actor_target_selection_latch_query_call(
    LegacyBattleActorTargetSelectionLatchQueryTrace& trace,
    const LegacyBattleActorTargetSelectionLatchQueryCallRequests& requests,
    const LegacyBattleActorTargetSelectionLatchOwners& owners,
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
        : LegacyBattleActorTargetSelectionLatchQueryRequest{};
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
    trace.last = query_legacy_battle_actor_target_selection_latch(
        resolve_legacy_battle_actor_target_selection_latch(owners, actor_token),
        request
    );
    return trace.last.status ==
        LegacyBattleActorTargetSelectionLatchQueryStatus::completed;
}

}  // namespace openswd3::battle
