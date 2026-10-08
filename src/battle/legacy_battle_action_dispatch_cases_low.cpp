#include "legacy_battle_action_dispatch_internal.hpp"

#include <stdexcept>

namespace openswd3::battle::action_dispatch_detail {

LegacyBattleActionDispatchResult ActionDispatchRunner::dispatch_low() {
    switch (action) {
    case 1U: {
        if (!require_group_b()) {
            return result;
        }
        const u32 target_token = state.side_mode != 0U
            ? group_a_token(group_b_index)
            : group_b_token(group_b_index);
        if (!begin_action(target_token)) {
            return result;
        }
        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index)) {
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
                state.selected_target_index = static_cast<u16>(group_b_index);
                state.selected_group_b_identity[group_b_index] = group_b_index;
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
                        state.side_mode == 0U ? 0x00453B15U : 0x00453C93U,
                        logical_flags(framebuffer_bytes & 3U)
                    )) {
                    return result;
                }
            }
        }

        if (state.side_mode != 0U) {
            result.pair_transition = advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = actor_token,
                    .secondary_object_token = group_b_token(group_b_index),
                    .effect_resource_slot_write_owners =
                        {
                            .action = &state,
                            .startup = context.startup,
                        },
                    .effect_resource_slot_write_requests =
                        context.effect_resource_slot_write_requests,
                }
            );
            ++result.pair_transition_calls;
            result.port_calls += result.pair_transition.port_calls;
            append_legacy_battle_actor_effect_resource_slot_write_trace(
                result.effect_resource_slot_write,
                result.pair_transition.effect_resource_slot_write
            );
            if (result.pair_transition.status !=
                LegacyBattlePairTransitionStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_effect_resource_slot_write_typed_stop;
                result.return_value = result.pair_transition.return_eax;
                return result;
            }
        }
        const u16 actor_class = low_word(
            invoke(state, port, result, kCallQueryActorClass, {actor_token}).eax
        );
        if (actor_class == 8U) {
            const u16 percent = low_word(
                invoke(state, port, result, kCallQueryPercent, {0x38U}).eax
            );
            state.signed_action_value = 0;
            const u32 base = (10U * port.battle_pair_primary_value()) / 100U;
            if (state.side_mode != 0U) {
                port.battle_pair_primary_value() = base +
                    (static_cast<u32>(percent) *
                     port.battle_pair_primary_value()) /
                        100U;
            } else {
                const u32 percentage_term =
                    (static_cast<u32>(percent) *
                     (port.battle_pair_primary_value() - base)) /
                    100U;
                port.battle_pair_primary_value() = base + percentage_term;
                if (!execute_legacy_battle_actor_effect_resource_slot_write_call(
                        {
                            .action = &state,
                            .startup = context.startup,
                        },
                        result.effect_resource_slot_write,
                        context.effect_resource_slot_write_requests,
                        actor_token,
                        0x246FU,
                        0x51EB851FU,
                        port.battle_pair_primary_value(),
                        0x00453B85U,
                        0x00453B8AU,
                        add_flags(percentage_term, base)
                    )) {
                    result.status = LegacyBattleActionDispatchStatus::
                        actor_effect_resource_slot_write_typed_stop;
                    result.return_value =
                        result.effect_resource_slot_write.last.return_eax;
                    return result;
                }
                port.battle_pair_primary_value() =
                    0U - port.battle_pair_primary_value();
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallPublishSignedValue,
                    {port.battle_pair_primary_value()}
                ));
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallCommitVisual,
                    {port.battle_pair_primary_value(), 0U, 0U}
                ));
                static_cast<void>(
                    invoke(state, port, result, 0x0047CF00U, {8U})
                );
                static_cast<void>(
                    invoke(state, port, result, 0x0047CEC0U, {1U})
                );
                synchronize_legacy_battle_actor_effect_resource_cursor_update(
                    {
                        .action = &state,
                        .startup = context.startup,
                    },
                    actor_token,
                    1U
                );
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallSetActorAction,
                {0x004B8A00U, 0x38U, 5U}
            ));
        }
        if (state.side_mode == 0U) {
            result.pair_transition = advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = actor_token,
                    .secondary_object_token = target_token,
                    .effect_resource_slot_write_owners =
                        {
                            .action = &state,
                            .startup = context.startup,
                        },
                    .effect_resource_slot_write_requests =
                        context.effect_resource_slot_write_requests,
                }
            );
            ++result.pair_transition_calls;
            result.port_calls += result.pair_transition.port_calls;
            append_legacy_battle_actor_effect_resource_slot_write_trace(
                result.effect_resource_slot_write,
                result.pair_transition.effect_resource_slot_write
            );
            if (result.pair_transition.status !=
                LegacyBattlePairTransitionStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_effect_resource_slot_write_typed_stop;
                result.return_value = result.pair_transition.return_eax;
                return result;
            }
        }
        port.battle_pair_primary_value() = 0U;
        state.selection_high_word = 0U;
        state.selection_word = 0U;
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                0x12CU,
                0U,
                reply.edx,
                0x00455CBBU,
                logical_flags(0U)
            )) {
            return result;
        }
        return result;
    }
    case 2U:
    case 3U: {
        if ((state.action_runtime_flags & 0x8000U) == 0U) {
            replace_low_word(
                state.action_runtime_flags,
                static_cast<u16>(state.action_runtime_flags | 0x8000U)
            );
            state.active_actor_snapshot = low_word(
                invoke(state, port, result, kCallQuerySelection, {actor_token})
                    .eax
            );
            state.target_identity.fill(0xFFFFFFFFU);
            replace_low_word(state.input_mode, 1U);
            state.selection_workspace.fill(0U);
            if (action == 2U &&
                invoke(state, port, result, kCallQuerySpecial, {actor_token})
                        .eax == 1U) {
                state.deformation_active = true;
                reply = invoke(state, port, result, 0x00489E90U, {0x2CU});
                state.deformation_owner_token = reply.eax;
                if (reply.eax != 0U) {
                    try {
                        state.deformation = std::make_unique<
                            asset_runtime::LegacyDeformationNode>(
                            asset_runtime::LegacyDeformationConfiguration{
                                .framebuffer_width = 640U,
                                .framebuffer_height = 480U,
                                .origin_x = 0,
                                .origin_y = 0,
                                .field_width = 200U,
                                .field_height = 200U,
                            }
                        );
                    } catch (...) {
                        static_cast<void>(invoke(
                            state,
                            port,
                            result,
                            0x00489D00U,
                            {state.deformation_owner_token}
                        ));
                        state.deformation_owner_token = 0U;
                        state.deformation_active = false;
                        throw;
                    }
                }
            }
            const i32 side_count = state.side_mode != 0U ? state.group_a_count
                                                         : state.group_b_count;
            if (state.available_actor_count > side_count) {
                state.available_actor_count = side_count;
            }
        }

        if ((state.action_runtime_flags & 1U) == 0U) {
            return result;
        }
        port.battle_pair_primary_value() = 0U;
        state.selection_word = 0U;
        state.selection_high_word = 0U;
        if (action == 2U) {
            if ((port.battle_debug_hotkey_state().battle_mode_flags_53bc24 &
                 0x20U) != 0U) {
                return result;
            }
            if (state.deformation_active) {
                state.deformation.reset();
                if (state.deformation_owner_token != 0U) {
                    static_cast<void>(invoke(
                        state,
                        port,
                        result,
                        0x00489D00U,
                        {state.deformation_owner_token}
                    ));
                }
            }
            state.deformation_owner_token = 0U;
            state.deformation_active = false;
        } else if (context.scripted_resource_release_test_compat) {
            reply = invoke(
                state,
                port,
                result,
                kCallComputeSelection,
                {state.selection_source, state.selection_context}
            );
            state.computed_selection_word = low_word(reply.eax);
        } else {
            if (!release_actor_resource()) {
                return result;
            }

            state.computed_selection_word =
                result.actor_resource_release.output_word;
        }
        state.frame_effect.fade_active = 1U;
        const auto& seeded_mode =
            context.actor_action_mode_requests[result.actor_action_mode_calls];
        const u32 mode_edx = action == 3U
            ? (context.scripted_resource_release_test_compat
                   ? reply.edx
                   : result.actor_resource_release.return_edx)
            : seeded_mode.entry_edx;
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                0x12CU,
                action == 3U ? 0U : seeded_mode.entry_eax,
                mode_edx,
                0x00453F8BU,
                action == 3U ? logical_flags(0U) : seeded_mode.entry_flags,
                action == 3U || seeded_mode.entry_flags_known
            )) {
            return result;
        }
        result.retreat_commit = commit_legacy_battle_retreat(
            {
                .packed_actor_counter = state.packed_actor_counter,
                .selection_gate = state.action_pending_aux,
                .selection_cache_gate = state.selection_cache_gate_b,
                .resolution_latch = state.resolution_latch,
                .text_messages = context.text_messages,
                .text_message_head = context.startup_reset == nullptr
                    ? nullptr
                    : &context.startup_reset->block_5214f8[0U],
                .group_a_actions = state.group_a_action_execution,
            },
            port,
            group_a_index
        );
        ++result.retreat_commit_calls;
        result.port_calls += result.retreat_commit.port_calls;
        if (result.retreat_commit.status !=
            LegacyBattleRetreatCommitStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::text_message_typed_stop;
        }
        return result;
    }
    case 4U: {
        if (!require_group_b()) {
            return result;
        }
        if (context.startup == nullptr ||
            group_a_index >= context.startup->party.size()) {
            result.status =
                LegacyBattleActionDispatchStatus::action_four_effect_typed_stop;
            return result;
        }
        result.action_four_effect = advance_legacy_battle_action_four_effect(
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
            result.status =
                LegacyBattleActionDispatchStatus::action_four_effect_typed_stop;
            return result;
        }
        reply.eax = result.action_four_effect.return_eax;
        reply.ecx = result.action_four_effect.return_ecx;
        reply.edx = result.action_four_effect.return_edx;
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
                state.selected_target_index = static_cast<u16>(group_b_index);
                state.selected_group_b_identity[group_b_index] = group_b_index;
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
                if (!update_effect_score(state, result, group_a_index, 5U)) {
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
    case 5U:
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 6U: {
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) > 1U) {
            replace_low_word(
                state.phase_counter,
                static_cast<u16>(low_word(state.phase_counter) - 1U)
            );
            if (low_word(state.phase_counter) != 2U) {
                return result;
            }
            const u32 message_id =
                state.phase_condition == 1U ? 0x117U : 0x116U;
            const u32 text_token =
                state.phase_condition == 1U ? 0x004A77F0U : 0x004A77E4U;
            static_cast<void>(invoke(
                state, port, result, kCallPlayMessage, {message_id, 0x004AB784U}
            ));
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U, 0xAU, 0x28U, text_token, 0x80000002U}
                )) {
                return result;
            }
            state.frame_effect.primary_suppression = 0U;
            state.frame_effect.red_factor = 0;
            state.frame_effect.green_factor = 0;
            state.frame_effect.blue_factor = 0;
            state.current_actor_index = 0xFFFFU;
            replace_low_word(state.phase_counter, 0U);
            state.selected_target_index = 0xFFFFU;
            state.phase_condition_aux = 0U;
            state.frame_effect.fade_active = 1U;
            const u32 low_byte = state.packed_actor_counter & 0xFFU;
            const u32 third_byte = (state.packed_actor_counter >> 16U) & 0xFFU;
            if (low_byte - third_byte >=
                static_cast<u32>(state.group_b_count)) {
                state.phase_terminal = 0U;
                port.battle_message_state() = 0x63U;
            }
            result.return_value = 1U;
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            const auto target_code_reply = invoke(
                state,
                port,
                result,
                kCallQueryTargetCode,
                {group_b_token(group_b_index)}
            );
            const i16 target_code = signed_low_word(target_code_reply.eax);
            result.fixed_count_lookup = lookup_legacy_battle_fixed_count(
                port.legacy_battle_fixed_object_state(),
                {
                    .key = static_cast<u32>(static_cast<u16>(target_code)),
                    .entry_eax = target_code_reply.eax,
                    .entry_ecx = target_code_reply.ecx,
                    .entry_edx = target_code_reply.edx,
                }
            );
            ++result.fixed_count_lookup_calls;
            if (result.fixed_count_lookup.status !=
                LegacyBattleFixedCountStatus::completed) {
                result.status =
                    LegacyBattleActionDispatchStatus::fixed_count_typed_stop;
                return result;
            }
            const u16 distance = low_word(result.fixed_count_lookup.return_eax);
            if (distance >= 0x14U) {
                state.phase_condition_aux = 1U;
            }
            result.target_phase_check = check_legacy_battle_target_phase(
                &state.group_a_action_execution[group_a_index],
                &state.group_b_message_profiles[group_b_index],
                port,
                {.target_token = group_b_token(group_b_index)}
            );
            ++result.target_phase_check_calls;
            result.port_calls += result.target_phase_check.value_query_calls +
                result.target_phase_check.random_calls;
            if (result.target_phase_check.status !=
                LegacyBattleTargetPhaseCheckStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_check_typed_stop;
                return result;
            }
            if (result.target_phase_check.return_eax == 1U) {
                state.phase_condition_aux = 1U;
            } else if (state.phase_condition_aux != 1U) {
                state.phase_condition = 0U;
                replace_low_word(state.phase_counter, 0x28U);
                return result;
            }
            if (context.startup == nullptr) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_start_typed_stop;
                return result;
            }
            auto target_phase_start_request =
                context.target_phase_start_request;
            target_phase_start_request.target_token =
                group_b_token(group_b_index);
            target_phase_start_request.surface_width =
                context.raster.surface.width;
            target_phase_start_request.surface_height =
                context.raster.surface.height;
            target_phase_start_request.coordinate_output_x_token =
                state.base_coordinate_output_x_token;
            target_phase_start_request.coordinate_output_y_token =
                state.base_coordinate_output_y_token;
            target_phase_start_request.entry_eax =
                result.target_phase_check.return_eax;
            target_phase_start_request.entry_ecx = actor_token;
            target_phase_start_request.entry_ebp =
                target_phase_start_request.target_token;
            target_phase_start_request.entry_esi = actor_token;
            target_phase_start_request.entry_edi = group_b_index;
            target_phase_start_request.entry_return_address =
                kActionDispatchTargetPhaseStartReturnAddress;
            result.target_phase_start = start_legacy_battle_target_phase(
                &state.group_a_target_phases[group_a_index],
                &state.group_a_action_execution[group_a_index],
                &context.startup->render_geometry,
                port,
                context,
                target_phase_start_request
            );
            ++result.target_phase_start_calls;
            result.port_calls += result.target_phase_start.port_calls;
            if (result.target_phase_start.status !=
                LegacyBattleTargetPhaseStartStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_start_typed_stop;
                return result;
            }
            state.phase_condition = 1U;
            const auto presentation_mode =
                invoke(state, port, result, kCallSetMode, {7U});
            if (!activate_actor_presentation(
                    state,
                    context,
                    result,
                    actor_token,
                    presentation_mode.eax,
                    presentation_mode.edx,
                    0x004540E4U,
                    0x004540E9U,
                    presentation_mode.flags
                )) {
                return result;
            }
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
            if (low_word(invoke(
                             state,
                             port,
                             result,
                             kCallQueryTargetCode,
                             {group_b_token(group_b_index)}
                )
                             .eax) == 0x1CU) {
                static_cast<void>(
                    invoke(state, port, result, kCallClearMode, {1U})
                );
            }
        }
        if (context.startup == nullptr) {
            result.status = LegacyBattleActionDispatchStatus::
                target_phase_advance_typed_stop;
            return result;
        }
        const auto& render_geometry = context.startup->render_geometry;
        std::span<const u32> surface_row_offsets;
        if (render_geometry.surface_row_offsets != nullptr &&
            render_geometry.surface_height > 0) {
            surface_row_offsets = {
                render_geometry.surface_row_offsets.get(),
                static_cast<std::size_t>(render_geometry.surface_height),
            };
        }
        result.target_phase_advance = advance_legacy_battle_target_phase(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            &context.action_updater,
            &context.frame_provider,
            &state.target_phase_particle_nodes,
            &state.target_phase_particle_rng,
            &state.target_phase_particle_shared,
            &state.target_phase_particle_diagnostics,
            {
                .width = context.raster.surface.width,
                .height = context.raster.surface.height,
                .row_offsets = surface_row_offsets,
                .pixels = context.framebuffer.physical_pixels(),
            },
            &context.shared_effects.pixel_conversion,
            port,
            {
                .target_token = actor_token,
                .time_seed = context.target_phase_time_seed,
                .spawn_stack_snapshot =
                    context.target_phase_spawn_stack_snapshot,
            }
        );
        ++result.target_phase_advance_calls;
        result.port_calls += result.target_phase_advance.port_calls;
        if (result.target_phase_advance.status !=
            LegacyBattleTargetPhaseAdvanceStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                target_phase_advance_typed_stop;
            return result;
        }
        if (result.target_phase_advance.return_eax == 1U) {
            if (!activate_actor_presentation(
                    state,
                    context,
                    result,
                    actor_token,
                    result.target_phase_advance.return_eax,
                    result.target_phase_advance.return_edx,
                    0x004546B5U,
                    0x004546BAU,
                    {},
                    false
                )) {
                return result;
            }
            const auto target_code = invoke(
                state,
                port,
                result,
                kCallQueryTargetCode,
                {group_b_token(group_b_index)}
            );
            result.fixed_count = accumulate_legacy_battle_fixed_count(
                port.legacy_battle_fixed_object_state(),
                port,
                {
                    .owner_token = kLegacyBattleFixedCountOwnerToken,
                    .key = target_code.eax,
                    .delta = 1U,
                    .entry_eax = target_code.eax,
                    .entry_ecx = target_code.ecx,
                    .entry_edx = target_code.edx,
                }
            );
            ++result.fixed_count_calls;
            result.port_calls += result.fixed_count.allocation_calls;
            if (result.fixed_count.status !=
                LegacyBattleFixedCountStatus::completed) {
                result.status =
                    LegacyBattleActionDispatchStatus::fixed_count_typed_stop;
                return result;
            }
            state.packed_actor_counter =
                (state.packed_actor_counter & 0xFFFFFF00U) |
                static_cast<compat::u8>(state.packed_actor_counter + 1U);
            if (!remove_attack_order_entry(context, result, group_b_index)) {
                return result;
            }
            state.selected_target_index = static_cast<u16>(group_b_index);
            replace_low_word(state.phase_counter, 0x1EU);
            const u16 status = state.group_b_status_words[group_b_index];
            if (!publish_player_item_quantity(port, result, status, 1U)) {
                return result;
            }
        }
        return result;
    }
    case 7U:
        if (context.actor_frame_action_group_b.caller_snapshot == nullptr &&
            !require_group_b()) {
            return result;
        }
        if (const auto& binding = context.actor_frame_action_group_b;
            binding.caller_snapshot != nullptr) {
            const auto caller = advance_legacy_battle_actor_frame_caller(
                LegacyBattleActorFrameCallerSite::action_group_b,
                group_b_index,
                {.action = &state, .startup = context.startup},
                *binding.caller_snapshot,
                binding.ports == nullptr
                    ? LegacyBattleActorFrameEntryRoutePorts{}
                    : *binding.ports
            );
            if (binding.observed != nullptr) {
                *binding.observed = caller;
            }
            if (!caller.returned) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_frame_caller_typed_stop;
                return result;
            }
            reply.eax = caller.eax;
            reply.edx = caller.edx;
        } else {
            throw std::logic_error{
                "NOTIMPLEMENTED: unbound battle action 7 group-B actor frame"
            };
        }

        if (reply.eax != 1U) {
            return result;
        }
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                0U,
                reply.eax,
                reply.edx,
                0x0045550CU,
                subtract_flags(reply.eax, 1U)  // 0x004554FB CMP EAX,EBX; EBX=1.
            )) {
            return result;
        }
        if (!remove_attack_order_entry(context, result, group_b_index)) {
            return result;
        }
        if (group_b_index < 4U) {
            state.packed_actor_counter =
                (state.packed_actor_counter & 0xFFFFFF00U) |
                static_cast<compat::u8>(state.packed_actor_counter + 1U);
        }
        result.return_value = 1U;
        return result;
    case 11U:
    case 12U:
        reply = invoke(
            state,
            port,
            result,
            action == 11U ? kCallQueryModeB : kCallQueryModeC,
            {actor_token}
        );
        if (reply.eax != 1U) {
            return result;
        }
        static_cast<void>(invoke(
            state, port, result, kCallClearMode, {action == 11U ? 0U : 1U}
        ));
        static_cast<void>(invoke(state, port, result, kCallFinalizeMode, {8U}));
        state.overlay_gate = 1U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 13U:
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
        }
        result.action_thirteen = advance_legacy_battle_action_thirteen(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = group_a_token(group_a_index),
                .opponent_token = group_b_token(group_b_index),
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .base_coordinate_output_x_token =
                    state.base_coordinate_output_x_token,
                .base_coordinate_output_y_token =
                    state.base_coordinate_output_y_token,
            }
        );
        ++result.action_thirteen_calls;
        if (result.action_thirteen.status !=
            LegacyBattleActionThirteenStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_thirteen_typed_stop;
            return result;
        }
        if (result.action_thirteen.return_eax != 1U) {
            return result;
        }
        state.temporary_record.fill(0U);
        state.temporary_record[0x19U] |= 0x20U;
        state.current_actor_index = 0xFFFFU;
        static_cast<void>(
            invoke(state, port, result, kCallCommitMessageRecord, {0x004FF140U})
        );
        state.current_actor_index = 0xFFFFU;
        state.frame_effect.fade_active = 1U;
        state.frame_effect.primary_suppression = 0U;
        state.frame_effect.red_factor = 0;
        state.frame_effect.green_factor = 0;
        state.frame_effect.blue_factor = 0;
        replace_low_word(state.phase_counter, 0U);
        for (u32 slot = 0U; slot < 8U; ++slot) {
            ++result.group_a_iterations;
            const u32 index = group_a_index * 10U + slot;
            u16 value = 0U;
            if (!read_group_a_event_slot(
                    state, context, result, index, value
                )) {
                return result;
            }
            if (value == 0U) {
                write_group_a_event_slot(
                    state, context, index, static_cast<u16>(group_b_index + 1U)
                );
                result.return_value = 1U;
                return result;
            }
        }
        result.return_value = 1U;
        return result;
    case 14U:
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
            state.frame_effect.stage = 1;
        }
        result.action_fourteen = advance_legacy_battle_action_fourteen(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = group_a_token(group_a_index),
                .opponent_token = group_b_token(group_b_index),
                .coordinate_output_x_token = state.coordinate_output_x_token,
                .coordinate_output_y_token = state.coordinate_output_y_token,
                .base_coordinate_output_x_token =
                    state.base_coordinate_output_x_token,
                .base_coordinate_output_y_token =
                    state.base_coordinate_output_y_token,
            }
        );
        ++result.action_fourteen_calls;
        if (result.action_fourteen.status !=
            LegacyBattleActionFourteenStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_fourteen_typed_stop;
            return result;
        }
        if (result.action_fourteen.return_eax != 1U) {
            return result;
        }
        state.frame_effect.primary_suppression = 0U;
        state.frame_effect.red_factor = 0;
        state.frame_effect.green_factor = 0;
        state.frame_effect.blue_factor = 0;
        replace_low_word(state.phase_counter, 0U);
        state.frame_effect.fade_active = 1U;
        port.battle_message_state() = 0x62U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    default:
        return result;
    }
}

}  // namespace openswd3::battle::action_dispatch_detail
