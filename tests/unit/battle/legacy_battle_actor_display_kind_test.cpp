#include "openswd3/battle/legacy_battle_actor_display_kind.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorDisplayKindStatus;
using openswd3::battle::query_legacy_battle_actor_display_kind;
using openswd3::compat::u16;

}  // namespace

void test_battle_actor_display_kind(openswd3::test::Context& test) {
    for (const u16 value :
         std::array<u16, 7>{0U, 1U, 0xFFU, 0x100U, 0x7FFFU, 0x8000U, 0xFFFFU}) {
        std::array<u16, 3> fields{0x1234U, value, 0x5678U};
        const auto before = fields;
        const auto result = query_legacy_battle_actor_display_kind(&fields[1U]);
        test.expect_true(
            result.status == LegacyBattleActorDisplayKindStatus::completed &&
                result.value == value && fields == before,
            "display-kind query reads the complete WORD without changing adjacent fields"
        );
    }

    {
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_unique<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& party = startup->party[2U].item_effect_application.display_kind;
        auto& enemy =
            (*startup->group_b_lifecycle)[3U].action_composition.display_kind;
        party = 0x1122U;
        enemy = 0x5566U;
        const auto first = query_legacy_battle_actor_display_kind(&party);
        party = 0xFFFFU;
        const auto second = query_legacy_battle_actor_display_kind(&party);
        const auto separate = query_legacy_battle_actor_display_kind(&enemy);
        test.expect_true(
            first.value == 0x1122U && second.value == 0xFFFFU &&
                separate.value == 0x5566U && party == 0xFFFFU &&
                enemy == 0x5566U,
            "display-kind query borrows separate actual actors and observes live writes without a cache"
        );
    }

    {
        const u16 value = 0xBEEFU;
        const auto missing = query_legacy_battle_actor_display_kind(nullptr);
        const auto unreadable = query_legacy_battle_actor_display_kind(
            &value, {.display_kind_readable = false}
        );
        const auto both = query_legacy_battle_actor_display_kind(
            &value,
            {.display_kind_readable = false, .return_address_readable = false}
        );
        const auto returned = query_legacy_battle_actor_display_kind(
            &value, {.return_address_readable = false}
        );
        test.expect_true(
            missing.status ==
                    LegacyBattleActorDisplayKindStatus::
                        display_kind_read_typed_stop &&
                !missing.value.has_value() &&
                unreadable.status == missing.status &&
                !unreadable.value.has_value() &&
                both.status == missing.status && !both.value.has_value() &&
                returned.status ==
                    LegacyBattleActorDisplayKindStatus::
                        return_address_read_typed_stop &&
                returned.value == 0xBEEFU && value == 0xBEEFU,
            "field failure has no value and precedes return failure, which preserves the observed WORD"
        );
    }
}
