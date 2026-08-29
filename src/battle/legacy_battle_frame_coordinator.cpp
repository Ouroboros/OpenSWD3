#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"

#include <bit>
#include <optional>
#include <string>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

class SecondaryRngBoundedAdapter final : public LegacyBattleBoundedRandomPort {
public:
    explicit SecondaryRngBoundedAdapter(
        input_time_rng::LegacySecondaryRng& random
    ) noexcept
        : random_(random) {}

    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        return random_.next_bounded(bound);
    }

private:
    input_time_rng::LegacySecondaryRng& random_;
};

[[nodiscard]] LegacyBattleFrameCoordinatorCallReply invoke(
    LegacyBattleFrameCoordinatorPort& port,
    LegacyBattleFrameCoordinatorResult& result,
    const LegacyBattleFrameCoordinatorCall call,
    const std::array<u32, 8>& arguments = {}
) {
    ++result.port_calls;
    return port.invoke({.call = call, .arguments = arguments});
}

[[nodiscard]] constexpr i32
wrapping_add_i32(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(left) + static_cast<u32>(right));
}

[[nodiscard]] bool
frame_zero_completed(const LegacyBattleFrameDrawResult& result) noexcept {
    return result.status == LegacyBattleFrameDrawStatus::completed;
}

[[nodiscard]] bool standalone_completed(
    const LegacyBattleStandaloneActionFrameDrawResult& result
) noexcept {
    return result.status ==
        LegacyBattleStandaloneActionFrameDrawStatus::completed;
}

[[nodiscard]] bool dialog_completed(
    const story_scene::LegacyDialogRuntimeResult& result
) noexcept {
    return result.status == story_scene::LegacyDialogRuntimeStatus::idle ||
        result.status == story_scene::LegacyDialogRuntimeStatus::completed;
}

[[nodiscard]] bool countdown_completed(
    const rendering::LegacyCountdownDisplayResult& result
) noexcept {
    return result.status ==
        rendering::LegacyCountdownDisplayStatus::completed ||
        result.status ==
        rendering::LegacyCountdownDisplayStatus::hidden_inactive ||
        result.status ==
        rendering::LegacyCountdownDisplayStatus::hidden_suppressed;
}

[[nodiscard]] std::optional<u32> query_internal_flag(
    const std::span<const compat::u8> flags, const u32 index
) noexcept {
    const std::size_t byte_index = static_cast<std::size_t>(index >> 3U);
    if (byte_index >= flags.size()) {
        return std::nullopt;
    }
    const u32 mask = 1U << (index & 7U);
    return (static_cast<u32>(flags[byte_index]) & mask) != 0U ? 1U : 0U;
}

}  // namespace

