#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"
#include "openswd3/battle/legacy_battle_group_a_storage.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <memory>
#include <optional>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

struct Fixture {
    std::unique_ptr<LegacyBattleStartupState> startup{
        std::make_unique<LegacyBattleStartupState>()
    };
    std::unique_ptr<LegacyBattleActionDispatchState> action{
        std::make_unique<LegacyBattleActionDispatchState>()
    };
    LegacyBattleGroupAStorage storage{*startup, *action};
};

}  // namespace

void test_battle_group_a_resource_cleanup(openswd3::test::Context& test) {
    for (const u32 mask : {0U, 1U, 2U, 3U}) {
        Fixture fixture;
        test.expect_true(
            fixture.storage.construct(), "construct actual party records"
        );
        auto& party = fixture.startup->party[0U];
        const auto primary = party.configuration.actor_record_token;
        const auto secondary = fixture.storage.allocate_profile();
        party.configuration.profile_token = secondary;
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = (mask & 1U) != 0U ? primary : 0U,
            .secondary_resource_token = (mask & 2U) != 0U ? secondary : 0U,
        };

        const auto result = release_legacy_battle_group_a_resources(
            &state, &fixture.storage, 0x005029D0U
        );
        test.expect_true(
            secondary != 0U &&
                result.status ==
                    LegacyBattleGroupAResourceCleanupStatus::completed &&
                result.primary_resource_released == ((mask & 1U) != 0U) &&
                result.secondary_resource_released == ((mask & 2U) != 0U) &&
                state.primary_resource_token == 0U &&
                state.secondary_resource_token == 0U &&
                fixture.storage.record_bytes(primary).empty() ==
                    ((mask & 1U) != 0U) &&
                fixture.storage.record_bytes(secondary).empty() ==
                    ((mask & 2U) != 0U),
            "each nonzero resource is retired while independent zero branches leave allocations registered"
        );
        const auto repeated = release_legacy_battle_group_a_resources(
            &state, nullptr, 0x005029D0U
        );
        test.expect_true(
            repeated.status ==
                    LegacyBattleGroupAResourceCleanupStatus::completed &&
                !repeated.primary_resource_released &&
                !repeated.secondary_resource_released,
            "two cleared pointers need no storage and do not repeat releases"
        );
    }

    {
        const auto missing = release_legacy_battle_group_a_resources(
            nullptr, nullptr, 0x005029D0U
        );
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 7U, .secondary_resource_token = 9U
        };
        const auto invalid =
            release_legacy_battle_group_a_resources(&state, nullptr, 0U);
        test.expect_true(
            missing.status ==
                    LegacyBattleGroupAResourceCleanupStatus::
                        actor_state_typed_stop &&
                invalid.status ==
                    LegacyBattleGroupAResourceCleanupStatus::
                        actor_state_typed_stop &&
                state.primary_resource_token == 7U &&
                state.secondary_resource_token == 9U,
            "missing owner and invalid actor address stop at the first field access without using storage"
        );
    }

    {
        Fixture fixture;
        test.expect_true(
            fixture.storage.construct(),
            "construct primary record for first-release failure"
        );
        auto& party = fixture.startup->party[0U];
        const auto primary = party.configuration.actor_record_token;
        const auto secondary = fixture.storage.allocate_profile();
        party.secondary_resource_token = secondary;
        test.expect_true(
            fixture.storage.release_heap_block(secondary).has_value(),
            "retire secondary allocation to leave an invalid pointer"
        );
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_a_resources(
                party.configuration.actor_record_token,
                party.secondary_resource_token,
                &fixture.storage,
                0x005029D0U
            ));
        } catch (const std::bad_optional_access&) {
            caught = true;
        }

        test.expect_true(
            caught && party.secondary_resource_token == secondary &&
                party.configuration.actor_record_token == primary &&
                !fixture.storage.record_bytes(primary).empty(),
            "secondary release failure preserves both pointers and does not release the primary record"
        );
    }

    {
        Fixture fixture;
        test.expect_true(
            fixture.storage.construct(),
            "construct shared record for release-order failure"
        );
        auto& party = fixture.startup->party[0U];
        const auto token = party.configuration.actor_record_token;
        party.secondary_resource_token = token;
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_a_resources(
                party.configuration.actor_record_token,
                party.secondary_resource_token,
                &fixture.storage,
                0x005029D0U
            ));
        } catch (const std::bad_optional_access&) {
            caught = true;
        }

        test.expect_true(
            caught && party.secondary_resource_token == 0U &&
                party.configuration.actor_record_token == token &&
                fixture.storage.record_bytes(token).empty(),
            "aliased pointers prove secondary releases first and primary failure preserves the completed prefix"
        );
    }
}
