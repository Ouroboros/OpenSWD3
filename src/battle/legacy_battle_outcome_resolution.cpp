#include "openswd3/battle/legacy_battle_outcome_resolution.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] bool run_darkening(
    LegacyBattleOutcomeResolutionState& state,
    LegacyBattleOutcomeResolutionBindings bindings,
    LegacyBattleOutcomeResolutionResult& result,
    LegacyBattleFullFrameDarkeningResult& destination
) {
    destination = update_legacy_battle_full_frame_darkening(
        state.darkening, bindings.framebuffer, bindings.shared_effects
    );
    ++result.darkening_calls;
    result.return_value = destination.return_value;
    if (destination.status != LegacyBattleFullFrameDarkeningStatus::completed) {
        result.status = LegacyBattleOutcomeResolutionStatus::
            full_frame_darkening_typed_stop;
        return false;
    }
    return true;
}

}  // namespace

LegacyBattleOutcomeResolutionResult update_legacy_battle_outcome_resolution(
    const LegacyBattleOutcomeResolutionBindings bindings,
    LegacyBattleOutcomeResolutionPort& port
) {
    LegacyBattleOutcomeResolutionResult result;
    auto& state = port.outcome_resolution_state();

    const u32 group_a_count = bindings.group_a_count;
    const u32 group_a_resolved = bindings.final_actor.excluded_group_a_count;
    const u32 group_a_progress =
        (bindings.action.phase_counter >> 16U) & 0xFFFFU;
    const u32 group_a_completed = bindings.final_actor.removed_group_a_count;
    result.group_a_remaining =
        group_a_count - group_a_progress - group_a_resolved;
    result.group_a_threshold_met =
        group_a_completed >= result.group_a_remaining;
    if (result.group_a_threshold_met) {
        const u32 darkening_gate = state.darkening_gate;
        state.resolution_latch = 1U;
        if (darkening_gate == 1U) {
            if (!run_darkening(
                    state, bindings, result, result.first_darkening
                )) {
                return result;
            }
            if (result.first_darkening.return_value == 1U) {
                const auto audio = port.invoke_outcome_resolution(
                    LegacyBattleOutcomeResolutionCall::suspend_audio_stream
                );
                ++result.audio_suspend_calls;
                result.first_finalization = finalize_legacy_battle_outcome(
                    port, bindings.group_b_count, audio.eax
                );
                ++result.outcome_calls;
                if (result.first_finalization.status !=
                    LegacyBattleOutcomeFinalizationStatus::completed) {
                    result.status = LegacyBattleOutcomeResolutionStatus::
                        outcome_finalization_typed_stop;
                    result.return_value =
                        result.first_finalization.return_value;
                    return result;
                }
                const u32 message_state = bindings.message_state;
                bindings.frame_active = 2U;
                if (message_state == 0x68U) {
                    bindings.frame_active = 0U;
                }
                if ((bindings.battle_mode_flags & 8U) != 0U) {
                    bindings.frame_active = 0U;
                }
            }
        }
    }

    const u32 group_b_packed_progress = bindings.action.packed_actor_counter;
    const u32 group_b_count = bindings.group_b_count;
    const u32 low_progress = group_b_packed_progress & 0xFFU;
    const u32 high_progress = (group_b_packed_progress >> 16U) & 0xFFU;
    result.group_b_difference = low_progress - high_progress;
    result.return_value = group_b_count;
    result.group_b_threshold_met =
        signed_bits(result.group_b_difference) >= signed_bits(group_b_count) ||
        state.force_group_b_resolution == 1U;
    if (!result.group_b_threshold_met) {
        return result;
    }

    const u32 darkening_gate = state.darkening_gate;
    state.resolution_latch = 1U;
    result.return_value = darkening_gate;
    if (darkening_gate != 1U) {
        return result;
    }

    if (!run_darkening(state, bindings, result, result.second_darkening)) {
        return result;
    }
    if (result.second_darkening.return_value != 1U) {
        return result;
    }

    result.second_finalization = finalize_legacy_battle_outcome(
        port, bindings.group_b_count, result.second_darkening.return_value
    );
    ++result.outcome_calls;
    result.return_value = result.second_finalization.return_value;
    if (result.second_finalization.status !=
        LegacyBattleOutcomeFinalizationStatus::completed) {
        result.status = LegacyBattleOutcomeResolutionStatus::
            outcome_finalization_typed_stop;
        return result;
    }
    bindings.frame_active = 0U;
    return result;
}

}  // namespace openswd3::battle
