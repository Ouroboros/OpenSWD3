#include "openswd3/battle/legacy_battle_group_a_value_pair.hpp"
#include "test.hpp"

void test_battle_group_a_value_pair(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAValuePairState;
    using openswd3::battle::LegacyBattleGroupAValuePairStatus;
    using openswd3::battle::publish_legacy_battle_group_a_value_pair;

    {
        LegacyBattleGroupAValuePairState state{
            .primary_value = 0x11111111U,
            .secondary_value = 0x22222222U,
        };
        const auto result = publish_legacy_battle_group_a_value_pair(
            state, 0x005029D0U, 0xA1B2C3D4U, 3U
        );
        test.expect_true(
            result.status == LegacyBattleGroupAValuePairStatus::completed &&
                state.primary_value == 0xA1B2C3D4U &&
                state.secondary_value == 0xA1B2C3D4U && result.writes == 2U &&
                result.return_eax == 0xA1B2C3D4U &&
                result.return_ecx == 0x005029D0U && result.return_edx == 3U,
            "group-A value publication writes both adjacent fields and preserves source index EDX"
        );
    }

    {
        LegacyBattleGroupAValuePairState state{
            .primary_value = 0x33333333U,
            .secondary_value = 0x44444444U,
        };
        const auto result = publish_legacy_battle_group_a_value_pair(
            state, 0U, 0x55667788U, 2U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAValuePairStatus::actor_typed_stop &&
                state.primary_value == 0x33333333U &&
                state.secondary_value == 0x44444444U && result.writes == 0U &&
                result.return_eax == 0x55667788U && result.return_ecx == 0U &&
                result.return_edx == 2U,
            "zero actor token stops at the first value-field write"
        );
    }
}
