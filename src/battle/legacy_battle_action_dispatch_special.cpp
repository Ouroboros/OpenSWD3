#include "legacy_battle_action_dispatch_internal.hpp"

namespace openswd3::battle {
using namespace action_dispatch_detail;

LegacyBattleDualRecordActionResult advance_legacy_battle_dual_record_action(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleDualRecordActionRequest& request
) {
    LegacyBattleDualRecordActionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleDualRecordActionStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& primary = actor->primary_action_record;
    primary.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    primary.base_variant = 0x2DU;
    ++result.action_update_calls;
    const auto primary_updated = context.action_updater.update(primary);
    if (primary_updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            primary.field_4a, primary.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleDualRecordActionStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleDualRecordActionStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleDualRecordActionStatus::phase_state_typed_stop;
        return result;
    }

    actor->source_x_offset = primary.field_76;
    actor->turn_render_flags = primary.mode_flags;
    actor->turn_target_x_offset = static_cast<u16>(primary.draw_offset_x);
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            static_cast<u16>(frame.width) -
            static_cast<u16>(primary.draw_offset_x)
        );
        if (primary.field_76 != 0U) {
            actor->source_x_offset = static_cast<u16>(
                static_cast<u16>(frame.width) - primary.field_76
            );
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = motion;
    shared->draw_motion_b = motion;
    shared->draw_motion_c = motion;

    registers.edx = actor->turn_frame_token;
    replace_low_word(registers.edx, primary.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.edx, 0x004AB784U})
    );

    const u32 draw_x = signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    registers.eax = draw_x;
    ++result.sample_pan_calls;
    if (std::bit_cast<i32>(draw_x) >= 0x140) {
        replace_low_word(registers.edx, primary.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.edx, 0x10U})
        );
    } else {
        replace_low_word(registers.ecx, primary.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.ecx, 0xFFFFFFF0U})
        );
    }

    const u32 modified_flags = (actor->turn_render_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    primary.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - primary.draw_offset_y,
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if ((primary.field_5a & 9U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    auto& secondary = actor->effect_action_record;
    secondary.action_id = request.secondary_action_id;
    secondary.base_variant = 0U;
    ++result.action_update_calls;
    const auto secondary_updated = context.action_updater.update(secondary);
    if (secondary_updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece secondary_frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            secondary.field_4a, secondary.field_4c, secondary_frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleDualRecordActionStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    shared->turn_frame_source_token = actor->turn_frame_token;

    actor->source_x_offset = secondary.field_76;
    actor->turn_render_flags = secondary.mode_flags;
    actor->turn_target_x_offset = static_cast<u16>(secondary.draw_offset_x);
    const u32 coordinate_gate = phase->render_toggle_gate;
    const u16 coordinate_source_x = secondary.field_76;
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            static_cast<u16>(secondary_frame.width) -
            static_cast<u16>(secondary.draw_offset_x)
        );
        if (secondary.field_76 != 0U) {
            actor->source_x_offset = static_cast<u16>(
                static_cast<u16>(secondary_frame.width) - secondary.field_76
            );
        }
    }

    LegacyBattleActorCoordinateFlags coordinate_entry_flags =
        subtract_flags(coordinate_gate, 1U);
    u32 coordinate_entry_edx = registers.edx;
    if (coordinate_gate == 1U) {
        coordinate_entry_edx = actor->turn_frame_token;
        coordinate_entry_flags = logical_word_flags(coordinate_source_x);
        if (coordinate_source_x != 0U) {
            u32 minuend = actor->turn_frame_token;
            replace_low_word(minuend, secondary_frame.width);
            u32 subtrahend = request.coordinate_frame_header_residue;
            replace_low_word(subtrahend, coordinate_source_x);
            coordinate_entry_flags = subtract_flags(minuend, subtrahend);
        }
    }
    u32 coordinate_x = request.coordinate_output_x_initial;
    u32 coordinate_y{};
    ++result.coordinate_query_calls;
    result.coordinate_query = query_coordinates(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.coordinate_token,
        coordinate_x,
        coordinate_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        request.coordinate_output_y_token,
        coordinate_entry_edx,
        coordinate_entry_flags
    );
    registers.eax = result.coordinate_query.return_eax;
    registers.ecx = result.coordinate_query.return_ecx;
    registers.edx = result.coordinate_query.return_edx;
    if (result.coordinate_query.status !=
        LegacyBattleActorCoordinateQueryStatus::completed) {
        result.status =
            LegacyBattleDualRecordActionStatus::actor_coordinate_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    const u16 draw_x_word =
        static_cast<u16>(low_word(coordinate_x) - actor->turn_target_x_offset);
    const u16 draw_y_word = static_cast<u16>(
        low_word(coordinate_y) - static_cast<u16>(secondary.draw_offset_y)
    );
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            signed_word_bits(draw_x_word),
            signed_word_bits(draw_y_word),
            secondary_frame.width,
            secondary_frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if (secondary.field_8c != 1U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    secondary = {};
    primary = {};
    result.action_record_clears = 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSpecialFiveHundredResult advance_legacy_battle_special_five_hundred(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleSpecialFiveHundredRequest& request
) {
    LegacyBattleSpecialFiveHundredResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFiveHundredStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    const auto update_registers =
        [&](const LegacyBattleActionCallReply& reply) {
            registers.eax = reply.eax;
            registers.ecx = reply.ecx;
            registers.edx = reply.edx;
        };
    const auto signed_record_word = [](const u16 value) {
        return static_cast<i32>(std::bit_cast<i16>(value));
    };

    auto& special = actor->special_action_record;
    special.base_variant = actor->special_profile_variant;
    actor->turn_completion_latch = 1U;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    ++result.special_update_calls;
    ++result.port_calls;
    auto reply = port.invoke_special_action_update(
        {
            .callee_token = kCallSpecialActionUpdate,
            .arguments =
                {
                    request.source_token,
                    request.actor_token + 0x0AF0U,
                },
            .eax = special.action_id,
            .ecx = request.actor_token,
            .edx = request.source_token,
        },
        special
    );
    update_registers(reply);

    registers.edx = 0x4000U;
    u16 flags = special.field_5a;
    if ((flags & 2U) != 0U) {
        if (special.field_24 != 0U) {
            actor->action_runtime_gate |= 0x4000U;
            actor->turn_action_record.action_id = special.field_24;
            actor->turn_action_record.base_variant = special.field_28;
        }
        if ((flags & 0x0200U) != 0U) {
            special.external_mode = 1U;
        }
        special.field_24 = 0U;
        special.field_5a = static_cast<u16>(flags & 0xFFFDU);
        special.field_28 = 0U;
    }

    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        registers.edx = special.field_78;
        ++result.turn_frame_calls;
        ++result.port_calls;
        reply = port.invoke_special_turn_frame(
            {
                .callee_token = kCallSpecialTurnFrame,
                .arguments =
                    {
                        request.actor_token + 0x0468U,
                        static_cast<u32>(special.field_78),
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->turn_action_record
        );
        update_registers(reply);
        if (reply.eax == 1U) {
            actor->action_runtime_gate &= ~0x4000U;
            special.field_5a = 0U;
            special.external_mode = 0U;
            actor->turn_action_record = {};
            ++result.action_record_clears;
        }
    }

    flags = special.field_5a;
    if ((flags & 8U) != 0U) {
        if ((flags & 0x0400U) != 0U) {
            if (shared == nullptr) {
                result.status = LegacyBattleSpecialFiveHundredStatus::
                    shared_state_typed_stop;
                result.return_eax = registers.eax;
                result.return_ecx = registers.ecx;
                result.return_edx = registers.edx;
                return result;
            }
            port.battle_color_initialization_gate() = 1U;
            result.color_initialization =
                initialize_legacy_battle_color_accumulation(
                    port.battle_color_accumulation_state(),
                    {
                        .current_red = signed_record_word(special.field_7a),
                        .current_green = signed_record_word(special.field_7c),
                        .current_blue = signed_record_word(special.field_7e),
                        .target_red = signed_record_word(special.field_80),
                        .target_green = signed_record_word(special.field_82),
                        .target_blue = signed_record_word(special.field_84),
                        .countdown = signed_record_word(special.field_86),
                    }
                );
            ++result.color_initialization_calls;
            registers.eax = result.color_initialization.return_eax;
            registers.ecx = result.color_initialization.return_ecx;
            registers.edx = result.color_initialization.return_edx;
            special.field_5a = static_cast<u16>(special.field_5a & 0xFBFFU);
        }
        if (shared == nullptr) {
            result.status =
                LegacyBattleSpecialFiveHundredStatus::shared_state_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        if ((shared->action_completion_flags & 0x8000U) == 0U) {
            shared->action_completion_flags |= 0x8000U;
        }
    }

    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFiveHundredStatus::shared_state_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if ((shared->action_completion_flags & 1U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->action_runtime_gate = 0U;
    special = {};
    ++result.action_record_clears;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSpecialFourOhFiveResult advance_legacy_battle_special_four_oh_five(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourOhFiveRequest& request
) {
    LegacyBattleSpecialFourOhFiveResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& special = actor->special_action_record;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    special.base_variant = actor->special_profile_variant;
    actor->turn_completion_latch = 1U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(special);
    registers.eax = updated.return_value;
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            special.field_4a, special.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    actor->turn_target_x_offset = static_cast<u16>(special.draw_offset_x);
    actor->source_x_offset = special.field_76;
    u32 render_flags = special.mode_flags;
    if (actor->special_draw_mirror_mode == 1U) {
        if ((render_flags & 1U) != 0U) {
            render_flags &= ~1U;
        } else {
            render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width - static_cast<u16>(special.draw_offset_x)
        );
        if (special.field_76 != 0U) {
            actor->source_x_offset =
                static_cast<u16>(frame.width - special.field_76);
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = motion;
    shared->draw_motion_b = motion;
    shared->draw_motion_c = motion;

    registers.eax = 0x004AB784U;
    registers.ecx = actor->turn_frame_token;
    replace_low_word(registers.ecx, special.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.ecx, 0x004AB784U})
    );

    const u32 draw_x = signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    ++result.sample_pan_calls;
    if (std::bit_cast<i32>(draw_x) >= 0x140) {
        registers.edx = draw_x;
        replace_low_word(registers.edx, special.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.edx, 0x10U})
        );
    } else {
        replace_low_word(registers.ecx, special.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.ecx, 0xFFFFFFF0U})
        );
    }

    const u32 modified_flags = (render_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    special.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x - 5U,
            signed_word_bits(actor->position_y) - shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - special.draw_offset_y,
            frame.width,
            frame.height,
            render_flags,
            0U,
        }
    ));

    auto& control = port.frame_effect_control_state();
    control.red_factor = std::bit_cast<i16>(special.field_64);
    control.green_factor = std::bit_cast<i16>(special.field_66);
    control.blue_factor = std::bit_cast<i16>(special.field_68);
    result.frame_refresh = refresh_legacy_battle_frame(port);
    ++result.frame_refresh_calls;
    result.port_calls += result.frame_refresh.port_calls;
    if (result.frame_refresh.status !=
        LegacyBattleFrameRefreshStatus::completed) {
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::frame_refresh_typed_stop;
        return result;
    }

    if (control.red_factor != 0 || control.green_factor != 0 ||
        control.blue_factor != 0) {
        control.primary_suppression = 1U;
    }

    if ((static_cast<u8>(special.field_5a) & 9U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    u32 coordinate_x{};
    u32 coordinate_y{};
    ++result.coordinate_query_calls;
    result.coordinate_query = query_coordinates(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.target_token,
        coordinate_x,
        coordinate_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        request.coordinate_output_y_token,
        registers.edx,
        logical_byte_flags(static_cast<u8>(special.field_5a) & 9U)
    );
    registers.eax = result.coordinate_query.return_eax;
    registers.ecx = result.coordinate_query.return_ecx;
    registers.edx = result.coordinate_query.return_edx;
    if (result.coordinate_query.status !=
        LegacyBattleActorCoordinateQueryStatus::completed) {
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::actor_coordinate_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    const u32 effect_x = draw_x + signed_word_bits(actor->source_x_offset);
    const u32 effect_y = signed_word_bits(actor->position_y) +
        signed_word_bits(special.field_78) - special.draw_offset_y;
    if (phase == nullptr) {
        result.status =
            LegacyBattleSpecialFourOhFiveStatus::phase_state_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    phase->tick = static_cast<u16>(phase->tick + 1U);
    ++result.effect_update_calls;
    ++result.port_calls;
    auto reply = port.invoke_special_four_oh_five_update(
        {
            .callee_token = 0x0047F940U,
            .arguments =
                {
                    request.target_token,
                    request.actor_token + 0x0630U,
                    0U,
                    actor->copied_runtime_word,
                    effect_x,
                    effect_y,
                    signed_word_bits(actor->source_y),
                    1U,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        actor->effect_action_record
    );
    registers.eax = reply.eax;
    registers.ecx = reply.ecx;
    registers.edx = reply.edx;
    if (reply.eax != 1U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    registers.ecx = request.target_token;
    ++result.target_refresh_calls;
    if (!apply_actor_field_26b8_high_bit_set_call(
            context.startup,
            result.actor_field_26b8_high_bit_set,
            request.actor_field_26b8_high_bit_set_requests,
            request.target_token,
            0x004734A8U,
            registers
        )) {
        result.status = LegacyBattleSpecialFourOhFiveStatus::
            actor_field_26b8_high_bit_set_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    registers.ecx = request.actor_token;
    ++result.effect_compute_calls;
    const auto computed = invoke_action(
        kCallComputeValue, {request.target_token, coordinate_x, coordinate_y}
    );
    i32 effect =
        static_cast<i32>(std::bit_cast<i16>(static_cast<u16>(computed.eax)));
    if (effect >= 0x270F) {
        effect = 0x270F;
    }
    result.effect_value = effect;
    shared->last_effect_value = effect;
    port.battle_pair_primary_value() += std::bit_cast<u32>(effect);

    registers.ecx = request.target_token;
    ++result.effect_publish_calls;
    static_cast<void>(invoke_action(
        kCallPublishSignedValue,
        {request.target_token, std::bit_cast<u32>(effect)}
    ));
    ++result.effect_publish_calls;
    static_cast<void>(
        invoke_action(kCallTargetEffectProperty, {request.target_token, 1U})
    );

    const u32 effect_before_negation =
        std::bit_cast<u32>(shared->last_effect_value);
    shared->last_effect_value = -shared->last_effect_value;
    registers.ecx = request.actor_token;
    registers.edx = std::bit_cast<u32>(shared->last_effect_value);
    ++result.effect_publish_calls;
    if (!execute_legacy_battle_actor_effect_resource_slot_write_call(
            actor,
            result.effect_resource_slot_write,
            request.effect_resource_slot_write_requests,
            request.actor_token,
            0x246FU,
            registers.eax,
            registers.edx,
            0x00473508U,
            0x0047350DU,
            increment_flags(~effect_before_negation, false),
            false
        )) {
        result.status = LegacyBattleSpecialFourOhFiveStatus::
            actor_effect_resource_slot_write_typed_stop;
        result.return_eax = result.effect_resource_slot_write.last.return_eax;
        result.return_ecx = result.effect_resource_slot_write.last.return_ecx;
        result.return_edx = result.effect_resource_slot_write.last.return_edx;
        return result;
    }
    registers.eax = result.effect_resource_slot_write.last.return_eax;
    registers.ecx = result.effect_resource_slot_write.last.return_ecx;
    registers.edx = result.effect_resource_slot_write.last.return_edx;
    ++result.effect_publish_calls;
    static_cast<void>(invoke_action(
        kCallPublishSignedValue,
        {
            request.actor_token,
            std::bit_cast<u32>(shared->last_effect_value),
        }
    ));
    ++result.effect_publish_calls;
    static_cast<void>(
        invoke_action(kCallActorEffectMode, {request.actor_token, 0U})
    );
    ++result.effect_publish_calls;
    static_cast<void>(
        invoke_action(kCallTargetEffectProperty, {request.actor_token, 1U})
    );
    synchronize_legacy_battle_actor_effect_resource_cursor_update(actor, 1U);
    registers.ecx = request.actor_token;
    ++result.effect_publish_calls;
    if (!execute_legacy_battle_actor_effect_resource_slot_write_call(
            actor,
            result.effect_resource_slot_write,
            request.effect_resource_slot_write_requests,
            request.actor_token,
            0x2366U,
            registers.eax,
            registers.edx,
            0x00473533U,
            0x00473538U,
            {},
            false
        )) {
        result.status = LegacyBattleSpecialFourOhFiveStatus::
            actor_effect_resource_slot_write_typed_stop;
        result.return_eax = result.effect_resource_slot_write.last.return_eax;
        result.return_ecx = result.effect_resource_slot_write.last.return_ecx;
        result.return_edx = result.effect_resource_slot_write.last.return_edx;
        return result;
    }
    registers.eax = result.effect_resource_slot_write.last.return_eax;
    registers.ecx = result.effect_resource_slot_write.last.return_ecx;
    registers.edx = result.effect_resource_slot_write.last.return_edx;
    ++result.effect_publish_calls;
    static_cast<void>(invoke_action(
        kCallPublishSignedValue,
        {request.actor_token, port.battle_pair_primary_value()}
    ));
    ++result.effect_publish_calls;
    static_cast<void>(
        invoke_action(kCallActorEffectMode, {request.actor_token, 8U})
    );
    ++result.effect_publish_calls;
    static_cast<void>(
        invoke_action(kCallTargetEffectProperty, {request.actor_token, 1U})
    );
    synchronize_legacy_battle_actor_effect_resource_cursor_update(actor, 1U);
    u32 accumulator_word_register = registers.edx;
    replace_low_word(
        accumulator_word_register,
        static_cast<u16>(port.battle_pair_primary_value())
    );
    ++result.effect_publish_calls;
    static_cast<void>(invoke_action(
        kCallCommitVisual,
        {
            request.actor_token,
            std::bit_cast<u32>(shared->last_effect_value),
            0U,
            accumulator_word_register,
        }
    ));

    phase->tick = 0U;
    actor->effect_action_record = {};
    special = {};
    result.action_record_clears = 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSpecialFourOhSixResult advance_legacy_battle_special_four_oh_six(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourOhSixRequest& request
) {
    LegacyBattleSpecialFourOhSixResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFourOhSixStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };
    auto& special = actor->special_action_record;
    actor->turn_completion_latch = 1U;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    special.base_variant = actor->special_profile_variant;
    ++result.action_update_calls;
    const auto primary_updated = context.action_updater.update(special);
    registers.eax = primary_updated.return_value;
    if (primary_updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece primary_frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            special.field_4a, special.field_4c, primary_frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleSpecialFourOhSixStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFourOhSixStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    u16 primary_target_x = static_cast<u16>(special.draw_offset_x);
    actor->source_x_offset = special.field_76;
    u32 render_flags = special.mode_flags;
    if (actor->special_draw_mirror_mode == 1U) {
        render_flags ^= 1U;
        primary_target_x = static_cast<u16>(
            primary_frame.width - static_cast<u16>(special.draw_offset_x)
        );
        if (special.field_76 != 0U) {
            actor->source_x_offset =
                static_cast<u16>(primary_frame.width - special.field_76);
        }
    }

    if ((static_cast<u8>(special.field_5a) & 8U) != 0U) {
        actor->motion_word = 0xFFE1U;
        actor->action_runtime_gate =
            (actor->action_runtime_gate & 0xFFFFFF00U) |
            ((actor->action_runtime_gate & 0xFFU) | 3U);
        special.field_5a = 0U;
        actor->turn_threshold = 0U;
    }

    const auto publish_primary_geometry = [&]() {
        shared->draw_height_third = static_cast<u32>(primary_frame.height) / 3U;
        shared->draw_height_quarter =
            static_cast<u32>(primary_frame.height) >> 2U;
        shared->draw_motion_a = 0xFFFFFFFAU;
        shared->draw_motion_b = 0xFFFFFFFAU;
        shared->draw_motion_c = 0xFFFFFFFAU;
    };
    const auto draw_primary = [&](const u32 flags, const i32 x_adjustment) {
        ++result.render_calls;
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                signed_word_bits(actor->position_x) -
                    signed_word_bits(primary_target_x) +
                    std::bit_cast<u32>(x_adjustment),
                signed_word_bits(actor->position_y) - special.draw_offset_y,
                primary_frame.width,
                primary_frame.height,
                flags,
                0U,
            }
        ));
    };

    if (actor->action_runtime_gate == 0U) {
        registers.edx = 0x004AB784U;
        registers.eax = actor->turn_frame_token;
        replace_low_word(registers.eax, special.field_58);
        ++result.sample_play_calls;
        static_cast<void>(
            invoke_action(kCallPlayMessage, {registers.eax, registers.edx})
        );
        const u32 draw_x = signed_word_bits(actor->position_x) -
            signed_word_bits(primary_target_x);
        ++result.sample_pan_calls;
        if (std::bit_cast<i32>(draw_x) >= 0x140) {
            registers.eax = draw_x;
            replace_low_word(registers.eax, special.field_58);
            static_cast<void>(
                invoke_action(kCallSetSamplePan, {registers.eax, 0x10U})
            );
        } else {
            replace_low_word(registers.edx, special.field_58);
            static_cast<void>(
                invoke_action(kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U})
            );
        }
        special.field_58 = 0U;
        publish_primary_geometry();
        const u32 modified_flags = (render_flags & 0x8000000FU) | 0x0CU;
        actor->render_flags = modified_flags;
        ++result.render_calls;
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                draw_x - 5U,
                signed_word_bits(actor->position_y) - shared->draw_height_third,
                primary_frame.width,
                primary_frame.height,
                modified_flags,
                0U,
            }
        ));
        draw_primary(render_flags, 0);
    }

    if ((actor->action_runtime_gate & 1U) != 0U) {
        if (std::bit_cast<i16>(actor->turn_threshold) <= -30) {
            actor->action_runtime_gate &= ~1U;
        } else {
            const u32 motion = signed_word_bits(actor->turn_threshold);
            shared->draw_motion_a = motion;
            shared->draw_motion_b = motion;
            shared->draw_motion_c = motion;
            draw_primary(render_flags | 4U, 0);
            actor->turn_threshold =
                static_cast<u16>(actor->turn_threshold - 1U);
        }
    }

    if ((actor->action_runtime_gate & 2U) != 0U) {
        auto& secondary = actor->special_secondary_action_record;
        secondary.action_id = 0x17FEU;
        secondary.external_mode = 0U;
        ++result.action_update_calls;
        const auto secondary_updated = context.action_updater.update(secondary);
        registers.eax = secondary_updated.return_value;
        if (secondary_updated.return_value == 0U) {
            result.return_eax = 0U;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }

        rendering::LegacyFramePiece secondary_frame{};
        ++result.frame_lookup_calls;
        if (!context.frame_provider.load_frame_piece(
                secondary.field_4a, secondary.field_4c, secondary_frame
            )) {
            actor->turn_frame_token = 0U;
            result.status =
                LegacyBattleSpecialFourOhSixStatus::frame_owner_typed_stop;
            result.return_eax = 0U;
            return result;
        }
        actor->turn_frame_token = request.actor_token + 0x254CU;
        shared->turn_frame_source_token = actor->turn_frame_token;

        actor->secondary_target_x_offset =
            static_cast<u16>(secondary.draw_offset_x);
        actor->secondary_source_x_offset = secondary.field_76;
        render_flags = secondary.mode_flags;
        if (actor->special_draw_mirror_mode == 1U) {
            render_flags ^= 1U;
            actor->secondary_target_x_offset = static_cast<u16>(
                secondary_frame.width -
                static_cast<u16>(secondary.draw_offset_x)
            );
            if (secondary.field_76 != 0U) {
                actor->secondary_source_x_offset = static_cast<u16>(
                    secondary_frame.width - secondary.field_76
                );
            }
        }

        registers.edx = 0x004AB784U;
        registers.eax = actor->turn_frame_token;
        replace_low_word(registers.eax, secondary.field_58);
        ++result.sample_play_calls;
        static_cast<void>(
            invoke_action(kCallPlayMessage, {registers.eax, registers.edx})
        );
        const u32 primary_draw_x = signed_word_bits(actor->position_x) -
            signed_word_bits(primary_target_x);
        ++result.sample_pan_calls;
        if (std::bit_cast<i32>(primary_draw_x) >= 0x140) {
            registers.eax = primary_draw_x;
            replace_low_word(registers.eax, secondary.field_58);
            static_cast<void>(
                invoke_action(kCallSetSamplePan, {registers.eax, 0x10U})
            );
        } else {
            replace_low_word(registers.edx, secondary.field_58);
            static_cast<void>(
                invoke_action(kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U})
            );
        }
        secondary.field_58 = 0U;

        if (std::bit_cast<i16>(actor->motion_word) > 0) {
            actor->action_runtime_gate = 4U;
            actor->turn_threshold = 0U;
            actor->motion_word = 0U;
        } else {
            const u32 motion = signed_word_bits(actor->motion_word);
            shared->draw_motion_a = motion;
            shared->draw_motion_b = motion;
            shared->draw_motion_c = motion;
            ++result.render_calls;
            static_cast<void>(invoke_action(
                kCallActionThirteenRender,
                {
                    signed_word_bits(actor->position_x) -
                        signed_word_bits(actor->secondary_source_x_offset) -
                        signed_word_bits(primary_target_x) +
                        signed_word_bits(actor->source_x_offset),
                    signed_word_bits(actor->position_y) -
                        signed_word_bits(secondary.field_78) +
                        signed_word_bits(special.field_78) -
                        special.draw_offset_y,
                    secondary_frame.width,
                    secondary_frame.height,
                    render_flags | 4U,
                    0U,
                }
            ));
            actor->motion_word = static_cast<u16>(actor->motion_word + 1U);
        }
    }

    if (actor->action_runtime_gate == 4U) {
        ++result.effect_update_calls;
        ++result.port_calls;
        auto reply = port.invoke_special_four_oh_six_effect_update(
            {
                .callee_token = 0x0047F940U,
                .arguments =
                    {
                        request.target_token,
                        request.actor_token + 0x0630U,
                        0U,
                        0x17FEU,
                        signed_word_bits(actor->position_x) -
                            signed_word_bits(primary_target_x) +
                            signed_word_bits(actor->source_x_offset),
                        signed_word_bits(actor->position_y) +
                            signed_word_bits(special.field_78) -
                            special.draw_offset_y,
                        0xFFFFFFFFU,
                        0U,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->effect_action_record
        );
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        if (reply.eax == 1U) {
            registers.ecx = request.target_token;
            ++result.target_refresh_calls;
            if (!apply_actor_field_26b8_high_bit_set_call(
                    context.startup,
                    result.actor_field_26b8_high_bit_set,
                    request.actor_field_26b8_high_bit_set_requests,
                    request.target_token,
                    0x00473A99U,
                    registers
                )) {
                result.status = LegacyBattleSpecialFourOhSixStatus::
                    actor_field_26b8_high_bit_set_typed_stop;
                result.return_eax = registers.eax;
                result.return_ecx = registers.ecx;
                result.return_edx = registers.edx;
                return result;
            }
            registers.ecx = request.actor_token;
            ++result.effect_compute_calls;
            const auto computed = invoke_action(
                kCallComputeValue,
                {
                    request.target_token,
                    request.stale_stack_word_6,
                    request.stale_stack_word_8,
                }
            );
            i32 effect = static_cast<i32>(
                std::bit_cast<i16>(static_cast<u16>(computed.eax))
            );
            if (effect >= 0x270F) {
                effect = 0x270F;
            }
            result.effect_value = effect;
            shared->last_effect_value = effect;
            port.battle_pair_primary_value() += std::bit_cast<u32>(effect);
            registers.ecx = request.target_token;
            ++result.effect_publish_calls;
            static_cast<void>(invoke_action(
                kCallPublishSignedValue,
                {request.target_token, std::bit_cast<u32>(effect)}
            ));
            ++result.effect_publish_calls;
            static_cast<void>(invoke_action(
                kCallTargetEffectProperty, {request.target_token, 1U}
            ));
            actor->action_runtime_gate = 0x4000U;
            actor->turn_threshold = 0xFFE1U;
        }
    }

    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        auto& secondary_effect = actor->effect_secondary_action_record;
        secondary_effect.action_id = 0x1F88U;
        secondary_effect.base_variant = 0U;
        ++result.secondary_update_calls;
        ++result.port_calls;
        const auto reply = port.invoke_special_four_oh_six_secondary_update(
            {
                .callee_token = 0x00483DB0U,
                .arguments =
                    {
                        request.target_token,
                        request.actor_token + 0x06C8U,
                        0U,
                        0U,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            secondary_effect
        );
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        if (reply.eax == 1U) {
            actor->action_runtime_gate = 0x2000U;
        }
    }

    if ((actor->action_runtime_gate & 0x2000U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if (std::bit_cast<i16>(actor->turn_threshold) > 0) {
        actor->turn_threshold = 0U;
        actor->action_runtime_gate = 0U;
        special = {};
        actor->special_secondary_action_record = {};
        actor->effect_action_record = {};
        actor->effect_secondary_action_record = {};
        result.action_record_clears = 4U;
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    const u32 final_motion = signed_word_bits(actor->turn_threshold);
    shared->draw_motion_a = final_motion;
    shared->draw_motion_b = final_motion;
    shared->draw_motion_c = final_motion;
    draw_primary(render_flags | 4U, 0);
    actor->turn_threshold = static_cast<u16>(actor->turn_threshold + 2U);
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSpecialFourOhNineResult advance_legacy_battle_special_four_oh_nine(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleSpecialFourOhNineRequest& request,
    const LegacyBattleActorCoordinateOwners& coordinate_owners
) {
    LegacyBattleSpecialFourOhNineResult result{};
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFourOhNineStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = 0U,
        .ecx = static_cast<u32>(actor->profile_value),
        .edx = static_cast<u32>(actor->special_profile_variant),
    };
    auto publish_registers = [&]() {
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto finish_zero = [&]() {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };

    u32 coordinate_x = 0U;
    u32 coordinate_y = 0U;
    auto& special = actor->special_action_record;
    registers.ecx += 0x5DCU;
    special.action_id = registers.ecx;
    special.base_variant = registers.edx;
    registers.ecx = actor->action_runtime_gate;
    actor->turn_completion_latch = 1U;
    if (actor->action_runtime_gate == 0U) {
        ++result.special_update_calls;
        ++result.port_calls;
        const auto primary_reply =
            port.invoke_special_four_hundred_primary_update(
                {
                    .callee_token = kCallSpecialActionUpdate,
                    .arguments =
                        {
                            request.target_token,
                            request.actor_token + 0x0AF0U,
                        },
                    .eax = registers.eax,
                    .ecx = request.actor_token,
                    .edx = registers.edx,
                },
                special,
                actor->turn_frame_token,
                actor->turn_render_flags,
                actor->special_primary_draw_x,
                actor->special_primary_draw_y
            );
        registers.eax = primary_reply.eax;
        registers.ecx = primary_reply.ecx;
        registers.edx = primary_reply.edx;
    }

    if (actor->action_runtime_gate == 1U) {
        const u32 coordinate_counter = special.base_variant;
        registers.edx = special.base_variant;
        special.base_variant = registers.edx + 1U;
        registers.edx = special.base_variant;
        ++result.coordinate_query_calls;
        result.coordinate_query = query_coordinates(
            coordinate_owners,
            request.target_token,
            coordinate_x,
            coordinate_y,
            request.coordinate_output_x_token,
            request.coordinate_output_y_token,
            request.coordinate_output_y_token,
            registers.edx,
            increment_flags(coordinate_counter, false)
        );
        registers.eax = result.coordinate_query.return_eax;
        registers.ecx = result.coordinate_query.return_ecx;
        registers.edx = result.coordinate_query.return_edx;
        if (result.coordinate_query.status !=
            LegacyBattleActorCoordinateQueryStatus::completed) {
            result.status = LegacyBattleSpecialFourOhNineStatus::
                actor_coordinate_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        registers.eax = coordinate_x;
        registers.edx = coordinate_y;
        registers.ecx = request.actor_token;
        ++result.coordinate_update_calls;
        ++result.port_calls;
        const auto updated = port.invoke_special_four_oh_nine_coordinate_update(
            {
                .callee_token = kCallSpecialFourOhNineCoordinateUpdate,
                .arguments =
                    {
                        coordinate_x,
                        coordinate_y,
                        request.actor_token + 0x0AF0U,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            special
        );
        registers.eax = updated.eax;
        registers.ecx = updated.ecx;
        registers.edx = updated.edx;
    }

    registers.ecx = actor->action_runtime_gate;
    registers.eax = 2U;
    if (actor->action_runtime_gate == 2U) {
        registers.ecx = special.base_variant;
        actor->special_effect_direct_mode |= 8U;
        special.base_variant = registers.ecx + registers.eax;
        registers.eax = special.base_variant;
        registers.ecx = request.actor_token;
        ++result.stage_two_calls;
        const auto stage_two = invoke_action(
            kCallActorExit,
            {
                request.target_token,
                special.action_id,
                special.base_variant,
            }
        );
        if (stage_two.eax == 1U) {
            actor->action_runtime_gate = 3U;
        }
    }

    if (special.field_8c == 1U) {
        registers.ecx = 0U;
        registers.eax = 0U;
        special = {};
        ++result.action_record_clears;
        actor->action_runtime_gate += 1U;
    }
    if ((special.field_5a & 8U) != 0U) {
        if (shared == nullptr) {
            result.status =
                LegacyBattleSpecialFourOhNineStatus::shared_state_typed_stop;
            return publish_registers();
        }
        registers.ecx = shared->action_completion_flags;
        registers.eax = 0x8000U;
        if ((shared->action_completion_flags & 0x8000U) == 0U) {
            shared->action_completion_flags |= 0x8000U;
        }
        special.field_5a = 0U;
    }
    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFourOhNineStatus::shared_state_typed_stop;
        return publish_registers();
    }
    if ((shared->action_completion_flags & 1U) == 0U ||
        std::bit_cast<i32>(actor->action_runtime_gate) < 3) {
        return finish_zero();
    }

    actor->action_runtime_gate = 0U;
    registers.ecx = 0U;
    registers.eax = 1U;
    special = {};
    ++result.action_record_clears;
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

}  // namespace openswd3::battle
