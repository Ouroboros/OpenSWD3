#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include "test.hpp"

namespace {

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
        .nodes = {
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
            .nodes = {
                {.token = 0x73000000U,
                 .category_flags = 0x0CU,
                 .mode_flags = 0x04U,
                 .type = 0x1FU,
                 .profile_id = 9U,
                 .value_flags = 0xFFFFU,
                 .text = "profile"}
            },
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
