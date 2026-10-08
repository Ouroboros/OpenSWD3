#include "legacy_battle_script_dispatch_internal.hpp"

namespace openswd3::battle::detail {

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_six() {
    const u32 caller_ecx = ecx_;
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish(eax_);
    }
    set_low_word(ecx_, actor);
    set_high_word(workspace_.packed_actor_state, actor);

    std::array<u16, 6> parameters{};
    if (!read_u16(wrapping_add(workspace_.cursor, 14U), parameters[5U])) {
        return finish(eax_);
    }
    set_low_word(eax_, parameters[5U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 12U), parameters[4U])) {
        return finish(eax_);
    }
    set_low_word(edx_, parameters[4U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 10U), parameters[3U])) {
        return finish(eax_);
    }
    set_low_word(eax_, parameters[3U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 8U), parameters[2U])) {
        return finish(eax_);
    }
    set_low_word(edx_, parameters[2U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 6U), parameters[1U])) {
        return finish(eax_);
    }
    set_low_word(eax_, parameters[1U]);
    eax_ = actor;
    if (!read_u16(wrapping_add(workspace_.cursor, 4U), parameters[0U])) {
        return finish(eax_);
    }
    set_low_word(edx_, parameters[0U]);

    const u32 actor_index = actor;
    const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
        actor_index * kLegacyBattleScriptGroupBElementSize;
    edx_ = actor_index * 1381U;
    ecx_ = actor_token;
    LegacyBattleActorGroupBElementState* actor_state = nullptr;
    if (bindings_.startup.group_b_lifecycle != nullptr &&
        actor_index < bindings_.startup.group_b_lifecycle->size()) {
        actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
    }

    result_.group_b_script_action_item_parameters =
        write_legacy_battle_group_b_script_action_item_parameters(
            actor_state,
            {
                .parameters = parameters,
                .actor_token = actor_token,
                .entry_eax = eax_,
                .entry_edx = edx_,
            }
        );
    ++result_.group_b_script_action_item_parameters_calls;
    eax_ = result_.group_b_script_action_item_parameters.return_eax;
    ecx_ = result_.group_b_script_action_item_parameters.return_ecx;
    edx_ = result_.group_b_script_action_item_parameters.return_edx;
    if (result_.group_b_script_action_item_parameters.status !=
        LegacyBattleGroupBScriptActionItemParametersStatus::completed) {
        result_.status = LegacyBattleScriptDispatchStatus::
            group_b_script_action_item_parameters_typed_stop;
        result_.stopped_offset =
            result_.group_b_script_action_item_parameters.stopped_offset;
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
    ecx_ = caller_ecx;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_seven() {
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 1U;
    invoke(
        LegacyBattleScriptDispatchCall::initialize_background,
        0U,
        {0x004FF1E4U, 0x004FF208U, 0x004FF238U, 0x004FF258U, 1U}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_eight() {
    u16 actor{};
    u16 target{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
        return finish();
    }
    const i32 actor_code = signed_word(actor);
    if (actor_code > 7) {
        bindings_.shared.selected_target =
            bindings_.final_actor.queued_actor_code;
        bindings_.final_actor.queued_actor_code = static_cast<u32>(actor_code);
        i32 published = signed_word(target) + 1;
        bindings_.final_actor.published_actor_code =
            static_cast<u32>(published);
        if (published > 7) {
            edx_ = static_cast<u32>(actor_code * 5 - 40);
            bindings_.startup.reset.value_53bfd0 = 1U;
            published -= 8;
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(published);
            const i32 index = actor_code - 8;
            if (index < 0 || index >= 10) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(index)
                );
            }
            bindings_.final_actor
                .group_a_slot_values[static_cast<std::size_t>(index)] = 1U;
        }
        const auto token = group_a_token(actor_code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        if (!set_actor_availability_block(actor_code, *token, 1U)) {
            return finish(eax_);
        }
        bindings_.target_selection.selected_action_kind = 6U;
        const i32 index = actor_code - 8;
        bindings_.shared.actor_state_words[static_cast<std::size_t>(index)] =
            1U;
    } else {
        edx_ = kLegacyBattleGroupASecondarySkipQueryToken;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_nine() {
    const u16 state = high_word(workspace_.packed_actor_state);
    if ((state & 0x8000U) == 0U) {
        const auto created = create_dynamic_text(true, false);
        if (created.status != LegacyBattleScriptDispatchStatus::completed) {
            return created;
        }
        set_high_word(
            workspace_.packed_actor_state,
            static_cast<u16>(high_word(workspace_.packed_actor_state) | 0x8000U)
        );
        bindings_.action.action_pending_aux = 1U;
    }
    if (!bindings_.dialogs.messages.empty()) {
        bindings_.action.frame_enabled = 0U;
        if (!run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }
    return finish_dynamic_text(true);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty() {
    workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
    u32 length{};
    if (!scan_percent_q(
            workspace_.text_offset, kLegacyBattleScriptTextScanLimit, length
        )) {
        return finish();
    }
    workspace_.text_buffer.fill(0U);
    for (u32 index = 0U; index < length; ++index) {
        u8 value{};
        if (!read_u8(wrapping_add(workspace_.text_offset, index), value)) {
            return finish();
        }
        workspace_.text_buffer[index] = value;
    }
    workspace_.cursor = wrapping_add(workspace_.text_offset, length + 2U);
    workspace_.text_offset = workspace_.cursor;
    u32 destination = 0U;
    for (const char value : bindings_.asset_root_path) {
        if (value == '\0') {
            break;
        }
        if (destination >= bindings_.shared.music_path.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::string_typed_stop, destination
            );
        }
        bindings_.shared.music_path[destination++] = static_cast<u8>(value);
    }
    constexpr std::array<u8, 6> prefix{'m', 'u', 's', 'i', 'c', '\\'};
    for (const u8 value : prefix) {
        if (destination >= bindings_.shared.music_path.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::string_typed_stop, destination
            );
        }
        bindings_.shared.music_path[destination++] = value;
    }
    for (u32 index = 0U; index < length; ++index) {
        if (destination >= bindings_.shared.music_path.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::string_typed_stop, destination
            );
        }
        bindings_.shared.music_path[destination++] =
            workspace_.text_buffer[index];
    }
    if (destination >= bindings_.shared.music_path.size()) {
        return stop(
            LegacyBattleScriptDispatchStatus::string_typed_stop, destination
        );
    }
    bindings_.shared.music_path[destination] = 0U;
    invoke(LegacyBattleScriptDispatchCall::stream_stop);
    invoke(LegacyBattleScriptDispatchCall::stream_start, 0U, {0U});
    invoke(LegacyBattleScriptDispatchCall::stream_set_volume);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_one() {
    u16 count = workspace_.word_d;
    u16 inner = high_word(workspace_.packed_value_b);
    u16 outer = low_word(workspace_.packed_value_b);
    if (count == 0U && inner == 0U && outer == 0U) {
        u16 scan = 1U;
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
        const bool group_a = actor > 7U;
        const u32 actor_index =
            group_a ? static_cast<u32>(actor) - 8U : static_cast<u32>(actor);
        const u32 token = group_a ? kLegacyBattleScriptGroupABaseToken +
                kLegacyBattleScriptGroupAElementSize * actor_index
                                  : kLegacyBattleScriptGroupBBaseToken +
                kLegacyBattleScriptGroupBElementSize * actor_index;
        if (group_a) {
            eax_ = 3021U * actor_index;
            edx_ = outer;
            flags_ = subtract_flags(1008U * actor_index, actor_index);
        } else {
            eax_ = actor_index;
            edx_ = 1381U * actor_index;
            flags_ = subtract_flags(24U * actor_index, actor_index);
        }

        if (!query_actor_target_selection_count(token, eax_, edx_, flags_)) {
            return finish(eax_);
        }

        flags_ = test_word_flags(low_word(eax_));
        if (flags_.zero) {
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
    if (inner != count) {
        workspace_.cursor =
            wrapping_add(workspace_.cursor, static_cast<u32>(count) * 2U + 8U);
        workspace_.packed_value_b = 0U;
    } else {
        workspace_.word_d = 0U;
        workspace_.packed_value_b = 0U;
    }
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_two() {
    std::array<u16, 7> words{};
    for (std::size_t index = 0U; index < words.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                ),
                words[index]
            )) {
            return finish();
        }
    }
    for (std::size_t index = 0U; index < 3U; ++index) {
        bindings_.shared.movement_start[index] =
            static_cast<float>(signed_word(words[index]));
        bindings_.shared.movement_target[index] =
            static_cast<float>(signed_word(words[index + 3U]));
    }
    workspace_.word_a = words[3];
    workspace_.word_b = words[4];
    workspace_.word_c = words[5];
    bindings_.shared.movement_frames = words[6];
    const float denominator =
        static_cast<float>(bindings_.shared.movement_frames);
    for (std::size_t index = 0U; index < 3U; ++index) {
        invoke(
            LegacyBattleScriptDispatchCall::x87_truncate,
            0U,
            {std::bit_cast<u32>(bindings_.shared.movement_start[index])}
        );
        const i32 start_integer = std::bit_cast<i32>(eax_);
        const i32 target_integer =
            static_cast<i32>(bindings_.shared.movement_target[index]);
        bindings_.shared.movement_step[index] =
            static_cast<float>(target_integer - start_integer) / denominator;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
    workspace_.word_a = 0U;
    workspace_.word_b = 0U;
    workspace_.word_c = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_three() {
    if (std::bit_cast<i32>(bindings_.shared.movement_frames) <= 0) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    }

    bindings_.action.frame_enabled = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.action.frame_enabled = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_four() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, actor);
    workspace_.word_a = argument;
    const auto token = actor_token(static_cast<i32>(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(LegacyBattleScriptDispatchCall::pending_482ec0, *token, {argument});
    if (eax_ == 1U) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        set_high_word(workspace_.packed_actor_state, 0U);
        workspace_.word_a = 0U;
    }

    if (!run_frame()) {
        return finish(eax_);
    }

    set_high_word(workspace_.packed_actor_state, 0U);
    workspace_.word_a = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_five() {
    u16 item_id{};
    u16 threshold{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), threshold)) {
        return finish();
    }
    set_high_word(workspace_.packed_value_a, item_id);
    set_low_word(workspace_.packed_value_b, threshold);
    invoke(
        LegacyBattleScriptDispatchCall::find_player_item, 0x004A9940U, {item_id}
    );
    workspace_.object_token = eax_;
    bool enough = false;
    if (eax_ != 0U) {
        auto* item = player_item(eax_);
        if (item == nullptr) {
            return stop(
                LegacyBattleScriptDispatchStatus::player_item_typed_stop, eax_
            );
        }
        const i32 total =
            signed_word(item->primary) + signed_word(item->secondary);
        enough = total >= static_cast<i32>(threshold);
    }
    if (enough) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        set_high_word(workspace_.packed_value_a, 0U);
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
    if (!run_frame()) {
        return finish(eax_);
    }

    set_high_word(workspace_.packed_value_a, 0U);
    set_low_word(workspace_.packed_value_b, 0U);
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_six() {
    u16 item_id{};
    u16 amount{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), amount)) {
        return finish();
    }
    set_high_word(workspace_.packed_value_a, item_id);
    set_low_word(workspace_.packed_value_b, amount);
    invoke(
        LegacyBattleScriptDispatchCall::find_player_item, 0x004A9940U, {item_id}
    );
    workspace_.object_token = eax_;
    auto* item = player_item(eax_);
    if (item == nullptr) {
        return stop(
            LegacyBattleScriptDispatchStatus::player_item_typed_stop, eax_
        );
    }
    u16 remaining = amount;
    if (item->primary != 0U) {
        if (signed_word(item->primary) <= static_cast<i32>(remaining)) {
            remaining = static_cast<u16>(remaining - item->primary);
            item->primary = 0U;
        } else {
            item->primary = static_cast<u16>(item->primary - remaining);
            remaining = 0U;
        }
        set_low_word(workspace_.packed_value_b, remaining);
    }
    if (item->secondary != 0U) {
        item->secondary = static_cast<u16>(item->secondary - remaining);
    }
    if (item->primary == 0U && item->secondary == 0U) {
        invoke(
            LegacyBattleScriptDispatchCall::detach_player_item,
            0x004A9940U,
            {item_id}
        );
    }
    if (!run_frame()) {
        return finish(eax_);
    }

    set_high_word(workspace_.packed_value_a, 0U);
    set_low_word(workspace_.packed_value_b, 0U);
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_seven() {
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    workspace_.completion_gate = 1U;
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_eight() {
    u16 actor{};
    u16 first{};
    u16 second{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), first) ||
        !read_u16(wrapping_add(workspace_.cursor, 6U), second)) {
        return finish();
    }
    workspace_.value_a = signed_word(actor);
    workspace_.word_a = first;
    workspace_.word_b = second;
    const auto token = actor_token(workspace_.value_a);
    if (!token.has_value()) {
        return finish(eax_);
    }
    const auto address = script_actor_address(actor);
    if (!publish_actor_coordinates(
            *token,
            first,
            second,
            actor > 7U ? address.coordinate_eax : address.selected_call_eax,
            actor > 7U ? edx_ : address.coordinate_eax,
            with_low_word(entry_esi_, second),
            entry_edi_,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.value_a = 0;
    workspace_.word_a = 0U;
    workspace_.word_b = 0U;
    bindings_.action.frame_enabled = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    bindings_.action.frame_enabled = 1U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_sixty_nine() {
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 0x08U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, actor);
    const u32 stale_argument = (eax_ & 0xFFFF0000U) | argument;
    const auto token = group_b_token(static_cast<i32>(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_47da90, *token, {stale_argument}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_one() {
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 0x10U;
    bindings_.shared.captured_group_a_count =
        static_cast<u8>(bindings_.startup.actor_metrics.group_a_count);
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    bindings_.message_state = 103U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_two() {
    std::array<u16, 4> words{};
    for (std::size_t index = 0U; index < words.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                ),
                words[index]
            )) {
            return finish();
        }
    }
    const u32 stale_second = (eax_ & 0xFFFF0000U) | words[1];
    invoke(
        LegacyBattleScriptDispatchCall::initialize_background,
        0U,
        {std::bit_cast<u32>(signed_word(words[0])),
         stale_second,
         std::bit_cast<u32>(signed_word(words[2])),
         std::bit_cast<u32>(signed_word(words[3])),
         1U}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_three() {
    u16 target{};
    u16 divisor_word{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), target) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), divisor_word)) {
        return finish();
    }
    workspace_.pair_x = target;
    workspace_.word_a = divisor_word;
    const i32 numerator =
        signed_word(target) - signed_word(workspace_.position_x);
    const i32 divisor = signed_word(divisor_word);
    eax_ = std::bit_cast<u32>(numerator);
    edx_ = numerator < 0 ? 0xFFFFFFFFU : 0U;
    if (divisor == 0) {
        return stop(
            LegacyBattleScriptDispatchStatus::divide_by_zero_typed_stop,
            wrapping_add(workspace_.cursor, 4U)
        );
    }
    if (numerator == std::numeric_limits<i32>::min() && divisor == -1) {
        return stop(
            LegacyBattleScriptDispatchStatus::divide_overflow_typed_stop,
            wrapping_add(workspace_.cursor, 4U)
        );
    }
    const i32 step = numerator / divisor;
    edx_ = std::bit_cast<u32>(numerator % divisor);
    bindings_.action.frame_enabled = 0U;
    port_.effect_shift_state().actor_delta = step;
    workspace_.position_x =
        static_cast<u16>(workspace_.position_x + static_cast<u16>(step));
    if (step == 0) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.position_x = 0U;
        set_high_word(workspace_.packed_value_a, 0U);
        workspace_.word_a = 0U;
        workspace_.position_y = 0U;
        bindings_.action.frame_enabled = 1U;
        return finish(1U);
    }
    i32 index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_a_count)) {
        const auto token = group_a_token(index + 8);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count_eax = bindings_.startup.actor_metrics.group_a_count;
        const auto query_flags = index == 0
            ? subtract_flags(count_eax, 0U)
            : subtract_flags(static_cast<u32>(index), count_eax);
        if (!query_actor_current_coordinate_words(
                0x0046CA77U,
                *token,
                workspace_.pair_x,
                workspace_.pair_y,
                kLegacyBattleScriptPairXToken,
                kLegacyBattleScriptPairYToken,
                count_eax,
                edx_,
                query_flags
            )) {
            return finish(eax_);
        }
        const u16 add_left = workspace_.pair_x;
        const u16 add_right =
            static_cast<u16>(port_.effect_shift_state().actor_delta);
        const u16 add_sum = static_cast<u16>(add_left + add_right);
        workspace_.pair_x = add_sum;
        const u32 value_x = with_low_word(count_eax, add_sum);
        const u32 value_y =
            with_low_word(kLegacyBattleScriptPairXToken, workspace_.pair_y);
        if (!publish_actor_coordinates(
                *token,
                value_x,
                value_y,
                value_x,
                value_y,
                *token,
                std::bit_cast<u32>(index),
                add_word_flags(add_left, add_right, add_sum)
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
        const u32 count_eax = bindings_.startup.actor_metrics.group_b_count;
        const auto query_flags = index == 0
            ? subtract_flags(count_eax, 0U)
            : subtract_flags(static_cast<u32>(index), count_eax);
        if (!query_actor_current_coordinate_words(
                0x0046CACBU,
                *token,
                workspace_.pair_x,
                workspace_.pair_y,
                kLegacyBattleScriptPairXToken,
                kLegacyBattleScriptPairYToken,
                count_eax,
                edx_,
                query_flags
            )) {
            return finish(eax_);
        }
        const u16 add_left = workspace_.pair_x;
        const u16 add_right =
            static_cast<u16>(port_.effect_shift_state().actor_delta);
        const u16 add_sum = static_cast<u16>(add_left + add_right);
        workspace_.pair_x = add_sum;
        const u32 value_x = with_low_word(count_eax, add_sum);
        const u32 value_y =
            with_low_word(kLegacyBattleScriptPairYToken, workspace_.pair_y);
        if (!publish_actor_coordinates(
                *token,
                value_x,
                value_y,
                value_x,
                kLegacyBattleScriptPairXToken,
                *token,
                std::bit_cast<u32>(index),
                add_word_flags(add_left, add_right, add_sum)
            )) {
            return finish(eax_);
        }
        ++index;
    }
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_four() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, actor);

    const u32 actor_index = actor;
    const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
        actor_index * kLegacyBattleScriptGroupBElementSize;
    LegacyBattleActorGroupBElementState* actor_state = nullptr;
    if (bindings_.startup.group_b_lifecycle != nullptr &&
        actor_index < bindings_.startup.group_b_lifecycle->size()) {
        actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
    }

    const u32 source_offset = wrapping_add(workspace_.cursor, 4U);
    const u32 caller_ecx = ecx_;
    result_.group_b_script_resource_parameters =
        write_legacy_battle_group_b_script_resource_parameters(
            actor_state,
            {
                .script_bytes = std::span<const u8>{bindings_.assets.script},
                .script_capacity = bindings_.assets.script_capacity,
                .source_offset = source_offset,
                .source_token = source_offset,
                .actor_token = actor_token,
                .entry_edx = actor_index * 345U,
            }
        );
    ++result_.group_b_script_resource_parameters_calls;
    eax_ = result_.group_b_script_resource_parameters.return_eax;
    ecx_ = result_.group_b_script_resource_parameters.return_ecx;
    edx_ = result_.group_b_script_resource_parameters.return_edx;
    if (result_.group_b_script_resource_parameters.status !=
        LegacyBattleGroupBScriptResourceParametersStatus::completed) {
        result_.status = LegacyBattleScriptDispatchStatus::
            group_b_script_resource_parameters_typed_stop;
        result_.stopped_offset =
            result_.group_b_script_resource_parameters.stopped_offset;
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 22U);
    ecx_ = caller_ecx;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_five() {
    const u32 caller_ecx = ecx_;
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish(eax_);
    }
    set_low_word(ecx_, actor);
    set_high_word(workspace_.packed_actor_state, actor);

    std::array<u16, 4> parameters{};
    if (!read_u16(wrapping_add(workspace_.cursor, 10U), parameters[3U])) {
        return finish(eax_);
    }
    set_low_word(eax_, parameters[3U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 8U), parameters[2U])) {
        return finish(eax_);
    }
    set_low_word(edx_, parameters[2U]);
    if (!read_u16(wrapping_add(workspace_.cursor, 6U), parameters[1U])) {
        return finish(eax_);
    }
    set_low_word(eax_, parameters[1U]);
    eax_ = actor;
    if (!read_u16(wrapping_add(workspace_.cursor, 4U), parameters[0U])) {
        return finish(eax_);
    }
    set_low_word(edx_, parameters[0U]);

    const u32 actor_index = actor;
    const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
        actor_index * kLegacyBattleScriptGroupBElementSize;
    edx_ = actor_index * 1381U;
    ecx_ = actor_token;
    LegacyBattleActorGroupBElementState* actor_state = nullptr;
    if (bindings_.startup.group_b_lifecycle != nullptr &&
        actor_index < bindings_.startup.group_b_lifecycle->size()) {
        actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
    }

    result_.group_b_script_special_action_item_parameters =
        write_legacy_battle_group_b_script_special_action_item_parameters(
            actor_state,
            {
                .parameters = parameters,
                .actor_token = actor_token,
                .entry_eax = eax_,
                .entry_edx = edx_,
            }
        );
    ++result_.group_b_script_special_action_item_parameters_calls;
    eax_ = result_.group_b_script_special_action_item_parameters.return_eax;
    ecx_ = result_.group_b_script_special_action_item_parameters.return_ecx;
    edx_ = result_.group_b_script_special_action_item_parameters.return_edx;
    if (result_.group_b_script_special_action_item_parameters.status !=
        LegacyBattleGroupBScriptSpecialActionItemParametersStatus::completed) {
        result_.status = LegacyBattleScriptDispatchStatus::
            group_b_script_special_action_item_parameters_typed_stop;
        result_.stopped_offset =
            result_.group_b_script_special_action_item_parameters
                .stopped_offset;
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 12U);
    ecx_ = caller_ecx;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_six() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, actor);
    const bool group_a = actor > 7U;
    const auto token = group_a ? group_a_token(static_cast<i32>(actor))
                               : group_b_token(static_cast<i32>(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_47f150,
        *token,
        group_a
            ? std::initializer_list<
                  u32>{std::bit_cast<u32>(-9999), 9999U, 9999U}
            : std::initializer_list<u32>{std::bit_cast<u32>(-100000), 0U, 0U}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_seven() {
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 0x40U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_eight() {
    constexpr u32 advance = 6U;
    if (workspace_.word_a == 0U) {
        u16 actor{};
        u16 target{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
            return finish();
        }
        set_high_word(workspace_.packed_value_a, actor);
        const i32 code = static_cast<i32>(actor);
        const auto token = group_a_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        bindings_.final_actor.published_actor_code =
            static_cast<u32>(signed_word(target) + 1);
        invoke(LegacyBattleScriptDispatchCall::pending_47ce80, *token);
        if (eax_ == 1U) {
            invoke(
                LegacyBattleScriptDispatchCall::pending_47e880,
                *token,
                {0x8000U}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47f150,
                *token,
                {std::bit_cast<u32>(-9999), 9999U, 9999U}
            );
        }
        const u32 actor_index = static_cast<u32>(code - 8);
        if (!select_actor_target(
                *token,
                target,
                actor_index * 1007U,
                edx_,
                0x0046DC9CU,
                0x0046DCA1U,
                subtract_flags(actor_index * 1008U, actor_index)
            )) {
            return finish(eax_);
        }
        if (!set_actor_action_mode(
                *token,
                advance,
                actor_index * 1007U,
                actor_index * 3021U,
                0x0046DCC9U,
                subtract_flags(actor_index * 1008U, actor_index)
            )) {
            return finish(eax_);
        }
        const u32 priority_actor_index = high_word(workspace_.packed_value_a);
        bindings_.shared.actor_order_workspace.fill(0U);
        bindings_.shared.attack_order_workspace.fill(0U);
        for (std::size_t index = 0U;
             index < bindings_.startup.reset.records_524788.size();
             ++index) {
            bindings_.startup.reset.records_524788[index].value_00 =
                0xFFFFFFFFU;
        }
        bindings_.target_selection.selected_action_kind = advance;
        bindings_.metrics.priority_actor_index = priority_actor_index;
        bindings_.action.action_pending_aux = 1U;
        bindings_.shared.script_phase_gate = 1U;
        bindings_.shared.script_aux_gate = 0U;
        const u32 start_gate_actor_code =
            bindings_.final_actor.published_actor_code;
        const u32 times_three =
            start_gate_actor_code + start_gate_actor_code * 2U;
        const u32 times_twenty_four = times_three << 3U;
        const u32 times_twenty_three =
            times_twenty_four - start_gate_actor_code;
        const u32 times_sixty_nine =
            times_twenty_three + times_twenty_three * 2U;
        const u32 times_three_hundred_forty_five =
            times_sixty_nine + times_sixty_nine * 4U;
        const u32 times_one_thousand_three_hundred_eighty_one =
            start_gate_actor_code + times_three_hundred_forty_five * 4U;
        const u32 start_gate_actor_token =
            0x005229E0U + times_one_thousand_three_hundred_eighty_one * 8U;
        if (!increment_actor_start_gate(
                start_gate_actor_token,
                times_one_thousand_three_hundred_eighty_one,
                times_three_hundred_forty_five,
                subtract_flags(times_twenty_four, start_gate_actor_code)
            )) {
            return finish();
        }
        bindings_.input_dispatch.selected_actor_reset_gate = 1U;
        workspace_.word_a = 0U;
    }
    bindings_.action.frame_enabled = 1U;
    if (!run_frame()) {
        return finish(eax_);
    }

    if (bindings_.input_dispatch.selected_actor_reset_gate != 0U) {
        return finish(1U);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, advance);
    workspace_.word_a = 0U;
    set_high_word(workspace_.packed_value_a, 0U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_seventy_nine() {
    u16 actor{};
    std::array<u16, 3> words{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    for (std::size_t index = 0U; index < words.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                ),
                words[index]
            )) {
            return finish();
        }
    }
    set_high_word(workspace_.packed_actor_state, actor);
    workspace_.word_a = words[0];
    workspace_.word_b = words[1];
    workspace_.word_c = words[2];
    if (actor > 7U) {
        const auto token = group_a_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47f150,
            *token,
            {std::bit_cast<u32>(signed_word(words[0])), words[1], words[2]}
        );
        if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                result_.actor_field_26b8_high_bit_set,
                request_.actor_field_26b8_high_bit_set_requests,
                *token,
                eax_,
                edx_,
                0x0046DADDU,
                flags_
            )) {
            eax_ = result_.actor_field_26b8_high_bit_set.last.return_eax;
            ecx_ = result_.actor_field_26b8_high_bit_set.last.return_ecx;
            edx_ = result_.actor_field_26b8_high_bit_set.last.return_edx;
            if (result_.actor_field_26b8_high_bit_set.last.flags_known) {
                flags_ = result_.actor_field_26b8_high_bit_set.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            return finish();
        }
        eax_ = result_.actor_field_26b8_high_bit_set.last.return_eax;
        ecx_ = result_.actor_field_26b8_high_bit_set.last.return_ecx;
        edx_ = result_.actor_field_26b8_high_bit_set.last.return_edx;
        flags_ = result_.actor_field_26b8_high_bit_set.last.flags;
        if (!execute_legacy_battle_actor_effect_resource_slot_write_call(
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                result_.effect_resource_slot_write,
                request_.effect_resource_slot_write_requests,
                *token,
                0x235EU,
                eax_,
                edx_,
                0x0046DB01U,
                0x0046DB06U,
                flags_
            )) {
            eax_ = result_.effect_resource_slot_write.last.return_eax;
            ecx_ = result_.effect_resource_slot_write.last.return_ecx;
            edx_ = result_.effect_resource_slot_write.last.return_edx;
            if (result_.effect_resource_slot_write.last.flags_known) {
                flags_ = result_.effect_resource_slot_write.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_effect_resource_slot_write_typed_stop;
            return finish();
        }
        eax_ = result_.effect_resource_slot_write.last.return_eax;
        ecx_ = result_.effect_resource_slot_write.last.return_ecx;
        edx_ = result_.effect_resource_slot_write.last.return_edx;
        flags_ = result_.effect_resource_slot_write.last.flags;
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d640,
            *token,
            {std::bit_cast<u32>(signed_word(words[0]))}
        );
        invoke(LegacyBattleScriptDispatchCall::pending_47cec0, *token, {1U});
        synchronize_legacy_battle_actor_effect_resource_cursor_update(
            {
                .action = &bindings_.action,
                .startup = &bindings_.startup,
            },
            *token,
            1U
        );
    } else {
        const auto token = group_b_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47f150,
            *token,
            {std::bit_cast<u32>(-signed_word(words[0])), words[1], words[2]}
        );
    }
    set_high_word(workspace_.packed_actor_state, 0U);
    workspace_.word_a = 0U;
    workspace_.word_b = 0U;
    workspace_.word_c = 0U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eighty() {
    u16 actor{};
    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, actor);
    workspace_.value_a = signed_word(argument);
    const auto token = group_b_token(static_cast<i32>(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }

    const u32 saved_entry_ecx = ecx_;
    if (bindings_.startup.group_b_lifecycle == nullptr) {
        bindings_.startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
    }
    auto& element = (*bindings_.startup.group_b_lifecycle)[actor];
    element.object_token = *token;
    if (element.resource_token == 0U) {
        element.resource_token =
            kLegacyBattleActorGroupBResourceStateBaseToken +
            static_cast<u32>(actor) * 0xA4U;
    }

    ScriptGroupBActionReconfigurationPort reconfiguration_port(*this);
    const auto reconfiguration = reconfigure_legacy_battle_group_b_action(
        &element,
        reconfiguration_port,
        {
            .definition_argument = std::bit_cast<u32>(workspace_.value_a),
            .actor_token = *token,
            .entry_edx = static_cast<u32>(actor) * 345U,
        },
        &reconfiguration_port
    );
    eax_ = reconfiguration.return_eax;
    ecx_ = reconfiguration.return_ecx;
    edx_ = reconfiguration.return_edx;
    if (reconfiguration.status !=
        LegacyBattleGroupBActionReconfigurationStatus::completed) {
        result_.status =
            LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
        return finish(eax_);
    }

    ecx_ = saved_entry_ecx;
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eighty_one() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    set_high_word(workspace_.packed_actor_state, value);
    if (bindings_.shared.comparison_word != value) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    bindings_.shared.comparison_word = 0U;
    set_high_word(workspace_.packed_actor_state, 0U);
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

LegacyBattleScriptDispatchResult ScriptRunner::case_eighty_two() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    bindings_.shared.mode_state = 2U;
    set_high_word(workspace_.packed_actor_state, value);
    if (value == 1U) {
        port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 0x100U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 &= ~0x100U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    bindings_.shared.mode_state = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_eighty_three() {
    port_.battle_debug_hotkey_state().battle_mode_flags_53bc24 |= 0x200U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

}  // namespace openswd3::battle::detail
