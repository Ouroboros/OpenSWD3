#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp"
#include "test.hpp"

#include <algorithm>
#include <optional>

void test_battle_group_b_resource_cleanup(openswd3::test::Context& test) {
    using namespace openswd3::battle;

    {
        LegacyBattleGroupBStorage storage;
        test.expect_true(
            storage.construct(), "construct enemy resource allocations"
        );
        auto& actor = (*storage.actors())[2U];
        const auto token = actor.resource_token;
        actor.resource_bytes.fill(0xA5U);
        actor.resource_description = {1U, 2U};
        const auto description_alias = actor.resource_description;
        test.expect_true(
            !storage.resource_bytes(token).empty(),
            "record is registered before release"
        );
        const auto result = release_legacy_battle_group_b_resource(
            &actor, &storage, actor.object_token
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::completed &&
                result.resource_released && actor.resource_token == 0U &&
                storage.resource_bytes(token).empty() &&
                !storage.read_linked_action_next(token).has_value() &&
                !storage.release_heap_block(token).has_value() &&
                std::ranges::all_of(
                    actor.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                actor.resource_description.empty() &&
                description_alias.size() == 2U,
            "release retires the actual record before clearing its pointer and borrowed description view"
        );
        const auto repeated = release_legacy_battle_group_b_resource(
            &actor, nullptr, actor.object_token
        );
        test.expect_true(
            repeated.status ==
                    LegacyBattleGroupBResourceCleanupStatus::completed &&
                !repeated.resource_released,
            "cleared pointer skips the allocator on repeated cleanup"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{.object_token = 0x00525508U};
        actor.resource_bytes.fill(0x5AU);
        const auto result = release_legacy_battle_group_b_resource(
            &actor, nullptr, actor.object_token
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::completed &&
                !result.resource_released &&
                std::ranges::all_of(
                    actor.resource_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ),
            "zero resource pointer needs no storage and preserves stale bytes"
        );
    }

    {
        const auto result = release_legacy_battle_group_b_resource(
            nullptr, nullptr, 0x00525508U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                !result.resource_released,
            "missing actor stops at the original resource field read"
        );
    }

    {
        LegacyBattleGroupBStorage storage;
        test.expect_true(
            storage.construct(),
            "construct enemy record for invalid actor access"
        );
        auto& actor = (*storage.actors())[0U];
        const auto token = actor.resource_token;
        actor.resource_bytes.fill(0x6BU);
        const auto result =
            release_legacy_battle_group_b_resource(&actor, &storage, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                actor.resource_token == token &&
                !storage.resource_bytes(token).empty() &&
                std::ranges::all_of(
                    actor.resource_bytes,
                    [](const auto value) { return value == 0x6BU; }
                ),
            "invalid actor address preserves the registered resource and its bytes"
        );
    }

    {
        LegacyBattleGroupBStorage storage;
        test.expect_true(
            storage.construct(), "construct enemy record for release failure"
        );
        auto& actor = (*storage.actors())[1U];
        const auto token = actor.resource_token;
        actor.resource_bytes.fill(0x7CU);
        actor.resource_description = {3U, 4U};
        test.expect_true(
            storage.release_heap_block(token).has_value(),
            "retire allocation while retaining stale actor pointer"
        );
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_b_resource(
                &actor, &storage, actor.object_token
            ));
        } catch (const std::bad_optional_access&) {
            caught = true;
        }

        test.expect_true(
            caught && actor.resource_token == token &&
                actor.resource_description.size() == 2U &&
                std::ranges::all_of(
                    actor.resource_bytes,
                    [](const auto value) { return value == 0x7CU; }
                ),
            "rejected actual release preserves the pointer, record and description view"
        );
    }
}
