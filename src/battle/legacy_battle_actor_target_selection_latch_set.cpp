#include "openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kTargetSelectionLatchOffset = 0x00002AA8U;

[[nodiscard]] bool resolve_index(
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

LegacyBattleActorTargetSelectionLatchView
resolve_legacy_battle_actor_target_selection_latch(
    const LegacyBattleActorTargetSelectionLatchOwners& owners,
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
        owners.startup != nullptr &&
        owners.startup->group_a_runtime_reset != nullptr) {
        return {
            .target_selection_latch =
                &(*owners.startup->group_a_runtime_reset)[index]
                     .target_selection_latch,
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
        return {
            .target_selection_latch =
                &(*owners.startup->group_b_lifecycle)[index]
                     .runtime_reset.target_selection_latch,
        };
    }

    return {};
}

LegacyBattleActorTargetSelectionLatchSetResult
set_legacy_battle_actor_target_selection_latch(
    const LegacyBattleActorTargetSelectionLatchView actor,
    const LegacyBattleActorTargetSelectionLatchSetRequest& request
) noexcept {
    LegacyBattleActorTargetSelectionLatchSetResult result{
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
        !request.access.target_selection_latch_writable) {
        result.status = LegacyBattleActorTargetSelectionLatchSetStatus::
            target_selection_latch_write_typed_stop;
        return result;
    }

    *actor.target_selection_latch = 1U;
    ++result.target_selection_latch_writes;

    result.return_eip =
        kLegacyBattleActorTargetSelectionLatchSetReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionLatchSetStatus::
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

bool execute_legacy_battle_actor_target_selection_latch_set_call(
    LegacyBattleActorTargetSelectionLatchSetTrace& trace,
    const LegacyBattleActorTargetSelectionLatchSetCallRequests& requests,
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
        : LegacyBattleActorTargetSelectionLatchSetRequest{};
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
    trace.last = set_legacy_battle_actor_target_selection_latch(
        resolve_legacy_battle_actor_target_selection_latch(owners, actor_token),
        request
    );
    return trace.last.status ==
        LegacyBattleActorTargetSelectionLatchSetStatus::completed;
}

}  // namespace openswd3::battle