LegacyBattleFrameCoordinatorResult run_legacy_battle_frame_coordinator(
    LegacyBattleFrameCoordinatorState& state,
    LegacyBattleFrameCoordinatorPort& port,
    LegacyBattleFrameCoordinatorContext& context,
    const LegacyBattleFrameCoordinatorRequest& request
) {
    LegacyBattleFrameCoordinatorResult result;
    state.active = 1U;

    LegacyBattleFrameCoordinatorCallReply reply = invoke(
        port, result, LegacyBattleFrameCoordinatorCall::query_music_gate
    );
    if (reply.eax == 1U && state.music_suppression == 0U) {
        static_cast<void>(port.start_music(state.music_path, 0U));
        result.music_started = true;
        reply = invoke(
            port,
            result,
            LegacyBattleFrameCoordinatorCall::music_commit,
            {state.music_runtime_handle}
        );
        ++result.music_commit_calls;
    }

    result.frame_input_resolution =
        coordinate_legacy_battle_frame_input_resolution(
            {
                .startup = context.startup,
                .final_actor = context.final_actor_step,
                .metrics = port.actor_metric_state(),
                .input_dispatch = port.battle_input_dispatch_state(),
                .input = context.input_normalization,
                .message_state = port.battle_message_state(),
                .choice_hotspots = context.choice_hotspots,
            },
            port,
            {
                .entry_eax = reply.eax,
                .entry_ecx = reply.ecx,
                .entry_edx = reply.edx,
            }
        );
    ++result.frame_input_resolution_calls;
    result.port_calls += result.frame_input_resolution.port_calls;
    if (result.frame_input_resolution.status !=
        LegacyBattleFrameInputResolutionStatus::completed) {
        result.status = LegacyBattleFrameCoordinatorStatus::
            frame_input_resolution_typed_stop;
        return result;
    }
    result.input_dispatch = coordinate_legacy_battle_input_dispatch(
        {
            .render_abort_latch = state.render_abort_latch,
            .startup_reset = context.startup.reset,
            .text_messages = context.startup.text_messages,
            .action_mode_source = context.startup.action_mode_source,
            .startup_party_presence = context.startup.party_presence,
            .startup_mode_flags = context.startup.mode_flags,
            .startup_supplemental_count_word =
                context.startup.supplemental_count_word,
            .startup_mirror_mode = context.startup.mirror_mode,
            .frame_input_resolution =
                port.battle_frame_input_resolution_state(),
            .final_actor = context.final_actor_step,
            .action = context.action_dispatch,
            .metrics = port.actor_metric_state(),
            .debug_hotkeys = port.battle_debug_hotkey_state(),
            .context_prompt = state.context_prompt,
            .message_state = port.battle_message_state(),
            .terminal_latch = port.battle_terminal_latch(),
            .one_shot_interaction_state =
                context.player_control.one_shot_interaction_state,
            .target_ready_gate = context.target_ready_gate,
            .outcome_darkening_gate =
                port.outcome_resolution_state().darkening_gate,
            .input_records = context.input_normalization.records,
            .keyboard = context.keyboard,
            .dialogs = context.dialogs,
            .choice_hotspots = context.choice_hotspots,
        },
        port,
        {
            .entry_eax = result.frame_input_resolution.return_eax,
            .entry_ecx = result.frame_input_resolution.return_ecx,
            .entry_edx = result.frame_input_resolution.return_edx,
            .mouse_y = request.mouse_y,
            .mouse_lower_bound = request.input_mouse_lower_bound,
            .mouse_upper_bound = request.input_mouse_upper_bound,
        }
    );
    ++result.input_dispatch_calls;
    result.port_calls += result.input_dispatch.port_calls;
    if (result.input_dispatch.status !=
        LegacyBattleInputDispatchStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::input_dispatch_typed_stop;
        return result;
    }
    result.pre_frame = advance_legacy_battle_pre_frame(
        context.final_actor_step,
        context.action_dispatch,
        port,
        result.input_dispatch.return_ecx,
        result.input_dispatch.return_edx
    );
    ++result.pre_frame_calls;
    result.port_calls += result.pre_frame.port_calls;
    if (result.pre_frame.status != LegacyBattlePreFrameStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::pre_frame_typed_stop;
        return result;
    }
    const auto actor_metrics = rebuild_legacy_battle_actor_metrics(port);
    result.port_calls += actor_metrics.port_calls;
    if (actor_metrics.status != LegacyBattleActorMetricStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::actor_metric_typed_stop;
        return result;
    }
    const auto actor_order = rebuild_legacy_battle_actor_order(
        port.actor_metric_state(),
        port.actor_metric_state().group_b_count,
        port.actor_metric_state().group_a_count,
        port.actor_metric_state().entry_edx
    );
    if (actor_order.status != LegacyBattleActorOrderStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::actor_order_typed_stop;
        return result;
    }
    result.debug_hotkeys = coordinate_legacy_battle_debug_hotkeys(
        context.keyboard,
        port.battle_debug_hotkey_state(),
        {
            .startup = context.startup,
            .final_actor = context.final_actor_step,
            .action = context.action_dispatch,
            .actor_metrics = port.actor_metric_state(),
            .actor_publication = port.actor_publication_state(),
            .effect_coordinator = port.effect_coordinator_state(),
            .effect_shift = port.effect_shift_state(),
            .actor_frames = context.actor_frames == nullptr
                ? nullptr
                : &context.actor_frames->state,
            .player_control = context.player_control,
            .message_state = port.battle_message_state(),
        },
        port
    );
    ++result.debug_hotkey_calls;
    result.port_calls += result.debug_hotkeys.port_calls;
    if (result.debug_hotkeys.status !=
        LegacyBattleDebugHotkeyStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::debug_hotkey_typed_stop;
        return result;
    }
    if (result.debug_hotkeys.return_value == 0U) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::pre_frame_returned_zero;
        result.return_value = 0U;
        return result;
    }

    reply = invoke(
        port,
        result,
        LegacyBattleFrameCoordinatorCall::lock_target_surface,
        {state.target_surface_token}
    );
    ++result.lock_calls;
    state.current_target_pointer_token = reply.eax;
    static_cast<void>(invoke(
        port,
        result,
        LegacyBattleFrameCoordinatorCall::unlock_target_surface,
        {state.target_surface_token, state.current_target_pointer_token}
    ));
    ++result.unlock_calls;

    if (state.render_abort_latch == 1U) {
        result.status = LegacyBattleFrameCoordinatorStatus::render_aborted;
        result.return_value = state.active;
        return result;
    }

    auto& selection_mode = context.final_actor_step.frame_gate_b;
    auto& selection_value = port.actor_metric_state().priority_actor_index;
    auto& input_source = context.startup.reset.records_524788[0].value_00;
    auto& selection_active = context.final_actor_step.selection_gate;
    auto& selection_source = context.final_actor_step.queued_actor_code;
    auto& debug_state = port.battle_debug_hotkey_state();

    if (selection_mode == 1U && selection_value == 0xFFFFFFFFU) {
        selection_mode = 0U;
    }
    if (input_source != 0xFFFFFFFFU && selection_active == 0U &&
        state.selection_enable == 1U && selection_mode == 0U) {
        if (state.selection_delay >= 0x10U) {
            result.attack_order_dequeue =
                dequeue_legacy_battle_attack_order_entry(
                    {
                        .records = context.startup.reset.records_524788,
                        .adjacent_intensity_records =
                            port.effect_coordinator_state().intensity_records,
                        .output =
                            {
                                .value_00 = &selection_value,
                                .tail_dwords = port.actor_metric_state()
                                                   .priority_actor_record_tail,
                            },
                    },
                    port,
                    {
                        .entry_eax = selection_mode,
                        .entry_ecx = selection_value,
                        .entry_edx = request.attack_order_dequeue_edx_snapshot,
                    }
                );
            ++result.selection_refresh_calls;
            result.port_calls += result.attack_order_dequeue.actor_query_calls;
            if (result.attack_order_dequeue.status !=
                LegacyBattleAttackOrderDequeueStatus::completed) {
                result.status = LegacyBattleFrameCoordinatorStatus::
                    attack_order_dequeue_typed_stop;
                return result;
            }
            if (selection_value != 0xFFFFFFFFU) {
                state.selection_delay = 0U;
                selection_active = 1U;
                state.selection_auxiliary = selection_value;
            }
        } else {
            state.selection_delay =
                static_cast<u16>(static_cast<u16>(state.selection_delay) + 1U);
        }
    }
    state.interaction_available =
        selection_value == 0xFFFFFFFFU && selection_source == 0U ? 1U : 0U;

    SecondaryRngBoundedAdapter selection_random(context.secondary_rng);
    result.selection_frame = draw_legacy_battle_selection_frame(
        {
            .startup = context.startup,
            .final_actor = context.final_actor_step,
            .metrics = port.actor_metric_state(),
            .actor_label_indices =
                context.startup.action_mode_source.actor_label_indices,
            .action = context.action_dispatch,
            .input_dispatch = port.battle_input_dispatch_state(),
            .frame_input = port.battle_frame_input_resolution_state(),
            .target_runtime = port.battle_target_selection_runtime_state(),
            .debug_hotkeys = port.battle_debug_hotkey_state(),
            .actor_frames = context.actor_frames == nullptr
                ? nullptr
                : &context.actor_frames->state,
            .message_state = port.battle_message_state(),
            .target_ready_gate = context.target_ready_gate,
            .panel_action_record = state.panel_action_record,
            .framebuffer = context.frame_zero.framebuffer,
            .clip = context.frame_zero.clip,
            .raster = context.raster,
            .shared_request = context.frame_zero.shared_request,
            .shared_effects = context.frame_zero.shared_effects,
            .jitter = context.frame_zero.jitter,
            .action_updater = context.action_updater,
            .frame_provider = context.frame_provider,
            .bounded_random = selection_random,
            .maps_payload = context.maps_payload,
            .shared_text = context.shared_text,
        },
        port,
        request.selection_frame_request
    );
    ++result.selection_frame_calls;
    result.port_calls += result.selection_frame.port_calls;
    if (result.selection_frame.status !=
        LegacyBattleSelectionFrameStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::selection_frame_typed_stop;
        return result;
    }
    LegacyBattleFrameEffectContext frame_effect_context{
        .framebuffer = context.frame_zero.framebuffer,
        .raster = context.raster,
        .shared_request = context.frame_zero.shared_request,
        .shared_effects = context.frame_zero.shared_effects,
        .jitter = context.frame_zero.jitter,
    };
    result.frame_effect = update_legacy_battle_frame_effect(
        state.frame_effect,
        context.frame_effect_port,
        frame_effect_context,
        context.frame_effect_source,
        context.frame_effect_surfaces,
        state.frame_effect.pending_rotation
    );
    ++result.frame_effect_calls;
    if (result.frame_effect.status !=
        LegacyBattleFrameEffectStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::frame_effect_typed_stop;
        return result;
    }
    if (state.conditional_mode != 1U || state.conditional_submode == 1U) {
        result.actor_priority = update_legacy_battle_actor_priority(
            port,
            request.actor_priority_eax_snapshot,
            request.actor_priority_ecx_snapshot,
            request.actor_priority_edx_snapshot
        );
        ++result.actor_priority_calls;
        result.port_calls += result.actor_priority.pair_query_calls;
        if (result.actor_priority.status !=
            LegacyBattleActorPriorityStatus::completed) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::actor_priority_typed_stop;
            return result;
        }
    }
    if (context.actor_frames != nullptr) {
        context.actor_frames->dispatch.shared_action_dispatch =
            &context.action_dispatch;
        context.actor_frames->dispatch.shared_final_actor =
            &context.final_actor_step;
        context.actor_frames->dispatch.target_selection_runtime =
            &port.battle_target_selection_runtime_state();
        context.actor_frames->dispatch.startup = &context.startup;
        context.actor_frames->dispatch.startup_reset = &context.startup.reset;
        context.actor_frames->dispatch.text_messages =
            &context.startup.text_messages;
    }
    result.actor_frame_sequence = advance_legacy_battle_actor_frame_sequence(
        port.actor_metric_state(), context.actor_frames
    );
    ++result.actor_frame_sequence_calls;
    if (result.actor_frame_sequence.status !=
        LegacyBattleActorFrameSequenceStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::actor_frame_typed_stop;
        return result;
    }
    result.frame_completion = update_legacy_battle_frame_completion(
        {
            .actors = port.actor_metric_state(),
            .final_actor = context.final_actor_step,
            .action = context.action_dispatch,
            .outcome = port.outcome_resolution_state(),
            .startup_reset = context.startup.reset,
            .message_state = port.battle_message_state(),
        },
        port,
        result.actor_frame_sequence.return_value,
        request.post_actor_frame_ecx_snapshot,
        request.post_actor_frame_edx_snapshot
    );
    ++result.frame_completion_calls;
    result.port_calls += result.frame_completion.mask_query_calls;
    if (result.frame_completion.status !=
        LegacyBattleFrameCompletionStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::frame_completion_typed_stop;
        return result;
    }
    result.pending_actions = commit_legacy_battle_pending_actions(
        {
            .ready_actor_slots = context.startup.reset.block_524420,
            .attack_order_records = context.startup.reset.records_524788,
            .attack_order_adjacent_record =
                &port.effect_coordinator_state().intensity_records[0],
            .global_mode = port.effect_coordinator_state().global_mode,
        },
        port,
        result.frame_completion.return_edx
    );
    ++result.pending_action_calls;
    result.port_calls += result.pending_actions.port_calls;
    if (result.pending_actions.status !=
        LegacyBattlePendingActionStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::pending_action_typed_stop;
        return result;
    }
    result.effect_coordinator = advance_legacy_battle_effect_coordinator(
        port.effect_coordinator_state(),
        context.startup,
        port,
        context.frame_zero.framebuffer,
        state.ui_state,
        selection_source
    );
    ++result.effect_coordinator_calls;
    result.port_calls += result.effect_coordinator.port_calls;
    if (result.effect_coordinator.status !=
        LegacyBattleEffectCoordinatorStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::effect_coordinator_typed_stop;
        return result;
    }
    if (result.effect_coordinator.return_value != 1U) {
        state.ui_state |= 1U;
    }

    result.fixed_frame = draw_legacy_battle_frame_zero(
        context.frame_zero.state,
        context.frame_zero.framebuffer,
        context.frame_zero.clip,
        context.frame_zero.shared_request,
        context.frame_zero.shared_effects,
        context.frame_zero.jitter,
        context.frame_zero.frame_provider,
        kLegacyBattleFrameCoordinatorFrameResource,
        0,
        384
    );
    ++result.fixed_frame_calls;
    if (!frame_zero_completed(result.fixed_frame)) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::fixed_frame_typed_stop;
        return result;
    }

    u32 stale_ecx = request.post_frame_zero_ecx_snapshot;
    if (selection_source != 0U) {
        const std::size_t source_index = selection_source;
        state.panel_action_record = {};
        state.panel_action_record.action_id =
            kLegacyBattleFrameCoordinatorPanelAction;
        state.panel_action_record.base_variant = 0U;
        result.panel_action_update =
            context.action_updater.update(state.panel_action_record);
        ++result.panel_action_update_calls;

        if (source_index >= request.role_index_map.size()) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::role_map_typed_stop;
            return result;
        }
        const u32 first_mapping = request.role_index_map[source_index];
        if (first_mapping >= request.role_index_map.size()) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::role_map_typed_stop;
            return result;
        }
        const i32 panel_left =
            std::bit_cast<i32>(request.role_index_map[first_mapping]);
        const u32 panel_resource =
            (first_mapping & 0xFFFF0000U) | state.panel_action_record.field_4a;
        result.panel_frame = rendering::draw_legacy_tiled_frame(
            context.frame_zero.framebuffer,
            context.raster,
            context.frame_provider,
            rendering::LegacyTiledFrameRequest{
                .resource_id = panel_resource,
                .left = panel_left,
                .top = 397,
                .right = wrapping_add_i32(panel_left, 0x74),
                .bottom = 467,
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            context.frame_zero.shared_effects,
            context.frame_zero.jitter
        );
        ++result.panel_frame_calls;
        if (result.panel_frame.status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::tiled_frame_typed_stop;
            return result;
        }
        stale_ecx = request.post_tiled_frame_ecx_snapshot;

        if (state.special_panel_suppression == 0U) {
            if (selection_source < 8U || selection_source >= 18U) {
                result.status =
                    LegacyBattleFrameCoordinatorStatus::role_actor_typed_stop;
                return result;
            }
            const u32 actor_token = kLegacyBattleActorGroupABaseToken +
                (selection_source - 8U) * kLegacyBattleActorGroupAElementSize;
            reply = invoke(
                port,
                result,
                LegacyBattleFrameCoordinatorCall::actor_ready_query,
                {actor_token}
            );
            stale_ecx = reply.ecx;
            if (reply.eax == 0U) {
                if (source_index >= request.role_positions.size()) {
                    result.status =
                        LegacyBattleFrameCoordinatorStatus::role_map_typed_stop;
                    return result;
                }
                const LegacyBattleFrameCoordinatorPosition& position =
                    request.role_positions[source_index];
                result.standalone_frame =
                    draw_legacy_battle_standalone_action_frame(
                        state.standalone_action,
                        context.frame_zero.framebuffer,
                        context.frame_zero.clip,
                        context.frame_zero.shared_request,
                        context.frame_zero.shared_effects,
                        context.frame_zero.jitter,
                        context.action_updater,
                        context.frame_provider,
                        kLegacyBattleFrameCoordinatorStandaloneAction,
                        position.x,
                        position.y,
                        request.standalone_action_update_ecx_snapshot,
                        request.standalone_action_update_edx_snapshot
                    );
                ++result.standalone_frame_calls;
                if (!standalone_completed(result.standalone_frame)) {
                    result.status = LegacyBattleFrameCoordinatorStatus::
                        standalone_frame_typed_stop;
                    return result;
                }
                stale_ecx = request.post_standalone_frame_ecx_snapshot;
            }
        }
    }

    result.gameplay_word_argument =
        (stale_ecx & 0xFFFF0000U) | request.gameplay_word;
    result.hud_frame = advance_legacy_battle_hud_frame(state.hud, port);
    ++result.hud_frame_calls;
    result.port_calls += result.hud_frame.port_calls;
    if (result.hud_frame.status != LegacyBattleHudFrameStatus::completed) {
        result.status = LegacyBattleFrameCoordinatorStatus::hud_typed_stop;
        return result;
    }
    static_cast<void>(invoke(
        port, result, LegacyBattleFrameCoordinatorCall::post_render_stage_1
    ));
    const std::span<const compat::u8> message_action_profiles =
        context.actor_frames == nullptr
        ? std::span<const compat::u8>{}
        : std::span<const compat::u8>{
              context.actor_frames->state.action_profile_bytes
          };
    result.message_phase = advance_legacy_battle_message_phase(
        {
            .state = port.battle_message_phase_state(),
            .startup = context.startup,
            .final_actor = context.final_actor_step,
            .action = context.action_dispatch,
            .metrics = port.actor_metric_state(),
            .debug_hotkeys = port.battle_debug_hotkey_state(),
            .input_dispatch = port.battle_input_dispatch_state(),
            .frame_input_resolution =
                port.battle_frame_input_resolution_state(),
            .selection_frame = port.battle_selection_frame_state(),
            .target_selection = port.battle_target_selection_runtime_state(),
            .target_ready_gate = context.target_ready_gate,
            .message_state = port.battle_message_state(),
            .dialogs = context.dialogs,
            .one_shot_interaction_state =
                context.player_control.one_shot_interaction_state,
            .outcome_darkening_gate =
                port.outcome_resolution_state().darkening_gate,
            .input_records = context.input_normalization.records,
            .action_profile_bytes = message_action_profiles,
            .victory_rewards =
                {
                    .state = port.battle_victory_reward_state(),
                    .startup = context.startup,
                    .metrics = port.actor_metric_state(),
                    .input_dispatch = port.battle_input_dispatch_state(),
                    .target_selection =
                        port.battle_target_selection_runtime_state(),
                    .party_member_resources =
                        context.story_vm.party_member_resources,
                    .script_variables = context.story_vm.script_variables,
                    .framebuffer = context.frame_zero.framebuffer,
                    .raster = context.raster,
                    .shared_effects = context.frame_zero.shared_effects,
                    .jitter = context.frame_zero.jitter,
                    .action_updater = context.action_updater,
                    .frame_provider = context.frame_provider,
                },
        },
        port,
        {
            .entry_ecx = request.message_phase_entry_ecx_snapshot,
            .entry_edx = request.message_phase_entry_edx_snapshot,
            .victory_reward_request = request.victory_reward_request,
        }
    );
    ++result.message_phase_calls;
    result.port_calls += result.message_phase.port_calls;
    if (result.message_phase.status !=
        LegacyBattleMessagePhaseStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::message_phase_typed_stop;
        return result;
    }
    result.text_message_frame = advance_legacy_battle_text_message_frame(
        {
            .messages = context.startup.text_messages,
            .head_token = context.startup.reset.block_5214f8[0],
            .freeze_gate = state.render_abort_latch,
            .panel_action_record =
                port.battle_victory_reward_state().panel_action_record,
            .color_fade = port.battle_selection_frame_state()
                              .selection_hint_frame.color_fade,
            .framebuffer = context.frame_zero.framebuffer,
            .clip = context.frame_zero.clip,
            .raster = context.raster,
            .shared_request = context.frame_zero.shared_request,
            .shared_effects = context.frame_zero.shared_effects,
            .jitter = context.frame_zero.jitter,
            .action_updater = context.action_updater,
            .frame_provider = context.frame_provider,
            .pixel_conversion = context.pixel_conversion,
        },
        port,
        request.text_message_frame_request
    );
    ++result.text_message_frame_calls;
    result.port_calls += result.text_message_frame.port_calls;
    if (result.text_message_frame.status !=
        LegacyBattleTextMessageFrameStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::text_message_frame_typed_stop;
        return result;
    }

    result.packed_rows = rendering::update_draw_legacy_packed_row_effects(
        context.packed_row_effects,
        context.packed_row_colors,
        context.secondary_rng,
        context.packed_row_draw_ports
    );
    result.role_heads = world_map::update_draw_legacy_role_head_actions(
        context.role_head_actions, context.role_head_action_ports
    );
    result.dialogs = story_scene::update_draw_legacy_dialogs(
        context.dialogs, context.dialog_input, context.dialog_ports
    );
    if (!dialog_completed(result.dialogs)) {
        result.status = LegacyBattleFrameCoordinatorStatus::dialog_typed_stop;
        return result;
    }
    result.debug_status_panel = draw_legacy_battle_debug_status_panel(
        {
            .debug_hotkeys = port.battle_debug_hotkey_state(),
            .target_selection = port.battle_target_selection_runtime_state(),
            .victory_rewards = port.battle_victory_reward_state(),
            .selection_frame = port.battle_selection_frame_state(),
            .framebuffer = context.frame_zero.framebuffer,
            .clip = context.frame_zero.clip,
            .raster = context.raster,
            .shared_request = context.frame_zero.shared_request,
            .shared_effects = context.frame_zero.shared_effects,
            .jitter = context.frame_zero.jitter,
            .action_updater = context.action_updater,
            .frame_provider = context.frame_provider,
            .pixel_conversion = context.pixel_conversion,
        },
        port,
        request.debug_status_panel_request
    );
    ++result.debug_status_panel_calls;
    result.port_calls += result.debug_status_panel.port_calls;
    if (result.debug_status_panel.status !=
        LegacyBattleDebugStatusPanelStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::debug_status_panel_typed_stop;
        return result;
    }

    result.countdowns[0] = rendering::draw_legacy_countdown(
        context.frame_zero.framebuffer,
        context.raster,
        context.countdown,
        context.countdown_flags,
        context.countdown_provider,
        rendering::LegacyCountdownDisplayRequest{
            .destination_x = 400,
            .destination_y = 8,
            .mode = 0,
        },
        context.frame_zero.shared_effects,
        context.frame_zero.jitter
    );
    ++result.countdown_calls;
    if (!countdown_completed(result.countdowns[0])) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::countdown_typed_stop;
        return result;
    }
    result.countdowns[1] = rendering::draw_legacy_countdown(
        context.frame_zero.framebuffer,
        context.raster,
        context.countdown,
        context.countdown_flags,
        context.countdown_provider,
        rendering::LegacyCountdownDisplayRequest{
            .destination_x = 10,
            .destination_y = 8,
            .mode = 1,
        },
        context.frame_zero.shared_effects,
        context.frame_zero.jitter
    );
    ++result.countdown_calls;
    if (!countdown_completed(result.countdowns[1])) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::countdown_typed_stop;
        return result;
    }

    ++result.input_queries;
    const std::optional<u32> input_flag =
        query_internal_flag(context.internal_flags, 0x11U);
    if (!input_flag.has_value()) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::internal_flag_typed_stop;
        return result;
    }
    if (*input_flag != 0U) {
        result.status = LegacyBattleFrameCoordinatorStatus::input_return_three;
        result.return_value = 3U;
        return result;
    }

    if (port.battle_debug_overlay_gate() == 1U) {
        result.debug_overlay = draw_legacy_battle_debug_overlay(
            {
                .overlay = state.debug_overlay,
                .hotkeys = port.battle_debug_hotkey_state(),
                .metrics = port.actor_metric_state(),
                .startup = context.startup,
                .final_actor = context.final_actor_step,
                .action = context.action_dispatch,
                .message_state = port.battle_message_state(),
                .effects = port.effect_coordinator_state(),
                .framebuffer = context.frame_zero.framebuffer,
            },
            port,
            {.vitality_stack_snapshot = request.debug_vitality_stack_snapshot}
        );
        ++result.debug_overlay_calls;
        result.port_calls += result.debug_overlay.port_calls;
        if (result.debug_overlay.status !=
            LegacyBattleDebugOverlayStatus::completed) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::debug_overlay_typed_stop;
            return result;
        }
    }
    result.outcome_resolution = update_legacy_battle_outcome_resolution(
        {
            .frame_active = state.active,
            .group_a_count = port.actor_metric_state().group_a_count,
            .group_b_count = port.actor_metric_state().group_b_count,
            .final_actor = context.final_actor_step,
            .action = context.action_dispatch,
            .message_state = port.battle_message_state(),
            .battle_mode_flags =
                port.battle_debug_hotkey_state().battle_mode_flags_53bc24,
            .framebuffer = context.frame_zero.framebuffer,
            .shared_effects = context.frame_zero.shared_effects,
        },
        port
    );
    ++result.outcome_resolution_calls;
    result.port_calls += result.outcome_resolution.audio_suspend_calls +
        result.outcome_resolution.outcome_calls;
    if (result.outcome_resolution.status !=
        LegacyBattleOutcomeResolutionStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::outcome_resolution_typed_stop;
        return result;
    }

    result.context_prompt = draw_legacy_battle_context_prompt(
        {
            .prompt = state.context_prompt,
            .action = context.action_dispatch,
            .final_actor = context.final_actor_step,
            .startup = context.startup,
            .message_state = port.battle_message_state(),
            .framebuffer = context.frame_zero.framebuffer,
            .clip = context.frame_zero.clip,
            .shared_request = context.frame_zero.shared_request,
            .shared_effects = context.frame_zero.shared_effects,
            .jitter = context.frame_zero.jitter,
            .action_updater = context.action_updater,
            .frame_provider = context.frame_provider,
        },
        port,
        {
            .mouse_x = request.mouse_x,
            .mouse_y = request.mouse_y,
            .action_update_edx_snapshot =
                request.context_prompt_action_update_edx_snapshot,
        }
    );
    ++result.context_prompt_calls;
    if (result.context_prompt.status !=
        LegacyBattleContextPromptStatus::completed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::context_prompt_typed_stop;
        return result;
    }
    if (port.battle_color_accumulation_state().countdown <= 0 &&
        port.battle_color_initialization_gate() != 1U) {
        result.color_initialization =
            initialize_legacy_battle_color_accumulation(
                port.battle_color_accumulation_state(),
                {
                    .current_red = 0x18,
                    .current_green = 0x18,
                    .current_blue = 0x18,
                    .target_red = 0,
                    .target_green = 0,
                    .target_blue = 0,
                    .countdown = 8,
                }
            );
        ++result.color_initialization_calls;
        port.battle_color_initialization_gate() = 0U;
    }
    static_cast<void>(invoke(
        port, result, LegacyBattleFrameCoordinatorCall::finalize_overlay, {1U}
    ));

    result.color_accumulation = update_legacy_battle_color_accumulation(
        port.battle_color_accumulation_state(),
        true,
        context.frame_zero.framebuffer,
        context.pixel_conversion
    );
    ++result.color_accumulation_calls;
    if (result.color_accumulation.status ==
        rendering::LegacyFrameColorTransitionStatus::framebuffer_failed) {
        result.status =
            LegacyBattleFrameCoordinatorStatus::color_accumulation_typed_stop;
        return result;
    }

    if (state.special_surface_gate != 0U &&
        (debug_state.battle_mode_flags_53bc24 & 0x00000100U) == 0U) {
        const u32 temporary = port.create_temporary_surface(
            kLegacyBattleFrameCoordinatorSurfaceOwnerToken,
            kLegacyBattleFrameCoordinatorSurfaceFormat
        );
        ++result.temporary_surface_calls;
        if (temporary == 0U) {
            result.status = LegacyBattleFrameCoordinatorStatus::
                temporary_surface_typed_stop;
            return result;
        }
        static_cast<void>(
            port.operate_surface(temporary, state.target_surface_token)
        );
        ++result.surface_operation_calls;
    } else {
        result.vertical_shift = run_legacy_battle_vertical_shift(
            port,
            state.special_surface_gate,
            debug_state.battle_mode_flags_53bc24,
            context.frame_zero.framebuffer
        );
        ++result.vertical_shift_calls;
        if (result.vertical_shift.status !=
            LegacyBattleVerticalShiftStatus::completed) {
            result.status =
                LegacyBattleFrameCoordinatorStatus::vertical_shift_typed_stop;
            return result;
        }
    }

    if (debug_state.screenshot_request == 1U) {
        state.screenshot_counter =
            static_cast<u16>(state.screenshot_counter + 1U);
        const u32 screenshot_number =
            static_cast<u32>(state.screenshot_counter) + 1000U;
        state.screenshot_path = std::filesystem::path(
            "c:\\snap\\" + std::to_string(screenshot_number) + ".bmp"
        );
        result.screenshot = rendering::write_legacy_16bit_framebuffer_bmp(
            context.frame_zero.framebuffer.physical_pixels(),
            640,
            480,
            state.screenshot_path.string(),
            context.pixel_conversion,
            context.bmp_ports
        );
        ++result.screenshot_calls;
        debug_state.screenshot_request = 0U;
    }

    result.return_value = state.active;
    return result;
}

}  // namespace openswd3::battle
