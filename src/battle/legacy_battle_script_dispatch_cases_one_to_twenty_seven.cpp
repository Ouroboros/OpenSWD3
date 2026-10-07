#include "legacy_battle_script_dispatch_internal.hpp"

namespace openswd3::battle::detail {

LegacyBattleScriptDispatchResult ScriptRunner::case_terminal() {
    cleanup_all_actors();
    if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
        return finish(eax_);
    }
    set_low_word(workspace_.waiting_state, 0U);
    invoke(LegacyBattleScriptDispatchCall::global_reset);
    shutdown_script_direct();
    return finish(0U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_one() {
    const u32 state = workspace_.waiting_state;
    if ((state & 0x8000U) == 0U) {
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), argument)) {
            return finish();
        }
        workspace_.waiting_argument = argument;
        set_low_word(
            workspace_.waiting_state, static_cast<u16>(argument | 0x8000U)
        );
        return finish(1U);
    }

    if ((state & 0x7FFFU) == 0U) {
        const u32 published_actor = static_cast<u32>(workspace_.coordinate_x);
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        set_low_word(workspace_.waiting_state, 0U);
        bindings_.shared.script_completion_gate = 1U;
        if (published_actor != 0U) {
            bindings_.final_actor.queued_actor_code = published_actor;
        }
        workspace_.coordinate_x = 0;
        return finish(1U);
    }

    bindings_.shared.frame_gate = 1U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.value_a = std::bit_cast<i32>(eax_);
    if (eax_ == 1U) {
        return finish(1U);
    }

    cleanup_all_actors();
    if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::global_reset);
    shutdown_script_direct();
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    set_low_word(workspace_.waiting_state, 0U);
    bindings_.shared.script_completion_gate = 1U;
    return finish(static_cast<u32>(workspace_.value_a));
}

