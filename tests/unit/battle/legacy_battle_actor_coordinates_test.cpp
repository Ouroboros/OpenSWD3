#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_coordinates(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateOwners;
    using openswd3::battle::LegacyBattleActorCoordinateQueryStatus;
    using openswd3::battle::LegacyBattleActorCoordinatesState;
    using openswd3::battle::query_legacy_battle_actor_coordinates;
    using openswd3::battle::resolve_legacy_battle_actor_coordinates;
    using openswd3::battle::view_legacy_battle_actor_coordinates;
    using openswd3::compat::u16;
    using openswd3::compat::u32;

    {
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0xFEDCU,
            .position_y = 0x8123U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            &y,
            {
                .actor_token = 0x005029D0U,
                .output_x_token = 0xAABBCCDDU,
                .output_y_token = 0x11223344U,
                .entry_eax = 0x55667788U,
                .entry_edx = 0x99AABBCCU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                x == 0xFEDCU && y == 0x8123U &&
                result.return_eax == 0xAABBCCDDU &&
                result.return_ecx == 0x00508123U &&
                result.return_edx == 0x11223344U && result.gate_reads == 1U &&
                result.coordinate_reads == 2U && result.output_writes == 2U &&
                !result.alternate_coordinates && !result.flags.carry &&
                result.flags.parity && !result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "primary coordinate branch preserves words, registers, and CMP flags"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .alternate_position_x = 0x1234U,
            .alternate_position_y = 0xFEDCU,
            .coordinate_mode_gate = 0x8001U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            &y,
            {
                .actor_token = 0x00525508U,
                .output_x_token = 0xAABBCCDDU,
                .output_y_token = 0x11223344U,
                .entry_eax = 0x55667788U,
                .entry_edx = 0x99AABBCCU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                x == 0x1234U && y == 0xFEDCU &&
                result.return_eax == 0x5566FEDCU &&
                result.return_ecx == 0x11223344U &&
                result.return_edx == 0xAABBCCDDU &&
                result.alternate_coordinates && !result.flags.zero &&
                result.flags.sign && !result.flags.parity &&
                result.flags.auxiliary_carry_defined,
            "alternate coordinate branch preserves AX replacement and pointer residues"
        );
    }

    {
        using Status = LegacyBattleActorCoordinateQueryStatus;
        struct FaultCase {
            bool alternate;
            Status expected;
        };
        constexpr std::array cases{
            FaultCase{false, Status::first_output_pointer_read_typed_stop},
            FaultCase{false, Status::primary_x_read_typed_stop},
            FaultCase{false, Status::first_output_write_typed_stop},
            FaultCase{false, Status::second_output_pointer_read_typed_stop},
            FaultCase{false, Status::primary_y_read_typed_stop},
            FaultCase{false, Status::second_output_write_typed_stop},
            FaultCase{true, Status::first_output_pointer_read_typed_stop},
            FaultCase{true, Status::alternate_x_read_typed_stop},
            FaultCase{true, Status::first_output_write_typed_stop},
            FaultCase{true, Status::alternate_y_read_typed_stop},
            FaultCase{true, Status::second_output_pointer_read_typed_stop},
            FaultCase{true, Status::second_output_write_typed_stop},
        };
        for (const auto& fault : cases) {
            LegacyBattleActorCoordinatesState actor{
                .position_x = 0x1111U,
                .position_y = 0x2222U,
                .alternate_position_x = 0x3333U,
                .alternate_position_y = 0x4444U,
                .coordinate_mode_gate = static_cast<u16>(fault.alternate),
            };
            auto request =
                openswd3::battle::LegacyBattleActorCoordinateQueryRequest{
                    .actor_token = 0x005029D0U,
                    .output_x_token = 0x10001000U,
                    .output_y_token = 0x20002000U,
                    .entry_eax = 0xCAFE1234U,
                    .entry_edx = 0x98766543U,
                };
            u16 x = 0xAAAAU;
            u16 y = 0xBBBBU;
            u16* output_x = &x;
            u16* output_y = &y;
            u32 expected_eax = request.entry_eax;
            u32 expected_ecx = request.actor_token;
            u32 expected_edx = request.entry_edx;
            switch (fault.expected) {
            case Status::first_output_pointer_read_typed_stop:
                request.first_output_pointer_readable = false;
                break;
            case Status::primary_x_read_typed_stop:
                actor.position_x_read_accessible = false;
                expected_eax = request.output_x_token;
                break;
            case Status::alternate_x_read_typed_stop:
                actor.alternate_position_x_read_accessible = false;
                expected_edx = request.output_x_token;
                break;
            case Status::first_output_write_typed_stop:
                request.first_output_writable = false;
                if (fault.alternate) {
                    expected_eax = 0xCAFE3333U;
                    expected_edx = request.output_x_token;
                } else {
                    expected_eax = request.output_x_token;
                    expected_edx = 0x98761111U;
                }
                break;
            case Status::second_output_pointer_read_typed_stop:
                request.second_output_pointer_readable = false;
                if (fault.alternate) {
                    expected_eax = 0xCAFE4444U;
                    expected_edx = request.output_x_token;
                } else {
                    expected_eax = request.output_x_token;
                    expected_edx = 0x98761111U;
                }
                break;
            case Status::primary_y_read_typed_stop:
                actor.position_y_read_accessible = false;
                expected_eax = request.output_x_token;
                expected_edx = request.output_y_token;
                break;
            case Status::alternate_y_read_typed_stop:
                actor.alternate_position_y_read_accessible = false;
                expected_eax = 0xCAFE3333U;
                expected_edx = request.output_x_token;
                break;
            case Status::second_output_write_typed_stop:
                request.second_output_writable = false;
                if (fault.alternate) {
                    expected_eax = 0xCAFE4444U;
                    expected_ecx = request.output_y_token;
                    expected_edx = request.output_x_token;
                } else {
                    expected_eax = request.output_x_token;
                    expected_ecx = 0x00502222U;
                    expected_edx = request.output_y_token;
                }
                break;
            case Status::completed:
            case Status::actor_gate_read_typed_stop:
                break;
            }
            const auto result = query_legacy_battle_actor_coordinates(
                view_legacy_battle_actor_coordinates(actor),
                output_x,
                output_y,
                request
            );
            const bool first_write_reached = fault.expected ==
                    Status::second_output_pointer_read_typed_stop ||
                fault.expected == Status::primary_y_read_typed_stop ||
                fault.expected == Status::alternate_y_read_typed_stop ||
                fault.expected == Status::second_output_write_typed_stop;
            test.expect_true(
                result.status == fault.expected &&
                    result.return_eax == expected_eax &&
                    result.return_ecx == expected_ecx &&
                    result.return_edx == expected_edx &&
                    result.gate_reads == 1U &&
                    result.flags.zero == !fault.alternate &&
                    result.flags.auxiliary_carry_defined &&
                    result.output_writes == (first_write_reached ? 1U : 0U) &&
                    x ==
                        (first_write_reached
                             ? (fault.alternate ? 0x3333U : 0x1111U)
                             : 0xAAAAU) &&
                    y == 0xBBBBU,
                "every reached access stops with the exact first-write prefix"
            );
        }
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .coordinate_mode_gate_read_accessible = false,
        };
        u16 x = 0xAAAAU;
        u16 y = 0xBBBBU;
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            &y,
            {
                .actor_token = 0x005029D0U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0xCAFE5678U,
                .entry_flags = {
                    .carry = true,
                    .parity = false,
                    .auxiliary_carry = true,
                    .auxiliary_carry_defined = false,
                    .zero = false,
                    .sign = true,
                    .overflow = true,
                },
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.return_eax == 0xDEADBEEFU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xCAFE5678U && x == 0xAAAAU &&
                y == 0xBBBBU && result.gate_reads == 0U && result.flags.carry &&
                !result.flags.parity && result.flags.auxiliary_carry &&
                !result.flags.auxiliary_carry_defined && !result.flags.zero &&
                result.flags.sign && result.flags.overflow,
            "selector read failure keeps entry registers, flags, and outputs untouched"
        );
    }

    {
        for (const u16 selector :
             std::array<u16, 5>{0U, 1U, 0x100U, 0x8000U, 0xFFFFU}) {
            LegacyBattleActorCoordinatesState actor{
                .position_x = 0x1234U,
                .position_y = 0x5678U,
                .alternate_position_x = 0x9ABCU,
                .alternate_position_y = 0xDEF0U,
                .coordinate_mode_gate = selector,
            };
            u16* selected_y = selector == 0U ? &actor.position_y
                                             : &actor.alternate_position_y;
            u16 output_y{};
            const auto result = query_legacy_battle_actor_coordinates(
                view_legacy_battle_actor_coordinates(actor),
                selected_y,
                &output_y
            );
            const u16 expected = selector == 0U ? 0x1234U : 0x9ABCU;
            test.expect_true(
                result.status ==
                        LegacyBattleActorCoordinateQueryStatus::completed &&
                    *selected_y == expected && output_y == expected,
                "first aliased store precedes the selected second source read"
            );
        }
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0x1357U,
            .position_y = 0x2468U,
        };
        u16 output{};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), &output, &output
        );
        test.expect_true(
            result.output_writes == 2U && output == 0x2468U,
            "identical output pointers receive X then Y without deduplication"
        );
    }

    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const LegacyBattleActorCoordinateOwners owners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a = resolve_legacy_battle_actor_coordinates(
            owners, 0x005029D0U + 3U * 0x2F34U
        );
        const auto group_a_fallback = resolve_legacy_battle_actor_coordinates(
            {.action = &action}, 0x005029D0U + 3U * 0x2F34U
        );
        const auto group_b = resolve_legacy_battle_actor_coordinates(
            owners, 0x00525508U + 2U * 0x2B28U
        );
        test.expect_true(
            group_a.position_x == &startup.party[3U].position_x &&
                group_a_fallback.position_x ==
                    &action.group_a_action_execution[3U].position_x &&
                group_b.position_x ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_execution.position_x &&
                resolve_legacy_battle_actor_coordinates(owners, 0x12345678U)
                        .coordinate_mode_gate == nullptr,
            "coordinate lookup prefers canonical startup actors and preserves exact fallbacks"
        );
    }
}
