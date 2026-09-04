#include "openswd3/battle/legacy_battle_actor_availability_block.hpp"

#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::LegacyBattleActorAvailabilityBlockState;
using openswd3::battle::LegacyBattleActorAvailabilityBlockStatus;
using openswd3::battle::set_legacy_battle_actor_availability_block;

void test_complete_write(openswd3::test::Context& context) {
    LegacyBattleActorAvailabilityBlockState actor{.value = 0x11223344U};
    const auto result = set_legacy_battle_actor_availability_block(
        &actor,
        {
            .value = 0x89ABCDEFU,
            .actor_token = 0x005029D0U,
            .entry_eax = 0xAABBCCDDU,
            .entry_edx = 0x55667788U,
        }
    );

    context.expect_equal(
        result.status,
        LegacyBattleActorAvailabilityBlockStatus::completed,
        "write completes"
    );
    context.expect_equal(actor.value, 0x89ABCDEFU, "complete dword is stored");
    context.expect_equal(result.actor_writes, 1U, "one actor write completes");
    context.expect_equal(
        result.return_eax, 0x89ABCDEFU, "stack argument replaces EAX"
    );
    context.expect_equal(
        result.return_ecx, 0x005029D0U, "actor token preserves ECX"
    );
    context.expect_equal(
        result.return_edx, 0x55667788U, "entry EDX is preserved"
    );
}

void test_write_stops(openswd3::test::Context& context) {
    for (const bool null_actor : std::array{false, true}) {
        LegacyBattleActorAvailabilityBlockState actor{
            .value = 0x11223344U,
            .write_accessible = null_actor,
        };
        const auto result = set_legacy_battle_actor_availability_block(
            null_actor ? nullptr : &actor,
            {
                .value = 1U,
                .actor_token = 0x00505904U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );

        context.expect_equal(
            result.status,
            LegacyBattleActorAvailabilityBlockStatus::actor_write_typed_stop,
            "unavailable actor write stops"
        );
        context.expect_equal(
            actor.value, 0x11223344U, "write stop preserves the owner"
        );
        context.expect_equal(
            result.actor_writes, 0U, "write stop completes no actor write"
        );
        context.expect_equal(
            result.return_eax, 1U, "write stop follows the argument load"
        );
        context.expect_equal(
            result.return_ecx, 0x00505904U, "write stop preserves ECX"
        );
        context.expect_equal(
            result.return_edx, 0xCAFEBABEU, "write stop preserves EDX"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context context;
    test_complete_write(context);
    test_write_stops(context);
    return context.exit_code();
}
