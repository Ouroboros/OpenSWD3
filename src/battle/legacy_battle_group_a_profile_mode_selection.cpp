#include "openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp"

namespace openswd3::battle {

using compat::u16;
using compat::u32;

LegacyBattleGroupAProfileModeSelectionResult
select_legacy_battle_group_a_profile_mode(
    LegacyBattleGroupAActionExecutionState* state,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleGroupAEmbeddedProfileApplicationState& embedded_profile,
    LegacyBattleGroupAItemEffectApplicationState& item_effect,
    const u32 actor_token,
    const u32 skip_primary,
    const u32 skip_secondary,
    LegacyBattleGroupAProfileModeSelectionPort& port,
    const LegacyBattleGroupAProfileModeSelectionRequest& request
) {
    LegacyBattleGroupAProfileModeSelectionResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || state == nullptr) {
        result.status = LegacyBattleGroupAProfileModeSelectionStatus::
            actor_state_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    const auto return_zero = [&] {
        result.return_eax = 0U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto publish_mode = [&] {
        state->profile_mode = 1U;
        ++result.profile_mode_writes;
    };

    if (skip_primary == 1U || skip_secondary == 1U ||
        item_effect.effect_flags != 0U) {
        return return_zero();
    }

    ecx = shared.last_identity;
    eax = state->identity_word;
    if (ecx == eax) {
        return return_zero();
    }

    if ((embedded_profile.status_bits & 1U) != 0U) {
        publish_mode();
        result.return_eax = 1U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    ecx = shared.profile_threshold + 8U;
    edx = shared.completion_counter;
    if (edx >= ecx) {
        publish_mode();
        shared.completion_counter = 0U;
        ++result.counter_clears;
        eax = state->identity_word;
        shared.last_identity = eax;
        ++result.identity_writes;
        result.return_eax = 1U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    if (shared.completion_counter < 8U) {
        return return_zero();
    }

    const auto reply = port.random_below(10U, eax, ecx, edx);
    ++result.random_calls;
    eax = reply.eax;
    ecx = reply.ecx;
    edx = reply.edx;
    if (static_cast<u16>(eax) <= 5U) {
        return return_zero();
    }

    publish_mode();
    shared.completion_counter = 0U;
    ++result.counter_clears;
    ecx = state->identity_word;
    shared.last_identity = ecx;
    ++result.identity_writes;
    result.return_eax = 1U;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
