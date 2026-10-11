#include "openswd3/battle/legacy_battle_actor_turn_completion.hpp"

#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::LegacyBattleActorTurnCompletionStatus;
using openswd3::battle::query_legacy_battle_actor_turn_completion;
using openswd3::compat::u32;

}

void test_battle_actor_turn_completion(openswd3::test::Context& test) {
    for (const u32 value : std::array<u32, 8>{
             0U, 1U, 2U, 0xFFFFU, 0x10000U, 0x10001U, 0x80000000U, 0xFFFFFFFFU
         }) {
        u32 shared = value;
        const auto result = query_legacy_battle_actor_turn_completion(&shared);
        test.expect_true(
            result.status == LegacyBattleActorTurnCompletionStatus::completed &&
                result.value == value && shared == value,
            "completion query observes the complete DWORD without writing or normalizing it"
        );
        const auto field_stop = query_legacy_battle_actor_turn_completion(
            &shared, {.latch_readable = false}
        );
        const auto return_stop = query_legacy_battle_actor_turn_completion(
            &shared, {.return_address_readable = false}
        );
        const auto both_stop = query_legacy_battle_actor_turn_completion(
            &shared, {.latch_readable = false, .return_address_readable = false}
        );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                !field_stop.value.has_value() &&
                both_stop.status == field_stop.status &&
                !both_stop.value.has_value() &&
                return_stop.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        return_address_read_typed_stop &&
                return_stop.value == value && shared == value,
            "completion field failure precedes RET access while RET failure retains the observed DWORD"
        );
    }

    for (u32 bit = 0U; bit < 32U; ++bit) {
        const u32 value = 1U << bit;
        const auto result = query_legacy_battle_actor_turn_completion(&value);
        test.expect_true(
            result.status == LegacyBattleActorTurnCompletionStatus::completed &&
                result.value == value,
            "each completion bit survives the full-width read"
        );
    }

    u32 first = 0U;
    u32 second = 0x80000000U;
    const auto initial = query_legacy_battle_actor_turn_completion(&first);
    first = 0x10000U;
    const auto changed = query_legacy_battle_actor_turn_completion(&first);
    const auto separate = query_legacy_battle_actor_turn_completion(&second);
    test.expect_true(
        initial.value == 0U && changed.value == 0x10000U &&
            separate.value == 0x80000000U && first == 0x10000U &&
            second == 0x80000000U,
        "completion observations use live independent storage rather than a cached or copied latch"
    );
    for (const bool return_readable : {false, true}) {
        const auto missing = query_legacy_battle_actor_turn_completion(
            nullptr, {.return_address_readable = return_readable}
        );
        test.expect_true(
            missing.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                !missing.value.has_value(),
            "missing completion storage is a field failure rather than successful zero"
        );
    }
}
