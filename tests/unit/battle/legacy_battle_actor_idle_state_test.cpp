#include "openswd3/battle/legacy_battle_actor_idle_state.hpp"

#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::LegacyBattleActorIdleStateStatus;
using openswd3::battle::query_legacy_battle_actor_idle_state;
using openswd3::compat::u32;

}  // namespace

void test_battle_actor_idle_state(openswd3::test::Context& test) {
    for (const u32 value : std::array<u32, 8>{
             0U, 1U, 2U, 0xFFFFU, 0x10000U, 0x10001U, 0x80000000U, 0xFFFFFFFFU
         }) {
        u32 state = value;
        const auto result = query_legacy_battle_actor_idle_state(&state);
        test.expect_true(
            result.status == LegacyBattleActorIdleStateStatus::completed &&
                result.value == value && state == value,
            "idle query reads the complete DWORD without normalization or writes"
        );
    }

    for (u32 bit = 0U; bit < 32U; ++bit) {
        const u32 state = 1U << bit;
        const auto result = query_legacy_battle_actor_idle_state(&state);
        test.expect_true(
            result.status == LegacyBattleActorIdleStateStatus::completed &&
                result.value == state,
            "idle query preserves every individual DWORD bit"
        );
    }

    {
        u32 first = 0U;
        u32 second = 0xFFFFFFFFU;
        const auto before = query_legacy_battle_actor_idle_state(&first);
        first = 0x10001U;
        const auto after = query_legacy_battle_actor_idle_state(&first);
        const auto separate = query_legacy_battle_actor_idle_state(&second);
        test.expect_true(
            before.value == 0U && after.value == 0x10001U &&
                separate.value == 0xFFFFFFFFU && first == 0x10001U &&
                second == 0xFFFFFFFFU,
            "idle query observes live changes and keeps distinct storage independent"
        );
    }

    {
        const u32 state = 0xCAFEBABEU;
        const auto missing = query_legacy_battle_actor_idle_state(nullptr);
        const auto unreadable = query_legacy_battle_actor_idle_state(
            &state, {.latch_readable = false}
        );
        const auto both = query_legacy_battle_actor_idle_state(
            &state, {.latch_readable = false, .return_address_readable = false}
        );
        const auto missing_return = query_legacy_battle_actor_idle_state(
            nullptr, {.return_address_readable = false}
        );
        test.expect_true(
            missing.status ==
                    LegacyBattleActorIdleStateStatus::latch_read_typed_stop &&
                !missing.value &&
                unreadable.status ==
                    LegacyBattleActorIdleStateStatus::latch_read_typed_stop &&
                !unreadable.value &&
                both.status ==
                    LegacyBattleActorIdleStateStatus::latch_read_typed_stop &&
                !both.value &&
                missing_return.status ==
                    LegacyBattleActorIdleStateStatus::latch_read_typed_stop &&
                !missing_return.value && state == 0xCAFEBABEU,
            "missing storage and failed field access stop before RET without an observed value"
        );
        const auto return_failure = query_legacy_battle_actor_idle_state(
            &state, {.return_address_readable = false}
        );
        test.expect_true(
            return_failure.status ==
                    LegacyBattleActorIdleStateStatus::
                        return_address_read_typed_stop &&
                return_failure.value == state,
            "RET failure retains the observed full DWORD"
        );
        const u32 zero = 0U;
        const auto zero_return_failure = query_legacy_battle_actor_idle_state(
            &zero, {.return_address_readable = false}
        );
        test.expect_true(
            zero_return_failure.status ==
                    LegacyBattleActorIdleStateStatus::
                        return_address_read_typed_stop &&
                zero_return_failure.value.has_value() &&
                *zero_return_failure.value == 0U,
            "an observed zero remains distinct from a failed field read"
        );
    }
}
