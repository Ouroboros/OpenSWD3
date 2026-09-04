#include "openswd3/battle/legacy_battle_actor_target_preparation.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kGroupBOneBasedToken = 0x005229E0U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;
inline constexpr u32 kSelectionWorkspaceBaseIndex = 10U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

}  // namespace

LegacyBattleActorTargetPreparationResult prepare_legacy_battle_actor_target(
    LegacyBattleActorTargetPreparationBindings bindings,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleActorTargetPreparationPort& port,
    const LegacyBattleActorTargetPreparationRequest& request
) {
    LegacyBattleActorTargetPreparationResult result;
    u32 eax = request.actor_code;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    u32 esi{};

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto invoke = [&](const LegacyBattleActorTargetPreparationCall call,
                            const u32 object_token,
                            const std::array<u32, 4>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_actor_target_preparation({
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };
    const auto prepare_group_b_query = [&](const u32 one_based_code) {
        eax = one_based_code * 0x565U;
        edx = one_based_code * 0x345U;
        ecx = kGroupBOneBasedToken + one_based_code * kGroupBStride;
        if (one_based_code == 0U || one_based_code > kGroupBCount) {
            result.status = LegacyBattleActorTargetPreparationStatus::
                group_b_actor_typed_stop;
            return false;
        }
        invoke(
            LegacyBattleActorTargetPreparationCall::query_group_b_completion,
            ecx
        );
        ++result.group_b_queries;
        return true;
    };

    bindings.debug_hotkeys.committed_actor_code = request.actor_code;
    ecx = request.actor_code - 8U;
    bindings.target_runtime.selected_action_kind = 1U;
    bindings.target_runtime.actor_commit_gate = 1U;
    eax = ecx * 0x3FU;
    const u32 workspace_index = kSelectionWorkspaceBaseIndex + ecx;
    if (workspace_index >= bindings.action.opponent_workspace.size()) {
        result.status = LegacyBattleActorTargetPreparationStatus::
            action_workspace_typed_stop;
        return finish();
    }
    bindings.action.opponent_workspace[workspace_index] = 1U;

    const u32 group_a_index = ecx;
    eax = group_a_index * 0xBCDU;
    const u32 group_a_token = kGroupABaseToken + group_a_index * kGroupAStride;
    if (group_a_index >= kGroupACount) {
        ecx = group_a_token;
        result.status =
            LegacyBattleActorTargetPreparationStatus::group_a_actor_typed_stop;
        return finish();
    }
    ecx = group_a_token;
    result.actor_availability_block =
        set_legacy_battle_actor_availability_block(
            &bindings.final_actor.group_a_availability_blocks[group_a_index],
            {
                .value = 1U,
                .actor_token = ecx,
                .entry_eax = eax,
                .entry_edx = edx,
            }
        );
    ++result.actor_availability_block_calls;
    eax = result.actor_availability_block.return_eax;
    ecx = result.actor_availability_block.return_ecx;
    edx = result.actor_availability_block.return_edx;
    if (result.actor_availability_block.status !=
        LegacyBattleActorAvailabilityBlockStatus::completed) {
        result.status = LegacyBattleActorTargetPreparationStatus::
            actor_availability_block_typed_stop;
        return finish();
    }

    eax = bindings.metrics.group_b_count;
    ecx = static_cast<u8>(bindings.action.opponent_processed_counter);
    if (signed_dword(ecx) >= signed_dword(eax)) {
        return finish();
    }

    eax = random.random_bounded(eax);
    ++result.random_calls;
    ++eax;
    bindings.final_actor.published_actor_code = eax;
    if (!prepare_group_b_query(eax)) {
        return finish();
    }
    if (eax != 1U) {
        return finish();
    }

    while (true) {
        ecx = bindings.final_actor.published_actor_code;
        eax = bindings.metrics.group_b_count;
        ++ecx;
        bindings.final_actor.published_actor_code = ecx;
        if (signed_dword(ecx) > signed_dword(eax)) {
            ecx = 1U;
            bindings.final_actor.published_actor_code = ecx;
        }
        ++esi;
        result.scanned_completed_targets = esi;
        if (signed_dword(esi) >= signed_dword(eax)) {
            return finish();
        }
        if (!prepare_group_b_query(ecx)) {
            return finish();
        }
        if (eax != 1U) {
            return finish();
        }
    }
}

}  // namespace openswd3::battle
