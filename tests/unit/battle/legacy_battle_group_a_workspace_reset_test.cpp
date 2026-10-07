#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <ranges>

void test_battle_group_a_workspace_reset(openswd3::test::Context& test) {
    openswd3::battle::LegacyBattleGroupAWorkspaceState state{
        .object_token = 0x005029D0U,
        .special_item_latch = 0x89ABCDEFU,
        .field_2f0c = 0xA5A5A5A5U,
        .untouched_field_2f26 = 0x2468U,
    };
    state.early_workspace.fill(0x11111111U);
    state.late_workspace.fill(0x22222222U);
    state.tail_words.fill(0x3333U);

    const auto result =
        openswd3::battle::reset_legacy_battle_group_a_workspace(state);

    test.expect_true(
        std::ranges::all_of(
            state.early_workspace, [](const auto value) { return value == 0U; }
        ) &&
            std::ranges::all_of(
                state.late_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                state.tail_words, [](const auto value) { return value == 0U; }
            ) &&
            state.special_item_latch == 0x89ABCDEFU && state.field_2f0c == 0U &&
            state.untouched_field_2f26 == 0x2468U &&
            result.explicit_words_zeroed == 11U &&
            result.explicit_dwords_zeroed == 1U &&
            result.upper_workspace_dwords_zeroed == 0xBEU &&
            result.early_workspace_dwords_zeroed == 0x4CU &&
            result.lower_workspace_dwords_zeroed == 0x29U &&
            result.return_eax == 0U && result.return_ecx == 0U &&
            result.return_edx == 0x005029D0U,
        "group-A workspace reset clears exact ranges and preserves skipped adjacent words"
    );

    openswd3::battle::LegacyBattleGroupAActionExecutionState action;
    openswd3::battle::LegacyBattleTargetPhaseState particle;
    openswd3::battle::LegacyBattleGroupAFinalProcessingState final_processing;
    openswd3::battle::LegacyBattleGroupAItemEffectApplicationState item_effect;
    action.special_particle_sequence_index = 0xFFFF1357U;
    action.special_particle_sequence_count = 7U;
    action.summon_action_id = 9U;
    action.effect_curve_index = 11U;
    action.special_action_record.action_id = 13U;
    action.special_secondary_action_record.action_id = 15U;
    final_processing.applied_output_value = 17U;
    final_processing.replacement_action_kind = 19U;
    item_effect.cached_profile_item_id = 21U;
    particle.spawn_action_records[4U].action_id = 23U;
    particle.spawn_counters[4U] = 25U;
    particle.tick = 27U;
    static_cast<void>(openswd3::battle::reset_legacy_battle_group_a_workspace(
        state,
        {.action = &action,
         .final_processing = &final_processing,
         .item_effect = &item_effect,
         .particle = &particle}
    ));
    test.expect_true(
        action.special_particle_sequence_index == 0U &&
            action.special_particle_sequence_count == 0U &&
            action.summon_action_id == 0U && action.effect_curve_index == 0U &&
            action.special_action_record.action_id == 0U &&
            action.special_secondary_action_record.action_id == 0U &&
            final_processing.applied_output_value == 0U &&
            final_processing.replacement_action_kind == 0U &&
            item_effect.cached_profile_item_id == 0U &&
            particle.spawn_action_records[4U].action_id == 0U &&
            particle.spawn_counters[4U] == 25U && particle.tick == 27U,
        "workspace stores reach live scalar and action-record views without clearing adjacent particle counters or tick"
    );

    openswd3::battle::LegacyBattleStartupState startup;
    auto& owned = startup.party[2U].workspace;
    owned.object_token = 0x00508838U;
    owned.special_item_latch = 0x12345678U;
    owned.early_workspace.fill(0xAAAAAAAAU);
    owned.late_workspace.fill(0xBBBBBBBBU);
    owned.tail_words.fill(0xCCCCU);
    const auto owned_result =
        openswd3::battle::reset_legacy_battle_group_a_workspace(owned);
    test.expect_true(
        owned_result.return_edx == owned.object_token &&
            owned.special_item_latch == 0x12345678U &&
            std::ranges::all_of(
                startup.party[2U].workspace.early_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                startup.party[2U].workspace.late_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                startup.party[2U].workspace.tail_words,
                [](const auto value) { return value == 0U; }
            ),
        "workspace reset mutates the startup party actor's unique physical view"
    );
}
