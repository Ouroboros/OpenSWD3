#include "openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp"
#include "test.hpp"

#include <algorithm>

void test_battle_group_a_actor_cleanup(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorListQueryState;
    using openswd3::battle::LegacyBattleActorListResourceNode;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAActorCleanupStatus;
    using openswd3::battle::LegacyBattleGroupAAttributeEffectState;
    using openswd3::battle::LegacyBattleGroupAFinalProcessingState;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationState;
    using openswd3::battle::LegacyBattleGroupAWorkspaceState;
    using openswd3::battle::cleanup_legacy_battle_group_a_actor;

    {
        const auto result = cleanup_legacy_battle_group_a_actor({}, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActorCleanupStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 10U &&
                result.return_edx == 0U &&
                result.explicit_words_zeroed == 0U,
            "group-A actor cleanup stops at the first actor word write with legacy setup registers"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.action_override_flags = 0xFFFFU;
        const auto result = cleanup_legacy_battle_group_a_actor(
            {.actor = &actor}, 0x005029D0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActorCleanupStatus::
                        profile_state_typed_stop &&
                actor.action_override_flags == 0U &&
                result.return_eax == 0U && result.return_ecx == 10U &&
                result.return_edx == 0x005029D0U &&
                result.explicit_words_zeroed == 1U,
            "group-A actor cleanup preserves the first word clear before a missing profile view stop"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAFinalProcessingState final_processing;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAAttributeEffectState attribute_effect;
        LegacyBattleActorListQueryState actor_list;
        actor.action_override_flags = 0xFFFFU;
        actor.special_particle_coordinate_suppression = 0xFFU;
        actor.special_effect_direct_mode = 0xFFU;
        actor.copied_runtime_word = 0xFFFFU;
        actor.source_y = 0xFFFFU;
        actor.special_profile_variant = 0xFFFFU;
        actor.summon_action_id = 0xFFFFU;
        actor.effect_curve_index = 0xBEEFU;
        workspace.tail_words.fill(0xFFFFU);
        final_processing.profile_buffer.fill(0xFFFFFFFFU);
        final_processing.pre_effect_words.fill(0xFFFFFFFFU);
        final_processing.actor_flags = 0xFFFFU;
        final_processing.applied_mode_value = 0xFFFFU;
        final_processing.applied_output_value = 0xFFFFU;
        final_processing.replacement_action_kind = 0xFFFFU;
        item_effect.cached_profile_item_id = 0xFFFFU;
        item_effect.derived_words.fill(0xFFFFU);
        attribute_effect.temporary_values.fill(0xFFFFU);

        const auto result = cleanup_legacy_battle_group_a_actor(
            {
                .actor = &actor,
                .workspace = &workspace,
                .final_processing = &final_processing,
                .item_effect = &item_effect,
                .attribute_effect = &attribute_effect,
                .actor_list = &actor_list,
            },
            0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleGroupAActorCleanupStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 0x00505000U &&
                result.return_edx == 0x005029D0U &&
                result.profile_dwords_zeroed == 10U &&
                result.pre_effect_dwords_zeroed == 4U &&
                result.explicit_words_zeroed == 11U &&
                actor.action_override_flags == 0U &&
                actor.special_particle_coordinate_suppression == 0U &&
                actor.special_effect_direct_mode == 0U &&
                actor.copied_runtime_word == 0U && actor.source_y == 0U &&
                actor.special_profile_variant == 0U &&
                actor.summon_action_id == 0U &&
                actor.effect_curve_index == 0xBEEFU &&
                workspace.tail_words[5U] == 0xFFFFU &&
                std::ranges::all_of(
                    final_processing.profile_buffer,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    final_processing.pre_effect_words,
                    [](const auto value) { return value == 0U; }
                ) &&
                item_effect.cached_profile_item_id == 0U &&
                std::ranges::all_of(
                    item_effect.derived_words,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    attribute_effect.temporary_values,
                    [](const auto value) { return value == 0U; }
                ),
            "group-A actor cleanup clears the exact profile, pre-effect, and eleven word views without touching plus-2F1A"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAFinalProcessingState final_processing;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAAttributeEffectState attribute_effect;
        LegacyBattleActorListQueryState actor_list;
        actor_list.selected_resource_token = 0xDEADU;
        actor_list.resource_head_token = 0x1000U;
        actor_list.resources.emplace_back();
        actor_list.resources.back().token = 0x1000U;
        actor_list.resources.back().primary_quantity = 2U;
        const auto result = cleanup_legacy_battle_group_a_actor(
            {
                .actor = &actor,
                .workspace = &workspace,
                .final_processing = &final_processing,
                .item_effect = &item_effect,
                .attribute_effect = &attribute_effect,
                .actor_list = &actor_list,
            },
            0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleGroupAActorCleanupStatus::completed &&
                result.return_eax == 0x1000U &&
                result.return_ecx == 0x00500001U &&
                result.return_edx == 0x005029D0U &&
                actor_list.resources[0U].primary_quantity == 1U &&
                actor_list.selected_resource_token == 0U &&
                result.resource_quantity_decrements == 1U &&
                result.resource_gate_clears == 1U,
            "group-A actor cleanup decrements a positive resource quantity and returns the mixed high-word register"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAFinalProcessingState final_processing;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAAttributeEffectState attribute_effect;
        LegacyBattleActorListQueryState actor_list;
        actor_list.selected_resource_token = 1U;
        actor_list.resource_head_token = 0x1000U;
        actor_list.resources.emplace_back();
        actor_list.resources.back().token = 0x1000U;
        actor_list.resources.back().primary_quantity = 0U;
        const auto result = cleanup_legacy_battle_group_a_actor(
            {
                .actor = &actor,
                .workspace = &workspace,
                .final_processing = &final_processing,
                .item_effect = &item_effect,
                .attribute_effect = &attribute_effect,
                .actor_list = &actor_list,
            },
            0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleGroupAActorCleanupStatus::completed &&
                result.return_eax == 0x1000U &&
                result.return_ecx == 0x00500000U &&
                actor_list.resources[0U].primary_quantity == 0U &&
                actor_list.selected_resource_token == 0U &&
                result.resource_quantity_decrements == 0U &&
                result.resource_gate_clears == 1U,
            "group-A actor cleanup preserves a zero resource quantity while still clearing the selection gate"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAFinalProcessingState final_processing;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAAttributeEffectState attribute_effect;
        LegacyBattleActorListQueryState actor_list;
        actor_list.selected_resource_token = 1U;
        actor_list.resource_head_token = 0x2000U;
        const auto result = cleanup_legacy_battle_group_a_actor(
            {
                .actor = &actor,
                .workspace = &workspace,
                .final_processing = &final_processing,
                .item_effect = &item_effect,
                .attribute_effect = &attribute_effect,
                .actor_list = &actor_list,
            },
            0x005029D0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActorCleanupStatus::
                        resource_node_typed_stop &&
                result.return_eax == 0x2000U &&
                result.return_ecx == 0x00505000U &&
                result.return_edx == 0x005029D0U &&
                actor_list.selected_resource_token == 1U &&
                result.explicit_words_zeroed == 11U,
            "group-A actor cleanup stops at the resource plus-six read after every preceding clear"
        );
    }
}
