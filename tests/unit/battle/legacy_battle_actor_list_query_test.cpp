#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include "test.hpp"

namespace {

class ActionPort final
    : public openswd3::battle::LegacyBattleActorListActionPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorListActionReply
    release_resource(
        const openswd3::compat::u32,
        const openswd3::compat::u32 first,
        const openswd3::compat::u32 second,
        const openswd3::compat::u32,
        const openswd3::compat::u32,
        const openswd3::compat::u32
    ) override {
        ++release_calls;
        arguments_zero = first == 0U && second == 0U;
        return {.eax = 4U, .ecx = 5U, .edx = 6U};
    }
    openswd3::compat::u32 release_calls{};
    bool arguments_zero{};
};

class StatePort final
    : public openswd3::battle::LegacyBattleActorListStatePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorListStateCallReply
    rebuild_resource_list(const openswd3::compat::u32) override {
        ++rebuild_calls;
        return {};
    }
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
    openswd3::compat::u32 rebuild_calls{};
    openswd3::compat::u32 message_calls{};
    openswd3::compat::u32 sample_calls{};
    openswd3::compat::u32 last_text{};
};

class QueryPort final
    : public openswd3::battle::LegacyBattleActorListQueryPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorListProfileReply
    load_profile(
        const openswd3::compat::u16 profile_id,
        const openswd3::compat::u32 eax,
        const openswd3::compat::u32 ecx,
        const openswd3::compat::u32 edx
    ) override {
        ++calls;
        last_profile = profile_id;
        return {.eax = eax, .ecx = ecx, .edx = edx, .profile_index = index};
    }
    openswd3::compat::u32 calls{};
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
                result.matches == 2U && port.calls == 0U,
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
        ActionPort port;
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, 0x005029D0U, port
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
        item_effect.mode_flags = 0x80U;
        list.selected_resource_token = 0x76000000U;
        list.primary_required = 9U;
        ActionPort port;
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, 0x005029D0U, port
        );
        test.expect_true(
            result.release_calls == 1U && result.refresh_calls == 1U &&
                port.arguments_zero && list.selected_resource_token == 0U &&
                list.primary_required == 0U && result.return_eax == 4U &&
                result.return_edx == 0U,
            "selected resource path releases with two zero arguments then refreshes using returned registers"
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
        ActionPort port;
        list.secondary_required = 0U;
        const auto result = execute_legacy_battle_actor_list_action(
            &list,
            &item_effect,
            nullptr,
            0x005029D0U,
            port,
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
        list.resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U,
             .resource_id = 5U,
             .primary_quantity = 1,
             .secondary_quantity = 0}
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
                result.resource_nodes_visited == 1U &&
                list.selected_resource_token == 0x76000000U &&
                result.index_commit_calls == 1U,
            "bit eleven rebuilds and selects a resource node with either positive quantity"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x0806U;
        list.resource_head_token = 0U;
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
