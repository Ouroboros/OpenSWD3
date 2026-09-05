#include "openswd3/battle/legacy_battle_group_a_final_processing.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | low;
}

[[nodiscard]] constexpr bool transition_action(const u16 value) noexcept {
    return value == 1U || value == 23U || value == 26U;
}

}  // namespace

LegacyBattleActorModeFourFinalizationResult
finalize_legacy_battle_actor_mode_four(
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    const u32 actor_token,
    const u32 entry_eax
) noexcept {
    LegacyBattleActorModeFourFinalizationResult result;
    result.return_eax = entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = actor_token;
    if (actor_token == 0U || final_state == nullptr) {
        result.status =
            LegacyBattleActorModeFourFinalizationStatus::final_state_typed_stop;
        return result;
    }

    const u16 replacement = final_state->replacement_action_kind;
    result.return_eax = (entry_eax & 0xFFFF0000U) | replacement;
    if (item_effect == nullptr) {
        result.status =
            LegacyBattleActorModeFourFinalizationStatus::item_state_typed_stop;
        return result;
    }
    item_effect->mode_flags =
        static_cast<compat::u8>(item_effect->mode_flags | 0x02U);
    ++result.mode_flag_writes;
    final_state->completion_latch = 1U;
    ++result.completion_latch_writes;
    if (replacement != 0U) {
        item_effect->action_kind = replacement;
        ++result.action_kind_writes;
    }
    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorModeFourFinalizationStatus::workspace_typed_stop;
        return result;
    }
    workspace->early_workspace.fill(0U);
    result.workspace_dwords_zeroed =
        static_cast<u32>(workspace->early_workspace.size());
    result.return_eax = 0U;
    result.return_ecx = 0U;
    return result;
}

