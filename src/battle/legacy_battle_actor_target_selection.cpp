#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

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

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

void append_access(
    LegacyBattleActorTargetSelectionResult& result,
    const LegacyBattleActorTargetSelectionAccessKind kind,
    const u32 instruction_address,
    const u32 memory_address,
    const u32 value,
    const compat::u8 width,
    const bool write
) noexcept {
    result.accesses[result.access_count++] = {
        .kind = kind,
        .instruction_address = instruction_address,
        .memory_address = memory_address,
        .value = value,
        .width = width,
        .write = write,
    };
}

void stop(
    LegacyBattleActorTargetSelectionResult& result,
    const LegacyBattleActorTargetSelectionStatus status,
    const u32 instruction_address
) noexcept {
    result.status = status;
    result.stop_instruction = instruction_address;
    result.return_eip = instruction_address;
}

}  // namespace

LegacyBattleActorTargetSelectionView
resolve_legacy_battle_actor_target_selection(
    const LegacyBattleActorTargetSelectionOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (owners.action != nullptr && owners.startup != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            owners.action->group_a_action_execution.size(),
            index
        )) {
        auto& execution = owners.action->group_a_action_execution[index];
        return {
            .idle_state_latch = &execution.idle_state_latch,
            .action_target = &execution.action_target,
            .progress = &owners.startup->party[index].progress.progress,
        };
    }

    if (owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            owners.startup->enemies.size(),
            index
        )) {
        auto& execution =
            (*owners.startup->group_b_lifecycle)[index].action_execution;
        return {
            .idle_state_latch = &execution.idle_state_latch,
            .action_target = &execution.action_target,
            .progress = &owners.startup->enemies[index].progress.progress,
        };
    }

    return {};
}

LegacyBattleActorTargetSelectionResult
apply_legacy_battle_actor_target_selection(
    const LegacyBattleActorTargetSelectionView& actor,
    const LegacyBattleActorTargetSelectionRequest& request
) noexcept {
    LegacyBattleActorTargetSelectionResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (!request.argument_read_accessible) {
        stop(
            result,
            LegacyBattleActorTargetSelectionStatus::argument_read_typed_stop,
            0x00478A70U
        );
        return result;
    }

    result.argument_value = request.argument_value;
    replace_low_word(result.return_eax, request.argument_value);
    append_access(
        result,
        LegacyBattleActorTargetSelectionAccessKind::argument_read,
        0x00478A70U,
        request.entry_esp + 4U,
        request.argument_value,
        2U,
        false
    );

    if (actor.idle_state_latch == nullptr ||
        !request.idle_state_write_accessible) {
        stop(
            result,
            LegacyBattleActorTargetSelectionStatus::idle_state_write_typed_stop,
            0x00478A75U
        );
        return result;
    }

    *actor.idle_state_latch = 1U;
    append_access(
        result,
        LegacyBattleActorTargetSelectionAccessKind::idle_state_write,
        0x00478A75U,
        request.actor_token + 0x2AB4U,
        1U,
        4U,
        true
    );

    if (actor.action_target == nullptr ||
        !request.action_target_write_accessible) {
        stop(
            result,
            LegacyBattleActorTargetSelectionStatus::
                action_target_write_typed_stop,
            0x00478A7FU
        );
        return result;
    }

    *actor.action_target = request.argument_value;
    append_access(
        result,
        LegacyBattleActorTargetSelectionAccessKind::action_target_write,
        0x00478A7FU,
        request.actor_token + 0x29A2U,
        request.argument_value,
        2U,
        true
    );

    if (actor.progress == nullptr || !request.progress_write_accessible) {
        stop(
            result,
            LegacyBattleActorTargetSelectionStatus::progress_write_typed_stop,
            0x00478A86U
        );
        return result;
    }

    replace_low_word(*actor.progress, 0U);
    append_access(
        result,
        LegacyBattleActorTargetSelectionAccessKind::progress_write,
        0x00478A86U,
        request.actor_token + 0x2A12U,
        0U,
        2U,
        true
    );

    if (!request.return_address_read_accessible) {
        stop(
            result,
            LegacyBattleActorTargetSelectionStatus::
                return_address_read_typed_stop,
            0x00478A8FU
        );
        return result;
    }

    append_access(
        result,
        LegacyBattleActorTargetSelectionAccessKind::return_address_read,
        0x00478A8FU,
        request.entry_esp,
        request.return_address,
        4U,
        false
    );
    result.return_esp = request.entry_esp + 8U;
    result.return_eip = request.return_address;
    result.returned = true;
    return result;
}

bool execute_legacy_battle_actor_target_selection_call(
    LegacyBattleActorTargetSelectionTrace& trace,
    const LegacyBattleActorTargetSelectionRequestList& requests,
    const LegacyBattleActorTargetSelectionOwners& owners,
    const u32 call_address,
    const u32 return_address,
    const u32 actor_token,
    const u16 argument_value,
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
        : LegacyBattleActorTargetSelectionRequest{};
    request.call_address = call_address;
    request.return_address = return_address;
    request.actor_token = actor_token;
    request.argument_value = argument_value;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;

    trace.call_addresses[call_index] = call_address;
    trace.return_addresses[call_index] = return_address;
    trace.actor_tokens[call_index] = actor_token;
    trace.argument_values[call_index] = argument_value;
    ++trace.calls;
    trace.last = apply_legacy_battle_actor_target_selection(
        resolve_legacy_battle_actor_target_selection(owners, actor_token),
        request
    );
    return trace.last.status ==
        LegacyBattleActorTargetSelectionStatus::completed;
}

}  // namespace openswd3::battle
