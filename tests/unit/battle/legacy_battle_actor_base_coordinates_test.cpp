#include "openswd3/battle/legacy_battle_actor_base_coordinates.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_base_coordinates(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorBaseCoordinateQueryRequest;
    using openswd3::battle::LegacyBattleActorBaseCoordinateQueryStatus;
    using openswd3::battle::LegacyBattleActorCoordinatesState;

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x0010U;
        actor.source_y_offset = 0x0020U;
        actor.position_y = 0x8000U;
        actor.target_phase_y_adjustment = 1;
        openswd3::compat::u16 output_x = 0xAAAAU;
        openswd3::compat::u16 output_y = 0xBBBBU;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {
                    .actor_token = 0x11112222U,
                    .output_x_token = 0x33334444U,
                    .output_y_token = 0x55556666U,
                    .entry_eax = 0xCAFE0000U,
                    .entry_edx = 0x77778888U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                output_x == 0xFFF0U && output_y == 0x7FFFU &&
                result.return_eax == 0xCAFE7FFFU &&
                result.return_ecx == 0x55556666U &&
                result.return_edx == 0x33334444U && result.actor_reads == 4U &&
                result.output_pointer_reads == 2U &&
                result.output_writes == 2U && !result.flags.carry &&
                result.flags.parity && result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && !result.flags.zero &&
                !result.flags.sign && result.flags.overflow,
            "base coordinates preserve final Y SUB flags and register residues"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 100U;
        actor.source_y_offset = 1U;
        actor.position_y = 200U;
        actor.target_phase_y_adjustment = 9;
        openswd3::compat::u16 output_y = 0U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &actor.position_y,
                &output_y
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                actor.position_y == 99U && output_y == 90U &&
                result.output_x == 99U && result.output_y == 90U,
            "base coordinate X store aliases the later actor Y source"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 90U;
        actor.source_y_offset = 10U;
        actor.position_y = 60U;
        actor.target_phase_y_adjustment = 20;
        openswd3::compat::u16 output = 0U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output,
                &output
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                output == 40U && result.output_writes == 2U,
            "base coordinate aliased outputs retain X then Y word stores"
        );
    }

    {
        const LegacyBattleActorBaseCoordinateQueryRequest request{
            .actor_token = 0x11112222U,
            .output_x_token = 0x33334444U,
            .output_y_token = 0x55556666U,
            .entry_eax = 0xAAAA0000U,
            .entry_edx = 0xBBBBCCCCU,
            .entry_flags = {
                .carry = true,
                .parity = false,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = false,
                .zero = false,
                .sign = true,
                .overflow = true,
            },
        };
        openswd3::compat::u16 output_x = 0x7777U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                {}, &output_x, &output_y, request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        position_x_read_typed_stop &&
                result.return_eax == 0xAAAA0000U &&
                result.return_ecx == 0x11112222U &&
                result.return_edx == 0xBBBBCCCCU && result.flags.carry &&
                !result.flags.parity && !result.flags.auxiliary_carry_defined &&
                result.flags.sign && result.flags.overflow &&
                result.actor_reads == 0U && result.output_writes == 0U,
            "base coordinate missing actor stops at the first X source read"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x1234U;
        openswd3::compat::u16 output_x = 0x7777U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {
                    .actor_token = 0x11112222U,
                    .output_x_token = 0x33334444U,
                    .entry_eax = 0xAAAA0000U,
                    .entry_edx = 0xBBBBCCCCU,
                    .output_x_pointer_readable = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        output_x_pointer_read_typed_stop &&
                result.return_eax == 0xAAAA1234U &&
                result.return_edx == 0xBBBBCCCCU && result.actor_reads == 1U &&
                result.output_pointer_reads == 0U && output_x == 0x7777U,
            "base coordinate X stack-pointer fault preserves the loaded AX"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x1234U;
        actor.source_y_offset_read_accessible = false;
        openswd3::compat::u16 output_x = 0x7777U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {
                    .actor_token = 0x11112222U,
                    .output_x_token = 0x33334444U,
                    .entry_eax = 0xAAAA0000U,
                    .entry_edx = 0xBBBBCCCCU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        x_adjustment_read_typed_stop &&
                result.return_eax == 0xAAAA1234U &&
                result.return_edx == 0x33334444U && result.actor_reads == 1U &&
                result.output_pointer_reads == 1U && output_x == 0x7777U,
            "base coordinate X adjustment fault follows the X pointer load"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 0x0010U;
        actor.source_y_offset = 0x0020U;
        openswd3::compat::u16 output_x = 0x7777U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {.output_x_writable = false}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        output_x_write_typed_stop &&
                result.output_x == 0xFFF0U && result.flags.carry &&
                result.flags.parity && !result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && !result.flags.zero &&
                result.flags.sign && !result.flags.overflow &&
                result.actor_reads == 2U && result.output_writes == 0U &&
                output_x == 0x7777U,
            "base coordinate X write fault retains the X SUB flags"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.source_y_offset = 10U;
        actor.position_y_read_accessible = false;
        openswd3::compat::u16 output_x = 0x7777U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {.entry_eax = 0xAAAA0000U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        position_y_read_typed_stop &&
                output_x == 20U && output_y == 0x8888U &&
                result.return_eax == 0xAAAA0014U &&
                result.output_writes == 1U && result.actor_reads == 2U,
            "base coordinate Y source fault preserves the committed X word"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.source_y_offset = 10U;
        actor.position_y = 0x4567U;
        actor.target_phase_y_adjustment_read_accessible = false;
        openswd3::compat::u16 output_x = 0U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {.entry_eax = 0xAAAA0000U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        y_adjustment_read_typed_stop &&
                output_x == 20U && output_y == 0x8888U &&
                result.return_eax == 0xAAAA4567U &&
                result.output_writes == 1U && result.actor_reads == 3U,
            "base coordinate Y adjustment fault retains loaded Y in AX"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.source_y_offset = 10U;
        actor.position_y = 70U;
        actor.target_phase_y_adjustment = 20;
        openswd3::compat::u16 output_x = 0U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {
                    .actor_token = 0x11112222U,
                    .output_y_token = 0x55556666U,
                    .entry_eax = 0xAAAA0000U,
                    .output_y_pointer_readable = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        output_y_pointer_read_typed_stop &&
                output_x == 20U && output_y == 0x8888U &&
                result.output_y == 50U && result.return_eax == 0xAAAA0032U &&
                result.return_ecx == 0x11112222U &&
                result.output_pointer_reads == 1U && result.output_writes == 1U,
            "base coordinate Y stack-pointer fault retains final SUB state"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor;
        actor.position_x = 30U;
        actor.source_y_offset = 10U;
        actor.position_y = 70U;
        actor.target_phase_y_adjustment = 20;
        openswd3::compat::u16 output_x = 0U;
        openswd3::compat::u16 output_y = 0x8888U;
        const auto result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::view_legacy_battle_actor_coordinates(actor),
                &output_x,
                &output_y,
                {
                    .actor_token = 0x11112222U,
                    .output_y_token = 0x55556666U,
                    .output_y_writable = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        output_y_write_typed_stop &&
                output_x == 20U && output_y == 0x8888U &&
                result.return_ecx == 0x55556666U &&
                result.output_pointer_reads == 2U && result.output_writes == 1U,
            "base coordinate Y write fault follows the Y pointer load"
        );
    }

    {
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->party[2].position_x = 100U;
        startup->party[2].source_y_offset = 7U;
        startup->party[2].position_y = 200U;
        startup->party[2].target_phase_y_adjustment = 11;
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        auto& group_b = (*startup->group_b_lifecycle)[3].action_execution;
        group_b.position_x = 50U;
        group_b.source_y_offset = 5U;
        group_b.position_y = 60U;
        group_b.target_phase_y_adjustment = 6;
        openswd3::compat::u16 group_a_x{};
        openswd3::compat::u16 group_a_y{};
        openswd3::compat::u16 group_b_x{};
        openswd3::compat::u16 group_b_y{};
        const auto group_a_result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::resolve_legacy_battle_actor_coordinates(
                    {.startup = startup.get()},
                    openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupABaseToken +
                        2U *
                            openswd3::battle::
                                kLegacyBattleActorCoordinatesGroupAStride
                ),
                &group_a_x,
                &group_a_y
            );
        const auto group_b_result =
            openswd3::battle::query_legacy_battle_actor_base_coordinates(
                openswd3::battle::resolve_legacy_battle_actor_coordinates(
                    {.startup = startup.get()},
                    openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBBaseToken +
                        3U *
                            openswd3::battle::
                                kLegacyBattleActorCoordinatesGroupBStride
                ),
                &group_b_x,
                &group_b_y
            );
        test.expect_true(
            group_a_result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                group_b_result.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                group_a_x == 93U && group_a_y == 189U && group_b_x == 45U &&
                group_b_y == 54U,
            "base coordinates resolve canonical startup group-A and group-B owners"
        );
    }
}