LegacyBattleScriptDispatchResult ScriptRunner::case_two() {
    u16 actor_word = high_word(workspace_.packed_actor_state);
    if ((actor_word & 0x8000U) == 0U) {
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor_word)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor_word);
        workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
        if (!invoke(
                LegacyBattleScriptDispatchCall::allocate,
                0U,
                {kLegacyBattleScriptDynamicCommandSize}
            )) {
            return finish(eax_);
        }

        workspace_.dynamic_command_token = eax_;
        if (workspace_.dynamic_command_token == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop,
                workspace_.dynamic_command_token
            );
        }
        auto& allocated = workspace_.dynamic_commands.emplace_back();
        allocated.allocation_token = workspace_.dynamic_command_token;
        allocated.record.role_index = 0U;
        actor_word = high_word(workspace_.packed_actor_state);
        if (!query_actor_coordinates(actor_word)) {
            return finish(eax_);
        }
        if (!initialize_dynamic_text_actor_group(actor_word, false)) {
            return finish(eax_);
        }
        auto* message = dynamic_command();
        if (message == nullptr) {
            return finish(eax_);
        }

        message->record.flags = 0x800U;
        if (!invoke(
                LegacyBattleScriptDispatchCall::format_dynamic_text,
                workspace_.dynamic_command_token,
                {workspace_.text_offset,
                 kLegacyBattleScriptShortTextToken,
                 1U,
                 0U,
                 0U}
            )) {
            return finish(eax_);
        }

        if (!invoke(
                LegacyBattleScriptDispatchCall::finalize_dynamic_text,
                workspace_.dynamic_command_token
            )) {
            return finish(eax_);
        }

        message = dynamic_command();
        if (message == nullptr) {
            return finish(eax_);
        }

        message->record.lifetime_limit |= 0xFFFFU;
        message->record.character_delay = 2U;
        message->record.flags |= low_word(workspace_.packed_actor_state);
        set_low_word(workspace_.packed_actor_state, 0U);
        if (!publish_dynamic_text_coordinates()) {
            return finish(eax_);
        }

        set_high_word(
            workspace_.packed_actor_state,
            static_cast<u16>(high_word(workspace_.packed_actor_state) | 0x8000U)
        );
        bindings_.message_state = 0U;
        bindings_.action.action_pending_aux = 1U;
    }

    if (!bindings_.dialogs.messages.empty()) {
        bindings_.shared.frame_gate = 0U;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }

    for (i32 index = 0; index < 10; ++index) {
        const auto token = group_a_token(index + 8);
        if (!token.has_value()) {
            return finish(eax_);
        }
        if (!invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            ) ||
            !invoke(
                LegacyBattleScriptDispatchCall::pending_47d900, *token, {0U}
            )) {
            return finish(eax_);
        }
    }
    for (i32 index = 0; index < kLegacyBattleScriptExtendedGroupBCleanupCount;
         ++index) {
        const u32 token = kLegacyBattleScriptGroupBBaseToken +
            static_cast<u32>(index) * kLegacyBattleScriptGroupBElementSize;
        if (!invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, token, {0U}
            )) {
            return finish(eax_);
        }
    }

    u32 offset = 0U;
    while (offset < kLegacyBattleScriptTextScanLimit) {
        u8 first{};
        if (!read_u8(wrapping_add(workspace_.text_offset, offset), first)) {
            return finish();
        }
        if (first == 0x25U) {
            u8 second{};
            if (!read_u8(
                    wrapping_add(workspace_.text_offset, offset + 1U), second
                )) {
                return finish();
            }
            if (second == 0x51U) {
                break;
            }
        }
        ++offset;
    }
    workspace_.cursor = wrapping_add(workspace_.text_offset, offset + 2U);
    workspace_.text_offset = workspace_.cursor;
    workspace_.short_text.fill(0U);
    bindings_.shared.frame_gate = 1U;
    bindings_.action.action_pending_aux = 0U;
    workspace_.pair_x = 0U;
    workspace_.pair_y = 0U;
    set_high_word(workspace_.packed_actor_state, 0U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_three() {
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_four() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    bindings_.shared.frame_value = std::bit_cast<u32>(signed_word(value));
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_five() {
    u16 actor{};
    u16 delta_x{};
    u16 delta_y{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), delta_x) ||
        !read_u16(wrapping_add(workspace_.cursor, 6U), delta_y)) {
        return finish();
    }
    bindings_.shared.frame_gate = 0U;
    workspace_.position_x = delta_x;
    workspace_.position_y = delta_y;
    const auto token = actor_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    const auto address = script_actor_address(actor);
    const u32 query_entry_edx =
        actor > 7U ? with_low_word(edx_, delta_y) : address.coordinate_edx;
    if (!query_actor_current_coordinate_dwords(
            0x0046A694U,
            *token,
            workspace_.value_a,
            workspace_.value_b,
            address.coordinate_eax,
            query_entry_edx,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.value_a = static_cast<i32>(
        std::bit_cast<u32>(workspace_.value_a) +
        std::bit_cast<u32>(signed_word(delta_x))
    );
    workspace_.value_b = static_cast<i32>(
        std::bit_cast<u32>(workspace_.value_b) +
        std::bit_cast<u32>(signed_word(delta_y))
    );
    const bool group_a = actor > 7U;
    if (!publish_actor_coordinates(
            *token,
            std::bit_cast<u32>(workspace_.value_a),
            std::bit_cast<u32>(workspace_.value_b),
            group_a ? address.selected_call_eax : address.coordinate_eax,
            group_a ? std::bit_cast<u32>(workspace_.value_b)
                    : address.coordinate_edx,
            with_low_word(entry_esi_, actor),
            0U,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    workspace_.pair_x = 0U;
    workspace_.pair_y = 0U;
    workspace_.value_a = 0;
    workspace_.value_b = 0;
    workspace_.position_x = 1U;
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    if (!rebuild_actor_order_direct()) {
        return finish(eax_);
    }
    if (workspace_.frame_after_move_gate == 1U && !run_frame()) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_six() {
    u32 state = workspace_.dynamic_wait_state;
    bindings_.shared.frame_gate = 0U;
    if ((state & 0x8000U) == 0U) {
        u16 count{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), count)) {
            return finish();
        }
        state = (state & 0xFFFF0000U) |
            static_cast<u32>(static_cast<u16>(count | 0x8000U));
        workspace_.dynamic_wait_state = state;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }

    if ((state & 0x7FFFU) == 0U) {
        workspace_.dynamic_wait_state = 0U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        bindings_.shared.frame_gate = 1U;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }

    workspace_.dynamic_wait_state = state - 1U;
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eight() {
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    bindings_.shared.frame_gate = 1U;
    if (!run_frame()) {
        return finish(eax_);
    }

    u16 actor{};
    u16 selector{};
    u16 limit{};
    if (!read_u16(workspace_.cursor, actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 2U), selector) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), limit)) {
        return finish();
    }
    if (selector != 1U) {
        return finish(1U);
    }
    const auto token = actor_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_484500,
        *token,
        {static_cast<u32>(workspace_.coordinate_x),
         static_cast<u32>(workspace_.coordinate_y)}
    );
    if (workspace_.coordinate_x > signed_word(limit)) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    u16 next{};
    if (!read_u16(workspace_.cursor, next)) {
        return finish();
    }
    if (!invoke(
            LegacyBattleScriptDispatchCall::script_page_load,
            0U,
            {std::bit_cast<u32>(signed_word(next))}
        )) {
        return finish(eax_);
    }
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_nine() {
    u16 source{};
    u16 target{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), source) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
        return finish();
    }
    const i32 source_code = signed_word(source);
    i32 target_code = signed_word(target);
    if (source_code > 7) {
        bindings_.shared.selected_target =
            bindings_.final_actor.queued_actor_code;
        bindings_.final_actor.queued_actor_code = static_cast<u32>(source_code);
        ++target_code;
        bindings_.final_actor.published_actor_code =
            static_cast<u32>(target_code);
        if (target_code > 7) {
            edx_ = static_cast<u32>(source_code * 5 - 40);
            target_code -= 8;
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(target_code);
            bindings_.startup.reset.value_53bfd0 = 1U;
            const i32 slot = source_code - 8;
            if (slot < 0 || slot >= 10) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(slot)
                );
            }
            bindings_.final_actor
                .group_a_slot_values[static_cast<std::size_t>(slot)] = 1U;
        }
        const auto source_token = group_a_token(source_code);
        if (!source_token.has_value()) {
            return finish(eax_);
        }
        if (!set_actor_availability_block(source_code, *source_token, 1U)) {
            return finish(eax_);
        }
        bindings_.shared.action_state = 1U;
    } else {
        if (source_code < 0 || source_code >= 18) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(source_code)
            );
        }
        bindings_.shared
            .actor_target_words[static_cast<std::size_t>(source_code)] = 0U;
        i32 candidate = target_code;
        while (candidate <= 7) {
            const auto candidate_token = group_b_token(candidate);
            if (!candidate_token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47ce80, *candidate_token
            );
            if (eax_ != 1U) {
                break;
            }
            ++candidate;
        }
        bindings_.shared
            .actor_target_words[static_cast<std::size_t>(source_code)] =
            static_cast<u16>(candidate);
        bindings_.shared.script_aux_gate = 1U;
    }
    const std::size_t source_index = static_cast<std::size_t>(source_code);
    if (source_code < 0 ||
        source_index >= bindings_.shared.actor_target_words.size()) {
        return stop(
            LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
            static_cast<u32>(source_code)
        );
    }
    bindings_.shared.actor_target_words[source_index] = static_cast<u16>(
        bindings_.shared.actor_target_words[source_index] | 0x8000U
    );
    if (!insert_attack_order_direct(2U, std::bit_cast<u32>(source_code), 0U)) {
        return finish(eax_);
    }
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 1U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_ten() {
    u16 state = high_word(workspace_.packed_actor_state);
    if ((state & 0x8000U) == 0U) {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        if (code > 7) {
            const u32 actor_index = static_cast<u32>(code - 8);
            if (!activate_actor_presentation(
                    *token,
                    actor_index * 1007U,
                    actor_index * 3021U,
                    0x0046AF55U,
                    0x0046AF5AU,
                    subtract_flags(actor_index * 1008U, actor_index)
                )) {
                return finish(eax_);
            }
            bindings_.shared.selection_gate_b = 1U;
            bindings_.shared.selection_gate_a = 1U;
            bindings_.shared.selected_target = static_cast<u32>(code - 8);
        } else {
            const u32 actor_index = static_cast<u32>(code);
            if (!activate_actor_presentation(
                    *token,
                    actor_index * 1381U,
                    actor_index * 345U,
                    0x0046AFA8U,
                    0x0046AFADU,
                    subtract_flags(actor_index * 24U, actor_index)
                )) {
                return finish(eax_);
            }
            bindings_.shared.selection_gate_c = 1U;
            bindings_.shared.selection_gate_a = 1U;
            bindings_.shared.selected_target = static_cast<u32>(code);
            if (code < 0 || code >= 18) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(code)
                );
            }
            bindings_.shared.actor_state_words[static_cast<std::size_t>(code)] =
                static_cast<u32>(code);
        }
        bindings_.shared.action_completion_gate = 0U;
        set_high_word(
            workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
        );
    }
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    set_high_word(workspace_.packed_actor_state, 0U);
    bindings_.action.action_pending_aux = 0U;
    bindings_.shared.selection_gate_a = 0U;
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eleven() {
    u16 state = high_word(workspace_.packed_actor_state);
    if ((state & 0x8000U) == 0U) {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        if (code > 7) {
            const u32 index = static_cast<u32>(code - 8);
            if (!select_actor_target(
                    *token,
                    0U,
                    index * 1007U,
                    index * 3021U,
                    0x0046B522U,
                    0x0046B527U,
                    subtract_flags(index * 1008U, index)
                )) {
                return finish(eax_);
            }
            if (!set_actor_action_mode(
                    *token,
                    17U,
                    index * 3021U,
                    edx_,
                    0x0046B550U,
                    subtract_flags(index * 1008U, index)
                )) {
                return finish(eax_);
            }
        } else {
            const u32 index = static_cast<u32>(code);
            if (!select_actor_target(
                    *token,
                    0U,
                    index,
                    index * 1381U,
                    0x0046B5B9U,
                    0x0046B5BEU,
                    subtract_flags(index * 24U, index)
                )) {
                return finish(eax_);
            }
            if (!set_actor_action_mode(
                    *token,
                    17U,
                    index,
                    index * 1381U,
                    0x0046B5E5U,
                    subtract_flags(index * 24U, index)
                )) {
                return finish(eax_);
            }
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d860, *token, {argument}
        );
        if (!insert_attack_order_direct(
                code > 7 ? 1U : 2U, std::bit_cast<u32>(code), 0U
            )) {
            return finish(eax_);
        }
        set_high_word(
            workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
        );
        bindings_.shared.action_completion_gate = 0U;
        bindings_.shared.frame_gate = 1U;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }

    if (bindings_.shared.action_completion_gate != 1U) {
        bindings_.shared.frame_gate = 1U;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    set_high_word(workspace_.packed_actor_state, 0U);
    bindings_.action.action_pending_aux = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twelve() {
    i32 index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_b_count)) {
        if (index < 0 ||
            index >=
                static_cast<i32>(bindings_.shared.actor_state_words.size())) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(index)
            );
        }
        if (bindings_.shared
                .actor_state_words[static_cast<std::size_t>(index)] !=
            0xFFFFFFFFU) {
            if (!run_frame()) {
                return finish(eax_);
            }

            set_high_word(workspace_.packed_value_a, 1U);
            return finish(1U);
        }
        ++index;
    }
    if (bindings_.message_phase.group_b_bypass_gate == 0U) {
        bindings_.shared.published_group_b_count =
            static_cast<u8>(bindings_.startup.actor_metrics.group_b_count);
        bindings_.shared.published_group_b_aux = 0U;
        if (bindings_.message_state != 98U &&
            static_cast<i32>(bindings_.message_state) < 99) {
            bindings_.input_dispatch.selected_actor_cleanup_gate = 0U;
            bindings_.message_state = 99U;
        }
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    bindings_.message_phase.group_b_bypass_gate = 1U;
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirteen() {
    u16 actor{};
    u16 delta_y{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), delta_y)) {
        return finish();
    }
    workspace_.position_x = delta_y;
    const auto token = actor_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    const auto address = script_actor_address(actor);
    const bool group_a = actor > 7U;
    const u32 query_entry_eax =
        group_a ? address.selected_call_eax : address.coordinate_eax;
    const u32 query_entry_edx = group_a ? address.coordinate_eax : edx_;
    if (!query_actor_current_coordinate_dwords(
            0x0046A7C6U,
            *token,
            workspace_.value_a,
            workspace_.value_b,
            query_entry_eax,
            query_entry_edx,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.value_b = static_cast<i32>(
        std::bit_cast<u32>(workspace_.value_b) +
        std::bit_cast<u32>(signed_word(delta_y))
    );
    const u32 value_b = std::bit_cast<u32>(workspace_.value_b);
    const u32 value_a = with_low_word(
        value_b, low_word(std::bit_cast<u32>(workspace_.value_a))
    );
    if (!publish_actor_coordinates(
            *token,
            value_a,
            value_b,
            group_a ? address.selected_call_eax : address.coordinate_eax,
            group_a ? address.coordinate_eax : address.coordinate_edx,
            with_low_word(entry_esi_, actor),
            entry_edi_,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    workspace_.pair_x = 0U;
    workspace_.pair_y = 0U;
    workspace_.value_a = 0;
    workspace_.value_b = 0;
    workspace_.position_x = 0U;
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fourteen() {
    u16 count = workspace_.word_d;
    u16 inner = high_word(workspace_.packed_value_b);
    u16 outer = low_word(workspace_.packed_value_b);
    if (count == 0U && inner == 0U && outer == 0U) {
        u16 scan = 1U;
        count = 0U;
        for (;;) {
            u16 value{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, static_cast<u32>(scan) * 2U
                    ),
                    value
                )) {
                return finish();
            }
            if (value == 0xFFFFU) {
                break;
            }
            ++scan;
            ++count;
        }
        workspace_.word_d = count;
    }

    if (count != 0U) {
        while (outer < count) {
            u16 actor{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(outer) * 2U
                    ),
                    actor
                )) {
                return finish();
            }
            const auto token = actor_token(signed_word(actor));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47ceb0, *token);
            if (eax_ == 1U) {
                ++inner;
                set_high_word(workspace_.packed_value_b, inner);
                if (inner == count) {
                    workspace_.cursor = wrapping_add(
                        workspace_.cursor, static_cast<u32>(count) * 2U + 4U
                    );
                    u16 next{};
                    if (!read_u16(workspace_.cursor, next)) {
                        return finish();
                    }
                    if (!invoke(
                            LegacyBattleScriptDispatchCall::script_page_load,
                            0U,
                            {std::bit_cast<u32>(signed_word(next))}
                        )) {
                        return finish(eax_);
                    }
                    break;
                }
            }
            ++outer;
            set_low_word(workspace_.packed_value_b, outer);
        }
    }

    if (inner != count) {
        workspace_.cursor =
            wrapping_add(workspace_.cursor, static_cast<u32>(count) * 2U + 8U);
        set_high_word(workspace_.packed_value_b, 0U);
        set_low_word(workspace_.packed_value_b, 0U);
    } else {
        workspace_.word_d = 0U;
        workspace_.packed_value_b = 0U;
    }
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifteen() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    bindings_.shared.frame_gate = 0U;
    const i32 code = signed_word(actor);
    const auto token = actor_token(code);
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::pending_47d810, *token, {argument});
    if (code > 7 && (argument & 0x2000U) != 0U) {
        ++bindings_.shared.actor_mode_count;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixteen() {
    u16 state = low_word(workspace_.packed_value_a);
    bindings_.shared.frame_gate = 0U;
    if ((state & 0x8000U) == 0U) {
        u16 count{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), count)) {
            return finish();
        }
        ecx_ = (ecx_ & 0xFFFF0000U) |
            static_cast<u32>(static_cast<u16>(count | 0x8000U));
        --ecx_;
        state = low_word(ecx_);
        set_low_word(workspace_.packed_value_a, state);
    }
    if ((low_word(workspace_.packed_value_a) & 0x7FFFU) != 0U) {
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        if (!invoke(
                LegacyBattleScriptDispatchCall::script_page_load,
                0U,
                {std::bit_cast<u32>(signed_word(argument))}
            )) {
            return finish(eax_);
        }
        set_low_word(
            workspace_.packed_value_a,
            static_cast<u16>(low_word(workspace_.packed_value_a) - 1U)
        );
        if (!run_frame()) {
            return finish(eax_);
        }

        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    set_low_word(workspace_.packed_value_a, 0U);
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eighteen() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    const i32 code = signed_word(actor);
    const auto token = actor_token(code);
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::pending_47d830, *token, {argument});
    if (code > 7 && (argument & 0x2000U) != 0U) {
        --bindings_.shared.actor_mode_count;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_nineteen() {
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    u16 index = low_word(workspace_.packed_value_b);
    u16 count = high_word(workspace_.packed_value_b);
    if (count == 0U) {
        for (;;) {
            u16 candidate{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, static_cast<u32>(index) * 2U
                    ),
                    candidate
                )) {
                return finish();
            }
            if (candidate == 0xFFFFU) {
                break;
            }
            index = static_cast<u16>(index + 2U);
            ++count;
            set_low_word(workspace_.packed_value_b, index);
            set_high_word(workspace_.packed_value_b, count);
        }
    }

    u16 selected{};
    if (count == 1U) {
        if (!read_u16(workspace_.cursor, selected)) {
            return finish();
        }
    } else {
        invoke(LegacyBattleScriptDispatchCall::random_bounded, 0U, {count});
        const u32 offset = eax_ << 1U;
        if (!read_u16(wrapping_add(workspace_.cursor, offset), selected)) {
            return finish();
        }
    }
    workspace_.position_x = selected;
    if (!invoke(
            LegacyBattleScriptDispatchCall::script_page_load,
            0U,
            {std::bit_cast<u32>(signed_word(selected))}
        )) {
        return finish(eax_);
    }
    workspace_.position_x = 0U;
    workspace_.packed_value_b = 0U;
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    const auto token = actor_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::pending_47f900, *token, {1U});
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_one() {
    return run_action_case(11U, true, true);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_two() {
    u16 delta_word{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), delta_word)) {
        return finish();
    }
    const u32 delta = std::bit_cast<u32>(signed_word(delta_word));
    workspace_.position_x = delta_word;
    i32 index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_a_count)) {
        const auto token = group_a_token(index + 8);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count = bindings_.startup.actor_metrics.group_a_count;
        const auto query_flags = index == 0
            ? test_flags(count)
            : subtract_flags(static_cast<u32>(index), count);
        if (!query_actor_current_coordinate_dwords(
                0x0046BB44U,
                *token,
                workspace_.value_a,
                workspace_.value_b,
                count,
                edx_,
                query_flags
            )) {
            return finish(eax_);
        }
        const u32 add_left = std::bit_cast<u32>(workspace_.value_a);
        const u32 add_sum = add_left + delta;
        workspace_.value_a = std::bit_cast<i32>(add_sum);
        const u32 value_b = with_low_word(
            kLegacyBattleScriptCoordinateXToken,
            low_word(std::bit_cast<u32>(workspace_.value_b))
        );
        if (!publish_actor_coordinates(
                *token,
                add_sum,
                value_b,
                add_sum,
                value_b,
                *token,
                std::bit_cast<u32>(index),
                add_flags(add_left, delta, add_sum)
            )) {
            return finish(eax_);
        }
        ++index;
    }
    index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_b_count)) {
        const auto token = group_b_token(index);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count = bindings_.startup.actor_metrics.group_b_count;
        const auto query_flags = index == 0
            ? test_flags(count)
            : subtract_flags(static_cast<u32>(index), count);
        if (!query_actor_current_coordinate_dwords(
                0x0046BB9DU,
                *token,
                workspace_.value_a,
                workspace_.value_b,
                count,
                edx_,
                query_flags
            )) {
            return finish(eax_);
        }
        const u32 add_left = std::bit_cast<u32>(workspace_.value_a);
        const u32 add_sum = add_left + delta;
        workspace_.value_a = std::bit_cast<i32>(add_sum);
        const u32 value_b = with_low_word(
            delta, low_word(std::bit_cast<u32>(workspace_.value_b))
        );
        if (!publish_actor_coordinates(
                *token,
                add_sum,
                value_b,
                add_sum,
                kLegacyBattleScriptCoordinateXToken,
                *token,
                std::bit_cast<u32>(index),
                add_flags(add_left, delta, add_sum)
            )) {
            return finish(eax_);
        }
        ++index;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    workspace_.position_x = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_three() {
    u16 slot_word{};
    u16 actor_word{};
    u16 candidate_word{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), slot_word) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), actor_word) ||
        !read_u16(wrapping_add(workspace_.cursor, 6U), candidate_word)) {
        return finish();
    }
    workspace_.value_a = signed_word(slot_word);
    workspace_.value_b = signed_word(actor_word);
    workspace_.value_c = signed_word(candidate_word);
    const i32 actor = workspace_.value_b;
    i32 candidate = workspace_.value_c;
    if (actor > 7) {
        bindings_.shared.selected_target =
            bindings_.final_actor.queued_actor_code;
        bindings_.final_actor.queued_actor_code = static_cast<u32>(actor);
        i32 published = candidate;
        if (candidate > 7) {
            edx_ = static_cast<u32>(actor * 5 - 40);
            published -= 8;
            bindings_.startup.reset.value_53bfd0 = 1U;
            const i32 index = actor - 8;
            if (index < 0 || index >= 10) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(index)
                );
            }
            bindings_.startup.reset
                .block_520e90[static_cast<std::size_t>(index)] = 1U;
        }
        ++published;
        bindings_.final_actor.published_actor_code =
            static_cast<u32>(published);
        const auto token = group_a_token(actor);
        if (!token.has_value()) {
            return finish(eax_);
        }
        if (!set_actor_availability_block(actor, *token, 1U)) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_4707b0,
            *token,
            {static_cast<u32>(candidate)}
        );
        invoke(LegacyBattleScriptDispatchCall::pending_47d8b0, *token);
        const i32 actor_index = actor - 8;
        if (actor_index < 0 || actor_index >= 10) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(actor_index)
            );
        }
        bindings_.startup.reset
            .block_520e90[static_cast<std::size_t>(actor_index) + 3U] =
            low_word(eax_);
        invoke(LegacyBattleScriptDispatchCall::pending_47d880, *token);
        if (eax_ != 0U) {
            bindings_.input_dispatch.selection_target_cache = 1U;
        }
        invoke(LegacyBattleScriptDispatchCall::pending_47d8d0, *token);
        if (eax_ != 0U) {
            bindings_.shared.target_selection_block = 1U;
        }
        bindings_.shared.action_state = 2U;
        bindings_.shared
            .actor_state_words[static_cast<std::size_t>(actor_index)] = 2U;
    } else {
        if (actor < 0 || actor >= 18) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(actor)
            );
        }
        const auto actor_index = static_cast<std::size_t>(actor);
        bindings_.shared.actor_target_words[actor_index] = 0U;
        while (candidate <= 7) {
            const auto token = group_b_token(candidate);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47ce80, *token);
            if (eax_ != 1U) {
                break;
            }
            ++candidate;
            workspace_.value_c = candidate;
        }
        bindings_.shared.actor_target_words[actor_index] =
            static_cast<u16>(candidate);
        bindings_.shared.selection_gate_b = 1U;
        bindings_.shared.script_aux_gate = 1U;
        bindings_.shared.actor_target_words[actor_index] = static_cast<u16>(
            bindings_.shared.actor_target_words[actor_index] | 0x4000U
        );
        const auto token = group_b_token(actor);
        if (!token.has_value()) {
            return finish(eax_);
        }
        LegacyBattleActorGroupBElementState* element = nullptr;
        if (bindings_.startup.group_b_lifecycle != nullptr &&
            actor_index < bindings_.startup.group_b_lifecycle->size()) {
            element = &(*bindings_.startup.group_b_lifecycle)[actor_index];
        }
        ScriptGroupBActionCompositionPort composition_port(*this);
        result_
            .group_b_action_composition = compose_legacy_battle_group_b_action(
            element,
            &bindings_.message_state,
            composition_port,
            port_,
            {
                .definition_argument = std::bit_cast<u32>(workspace_.value_a),
                .actor_token = *token,
                .output_token = 0x0053BD40U,
                .entry_eax = eax_,
                .entry_ecx = *token,
                .entry_edx = static_cast<u32>(345 * actor),
                .action_mode_request = request_.actor_action_mode_requests
                                           [result_.actor_action_mode_calls],
            }
        );
        ++result_.group_b_action_composition_calls;
        if (result_.group_b_action_composition.mode_update_calls != 0U) {
            record_nested_actor_action_mode(
                result_.group_b_action_composition.actor_action_mode
            );
        }
        eax_ = result_.group_b_action_composition.return_eax;
        ecx_ = result_.group_b_action_composition.return_ecx;
        edx_ = result_.group_b_action_composition.return_edx;
        if (result_.group_b_action_composition.status !=
            LegacyBattleGroupBActionCompositionStatus::completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                group_b_action_composition_typed_stop;
            return finish(eax_);
        }
        if (!insert_attack_order_direct(2U, std::bit_cast<u32>(actor), 0U)) {
            return finish(eax_);
        }
    }
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 1U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_four() {
    cleanup_all_actors();
    if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::global_reset);
    u16 battle_id{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), battle_id)) {
        return finish();
    }
    invoke(LegacyBattleScriptDispatchCall::initialize_battle, 0U, {battle_id});
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_five() {
    if (bindings_.target_selection.transition_sample_word > 0U &&
        bindings_.target_selection.completion_gate == 0U) {
        bindings_.message_state = 102U;
    } else {
        bindings_.target_selection.completion_gate = 1U;
    }
    bindings_.shared.published_group_b_count =
        static_cast<u8>(bindings_.startup.actor_metrics.group_b_count);
    bindings_.shared.published_group_b_aux = 0U;
    bindings_.message_phase.group_b_bypass_gate = 1U;
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    if (eax_ != 0U) {
        return finish(1U);
    }
    bindings_.message_phase.group_b_bypass_gate = 0U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_six() {
    return run_action_case(12U, false, false);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_seven() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    const auto token = actor_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::pending_47d900, *token, {argument});
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

}  // namespace openswd3::battle::detail
