#include "legacy_battle_script_dispatch_internal.hpp"

namespace openswd3::battle::detail {

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_eight() {
    u16 actor{};
    std::array<u16, 3> arguments{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                ),
                arguments[index]
            )) {
            return finish();
        }
    }
    const i32 code = signed_word(actor);
    const auto token = actor_token(code);
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_47d950,
        *token,
        {arguments[0], arguments[1], arguments[2]}
    );
    if (code > 7) {
        invoke(
            LegacyBattleScriptDispatchCall::pending_484500,
            *token,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        const i32 index = code - 8;
        if (index < 0 || index >= 10) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(index)
            );
        }
        bindings_.startup.party_metrics[static_cast<std::size_t>(index)]
            .primary_numerator = workspace_.coordinate_y;
        invoke(
            LegacyBattleScriptDispatchCall::pending_4838a0,
            *token,
            {workspace_.word_a, workspace_.word_b}
        );
        bindings_.startup.party_metrics[static_cast<std::size_t>(index)]
            .secondary_numerator = signed_word(workspace_.word_b);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    workspace_.word_a = 0U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_twenty_nine() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }

    set_high_word(workspace_.packed_actor_state, actor);
    const auto address = script_actor_address(actor);
    ecx_ = address.token;
    if (actor > 7U) {
        eax_ = address.selected_call_eax;
        flags_ = address.coordinate_flags;
        if (!toggle_actor_binary_state(
                address.token, eax_, edx_, 0x0046B04EU, 0x0046B053U, flags_
            )) {
            return finish();
        }
    } else {
        eax_ = address.coordinate_eax;
        edx_ = address.coordinate_edx;
        flags_ = address.coordinate_flags;
        if (!toggle_actor_binary_state(
                address.token, eax_, edx_, 0x0046B072U, 0x0046B077U, flags_
            )) {
            return finish();
        }
        bindings_.shared.published_group_b_aux =
            static_cast<u8>(bindings_.shared.published_group_b_aux + 1U);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    set_high_word(workspace_.packed_actor_state, 0U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    bindings_.shared.frame_gate = 0U;
    invoke(LegacyBattleScriptDispatchCall::sample_play, 0x004FF1E4U, {value});
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_one() {
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    u16 index = high_word(workspace_.packed_value_b);
    u32 count = workspace_.list_count;
    for (;;) {
        u16 value{};
        if (!read_u16(
                wrapping_add(workspace_.cursor, static_cast<u32>(index) * 2U),
                value
            )) {
            return finish();
        }
        if (value == 0xFFFFU) {
            break;
        }
        index = static_cast<u16>(index + 2U);
        ++count;
        set_high_word(workspace_.packed_value_b, index);
        workspace_.list_count = count;
    }
    if (bindings_.shared.external_choice + 1U > count) {
        invoke(LegacyBattleScriptDispatchCall::message_box);
        return finish(0U);
    }
    u16 selected{};
    if (!read_u16(
            wrapping_add(
                workspace_.cursor, bindings_.shared.external_choice * 4U
            ),
            selected
        )) {
        return finish();
    }
    workspace_.position_x = selected;
    if (!invoke(
            LegacyBattleScriptDispatchCall::script_page_load,
            0U,
            {std::bit_cast<u32>(signed_word(selected))}
        )) {
        return finish(eax_);
    }
    workspace_.list_count = 0U;
    set_high_word(workspace_.packed_value_b, 0U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_three() {
    std::array<u16, 4> values{};
    for (std::size_t index = 0U; index < values.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                ),
                values[index]
            )) {
            return finish();
        }
    }
    invoke(LegacyBattleScriptDispatchCall::allocate, 0U, {180U});
    workspace_.dynamic_list_token = eax_;
    if (eax_ == 0U) {
        return stop(
            LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
        );
    }
    LegacyBattleScriptPanelNode node{
        .token = eax_,
        .value_00 = values[0],
        .value_04 = values[1],
        .value_08 = values[2],
        .value_0c = values[3],
        .display_x = -120,
        .display_y = 0,
        .next_token = bindings_.shared.panel_head_token,
    };
    invoke(LegacyBattleScriptDispatchCall::random_bounded, node.token);
    if (signed_word(values[2]) > 320) {
        node.display_x = 760;
    }
    workspace_.panel_nodes.push_back(node);
    bindings_.shared.panel_head_token = node.token;
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_four() {
    u16 value_00{};
    u16 value_08{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value_00) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), value_08)) {
        return finish();
    }
    u32 token = bindings_.shared.panel_head_token;
    while (token != 0U) {
        auto* node = panel_node(token);
        if (node == nullptr) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop, token
            );
        }
        if (low_word(node->value_00) == value_00 &&
            low_word(node->value_08) == value_08) {
            node->state_9a = 0xFFFFU;
            if (std::bit_cast<i16>(node->state_98) > 320) {
                node->state_9a = 1U;
            }
            break;
        }
        token = node->next_token;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_five() {
    workspace_.packed_actor_state |= 0xA0U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_six() {
    std::array<u16, 7> values{};
    for (std::size_t index = 0U; index < values.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                ),
                values[index]
            )) {
            return finish();
        }
    }
    if (values[0] >= 0x100U) {
        invoke(
            LegacyBattleScriptDispatchCall::noop_service,
            0x004A7B8CU,
            {values[1], 0x004A1810U}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
        return finish(1U);
    }
    for (u32 token = bindings_.shared.effect_head_token; token != 0U;) {
        auto* node = effect_node(token);
        if (node == nullptr) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop, token
            );
        }
        if (static_cast<u8>(node->type) == static_cast<u8>(values[0])) {
            set_high_word(workspace_.packed_value_a, 1U);
            invoke(
                LegacyBattleScriptDispatchCall::noop_service,
                0x004A7B8CU,
                {values[1], 0x004A1810U}
            );
            workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
            return finish(1U);
        }
        token = node->next_token;
    }
    invoke(LegacyBattleScriptDispatchCall::allocate, 0U, {24U});
    if (eax_ == 0U) {
        return stop(
            LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
        );
    }
    LegacyBattleScriptEffectNode node{
        .token = eax_,
        .type = values[0],
        .parameter = values[1],
        .x = std::bit_cast<i16>(static_cast<u16>(values[3] & 0xFFFEU)),
        .y = std::bit_cast<i16>(static_cast<u16>(values[4] & 0xFFFEU)),
        .width = std::bit_cast<i16>(static_cast<u16>(values[5] & 0xFFFEU)),
        .height = std::bit_cast<i16>(static_cast<u16>(values[6] & 0xFFFEU)),
        .first_words = {},
        .second_words = {},
        .next_token = bindings_.shared.effect_head_token,
    };
    const i32 right = static_cast<i32>(node.x) + static_cast<i32>(node.width);
    const i32 bottom = static_cast<i32>(node.y) + static_cast<i32>(node.height);
    if (node.x < 0 || node.y < 0 || right > 640 || bottom > 480) {
        invoke(LegacyBattleScriptDispatchCall::release_allocation, node.token);
        invoke(
            LegacyBattleScriptDispatchCall::noop_service,
            0x004A186CU,
            {0x004A7B68U,
             std::bit_cast<u32>(static_cast<i32>(node.x)),
             std::bit_cast<u32>(static_cast<i32>(node.y)),
             std::bit_cast<u32>(static_cast<i32>(node.width)),
             std::bit_cast<u32>(static_cast<i32>(node.height))}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
        return finish(1U);
    }
    const u32 allocation_size =
        std::bit_cast<u32>(static_cast<i32>(node.height) * 2);
    invoke(
        LegacyBattleScriptDispatchCall::allocate, node.token, {allocation_size}
    );
    const u32 first_token = eax_;
    invoke(
        LegacyBattleScriptDispatchCall::allocate, node.token, {allocation_size}
    );
    const u32 second_token = eax_;
    u16 fill = 0U;
    u16 flag = 0x8000U;
    if (values[2] == 1U) {
        fill = static_cast<u16>(node.width - 2);
        flag = 0x4000U;
    } else if (values[2] == 2U) {
        fill = 0x0800U;
        flag = 0x0800U;
    }
    node.type = static_cast<u16>(node.type | flag);
    if (node.height > 0) {
        if (first_token == 0U || second_token == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop,
                first_token == 0U ? first_token : second_token
            );
        }
        node.first_words.assign(static_cast<std::size_t>(node.height), fill);
        node.second_words.assign(static_cast<std::size_t>(node.height), 2U);
    }
    workspace_.effect_nodes.push_back(std::move(node));
    bindings_.shared.effect_head_token = workspace_.effect_nodes.back().token;
    workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_seven() {
    u16 type{};
    u16 operation{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), type) ||
        !read_u16(wrapping_add(workspace_.cursor, 4U), operation)) {
        return finish();
    }
    if (type >= 0x100U) {
        invoke(
            LegacyBattleScriptDispatchCall::noop_service,
            0x004A7B8CU,
            {operation}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }
    u32 previous = 0U;
    u32 token = bindings_.shared.effect_head_token;
    while (token != 0U) {
        auto* node = effect_node(token);
        if (node == nullptr) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop, token
            );
        }
        if (static_cast<u8>(node->type) == static_cast<u8>(type)) {
            if (operation == 0U) {
                bindings_.shared.effect_mask = 0x2000U;
            } else if (operation == 1U) {
                bindings_.shared.effect_mask = 0x1000U;
            } else if (operation == 2U) {
                if (previous == 0U) {
                    bindings_.shared.effect_head_token = node->next_token;
                } else {
                    auto* previous_node = effect_node(previous);
                    if (previous_node == nullptr) {
                        return stop(
                            LegacyBattleScriptDispatchStatus::
                                shared_state_typed_stop,
                            previous
                        );
                    }
                    previous_node->next_token = node->next_token;
                }
                invoke(
                    LegacyBattleScriptDispatchCall::release_allocation,
                    token,
                    {0U}
                );
                invoke(
                    LegacyBattleScriptDispatchCall::release_allocation,
                    token,
                    {1U}
                );
                invoke(
                    LegacyBattleScriptDispatchCall::release_allocation,
                    token,
                    {2U}
                );
                workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
                return finish(1U);
            }
            node->type =
                static_cast<u16>(node->type | bindings_.shared.effect_mask);
            workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
            return finish(1U);
        }
        previous = token;
        token = node->next_token;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_thirty_nine() {
    u32 publication_esi = entry_esi_;
    u16 state = high_word(workspace_.packed_actor_state);
    if ((state & 0x8000U) == 0U) {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        set_high_word(
            workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
        );
        const u16 actor_index = static_cast<u16>(actor & 0x7FFFU);
        const auto token = actor_token(static_cast<i32>(actor_index));
        if (!token.has_value()) {
            return finish(eax_);
        }
        const auto address = script_actor_address(actor_index);
        const bool group_a = actor_index > 7U;
        if (!query_actor_current_coordinate_dwords(
                0x0046C610U,
                *token,
                workspace_.value_a,
                workspace_.value_b,
                group_a ? address.coordinate_eax : actor_index,
                group_a ? actor_index : address.coordinate_eax,
                address.coordinate_flags
            )) {
            return finish(eax_);
        }
        const u16 base_x = low_word(std::bit_cast<u32>(workspace_.value_a));
        const u16 base_y = low_word(std::bit_cast<u32>(workspace_.value_b));
        workspace_.list_words[0] = base_x;
        workspace_.list_words[1] = base_y;
        workspace_.list_words[2] = base_x;
        workspace_.list_words[3] = base_y;
        for (std::size_t point = 0U; point < 5U; ++point) {
            u16 delta_x{};
            u16 delta_y{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 4U + static_cast<u32>(point) * 4U
                    ),
                    delta_x
                ) ||
                !read_u16(
                    wrapping_add(
                        workspace_.cursor, 6U + static_cast<u32>(point) * 4U
                    ),
                    delta_y
                )) {
                return finish();
            }
            publication_esi = with_low_word(publication_esi, delta_x);
            workspace_.list_words[4U + point * 2U] =
                static_cast<u16>(base_x + delta_x);
            workspace_.list_words[5U + point * 2U] =
                static_cast<u16>(base_y + delta_y);
        }
        const u16 final_x = workspace_.list_words[12];
        const u16 final_y = workspace_.list_words[13];
        workspace_.list_words[14] = final_x;
        workspace_.list_words[15] = final_y;
        workspace_.list_words[16] = final_x;
        workspace_.list_words[17] = final_y;
        workspace_.position_x = 1U;
        workspace_.position_y = 0U;
    }
    if (std::bit_cast<i16>(workspace_.position_y) >= 6) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 24U);
        set_high_word(workspace_.packed_actor_state, 0U);
        workspace_.position_x = 0U;
        workspace_.position_y = 0U;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        return finish(1U);
    }
    const std::size_t segment = workspace_.position_y;
    const std::size_t base = segment * 2U;
    const auto curve = sample_legacy_battle_script_curve(
        static_cast<float>(signed_word(workspace_.position_x)),
        {{
            {std::bit_cast<i16>(workspace_.list_words[base]),
             std::bit_cast<i16>(workspace_.list_words[base + 1U])},
            {std::bit_cast<i16>(workspace_.list_words[base + 2U]),
             std::bit_cast<i16>(workspace_.list_words[base + 3U])},
            {std::bit_cast<i16>(workspace_.list_words[base + 4U]),
             std::bit_cast<i16>(workspace_.list_words[base + 5U])},
            {std::bit_cast<i16>(workspace_.list_words[base + 6U]),
             std::bit_cast<i16>(workspace_.list_words[base + 7U])},
        }}
    );
    workspace_.value_a = curve.x;
    workspace_.value_b = curve.y;
    workspace_.coordinate_x = curve.x;
    workspace_.coordinate_y = curve.y;
    eax_ = curve.return_value;
    ecx_ = 0x0053CCE8U;
    edx_ = 0x0053CCECU;
    const i32 actor =
        static_cast<i32>(high_word(workspace_.packed_actor_state) & 0x7FFFU);
    const auto token = actor_token(actor);
    if (!token.has_value()) {
        return finish(eax_);
    }
    const auto address = script_actor_address(static_cast<u16>(actor));
    if (!publish_actor_coordinates(
            *token,
            std::bit_cast<u32>(workspace_.coordinate_x),
            std::bit_cast<u32>(workspace_.coordinate_y),
            address.selected_call_eax,
            address.coordinate_eax,
            publication_esi,
            entry_edi_,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    ++workspace_.position_x;
    if (workspace_.position_x > 20U) {
        ++workspace_.position_y;
        workspace_.position_x = 0U;
    }
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty() {
    u16 target{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), target)) {
        return finish();
    }
    workspace_.position_y = target;
    if (workspace_.position_x == 0U) {
        workspace_.position_x = target;
    }
    i32 delta{};
    const i32 signed_target = signed_word(target);
    if (signed_target >= 0 && signed_target <= 16) {
        const auto token = actor_token(signed_target);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const auto address = script_actor_address(target);
        const u32 query_entry_edx = target > 7U ? edx_ : address.coordinate_edx;
        if (!query_actor_current_coordinate_words(
                0x0046C8AAU,
                *token,
                workspace_.pair_x,
                workspace_.pair_y,
                kLegacyBattleScriptPairXToken,
                kLegacyBattleScriptPairYToken,
                address.coordinate_eax,
                query_entry_edx,
                address.coordinate_flags
            )) {
            return finish(eax_);
        }
        const i32 x = signed_word(workspace_.pair_x);
        const i32 dividend = x + 320;
        edx_ = dividend < 0 ? 0xFFFFFFFFU : 0U;
        const i32 current = dividend / 2;
        workspace_.position_x = static_cast<u16>(current);
        delta = 320 - current;
        eax_ = std::bit_cast<u32>(delta);
    } else {
        const i32 current = signed_word(workspace_.position_x);
        const i32 quotient = current / 3;
        workspace_.position_x = static_cast<u16>(quotient);
        edx_ = std::bit_cast<u32>(quotient);
        delta = -quotient;
        eax_ = std::bit_cast<u32>(delta);
    }
    if (delta == 0) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.position_x = 0U;
        set_high_word(workspace_.packed_value_a, 0U);
        workspace_.word_a = 0U;
        workspace_.position_y = 0U;
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }
    bindings_.shared.frame_gate = 0U;
    i32 index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_a_count)) {
        const auto token = group_a_token(index + 8);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count_eax = bindings_.startup.actor_metrics.group_a_count;
        const auto query_flags = index == 0
            ? subtract_flags(count_eax, 1U)
            : subtract_flags(static_cast<u32>(index), count_eax);
        if (!query_actor_current_coordinate_words(
                0x0046C929U,
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
        const u16 add_right = static_cast<u16>(delta);
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
    index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_b_count)) {
        const auto token = group_b_token(index);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count_eax = bindings_.startup.actor_metrics.group_b_count;
        const auto query_flags = index == 0
            ? subtract_flags(count_eax, 1U)
            : subtract_flags(static_cast<u32>(index), count_eax);
        if (!query_actor_current_coordinate_words(
                0x0046C97DU,
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
        const u16 add_right = static_cast<u16>(delta);
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
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    if (!run_frame()) {
        return finish(eax_);
    }

    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_one() {
    bindings_.shared.script_phase_gate = 0U;
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    invoke(LegacyBattleScriptDispatchCall::visual_transition, 0U, {value});
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    bindings_.shared.script_phase_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_two() {
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    workspace_.text_offset = workspace_.cursor;
    workspace_.short_text.fill(0U);
    u32 length{};
    if (!scan_percent_q(workspace_.text_offset, 32U, length)) {
        return finish();
    }
    const bool found = length < 32U;
    if (found) {
        for (u32 index = 0U; index < length; ++index) {
            u8 value{};
            if (!read_u8(wrapping_add(workspace_.text_offset, index), value)) {
                return finish();
            }
            workspace_.short_text[index] = value;
        }
        length += 2U;
    }
    for (u16 name_index = 0U; name_index < 4U; ++name_index) {
        invoke(LegacyBattleScriptDispatchCall::compare_text, 0U, {name_index});
        if (eax_ == 0U) {
            invoke(
                LegacyBattleScriptDispatchCall::dispatch_named_text,
                0U,
                {name_index}
            );
            break;
        }
    }
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.text_offset, length);
    workspace_.position_x = 0U;
    bindings_.shared.frame_gate = 1U;
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_three() {
    bindings_.shared.published_group_b_count =
        static_cast<u8>(bindings_.startup.actor_metrics.group_b_count);
    bindings_.shared.published_group_b_aux = 0U;
    bindings_.shared.script_phase_gate = 1U;
    bindings_.message_state = 99U;
    bindings_.input_dispatch.selected_actor_cleanup_gate = 0U;
    bindings_.message_phase.group_b_bypass_gate = 1U;
    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_four() {
    const auto created = create_dynamic_text(false, true);
    if (created.status != LegacyBattleScriptDispatchStatus::completed) {
        return created;
    }
    return finish_dynamic_text();
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_five() {
    if (bindings_.startup.mirror_mode == 1U) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        bindings_.startup.mirror_mode = 0U;
        return finish(1U);
    }
    bindings_.startup.mirror_mode = 1U;
    i32 index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_b_count)) {
        const auto token = group_b_token(index);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 count = bindings_.startup.actor_metrics.group_b_count;
        eax_ = count;
        ecx_ = *token;
        flags_ = index == 0 ? test_flags(count)
                            : subtract_flags(static_cast<u32>(index), count);
        invoke(LegacyBattleScriptDispatchCall::pending_47f900, *token, {1U});
        if (!query_actor_current_coordinate_words(
                0x0046BA42U,
                *token,
                workspace_.pair_x,
                workspace_.pair_y,
                kLegacyBattleScriptPairXToken,
                kLegacyBattleScriptPairYToken,
                eax_,
                edx_,
                flags_
            )) {
            return finish(eax_);
        }
        const u32 packed_pair = static_cast<u32>(workspace_.pair_x) |
            (static_cast<u32>(workspace_.pair_y) << 16U);
        const u32 mirrored = 640U - packed_pair;
        workspace_.pair_x = low_word(mirrored);
        const u32 value_y =
            with_low_word(kLegacyBattleScriptPairYToken, workspace_.pair_y);
        if (!publish_actor_coordinates(
                *token,
                mirrored,
                value_y,
                mirrored,
                kLegacyBattleScriptPairXToken,
                *token,
                std::bit_cast<u32>(index),
                subtract_flags(640U, packed_pair)
            )) {
            return finish(eax_);
        }
        ++index;
    }
    index = 0;
    while (index < static_cast<i32>(
                       bindings_.startup.actor_metrics.group_a_count)) {
        const auto token = group_a_token(index + 8);
        if (!token.has_value()) {
            return finish(eax_);
        }
        const u32 actor_skip =
            bindings_.victory
                .group_a_skip_secondary[static_cast<std::size_t>(index)];
        const u32 argument = actor_skip == 1U ? 0U : 1U;
        eax_ = bindings_.startup.actor_metrics.group_a_count;
        ecx_ = *token;
        flags_ = subtract_flags(actor_skip, 1U);
        invoke(
            LegacyBattleScriptDispatchCall::pending_47f900, *token, {argument}
        );
        if (!query_actor_current_coordinate_words(
                0x0046BAB5U,
                *token,
                workspace_.pair_x,
                workspace_.pair_y,
                kLegacyBattleScriptPairXToken,
                kLegacyBattleScriptPairYToken,
                eax_,
                edx_,
                flags_
            )) {
            return finish(eax_);
        }
        const u32 packed_pair = static_cast<u32>(workspace_.pair_x) |
            (static_cast<u32>(workspace_.pair_y) << 16U);
        const u32 mirrored = 640U - packed_pair;
        workspace_.pair_x = low_word(mirrored);
        const u32 value_y = with_low_word(packed_pair, workspace_.pair_y);
        if (!publish_actor_coordinates(
                *token,
                mirrored,
                value_y,
                mirrored,
                value_y,
                *token,
                kLegacyBattleScriptGroupAMirrorBaseToken +
                    std::bit_cast<u32>(index) * 8U,
                subtract_flags(640U, packed_pair)
            )) {
            return finish(eax_);
        }
        bindings_.shared.group_a_mirror_x[static_cast<std::size_t>(index)] =
            624U -
            bindings_.shared.group_a_mirror_x[static_cast<std::size_t>(index)];
        ++index;
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_six() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    invoke(
        LegacyBattleScriptDispatchCall::random_bounded_tertiary,
        0U,
        {std::bit_cast<u32>(signed_word(value))}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_seven() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    invoke(
        LegacyBattleScriptDispatchCall::random_bounded_quaternary,
        0U,
        {std::bit_cast<u32>(signed_word(value))}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_eight() {
    u16 value{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
        return finish();
    }
    invoke(
        LegacyBattleScriptDispatchCall::random_bounded_secondary,
        0U,
        {std::bit_cast<u32>(signed_word(value))}
    );
    workspace_.value_a = std::bit_cast<i32>(eax_);
    if (eax_ == 1U) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
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
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_forty_nine() {
    bindings_.shared.actor_mode_count = 0xFFU;
    bindings_.startup.actor_metrics.group_a_count = 0U;
    invoke(LegacyBattleScriptDispatchCall::actor_metrics);
    workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    const auto token = group_b_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    const auto address = script_actor_address(actor);
    eax_ = address.coordinate_eax;
    ecx_ = *token;
    edx_ = address.coordinate_edx;
    flags_ = address.coordinate_flags;
    invoke(LegacyBattleScriptDispatchCall::pending_47f910, *token);
    set_high_word(workspace_.packed_value_a, low_word(eax_));
    const auto first = group_b_token(0);
    if (!first.has_value()) {
        return finish(eax_);
    }
    if (!query_actor_current_coordinate_dwords(
            0x0046CD72U,
            *first,
            workspace_.value_a,
            workspace_.value_b,
            eax_,
            edx_,
            flags_
        )) {
        return finish(eax_);
    }
    const u32 value_x = with_low_word(
        kLegacyBattleScriptCoordinateXToken,
        low_word(std::bit_cast<u32>(workspace_.value_a))
    );
    const u32 value_y = with_low_word(
        kLegacyBattleScriptCoordinateYToken,
        low_word(std::bit_cast<u32>(workspace_.value_b))
    );
    if (!publish_actor_coordinates(
            *token,
            value_x,
            value_y,
            address.coordinate_eax,
            address.coordinate_edx,
            entry_esi_,
            entry_edi_,
            address.coordinate_flags
        )) {
        return finish(eax_);
    }
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    set_high_word(workspace_.packed_value_a, 0U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_one() {
    u16 actor{};
    u8 parameter{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
        !read_u8(wrapping_add(workspace_.cursor, 4U), parameter)) {
        return finish();
    }
    const u32 stale_parameter = (ecx_ & 0xFFFFFF00U) | parameter;
    const auto token = group_b_token(signed_word(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_47f3a0,
        *token,
        {stale_parameter}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_two() {
    u16 actor{};
    std::array<u16, 3> arguments{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                ),
                arguments[index]
            )) {
            return finish();
        }
    }
    const auto token = actor_token(static_cast<i32>(actor));
    if (!token.has_value()) {
        return finish(eax_);
    }
    invoke(
        LegacyBattleScriptDispatchCall::pending_47da10,
        *token,
        {arguments[0], arguments[1], arguments[2]}
    );
    workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_three() {
    u16 item_id{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id)) {
        return finish();
    }
    const u16 sample = bindings_.target_selection.transition_sample_word;
    invoke(
        LegacyBattleScriptDispatchCall::player_item_quantity, 0U, {item_id, 1U}
    );
    const std::size_t index = sample;
    if (index >= bindings_.victory.player_item_tokens.size()) {
        return stop(
            LegacyBattleScriptDispatchStatus::shared_state_typed_stop, sample
        );
    }
    bindings_.victory.player_item_tokens[index] = eax_;
    ++bindings_.victory.collected_item_quantities[index];
    bindings_.victory.collected_item_ids[index] = item_id;
    ++bindings_.target_selection.transition_sample_word;
    workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_four() {
    std::array<u16, 3> words{};
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
    workspace_.value_a = signed_word(words[0]);
    workspace_.value_b = signed_word(words[1]);
    workspace_.value_c = signed_word(words[2]);
    if (workspace_.value_b > 7) {
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }
    const i32 slot = workspace_.value_b;
    if (slot < 0 || slot >= 18) {
        return stop(
            LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
            static_cast<u32>(slot)
        );
    }
    bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] = 0U;
    i32 candidate = workspace_.value_c;
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
    }
    bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] =
        static_cast<u16>(candidate);
    const auto actor_token = group_b_token(workspace_.value_b);
    if (!actor_token.has_value()) {
        return finish(eax_);
    }
    LegacyBattleActorGroupBElementState* actor = nullptr;
    if (bindings_.startup.group_b_lifecycle != nullptr) {
        actor = &(
            *bindings_.startup.group_b_lifecycle
        )[static_cast<std::size_t>(workspace_.value_b)];
    }
    result_.group_b_action_profile_selection =
        select_legacy_battle_group_b_action_profile(
            actor,
            {
                .low_word =
                    &bindings_.shared
                         .actor_target_words[static_cast<std::size_t>(slot)],
                .high_word = &bindings_.shared.actor_target_words
                                  [static_cast<std::size_t>(slot) + 1U],
            },
            port_,
            {
                .selector_argument = std::bit_cast<u32>(workspace_.value_a),
                .output_token = 0x005028ACU + static_cast<u32>(slot) * 2U,
                .actor_token = *actor_token,
                .action_mode_requests = {
                    request_.actor_action_mode_requests
                        [result_.actor_action_mode_calls],
                    request_.actor_action_mode_requests
                        [result_.actor_action_mode_calls],
                },
            }
        );
    ++result_.group_b_action_profile_selection_calls;
    if (result_.group_b_action_profile_selection.mode_update_calls != 0U) {
        record_nested_actor_action_mode(
            result_.group_b_action_profile_selection.actor_action_mode
        );
    }
    eax_ = result_.group_b_action_profile_selection.return_eax;
    ecx_ = result_.group_b_action_profile_selection.return_ecx;
    edx_ = result_.group_b_action_profile_selection.return_edx;
    if (result_.group_b_action_profile_selection.status !=
        LegacyBattleGroupBActionProfileSelectionStatus::completed) {
        result_.status = LegacyBattleScriptDispatchStatus::
            group_b_action_profile_selection_typed_stop;
        return finish(eax_);
    }
    const u16 mask = eax_ == 1U ? 0x8000U : 0x4000U;
    bindings_.shared
        .actor_target_words[static_cast<std::size_t>(slot)] = static_cast<u16>(
        bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] |
        mask
    );
    if (!insert_attack_order_direct(
            2U, std::bit_cast<u32>(workspace_.value_a), 0U
        )) {
        return finish(eax_);
    }

    bindings_.shared.frame_gate = 0U;
    if (!run_frame()) {
        return finish(eax_);
    }

    workspace_.value_a = 0;
    workspace_.value_b = 0;
    workspace_.value_c = 0;
    bindings_.shared.frame_gate = 1U;
    workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
    return finish(1U);
}

LegacyBattleScriptDispatchResult ScriptRunner::case_fifty_five() {
    u16 actor{};
    if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
        return finish();
    }
    workspace_.value_a = signed_word(actor);
    eax_ = std::bit_cast<u32>(workspace_.value_a) - 8U;

    u16 argument{};
    if (!read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
        return finish(eax_);
    }
    workspace_.value_b = signed_word(argument);
    ecx_ = std::bit_cast<u32>(workspace_.value_b);

    ScriptPartyItemDefinitionPort item_port(*this);
    result_.party_item_definition = prepare_legacy_battle_party_item_definition(
        port_.world_item_list_state(),
        port_.battle_level_advancement_state().growth_caption_text,
        item_port,
        port_,
        {
            .party_index = eax_,
            .item_id = ecx_,
            .window_token = bindings_.startup.window_token,
            .entry_eax = eax_,
            .entry_ecx = ecx_,
            .entry_edx = edx_,
        }
    );
    ++result_.party_item_definition_calls;
    eax_ = result_.party_item_definition.return_eax;
    ecx_ = result_.party_item_definition.return_ecx;
    edx_ = result_.party_item_definition.return_edx;
    if (result_.party_item_definition.status !=
        LegacyBattlePartyItemDefinitionStatus::completed) {
        result_.status =
            LegacyBattleScriptDispatchStatus::party_item_definition_typed_stop;
        return finish(eax_);
    }
    if (eax_ == 1U) {
        bindings_.target_selection.transition_mode = 1U;
    }
    eax_ = 0U;
    workspace_.value_a = 0;
    workspace_.value_b = 0;
    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
    ecx_ = entry_ecx_;
    return finish(1U);
}

}  // namespace openswd3::battle::detail
