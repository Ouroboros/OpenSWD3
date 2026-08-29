#include "openswd3/battle/legacy_battle_group_a_resource_pair.hpp"
#include "test.hpp"

void test_battle_group_a_resource_pair(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAResourcePairState;
    using openswd3::battle::LegacyBattleGroupAResourcePairStatus;
    using openswd3::battle::publish_legacy_battle_group_a_resource_pair;

    {
        LegacyBattleGroupAResourcePairState state{
            .primary_token = 0x11111111U,
            .secondary_token = 0x22222222U,
        };
        const auto result = publish_legacy_battle_group_a_resource_pair(
            state, 0x005029D0U, 0x004A9940U, 0xAABBCCDDU
        );
        test.expect_true(
            result.status == LegacyBattleGroupAResourcePairStatus::completed &&
                state.primary_token == 0x004A9940U &&
                state.secondary_token == 0x004A9940U && result.writes == 2U &&
                result.return_eax == 0x004A9940U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xAABBCCDDU,
            "group-A resource publication writes both adjacent fields and preserves entry EDX"
        );
    }

    {
        LegacyBattleGroupAResourcePairState state{
            .primary_token = 0x33333333U,
            .secondary_token = 0x44444444U,
        };
        const auto result = publish_legacy_battle_group_a_resource_pair(
            state, 0U, 0x004A9940U, 0x12345678U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAResourcePairStatus::actor_typed_stop &&
                state.primary_token == 0x33333333U &&
                state.secondary_token == 0x44444444U && result.writes == 0U &&
                result.return_eax == 0x004A9940U && result.return_ecx == 0U &&
                result.return_edx == 0x12345678U,
            "zero actor token stops at the first resource-field write"
        );
    }
}