LegacyBattleGroupAFinalProcessingResult process_legacy_battle_group_a_final(
    LegacyBattleGroupAFinalProcessingState* state,
    LegacyBattleGroupAActionExecutionState* action,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAAttributeAggregationState& aggregation,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    const u32 actor_token,
    const u32 skip_primary,
    const u32 skip_secondary,
    LegacyBattleGroupAFinalProcessingPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupAFinalProcessingRequest& request
) {
    LegacyBattleGroupAFinalProcessingResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || state == nullptr || action == nullptr ||
        item_effect == nullptr) {
        result.status =
            LegacyBattleGroupAFinalProcessingStatus::actor_state_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    const auto finish = [&](const u32 value) {
        result.return_eax = value;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto publish_transition = [&](const u16 value) {
        item_effect->display_kind = value;
        ++result.display_kind_writes;
        item_effect->action_kind = 200U;
        ++result.action_kind_writes;
    };

    if (item_effect->action_kind == 0U) {
        return finish(1U);
    }

    if ((item_effect->mode_flags & 0x02U) != 0U) {
        eax = replace_low_word(eax, state->replacement_action_kind);
        state->completion_latch = 1U;
        ++result.completion_latch_writes;
        if (static_cast<u16>(eax) != 0U) {
            item_effect->action_kind = static_cast<u16>(eax);
            ++result.action_kind_writes;
        }
        if ((static_cast<compat::u8>(action->profile_word(0x0CU)) & 0x28U) ==
            0U) {
            return finish(0U);
        }
        eax = replace_low_word(eax, item_effect->action_kind);
        publish_transition(static_cast<u16>(eax));
        return finish(1U);
    }

    state->pre_effect_words.fill(0U);
    result.pre_effect_dwords_zeroed =
        static_cast<u32>(state->pre_effect_words.size());
    eax = replace_low_word(eax, item_effect->action_kind);
    if (static_cast<u16>(eax) != 27U) {
        if (configuration == nullptr ||
            configuration->actor_record_token == 0U) {
            result.status = LegacyBattleGroupAFinalProcessingStatus::
                actor_record_typed_stop;
            result.return_eax = eax;
            result.return_ecx = actor_token;
            result.return_edx = edx;
            return result;
        }
        ecx = configuration->actor_record_token;
        edx = replace_low_word(
            edx, static_cast<u16>(configuration->actor_record[9] >> 16U)
        );
        item_effect->derived_words[0] = static_cast<u16>(edx);
        ++result.derived_word_writes;
    }

    eax &= 0x0000FFFFU;
    if (configuration == nullptr) {
        result.status =
            LegacyBattleGroupAFinalProcessingStatus::actor_record_typed_stop;
        return finish(eax);
    }
    result.item_effect = apply_legacy_battle_group_a_item_effect(
        item_effect,
        progress,
        *configuration,
        actor_token,
        port,
        {.effect_kind = eax, .entry_eax = eax, .entry_edx = edx}
    );
    ++result.item_effect_calls;
    eax = result.item_effect.return_eax;
    ecx = result.item_effect.return_ecx;
    edx = result.item_effect.return_edx;
    if (result.item_effect.status !=
        LegacyBattleGroupAItemEffectApplicationStatus::completed) {
        result.status =
            LegacyBattleGroupAFinalProcessingStatus::item_effect_typed_stop;
        return finish(eax);
    }

    result.profile_mode = select_legacy_battle_group_a_profile_mode(
        action,
        shared,
        aggregation.embedded_profile_application,
        *item_effect,
        actor_token,
        skip_primary,
        skip_secondary,
        port,
        {.entry_eax = eax, .entry_edx = edx}
    );
    ++result.profile_mode_calls;
    eax = result.profile_mode.return_eax;
    ecx = result.profile_mode.return_ecx;
    edx = result.profile_mode.return_edx;
    if (result.profile_mode.status !=
        LegacyBattleGroupAProfileModeSelectionStatus::completed) {
        result.status =
            LegacyBattleGroupAFinalProcessingStatus::profile_mode_typed_stop;
        return finish(eax);
    }

    action->profile_buffer.fill(0U);
    result.profile_buffer_dwords_zeroed =
        static_cast<u32>(action->profile_buffer.size());
    const u32 buffer_token = actor_token + 0x0D90U;
    auto profile_result = load_legacy_battle_mon_profile(
        std::as_writable_bytes(std::span{action->profile_buffer}),
        mon_port,
        {
            .path = "mon.dat",
            .output_token = buffer_token,
            .profile_id = 0U,
            .file_name_token = 0x004AAED0U,
            .entry_eax = 0U,
            .entry_ecx = ecx,
            .entry_edx = edx,
        }
    );
    ++result.profile_load_calls;
    eax = profile_result.return_eax;
    ecx = profile_result.return_ecx;
    edx = profile_result.return_edx;
    if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
        result.status =
            LegacyBattleGroupAFinalProcessingStatus::profile_load_typed_stop;
        return finish(eax);
    }

    if ((aggregation.embedded_profile_application.status_bits & 0x20U) != 0U) {
        action->profile_buffer[1] |= 0x00000100U;
        ++result.profile_buffer_flag_writes;
        eax = action->profile_buffer[1];
    }

    eax = replace_low_word(eax, state->profile_record_id);
    if (static_cast<u16>(eax) != 0U) {
        profile_result = load_legacy_battle_mon_profile(
            std::as_writable_bytes(std::span{action->profile_buffer}),
            mon_port,
            {
                .path = "mon.dat",
                .output_token = buffer_token,
                .profile_id = static_cast<u16>(eax),
                .file_name_token = 0x004AAED0U,
                .entry_eax = eax,
                .entry_ecx = ecx,
                .entry_edx = edx,
            }
        );
        ++result.profile_load_calls;
        eax = profile_result.return_eax;
        ecx = profile_result.return_ecx;
        edx = profile_result.return_edx;
        if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
            result.status = LegacyBattleGroupAFinalProcessingStatus::
                profile_load_typed_stop;
            return finish(eax);
        }

        eax = replace_low_word(eax, action->profile_word(0x0CU));
        if ((static_cast<compat::u8>(eax) & 0x28U) == 0U &&
            item_effect->action_kind != 23U) {
            return finish(1U);
        }
        const u16 action_kind = item_effect->action_kind;
        if (transition_action(action_kind)) {
            ecx = state->transition_gate_a;
            if (ecx == 0U) {
                ecx = state->transition_gate_b;
                if (ecx == 0U) {
                    publish_transition(action_kind);
                }
            }
        }
        return finish(1U);
    }

    if ((static_cast<compat::u8>(action->profile_word(0x0CU)) & 0x28U) != 0U) {
        const u16 action_kind = item_effect->action_kind;
        if (transition_action(action_kind)) {
            ecx = state->transition_gate_a;
            if (ecx == 0U) {
                ecx = state->transition_gate_b;
                if (ecx == 0U) {
                    publish_transition(action_kind);
                }
            }
        }
    }
    return finish(0U);
}

}  // namespace openswd3::battle
