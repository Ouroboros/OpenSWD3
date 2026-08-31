#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp"
#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionProfileModeRequest;
using openswd3::battle::LegacyBattleGroupBActionProfileModeStatus;
using openswd3::compat::u16;

void write_word(
    std::array<openswd3::compat::u8, 0xA4U>& bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<openswd3::compat::u8>(value);
    bytes[offset + 1U] = static_cast<openswd3::compat::u8>(value >> 8U);
}

}  // namespace

void test_battle_group_b_action_profile_mode(openswd3::test::Context& test) {
    using openswd3::battle::compose_legacy_battle_group_b_action_profile_mode;

    {
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            nullptr,
            mon,
            {.actor_token = 0x00525508U,
             .entry_eax = 0x11111111U,
             .entry_ecx = 0x22222222U,
             .entry_edx = 0x33333333U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        actor_state_typed_stop &&
                result.profile_load_calls == 0U,
            "group B profile mode stops before MON access when the actor is unavailable"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0U,
        };
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor, mon, {.actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        resource_state_typed_stop &&
                result.profile_dwords_cleared == 10U &&
                result.profile_load_calls == 0U,
            "group B profile mode preserves the clear prefix before a null resource stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x60U, 0x1234U);
        write_word(actor.resource_bytes, 0x56U, 7U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.set_profile_word(0x14U, 0x5678U);
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            mon,
            {.actor_token = actor.object_token,
             .entry_eax = 0x71000000U,
             .entry_ecx = 0x72000000U,
             .entry_edx = 0x73000000U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::completed &&
                result.profile_id == 0x1234U &&
                result.profile_load_calls == 1U && mon.open_calls == 1U &&
                mon.seek_calls == 3U && mon.read_calls == 3U &&
                mon.allocation_calls == 1U && mon.release_calls == 1U &&
                actor.action_composition.derived_words[0U] == 7U &&
                actor.action_composition.action_kind == 1U,
            "group B profile mode loads MON data and applies the resource word"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        write_word(actor.resource_bytes, 0x60U, 0x1234U);
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.allocation_succeeds = false;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor, mon, {.actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        profile_load_typed_stop &&
                result.profile_load_calls == 1U && mon.release_calls == 0U,
            "group B profile mode stops at the original zero-allocation access point"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{};
        actor.action_composition.profile_mode_selector = 1U;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{0x02U};
        actor.action_configuration.profile_buffer[0x14U] = std::byte{0x34U};
        actor.action_configuration.profile_buffer[0x15U] = std::byte{0x12U};
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor, mon, LegacyBattleGroupBActionProfileModeRequest{}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::completed &&
                result.profile_load_calls == 0U &&
                result.return_eax == 0x1234U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.action_kind == 0U,
            "group B profile mode preserves the preloaded mode-two fast path"
        );
    }
}
