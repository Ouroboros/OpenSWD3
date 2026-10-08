#include "legacy_battle_action_dispatch_internal.hpp"

namespace openswd3::battle::action_dispatch_detail {

bool ActionDispatchRunner::require_group_b() {
    if (group_b_index >= 8U) {
        result.status =
            LegacyBattleActionDispatchStatus::group_b_index_typed_stop;
        return false;
    }
    return true;
}

u32 ActionDispatchRunner::side_token(const u32 index) {
    return state.side_mode != 0U ? group_a_token(index) : group_b_token(index);
}

bool ActionDispatchRunner::begin_action(const u32 target_token) {
    if (context.startup == nullptr ||
        group_a_index >= state.group_a_action_execution.size() ||
        group_a_index >= context.startup->party.size()) {
        result.status = LegacyBattleActionDispatchStatus::
            group_a_action_execution_typed_stop;
        return false;
    }
    const u32 skip_primary = group_a_index < context.group_a_skip_primary.size()
        ? context.group_a_skip_primary[group_a_index]
        : 0U;
    const u32 skip_secondary =
        group_a_index < context.group_a_skip_secondary.size()
        ? context.group_a_skip_secondary[group_a_index]
        : 0U;
    auto& party = context.startup->party[group_a_index];
    result.group_a_action_execution =
        advance_legacy_battle_group_a_action_execution(
            &state.group_a_action_execution[group_a_index],
            state.group_a_action_shared,
            state,
            party.progress,
            party.item_effect_application,
            actor_token,
            target_token,
            0U,
            skip_primary,
            skip_secondary,
            port,
            {
                .startup = context.startup,
                .actor_field_26b8_high_bit_set_requests =
                    context.actor_field_26b8_high_bit_set_requests,
            }
        );
    ++result.group_a_action_execution_calls;
    result.port_calls += result.group_a_action_execution.port_calls;
    append_nested_actor_field_26b8_high_bit_set(
        result.actor_field_26b8_high_bit_set,
        result.group_a_action_execution.actor_field_26b8_high_bit_set
    );
    if (result.group_a_action_execution.status !=
        LegacyBattleGroupAActionExecutionStatus::completed) {
        result.status = LegacyBattleActionDispatchStatus::
            group_a_action_execution_typed_stop;
        return false;
    }
    return result.group_a_action_execution.return_eax == 1U;
}

bool ActionDispatchRunner::release_actor_resource() {
    if (context.startup == nullptr ||
        group_a_index >= context.startup->party.size()) {
        result.status = LegacyBattleActionDispatchStatus::
            group_a_actor_list_action_typed_stop;
        return false;
    }

    auto& party = context.startup->party[group_a_index];
    result.actor_resource_release = release_legacy_battle_actor_resource(
        &party.actor_list,
        &party.workspace,
        actor_token,
        {.entry_edx = state.selection_source}
    );
    ++result.actor_resource_release_calls;
    if (result.actor_resource_release.status !=
        LegacyBattleActorListQueryStatus::completed) {
        result.status = LegacyBattleActionDispatchStatus::
            group_a_actor_list_action_typed_stop;
        return false;
    }

    return true;
}

LegacyBattleActionDispatchResult ActionDispatchRunner::dispatch_extended() {
    if (action <= 0x194U) {
        if (action == 0x64U) {
            result.return_value = 1U;
            return result;
        }
        if (action == 0xC8U) {
            if ((state.side_mode != 0U && group_b_index >= 10U) ||
                (state.side_mode == 0U && !require_group_b())) {
                return result;
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallSimpleActorUpdate,
                {side_token(group_b_index)}
            ));
            return result;
        }
        if (action == 0x12CU) {
            if ((state.side_mode != 0U && group_b_index >= 10U) ||
                (state.side_mode == 0U && !require_group_b())) {
                return result;
            }
            reply = invoke(
                state,
                port,
                result,
                kCallActorExit,
                {side_token(group_b_index), 0xFFFFFFFFU, 0U}
            );
            if (reply.eax == 1U) {
                state.current_actor_index = 0xFFFFU;
                result.return_value = 1U;
            }
            return result;
        }
        if (action == 0x190U || action == 0x192U) {
            if (!require_group_b()) {
                return result;
            }
            if (action == 0x192U) {
                auto& control = port.frame_effect_control_state();
                control.red_factor = -12;
                control.green_factor = -12;
                control.blue_factor = -12;
                control.primary_suppression = 1U;
                state.frame_effect.alternate_surface_mode = 1U;
                result.action_four_oh_two =
                    advance_legacy_battle_action_four_oh_two(
                        &state.group_a_action_execution[group_a_index],
                        port,
                        {
                            .actor_token = actor_token,
                            .target_token = group_b_token(group_b_index),
                            .coordinate_output_x_token =
                                state.coordinate_output_x_token,
                            .coordinate_output_y_token =
                                state.coordinate_output_y_token,
                        },
                        {
                            .action = context.shared_action_dispatch,
                            .startup = context.startup,
                        }
                    );
                ++result.action_four_oh_two_calls;
                result.port_calls += result.action_four_oh_two.port_calls;
                if (result.action_four_oh_two.status !=
                    LegacyBattleActionFourOhTwoStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        action_four_oh_two_typed_stop;
                    return result;
                }
                reply.eax = result.action_four_oh_two.return_eax;
                reply.ecx = result.action_four_oh_two.return_ecx;
                reply.edx = result.action_four_oh_two.return_edx;
            } else {
                if (context.startup == nullptr ||
                    group_a_index >= context.startup->party.size()) {
                    result.status = LegacyBattleActionDispatchStatus::
                        special_four_hundred_typed_stop;
                    return result;
                }
                result.special_four_hundred =
                    advance_legacy_battle_special_four_hundred(
                        &state.group_a_action_execution[group_a_index],
                        &context.startup->party[group_a_index].progress,
                        &state.group_a_action_shared,
                        port,
                        context,
                        {
                            .actor_token = actor_token,
                            .target_token = group_b_token(group_b_index),
                            .coordinate_output_x_token =
                                state.coordinate_output_x_token,
                            .coordinate_output_y_token =
                                state.coordinate_output_y_token,
                            .actor_field_26b8_high_bit_set_requests =
                                context.actor_field_26b8_high_bit_set_requests,
                        }
                    );
                ++result.special_four_hundred_calls;
                result.port_calls += result.special_four_hundred.port_calls;
                append_nested_actor_field_26b8_high_bit_set(
                    result.actor_field_26b8_high_bit_set,
                    result.special_four_hundred.actor_field_26b8_high_bit_set
                );
                if (result.special_four_hundred.status !=
                    LegacyBattleSpecialFourHundredStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        special_four_hundred_typed_stop;
                    return result;
                }
                reply.eax = result.special_four_hundred.return_eax;
                reply.ecx = result.special_four_hundred.return_ecx;
                reply.edx = result.special_four_hundred.return_edx;
            }
            if (reply.eax != 1U) {
                return result;
            }
            state.action_pending = 1U;
            if (!publish_target(state, result, group_b_index) ||
                !update_effect_score(state, result, group_a_index, 2U)) {
                return result;
            }
            if (state.blocking_effect == 0U) {
                reply = invoke(
                    state,
                    port,
                    result,
                    kCallCommitVisual,
                    {port.battle_pair_primary_value(), 0U, 0U}
                );
                if (reply.eax == 1U) {
                    state.selected_target_index =
                        static_cast<u16>(group_b_index);
                    state.selected_group_b_identity[group_b_index] =
                        group_b_index;
                    port.battle_pair_primary_value() = 0xFFFFFFFFU;
                    if (!clear_framebuffer(port, context, result)) {
                        return result;
                    }
                    const u32 framebuffer_bytes =
                        static_cast<u32>(context.raster.surface.width) *
                        static_cast<u32>(context.raster.surface.height) * 2U;
                    if (!apply_legacy_battle_actor_action_mode_call(
                            state,
                            context,
                            result,
                            actor_token,
                            0x12CU,
                            0xFFFFFFFFU,
                            framebuffer_bytes,
                            0x00455AE8U,
                            logical_flags(framebuffer_bytes & 3U)
                        )) {
                        return result;
                    }
                    if (!update_effect_score(
                            state, result, group_a_index, 5U
                        )) {
                        return result;
                    }
                }
            }
            port.battle_pair_primary_value() = 0U;
            const auto pending_clear =
                invoke(state, port, result, kCallClearPendingAction, {0U});
            if (!apply_legacy_battle_actor_action_mode_call(
                    state,
                    context,
                    result,
                    actor_token,
                    0x12CU,
                    pending_clear.eax,
                    pending_clear.edx,
                    0x00455CBBU,
                    pending_clear.flags
                )) {
                return result;
            }
            return result;
        }
        if (action == 0x194U) {
            if (!require_group_b()) {
                return result;
            }
            u16 phase = state.special_phase;
            if ((phase & 0x7FFFU) == 0U) {
                state.special_phase = 1U;
                for (i32 index = 0; index < state.group_a_count; ++index) {
                    ++result.group_a_iterations;
                    if (index >= 10) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_a_index_typed_stop;
                        return result;
                    }
                    if (index != static_cast<i32>(group_a_index)) {
                        reply = invoke(
                            state,
                            port,
                            result,
                            kCallActorSuspended,
                            {group_a_token(static_cast<u32>(index))}
                        );
                        if (reply.eax != 1U) {
                            static_cast<void>(invoke(
                                state, port, result, kCallPushState, {4U}
                            ));
                            static_cast<void>(invoke(
                                state, port, result, kCallPushState, {0x40U}
                            ));
                        }
                    }
                }
                for (i32 index = 0; index < state.group_b_count; ++index) {
                    ++result.group_b_iterations;
                    if (index >= 8) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_index_typed_stop;
                        return result;
                    }
                    static_cast<void>(
                        invoke(state, port, result, kCallPushState, {0x40U})
                    );
                    static_cast<void>(
                        invoke(state, port, result, kCallPushState, {4U})
                    );
                }
                phase = state.special_phase;
            }
            if ((phase & 0x7FFFU) == 1U) {
                state.special_phase = 2U;
                state.frame_effect.split_extent = 1U;
                state.frame_effect.split_suppression = 1U;
                phase = 2U;
            }
            if ((phase & 0x7FFFU) == 2U) {
                if (context.startup == nullptr ||
                    group_a_index >= context.startup->party.size()) {
                    result.status = LegacyBattleActionDispatchStatus::
                        action_four_effect_typed_stop;
                    return result;
                }
                result.action_four_effect =
                    advance_legacy_battle_action_four_effect(
                        &state.group_a_action_execution[group_a_index],
                        &context.startup->party[group_a_index].progress,
                        &state.group_a_action_shared,
                        port,
                        context,
                        {
                            .actor_token = actor_token,
                            .target_token = group_b_token(group_b_index),
                            .actor_field_26b8_high_bit_set_requests =
                                context.actor_field_26b8_high_bit_set_requests,
                        }
                    );
                ++result.action_four_effect_calls;
                result.port_calls += result.action_four_effect.port_calls;
                append_nested_actor_field_26b8_high_bit_set(
                    result.actor_field_26b8_high_bit_set,
                    result.action_four_effect.actor_field_26b8_high_bit_set
                );
                if (result.action_four_effect.status !=
                    LegacyBattleActionFourEffectStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        action_four_effect_typed_stop;
                    return result;
                }
                reply.eax = result.action_four_effect.return_eax;
                reply.ecx = result.action_four_effect.return_ecx;
                reply.edx = result.action_four_effect.return_edx;
                if (reply.eax == 1U) {
                    state.action_pending = 1U;
                    if (!publish_target(state, result, group_b_index) ||
                        !update_effect_score(
                            state, result, group_a_index, 2U
                        )) {
                        return result;
                    }
                    if (state.blocking_effect == 0U &&
                        invoke(
                            state,
                            port,
                            result,
                            kCallCommitVisual,
                            {port.battle_pair_primary_value(), 0U, 0U}
                        )
                                .eax == 1U) {
                        state.selected_target_index =
                            static_cast<u16>(group_b_index);
                        state.selected_group_b_identity[group_b_index] =
                            group_b_index;
                        port.battle_pair_primary_value() = 0xFFFFFFFFU;
                        if (!clear_framebuffer(port, context, result) ||
                            !update_effect_score(
                                state, result, group_a_index, 5U
                            )) {
                            return result;
                        }
                        const u32 framebuffer_bytes =
                            static_cast<u32>(context.raster.surface.width) *
                            static_cast<u32>(context.raster.surface.height) *
                            2U;
                        if (!apply_legacy_battle_actor_action_mode_call(
                                state,
                                context,
                                result,
                                actor_token,
                                0x12CU,
                                0xFFFFFFFFU,
                                framebuffer_bytes,
                                0x004558F9U,
                                logical_flags(framebuffer_bytes & 3U)
                            )) {
                            return result;
                        }
                    }
                    port.battle_pair_primary_value() = 0U;
                    state.special_phase = 0U;
                    state.frame_effect.split_extent = 0U;
                    state.frame_effect.split_suppression = 0U;
                    const auto pending_clear = invoke(
                        state, port, result, kCallClearPendingAction, {0U}
                    );
                    if (!apply_legacy_battle_actor_action_mode_call(
                            state,
                            context,
                            result,
                            actor_token,
                            0x12CU,
                            pending_clear.eax,
                            pending_clear.edx,
                            0x00455951U,
                            pending_clear.flags
                        )) {
                        return result;
                    }
                    for (i32 index = 0; index < state.group_a_count; ++index) {
                        ++result.group_a_iterations;
                        if (index >= 10) {
                            result.status = LegacyBattleActionDispatchStatus::
                                group_a_index_typed_stop;
                            return result;
                        }
                        if (index != static_cast<i32>(group_a_index)) {
                            static_cast<void>(
                                invoke(state, port, result, kCallPopState, {4U})
                            );
                            static_cast<void>(invoke(
                                state, port, result, kCallPopState, {0x40U}
                            ));
                        }
                    }
                    for (i32 index = 0; index < state.group_b_count; ++index) {
                        ++result.group_b_iterations;
                        if (index >= 8) {
                            result.status = LegacyBattleActionDispatchStatus::
                                group_b_index_typed_stop;
                            return result;
                        }
                        static_cast<void>(
                            invoke(state, port, result, kCallPopState, {4U})
                        );
                        static_cast<void>(
                            invoke(state, port, result, kCallPopState, {0x40U})
                        );
                    }
                }
            }
            return result;
        }
        return result;
    }

    if (!require_group_b()) {
        return result;
    }
    const u32 target_token = group_b_token(group_b_index);
    if (action == 0x195U) {
        result
            .special_four_oh_five = advance_legacy_battle_special_four_oh_five(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .target_token = target_token,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .actor_field_26b8_high_bit_set_requests =
                    context.actor_field_26b8_high_bit_set_requests,
                .effect_resource_slot_write_requests =
                    context.effect_resource_slot_write_requests,
            }
        );
        ++result.special_four_oh_five_calls;
        result.port_calls += result.special_four_oh_five.port_calls;
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            result.special_four_oh_five.actor_field_26b8_high_bit_set
        );
        append_legacy_battle_actor_effect_resource_slot_write_trace(
            result.effect_resource_slot_write,
            result.special_four_oh_five.effect_resource_slot_write
        );
        if (result.special_four_oh_five.status !=
            LegacyBattleSpecialFourOhFiveStatus::completed) {
            result.status = result.special_four_oh_five.status ==
                    LegacyBattleSpecialFourOhFiveStatus::
                        actor_effect_resource_slot_write_typed_stop
                ? LegacyBattleActionDispatchStatus::
                      actor_effect_resource_slot_write_typed_stop
                : LegacyBattleActionDispatchStatus::
                      special_four_oh_five_typed_stop;
            return result;
        }
        if (result.special_four_oh_five.return_eax != 1U) {
            return result;
        }
    } else if (action == 0x196U) {
        result.special_four_oh_six = advance_legacy_battle_special_four_oh_six(
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .target_token = target_token,
                .actor_field_26b8_high_bit_set_requests =
                    context.actor_field_26b8_high_bit_set_requests,
            }
        );
        ++result.special_four_oh_six_calls;
        result.port_calls += result.special_four_oh_six.port_calls;
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            result.special_four_oh_six.actor_field_26b8_high_bit_set
        );
        if (result.special_four_oh_six.status !=
            LegacyBattleSpecialFourOhSixStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                special_four_oh_six_typed_stop;
            return result;
        }
        if (result.special_four_oh_six.return_eax != 1U) {
            return result;
        }
    } else if (action == 0x199U) {
        result
            .special_four_oh_nine = advance_legacy_battle_special_four_oh_nine(
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            {
                .actor_token = actor_token,
                .target_token = target_token,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
            },
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            }
        );
        ++result.special_four_oh_nine_calls;
        result.port_calls += result.special_four_oh_nine.port_calls;
        if (result.special_four_oh_nine.status !=
            LegacyBattleSpecialFourOhNineStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                special_four_oh_nine_typed_stop;
            return result;
        }
        if (result.special_four_oh_nine.return_eax != 1U) {
            return result;
        }
    } else if (action == 0x1F4U) {
        result.special_five_hundred =
            advance_legacy_battle_special_five_hundred(
                &state.group_a_action_execution[group_a_index],
                &state.group_a_action_shared,
                port,
                {
                    .actor_token = actor_token,
                    .source_token = target_token,
                }
            );
        ++result.special_five_hundred_calls;
        result.port_calls += result.special_five_hundred.port_calls;
        if (result.special_five_hundred.status !=
            LegacyBattleSpecialFiveHundredStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                special_five_hundred_typed_stop;
            return result;
        }
        if (result.special_five_hundred.return_eax != 1U) {
            return result;
        }
    } else {
        return result;
    }

    state.action_pending = 1U;
    if (!publish_target(state, result, group_b_index) ||
        !update_effect_score(state, result, group_a_index, 2U)) {
        return result;
    }
    bool special_framebuffer_cleared = false;
    if (action != 0x1F4U && state.blocking_effect == 0U &&
        invoke(
            state,
            port,
            result,
            kCallCommitVisual,
            {port.battle_pair_primary_value(), 0U, 0U}
        )
                .eax == 1U) {
        state.selected_target_index = static_cast<u16>(group_b_index);
        state.selected_group_b_identity[group_b_index] = group_b_index;
        if (action == 0x195U || action == 0x196U || action == 0x199U) {
            port.battle_pair_primary_value() = 0xFFFFFFFFU;
        }
        if (action == 0x199U) {
            static_cast<void>(
                invoke(state, port, result, kCallSetScreenMode, {1U})
            );
            replace_low_word(state.scan_push_state, 0x8000U);
        }
        if (!clear_framebuffer(port, context, result)) {
            return result;
        }
        special_framebuffer_cleared = true;
        if (action == 0x195U || action == 0x196U) {
            const u32 framebuffer_bytes =
                static_cast<u32>(context.raster.surface.width) *
                static_cast<u32>(context.raster.surface.height) * 2U;
            if (!apply_legacy_battle_actor_action_mode_call(
                    state,
                    context,
                    result,
                    actor_token,
                    0x12CU,
                    0xFFFFFFFFU,
                    framebuffer_bytes,
                    0x00455AE8U,
                    logical_flags(framebuffer_bytes & 3U)
                )) {
                return result;
            }
        }
        if (!update_effect_score(state, result, group_a_index, 5U)) {
            return result;
        }
    }
    port.battle_pair_primary_value() = 0U;
    if (action == 0x199U) {
        state.current_actor_index = 0xFFFFU;
        const auto& seeded =
            context.actor_action_mode_requests[result.actor_action_mode_calls];
        u32 mode_eax = seeded.entry_eax;
        u32 mode_edx = seeded.entry_edx;
        auto mode_flags = seeded.entry_flags;
        bool mode_flags_known = seeded.entry_flags_known;
        if (special_framebuffer_cleared) {
            const u32 mapped_actor = state.group_a_to_actor[group_a_index];
            const u32 effect_score = state.actor_effect_score[mapped_actor];
            mode_eax = 0x004ACF54U + mapped_actor * 96U;
            mode_edx = static_cast<u32>(context.raster.surface.width) *
                static_cast<u32>(context.raster.surface.height) * 2U;
            mode_flags = add_flags(effect_score - 5U, 5U);
            mode_flags_known = true;
        }
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                0x12CU,
                mode_eax,
                mode_edx,
                0x00455C1BU,
                mode_flags,
                mode_flags_known
            )) {
            return result;
        }
    }
    const auto pending_clear =
        invoke(state, port, result, kCallClearPendingAction, {0U});
    if (action != 0x199U &&
        !apply_legacy_battle_actor_action_mode_call(
            state,
            context,
            result,
            actor_token,
            0x12CU,
            pending_clear.eax,
            pending_clear.edx,
            0x00455CBBU,
            pending_clear.flags
        )) {
        return result;
    }
    if (action == 0x199U) {
        result.return_value = 1U;
    }
    return result;
}

