#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionProfileSelectionStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_word(
    std::array<u8, 0xA4U>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

}  // namespace

void test_battle_group_b_action_profile_selection(
    openswd3::test::Context& test
) {
    using openswd3::battle::select_legacy_battle_group_b_action_profile;

    {
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        const auto result = select_legacy_battle_group_b_action_profile(
            nullptr, {}, mon, {.actor_token = 0x00525508U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        actor_state_typed_stop &&
                result.profile_load_calls == 0U,
            "group B profile selection stops before MON access for a null actor"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x72U, 0x1111U);
        write_word(actor.resource_bytes, 0x76U, 0x2222U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.set_profile_dword(0x0CU, 0x34560000U);
        u32 output = 0xFFFFFFFFU;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            {.dword = &output},
            mon,
            {.selector_argument = 1U,
             .output_token = 0x00600000U,
             .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.profile_id == 0x1111U &&
                result.derived_word == 0x3456U &&
                result.profile_load_calls == 1U &&
                actor.action_composition.derived_words[0U] == 0x3456U &&
                actor.action_composition.profile_mode_selector == 1U &&
                actor.action_composition.action_kind == 1U &&
                output == 0xFFFFFFFFU,
            "group B profile selection loads the selector-one MON record and chooses mode one"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x76U, 0x2222U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.set_profile_dword(0x0CU, 0x45670002U);
        mon.set_profile_word(0x14U, 0x1234U);
        u32 output = 0U;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            {.dword = &output},
            mon,
            {.selector_argument = 2U,
             .output_token = 0x00600000U,
             .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.profile_id == 0x2222U &&
                result.derived_word == 0x4567U &&
                result.output_value == 0x1234U && output == 0x1234U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.action_kind == 0U,
            "group B profile selection publishes the typed mode-two output"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x72U, 0x1111U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.set_profile_dword(0x0CU, 0x00000002U);
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            {},
            mon,
            {.selector_argument = 1U, .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                LegacyBattleGroupBActionProfileSelectionStatus::
                    output_state_typed_stop,
            "group B profile selection stops at the original missing output write"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x72U, 0x1111U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.allocation_succeeds = false;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            {},
            mon,
            {.selector_argument = 1U, .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        profile_load_typed_stop &&
                result.profile_load_calls == 1U,
            "group B profile selection propagates the MON allocation typed stop"
        );
    }
}
