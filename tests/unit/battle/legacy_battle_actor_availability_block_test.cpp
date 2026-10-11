#include "openswd3/battle/legacy_battle_actor_availability_block.hpp"
#include "test.hpp"

#include <array>

int main() {
    openswd3::test::Context context;
    using openswd3::battle::LegacyBattleActorAvailabilityBlockState;
    using Status = openswd3::battle::LegacyBattleActorAvailabilityBlockStatus;
    using openswd3::battle::set_legacy_battle_actor_availability_block;

    for (const auto value : std::array{
             0U, 1U, 0x7FFFFFFFU, 0x80000000U, 0x89ABCDEFU, 0xFFFFFFFFU
         }) {
        LegacyBattleActorAvailabilityBlockState actor{.value = 0x11223344U};
        const auto result =
            set_legacy_battle_actor_availability_block(&actor, value);
        context.expect_equal(
            result.status, Status::completed, "write completes"
        );
        context.expect_equal(
            actor.value, value, "all DWORD bits reach shared storage"
        );
        actor.value = 0x55667788U;
        const auto repeated =
            set_legacy_battle_actor_availability_block(&actor, value);
        context.expect_true(
            repeated.status == Status::completed && actor.value == value,
            "later writes replace the current shared value"
        );
    }

    for (const bool null_actor : std::array{false, true}) {
        LegacyBattleActorAvailabilityBlockState actor{
            .value = 0x11223344U,
            .write_accessible = null_actor,
        };
        const auto result = set_legacy_battle_actor_availability_block(
            null_actor ? nullptr : &actor, 1U
        );
        context.expect_equal(
            result.status,
            Status::actor_write_typed_stop,
            "unavailable actor write fails"
        );
        context.expect_equal(
            actor.value, 0x11223344U, "failed write preserves storage"
        );
    }

    return context.exit_code();
}
