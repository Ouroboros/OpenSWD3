#include "openswd3/battle/legacy_battle_retreat_commit.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u32 low_byte) noexcept {
    return (value & 0xFFFFFF00U) | (low_byte & 0xFFU);
}

}  // namespace

LegacyBattleRetreatCommitResult commit_legacy_battle_retreat(
    const LegacyBattleRetreatCommitBindings bindings,
    LegacyBattleRetreatCommitPort& port,
    const u32 group_a_index
) {
    LegacyBattleRetreatCommitResult result;
    result.selected_object_token = kLegacyBattleRetreatCommitGroupABaseToken +
        group_a_index * kLegacyBattleRetreatCommitGroupAStride;
    result.selected_actor = port.invoke_retreat_commit({
        .call = LegacyBattleRetreatCommitCall::query_selected_actor_ready,
        .object_token = result.selected_object_token,
    });
    ++result.port_calls;
    result.return_value = result.selected_actor.eax;
    result.final_ecx = result.selected_actor.ecx;
    result.final_edx = result.selected_actor.edx;
    if (result.selected_actor.eax != 1U) {
        return result;
    }

    result.primary_actor = port.invoke_retreat_commit({
        .call = LegacyBattleRetreatCommitCall::query_primary_actor_state,
        .object_token = kLegacyBattleRetreatCommitGroupABaseToken,
    });
    ++result.port_calls;
    const u32 mode_flags =
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24;
    result.mode_bit_blocked = (mode_flags & 0x00000200U) != 0U;
    if (result.primary_actor.eax == 0U || result.mode_bit_blocked) {
        result.branch = LegacyBattleRetreatCommitBranch::warning;
        result.warning_text = port.invoke_retreat_commit({
            .call = LegacyBattleRetreatCommitCall::display_warning,
            .arguments = {
                0x0118U,
                0x000AU,
                0x0032U,
                kLegacyBattleRetreatCommitWarningTextToken,
                0x40000002U,
            },
        });
        ++result.port_calls;
        result.warning_sample = port.invoke_retreat_commit({
            .call = LegacyBattleRetreatCommitCall::play_warning_sample,
            .arguments = {
                kLegacyBattleRetreatCommitWarningSample,
                static_cast<u32>(port.battle_sample_mix_level()),
            },
        });
        ++result.port_calls;
        result.return_value = result.warning_sample.eax;
        result.final_ecx = result.warning_sample.ecx;
        result.final_edx = result.warning_sample.edx;
        return result;
    }

    const u32 group_b_count = port.actor_metric_state().group_b_count;
    auto& state = port.retreat_commit_state();
    state.completion_gate_a = 1U;
    state.completion_gate_b = 1U;
    port.outcome_resolution_state().resolution_latch = 0U;
    state.auxiliary_latch = 0U;
    port.battle_debug_hotkey_state().committed_actor_code = 0U;
    port.battle_debug_overlay_gate() = 0U;
    state.selected_actor_token = 0xFFFFFFFFU;
    port.battle_message_state() = 0U;
    bindings.packed_actor_counter =
        replace_low_byte(bindings.packed_actor_counter, group_b_count);
    port.outcome_resolution_state().darkening_gate = 1U;

    result.branch = LegacyBattleRetreatCommitBranch::committed;
    result.return_value = 0U;
    result.final_ecx =
        replace_low_byte(result.primary_actor.ecx, group_b_count);
    result.final_edx = result.primary_actor.edx;
    result.state_committed = true;
    return result;
}

}  // namespace openswd3::battle
