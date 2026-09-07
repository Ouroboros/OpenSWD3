#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_render_offsets.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_render_offsets(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateOwners;
    using openswd3::battle::LegacyBattleActorRenderOffsetQueryRequest;
    using openswd3::battle::LegacyBattleActorRenderOffsetQueryStatus;
    using openswd3::battle::LegacyBattleActorRenderOffsetState;
    using openswd3::battle::query_legacy_battle_actor_render_offsets;
    using openswd3::battle::resolve_legacy_battle_actor_render_offsets;
    using openswd3::battle::view_legacy_battle_actor_render_offsets;
    using openswd3::compat::u16;
    using openswd3::compat::u32;

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 0x1234U,
            .render_y_base = 0xFEDCU,
            .mirror_mode = 2U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &x,
            &y,
            {
                .actor_token = 0x005029D0U,
                .output_x_token = 0xAABBCCDDU,
                .output_y_token = 0x11223344U,
                .entry_eax = 0x55667788U,
                .entry_edx = 0x99AA0000U,
                .entry_esi = 0xCAFE7777U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRenderOffsetQueryStatus::completed &&
                x == 0x1234U && y == 0xFEDCU &&
                result.return_eax == 0xAABBCCDDU &&
                result.return_ecx == 0x005029D0U && result.return_edx == 2U &&
                result.return_esi == 0xCAFE7777U &&
                result.output_pointer_reads == 2U && result.actor_reads == 4U &&
                result.output_writes == 2U && !result.override_coordinates &&
                !result.mirrored_x && !result.flags.carry &&
                !result.flags.parity && !result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && !result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "base offsets preserve output order, callee-saved ESI, and mirror CMP flags"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 1U,
            .render_y_base = 2U,
            .override_mode_flags = 0x82U,
            .override_x = 0xABCDU,
            .override_y = 0xFEDCU,
            .mirror_mode = 0U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &x,
            &y,
            {.entry_esi = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRenderOffsetQueryStatus::completed &&
                x == 0xABCDU && y == 0xFEDCU &&
                result.override_mode_flags == 0x82U &&
                result.override_coordinates && !result.mirrored_x &&
                result.actor_reads == 6U && result.output_writes == 4U &&
                result.return_esi == 0x12345678U && result.flags.carry &&
                result.flags.parity && result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && !result.flags.zero &&
                result.flags.sign && !result.flags.overflow,
            "override bit publishes the special pair before the non-mirror return"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 5U,
            .render_y_base = 8U,
            .override_mode_flags = 2U,
            .override_x = 20U,
            .override_y = 30U,
            .mirror_mode = 1U,
            .render_source_token = 0xABCD1000U,
            .render_source_width = 100U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &x,
            &y,
            {
                .output_x_token = 0x11112222U,
                .entry_esi = 0x33334444U,
            }
        );
        test.expect_true(
            x == 95U && y == 30U && result.output_x == 95U &&
                result.output_y == 30U && result.mirrored_x &&
                result.actor_reads == 9U && result.output_writes == 5U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xABCD005FU && result.return_edx == 5U &&
                result.return_esi == 0x33334444U && !result.flags.carry &&
                result.flags.parity && result.flags.auxiliary_carry &&
                result.flags.auxiliary_carry_defined && !result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "mirror branch subtracts the base X rather than the override X"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 0U,
            .render_y_base = 7U,
            .override_mode_flags = 2U,
            .override_x = 9U,
            .override_y = 11U,
            .mirror_mode = 1U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor), &x, &y
        );
        test.expect_true(
            x == 9U && y == 11U && !result.mirrored_x &&
                result.output_writes == 4U && result.flags.zero &&
                result.flags.parity && !result.flags.auxiliary_carry_defined,
            "zero base X keeps the override pair and returns with TEST flags"
        );
    }

    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        auto& actor = action.group_a_action_execution[0];
        actor.render_x_base = 1U;
        actor.render_y_base = 2U;
        actor.action_override_flags = 0x0200U;
        actor.special_action_record.field_76 = 7U;
        actor.special_action_record.field_78 = 8U;
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            resolve_legacy_battle_actor_render_offsets(
                {.action = &action},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            ),
            &x,
            &y
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRenderOffsetQueryStatus::completed &&
                result.override_mode_flags == 2U &&
                result.override_coordinates && x == 7U && y == 8U,
            "Group-A action owner reads override bit from actor offset 2A87"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 5U,
            .render_y_base = 8U,
            .mirror_mode = 1U,
            .render_source_token = 0U,
            .render_source_width = 100U,
        };
        u16 x{};
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &x,
            &y,
            {.entry_esi = 0xABCD1234U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRenderOffsetQueryStatus::
                        render_source_width_read_typed_stop &&
                x == 5U && y == 8U && result.output_writes == 2U &&
                result.actor_reads == 6U && result.return_ecx == 0U &&
                result.return_edx == 5U && result.return_esi == 0xABCD1234U &&
                !result.flags.carry && result.flags.parity &&
                !result.flags.auxiliary_carry_defined && !result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "null render source stops at width dereference after restoring ESI"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 1U,
            .render_y_base = 2U,
            .override_mode_flags = 2U,
            .override_x = 7U,
            .override_y = 8U,
            .mirror_mode = 2U,
        };
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &actor.override_y,
            &y
        );
        test.expect_true(
            result.output_writes == 4U && actor.override_y == 7U && y == 7U,
            "override-X output alias changes the later override-Y source read"
        );
    }

    {
        using Status = LegacyBattleActorRenderOffsetQueryStatus;
        struct FaultCase {
            Status status;
            u32 actor_reads;
            u32 pointer_reads;
            u32 output_writes;
        };
        constexpr std::array cases{
            FaultCase{Status::first_output_pointer_read_typed_stop, 0U, 0U, 0U},
            FaultCase{Status::render_x_base_read_typed_stop, 0U, 1U, 0U},
            FaultCase{Status::initial_x_write_typed_stop, 1U, 1U, 0U},
            FaultCase{
                Status::second_output_pointer_read_typed_stop, 1U, 1U, 1U
            },
            FaultCase{Status::render_y_base_read_typed_stop, 1U, 2U, 1U},
            FaultCase{Status::initial_y_write_typed_stop, 2U, 2U, 1U},
            FaultCase{Status::override_mode_flags_read_typed_stop, 2U, 2U, 2U},
            FaultCase{Status::override_x_read_typed_stop, 3U, 2U, 2U},
            FaultCase{Status::override_x_write_typed_stop, 4U, 2U, 2U},
            FaultCase{Status::override_y_read_typed_stop, 4U, 2U, 3U},
            FaultCase{Status::override_y_write_typed_stop, 5U, 2U, 3U},
            FaultCase{Status::mirror_mode_read_typed_stop, 5U, 2U, 4U},
            FaultCase{Status::mirror_render_x_base_read_typed_stop, 6U, 2U, 4U},
            FaultCase{Status::render_source_token_read_typed_stop, 7U, 2U, 4U},
            FaultCase{Status::render_source_width_read_typed_stop, 8U, 2U, 4U},
            FaultCase{Status::mirror_x_write_typed_stop, 9U, 2U, 4U},
        };

        for (const auto& fault : cases) {
            LegacyBattleActorRenderOffsetState actor{
                .render_x_base = 5U,
                .render_y_base = 8U,
                .override_mode_flags = 2U,
                .override_x = 20U,
                .override_y = 30U,
                .mirror_mode = 1U,
                .render_source_token = 0x1000U,
                .render_source_width = 100U,
            };
            LegacyBattleActorRenderOffsetQueryRequest request{
                .actor_token = 0x005029D0U,
                .output_x_token = 0x11112222U,
                .output_y_token = 0x33334444U,
                .entry_eax = 0x55556666U,
                .entry_edx = 0x77778888U,
                .entry_esi = 0x9999AAAAU,
            };
            switch (fault.status) {
            case Status::first_output_pointer_read_typed_stop:
                request.first_output_pointer_readable = false;
                break;

            case Status::render_x_base_read_typed_stop:
                actor.render_x_base_read_accessible = false;
                break;

            case Status::initial_x_write_typed_stop:
                request.initial_x_writable = false;
                break;

            case Status::second_output_pointer_read_typed_stop:
                request.second_output_pointer_readable = false;
                break;

            case Status::render_y_base_read_typed_stop:
                actor.render_y_base_read_accessible = false;
                break;

            case Status::initial_y_write_typed_stop:
                request.initial_y_writable = false;
                break;

            case Status::override_mode_flags_read_typed_stop:
                actor.override_mode_flags_read_accessible = false;
                break;

            case Status::override_x_read_typed_stop:
                actor.override_x_read_accessible = false;
                break;

            case Status::override_x_write_typed_stop:
                request.override_x_writable = false;
                break;

            case Status::override_y_read_typed_stop:
                actor.override_y_read_accessible = false;
                break;

            case Status::override_y_write_typed_stop:
                request.override_y_writable = false;
                break;

            case Status::mirror_mode_read_typed_stop:
                actor.mirror_mode_read_accessible = false;
                break;

            case Status::mirror_render_x_base_read_typed_stop:
                request.mirror_render_x_base_readable = false;
                break;

            case Status::render_source_token_read_typed_stop:
                actor.render_source_token_read_accessible = false;
                break;

            case Status::render_source_width_read_typed_stop:
                actor.render_source_width_read_accessible = false;
                break;

            case Status::mirror_x_write_typed_stop:
                request.mirror_x_writable = false;
                break;

            case Status::completed:
                break;
            }

            u16 x = 0xAAAAU;
            u16 y = 0xBBBBU;
            const auto result = query_legacy_battle_actor_render_offsets(
                view_legacy_battle_actor_render_offsets(actor), &x, &y, request
            );
            test.expect_true(
                result.status == fault.status &&
                    result.actor_reads == fault.actor_reads &&
                    result.output_pointer_reads == fault.pointer_reads &&
                    result.output_writes == fault.output_writes,
                "each render-offset access stops after the exact write prefix"
            );
        }
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 0x1357U,
            .render_y_base = 0x2468U,
            .mirror_mode = 2U,
        };
        u16 y{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor),
            &actor.render_y_base,
            &y
        );
        test.expect_true(
            result.output_writes == 2U && actor.render_y_base == 0x1357U &&
                y == 0x1357U,
            "first output alias changes the later base-Y source read"
        );
    }

    {
        LegacyBattleActorRenderOffsetState actor{
            .render_x_base = 3U,
            .render_y_base = 4U,
            .override_mode_flags = 2U,
            .override_x = 5U,
            .override_y = 6U,
            .mirror_mode = 2U,
        };
        u16 output{};
        const auto result = query_legacy_battle_actor_render_offsets(
            view_legacy_battle_actor_render_offsets(actor), &output, &output
        );
        test.expect_true(
            result.output_writes == 4U && output == 6U,
            "identical output pointers retain all four ordered word stores"
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
        const auto group_a = resolve_legacy_battle_actor_render_offsets(
            owners, 0x005029D0U + 3U * 0x2F34U
        );
        const auto group_a_fallback =
            resolve_legacy_battle_actor_render_offsets(
                {.action = &action}, 0x005029D0U + 3U * 0x2F34U
            );
        const auto group_b = resolve_legacy_battle_actor_render_offsets(
            owners, 0x00525508U + 2U * 0x2B28U
        );
        test.expect_true(
            group_a.render_x_base ==
                    &startup.party[3U].render_offsets.render_x_base &&
                group_a_fallback.render_x_base ==
                    &action.group_a_action_execution[3U].render_x_base &&
                group_b.override_mode_flags ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_composition.mode_flags &&
                group_b.override_x ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_execution.special_action_record.field_76 &&
                resolve_legacy_battle_actor_render_offsets(owners, 0x12345678U)
                        .render_x_base == nullptr,
            "render-offset lookup keeps startup, action fallback, and Group-B owners distinct"
        );
    }
}