LegacyBattleActionDispatchResult ActionDispatchRunner::run() {
    if (group_a_index >= 10U) {
        result.status =
            LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
        return result;
    }
    actor_token = group_a_token(group_a_index);
    auto action_kind_request = context.actor_action_kind_request;
    action_kind_request.actor_token = actor_token;
    action_kind_request.entry_eax = group_a_index * 0xBCDU;
    action_kind_request.entry_return_address = 0x004539EBU;
    action_kind_request.entry_flags =
        subtract_flags(group_a_index * 0x3F0U, group_a_index);
    action_kind_request.entry_flags_known = true;
    result.actor_action_kind = query_legacy_battle_actor_action_kind(
        resolve_legacy_battle_actor_action_kind(
            {.action = &state, .startup = context.startup}, actor_token
        ),
        action_kind_request
    );
    ++result.actor_action_kind_calls;
    if (result.actor_action_kind.status !=
        LegacyBattleActorActionKindStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_action_kind_typed_stop;
        result.return_value = result.actor_action_kind.return_eax;
        return result;
    }
    action = low_word(result.actor_action_kind.return_eax);
    result.action_code = action;
    reply = invoke(state, port, result, kCallActorTerminal, {actor_token});
    if (reply.eax == 1U) {
        result.return_value = 1U;
        return result;
    }
    if (action == 0U) {
        auto display_kind_request = context.actor_display_kind_request;
        display_kind_request.actor_token = actor_token;
        display_kind_request.entry_eax = reply.eax;
        display_kind_request.entry_edx = reply.edx;
        display_kind_request.entry_return_address = 0x00453A13U;
        display_kind_request.entry_flags = logical_flags(action);
        display_kind_request.entry_flags_known = true;
        result.actor_display_kind = query_legacy_battle_actor_display_kind(
            resolve_legacy_battle_actor_display_kind(
                {.startup = context.startup}, actor_token
            ),
            display_kind_request
        );
        ++result.actor_display_kind_calls;
        if (result.actor_display_kind.status !=
            LegacyBattleActorDisplayKindStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::actor_display_kind_typed_stop;
            result.return_value = result.actor_display_kind.return_eax;
            return result;
        }
        action = low_word(result.actor_display_kind.return_eax);
        result.action_code = action;
        if (action == 0U) {
            result.return_value = 1U;
            return result;
        }
    }

    if (action == 0x63U) {
        static_cast<void>(
            invoke(state, port, result, kCallSetScreenMode, {0U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallSetGlobalMode, {0U})
        );
        state.stored_group_b_index = 0xFFFFU;
        state.stored_group_a_index = 0xFFFFU;
        state.current_actor_index = 0xFFFFU;
        state.result_mode = 1U;
        state.battle_submode = 2U;
        ++result.terminal_resets;
        result.return_value = 1U;
        return result;
    }

    if (action > 0x63U) {
        return dispatch_extended();
    }

    return action <= 14U ? dispatch_low() : dispatch_high();
}

}  // namespace openswd3::battle::action_dispatch_detail
