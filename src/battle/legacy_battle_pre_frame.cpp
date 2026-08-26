#include "openswd3/battle/legacy_battle_pre_frame.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

inline constexpr u32 kLegacyBattleGroupBSentinelToken =
    kLegacyBattleActionGroupBBaseToken - kLegacyBattleActionGroupBStride;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 actor_code) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        (actor_code - 8U) * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32
group_b_token_from_one_based(const u32 index) noexcept {
    return kLegacyBattleGroupBSentinelToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

}  // namespace

LegacyBattlePreFrameResult advance_legacy_battle_pre_frame(
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattlePreFramePort& port,
    const u32 entry_ecx,
    const u32 entry_edx
) {
    LegacyBattlePreFrameResult result;
    u32 eax = port.battle_terminal_latch();
    u32 ecx = entry_ecx;
    u32 edx = entry_edx;
    const auto finish = [&]() {
        result.return_value = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto invoke = [&](const LegacyBattlePreFrameCall call,
                            const u32 actor_token,
                            const u32 argument = 0U) {
        ++result.port_calls;
        const auto reply = port.invoke_pre_frame(
            {.call = call, .actor_token = actor_token, .argument = argument}
        );
        if (reply.publish_group_b_count) {
            port.actor_metric_state().group_b_count = reply.group_b_count;
        }
        if (reply.publish_secondary_actor_code) {
            final_actor.secondary_actor_code = reply.secondary_actor_code;
        }
        if (reply.publish_source_actor_code) {
            final_actor.source_actor_code = reply.source_actor_code;
        }
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        return reply;
    };

    if (eax != 1U) {
        return finish();
    }

    eax = final_actor.active_actor_code;
    if (eax == 0U) {
        return finish();
    }

    edx = port.battle_message_state();
    ecx = 3U;
    if (edx == ecx) {
        return finish();
    }

    edx = final_actor.source_actor_code;
    const bool source_available = edx != 0xFFFFFFFFU;
    final_actor.action_execution_active = 1U;
    const u32 workspace_index = eax + 2U;
    if (workspace_index >= action.opponent_workspace.size()) {
        result.status =
            LegacyBattlePreFrameStatus::opponent_workspace_typed_stop;
        return finish();
    }
    action.opponent_workspace[workspace_index] = 1U;

    if (!source_available) {
        final_actor.pre_frame_gate_a = 1U;
        port.battle_message_state() = ecx;
        return finish();
    }

    const u32 actor_code = eax;
    final_actor.secondary_actor_code = actor_code;
    final_actor.active_actor_code = 0U;
    final_actor.auxiliary_gate = 1U;
    static_cast<void>(invoke(
        LegacyBattlePreFrameCall::configure_group_a_actor,
        group_a_token(actor_code),
        1U
    ));

    ecx = final_actor.source_actor_code;
    edx = final_actor.secondary_actor_code;
    final_actor.published_actor_code = ecx;
    port.battle_message_state() = 0U;
    final_actor.pre_frame_gate_b = 0U;
    final_actor.pre_frame_gate_a = 0U;
    const auto current = invoke(
        LegacyBattlePreFrameCall::query_group_a_actor, group_a_token(edx)
    );
    if (current.eax == 1U) {
        const u32 current_actor = final_actor.secondary_actor_code;
        const u32 current_workspace_index = current_actor + 2U;
        final_actor.action_execution_active = 5U;
        if (current_workspace_index >= action.opponent_workspace.size()) {
            result.status =
                LegacyBattlePreFrameStatus::opponent_workspace_typed_stop;
            return finish();
        }
        action.opponent_workspace[current_workspace_index] = 5U;
        static_cast<void>(invoke(
            LegacyBattlePreFrameCall::notify_group_a_actor,
            group_a_token(current_actor)
        ));
        static_cast<void>(invoke(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            group_a_token(final_actor.secondary_actor_code),
            1U
        ));
        eax = final_actor.secondary_actor_code;
        edx = eax * 5U - 40U;
        if (edx >= final_actor.actor_runtime_records.size() * 5U) {
            result.status =
                LegacyBattlePreFrameStatus::actor_runtime_record_typed_stop;
            return finish();
        }
        final_actor.actor_runtime_records[edx / 5U][edx % 5U] = 1U;
        return finish();
    }

    const auto selected = invoke(
        LegacyBattlePreFrameCall::query_group_b_actor,
        group_b_token_from_one_based(final_actor.published_actor_code)
    );
    if (selected.eax != 1U) {
        return finish();
    }

    eax = port.actor_metric_state().group_b_count;
    u32 index = 0U;
    if (signed_dword(eax) > 0) {
        for (;;) {
            const auto actor = invoke(
                LegacyBattlePreFrameCall::query_group_b_actor,
                group_b_token(index)
            );
            ++result.group_b_iterations;
            if (actor.eax == 0U) {
                ++index;
                final_actor.published_actor_code = index;
                return finish();
            }
            eax = port.actor_metric_state().group_b_count;
            ++index;
            if (signed_dword(index) >= signed_dword(eax)) {
                break;
            }
        }
    }

    static_cast<void>(invoke(
        LegacyBattlePreFrameCall::configure_group_a_actor,
        group_a_token(final_actor.secondary_actor_code),
        0U
    ));
    ecx = final_actor.secondary_actor_code;
    final_actor.action_execution_active = 0U;
    final_actor.published_actor_code = 1U;
    final_actor.source_actor_code = 0xFFFFFFFFU;
    const u32 final_workspace_index = ecx + 2U;
    if (final_workspace_index >= action.opponent_workspace.size()) {
        result.status =
            LegacyBattlePreFrameStatus::opponent_workspace_typed_stop;
        return finish();
    }
    action.opponent_workspace[final_workspace_index] = 0U;
    port.battle_terminal_latch() = 0U;
    return finish();
}

}  // namespace openswd3::battle
