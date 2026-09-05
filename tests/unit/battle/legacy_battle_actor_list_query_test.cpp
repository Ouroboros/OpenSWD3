#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include "test.hpp"

namespace {

class SelectionPort final
    : public openswd3::battle::LegacyBattleActorResourceSelectionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        if (request.call ==
                openswd3::battle::LegacyBattleMonDatabaseCall::seek_file &&
            seek_calls % 3U == 1U) {
            profile_ids.push_back(
                static_cast<openswd3::compat::u16>(
                    (request.distance - auxiliary_root - 0x200U) / 4U
                )
            );
        }
        return LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
    }
    void report_missing_runtime_word(
        const openswd3::compat::u16 resource_id
    ) override {
        diagnostics.push_back(resource_id);
    }
    std::vector<openswd3::compat::u16> profile_ids;
    std::vector<openswd3::compat::u16> diagnostics;
};

class StatePort final
    : public openswd3::battle::LegacyBattleActorListStatePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorListStateCallReply
    publish_message(const openswd3::compat::u32 text_token) override {
        ++message_calls;
        last_text = text_token;
        return {.publish_message_token = true, .message_token = 0x74000000U};
    }
    [[nodiscard]] openswd3::battle::LegacyBattleActorListStateCallReply
    play_sample(
        const openswd3::compat::u32, const openswd3::compat::u32
    ) override {
        ++sample_calls;
        return {};
    }
    openswd3::compat::u32 message_calls{};
    openswd3::compat::u32 sample_calls{};
    openswd3::compat::u32 last_text{};
};

class QueryPort final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        set_profile_dword(0x10U, index);
        if (request.call ==
                openswd3::battle::LegacyBattleMonDatabaseCall::seek_file &&
            seek_calls % 3U == 1U) {
            last_profile = static_cast<openswd3::compat::u16>(
                (request.distance - auxiliary_root - 0x200U) / 4U
            );
        }
        return LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
    }
    openswd3::compat::u16 last_profile{};
    openswd3::compat::u32 index{};
};

}  // namespace

