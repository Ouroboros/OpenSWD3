#include "openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp"
#include "test.hpp"

namespace {

void set_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const openswd3::compat::u16 value
) {
    profile[offset] = static_cast<std::byte>(value & 0xFFU);
    profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
}

}  // namespace

void test_battle_actor_message_percent_refresh(openswd3::test::Context& test) {
    using namespace openswd3::battle;

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.message_percent = 0x4321U;
        LegacyBattleGroupAAttributeAggregationState attributes;
        set_profile_word(attributes.embedded_profiles[0U], 0x48U, 0xFFFEU);
        LegacyBattleFixedObjectState fixed;
        fixed.object_words[2U][0U] = 0xDEAD0000U;
        const auto result = refresh_legacy_battle_actor_message_percent(
            &actor, attributes, fixed
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::completed &&
                result.message_percent == 0x4321U &&
                actor.message_percent == 0x4321U,
            "unmatched signed profile kinds retain the actor percentage without reading the fixed chain"
        );
    }

    for (const std::size_t slot : {0U, 1U}) {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAAttributeAggregationState attributes;
        set_profile_word(attributes.embedded_profiles[slot], 0x48U, 30U);
        set_profile_word(attributes.embedded_profiles[slot], 0x50U, 0xFFFFU);
        LegacyBattleFixedObjectState fixed;
        fixed.object_words[2U][1U] = 0x1234FFFFU;
        fixed.object_words[2U][2U] = 0xDEADBEEFU;
        const auto result = refresh_legacy_battle_actor_message_percent(
            &actor, attributes, fixed
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::completed &&
                result.message_percent == 0xBEEFU &&
                actor.message_percent == 0xBEEFU,
            "either profile slot selects the fixed root by its unsigned item word and publishes only the percentage word"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAAttributeAggregationState attributes;
        for (auto& profile : attributes.embedded_profiles) {
            set_profile_word(profile, 0x48U, 30U);
        }
        set_profile_word(attributes.embedded_profiles[0U], 0x50U, 7U);
        set_profile_word(attributes.embedded_profiles[1U], 0x50U, 8U);
        LegacyBattleFixedObjectState fixed;
        fixed.object_words[2U][0U] = 0x71000000U;
        fixed.fixed_count_nodes.push_back({
            .legacy_token = 0x71000000U,
            .words = {0U, 7U, 93U, 0U, 0U},
        });
        const auto result = refresh_legacy_battle_actor_message_percent(
            &actor, attributes, fixed
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::completed &&
                result.message_percent == 93U && actor.message_percent == 93U,
            "the first matching profile wins and reads the actual linked node"
        );

        fixed.fixed_count_nodes.front().words[1U] = 8U;
        const auto missing = refresh_legacy_battle_actor_message_percent(
            &actor, attributes, fixed
        );
        test.expect_true(
            missing.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::completed &&
                missing.message_percent == 0U && actor.message_percent == 0U,
            "an absent first-profile item publishes zero without trying the second matching profile"
        );

        actor.message_percent = 41U;
        fixed.fixed_count_nodes.front().words[1U] = 7U;
        fixed.fixed_count_nodes.front().accessible_bytes = 8U;
        const auto stopped = refresh_legacy_battle_actor_message_percent(
            &actor, attributes, fixed
        );
        test.expect_true(
            stopped.status == LegacyBattleActorMessagePercentRefreshStatus::
                                  fixed_record_typed_stop &&
                actor.message_percent == 41U,
            "an inaccessible matching value stops before overwriting the actor percentage"
        );
    }

    {
        LegacyBattleGroupAAttributeAggregationState attributes;
        LegacyBattleFixedObjectState fixed;
        const auto result = refresh_legacy_battle_actor_message_percent(
            nullptr, attributes, fixed
        );
        test.expect_true(
            result.status == LegacyBattleActorMessagePercentRefreshStatus::
                                 actor_state_typed_stop,
            "a missing actor stops before accessing its embedded profiles"
        );
    }
}
