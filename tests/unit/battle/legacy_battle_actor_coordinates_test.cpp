#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "test.hpp"

void test_battle_actor_coordinates(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateBindings;
    using openswd3::battle::LegacyBattleActorCoordinateQueryStatus;
    using openswd3::battle::LegacyBattleActorCoordinatesState;
    using openswd3::battle::query_legacy_battle_actor_coordinates;
    using openswd3::battle::resolve_legacy_battle_actor_coordinates;
    using openswd3::battle::view_legacy_battle_actor_coordinates;

    {
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0xFEDCU,
            .position_y = 0x8123U,
        };
        openswd3::compat::u16 x{};
        openswd3::compat::u16 y{};
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
                x == 0xFEDCU && y == 0x8123U && result.output_x == 0xFEDCU &&
                result.output_y == 0x8123U &&
                result.return_eax == 0xAABBCCDDU &&
                result.return_ecx == 0x00508123U &&
                result.return_edx == 0x11223344U && result.gate_reads == 1U &&
                result.coordinate_reads == 2U && result.output_writes == 2U &&
                !result.alternate_coordinates && result.zero_flag,
            "actor coordinate query preserves the primary-branch word values and terminal registers"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .alternate_position_x = 0x1234U,
            .alternate_position_y = 0xFEDCU,
            .coordinate_mode_gate = 1U,
        };
        openswd3::compat::u16 x{};
        openswd3::compat::u16 y{};
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
                result.return_edx == 0xAABBCCDDU && result.gate_reads == 1U &&
                result.coordinate_reads == 2U && result.output_writes == 2U &&
                result.alternate_coordinates && !result.zero_flag,
            "actor coordinate query preserves the alternate-branch AX replacements and pointer residues"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0x1111U,
            .position_y = 0x2222U,
            .position_y_read_accessible = false,
        };
        openswd3::compat::u16 x{0xAAAAU};
        openswd3::compat::u16 y{0xBBBBU};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            &y,
            {
                .actor_token = 0x005029D0U,
                .output_x_token = 0x10001000U,
                .output_y_token = 0x20002000U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0xCAFE5678U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                x == 0x1111U && y == 0xBBBBU &&
                result.return_eax == 0x10001000U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x20002000U &&
                result.coordinate_reads == 1U && result.output_writes == 1U,
            "primary second-read stop retains the first committed word and the loaded second pointer"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .alternate_position_x = 0x1111U,
            .alternate_position_y = 0x2222U,
            .coordinate_mode_gate = 1U,
        };
        openswd3::compat::u16 x{0xAAAAU};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            nullptr,
            {
                .actor_token = 0x00525508U,
                .output_x_token = 0x10001000U,
                .output_y_token = 0x20002000U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0xCAFE5678U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        second_output_write_typed_stop &&
                x == 0x1111U && result.return_eax == 0xDEAD2222U &&
                result.return_ecx == 0x20002000U &&
                result.return_edx == 0x10001000U &&
                result.coordinate_reads == 2U && result.output_writes == 1U,
            "alternate second-write stop retains the first commit and complete narrow-register prefix"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .coordinate_mode_gate_read_accessible = false,
        };
        openswd3::compat::u16 x{0xAAAAU};
        openswd3::compat::u16 y{0xBBBBU};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            &x,
            &y,
            {
                .actor_token = 0x005029D0U,
                .output_x_token = 0x10001000U,
                .output_y_token = 0x20002000U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0xCAFE5678U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                x == 0xAAAAU && y == 0xBBBBU &&
                result.return_eax == 0xDEADBEEFU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xCAFE5678U && result.gate_reads == 0U &&
                result.coordinate_reads == 0U && result.output_writes == 0U,
            "actor coordinate query stops at the gate read with entry registers and outputs untouched"
        );
    }

    {
        using Status = LegacyBattleActorCoordinateQueryStatus;
        using openswd3::compat::u16;
        using openswd3::compat::u32;
        struct FaultVector {
            Status status;
            u32 eax;
            u32 ecx;
            u32 edx;
            u32 reads;
            u32 writes;
        };
        // Derived from each reached MOV, before the failing memory access.
        constexpr std::array<std::array<FaultVector, 4>, 2> vectors{{
            {{
                {Status::primary_x_read_typed_stop,
                 0x10001000U,
                 0x005029D0U,
                 0x98766543U,
                 0U,
                 0U},
                {Status::first_output_write_typed_stop,
                 0x10001000U,
                 0x005029D0U,
                 0x9876A135U,
                 1U,
                 0U},
                {Status::primary_y_read_typed_stop,
                 0x10001000U,
                 0x005029D0U,
                 0x20002000U,
                 1U,
                 1U},
                {Status::second_output_write_typed_stop,
                 0x10001000U,
                 0x00508000U,
                 0x20002000U,
                 2U,
                 1U},
            }},
            {{
                {Status::alternate_x_read_typed_stop,
                 0xCAFE1234U,
                 0x005029D0U,
                 0x10001000U,
                 0U,
                 0U},
                {Status::first_output_write_typed_stop,
                 0xCAFECE57U,
                 0x005029D0U,
                 0x10001000U,
                 1U,
                 0U},
                {Status::alternate_y_read_typed_stop,
                 0xCAFECE57U,
                 0x005029D0U,
                 0x10001000U,
                 1U,
                 1U},
                {Status::second_output_write_typed_stop,
                 0xCAFEFFFFU,
                 0x20002000U,
                 0x10001000U,
                 2U,
                 1U},
            }},
        }};
        for (std::size_t branch = 0U; branch < vectors.size(); ++branch) {
            for (std::size_t fault = 0U; fault < vectors[branch].size();
                 ++fault) {
                LegacyBattleActorCoordinatesState actor{
                    .position_x = 0xA135U,
                    .position_y = 0x8000U,
                    .alternate_position_x = 0xCE57U,
                    .alternate_position_y = 0xFFFFU,
                    .coordinate_mode_gate = static_cast<u16>(branch),
                };
                auto view = view_legacy_battle_actor_coordinates(actor);
                if (fault == 0U) {
                    actor.position_x_read_accessible = false;
                    actor.alternate_position_x_read_accessible = false;
                }

                if (fault == 2U) {
                    actor.position_y_read_accessible = false;
                    actor.alternate_position_y_read_accessible = false;
                }

                u16 x = 0x1111U;
                u16 y = 0x2222U;
                const auto result = query_legacy_battle_actor_coordinates(
                    view,
                    fault == 1U ? nullptr : &x,
                    fault == 3U ? nullptr : &y,
                    {
                        .actor_token = 0x005029D0U,
                        .output_x_token = 0x10001000U,
                        .output_y_token = 0x20002000U,
                        .entry_eax = 0xCAFE1234U,
                        .entry_edx = 0x98766543U,
                    }
                );
                const auto& expected = vectors[branch][fault];
                const u16 first = branch == 0U ? 0xA135U : 0xCE57U;
                test.expect_true(
                    result.status == expected.status &&
                        result.return_eax == expected.eax &&
                        result.return_ecx == expected.ecx &&
                        result.return_edx == expected.edx &&
                        result.coordinate_reads == expected.reads &&
                        result.output_writes == expected.writes &&
                        x == (expected.writes == 0U ? 0x1111U : first) &&
                        y == 0x2222U,
                    "each branch stops at the exact failing access with its full register and write prefix"
                );
            }
        }
    }

    {
        using openswd3::compat::u16;
        for (const u16 gate :
             std::array<u16, 5>{0U, 1U, 0x100U, 0x8000U, 0xFFFFU}) {
            LegacyBattleActorCoordinatesState actor{
                .position_x = 0x1234U,
                .position_y = 0x5678U,
                .alternate_position_x = 0x9ABCU,
                .alternate_position_y = 0xDEF0U,
                .coordinate_mode_gate = gate,
            };
            u16* const selected_y =
                gate == 0U ? &actor.position_y : &actor.alternate_position_y;
            u16 y{};
            const auto result = query_legacy_battle_actor_coordinates(
                view_legacy_battle_actor_coordinates(actor), selected_y, &y, {}
            );
            const u16 expected = gate == 0U ? 0x1234U : 0x9ABCU;
            test.expect_true(
                result.status ==
                        LegacyBattleActorCoordinateQueryStatus::completed &&
                    result.output_y == expected && y == expected &&
                    *selected_y == expected,
                "the full gate word selects a branch and the first aliased write precedes the second source read"
            );
        }
    }

    {
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0x1357U,
            .position_y = 0x2468U,
        };
        openswd3::compat::u16 output{};
        const auto result = query_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), &output, &output, {}
        );
        test.expect_true(
            result.output_writes == 2U && output == 0x2468U,
            "identical output pointers receive X then Y without deduplication"
        );
    }

    {
        LegacyBattleActorCoordinatesState group_a{};
        LegacyBattleActorCoordinatesState group_b{};
        LegacyBattleActorCoordinateBindings bindings{};
        bindings.group_a[3U] = view_legacy_battle_actor_coordinates(group_a);
        bindings.group_b[2U] = view_legacy_battle_actor_coordinates(group_b);
        test.expect_true(
            resolve_legacy_battle_actor_coordinates(
                bindings, 0x005029D0U + 3U * 0x2F34U
            )
                        .position_x == &group_a.position_x &&
                resolve_legacy_battle_actor_coordinates(
                    bindings, 0x00525508U + 2U * 0x2B28U
                )
                        .position_x == &group_b.position_x &&
                resolve_legacy_battle_actor_coordinates(bindings, 0x12345678U)
                        .coordinate_mode_gate == nullptr,
            "actor coordinate bindings resolve only exact physical group strides"
        );
    }
}