void test_battle_actor_list_query(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    const std::array<openswd3::compat::u16, 4> table{10U, 20U, 30U, 40U};

    LegacyBattleGroupAActionExecutionState actor;
    actor.next_list_index = 0x71000000U;
    LegacyBattleActorListQueryState list{
        .owner_token = 0x71000000U,
        .head_token = 0x72000000U,
        .nodes =
            {
                {.token = 0x72000000U,
                 .next_token = 0x72000100U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x05U,
                 .type = 0x1AU,
                 .text = {}},
                {.token = 0x72000100U,
                 .next_token = 0x72000200U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x05U,
                 .type = 0x1BU,
                 .profile_id = 7U,
                 .value_flags = 0xC800U,
                 .text = "first"},
                {.token = 0x72000200U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x01U,
                 .type = 0x1EU,
                 .profile_id = 8U,
                 .value_flags = 0x8005U,
                 .text = "second"},
            },
        .resource_owner_token = 0U,
        .resource_head_token = 0U,
        .resources = {},
        .selected_resource_token = 0U,
        .primary_required = 0U,
        .secondary_required = 0U,
    };

    {
        QueryPort port;
        port.index = 2U;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            port,
            {.category_selector = 0U,
             .type_selector = 0U,
             .occurrence = 1U,
             .entry_output_word = 0x7777U,
             .return_table = table}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                actor.current_list_index == list.owner_token &&
                result.nodes_visited == 2U && result.matches == 1U &&
                result.output_text == "first" && result.output_word == 1000U &&
                result.return_eax == 30U && port.last_profile == 7U,
            "list query commits the index then applies ordered category, type, and value overrides"
        );
    }

    {
        QueryPort port;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            port,
            {.category_selector = 0U,
             .type_selector = 0U,
             .occurrence = 0U,
             .entry_output_word = 3U,
             .return_table = table}
        );
        test.expect_true(
            result.return_eax == 0xFFFFU && result.output_word == 0xFFFFU &&
                result.matches == 2U && port.read_calls == 0U,
            "zero occurrence never matches because the counter increments before comparison"
        );
    }

    {
        LegacyBattleActorListQueryState type31{
            .owner_token = list.owner_token,
            .head_token = 0x73000000U,
            .nodes =
                {{.token = 0x73000000U,
                  .category_flags = 0x0CU,
                  .mode_flags = 0x04U,
                  .type = 0x1FU,
                  .profile_id = 9U,
                  .value_flags = 0xFFFFU,
                  .text = "profile"}},
            .resource_owner_token = 0U,
            .resource_head_token = 0U,
            .resources = {},
            .selected_resource_token = 0U,
            .primary_required = 0U,
            .secondary_required = 0U,
        };
        QueryPort port;
        port.index = 1U;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &type31,
            0x005029D0U,
            port,
            {.category_selector = 1U,
             .type_selector = 1U,
             .occurrence = 1U,
             .entry_output_word = 0xAAAAU,
             .return_table = table}
        );
        test.expect_true(
            result.output_word == 1U && result.return_eax == 20U &&
                result.profile_load_calls == 1U,
            "type thirty-one forces output one after all value flag overrides"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        const auto result = count_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            0x00610000U,
            {.category_selector = 0U,
             .type_selector = 0U,
             .entry_count = 0xFFU,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.nodes_visited == 3U && result.matches == 2U &&
                result.count == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x00610000U &&
                result.return_edx == 0xBBBBBBBBU,
            "list count preserves the caller byte then wraps once per matching node"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        const auto result = count_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            0x00610000U,
            {.category_selector = 0U, .type_selector = 7U, .entry_count = 9U}
        );
        test.expect_true(
            result.count == 9U && result.matches == 0U &&
                result.nodes_visited == 3U,
            "every non-twenty-eight type selector still tests only type thirty-one"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[1U] = 5U << 16U;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        item_effect.mode_flags = 0x80U;
        list.selected_resource_token = 0U;
        list.primary_required = 7U;
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, nullptr, 0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                (configuration.actor_record[1U] >> 16U) == 0U &&
                list.primary_required == 0U && result.capacity_writes == 2U &&
                result.refresh_calls == 1U && result.release_calls == 0U,
            "actor list action wraps subtraction then clamps a negative signed capacity"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAWorkspaceState workspace;
        item_effect.mode_flags = 0x80U;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.primary_required = 9U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 4U,
             .secondary_quantity = 2,
             .name = {}},
        };
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.release_calls == 1U && result.refresh_calls == 1U &&
                list.selected_resource_token == 0U &&
                list.primary_required == 0U &&
                list.resources[1U].secondary_quantity == 1 &&
                result.return_eax == 0x76000004U && result.return_edx == 0U,
            "selected resource path uses the typed release then refreshes using its returned registers"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 0xA5A50003U;
        list.secondary_required = 5U;
        const auto result = refresh_legacy_battle_actor_list_action(
            &list, &configuration, 0x005029D0U, 10U, 0xBEEF0000U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                configuration.actor_record[2U] == 0xA5A50000U &&
                list.secondary_required == 0U && result.capacity_writes == 2U &&
                result.secondary_required_clears == 1U &&
                result.return_eax == configuration.actor_record_token &&
                result.return_edx == 0xBEEF0005U,
            "secondary refresh preserves high words, wraps subtraction, clamps signed negative and returns stale register halves"
        );
    }

    {
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        list.secondary_required = 0U;
        const auto result = execute_legacy_battle_actor_list_action(
            &list,
            &item_effect,
            nullptr,
            nullptr,
            0x005029D0U,
            {.entry_eax = 10U, .entry_edx = 12U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.refresh_calls == 1U && result.return_eax == 10U &&
                result.return_edx == 0U,
            "mode bit clear skips primary state and still runs typed secondary refresh"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[1U] = 5U << 16U;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xFFFFU;
        LegacyBattleGroupAActionExecutionState action;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 2U,
             .primary_quantity = 1U,
             .tertiary_quantity = 4,
             .name = {},
             .category_mask = 0x10U,
             .derived_words_40 = {11U, 12U, 13U},
             .flags_49 = 0x02U,
             .profile_id_4a = 7U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x8003U,
             .alternate_profile_id_54 = 9U},
        };
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            &configuration,
            &final_state,
            &item,
            &workspace,
            &action,
            0x005029D0U,
            port,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.output_mode == 1U &&
                port.profile_ids[0U] == 9U && result.diagnostic_calls == 1U &&
                list.selected_resource_token == 0x76000010U &&
                list.primary_required == 3U && item.mode_flags == 0x40U &&
                item.derived_words[1U] == 11U &&
                final_state.profile_copy_latch == 1U &&
                list.resources[1U].primary_quantity == 2U,
            "resource selection loads the alternate profile, applies gates, diagnoses zero runtime word and increments quantity"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAActionExecutionState action;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .tertiary_quantity = 1,
             .name = {},
             .category_mask = 0x2000U,
             .derived_word_30 = 0x1234U,
             .profile_id_4a = 7U,
             .capacity_gate_flags = 0x2000U},
        };
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            nullptr,
            &final_state,
            &item,
            &workspace,
            &action,
            0x005029D0U,
            port,
            port,
            {.category_selector = 4U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 1U && port.profile_ids[0U] == 7U &&
                item.derived_words[0U] == 0x1234U &&
                list.resources[1U].primary_quantity == 0U,
            "category four bit-thirteen selection takes the early profile path without quantity mutation"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xFFFFU;
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            nullptr,
            nullptr,
            &final_state,
            nullptr,
            &workspace,
            nullptr,
            0x005029D0U,
            port,
            port,
            {.occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 0U && final_state.profile_buffer[0U] == 0U &&
                final_state.pre_effect_words[0U] == 0U &&
                workspace.tail_words[2U] == 0U,
            "zero occurrence preserves the mandatory clearing prefix before failure"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .name = "ordinary"},
            {.token = 0x76000020U,
             .secondary_quantity = -1,
             .tertiary_quantity = 2,
             .name = "flagged",
             .capacity_gate_flags = 0x2000U},
        };
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list,
            0x005029D0U,
            {.occurrence = 1U,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.commit_calls == 1U && result.nodes_visited == 2U &&
                result.flagged_matches == 1U &&
                result.copied_name == "flagged" &&
                result.output_quantity == 1U && result.return_eax == 1U &&
                result.return_ecx == 1U && result.return_edx == 1U,
            "flagged resource query counts bit thirteen and wraps the secondary plus tertiary word sum"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .secondary_quantity = 3,
             .tertiary_quantity = 4,
             .name = "stale-zero"},
        };
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 0x005029D0U, {.occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.flagged_matches == 0U &&
                result.copied_name == "stale-zero" &&
                result.output_quantity == 7U,
            "flagged resource occurrence zero preserves the stale success on the first unflagged node"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = "twenty-byte-name-xxxx",
             .capacity_gate_flags = 0x2000U},
        };
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 0x005029D0U, {.occurrence = 1U, .output_capacity = 20U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                result.nodes_visited == 1U && result.return_eax == 0x76000010U,
            "flagged resource query stops at the original caller buffer boundary instead of truncating lstrcpy"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = "only",
             .capacity_gate_flags = 0x2000U},
        };
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 0x005029D0U, {.occurrence = 2U}
        );
        test.expect_true(
            result.return_eax == 0U && result.nodes_visited == 2U &&
                result.flagged_matches == 1U &&
                list.resource_head_token == 0U && result.copied_name.empty(),
            "flagged resource query destructively reaches null when the requested occurrence is absent"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0xDEADBEEFU;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .next_token = 0x77000020U,
             .resource_id = 0x222U,
             .name = "ordinary"},
            {.token = 0x77000020U,
             .resource_id = 0x0300U,
             .secondary_quantity = -1,
             .tertiary_quantity = 2,
             .name = "fixed",
             .mode_flags = 0x01U},
        };
        const auto result = query_legacy_battle_actor_mode_resource(
            &list,
            0x005029D0U,
            {.mode = 0U,
             .occurrence = 1U,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.nodes_visited == 2U && result.matches == 0U &&
                result.selected_writes == 0U && result.outputs_published &&
                result.copied_name == "fixed" && result.output_quantity == 1U &&
                result.return_eax == 1U && result.return_ecx == 0x0300U &&
                result.return_edx == 1U &&
                list.selected_resource_token == 0xDEADBEEFU,
            "mode resource query finds the fixed identifier without rewriting the selected token and wraps both quantity words"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .resource_id = 0x0300U,
             .name = "twenty-byte-name-xxx",
             .mode_flags = 0x01U},
        };
        const auto result = query_legacy_battle_actor_mode_resource(
            &list,
            0x005029D0U,
            {.mode = 0U, .occurrence = 1U, .output_capacity = 20U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                result.return_eax == 0x77000010U && !result.outputs_published,
            "mode resource query stops at the caller string boundary before publishing wrapped quantities"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .next_token = 0x77000020U,
             .resource_id = 0x0300U,
             .name = "excluded",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x77000020U,
             .next_token = 0x77000030U,
             .resource_id = 0x222U,
             .name = "first",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x77000030U,
             .resource_id = 0x333U,
             .secondary_quantity = 3,
             .tertiary_quantity = 4,
             .name = "second",
             .category_mask = 0x08000000U,
             .mode_flags = 0x04U},
        };
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 0x005029D0U, {.mode = 1U, .occurrence = 2U}
        );
        test.expect_true(
            result.return_eax == 1U && result.nodes_visited == 3U &&
                result.matches == 2U && result.selected_writes == 1U &&
                result.outputs_published && result.copied_name == "second" &&
                result.output_quantity == 7U &&
                list.selected_resource_token == 0x77000030U,
            "mode resource query counts bit twenty-seven nodes while excluding the fixed identifier"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .secondary_quantity = 9,
             .tertiary_quantity = 8,
             .name = "stale",
             .mode_flags = 0U},
        };
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 0x005029D0U, {.mode = 1U, .occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.matches == 0U &&
                result.selected_writes == 1U && !result.outputs_published &&
                result.copied_name.empty() && result.output_quantity == 0U &&
                list.selected_resource_token == 0x77000010U,
            "mode resource occurrence zero selects the first unqualified node but preserves stale outputs when mode bits are clear"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .resource_id = 0x222U,
             .name = "only",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
        };
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 0x005029D0U, {.mode = 1U, .occurrence = 2U}
        );
        test.expect_true(
            result.return_eax == 0U && result.nodes_visited == 2U &&
                result.matches == 1U && list.resource_head_token == 0U,
            "mode resource query destructively reaches null when the requested filtered occurrence is absent"
        );
    }

    {
        list.next_resource_head_token = 0x79000000U;
        list.resources = {
            {.token = 0x79000000U, .next_token = 0x79000010U, .name = {}},
            {.token = 0x79000010U,
             .next_token = 0x79000020U,
             .resource_id = 0x0300U,
             .name = "fixed",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x79000020U,
             .next_token = 0x79000030U,
             .resource_id = 0x0222U,
             .name = "unflagged",
             .mode_flags = 0x01U},
            {.token = 0x79000030U,
             .next_token = 0x79000040U,
             .resource_id = 0x0223U,
             .name = "closed",
             .category_mask = 0x08000000U,
             .mode_flags = 0U},
            {.token = 0x79000040U,
             .next_token = 0x79000050U,
             .resource_id = 0x0224U,
             .secondary_quantity = 32767,
             .tertiary_quantity = 32767,
             .name = "large",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x79000050U,
             .resource_id = 0x0225U,
             .secondary_quantity = 1,
             .tertiary_quantity = 2,
             .name = "small",
             .category_mask = 0x08000000U,
             .mode_flags = 0x04U},
        };
        const auto result = count_legacy_battle_actor_mode_resources(
            &list,
            0x005029D0U,
            {.output_token = 0x0012FFD4U,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.commit_calls == 1U && result.nodes_visited == 6U &&
                result.matches == 2U && result.count == 1U &&
                result.return_eax == 0U && result.return_ecx == 0x0012FFD4U &&
                result.return_edx == 0x0300U && list.resource_head_token == 0U,
            "mode resource count filters bit twenty-seven, fixed identifiers and output mode bits before wrapping all quantity words"
        );
    }

    {
        list.next_resource_head_token = 0x79000000U;
        list.resources = {
            {.token = 0x79000000U, .next_token = 0x79000010U, .name = {}},
            {.token = 0x79000010U,
             .resource_id = 0x0222U,
             .secondary_quantity = 7,
             .tertiary_quantity = 8,
             .name = "excluded"},
        };
        const auto result = count_legacy_battle_actor_mode_resources(
            &list, 0x005029D0U, {.output_token = 0x0012FFD4U}
        );
        test.expect_true(
            result.return_eax == 0U && result.matches == 0U &&
                result.count == 0U,
            "mode resource count zeroes its output before traversing an excluded chain"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x33U,
             .primary_quantity = 4U,
             .secondary_quantity = 2,
             .name = {},
             .gate_word_48 = 0x0402U},
        };
        const auto result = release_legacy_battle_actor_resource(
            &list,
            &workspace,
            0x005029D0U,
            {.entry_eax = 0xAAAAAAAAU, .entry_edx = 0xBBBB0000U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.commit_calls == 1U && result.nodes_visited == 1U &&
                result.gate_writes == 1U && result.quantity_writes == 0U &&
                result.deallocation_calls == 0U &&
                list.resources[1U].gate_word_48 == 0x0401U &&
                list.resources[1U].primary_quantity == 4U &&
                list.selected_resource_token == 0x76000010U &&
                result.return_eax == 0x76000033U && result.return_edx == 1U,
            "resource release decrements the low-byte gate first and preserves quantities while the gate remains nonzero"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x44U,
             .primary_quantity = 2U,
             .secondary_quantity = 2,
             .name = {},
             .capacity_gate_flags = 0x2000U},
        };
        const auto result = release_legacy_battle_actor_resource(
            &list, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.quantity_writes == 2U && result.selected_clears == 1U &&
                result.deallocation_calls == 0U &&
                list.resources[1U].primary_quantity == 1U &&
                list.resources[1U].secondary_quantity == 1 &&
                list.selected_resource_token == 0U &&
                result.output_word == 0U && result.return_eax == 0x76000000U,
            "resource release decrements secondary before tertiary, also decrements primary, clears selection and applies bit-thirteen return suppression"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000020U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .resource_id = 1U,
             .name = {}},
            {.token = 0x76000020U,
             .next_token = 0x76000030U,
             .resource_id = 2U,
             .primary_quantity = 1U,
             .secondary_quantity = 1,
             .name = {}},
            {.token = 0x76000030U, .resource_id = 3U, .name = {}},
        };
        const auto result = release_legacy_battle_actor_resource(
            &list, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.nodes_visited == 2U && result.commit_calls == 2U &&
                result.deallocation_calls == 1U && result.relink_writes == 1U &&
                list.resources.size() == 3U &&
                list.resources[1U].next_token == 0x76000030U &&
                list.selected_resource_token == 0U &&
                result.return_ecx == 0x76000010U &&
                result.return_eax == 0x76000002U,
            "resource release removes a depleted middle node and replays the destructive traversal count to patch its predecessor"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0xABCD0000U;
        list.selected_resource_token = 0U;
        list.resources = {{.token = 0xABCD0000U, .name = {}}};
        const auto result = release_legacy_battle_actor_resource(
            &list, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.return_eax == 0xABCD0000U && result.return_ecx == 0U &&
                result.nodes_visited == 0U,
            "missing selected resource zeroes only AX and preserves the committed head high word"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .primary_quantity = 1U,
             .tertiary_quantity = 2,
             .name = {},
             .category_mask = 0x2000U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x2000U},
            {.token = 0x76000020U, .name = {}, .capacity_gate_flags = 0x2000U},
        };
        const auto result = count_legacy_battle_actor_resource_list(
            &list,
            0x005029D0U,
            {.category_selector = 4U, .initial_count = 0xFFFEU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.count == 1U && result.nodes_visited == 2U &&
                result.positive_matches == 1U && result.extra_matches == 2U &&
                result.commit_calls == 1U && list.resource_head_token == 0U,
            "resource count preserves the initial word, wraps increments, and category four counts its extra bit independently"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 2U;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .primary_quantity = 3U,
             .secondary_quantity = -1,
             .tertiary_quantity = 10,
             .name = "resource",
             .category_mask = 0x10U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x4003U},
        };
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            &configuration,
            0x005029D0U,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.commit_calls == 1U &&
                result.nodes_visited == 1U && result.matches == 1U &&
                result.copied_name == "resource" &&
                result.output_quantity == 6U &&
                result.output_flags == 0xC000U &&
                list.resource_head_token == 0x76000010U,
            "resource query destructively advances the chain, counts a positive match and publishes quantity plus both flag bits"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = {},
             .category_mask = 0U,
             .mode_flags = 0U},
        };
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            nullptr,
            0x005029D0U,
            {.category_selector = 0U, .occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.matches == 0U &&
                result.output_flags == 0U && result.copied_name.empty(),
            "occurrence zero takes the stale success branch after the first nonmatch and rechecks filters without copying"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_40 = 11U;
        list.nodes[1U].value_42 = 12U;
        list.nodes[1U].value_44 = 13U;
        list.nodes[1U].copy_flags = 0U;
        list.nodes[1U].output_value = 77U;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        QueryPort port;
        const auto result = apply_legacy_battle_actor_list(
            &actor,
            &list,
            &final_state,
            &item_effect,
            0x005029D0U,
            port,
            {.category_selector = 0U, .type_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.output_value == 77U &&
                result.profile_buffer_dwords_zeroed == 10U &&
                result.pre_effect_dwords_zeroed == 4U &&
                (item_effect.mode_flags & 0x80U) != 0U &&
                item_effect.derived_words[1U] == 11U &&
                item_effect.derived_words[2U] == 12U &&
                item_effect.derived_words[3U] == 13U &&
                reinterpret_cast<const char*>(
                    final_state.pre_effect_words.data()
                )[0] == 'f',
            "list apply clears both buffers, loads the profile, copies text and publishes derived words"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer[0U] = 0xAABBCCDDU;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        item_effect.mode_flags = 0x40U;
        QueryPort port;
        const auto result = apply_legacy_battle_actor_list(
            &actor,
            &list,
            &final_state,
            &item_effect,
            0x005029D0U,
            port,
            {.category_selector = 0U, .type_selector = 0U, .occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 0xFFFFU && result.index_commit_calls == 0U &&
                final_state.profile_buffer[0U] == 0xAABBCCDDU &&
                item_effect.mode_flags == 0x40U,
            "zero occurrence returns minus one before mode flags, clear, and index commit"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x800AU;
        list.primary_required = 0U;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U,
             .occurrence = 1U,
             .actor_primary_capacity = 10}
        );
        test.expect_true(
            result.return_eax == 1U && result.index_commit_calls == 2U &&
                list.primary_required == 10U &&
                list.selected_resource_token == 0U &&
                result.message_calls == 0U,
            "state processing publishes the primary threshold then succeeds on equal signed capacity"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x0805U;
        list.resource_owner_token = 0x75000000U;
        list.resource_head_token = 0x76000010U;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U,
             .resource_id = 5U,
             .primary_quantity = 1,
             .secondary_quantity = 0,
             .name = {}}
        };
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 1U && result.rebuild_calls == 1U &&
                result.resource_commit.head_writes == 1U &&
                list.resource_head_token == 0x76000000U &&
                result.resource_nodes_visited == 1U &&
                list.selected_resource_token == 0x76000000U &&
                result.index_commit_calls == 1U,
            "bit eleven rebuilds and selects a resource node with either positive quantity"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x0806U;
        list.resource_head_token = 0x76000000U;
        list.next_resource_head_token = 0U;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 0U && result.message_calls == 1U &&
                result.sample_calls == 1U &&
                result.message_token == 0x74000000U &&
                port.last_text == 0x004A7CD0U,
            "missing resource publishes one guarded message and sample"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.occurrence = 0U, .entry_eax = 0xFFFFFFFFU}
        );
        test.expect_true(
            result.return_eax == 0U && result.index_commit_calls == 0U &&
                result.nodes_visited == 0U,
            "zero occurrence returns before list-index commit"
        );
    }

    {
        QueryPort port;
        LegacyBattleActorListQueryState bad = list;
        bad.head_token = 0xDEADBEEFU;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &bad,
            0x005029D0U,
            port,
            {.occurrence = 1U, .return_table = table}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                result.index_commit_calls == 1U && result.nodes_visited == 0U,
            "missing node stops after the shared list-index commit"
        );
    }
}
