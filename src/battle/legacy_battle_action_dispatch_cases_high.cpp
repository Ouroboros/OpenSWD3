#include "legacy_battle_action_dispatch_internal.hpp"

namespace openswd3::battle::action_dispatch_detail {

LegacyBattleActionDispatchResult ActionDispatchRunner::dispatch_high() {
    switch (action) {
    case 15U: {
        if (low_word(state.phase_counter) == 0U) {
            const u16 count = static_cast<u16>(state.summon_packed >> 16U);
            if ((port.battle_debug_hotkey_state().battle_mode_flags_53bc24 &
                 4U) == 0U &&
                count < 2U) {
                replace_high_word(
                    state.summon_packed, static_cast<u16>(count + 1U)
                );
                ++state.group_a_count;
            }
            if (group_a_index >= state.group_a_status_words.size()) {
                result.status =
                    LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
                return result;
            }
            const u16 summon_index = state.group_a_status_words[group_a_index];
            replace_low_word(state.summon_packed, summon_index);
            if (summon_index >= 10U) {
                result.status =
                    LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
                return result;
            }
            static_cast<void>(invoke(
                state, port, kCallSelectSummon, {group_a_token(summon_index)}
            ));
            const auto summon_mode_reply =
                invoke(state, port, kCallSummonMode, {1U});
            auto snapshot_clear_request =
                context.actor_frame_snapshot_clear_request;
            snapshot_clear_request.actor_token = group_a_token(summon_index);
            snapshot_clear_request.entry_eax =
                static_cast<u32>(summon_index) * 0xBCDU;
            snapshot_clear_request.entry_edx = summon_mode_reply.edx;
            snapshot_clear_request.entry_edi = 14U;
            snapshot_clear_request.entry_return_address = 0x0045529FU;
            snapshot_clear_request.entry_flags = subtract_flags(
                static_cast<u32>(summon_index) * 0x3F0U,
                static_cast<u32>(summon_index)
            );
            snapshot_clear_request.entry_flags_known = true;
            result.actor_frame_snapshot_clear =
                clear_legacy_battle_actor_frame_snapshot(
                    resolve_legacy_battle_actor_frame_snapshot_clear(
                        {.action = &state}, snapshot_clear_request.actor_token
                    ),
                    snapshot_clear_request
                );
            ++result.actor_frame_snapshot_clear_calls;
            if (result.actor_frame_snapshot_clear.status !=
                LegacyBattleActorFrameSnapshotClearStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_frame_snapshot_clear_typed_stop;
                result.return_value =
                    result.actor_frame_snapshot_clear.return_eax;
                return result;
            }
            static_cast<void>(invoke(state, port, kCallClearMode, {1U}));
            if (state.summon_gate == 0U) {
                static_cast<void>(
                    invoke(state, port, kCallSetGlobalMode, {1U})
                );
            }
            LegacyBattleGroupAConfigurationState* summon_state = nullptr;
            LegacyBattleGroupAPlacementRecord summon_source{};
            const LegacyBattleGroupAPlacementRecord* summon_source_view =
                nullptr;
            u32 summon_window_token = 0U;
            if (context.startup != nullptr) {
                auto& party = context.startup->party[summon_index];
                summon_state = &party.configuration;
                summon_source = {
                    .prefix = party.placement_prefix,
                    .role_id = party.role_id,
                    .position_x = party.position_x,
                    .position_y = party.position_y,
                    .field_1a = party.placement_field_1a,
                    .active = party.active,
                };
                summon_source_view = &summon_source;
                summon_window_token = context.startup->window_token;
            }
            result.summon_materialization =
                materialize_legacy_battle_group_a_summon(
                    summon_state,
                    summon_source_view,
                    group_a_token(summon_index),
                    0x0053AF70U + summon_index * 0x20U,
                    summon_window_token,
                    port
                );
            ++result.summon_materialization_calls;
            if (result.summon_materialization.status !=
                LegacyBattleGroupASummonMaterializationStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    summon_materialization_typed_stop;
                return result;
            }
            replace_low_word(state.phase_counter, 1U);
            state.summon_x = 0U;
            state.summon_y = 0U;
        }
        auto& control = port.frame_effect_control_state();
        control.red_factor = -12;
        control.green_factor = -12;
        control.blue_factor = -12;
        control.primary_suppression = 1U;
        if (!refresh_shared_frame(port, result)) {
            return result;
        }

        const u16 summon_index = low_word(state.summon_packed);
        if (summon_index >= state.summon_target_x.size()) {
            result.status =
                LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
            return result;
        }
        result.summon_frame = advance_legacy_battle_summon_frame(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context.action_updater,
            context.frame_provider,
            {
                .actor_token = group_a_token(group_a_index),
                .position_x = state.summon_target_x[summon_index],
                .position_y = state.summon_target_y[summon_index],
            }
        );
        ++result.summon_frame_calls;
        if (result.summon_frame.status !=
            LegacyBattleSummonFrameStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::summon_frame_typed_stop;
            return result;
        }
        if (result.summon_frame.return_eax != 1U) {
            return result;
        }
        if (!remove_attack_order_entry(
                context, result, static_cast<u32>(summon_index + 8U)
            )) {
            return result;
        }
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24 &=
            0xFFFFFFFBU;
        state.summon_runtime[summon_index] = 0U;
        replace_low_word(state.summon_packed, 0U);
        state.group_a_status_words[group_a_index] = 0U;
        state.summon_status = 0x80U;
        state.message_aux = 0U;
        state.current_actor_index = 0xFFFFU;
        replace_low_word(state.phase_counter, 0U);
        if (!rebuild_shared_actor_metrics(
                state, port, context.startup, result
            ) ||
            !rebuild_shared_actor_order(port, result)) {
            return result;
        }
        result.return_value = 1U;
        return result;
    }
    case 17U:
        if (state.result_mode == 0U) {
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 22U: {
        if ((state.action_runtime_flags & 0x8000U) == 0U) {
            const rendering::LegacyBlitClipRectangle clip{
                .left = context.raster.clip_left,
                .top = context.raster.clip_top,
                .width = context.raster.clip_width,
                .height = context.raster.clip_height,
            };
            result.status_indicator = advance_legacy_battle_status_indicator(
                state.status_indicator,
                context.framebuffer,
                clip,
                context.shared_request,
                context.shared_effects,
                context.jitter,
                context.action_updater,
                context.frame_provider,
                context.bounded_random,
                context.indicator_sound,
                context.status_indicator_action_eax_snapshot
            );
            ++result.status_indicator_calls;
            if (result.status_indicator.status ==
                LegacyBattleStatusIndicatorStatus::blit_typed_stop) {
                result.status = LegacyBattleActionDispatchStatus::
                    status_indicator_typed_stop;
                return result;
            }
            if (result.status_indicator.return_value != 1U) {
                return result;
            }
            state.active_actor_snapshot = 6U;
            const bool group_a_side = state.side_selection_word != 0U;
            const u32 live_base = kLegacyBattleActionGroupABaseToken;
            const std::size_t action_target_caller = group_a_side ? 1U : 0U;
            auto action_target_request =
                context.action_dispatch_action_target_requests
                    [action_target_caller];
            action_target_request.actor_token = live_base;
            action_target_request.entry_eax =
                result.status_indicator.return_value;
            action_target_request.entry_return_address =
                group_a_side ? 0x00454AEBU : 0x00454A42U;
            action_target_request.entry_flags =
                subtract_word_flags(state.side_selection_word, 0U);
            action_target_request.entry_flags_known = true;
            result.actor_action_target =
                query_legacy_battle_actor_action_target(
                    resolve_legacy_battle_actor_action_target(
                        {.action = &state, .startup = context.startup},
                        live_base
                    ),
                    action_target_request
                );
            result.actor_action_targets[result.actor_action_target_calls] =
                result.actor_action_target;
            ++result.actor_action_target_calls;
            if (result.actor_action_target.status !=
                LegacyBattleActorActionTargetStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_action_target_typed_stop;
                result.return_value = result.actor_action_target.return_eax;
                return result;
            }
            const u16 selected =
                low_word(result.actor_action_target.return_eax);
            const u32 selected_index =
                to_bits(static_cast<i32>(signed_low_word(selected)));
            const u32 times_three = selected_index + selected_index * 2U;
            const u32 times_twenty_four = times_three << 3U;
            const u32 times_twenty_three = times_twenty_four - selected_index;
            const u32 times_sixty_nine =
                times_twenty_three + times_twenty_three * 2U;
            const u32 times_three_hundred_forty_five =
                times_sixty_nine + times_sixty_nine * 4U;
            const u32 times_one_thousand_three_hundred_eighty_one =
                selected_index + times_three_hundred_forty_five * 4U;
            const u32 selected_actor_token =
                kLegacyBattleActionGroupBBaseToken +
                times_one_thousand_three_hundred_eighty_one * 8U;
            if (!execute_legacy_battle_actor_gate_decay_call(
                    result.actor_gate_decay,
                    context.actor_gate_decay_requests,
                    {.action = &state, .startup = context.startup},
                    group_a_side ? 0x00454B06U : 0x00454A5DU,
                    group_a_side ? 0x00454B0BU : 0x00454A62U,
                    selected_actor_token,
                    times_one_thousand_three_hundred_eighty_one,
                    times_three_hundred_forty_five,
                    subtract_flags(times_twenty_four, selected_index),
                    true,
                    context.actor_gate_decay_request_offset
                )) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_gate_decay_typed_stop;
                result.return_value = result.actor_gate_decay.last.return_eax;
                return result;
            }
            state.available_actor_count = 0;
            if (group_a_side) {
                state.side_mode = 1U;
                for (i32 index = 0; index < state.group_a_count; ++index) {
                    ++result.group_a_iterations;
                    if (index >= 10) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_a_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            kCallActorTerminal,
                            {group_a_token(static_cast<u32>(index))}
                        )
                            .eax != 1U) {
                        ++state.available_actor_count;
                    }
                }
                i32 first = 0;
                while (first < state.group_a_count) {
                    if (first >= 10) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_a_index_typed_stop;
                        return result;
                    }
                    const auto terminal = invoke(
                        state,
                        port,
                        kCallActorTerminal,
                        {group_a_token(static_cast<u32>(first))}
                    );
                    if (terminal.eax == 0U) {
                        if (!execute_legacy_battle_actor_target_selection_call(
                                result.actor_target_selection,
                                context.actor_target_selection_requests,
                                {.action = &state, .startup = context.startup},
                                0x00454B8DU,
                                0x00454B92U,
                                actor_token,
                                static_cast<u16>(first),
                                terminal.eax,
                                terminal.edx,
                                logical_flags(terminal.eax),
                                true,
                                context.actor_target_selection_request_offset
                            )) {
                            result.status = LegacyBattleActionDispatchStatus::
                                actor_target_selection_typed_stop;
                            result.return_value =
                                result.actor_target_selection.last.return_eax;
                            return result;
                        }
                        break;
                    }
                    ++first;
                }
            } else {
                for (i32 index = 0; index < state.group_b_count; ++index) {
                    ++result.group_b_iterations;
                    if (index >= 8) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            kCallActorTerminal,
                            {group_b_token(static_cast<u32>(index))}
                        )
                            .eax != 1U) {
                        ++state.available_actor_count;
                    }
                }
                i32 first = 0;
                while (first < state.group_b_count) {
                    if (first >= 8) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_index_typed_stop;
                        return result;
                    }
                    const auto terminal = invoke(
                        state,
                        port,
                        kCallActorTerminal,
                        {group_b_token(static_cast<u32>(first))}
                    );
                    if (terminal.eax == 0U) {
                        if (!execute_legacy_battle_actor_target_selection_call(
                                result.actor_target_selection,
                                context.actor_target_selection_requests,
                                {.action = &state, .startup = context.startup},
                                0x00454B8DU,
                                0x00454B92U,
                                actor_token,
                                static_cast<u16>(first),
                                terminal.eax,
                                terminal.edx,
                                logical_flags(terminal.eax),
                                true,
                                context.actor_target_selection_request_offset
                            )) {
                            result.status = LegacyBattleActionDispatchStatus::
                                actor_target_selection_typed_stop;
                            result.return_value =
                                result.actor_target_selection.last.return_eax;
                            return result;
                        }
                        break;
                    }
                    ++first;
                }
            }
            state.scene_value = 1U;
            const auto published_scene = invoke(
                state,
                port,
                kCallPublishScene,
                {0x5FDU, 0x004FE5D4U + 4U * group_a_index}
            );
            if (!set_actor_target_selection_latch(
                    context,
                    result,
                    actor_token,
                    0x00454BAEU,
                    0x00454BB3U,
                    published_scene.eax,
                    published_scene.edx,
                    published_scene.flags
                )) {
                return result;
            }
            state.action_runtime_flags |= 0x8000U;
            return result;
        }
        if ((state.action_runtime_flags & 1U) == 0U) {
            return result;
        }
        state.frame_effect.fade_active = 1U;
        result.return_value = 1U;
        return result;
    }
    case 23U: {
        if (!require_group_b()) {
            return result;
        }
        const u32 skip_primary =
            group_a_index < context.group_a_skip_primary.size()
            ? context.group_a_skip_primary[group_a_index]
            : 0U;
        result.action_twenty_three = advance_legacy_battle_action_twenty_three(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .opponent_token = group_b_token(group_b_index),
                .skip_primary = skip_primary,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
            }
        );
        ++result.action_twenty_three_calls;
        if (result.action_twenty_three.status !=
            LegacyBattleActionTwentyThreeStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_three_typed_stop;
            return result;
        }
        if (result.action_twenty_three.return_eax != 1U) {
            return result;
        }
        result.action_twenty_three_message =
            consume_legacy_battle_action_twenty_three_message(
                &state.group_a_action_execution[group_a_index],
                &state.group_b_message_profiles[group_b_index],
                port,
                {
                    .actor_token = actor_token,
                    .profile_token = group_b_token(group_b_index) + 0x0CU,
                }
            );
        ++result.action_twenty_three_message_calls;
        if (result.action_twenty_three_message.status !=
            LegacyBattleActionTwentyThreeMessageStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_three_message_typed_stop;
            return result;
        }
        const u16 message_code =
            low_word(result.action_twenty_three_message.return_eax);
        if (message_code != 0U && message_code < 0x61A8U) {
            const auto definition_result = load_legacy_battle_mon_definition(
                port.legacy_battle_mon_definition_scratch(),
                port.legacy_battle_mon_definition_scratch_description(),
                port,
                {
                    .path = "mon.dat",
                    .definition_id = message_code,
                }
            );
            if (legacy_battle_mon_definition_load_stopped(
                    definition_result.status
                )) {
                result.status = LegacyBattleActionDispatchStatus::
                    mon_definition_load_typed_stop;
                return result;
            }
            const auto release_result =
                release_legacy_battle_mon_definition_text(
                    port.legacy_battle_mon_definition_scratch(),
                    port.legacy_battle_mon_definition_scratch_description(),
                    port,
                    0x0053BC28U
                );
            if (legacy_battle_mon_definition_text_release_stopped(
                    release_result.status
                )) {
                result.status = LegacyBattleActionDispatchStatus::
                    mon_definition_release_typed_stop;
                return result;
            }
            static_cast<void>(
                invoke(state, port, kCallPlayMessage, {0x117U, 0x004AB784U})
            );
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U, 0xAU, 0x32U, 0x0053C16CU, 0x80000002U}
                )) {
                return result;
            }
            if (!publish_player_item_quantity(port, result, message_code, 1U)) {
                return result;
            }
        } else {
            static_cast<void>(
                invoke(state, port, kCallPlayMessage, {0x116U, 0x004AB784U})
            );
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U,
                     0xAU,
                     0x1EU,
                     message_code == 0x61A8U ? 0x004A77BCU : 0x004A77B0U,
                     0x80000002U}
                )) {
                return result;
            }
        }
        const auto& seeded_mode =
            context.actor_action_mode_requests[result.actor_action_mode_calls];
        u32 mode_eax = seeded_mode.entry_eax;
        u32 mode_edx = seeded_mode.entry_edx;
        if (message_code != 0U && message_code < 0x61A8U) {
            mode_eax = result.player_item.return_token;
        } else if (!result.text_messages.empty()) {
            mode_eax = result.text_messages.back().return_registers.eax;
            mode_edx = result.text_messages.back().return_registers.edx;
        }
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                0x12CU,
                mode_eax,
                mode_edx,
                0x00455CBBU,
                seeded_mode.entry_flags,
                false
            )) {
            return result;
        }
        return result;
    }
    case 24U: {
        if (!require_group_b()) {
            return result;
        }
        result.action_twenty_four = advance_legacy_battle_action_twenty_four(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {.actor_token = actor_token}
        );
        ++result.action_twenty_four_calls;
        if (result.action_twenty_four.status !=
            LegacyBattleActionTwentyFourStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_twenty_four_typed_stop;
            return result;
        }
        replace_low_word(
            state.packed_action_state,
            low_word(result.action_twenty_four.return_eax)
        );
        if ((low_word(state.packed_action_state) & 0x8000U) != 0U) {
            for (i32 index = 0; index < state.group_b_count; ++index) {
                ++result.group_b_iterations;
                if (index >= 8) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop;
                    return result;
                }
                const u32 index_u32 = static_cast<u32>(index);
                if (invoke(
                        state,
                        port,
                        kCallActorTerminal,
                        {group_b_token(index_u32)}
                    )
                        .eax == 1U) {
                    continue;
                }
                port.actor_metric_state().group_b_order[index_u32] = index_u32;
                reply = invoke(
                    state,
                    port,
                    kCallComputeValue,
                    {group_b_token(index_u32),
                     state.selection_word,
                     state.selection_high_word}
                );
                i32 value = signed_low_word(reply.eax);
                if (value >= 0x270F) {
                    value = 0x270F;
                }
                state.signed_action_value = value;
                port.battle_pair_primary_value() += static_cast<u32>(value);
                if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                        {
                            .action = &state,
                            .startup = context.startup,
                        },
                        result.actor_field_26b8_high_bit_set,
                        context.actor_field_26b8_high_bit_set_requests,
                        group_b_token(index_u32),
                        0x004541C3U
                    )) {
                    result.status = LegacyBattleActionDispatchStatus::
                        actor_field_26b8_high_bit_set_typed_stop;
                    result.return_value =
                        result.actor_field_26b8_high_bit_set.last.return_eax;
                    return result;
                }
                static_cast<void>(invoke(
                    state,
                    port,
                    kCallPublishSignedValue,
                    {port.battle_pair_primary_value()}
                ));
                static_cast<void>(invoke(state, port, 0x0047CEC0U, {1U}));
                if (state.blocking_effect == 0U &&
                    invoke(
                        state,
                        port,
                        kCallCommitVisual,
                        {port.battle_pair_primary_value(), 0U, 0U}
                    )
                            .eax == 1U) {
                    context.screen_flash.active = 1U;
                    state.selected_target_index = static_cast<u16>(index);
                    state.selected_group_b_identity[index_u32] = index_u32;
                    if (!clear_framebuffer(context, result)) {
                        return result;
                    }
                }
                port.battle_pair_primary_value() = 0U;
            }
            state.message_aux = low_word(state.packed_action_state) & 0x7FFFU;
        }
        if (low_word(state.packed_action_state) == 2U) {
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        replace_low_word(state.packed_action_state, 0U);
        return result;
    }
    case 25U:
        if (!require_group_b()) {
            return result;
        }
        if (state.stored_group_b_index == 0xFFFFU) {
            result.action_twenty_five_ready =
                query_legacy_battle_action_twenty_five_ready(
                    &state.group_b_message_profiles[group_b_index]
                );
            ++result.action_twenty_five_ready_calls;
            if (result.action_twenty_five_ready.status !=
                LegacyBattleActionTwentyFiveReadyStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    action_twenty_five_ready_typed_stop;
                return result;
            }
            if (result.action_twenty_five_ready.return_eax == 1U) {
                if (!publish_text_message(
                        context,
                        port,
                        result,
                        {0x118U, 0xAU, 0x32U, 0x004A77A4U, 0x80000002U}
                    )) {
                    return result;
                }
                const auto mode_reply =
                    invoke(state, port, kCallSetGlobalMode, {1U});
                state.stored_group_b_index = static_cast<u16>(group_b_index);
                state.stored_group_a_index = static_cast<u16>(group_a_index);
                if (!execute_legacy_battle_actor_runtime_reset_call(
                        {.action = &state, .startup = context.startup},
                        context.bounded_random,
                        result.actor_runtime_reset,
                        context.actor_runtime_reset_requests,
                        group_b_token(group_b_index),
                        mode_reply.eax,
                        mode_reply.edx,
                        0x00454FAFU,
                        0x00454FB4U
                    )) {
                    result.status = LegacyBattleActionDispatchStatus::
                        actor_runtime_reset_typed_stop;
                    result.return_value =
                        result.actor_runtime_reset.last.return_eax;
                    return result;
                }
                if (!remove_attack_order_entry(
                        context, result, group_b_index
                    )) {
                    return result;
                }
                state.current_actor_index = 0xFFFFU;
            } else {
                if (!publish_text_message(
                        context,
                        port,
                        result,
                        {0x118U, 0xAU, 0x32U, 0x004A7798U, 0x80000002U}
                    )) {
                    return result;
                }
            }
            result.return_value = 1U;
            return result;
        }
        if (state.stored_group_b_index >= state.group_b_status_words.size()) {
            result.status =
                LegacyBattleActionDispatchStatus::target_table_typed_stop;
            return result;
        }
        state.choice_cursor = state.choice_state + 1U;
        state.choice_commit = 1U;
        state.group_b_status_words[state.stored_group_b_index] = 0x8000U;
        if (state.message_gate != 0U) {
            state.group_b_status_words[state.stored_group_b_index] = 0x4000U;
            const u32 object_token = group_b_token(state.stored_group_b_index);
            LegacyBattleActorGroupBElementState* actor = nullptr;
            if (context.startup != nullptr &&
                context.startup->group_b_lifecycle != nullptr &&
                state.stored_group_b_index <
                    context.startup->group_b_lifecycle->size()) {
                actor = &(
                    *context.startup->group_b_lifecycle
                )[state.stored_group_b_index];
            }
            ActionCompositionPortAdapter adapter(port);
            result.group_b_action_composition =
                compose_legacy_battle_group_b_action(
                    actor,
                    &port.battle_message_state(),
                    adapter,
                    port,
                    {
                        .definition_argument = state.message_gate,
                        .actor_token = object_token,
                        .output_token = 0x0053BD40U,
                        .entry_eax = object_token,
                        .entry_ecx = object_token,
                        .entry_edx = object_token,
                        .action_mode_request =
                            context.actor_action_mode_requests
                                [result.actor_action_mode_calls],
                    }
                );
            ++result.group_b_action_composition_calls;
            if (result.group_b_action_composition.mode_update_calls != 0U) {
                append_nested_actor_action_mode(
                    result, result.group_b_action_composition.actor_action_mode
                );
            }
            if (result.group_b_action_composition.status !=
                LegacyBattleGroupBActionCompositionStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    group_b_action_composition_typed_stop;
                result.return_value =
                    result.group_b_action_composition.return_eax;
                return result;
            }
            state.message_gate = 0U;
        }
        if (state.message_aux != 0U) {
            state.group_b_status_words[state.stored_group_b_index] = 0x8000U;
            const u32 object_token = group_b_token(state.stored_group_b_index);
            LegacyBattleActorGroupBElementState* actor = nullptr;
            if (context.startup != nullptr &&
                context.startup->group_b_lifecycle != nullptr &&
                state.stored_group_b_index <
                    context.startup->group_b_lifecycle->size()) {
                actor = &(
                    *context.startup->group_b_lifecycle
                )[state.stored_group_b_index];
            }
            result.group_b_action_profile_selection =
                select_legacy_battle_group_b_action_profile(
                    actor,
                    &port.battle_message_state(),
                    port,
                    {
                        .selector_argument = 0U,
                        .output_token = 0x0053BD40U,
                        .actor_token = object_token,
                        .action_mode_requests = {
                            context.actor_action_mode_requests
                                [result.actor_action_mode_calls],
                            context.actor_action_mode_requests
                                [result.actor_action_mode_calls],
                        },
                    }
                );
            ++result.group_b_action_profile_selection_calls;
            if (result.group_b_action_profile_selection.mode_update_calls !=
                0U) {
                append_nested_actor_action_mode(
                    result,
                    result.group_b_action_profile_selection.actor_action_mode
                );
            }
            if (result.group_b_action_profile_selection.status !=
                LegacyBattleGroupBActionProfileSelectionStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    group_b_action_profile_selection_typed_stop;
                result.return_value =
                    result.group_b_action_profile_selection.return_eax;
                return result;
            }
        }
        state.group_b_status_words[state.stored_group_b_index] |=
            static_cast<u16>(state.choice_cursor - 1U);
        result.attack_order = append_legacy_battle_attack_order_entry(
            context.attack_order_records,
            2U,
            state.stored_group_b_index,
            state.choice_cursor - 1U,
            0U
        );
        ++result.attack_order_calls;
        if (result.attack_order.status !=
            LegacyBattleAttackOrderEntryStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::attack_order_typed_stop;
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 26U: {
        state.phase_condition = 1U;
        if (low_word(state.scan_push_state) == 1U) {
            static_cast<void>(invoke(state, port, kCallPushState, {0x40U}));
        }
        result.scale_scan = draw_legacy_battle_scale_scan(
            state.scale_scan,
            context.framebuffer,
            context.shared_request,
            context.shared_effects,
            context.jitter,
            context.frame_provider,
            0x140,
            0xC8
        );
        ++result.scale_scan_calls;
        if (result.scale_scan.status !=
            LegacyBattleScaleScanStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::scale_scan_typed_stop;
            return result;
        }
        const auto finish_scan = [&](const u32 return_address) {
            const auto popped = invoke(state, port, kCallPopState, {0x40U});
            if (!apply_legacy_battle_actor_action_mode_call(
                    state,
                    context,
                    result,
                    actor_token,
                    0x12CU,
                    popped.eax,
                    popped.edx,
                    return_address,
                    popped.flags
                )) {
                return false;
            }
            state.scan_word = 0U;
            state.phase_condition = 0U;
            replace_low_word(state.scan_dialog_state, 0U);
            replace_low_word(state.scan_push_state, 0U);
            return true;
        };
        if (result.scale_scan.return_value == 1U &&
            (state.scan_runtime & 0x8000U) == 0U) {
            if (!finish_scan(0x00455053U)) {
                return result;
            }
            return result;
        }
        if ((state.scan_runtime & 1U) == 0U) {
            return result;
        }
        state.scan_runtime = 0x8000U;
        if (low_word(state.scan_dialog_state) == 0U) {
            if (state.scan_word != 0U) {
                state.scan_word = 0U;
                static_cast<void>(
                    invoke(state, port, kCallPlayMessage, {0x2CU, 0x004AB784U})
                );
                static_cast<void>(invoke(state, port, kCallPopState, {0x40U}));
            } else {
                replace_low_word(state.scan_dialog_state, 1U);
                state.scan_word = 0U;
            }
        }
        if (!require_group_b()) {
            return result;
        }
        if (!begin_action(group_b_token(group_b_index))) {
            return result;
        }
        state.scan_runtime &= 0xFFFU;
        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index)) {
            return result;
        }
        state.phase_condition = 0U;
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            port.battle_pair_primary_value() = 0xFFFFFFFFU;
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            context.screen_flash.active = 1U;
            static_cast<void>(invoke(state, port, kCallSetScreenMode, {1U}));
            if (!clear_framebuffer(context, result)) {
                return result;
            }
            replace_low_word(state.scan_push_state, 0x8000U);
        }
        port.battle_pair_primary_value() = 0U;
        static_cast<void>(invoke(state, port, kCallPushState, {0x40U}));
        if ((state.scan_push_state & 0x8000U) != 0U &&
            !finish_scan(0x004551C8U)) {
            return result;
        }
        return result;
    }
    case 27U:
        if (!require_group_b()) {
            return result;
        }
        result.action_twenty_seven = advance_legacy_battle_action_twenty_seven(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = group_a_token(group_a_index),
                .target_token = group_b_token(group_b_index),
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .actor_field_26b8_high_bit_set_requests =
                    context.actor_field_26b8_high_bit_set_requests,
            }
        );
        ++result.action_twenty_seven_calls;
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            result.action_twenty_seven.actor_field_26b8_high_bit_set
        );
        if (result.action_twenty_seven.status !=
            LegacyBattleActionTwentySevenStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_seven_typed_stop;
            return result;
        }
        if (result.action_twenty_seven.return_eax != 1U) {
            return result;
        }
        if (context.scripted_resource_release_test_compat) {
            state.computed_selection_word =
                low_word(invoke(
                             state,
                             port,
                             kCallComputeSelection,
                             {4U, state.selection_context}
                )
                             .eax);
        } else {
            if (!release_actor_resource()) {
                return result;
            }

            state.computed_selection_word =
                result.actor_resource_release.output_word;
        }
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            context.screen_flash.active = 1U;
            if (!clear_framebuffer(context, result)) {
                return result;
            }
        }
        port.battle_pair_primary_value() = 0U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 28U:
    case 29U:
    case 32U: {
        const u32 required_action = action == 29U ? 0x1791U : 0x1965U;
        LegacyBattleGroupAActionExecutionState* action_actor =
            &state.group_a_action_execution[group_a_index];
        LegacyBattleTargetPhaseState* action_phase =
            &state.group_a_target_phases[group_a_index];
        u32 object_token = actor_token;
        if (action == 29U) {
            if (!require_group_b()) {
                return result;
            }
            auto& owned_phase =
                state.group_b_target_phases[group_b_index][group_a_index];
            if (owned_phase == nullptr) {
                owned_phase = std::make_unique<LegacyBattleTargetPhaseState>();
            }
            LegacyBattleActorGroupBElementState* group_b_owner =
                context.startup == nullptr ||
                    context.startup->group_b_lifecycle == nullptr
                ? nullptr
                : &(*context.startup->group_b_lifecycle)[group_b_index];
            owned_phase->borrowed_mode_flags = group_b_owner == nullptr
                ? nullptr
                : &group_b_owner->action_composition.mode_flags;
            owned_phase->borrowed_resource_token = group_b_owner == nullptr
                ? nullptr
                : &group_b_owner->action_execution.target_phase_resource_token;
            action_actor = group_b_owner == nullptr
                ? nullptr
                : &group_b_owner->action_execution;
            action_phase = owned_phase.get();
            object_token = group_b_token(group_b_index);
        }
        result.dual_record_action = advance_legacy_battle_dual_record_action(
            action_phase,
            action_actor,
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = object_token,
                .coordinate_token = object_token,
                .secondary_action_id = required_action,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .coordinate_output_x_initial =
                    state.dual_record_coordinate_x_initial,
                .coordinate_frame_header_residue =
                    state.dual_record_coordinate_frame_header_residue,
            }
        );
        ++result.dual_record_action_calls;
        if (result.dual_record_action.status !=
            LegacyBattleDualRecordActionStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::dual_record_action_typed_stop;
            return result;
        }
        if (result.dual_record_action.return_eax != 1U) {
            return result;
        }
        state.temporary_record.fill(0U);
        const u16 percent =
            low_word(invoke(state, port, kCallQueryPercent, {action}).eax);
        state.temporary_record_flags = action == 28U ? 0x10000000U
            : action == 29U                          ? 0x08000000U
                                                     : 0x02000000U;
        state.temporary_record_mode =
            static_cast<compat::u8>((4U * percent) / 100U + 2U);
        static_cast<void>(invoke(
            state,
            port,
            kCallCommitTemporaryRecord,
            {state.temporary_record_flags, state.temporary_record_mode}
        ));
        if (action == 29U) {
            if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                    {
                        .action = &state,
                        .startup = context.startup,
                    },
                    result.actor_field_26b8_high_bit_set,
                    context.actor_field_26b8_high_bit_set_requests,
                    group_b_token(group_b_index),
                    0x0045446EU
                )) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_field_26b8_high_bit_set_typed_stop;
                result.return_value =
                    result.actor_field_26b8_high_bit_set.last.return_eax;
                return result;
            }
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    }
    case 31U: {
        port.battle_message_state() = 0U;
        if (state.message_gate == 0U) {
            state.message_gate = 0x80000000U;
            rendering::initialize_legacy_countdown(
                state.countdown,
                context.countdown_flags,
                {
                    .minutes = 0,
                    .seconds = 5,
                    .primary_transition_value = 0U,
                    .mode = 1,
                }
            );
            u32 coordinate_x = state.message_coordinate_x;
            u32 coordinate_y = state.message_coordinate_y;
            ++result.coordinate_query_calls;
            result.coordinate_query = query_coordinates(
                {
                    .action = context.shared_action_dispatch,
                    .startup = context.startup,
                },
                group_b_token(group_b_index),
                coordinate_x,
                coordinate_y,
                0x0053BF50U,
                0x0053BF52U,
                group_b_index * 345U,
                reply.edx,
                subtract_flags(group_b_index * 24U, group_b_index)
            );
            if (result.coordinate_query.output_writes >= 1U) {
                state.message_coordinate_x = low_word(coordinate_x);
            }
            if (result.coordinate_query.output_writes >= 2U) {
                state.message_coordinate_y = low_word(coordinate_y);
            }
            if (result.coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_coordinate_typed_stop;
                return result;
            }
            static_cast<void>(clear_legacy_battle_action_record(
                state.persistent_action_record
            ));
            ++result.action_record_clear_calls;
        }
        bool escape_pressed{};
        if (!query_internal_flag(
                context.internal_flags, 0x4BU, escape_pressed
            )) {
            result.status =
                LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
            return result;
        }
        if (escape_pressed) {
            if (!clear_internal_flag(context.internal_flags, 0x4BU)) {
                result.status =
                    LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
                return result;
            }
            state.message_gate = 0U;
            state.message_aux = 0U;
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        if ((state.message_gate & 1U) == 0U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, kCallPlayMessage, {0x2EU, 0x004AB784U})
        );
        static_cast<void>(
            clear_legacy_battle_action_record(state.persistent_action_record)
        );
        ++result.action_record_clear_calls;
        state.message_gate = 0x80000000U;
        state.message_aux = 1U;
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            kCallComputeValue,
            {group_b_token(group_b_index),
             state.selection_word,
             state.selection_high_word}
        );
        port.battle_pair_primary_value() =
            static_cast<u32>(signed_low_word(reply.eax));
        if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                {
                    .action = &state,
                    .startup = context.startup,
                },
                result.actor_field_26b8_high_bit_set,
                context.actor_field_26b8_high_bit_set_requests,
                group_b_token(group_b_index),
                0x00454D86U
            )) {
            result.status = LegacyBattleActionDispatchStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            result.return_value =
                result.actor_field_26b8_high_bit_set.last.return_eax;
            return result;
        }
        static_cast<void>(invoke(
            state,
            port,
            kCallPublishSignedValue,
            {port.battle_pair_primary_value()}
        ));
        static_cast<void>(invoke(state, port, 0x0047CEC0U, {1U}));
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            if (!clear_internal_flag(context.internal_flags, 0x4AU)) {
                result.status =
                    LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
                return result;
            }
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            context.screen_flash.active = 1U;
            if (!clear_framebuffer(context, result)) {
                return result;
            }
            port.battle_pair_primary_value() = 0U;
            state.message_gate = 0U;
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        port.battle_pair_primary_value() = 0U;
        return result;
    }
    case 33U:
        if (!require_group_b()) {
            return result;
        }
        result.target_ready = advance_legacy_battle_target_ready(
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .target_token = group_b_token(group_b_index),
                .unused_argument = 0x1791U,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .actor_field_26b8_high_bit_set_requests =
                    context.actor_field_26b8_high_bit_set_requests,
            }
        );
        ++result.target_ready_calls;
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            result.target_ready.actor_field_26b8_high_bit_set
        );
        if (result.target_ready.status !=
            LegacyBattleTargetReadyStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::target_ready_typed_stop;
            return result;
        }
        if (result.target_ready.return_eax != 1U) {
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        reply = invoke(
            state, port, kCallResolveTarget, {group_b_token(group_b_index)}
        );
        if (reply.eax == 0U) {
            result.status =
                LegacyBattleActionDispatchStatus::target_object_typed_stop;
            return result;
        }
        if ((reply.object_flags & 0x20U) != 0U) {
            result.return_value = 1U;
            return result;
        }
        reply = invoke(state, port, kCallQueryPercent, {0x21U});
        result.target_property_chance =
            check_legacy_battle_target_property_chance(
                context.bounded_random, {.value = low_word(reply.eax)}
            );
        ++result.target_property_chance_calls;
        if (result.target_property_chance.return_eax == 1U) {
            const auto presentation_mode =
                invoke(state, port, kCallSetMode, {7U});
            const u32 group_b_actor_token = group_b_token(group_b_index);
            if (!activate_actor_presentation(
                    state,
                    context,
                    result,
                    group_b_actor_token,
                    presentation_mode.eax,
                    presentation_mode.edx,
                    0x0045472EU,
                    0x00454733U,
                    presentation_mode.flags
                )) {
                return result;
            }
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            context.screen_flash.active = 1U;
            if (!clear_framebuffer(context, result)) {
                return result;
            }
        }
        result.return_value = 1U;
        return result;
    case 34U:
    case 35U:
    case 36U:
        result.dual_record_action = advance_legacy_battle_dual_record_action(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .coordinate_token = actor_token,
                .secondary_action_id = 0x17BAU,
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .coordinate_output_x_initial =
                    state.dual_record_coordinate_x_initial,
                .coordinate_frame_header_residue =
                    state.dual_record_coordinate_frame_header_residue,
            }
        );
        ++result.dual_record_action_calls;
        if (result.dual_record_action.status !=
            LegacyBattleDualRecordActionStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::dual_record_action_typed_stop;
            return result;
        }
        if (result.dual_record_action.return_eax != 1U) {
            return result;
        }
        reply = invoke(
            state,
            port,
            kCallComputeValue,
            {actor_token, state.selection_word, state.selection_high_word}
        );
        state.signed_action_value = signed_low_word(reply.eax);
        if (action == 34U) {
            static_cast<void>(invoke(
                state,
                port,
                kCallPublishSignedValue,
                {static_cast<u32>(state.signed_action_value)}
            ));
            static_cast<void>(invoke(state, port, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state,
                port,
                kCallCommitVisual,
                {static_cast<u32>(state.signed_action_value), 0U, 0U}
            ));
            state.signed_action_value = 0;
        } else if (action == 35U) {
            static_cast<void>(invoke(
                state,
                port,
                kCallPublishSignedValue,
                {static_cast<u32>(static_cast<i16>(state.selection_word))}
            ));
            static_cast<void>(invoke(state, port, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state, port, kCallCommitVisual, {0U, state.selection_word, 0U}
            ));
            state.selection_word = 0U;
        } else {
            static_cast<void>(invoke(
                state,
                port,
                kCallPublishSignedValue,
                {static_cast<u32>(static_cast<i16>(state.selection_high_word))}
            ));
            static_cast<void>(invoke(state, port, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state,
                port,
                kCallCommitVisual,
                {0U, 0U, state.selection_high_word}
            ));
            state.selection_high_word = 0U;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    default:
        return result;
    }
}

}  // namespace openswd3::battle::action_dispatch_detail
