#include "openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

inline constexpr u32 kCountOffset = 0x00002A76U;
inline constexpr u32 kReturnInstruction = 0x00478AB7U;

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

}  // namespace

LegacyBattleActorTargetSelectionCountQueryView
resolve_legacy_battle_actor_target_selection_count_query(
    const LegacyBattleActorTargetSelectionCountQueryOwners& owners,
    const u32 actor_token
) noexcept {
    const auto actor =
        resolve_legacy_battle_actor_target_selection_count_increment(
            owners, actor_token
        );
    return {.target_selection_count = actor.target_selection_count};
}

LegacyBattleActorTargetSelectionCountQueryResult
query_legacy_battle_actor_target_selection_count(
    const LegacyBattleActorTargetSelectionCountQueryView actor,
    const LegacyBattleActorTargetSelectionCountQueryRequest& request
) noexcept {
    LegacyBattleActorTargetSelectionCountQueryResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .field_token = request.actor_token + kCountOffset,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (actor.target_selection_count == nullptr ||
        !request.access.count_readable) {
        result.status = LegacyBattleActorTargetSelectionCountQueryStatus::
            count_read_typed_stop;
        return result;
    }

    result.count_value = *actor.target_selection_count;
    ++result.field_reads;
    result.return_eax = replace_low_word(result.return_eax, result.count_value);

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionCountQueryStatus::
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

bool execute_legacy_battle_actor_target_selection_count_query_call(
    LegacyBattleActorTargetSelectionCountQueryTrace& trace,
    const LegacyBattleActorTargetSelectionCountQueryCallRequests& requests,
    const LegacyBattleActorTargetSelectionCountQueryOwners& owners,
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
        : LegacyBattleActorTargetSelectionCountQueryRequest{};
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
    trace.last = query_legacy_battle_actor_target_selection_count(
        resolve_legacy_battle_actor_target_selection_count_query(
            owners, actor_token
        ),
        request
    );
    return trace.last.status ==
        LegacyBattleActorTargetSelectionCountQueryStatus::completed;
}

}  // namespace openswd3::battle
