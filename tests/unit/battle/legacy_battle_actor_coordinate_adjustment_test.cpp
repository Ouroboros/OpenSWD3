#include "openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp"
#include "test.hpp"

void test_battle_actor_coordinate_adjustment(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateAdjustmentStatus;
    using openswd3::battle::LegacyBattleActorCoordinateFlags;
    using openswd3::battle::LegacyBattleActorCoordinatesState;
    using openswd3::battle::adjust_legacy_battle_actor_coordinates;
    using openswd3::battle::view_legacy_battle_actor_coordinates;

    const LegacyBattleActorCoordinateFlags entry_flags{
        .carry = false,
        .parity = false,
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = false,
        .sign = true,
        .overflow = true,
    };

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x7FFFU;
        actor.position_y = 0xFFFFU;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .actor_token = 0x005029D0U,
                .x_delta = 1U,
                .y_delta = 1U,
                .entry_eax = 0xCAFE1234U,
                .entry_edx = 0xBEEF5678U,
                .entry_flags = entry_flags,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::completed &&
                actor.position_x == 0x8000U && actor.position_y == 0U &&
                result.return_eax == 0xCAFE0001U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xBEEF0001U && result.flags.carry &&
                result.flags.parity && result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && result.flags.zero &&
                !result.flags.sign && !result.flags.overflow &&
                result.argument_reads == 2U && result.coordinate_adds == 2U,
            "actor coordinate adjustment preserves word wrap, registers, and final Y ADD flags"
        );
    }

    {
        openswd3::compat::u16 coordinate = 0xFFFFU;
        bool accessible = true;
        openswd3::battle::LegacyBattleActorCoordinatesView actor{
            .position_x = &coordinate,
            .position_y = &coordinate,
            .position_x_read_accessible = &accessible,
            .position_x_write_accessible = &accessible,
            .position_y_read_accessible = &accessible,
            .position_y_write_accessible = &accessible,
        };
        const auto result = adjust_legacy_battle_actor_coordinates(
            actor, {.x_delta = 1U, .y_delta = 1U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::completed &&
                coordinate == 1U && result.coordinate_adds == 2U &&
                !result.flags.carry && !result.flags.zero,
            "actor coordinate adjustment Y ADD observes the committed aliased X word"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 10U;
        actor.position_y = 20U;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .actor_token = 0x11112222U,
                .x_delta = 0xFFFFFFF6U,
                .y_delta = 0U,
                .entry_eax = 0xAAAA5555U,
                .entry_edx = 0xBBBB6666U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::completed &&
                actor.position_x == 0U && actor.position_y == 20U &&
                result.return_eax == 0xAAAAFFF6U &&
                result.return_ecx == 0x11112222U &&
                result.return_edx == 0xBBBB0000U && !result.flags.carry &&
                result.flags.parity && !result.flags.auxiliary_carry &&
                !result.flags.zero && !result.flags.sign &&
                !result.flags.overflow,
            "actor coordinate adjustment accepts a negative low-word X delta"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.position_y = 40U;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .actor_token = 0x12345678U,
                .x_delta = 10U,
                .y_delta = 20U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_edx = 0xBBBBBBBBU,
                .entry_flags = entry_flags,
                .x_argument_readable = false,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::
                        x_argument_read_typed_stop &&
                actor.position_x == 30U && actor.position_y == 40U &&
                result.return_eax == 0xAAAAAAAAU &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0xBBBBBBBBU && result.flags.sign &&
                result.flags.overflow &&
                !result.flags.auxiliary_carry_defined &&
                result.argument_reads == 0U && result.coordinate_adds == 0U,
            "actor coordinate adjustment first argument fault preserves entry state"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.position_y = 40U;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .actor_token = 0x12345678U,
                .x_delta = 10U,
                .y_delta = 20U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_edx = 0xBBBBBBBBU,
                .entry_flags = entry_flags,
                .y_argument_readable = false,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::
                        y_argument_read_typed_stop &&
                actor.position_x == 30U && actor.position_y == 40U &&
                result.return_eax == 0xAAAA000AU &&
                result.return_edx == 0xBBBBBBBBU && result.flags.sign &&
                result.flags.overflow && result.argument_reads == 1U &&
                result.coordinate_adds == 0U,
            "actor coordinate adjustment second argument fault preserves loaded AX"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.position_y = 40U;
        actor.position_x_write_accessible = false;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .x_delta = 10U,
                .y_delta = 20U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_edx = 0xBBBBBBBBU,
                .entry_flags = entry_flags,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::
                        position_x_add_typed_stop &&
                actor.position_x == 30U && actor.position_y == 40U &&
                result.return_eax == 0xAAAA000AU &&
                result.return_edx == 0xBBBB0014U && result.flags.sign &&
                result.flags.overflow && result.argument_reads == 2U &&
                result.coordinate_adds == 0U,
            "actor coordinate adjustment X access fault commits neither word nor ADD flags"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x7FFFU;
        actor.position_y = 40U;
        actor.position_y_read_accessible = false;
        const auto result = adjust_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            {
                .actor_token = 0x12345678U,
                .x_delta = 1U,
                .y_delta = 20U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_edx = 0xBBBBBBBBU,
                .entry_flags = entry_flags,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateAdjustmentStatus::
                        position_y_add_typed_stop &&
                actor.position_x == 0x8000U && actor.position_y == 40U &&
                result.return_eax == 0xAAAA0001U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0xBBBB0014U && !result.flags.carry &&
                result.flags.parity && result.flags.auxiliary_carry &&
                !result.flags.zero && result.flags.sign &&
                result.flags.overflow && result.argument_reads == 2U &&
                result.coordinate_adds == 1U,
            "actor coordinate adjustment Y access fault preserves committed X and X ADD flags"
        );
    }
}
