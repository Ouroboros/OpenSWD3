#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_set_and_clear_role_wait_override(openswd3::test::Context& test) {
    Fixture assigned;
    assigned.roles[1].action.wait_remaining = 9U;
    auto assigned_script = std::span<u8>{assigned.ports.initial_window};
    write_u16(assigned_script, 0U, OP_77_SET_ROLE_WAIT_OVERRIDE);
    write_u16(assigned_script, 2U, 0xFFF0U);
    write_u16(assigned_script, 4U, 3U);
    write_u16(assigned_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(assigned_script, 8U, 0x00F8U);
    const auto assigned_result = assigned.step();

    Fixture cleared;
    cleared.roles[1].action.wait_override = 0x8123U;
    cleared.roles[1].action.wait_remaining = 9U;
    auto cleared_script = std::span<u8>{cleared.ports.initial_window};
    write_u16(cleared_script, 0U, OP_78_CLEAR_ROLE_WAIT_OVERRIDE);
    write_u16(cleared_script, 2U, 0x00F8U);
    write_u16(cleared_script, 4U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(cleared_script, 6U, 0x00F8U);
    const auto cleared_result = cleared.step();

    Fixture missing;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, OP_77_SET_ROLE_WAIT_OVERRIDE);
    write_u16(missing_script, 2U, 0x7777U);
    write_u16(missing_script, 4U, 5U);
    const auto missing_result = missing.step();

    test.expect_true(
        assigned_result.status == LegacyWorldStoryVmStatus::yielded &&
            assigned_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            assigned_result.executed_instruction_count == 2U &&
            assigned_result.action_update_count == 2U &&
            assigned.roles[1].action.wait_override == 0x8003U &&
            assigned.roles[1].action.wait_remaining == 0U &&
            assigned.context.instruction_offset == 10U &&
            assigned.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            cleared_result.status == LegacyWorldStoryVmStatus::yielded &&
            cleared_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            cleared_result.executed_instruction_count == 2U &&
            cleared_result.action_update_count == 2U &&
            cleared.roles[1].action.wait_override == 0U &&
            cleared.roles[1].action.wait_remaining == 0U &&
            cleared.context.instruction_offset == 8U &&
            cleared.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.instruction_offset == 0U &&
            missing_result.first_operand_available &&
            missing_result.first_operand_word == 0x7777U &&
            missing.context.instruction_offset == 0U,
        "opcodes 77 and 78 refresh the role wait override while an unresolved " "selector preserves the undefined-width instruction boundary"
    );
}

void test_role_wait_override_lookup_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u16, 2U> opcodes{
        OP_77_SET_ROLE_WAIT_OVERRIDE,
        OP_78_CLEAR_ROLE_WAIT_OVERRIDE,
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 opcode : opcodes) {
        for (const u16 alias_mask : alias_masks) {
            Fixture missing;
            missing.context.talk_data_offset = 0x1111U;
            missing.context.instruction_offset = 0x7FFCU;
            missing.state.loaded_file_number = 1U;
            missing.state.loaded_data_offset = 0x1111U;
            missing.state.window_loaded = true;
            missing.state.previous_opcode = 0x66U;
            write_u16(
                missing.state.window,
                0x7FFCU,
                static_cast<u16>(opcode | alias_mask)
            );
            write_u16(missing.state.window, 0x7FFEU, 0xFFFFU);

            const auto missing_result = missing.step();

            test.expect_true(
                missing_result.status ==
                        LegacyWorldStoryVmStatus::role_not_found &&
                    missing_result.opcode == opcode &&
                    missing.context.instruction_offset == 0x7FFCU &&
                    missing.state.previous_opcode == 0x66U,
                "opcode 77/78 aliases typed-stop an unresolved selector before using the original stale-width advance"
            );
        }
    }

    Fixture payload_truncated;
    payload_truncated.context.talk_data_offset = 0x1111U;
    payload_truncated.context.instruction_offset = 0x7FFCU;
    payload_truncated.state.loaded_file_number = 1U;
    payload_truncated.state.loaded_data_offset = 0x1111U;
    payload_truncated.state.window_loaded = true;
    payload_truncated.state.previous_opcode = 0x66U;
    write_u16(
        payload_truncated.state.window, 0x7FFCU, OP_77_SET_ROLE_WAIT_OVERRIDE
    );
    write_u16(payload_truncated.state.window, 0x7FFEU, 0x00F8U);

    const auto payload_truncated_result = payload_truncated.step();

    test.expect_true(
        payload_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            payload_truncated.roles[1].action.wait_override == 0U &&
            payload_truncated.context.instruction_offset == 0x7FFCU &&
            payload_truncated.state.previous_opcode == 0x66U,
        "opcode 77 reads its payload only after selector lookup succeeds"
    );
}

void test_role_wait_override_exact_tails(openswd3::test::Context& test) {
    Fixture assigned;
    assigned.roles[1].action.wait_remaining = 9U;
    assigned.context.talk_data_offset = 0x1111U;
    assigned.context.instruction_offset = 0x7FFAU;
    assigned.state.loaded_file_number = 1U;
    assigned.state.loaded_data_offset = 0x1111U;
    assigned.state.window_loaded = true;
    assigned.state.previous_opcode = 0x66U;
    write_u16(
        assigned.state.window,
        0x7FFAU,
        static_cast<u16>(OP_77_SET_ROLE_WAIT_OVERRIDE | 0x8000U)
    );
    write_u16(assigned.state.window, 0x7FFCU, 0xFFF0U);
    write_u16(assigned.state.window, 0x7FFEU, 0xFFFFU);

    const auto assigned_result = assigned.step();

    test.expect_true(
        assigned_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            assigned_result.opcode == OP_77_SET_ROLE_WAIT_OVERRIDE &&
            assigned_result.executed_instruction_count == 1U &&
            assigned_result.action_update_count == 1U &&
            assigned.roles[1].action.wait_override == 0xFFFFU &&
            assigned.roles[1].action.wait_remaining == 0U &&
            assigned.context.instruction_offset == 0x8000U &&
            assigned.state.previous_opcode == OP_77_SET_ROLE_WAIT_OVERRIDE,
        "opcode 77 exact tail writes the flagged override, refreshes and publishes before next fetch fails"
    );

    Fixture cleared;
    cleared.roles[1].action.wait_override = 0x8123U;
    cleared.roles[1].action.wait_remaining = 9U;
    cleared.context.talk_data_offset = 0x1111U;
    cleared.context.instruction_offset = 0x7FFCU;
    cleared.state.loaded_file_number = 1U;
    cleared.state.loaded_data_offset = 0x1111U;
    cleared.state.window_loaded = true;
    cleared.state.previous_opcode = 0x66U;
    write_u16(
        cleared.state.window,
        0x7FFCU,
        static_cast<u16>(OP_78_CLEAR_ROLE_WAIT_OVERRIDE | 0xC000U)
    );
    write_u16(cleared.state.window, 0x7FFEU, 0xFFFEU);

    const auto cleared_result = cleared.step(0, 0, 1U);

    test.expect_true(
        cleared_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            cleared_result.opcode == OP_78_CLEAR_ROLE_WAIT_OVERRIDE &&
            cleared_result.executed_instruction_count == 1U &&
            cleared_result.action_update_count == 1U &&
            cleared.roles[1].action.wait_override == 0U &&
            cleared.roles[1].action.wait_remaining == 0U &&
            cleared.context.instruction_offset == 0x8000U &&
            cleared.state.previous_opcode == OP_78_CLEAR_ROLE_WAIT_OVERRIDE,
        "opcode 78 exact tail clears the controlled-role override, refreshes and publishes before next fetch fails"
    );
}

void test_clear_text_control_bit29(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        exact_tail.state.text_control_flags = 0xFFFFFFFFU;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_80_CLEAR_TEXT_CONTROL_BIT29 | alias_mask)
        );

        const auto exact_result = exact_tail.step();

        test.expect_true(
            exact_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_result.opcode == OP_80_CLEAR_TEXT_CONTROL_BIT29 &&
                exact_result.executed_instruction_count == 1U &&
                exact_tail.state.text_control_flags == 0xDFFFFFFFU &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_80_CLEAR_TEXT_CONTROL_BIT29,
            "opcode 80 aliases clear only text-control bit 29 before exact-tail next fetch failure"
        );
    }

    Fixture ordinary;
    ordinary.state.text_control_flags = 0xA0000000U;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_u16(script, 0U, OP_80_CLEAR_TEXT_CONTROL_BIT29);
    write_u16(script, 2U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 4U, 0x00F8U);

    const auto ordinary_result = ordinary.step();

    test.expect_true(
        ordinary_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ordinary_result.executed_instruction_count == 2U &&
            ordinary.state.text_control_flags == 0x80000000U &&
            ordinary.context.instruction_offset == 6U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 80 preserves other text-control bits and continues in the same call"
    );
}

void test_enqueue_role_head_action_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode,
                                 const u16 target_x,
                                 const u16 encoded_y) {
        write_u16(bytes, offset, raw_opcode);
        write_u16(bytes, offset + 2U, 0x2711U);
        write_u16(bytes, offset + 4U, 9U);
        write_u16(bytes, offset + 6U, target_x);
        write_u16(bytes, offset + 8U, encoded_y);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        openswd3::world_map::LegacyRoleHeadActionList actions;
        exact_tail.runtime.role_head_actions = &actions;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FF6U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            0x7FF6U,
            static_cast<u16>(OP_81_ENQUEUE_ROLE_HEAD_ACTION | alias_mask),
            50U,
            0x8078U
        );

        const auto result = exact_tail.step();
        const auto& node = actions.front();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_81_ENQUEUE_ROLE_HEAD_ACTION &&
                result.executed_instruction_count == 1U &&
                actions.size() == 1U && node.action.action_id == 0x2711U &&
                node.action.base_variant == 9U &&
                node.action.variant_delta == 0U && node.target_x == 50 &&
                node.y == 120 && node.current_x == 50 &&
                node.horizontal_motion == std::bit_cast<i16>(u16{0x8000U}) &&
                node.next_pointer_32 == 0U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_81_ENQUEUE_ROLE_HEAD_ACTION,
            "opcode 81 aliases apply the bit15 special start before exact-tail next fetch failure"
        );
    }

    Fixture ordinary;
    openswd3::world_map::LegacyRoleHeadActionList actions;
    actions.emplace_back();
    actions.back().action.action_id = 0x9999U;
    ordinary.runtime.role_head_actions = &actions;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_record(script, 0U, OP_81_ENQUEUE_ROLE_HEAD_ACTION, 321U, 120U);
    write_u16(script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 12U, 0x00F8U);
    const auto ordinary_result = ordinary.step();
    test.expect_true(
        ordinary_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ordinary_result.executed_instruction_count == 2U &&
            actions.size() == 2U && actions.front().target_x == 321 &&
            actions.front().current_x == 760 &&
            actions.front().horizontal_motion == 0 &&
            actions.back().action.action_id == 0x9999U &&
            ordinary.context.instruction_offset == 14U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 81 starts signed X above 320 at 760, prepends and continues in the same call"
    );

    constexpr std::array<std::pair<u16, i16>, 2U> left_cases{
        std::pair<u16, i16>{320U, -120},
        std::pair<u16, i16>{0xFFFFU, -120},
    };
    for (const auto [target_x, expected_start] : left_cases) {
        Fixture fixture;
        openswd3::world_map::LegacyRoleHeadActionList left_actions;
        fixture.runtime.role_head_actions = &left_actions;
        auto left_script = std::span<u8>{fixture.ports.initial_window};
        write_record(
            left_script, 0U, OP_81_ENQUEUE_ROLE_HEAD_ACTION, target_x, 0x4567U
        );
        write_u16(left_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
        write_u16(left_script, 12U, 0x00F8U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                left_actions.front().target_x == std::bit_cast<i16>(target_x) &&
                left_actions.front().current_x == expected_start &&
                left_actions.front().y == 0x4567,
            "opcode 81 uses signed target X and the inclusive 320 left-start boundary"
        );
    }
}

void test_enqueue_role_head_action_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> operands{0x2711U, 9U, 50U, 0x8078U};
    for (std::size_t available = 0U; available < operands.size(); ++available) {
        Fixture truncated;
        openswd3::world_map::LegacyRoleHeadActionList actions;
        truncated.runtime.role_head_actions = &actions;
        const u16 offset =
            static_cast<u16>(0x8000U - (available + 1U) * sizeof(u16));
        truncated.context.talk_data_offset = 0x1111U;
        truncated.context.instruction_offset = offset;
        truncated.state.loaded_file_number = 1U;
        truncated.state.loaded_data_offset = 0x1111U;
        truncated.state.window_loaded = true;
        truncated.state.previous_opcode = 0x66U;
        write_u16(
            truncated.state.window, offset, OP_81_ENQUEUE_ROLE_HEAD_ACTION
        );
        for (std::size_t index = 0U; index < available; ++index) {
            write_u16(
                truncated.state.window,
                static_cast<std::size_t>(offset) + 2U + index * 2U,
                operands[index]
            );
        }
        const auto result = truncated.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                actions.empty() &&
                truncated.context.instruction_offset == offset &&
                truncated.state.previous_opcode == 0x66U,
            "opcode 81 truncations release the temporary unlinked node and preserve IP/previous"
        );
    }

    Fixture unavailable;
    auto script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(script, 0U, OP_81_ENQUEUE_ROLE_HEAD_ACTION);
    for (std::size_t index = 0U; index < operands.size(); ++index) {
        write_u16(script, 2U + index * 2U, operands[index]);
    }
    unavailable.state.previous_opcode = 0x66U;
    const auto result = unavailable.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 81 owner absence occurs after complete node initialization but before insertion"
    );
}

void test_dismiss_role_head_action_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        openswd3::world_map::LegacyRoleHeadActionList actions(2U);
        auto first = actions.begin();
        first->action.action_id = 0x2711U;
        first->action.base_variant = 0U;
        first->current_x = 100;
        first->horizontal_motion = 0;
        auto second = std::next(first);
        second->action.action_id = 0x2711U;
        second->action.base_variant = 0U;
        second->current_x = 500;
        second->horizontal_motion = 77;
        exact_tail.runtime.role_head_actions = &actions;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFAU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(
            exact_tail.state.window,
            0x7FFAU,
            static_cast<u16>(OP_82_DISMISS_ROLE_HEAD_ACTION | alias_mask)
        );
        write_u16(exact_tail.state.window, 0x7FFCU, 0x2711U);
        write_u16(exact_tail.state.window, 0x7FFEU, 0U);

        const auto result = exact_tail.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_82_DISMISS_ROLE_HEAD_ACTION &&
                result.executed_instruction_count == 1U &&
                first->horizontal_motion == -1 &&
                second->horizontal_motion == 77 &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_82_DISMISS_ROLE_HEAD_ACTION,
            "opcode 82 aliases update only the first matching head action before exact-tail next fetch failure"
        );
    }

    Fixture ordinary;
    openswd3::world_map::LegacyRoleHeadActionList actions(2U);
    auto special = actions.begin();
    special->action.action_id = 0x2711U;
    special->action.base_variant = 9U;
    special->current_x = 50;
    special->horizontal_motion = std::bit_cast<i16>(u16{0x8000U});
    auto right = std::next(special);
    right->action.action_id = 0x2712U;
    right->action.base_variant = 2U;
    right->current_x = 321;
    right->horizontal_motion = 0;
    ordinary.runtime.role_head_actions = &actions;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_u16(script, 0U, OP_82_DISMISS_ROLE_HEAD_ACTION);
    write_u16(script, 2U, 0x2711U);
    write_u16(script, 4U, 9U);
    write_u16(script, 6U, OP_82_DISMISS_ROLE_HEAD_ACTION);
    write_u16(script, 8U, 0x2712U);
    write_u16(script, 10U, 2U);
    write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 14U, 0x00F8U);

    const auto ordinary_result = ordinary.step();

    test.expect_true(
        ordinary_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ordinary_result.executed_instruction_count == 3U &&
            special->horizontal_motion == 10000 &&
            right->horizontal_motion == 1 &&
            ordinary.context.instruction_offset == 16U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 82 preserves bit15 dismissal, signed X right dismissal and same-call continuation"
    );
}

void test_dismiss_role_head_action_boundaries(openswd3::test::Context& test) {
    Fixture empty;
    openswd3::world_map::LegacyRoleHeadActionList empty_actions;
    empty.runtime.role_head_actions = &empty_actions;
    empty.context.talk_data_offset = 0x1111U;
    empty.context.instruction_offset = 0x7FFEU;
    empty.state.loaded_file_number = 1U;
    empty.state.loaded_data_offset = 0x1111U;
    empty.state.window_loaded = true;
    empty.state.previous_opcode = 0x66U;
    write_u16(empty.state.window, 0x7FFEU, OP_82_DISMISS_ROLE_HEAD_ACTION);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            empty.context.instruction_offset == 0x8004U &&
            empty.state.previous_opcode == OP_82_DISMISS_ROLE_HEAD_ACTION,
        "opcode 82 empty list consumes six bytes without reading either operand"
    );

    Fixture id_miss;
    openswd3::world_map::LegacyRoleHeadActionList miss_actions(1U);
    miss_actions.front().action.action_id = 0x9999U;
    miss_actions.front().horizontal_motion = 55;
    id_miss.runtime.role_head_actions = &miss_actions;
    id_miss.context.talk_data_offset = 0x1111U;
    id_miss.context.instruction_offset = 0x7FFCU;
    id_miss.state.loaded_file_number = 1U;
    id_miss.state.loaded_data_offset = 0x1111U;
    id_miss.state.window_loaded = true;
    id_miss.state.previous_opcode = 0x66U;
    write_u16(id_miss.state.window, 0x7FFCU, OP_82_DISMISS_ROLE_HEAD_ACTION);
    write_u16(id_miss.state.window, 0x7FFEU, 0x2711U);
    const auto miss_result = id_miss.step();
    test.expect_true(
        miss_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            miss_actions.front().horizontal_motion == 55 &&
            id_miss.context.instruction_offset == 0x8002U &&
            id_miss.state.previous_opcode == OP_82_DISMISS_ROLE_HEAD_ACTION,
        "opcode 82 ID miss does not read the absent variant and silently advances six bytes"
    );

    Fixture variant_truncated;
    openswd3::world_map::LegacyRoleHeadActionList matching_actions(1U);
    matching_actions.front().action.action_id = 0x2711U;
    variant_truncated.runtime.role_head_actions = &matching_actions;
    variant_truncated.context.talk_data_offset = 0x1111U;
    variant_truncated.context.instruction_offset = 0x7FFCU;
    variant_truncated.state.loaded_file_number = 1U;
    variant_truncated.state.loaded_data_offset = 0x1111U;
    variant_truncated.state.window_loaded = true;
    variant_truncated.state.previous_opcode = 0x66U;
    write_u16(
        variant_truncated.state.window, 0x7FFCU, OP_82_DISMISS_ROLE_HEAD_ACTION
    );
    write_u16(variant_truncated.state.window, 0x7FFEU, 0x2711U);
    const auto truncated_result = variant_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            variant_truncated.context.instruction_offset == 0x7FFCU &&
            variant_truncated.state.previous_opcode == 0x66U,
        "opcode 82 reads the variant only after the first action-ID match"
    );

    Fixture unavailable;
    auto script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(script, 0U, OP_82_DISMISS_ROLE_HEAD_ACTION);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 82 typed owner absence stops before original global-head access"
    );
}

void test_upsert_packed_row_effect_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode,
                                 const u16 effect_id,
                                 const u16 color,
                                 const u16 mode,
                                 const u16 x,
                                 const u16 y,
                                 const u16 width,
                                 const u16 height) {
        write_u16(bytes, offset, raw_opcode);
        write_u16(bytes, offset + 2U, effect_id);
        write_u16(bytes, offset + 4U, color);
        write_u16(bytes, offset + 6U, mode);
        write_u16(bytes, offset + 8U, x);
        write_u16(bytes, offset + 10U, y);
        write_u16(bytes, offset + 12U, width);
        write_u16(bytes, offset + 14U, height);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
            {.mode = 0x0805U},
            {.mode = 0x4005U},
            {.mode = 0x8006U},
        };
        exact_tail.runtime.packed_row_effects = &effects;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FF0U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            0x7FF0U,
            static_cast<u16>(OP_83_UPSERT_PACKED_ROW_EFFECT | alias_mask),
            5U,
            9U,
            1U,
            131U,
            11U,
            383U,
            5U
        );

        const auto result = exact_tail.step();
        const auto& effect = effects.front();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_83_UPSERT_PACKED_ROW_EFFECT &&
                result.executed_instruction_count == 1U &&
                effects.size() == 2U && effect.base_x == 130 &&
                effect.base_y == 10 && effect.limit == 382 &&
                effect.row_count == 4 && effect.mode == 0x4005U &&
                effect.color_index == 9 && effect.row_offsets.size() == 4U &&
                std::ranges::all_of(
                    effect.row_offsets,
                    [](const i16 value) { return value == 380; }
                ) &&
                std::ranges::all_of(
                    effect.row_lengths,
                    [](const i16 value) { return value == 2; }
                ) &&
                std::next(effects.begin())->mode == 0x8006U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_83_UPSERT_PACKED_ROW_EFFECT,
            "opcode 83 aliases remove every matching low-byte ID and prepend an even-masked mode-1 effect before exact-tail failure"
        );
    }

    Fixture ordinary;
    std::list<openswd3::rendering::LegacyPackedRowEffect> effects;
    ordinary.runtime.packed_row_effects = &effects;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_record(
        script, 0U, OP_83_UPSERT_PACKED_ROW_EFFECT, 1U, 3U, 2U, 10U, 20U, 8U, 2U
    );
    write_record(
        script,
        16U,
        OP_83_UPSERT_PACKED_ROW_EFFECT,
        2U,
        4U,
        11U,
        30U,
        40U,
        10U,
        2U
    );
    write_u16(script, 32U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 34U, 0x00F8U);

    const auto result = ordinary.step();
    const auto first = effects.begin();
    const auto second = std::next(first);

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 3U && effects.size() == 2U &&
            first->mode == 0x8002U &&
            std::ranges::all_of(
                first->row_offsets, [](const i16 value) { return value == 0; }
            ) &&
            second->mode == 0x0801U &&
            std::ranges::all_of(
                second->row_offsets, [](const i16 value) { return value == 0; }
            ) &&
            ordinary.context.instruction_offset == 36U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 83 initializes mode 2/simple and default/grow records and continues in the same call"
    );
}

void test_upsert_packed_row_effect_boundaries(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 effect_id,
                                 const u16 x,
                                 const u16 y,
                                 const u16 width,
                                 const u16 height) {
        write_u16(bytes, offset, OP_83_UPSERT_PACKED_ROW_EFFECT);
        write_u16(bytes, offset + 2U, effect_id);
        write_u16(bytes, offset + 4U, 9U);
        write_u16(bytes, offset + 6U, 0U);
        write_u16(bytes, offset + 8U, x);
        write_u16(bytes, offset + 10U, y);
        write_u16(bytes, offset + 12U, width);
        write_u16(bytes, offset + 14U, height);
    };

    Fixture invalid_id;
    invalid_id.context.talk_data_offset = 0x1111U;
    invalid_id.context.instruction_offset = 0x7FFCU;
    invalid_id.state.loaded_file_number = 1U;
    invalid_id.state.loaded_data_offset = 0x1111U;
    invalid_id.state.window_loaded = true;
    invalid_id.state.previous_opcode = 0x66U;
    write_u16(invalid_id.state.window, 0x7FFCU, OP_83_UPSERT_PACKED_ROW_EFFECT);
    write_u16(invalid_id.state.window, 0x7FFEU, 0x0100U);
    const auto invalid_id_result = invalid_id.step();
    test.expect_true(
        invalid_id_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            invalid_id.context.instruction_offset == 0x800CU &&
            invalid_id.state.previous_opcode == OP_83_UPSERT_PACKED_ROW_EFFECT,
        "opcode 83 invalid ID consumes sixteen bytes without touching the list owner or later operands"
    );

    constexpr std::array<u16, 7U> operands{
        5U,
        9U,
        1U,
        130U,
        10U,
        382U,
        4U,
    };
    for (std::size_t available = 0U; available < operands.size(); ++available) {
        Fixture truncated;
        std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
            {.mode = 0x0805U},
            {.mode = 0x8006U},
        };
        truncated.runtime.packed_row_effects = &effects;
        const u16 offset =
            static_cast<u16>(0x8000U - (available + 1U) * sizeof(u16));
        truncated.context.talk_data_offset = 0x1111U;
        truncated.context.instruction_offset = offset;
        truncated.state.loaded_file_number = 1U;
        truncated.state.loaded_data_offset = 0x1111U;
        truncated.state.window_loaded = true;
        truncated.state.previous_opcode = 0x66U;
        write_u16(
            truncated.state.window, offset, OP_83_UPSERT_PACKED_ROW_EFFECT
        );
        for (std::size_t index = 0U; index < available; ++index) {
            write_u16(
                truncated.state.window,
                static_cast<std::size_t>(offset) + 2U + index * 2U,
                operands[index]
            );
        }

        const auto result = truncated.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                effects.size() == (available == 0U ? 2U : 1U) &&
                effects.front().mode == (available == 0U ? 0x0805U : 0x8006U) &&
                truncated.context.instruction_offset == offset &&
                truncated.state.previous_opcode == 0x66U,
            "opcode 83 truncations preserve the staged same-ID deletion before later operand reads"
        );
    }

    constexpr std::array<std::array<u16, 4U>, 4U> invalid_rectangles{
        std::array<u16, 4U>{0xFFFFU, 10U, 20U, 4U},
        std::array<u16, 4U>{10U, 0xFFFFU, 20U, 4U},
        std::array<u16, 4U>{630U, 10U, 12U, 4U},
        std::array<u16, 4U>{10U, 470U, 20U, 12U},
    };
    for (const auto& rectangle : invalid_rectangles) {
        Fixture invalid;
        std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
            {.mode = 0x0805U},
            {.mode = 0x8006U},
        };
        invalid.runtime.packed_row_effects = &effects;
        auto script = std::span<u8>{invalid.ports.initial_window};
        write_record(
            script,
            0U,
            5U,
            rectangle[0],
            rectangle[1],
            rectangle[2],
            rectangle[3]
        );
        write_u16(script, 16U, OP_14_WAIT_ROLE_ACTION_STATUS);
        write_u16(script, 18U, 0x00F8U);
        const auto result = invalid.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                effects.size() == 1U && effects.front().mode == 0x8006U &&
                invalid.context.instruction_offset == 20U,
            "opcode 83 invalid rectangles retain prior same-ID deletion, create no replacement and continue"
        );
    }

    Fixture non_positive;
    std::list<openswd3::rendering::LegacyPackedRowEffect> effects;
    non_positive.runtime.packed_row_effects = &effects;
    auto script = std::span<u8>{non_positive.ports.initial_window};
    write_record(script, 0U, 5U, 10U, 10U, 0U, 0U);
    write_record(script, 16U, 6U, 10U, 10U, 0xFFFEU, 0xFFFEU);
    write_u16(script, 32U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 34U, 0x00F8U);
    const auto non_positive_result = non_positive.step();
    test.expect_true(
        non_positive_result.status == LegacyWorldStoryVmStatus::yielded &&
            effects.size() == 2U && effects.front().limit == -2 &&
            effects.front().row_count == -2 &&
            effects.front().row_offsets.empty() &&
            effects.front().row_lengths.empty() &&
            std::next(effects.begin())->limit == 0 &&
            std::next(effects.begin())->row_count == 0,
        "opcode 83 keeps zero and negative dimensions because the original has no positive-size gate"
    );

    Fixture unavailable;
    auto unavailable_script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(unavailable_script, 0U, OP_83_UPSERT_PACKED_ROW_EFFECT);
    write_u16(unavailable_script, 2U, 5U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 83 typed owner absence stops at the original valid-ID list-head access"
    );
}

void test_control_packed_row_effect_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
            {.mode = 0x8005U},
            {.mode = 0x4005U},
        };
        exact_tail.runtime.packed_row_effects = &effects;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFAU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(
            exact_tail.state.window,
            0x7FFAU,
            static_cast<u16>(OP_84_CONTROL_PACKED_ROW_EFFECT | alias_mask)
        );
        write_u16(exact_tail.state.window, 0x7FFCU, 5U);
        write_u16(exact_tail.state.window, 0x7FFEU, 0U);

        const auto result = exact_tail.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_84_CONTROL_PACKED_ROW_EFFECT &&
                result.executed_instruction_count == 1U &&
                effects.size() == 2U && effects.front().mode == 0x2005U &&
                std::next(effects.begin())->mode == 0x4005U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_84_CONTROL_PACKED_ROW_EFFECT,
            "opcode 84 aliases replace only the first matching high mode before exact-tail next fetch failure"
        );
    }

    Fixture ordinary;
    std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
        {.mode = 0x8005U},
        {.mode = 0x4006U},
        {.mode = 0x0806U},
    };
    ordinary.runtime.packed_row_effects = &effects;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_u16(script, 0U, OP_84_CONTROL_PACKED_ROW_EFFECT);
    write_u16(script, 2U, 5U);
    write_u16(script, 4U, 1U);
    write_u16(script, 6U, OP_84_CONTROL_PACKED_ROW_EFFECT);
    write_u16(script, 8U, 6U);
    write_u16(script, 10U, 2U);
    write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 14U, 0x00F8U);

    const auto result = ordinary.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 3U && effects.size() == 2U &&
            effects.front().mode == 0x1005U &&
            std::next(effects.begin())->mode == 0x0806U &&
            ordinary.context.instruction_offset == 16U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 84 operation 1 replaces mode, operation 2 removes only the first match and execution continues"
    );
}

void test_control_packed_row_effect_boundaries(openswd3::test::Context& test) {
    const auto prime_tail = [](Fixture& fixture, const u16 effect_id) {
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        write_u16(
            fixture.state.window, 0x7FFCU, OP_84_CONTROL_PACKED_ROW_EFFECT
        );
        write_u16(fixture.state.window, 0x7FFEU, effect_id);
    };

    Fixture invalid_id;
    prime_tail(invalid_id, 0x0100U);
    const auto invalid_id_result = invalid_id.step();
    test.expect_true(
        invalid_id_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            invalid_id.context.instruction_offset == 0x8002U &&
            invalid_id.state.previous_opcode == OP_84_CONTROL_PACKED_ROW_EFFECT,
        "opcode 84 invalid ID consumes six bytes without touching the owner or operation"
    );

    Fixture empty;
    std::list<openswd3::rendering::LegacyPackedRowEffect> empty_effects;
    empty.runtime.packed_row_effects = &empty_effects;
    prime_tail(empty, 5U);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            empty.context.instruction_offset == 0x8002U &&
            empty.state.previous_opcode == OP_84_CONTROL_PACKED_ROW_EFFECT,
        "opcode 84 empty list does not read the absent operation and silently advances"
    );

    Fixture miss;
    std::list<openswd3::rendering::LegacyPackedRowEffect> miss_effects{
        {.mode = 0x8006U},
    };
    miss.runtime.packed_row_effects = &miss_effects;
    prime_tail(miss, 5U);
    const auto miss_result = miss.step();
    test.expect_true(
        miss_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            miss_effects.front().mode == 0x8006U &&
            miss.context.instruction_offset == 0x8002U,
        "opcode 84 ID miss does not read the absent operation"
    );

    Fixture operation_truncated;
    std::list<openswd3::rendering::LegacyPackedRowEffect> matching_effects{
        {.mode = 0x8005U},
    };
    operation_truncated.runtime.packed_row_effects = &matching_effects;
    prime_tail(operation_truncated, 5U);
    const auto truncated_result = operation_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            matching_effects.front().mode == 0x8005U &&
            operation_truncated.context.instruction_offset == 0x7FFCU &&
            operation_truncated.state.previous_opcode == 0x66U,
        "opcode 84 reads the operation only after the first ID match"
    );

    Fixture unavailable;
    auto unavailable_script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(unavailable_script, 0U, OP_84_CONTROL_PACKED_ROW_EFFECT);
    write_u16(unavailable_script, 2U, 5U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 84 typed owner absence stops at the original valid-ID list-head access"
    );

    Fixture unsupported;
    std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
        {.mode = 0x8002U},
    };
    unsupported.runtime.packed_row_effects = &effects;
    auto unsupported_script = std::span<u8>{unsupported.ports.initial_window};
    write_u16(unsupported_script, 0U, OP_84_CONTROL_PACKED_ROW_EFFECT);
    write_u16(unsupported_script, 2U, 2U);
    write_u16(unsupported_script, 4U, 3U);
    unsupported.state.previous_opcode = 0x66U;
    const auto unsupported_result = unsupported.step();
    test.expect_true(
        unsupported_result.status ==
                LegacyWorldStoryVmStatus::
                    unsupported_packed_row_effect_operation &&
            effects.front().mode == 0x8002U &&
            unsupported.context.instruction_offset == 0U &&
            unsupported.state.previous_opcode == 0x66U,
        "opcode 84 stale-var_44 operations typed-stop without inventing a replacement high mode"
    );
}

void test_begin_story_video_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode,
                                 const std::string_view filename) {
        write_u16(bytes, offset, raw_opcode);
        std::ranges::copy(
            filename, bytes.begin() + static_cast<std::ptrdiff_t>(offset + 2U)
        );
        bytes[offset + 2U + filename.size()] = 0x25U;
        bytes[offset + 3U + filename.size()] = 0x51U;
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::string_view kFilename{"clip.avi"};
    constexpr std::size_t kLength = 2U + kFilename.size() + 2U;
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        std::vector<u32> events;
        exact_tail.ports.framebuffer_clear_callback = [&]() {
            events.push_back(1U);
        };
        exact_tail.ports.framebuffer_present_callback = [&]() {
            events.push_back(2U);
        };
        exact_tail.ports.audio_service_callback = [&]() {
            events.push_back(3U);
        };
        exact_tail.ports.story_video_prepare_callback = [&]() {
            events.push_back(4U);
        };
        exact_tail.ports.story_video_begin_callback = [&]() {
            events.push_back(5U);
        };
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset =
            static_cast<u16>(0x8000U - kLength);
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            exact_tail.context.instruction_offset,
            static_cast<u16>(OP_85_BEGIN_STORY_VIDEO | alias_mask),
            kFilename
        );

        const auto result = exact_tail.step();
        const std::string filename{
            exact_tail.ports.last_video_filename.begin(),
            exact_tail.ports.last_video_filename.end(),
        };

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_85_BEGIN_STORY_VIDEO &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 2U &&
                events == std::vector<u32>{1U, 2U, 3U, 4U, 5U, 3U} &&
                exact_tail.ports.framebuffer_clear_count == 1U &&
                exact_tail.ports.framebuffer_present_count == 1U &&
                exact_tail.ports.direct_audio_service_count == 2U &&
                exact_tail.ports.video_prepare_count == 1U &&
                exact_tail.ports.video_begin_count == 1U &&
                filename == kFilename &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == OP_85_BEGIN_STORY_VIDEO,
            "opcode 85 aliases clear, present, service audio, prepare and begin in order before exact-tail yield"
        );
    }

    Fixture rejected;
    rejected.ports.video_prepare_success = false;
    auto rejected_script = std::span<u8>{rejected.ports.initial_window};
    write_record(rejected_script, 0U, OP_85_BEGIN_STORY_VIDEO, "ignored.mpg");
    rejected.state.previous_opcode = 0x66U;
    const auto rejected_result = rejected.step();
    test.expect_true(
        rejected_result.status == LegacyWorldStoryVmStatus::yielded &&
            rejected_result.direct_audio_service_count == 2U &&
            rejected.ports.framebuffer_clear_count == 1U &&
            rejected.ports.framebuffer_present_count == 1U &&
            rejected.ports.direct_audio_service_count == 2U &&
            rejected.ports.video_prepare_count == 1U &&
            rejected.ports.video_begin_count == 0U &&
            rejected.context.instruction_offset == 0U &&
            rejected.state.previous_opcode == OP_85_BEGIN_STORY_VIDEO,
        "opcode 85 CD preflight rejection yields and publishes previous without consuming the filename"
    );

    Fixture truncated;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.context.instruction_offset = 0x7FF8U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FF8U, OP_85_BEGIN_STORY_VIDEO);
    std::ranges::copy(
        std::string_view{"broken"}, truncated.state.window.begin() + 0x7FFAU
    );
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 1U &&
            truncated.ports.framebuffer_clear_count == 1U &&
            truncated.ports.framebuffer_present_count == 1U &&
            truncated.ports.direct_audio_service_count == 1U &&
            truncated.ports.video_prepare_count == 1U &&
            truncated.ports.video_begin_count == 0U &&
            truncated.context.instruction_offset == 0x7FF8U &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 85 missing terminator typed-stops only after clear, present, audio service and preflight"
    );
}

void test_rewrite_role_head_action_key_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode,
                                 const u16 old_action_id,
                                 const u16 old_variant,
                                 const u16 new_action_id,
                                 const u16 new_variant) {
        write_u16(bytes, offset, raw_opcode);
        write_u16(bytes, offset + 2U, old_action_id);
        write_u16(bytes, offset + 4U, old_variant);
        write_u16(bytes, offset + 6U, new_action_id);
        write_u16(bytes, offset + 8U, new_variant);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        openswd3::world_map::LegacyRoleHeadActionList actions(3U);
        auto variant_miss = actions.begin();
        variant_miss->action.action_id = 10001U;
        variant_miss->action.base_variant = 9U;
        auto first_exact = std::next(variant_miss);
        first_exact->action.action_id = 10001U;
        first_exact->action.base_variant = 1U;
        auto duplicate = std::next(first_exact);
        duplicate->action.action_id = 10001U;
        duplicate->action.base_variant = 1U;
        exact_tail.runtime.role_head_actions = &actions;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FF6U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            0x7FF6U,
            static_cast<u16>(OP_86_REWRITE_ROLE_HEAD_ACTION_KEY | alias_mask),
            10001U,
            1U,
            10002U,
            24U
        );

        const auto result = exact_tail.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY &&
                result.executed_instruction_count == 1U &&
                variant_miss->action.action_id == 10001U &&
                variant_miss->action.base_variant == 9U &&
                first_exact->action.action_id == 10002U &&
                first_exact->action.base_variant == 24U &&
                duplicate->action.action_id == 10001U &&
                duplicate->action.base_variant == 1U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
            "opcode 86 aliases rewrite only the first exact head-action key before exact-tail next fetch"
        );
    }

    Fixture ordinary;
    openswd3::world_map::LegacyRoleHeadActionList ordinary_actions(1U);
    ordinary_actions.front().action.action_id = 10002U;
    ordinary_actions.front().action.base_variant = 18U;
    ordinary.runtime.role_head_actions = &ordinary_actions;
    auto ordinary_script = std::span<u8>{ordinary.ports.initial_window};
    write_record(
        ordinary_script,
        0U,
        OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
        10002U,
        18U,
        10002U,
        24U
    );
    write_u16(ordinary_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(ordinary_script, 12U, 0x00F8U);
    const auto ordinary_result = ordinary.step();
    test.expect_true(
        ordinary_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ordinary_result.executed_instruction_count == 2U &&
            ordinary_actions.front().action.action_id == 10002U &&
            ordinary_actions.front().action.base_variant == 24U &&
            ordinary.context.instruction_offset == 14U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 86 rewrites both key fields and continues in the same call"
    );

    Fixture empty;
    openswd3::world_map::LegacyRoleHeadActionList empty_actions;
    empty.runtime.role_head_actions = &empty_actions;
    empty.context.talk_data_offset = 0x1111U;
    empty.context.instruction_offset = 0x7FFEU;
    empty.state.loaded_file_number = 1U;
    empty.state.loaded_data_offset = 0x1111U;
    empty.state.window_loaded = true;
    empty.state.previous_opcode = 0x66U;
    write_u16(empty.state.window, 0x7FFEU, OP_86_REWRITE_ROLE_HEAD_ACTION_KEY);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            empty.context.instruction_offset == 0x8008U &&
            empty.state.previous_opcode == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
        "opcode 86 empty list reads no operands and still advances ten bytes"
    );

    Fixture id_miss;
    openswd3::world_map::LegacyRoleHeadActionList miss_actions(1U);
    miss_actions.front().action.action_id = 0x00010001U;
    miss_actions.front().action.base_variant = 1U;
    id_miss.runtime.role_head_actions = &miss_actions;
    id_miss.context.talk_data_offset = 0x1111U;
    id_miss.context.instruction_offset = 0x7FFCU;
    id_miss.state.loaded_file_number = 1U;
    id_miss.state.loaded_data_offset = 0x1111U;
    id_miss.state.window_loaded = true;
    id_miss.state.previous_opcode = 0x66U;
    write_u16(
        id_miss.state.window, 0x7FFCU, OP_86_REWRITE_ROLE_HEAD_ACTION_KEY
    );
    write_u16(id_miss.state.window, 0x7FFEU, 1U);
    const auto id_miss_result = id_miss.step();
    test.expect_true(
        id_miss_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            miss_actions.front().action.action_id == 0x00010001U &&
            miss_actions.front().action.base_variant == 1U &&
            id_miss.context.instruction_offset == 0x8006U &&
            id_miss.state.previous_opcode == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
        "opcode 86 compares zero-extended old ID against the full node dword and ID miss reads no later operands"
    );

    Fixture variant_miss;
    openswd3::world_map::LegacyRoleHeadActionList variant_miss_actions(1U);
    variant_miss_actions.front().action.action_id = 10001U;
    variant_miss_actions.front().action.base_variant = 9U;
    variant_miss.runtime.role_head_actions = &variant_miss_actions;
    variant_miss.context.talk_data_offset = 0x1111U;
    variant_miss.context.instruction_offset = 0x7FFAU;
    variant_miss.state.loaded_file_number = 1U;
    variant_miss.state.loaded_data_offset = 0x1111U;
    variant_miss.state.window_loaded = true;
    variant_miss.state.previous_opcode = 0x66U;
    write_u16(
        variant_miss.state.window, 0x7FFAU, OP_86_REWRITE_ROLE_HEAD_ACTION_KEY
    );
    write_u16(variant_miss.state.window, 0x7FFCU, 10001U);
    write_u16(variant_miss.state.window, 0x7FFEU, 1U);
    const auto variant_miss_result = variant_miss.step();
    test.expect_true(
        variant_miss_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            variant_miss_actions.front().action.action_id == 10001U &&
            variant_miss_actions.front().action.base_variant == 9U &&
            variant_miss.context.instruction_offset == 0x8004U &&
            variant_miss.state.previous_opcode ==
                OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
        "opcode 86 variant miss reads no new key operands and still advances ten bytes"
    );

    const auto make_matching = []() {
        openswd3::world_map::LegacyRoleHeadActionList actions(1U);
        actions.front().action.action_id = 10001U;
        actions.front().action.base_variant = 9U;
        return actions;
    };
    for (const std::size_t available_words : {2U, 3U, 4U}) {
        Fixture truncated;
        auto actions = make_matching();
        truncated.runtime.role_head_actions = &actions;
        const u16 offset =
            static_cast<u16>(0x8000U - available_words * sizeof(u16));
        truncated.context.talk_data_offset = 0x1111U;
        truncated.context.instruction_offset = offset;
        truncated.state.loaded_file_number = 1U;
        truncated.state.loaded_data_offset = 0x1111U;
        truncated.state.window_loaded = true;
        truncated.state.previous_opcode = 0x66U;
        write_u16(
            truncated.state.window, offset, OP_86_REWRITE_ROLE_HEAD_ACTION_KEY
        );
        write_u16(truncated.state.window, offset + 2U, 10001U);
        if (available_words >= 3U) {
            write_u16(truncated.state.window, offset + 4U, 9U);
        }
        if (available_words >= 4U) {
            write_u16(truncated.state.window, offset + 6U, 10002U);
        }

        const auto result = truncated.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                actions.front().action.action_id ==
                    (available_words == 4U ? 10002U : 10001U) &&
                actions.front().action.base_variant == 9U &&
                truncated.context.instruction_offset == offset &&
                truncated.state.previous_opcode == 0x66U,
            "opcode 86 staged truncation preserves the new-ID write before the new-variant unsafe read"
        );
    }

    Fixture unavailable;
    auto unavailable_script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(unavailable_script, 0U, OP_86_REWRITE_ROLE_HEAD_ACTION_KEY);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 86 typed owner absence stops at the original list-head access before operands"
    );
}

void test_reload_random_target_protocol(openswd3::test::Context& test) {
    const auto write_three_target_record = [](const std::span<u8> bytes,
                                              const std::size_t offset,
                                              const u16 raw_opcode) {
        write_u16(bytes, offset, raw_opcode);
        write_u32(bytes, offset + 2U, 0x11111111U);
        write_u32(bytes, offset + 6U, 0x22222222U);
        write_u32(bytes, offset + 10U, 0x33333333U);
        write_u32(bytes, offset + 14U, 0xFF00FF00U);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.secondary_rng.seed(0x12345678U);
        write_u16(exact_tail.ports.transferred_window, 0U, 88U);
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FEEU;
        exact_tail.state.loaded_file_number = 2U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_three_target_record(
            exact_tail.state.window,
            0x7FEEU,
            static_cast<u16>(OP_87_RELOAD_RANDOM_TARGET | alias_mask)
        );

        const auto result = exact_tail.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
                result.opcode == 88U &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                exact_tail.secondary_rng.index() == 2U &&
                exact_tail.ports.data_load_count == 1U &&
                exact_tail.ports.last_data_file_number == 2U &&
                exact_tail.ports.last_data_offset == 0x22222222U &&
                !exact_tail.ports.last_data_clear_before_read &&
                exact_tail.ports.direct_audio_service_count == 1U &&
                exact_tail.context.talk_data_offset == 0x22222222U &&
                exact_tail.context.instruction_offset == 0U &&
                exact_tail.state.loaded_data_offset == 0x22222222U &&
                exact_tail.state.window_loaded &&
                exact_tail.state.previous_opcode == OP_87_RELOAD_RANDOM_TARGET,
            "opcode 87 aliases scan an exact-tail table, consume the assembly RNG stream, reload the selected target and continue in the same call"
        );
    }

    Fixture empty;
    empty.secondary_rng.seed(0x12345678U);
    auto empty_script = std::span<u8>{empty.ports.initial_window};
    write_u16(empty_script, 0U, OP_87_RELOAD_RANDOM_TARGET);
    write_u32(empty_script, 2U, 0xFF00FF00U);
    empty.state.previous_opcode = 0x66U;
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::random_target_divide_by_zero &&
            empty.secondary_rng.index() == 0U &&
            empty.ports.data_load_count == 0U &&
            empty.ports.direct_audio_service_count == 0U &&
            empty.context.instruction_offset == 0U &&
            empty.state.previous_opcode == 0x66U,
        "opcode 87 empty table typed-stops at the original unsigned divide by zero before RNG state access"
    );

    Fixture unterminated;
    unterminated.secondary_rng.seed(0x12345678U);
    unterminated.context.talk_data_offset = 0x1111U;
    unterminated.context.instruction_offset = 0x7FFAU;
    unterminated.state.loaded_file_number = 1U;
    unterminated.state.loaded_data_offset = 0x1111U;
    unterminated.state.window_loaded = true;
    unterminated.state.previous_opcode = 0x66U;
    write_u16(unterminated.state.window, 0x7FFAU, OP_87_RELOAD_RANDOM_TARGET);
    write_u32(unterminated.state.window, 0x7FFCU, 0x11111111U);
    const auto unterminated_result = unterminated.step();
    test.expect_true(
        unterminated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            unterminated.secondary_rng.index() == 0U &&
            unterminated.ports.data_load_count == 0U &&
            unterminated.context.instruction_offset == 0x7FFAU &&
            unterminated.state.previous_opcode == 0x66U,
        "opcode 87 missing sentinel typed-stops after the bounded table scan and before RNG access"
    );

    Fixture unavailable;
    unavailable.runtime.secondary_rng = nullptr;
    auto unavailable_script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(unavailable_script, 0U, OP_87_RELOAD_RANDOM_TARGET);
    write_u32(unavailable_script, 2U, 0x11111111U);
    write_u32(unavailable_script, 6U, 0xFF00FF00U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.ports.data_load_count == 0U &&
            unavailable.ports.direct_audio_service_count == 0U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 87 typed RNG owner absence stops only after a valid non-empty table scan"
    );

    Fixture load_failed;
    load_failed.secondary_rng.seed(0x12345678U);
    load_failed.ports.data_load_status = LegacyTalkWindowStatus::open_failed;
    auto failed_script = std::span<u8>{load_failed.ports.initial_window};
    write_u16(failed_script, 0U, OP_87_RELOAD_RANDOM_TARGET);
    write_u32(failed_script, 2U, 0x12345678U);
    write_u32(failed_script, 6U, 0xFF00FF00U);
    load_failed.state.previous_opcode = 0x66U;
    const auto failed_result = load_failed.step();
    test.expect_true(
        failed_result.status == LegacyWorldStoryVmStatus::load_failed &&
            failed_result.direct_audio_service_count == 1U &&
            load_failed.secondary_rng.index() == 2U &&
            load_failed.ports.data_load_count == 1U &&
            load_failed.ports.last_data_offset == 0x12345678U &&
            load_failed.ports.direct_audio_service_count == 1U &&
            load_failed.context.talk_data_offset == 0x12345678U &&
            load_failed.context.instruction_offset == 0U &&
            !load_failed.state.window_loaded &&
            load_failed.state.previous_opcode == OP_87_RELOAD_RANDOM_TARGET,
        "opcode 87 load failure preserves RNG, audio, target publication and previous before typed stop"
    );
}

void test_request_battle_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode,
                                 const u16 battle_id) {
        write_u16(bytes, offset, raw_opcode);
        write_u16(bytes, offset + 2U, battle_id);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows(2U);
        openswd3::world_map::LegacyRoleHeadActionList role_heads(2U);
        openswd3::world_map::LegacyMovingActionList moving_actions(1U);
        u32 battle_request = 0x11111111U;
        exact_tail.runtime.packed_row_effects = &packed_rows;
        exact_tail.runtime.role_head_actions = &role_heads;
        exact_tail.runtime.moving_actions = &moving_actions;
        exact_tail.runtime.battle_request_value = &battle_request;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            0x7FFCU,
            static_cast<u16>(OP_88_REQUEST_BATTLE | alias_mask),
            0x8001U
        );

        const auto result = exact_tail.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_88_REQUEST_BATTLE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                packed_rows.empty() && role_heads.empty() &&
                moving_actions.size() == 1U && battle_request == 0xFFFF8001U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == OP_88_REQUEST_BATTLE,
            "opcode 88 aliases release only packed-row and role-head lists, sign-extend the request and yield at the exact tail"
        );
    }

    constexpr std::array<std::pair<u16, u32>, 4U> signed_requests{
        std::pair<u16, u32>{0x0000U, 0x80000000U},
        std::pair<u16, u32>{0x7FFFU, 0x80007FFFU},
        std::pair<u16, u32>{0x8000U, 0xFFFF8000U},
        std::pair<u16, u32>{0xFFFFU, 0xFFFFFFFFU},
    };
    for (const auto [raw_battle_id, expected_request] : signed_requests) {
        Fixture fixture;
        std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows;
        openswd3::world_map::LegacyRoleHeadActionList role_heads;
        u32 battle_request = 0U;
        fixture.runtime.packed_row_effects = &packed_rows;
        fixture.runtime.role_head_actions = &role_heads;
        fixture.runtime.battle_request_value = &battle_request;
        auto script = std::span<u8>{fixture.ports.initial_window};
        write_record(script, 0U, OP_88_REQUEST_BATTLE, raw_battle_id);

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                battle_request == expected_request &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_88_REQUEST_BATTLE,
            "opcode 88 preserves signed 16-bit battle request extension"
        );
    }

    Fixture packed_unavailable;
    openswd3::world_map::LegacyRoleHeadActionList untouched_heads(1U);
    openswd3::world_map::LegacyMovingActionList untouched_moving(1U);
    u32 untouched_battle = 0x11111111U;
    packed_unavailable.runtime.role_head_actions = &untouched_heads;
    packed_unavailable.runtime.moving_actions = &untouched_moving;
    packed_unavailable.runtime.battle_request_value = &untouched_battle;
    packed_unavailable.state.previous_opcode = 0x66U;
    write_u16(
        packed_unavailable.ports.initial_window, 0U, OP_88_REQUEST_BATTLE
    );
    const auto packed_unavailable_result = packed_unavailable.step();
    test.expect_true(
        packed_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            untouched_heads.size() == 1U && untouched_moving.size() == 1U &&
            untouched_battle == 0x11111111U &&
            packed_unavailable.context.instruction_offset == 0U &&
            packed_unavailable.state.previous_opcode == 0x66U,
        "opcode 88 packed-row owner absence stops at the first release before other owners or operands"
    );

    Fixture head_unavailable;
    std::list<openswd3::rendering::LegacyPackedRowEffect> released_rows(1U);
    openswd3::world_map::LegacyMovingActionList retained_moving(1U);
    u32 retained_battle = 0x11111111U;
    head_unavailable.runtime.packed_row_effects = &released_rows;
    head_unavailable.runtime.moving_actions = &retained_moving;
    head_unavailable.runtime.battle_request_value = &retained_battle;
    head_unavailable.state.previous_opcode = 0x66U;
    write_u16(head_unavailable.ports.initial_window, 0U, OP_88_REQUEST_BATTLE);
    const auto head_unavailable_result = head_unavailable.step();
    test.expect_true(
        head_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            released_rows.empty() && retained_moving.size() == 1U &&
            retained_battle == 0x11111111U &&
            head_unavailable.context.instruction_offset == 0U &&
            head_unavailable.state.previous_opcode == 0x66U,
        "opcode 88 role-head owner absence preserves the already completed packed-row release"
    );

    Fixture operand_truncated;
    std::list<openswd3::rendering::LegacyPackedRowEffect> truncated_rows(1U);
    openswd3::world_map::LegacyRoleHeadActionList truncated_heads(1U);
    openswd3::world_map::LegacyMovingActionList truncated_moving(1U);
    u32 truncated_battle = 0x11111111U;
    operand_truncated.runtime.packed_row_effects = &truncated_rows;
    operand_truncated.runtime.role_head_actions = &truncated_heads;
    operand_truncated.runtime.moving_actions = &truncated_moving;
    operand_truncated.runtime.battle_request_value = &truncated_battle;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x66U;
    write_u16(operand_truncated.state.window, 0x7FFEU, OP_88_REQUEST_BATTLE);
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_rows.empty() && truncated_heads.empty() &&
            truncated_moving.size() == 1U && truncated_battle == 0x11111111U &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U,
        "opcode 88 operand truncation retains both completed list releases before the unsafe read"
    );

    Fixture battle_unavailable;
    std::list<openswd3::rendering::LegacyPackedRowEffect> request_rows(1U);
    openswd3::world_map::LegacyRoleHeadActionList request_heads(1U);
    battle_unavailable.runtime.packed_row_effects = &request_rows;
    battle_unavailable.runtime.role_head_actions = &request_heads;
    auto request_script =
        std::span<u8>{battle_unavailable.ports.initial_window};
    write_record(request_script, 0U, OP_88_REQUEST_BATTLE, 123U);
    battle_unavailable.state.previous_opcode = 0x66U;
    const auto battle_unavailable_result = battle_unavailable.step();
    test.expect_true(
        battle_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            request_rows.empty() && request_heads.empty() &&
            battle_unavailable.context.instruction_offset == 0U &&
            battle_unavailable.state.previous_opcode == 0x66U,
        "opcode 88 battle-request owner absence occurs only after releases and operand read"
    );
}

void test_load_name_record_protocol(openswd3::test::Context& test) {
    constexpr std::array<u8, 4U> default_first{0xC1U, 0xC9U, 0xAFU, 0x53U};
    constexpr std::array<u8, 4U> default_second{0xA9U, 0x67U, 0xA5U, 0x69U};
    const auto make_record = [](const std::span<const u8, 4U> prefix) {
        std::array<u8, 32U> record{};
        for (std::size_t index = 0U; index < record.size(); ++index) {
            record[index] = static_cast<u8>(0x80U + index);
        }
        std::ranges::copy(prefix, record.begin());
        record[4] = static_cast<u8>('-');
        record[5] = static_cast<u8>('X');
        record[6] = static_cast<u8>('%');
        record[7] = static_cast<u8>('Q');
        return record;
    };
    const auto set_default_names = [&](Fixture& fixture) {
        std::ranges::copy(default_first, fixture.first_name.begin());
        fixture.first_name[default_first.size()] = 0U;
        std::ranges::copy(default_second, fixture.second_name.begin());
        fixture.second_name[default_second.size()] = 0U;
    };
    const auto install_record = [](Fixture& fixture,
                                   const u32 record_index,
                                   const std::array<u8, 32U>& record,
                                   const u32 table_offset = 0x40U) {
        fixture.maps_payload.fill(0U);
        write_u32(fixture.maps_payload, 0x20U, table_offset);
        const u32 entry_offset = table_offset + record_index * 4U;
        write_u32(fixture.maps_payload, entry_offset, 0x80U);
        std::ranges::copy(record, fixture.maps_payload.begin() + 0x80U);
    };
    const auto prime_exact_tail =
        [](Fixture& fixture, const u16 raw_opcode, const u16 operand) {
            fixture.context.talk_data_offset = 0x1111U;
            fixture.context.instruction_offset = 0x7FFCU;
            fixture.state.loaded_file_number = 1U;
            fixture.state.loaded_data_offset = 0x1111U;
            fixture.state.window_loaded = true;
            write_u16(fixture.state.window, 0x7FFCU, raw_opcode);
            write_u16(fixture.state.window, 0x7FFEU, operand);
        };

    const auto first_record = make_record(default_first);
    auto replaced_first = first_record;
    replaced_first[6] = 0U;
    constexpr std::array<u8, 5U> first_replacement{'A', 'l', 'i', 'c', 'e'};
    std::array<u8, 32U> expected_replaced_first{};
    std::ranges::copy(first_replacement, expected_replaced_first.begin());
    for (std::size_t index = 0U;
         index < expected_replaced_first.size() - first_replacement.size();
         ++index) {
        expected_replaced_first[first_replacement.size() + index] =
            replaced_first[default_first.size() + index];
    }
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture explicit_record;
        install_record(explicit_record, 3U, first_record);
        std::ranges::copy(
            first_replacement, explicit_record.first_name.begin()
        );
        prime_exact_tail(
            explicit_record,
            static_cast<u16>(OP_91_LOAD_NAME_RECORD | alias_mask),
            3U
        );

        const auto result = explicit_record.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_91_LOAD_NAME_RECORD &&
                result.executed_instruction_count == 1U &&
                explicit_record.state.speaker_name == expected_replaced_first &&
                explicit_record.context.instruction_offset == 0x8000U &&
                explicit_record.state.previous_opcode == OP_91_LOAD_NAME_RECORD,
            "opcode 91 aliases copy 32 bytes, terminate percent-Q, apply fixed-buffer name replacement and complete at the exact tail"
        );

        Fixture dynamic_record;
        install_record(dynamic_record, 3U, first_record);
        set_default_names(dynamic_record);
        dynamic_record.state.script_variables[11U] = 3U;
        prime_exact_tail(
            dynamic_record,
            static_cast<u16>(OP_162_LOAD_DYNAMIC_NAME_RECORD | alias_mask),
            11U
        );

        const auto dynamic_result = dynamic_record.step();
        auto expected_dynamic = first_record;
        expected_dynamic[6] = 0U;
        test.expect_true(
            dynamic_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                dynamic_result.opcode == OP_162_LOAD_DYNAMIC_NAME_RECORD &&
                dynamic_result.executed_instruction_count == 1U &&
                dynamic_record.state.speaker_name == expected_dynamic &&
                dynamic_record.context.instruction_offset == 0x8000U &&
                dynamic_record.state.previous_opcode ==
                    OP_162_LOAD_DYNAMIC_NAME_RECORD,
            "opcode 162 aliases resolve variable 11 and share the exact name-record copy path"
        );
    }

    Fixture current_source;
    install_record(current_source, 0U, first_record);
    set_default_names(current_source);
    current_source.context.source_guid = 0U;
    auto current_script = std::span<u8>{current_source.ports.initial_window};
    write_u16(current_script, 0U, OP_91_LOAD_NAME_RECORD);
    write_u16(current_script, 2U, 0xFFF0U);
    write_u16(current_script, 4U, kStoryVmTypedStop);
    const auto current_result = current_source.step();
    test.expect_true(
        current_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_result.opcode == kStoryVmTypedStop &&
            current_result.executed_instruction_count == 2U &&
            current_source.context.instruction_offset == 4U &&
            current_source.state.previous_opcode == OP_91_LOAD_NAME_RECORD,
        "opcode 91 translates FFF0 to the current source while preserving record index zero and same-call continuation"
    );

    Fixture variable_twelve;
    const auto second_record = make_record(default_second);
    install_record(variable_twelve, 4U, second_record);
    set_default_names(variable_twelve);
    variable_twelve.state.script_variables[12U] = 4U;
    auto variable_twelve_script =
        std::span<u8>{variable_twelve.ports.initial_window};
    write_u16(variable_twelve_script, 0U, OP_162_LOAD_DYNAMIC_NAME_RECORD);
    write_u16(variable_twelve_script, 2U, 12U);
    write_u16(variable_twelve_script, 4U, kStoryVmTypedStop);
    const auto variable_twelve_result = variable_twelve.step();
    test.expect_true(
        variable_twelve_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            variable_twelve_result.executed_instruction_count == 2U &&
            variable_twelve.context.instruction_offset == 4U &&
            variable_twelve.state.previous_opcode ==
                OP_162_LOAD_DYNAMIC_NAME_RECORD &&
            variable_twelve.state.speaker_name[6U] == 0U,
        "opcode 162 accepts variable 12 and continues in the same call"
    );

    for (const u16 invalid_index : std::array<u16, 2U>{10U, 13U}) {
        Fixture invalid_variable;
        invalid_variable.maps_payload.fill(0xCCU);
        invalid_variable.state.speaker_name.fill(0x5AU);
        auto script = std::span<u8>{invalid_variable.ports.initial_window};
        write_u16(script, 0U, OP_162_LOAD_DYNAMIC_NAME_RECORD);
        write_u16(script, 2U, invalid_index);
        write_u16(script, 4U, kStoryVmTypedStop);
        invalid_variable.state.previous_opcode = 0x66U;

        const auto result = invalid_variable.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.executed_instruction_count == 2U &&
                invalid_variable.state.speaker_name.front() == 0x5AU &&
                invalid_variable.context.instruction_offset == 4U &&
                invalid_variable.state.previous_opcode ==
                    OP_162_LOAD_DYNAMIC_NAME_RECORD,
            "opcode 162 invalid variable selectors consume four bytes without touching maps or the name record"
        );
    }

    Fixture zero_dynamic_index;
    zero_dynamic_index.maps_payload.fill(0xCCU);
    zero_dynamic_index.state.speaker_name.fill(0x5AU);
    auto zero_script = std::span<u8>{zero_dynamic_index.ports.initial_window};
    write_u16(zero_script, 0U, OP_162_LOAD_DYNAMIC_NAME_RECORD);
    write_u16(zero_script, 2U, 11U);
    write_u16(zero_script, 4U, kStoryVmTypedStop);
    zero_dynamic_index.state.previous_opcode = 0x66U;
    const auto zero_result = zero_dynamic_index.step();
    test.expect_true(
        zero_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_result.executed_instruction_count == 2U &&
            zero_dynamic_index.state.speaker_name.front() == 0x5AU &&
            zero_dynamic_index.context.instruction_offset == 4U &&
            zero_dynamic_index.state.previous_opcode ==
                OP_162_LOAD_DYNAMIC_NAME_RECORD,
        "opcode 162 zero dynamic record indices consume without maps access"
    );

    Fixture wrapped_dynamic_index;
    install_record(wrapped_dynamic_index, 0xFFFFFFFFU, first_record, 0x44U);
    set_default_names(wrapped_dynamic_index);
    wrapped_dynamic_index.state.script_variables[11U] = 0xFFFFFFFFU;
    auto wrapped_script =
        std::span<u8>{wrapped_dynamic_index.ports.initial_window};
    write_u16(wrapped_script, 0U, OP_162_LOAD_DYNAMIC_NAME_RECORD);
    write_u16(wrapped_script, 2U, 11U);
    write_u16(wrapped_script, 4U, kStoryVmTypedStop);
    const auto wrapped_result = wrapped_dynamic_index.step();
    test.expect_true(
        wrapped_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            wrapped_result.executed_instruction_count == 2U &&
            wrapped_dynamic_index.state.speaker_name[6U] == 0U &&
            wrapped_dynamic_index.state.previous_opcode ==
                OP_162_LOAD_DYNAMIC_NAME_RECORD,
        "opcode 162 preserves 32-bit table-entry multiplication and addition wrapping"
    );

    Fixture missing_terminator;
    auto unterminated_record = first_record;
    unterminated_record[6U] = 0xCCU;
    unterminated_record[7U] = 0xDDU;
    install_record(missing_terminator, 3U, unterminated_record);
    set_default_names(missing_terminator);
    auto unterminated_script =
        std::span<u8>{missing_terminator.ports.initial_window};
    write_u16(unterminated_script, 0U, OP_91_LOAD_NAME_RECORD);
    write_u16(unterminated_script, 2U, 3U);
    missing_terminator.state.previous_opcode = 0x66U;
    const auto unterminated_result = missing_terminator.step();
    test.expect_true(
        unterminated_result.status ==
                LegacyWorldStoryVmStatus::name_terminator_not_found &&
            missing_terminator.state.speaker_name == unterminated_record &&
            missing_terminator.context.instruction_offset == 0U &&
            missing_terminator.state.previous_opcode == 0x66U,
        "opcode 91 missing percent-Q stops only after the 32-byte record copy"
    );

    Fixture maps_failure;
    maps_failure.maps_payload.fill(0U);
    write_u32(maps_failure.maps_payload, 0x20U, 0xFEU);
    maps_failure.state.speaker_name.fill(0x5AU);
    auto maps_failure_script = std::span<u8>{maps_failure.ports.initial_window};
    write_u16(maps_failure_script, 0U, OP_91_LOAD_NAME_RECORD);
    write_u16(maps_failure_script, 2U, 1U);
    maps_failure.state.previous_opcode = 0x66U;
    const auto maps_failure_result = maps_failure.step();
    test.expect_true(
        maps_failure_result.status ==
                LegacyWorldStoryVmStatus::maps_payload_out_of_range &&
            maps_failure.state.speaker_name.front() == 0x5AU &&
            maps_failure.context.instruction_offset == 0U &&
            maps_failure.state.previous_opcode == 0x66U,
        "opcode 91 table-entry failure stops before record copy and publication"
    );

    Fixture operand_truncated;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.speaker_name.fill(0x5AU);
    operand_truncated.state.previous_opcode = 0x66U;
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_162_LOAD_DYNAMIC_NAME_RECORD
    );
    const auto truncated_result = operand_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.state.speaker_name.front() == 0x5AU &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U,
        "opcode 162 stops before the unsafe variable selector read"
    );
}

void test_set_reserved_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(
            exact_tail.state.window,
            0x7FFCU,
            static_cast<u16>(OP_92_SET_RESERVED_GLOBAL_BIT | alias_mask)
        );
        write_u16(exact_tail.state.window, 0x7FFEU, 1U);

        const auto result = exact_tail.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_92_SET_RESERVED_GLOBAL_BIT &&
                result.executed_instruction_count == 1U &&
                (exact_tail.state.flags[3U] & 0x40U) != 0U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_92_SET_RESERVED_GLOBAL_BIT,
            "opcode 92 aliases set reserved bit 30 and complete before an exact-tail next-fetch failure"
        );
    }

    struct SafeCase {
        u16 selector;
        u32 bit_index;
    };
    constexpr std::array<SafeCase, 4U> safe_cases{
        SafeCase{0U, 29U},
        SafeCase{4U, 33U},
        SafeCase{5U, 34U},
        SafeCase{8162U, 8191U},
    };
    for (const auto safe_case : safe_cases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_92_SET_RESERVED_GLOBAL_BIT);
        write_u16(fixture.state.window, 2U, safe_case.selector);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);

        const auto result = fixture.step();
        const std::size_t byte_index = safe_case.bit_index >> 3U;
        const u8 mask = static_cast<u8>(1U << (safe_case.bit_index & 7U));
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                (fixture.state.flags[byte_index] & mask) != 0U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_92_SET_RESERVED_GLOBAL_BIT,
            "opcode 92 preserves selector-minus-one wrapping, invalid-but-accessed values, the final owned bit and same-call continuation"
        );
    }

    for (const u16 unsafe_selector : std::array<u16, 2U>{8163U, 0xFFFFU}) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_92_SET_RESERVED_GLOBAL_BIT);
        write_u16(fixture.state.window, 2U, unsafe_selector);
        fixture.state.flags.fill(0x5AU);
        const auto original_flags = fixture.state.flags;
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::global_bit_index_out_of_range &&
                fixture.state.flags == original_flags &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0x66U,
            "opcode 92 typed-stops only when the original computed byte leaves the owned global-bit array"
        );
    }

    Fixture operand_truncated;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x66U;
    operand_truncated.state.flags.fill(0x5AU);
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_92_SET_RESERVED_GLOBAL_BIT
    );
    const auto original_flags = operand_truncated.state.flags;

    const auto truncated_result = operand_truncated.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.state.flags == original_flags &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U,
        "opcode 92 stops before the unsafe operand read and all bit effects"
    );
}

void test_clear_reserved_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        exact_tail.state.flags.fill(0xFFU);
        write_u16(
            exact_tail.state.window,
            0x7FFCU,
            static_cast<u16>(OP_93_CLEAR_RESERVED_GLOBAL_BIT | alias_mask)
        );
        write_u16(exact_tail.state.window, 0x7FFEU, 1U);

        const auto result = exact_tail.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_93_CLEAR_RESERVED_GLOBAL_BIT &&
                result.executed_instruction_count == 1U &&
                (exact_tail.state.flags[3U] & 0x40U) == 0U &&
                (exact_tail.state.flags[3U] & 0xBFU) == 0xBFU &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_93_CLEAR_RESERVED_GLOBAL_BIT,
            "opcode 93 aliases clear only reserved bit 30 and complete before an exact-tail next-fetch failure"
        );
    }

    struct SafeCase {
        u16 selector;
        u32 bit_index;
    };
    constexpr std::array<SafeCase, 4U> safe_cases{
        SafeCase{0U, 29U},
        SafeCase{4U, 33U},
        SafeCase{5U, 34U},
        SafeCase{8162U, 8191U},
    };
    for (const auto safe_case : safe_cases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_93_CLEAR_RESERVED_GLOBAL_BIT);
        fixture.state.flags.fill(0xFFU);
        write_u16(fixture.state.window, 2U, safe_case.selector);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);

        const auto result = fixture.step();
        const std::size_t byte_index = safe_case.bit_index >> 3U;
        const u8 mask = static_cast<u8>(1U << (safe_case.bit_index & 7U));
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                (fixture.state.flags[byte_index] & mask) == 0U &&
                fixture.state.flags[byte_index] ==
                    static_cast<u8>(0xFFU - mask) &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_93_CLEAR_RESERVED_GLOBAL_BIT,
            "opcode 93 preserves selector-minus-one wrapping, invalid-but-accessed values, the final owned clear and same-call continuation"
        );
    }

    for (const u16 unsafe_selector : std::array<u16, 2U>{8163U, 0xFFFFU}) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_93_CLEAR_RESERVED_GLOBAL_BIT);
        write_u16(fixture.state.window, 2U, unsafe_selector);
        fixture.state.flags.fill(0xA5U);
        const auto original_flags = fixture.state.flags;
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::global_bit_index_out_of_range &&
                fixture.state.flags == original_flags &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0x66U,
            "opcode 93 typed-stops only when the original computed clear leaves the owned global-bit array"
        );
    }

    Fixture operand_truncated;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x66U;
    operand_truncated.state.flags.fill(0xA5U);
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_93_CLEAR_RESERVED_GLOBAL_BIT
    );
    const auto original_flags = operand_truncated.state.flags;

    const auto truncated_result = operand_truncated.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.state.flags == original_flags &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U,
        "opcode 93 stops before the unsafe operand read and all clear effects"
    );
}

void test_set_scene_render_bit1_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        u8 scene_render_flags = 0xA5U;
        exact_tail.runtime.scene_render_flags = &scene_render_flags;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_94_SET_SCENE_RENDER_BIT1 | alias_mask)
        );

        const auto result = exact_tail.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_94_SET_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                scene_render_flags == 0xA7U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == OP_94_SET_SCENE_RENDER_BIT1,
            "opcode 94 aliases preserve all other scene bits, publish previous and yield after an exact-tail record"
        );
    }

    Fixture all_bits;
    prime_loaded_instruction(all_bits, OP_94_SET_SCENE_RENDER_BIT1);
    u8 all_scene_render_flags = 0xFFU;
    all_bits.runtime.scene_render_flags = &all_scene_render_flags;
    const auto all_bits_result = all_bits.step();
    test.expect_true(
        all_bits_result.status == LegacyWorldStoryVmStatus::yielded &&
            all_scene_render_flags == 0xFFU &&
            all_bits.context.instruction_offset == 2U &&
            all_bits.state.previous_opcode == OP_94_SET_SCENE_RENDER_BIT1,
        "opcode 94 ORs only scene bit 1"
    );

    Fixture runtime_unavailable;
    prime_loaded_instruction(runtime_unavailable, OP_94_SET_SCENE_RENDER_BIT1);
    u8 unavailable_scene_render_flags = 0xA5U;
    runtime_unavailable.runtime.scene_render_flags = nullptr;
    runtime_unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = runtime_unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_scene_render_flags == 0xA5U &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.state.previous_opcode == 0x66U,
        "opcode 94 typed-stops at the original fixed scene-flag owner before all side effects"
    );
}

void test_clear_scene_render_bit1_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        u8 scene_render_flags = 0xA7U;
        exact_tail.runtime.scene_render_flags = &scene_render_flags;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_95_CLEAR_SCENE_RENDER_BIT1 | alias_mask)
        );

        const auto result = exact_tail.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                scene_render_flags == 0xA5U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1,
            "opcode 95 aliases preserve all other scene bits, publish previous and yield after an exact-tail record"
        );
    }

    Fixture already_clear;
    prime_loaded_instruction(already_clear, OP_95_CLEAR_SCENE_RENDER_BIT1);
    u8 clear_scene_render_flags = 0xA5U;
    already_clear.runtime.scene_render_flags = &clear_scene_render_flags;
    const auto already_clear_result = already_clear.step();
    test.expect_true(
        already_clear_result.status == LegacyWorldStoryVmStatus::yielded &&
            clear_scene_render_flags == 0xA5U &&
            already_clear.context.instruction_offset == 2U &&
            already_clear.state.previous_opcode ==
                OP_95_CLEAR_SCENE_RENDER_BIT1,
        "opcode 95 clears only scene bit 1"
    );

    Fixture runtime_unavailable;
    prime_loaded_instruction(
        runtime_unavailable, OP_95_CLEAR_SCENE_RENDER_BIT1
    );
    u8 unavailable_scene_render_flags = 0xA7U;
    runtime_unavailable.runtime.scene_render_flags = nullptr;
    runtime_unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = runtime_unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_scene_render_flags == 0xA7U &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.state.previous_opcode == 0x66U,
        "opcode 95 typed-stops at the original fixed scene-flag owner before all side effects"
    );
}

void test_begin_custom_ani_protocol(openswd3::test::Context& test) {
    const auto write_payload = [](Fixture& fixture,
                                  const std::size_t offset,
                                  const std::string_view payload) {
        std::ranges::copy(
            payload,
            fixture.state.window.begin() + static_cast<std::ptrdiff_t>(offset)
        );
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_96_BEGIN_CUSTOM_ANI | alias_mask)
        );
        u8 scene_render_flags = 0xA5U;
        fixture.runtime.scene_render_flags = &scene_render_flags;
        write_payload(fixture, 2U, "%*demo.ani%Q");

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_96_BEGIN_CUSTOM_ANI &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 3U &&
                fixture.context.instruction_offset == 14U &&
                fixture.state.previous_opcode == OP_96_BEGIN_CUSTOM_ANI &&
                fixture.ports.last_ani_frame_interval == 70U &&
                fixture.ports.ani_prepare_count == 1U &&
                fixture.ports.ani_begin_count == 1U &&
                fixture.ports.last_ani_filename ==
                    std::vector<u8>{'d', 'e', 'm', 'o', '.', 'a', 'n', 'i'} &&
                fixture.ports.last_ani_flags ==
                    (openswd3::asset_runtime::kLegacyAniSkipRevealFlag |
                     openswd3::asset_runtime::kLegacyAniEndingEffectFlag) &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{8U, 2U, 9U, 2U, 10U, 2U},
            "opcode 96 aliases set interval, parse independent percent-star flags, service audio, start ANI, publish previous and yield"
        );
    }

    struct PrefixCase {
        std::string_view payload;
        u8 flags;
        u16 length;
    };
    constexpr std::array<PrefixCase, 4U> prefix_cases{
        PrefixCase{"demo.ani%Q", 0U, 12U},
        PrefixCase{
            "%demo.ani%Q",
            openswd3::asset_runtime::kLegacyAniSkipRevealFlag,
            13U,
        },
        PrefixCase{
            "*demo.ani%Q",
            openswd3::asset_runtime::kLegacyAniEndingEffectFlag,
            13U,
        },
        PrefixCase{
            "%*demo.ani%Q",
            openswd3::asset_runtime::kLegacyAniSkipRevealFlag |
                openswd3::asset_runtime::kLegacyAniEndingEffectFlag,
            14U,
        },
    };
    for (const auto prefix_case : prefix_cases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_96_BEGIN_CUSTOM_ANI);
        u8 scene_render_flags = 0x12U;
        fixture.runtime.scene_render_flags = &scene_render_flags;
        write_payload(fixture, 2U, prefix_case.payload);

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                fixture.ports.last_ani_flags == prefix_case.flags &&
                fixture.ports.last_ani_filename ==
                    std::vector<u8>{'d', 'e', 'm', 'o', '.', 'a', 'n', 'i'} &&
                fixture.context.instruction_offset == prefix_case.length &&
                fixture.state.previous_opcode == OP_96_BEGIN_CUSTOM_ANI,
            "opcode 96 parses percent and star as ordered independent optional prefixes"
        );
    }

    Fixture exact_tail;
    constexpr std::string_view exact_payload{"%*demo.ani%Q"};
    constexpr std::size_t exact_record_size = 2U + exact_payload.size();
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = static_cast<u16>(
        openswd3::resource_io::kLegacyTalkWindowSize - exact_record_size
    );
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    u8 exact_scene_flags = 0xA5U;
    exact_tail.runtime.scene_render_flags = &exact_scene_flags;
    write_u16(
        exact_tail.state.window,
        exact_tail.context.instruction_offset,
        OP_96_BEGIN_CUSTOM_ANI
    );
    write_payload(
        exact_tail,
        static_cast<std::size_t>(exact_tail.context.instruction_offset) + 2U,
        exact_payload
    );
    const auto exact_result = exact_tail.step();
    test.expect_true(
        exact_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_96_BEGIN_CUSTOM_ANI &&
            exact_tail.ports.ani_begin_count == 1U,
        "opcode 96 accepts a complete percent-Q record ending exactly at the window tail"
    );

    Fixture open_failure;
    prime_loaded_instruction(open_failure, OP_96_BEGIN_CUSTOM_ANI);
    u8 failed_scene_flags = 0xA5U;
    open_failure.runtime.scene_render_flags = &failed_scene_flags;
    write_payload(open_failure, 2U, "missing.ani%Q");
    open_failure.ports.ani_start_result = {
        .status = openswd3::asset_runtime::LegacyAniActivityStartStatus::
            archive_open_failed,
        .open_status =
            openswd3::asset_runtime::LegacyAniOpenStatus::file_open_failed,
        .frame_status =
            openswd3::asset_runtime::LegacyAniFrameLoadStatus::archive_not_open,
    };
    const auto open_failure_result = open_failure.step();
    test.expect_true(
        open_failure_result.status == LegacyWorldStoryVmStatus::yielded &&
            open_failure.context.instruction_offset == 15U &&
            open_failure.state.previous_opcode == OP_96_BEGIN_CUSTOM_ANI &&
            open_failure.ports.ani_begin_count == 1U,
        "opcode 96 archive-open failure remains a consumed common-join yield"
    );

    Fixture preflight_exit;
    prime_loaded_instruction(preflight_exit, OP_96_BEGIN_CUSTOM_ANI);
    u8 preflight_scene_flags = 0xA5U;
    preflight_exit.runtime.scene_render_flags = &preflight_scene_flags;
    preflight_exit.state.previous_opcode = 0x66U;
    preflight_exit.ports.ani_prepare_success = false;
    write_payload(preflight_exit, 2U, "demo.ani%Q");
    const auto preflight_result = preflight_exit.step();
    test.expect_true(
        preflight_result.status == LegacyWorldStoryVmStatus::yielded &&
            preflight_result.direct_audio_service_count == 1U &&
            preflight_exit.context.instruction_offset == 12U &&
            preflight_exit.state.previous_opcode == 0x66U &&
            preflight_exit.ports.ani_begin_count == 0U &&
            preflight_exit.ports.story_protocol_events ==
                std::vector<u32>{8U, 2U, 9U},
        "opcode 96 CD-style preflight exit preserves consumption and audio but bypasses common previous publication"
    );

    Fixture scene_owner_not_read;
    prime_loaded_instruction(scene_owner_not_read, OP_96_BEGIN_CUSTOM_ANI);
    scene_owner_not_read.runtime.scene_render_flags = nullptr;
    scene_owner_not_read.state.previous_opcode = 0x66U;
    write_payload(scene_owner_not_read, 2U, "demo.ani%Q");
    const auto owner_result = scene_owner_not_read.step();
    test.expect_true(
        owner_result.status == LegacyWorldStoryVmStatus::yielded &&
            owner_result.direct_audio_service_count == 3U &&
            scene_owner_not_read.context.instruction_offset == 12U &&
            scene_owner_not_read.state.previous_opcode ==
                OP_96_BEGIN_CUSTOM_ANI &&
            scene_owner_not_read.ports.ani_begin_count == 1U,
        "opcode 96 leaves live scene flags to the asynchronous ANI port rather than adding a staged VM owner read"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, OP_96_BEGIN_CUSTOM_ANI);
    missing_terminator.state.previous_opcode = 0x66U;
    std::ranges::fill_n(
        missing_terminator.state.window.begin() + 2, 32U, u8{'A'}
    );
    const auto missing_result = missing_terminator.step();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::ani_filename_terminator_not_found &&
            missing_terminator.ports.last_ani_frame_interval == 70U &&
            missing_terminator.ports.direct_audio_service_count == 0U &&
            missing_terminator.context.instruction_offset == 0U &&
            missing_terminator.state.previous_opcode == 0x66U,
        "opcode 96 bounds the original 32-byte temporary filename copy and retains the prior interval side effect"
    );

    Fixture operand_truncated;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x66U;
    write_u16(operand_truncated.state.window, 0x7FFEU, OP_96_BEGIN_CUSTOM_ANI);
    const auto truncated_result = operand_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.ports.last_ani_frame_interval == 70U &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U,
        "opcode 96 sets the interval before its first post-opcode unsafe byte read"
    );
}

void test_wait_custom_ani_complete_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFEU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        fixture.ports.ani_active = true;
        write_u16(
            fixture.state.window,
            fixture.context.instruction_offset,
            static_cast<u16>(OP_97_WAIT_CUSTOM_ANI_COMPLETE | alias_mask)
        );

        const auto active_result = fixture.step();
        test.expect_true(
            active_result.status == LegacyWorldStoryVmStatus::yielded &&
                active_result.opcode == OP_97_WAIT_CUSTOM_ANI_COMPLETE &&
                active_result.executed_instruction_count == 1U &&
                active_result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0x7FFEU &&
                fixture.state.previous_opcode ==
                    OP_97_WAIT_CUSTOM_ANI_COMPLETE &&
                fixture.ports.ani_active_query_count == 1U &&
                fixture.ports.ani_frame_interval_write_count == 0U,
            "opcode 97 aliases publish previous and yield without advancing while the ANI activity is active"
        );

        fixture.ports.ani_active = false;
        const auto inactive_result = fixture.step();
        test.expect_true(
            inactive_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                inactive_result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode ==
                    OP_97_WAIT_CUSTOM_ANI_COMPLETE &&
                fixture.ports.ani_active_query_count == 2U &&
                fixture.ports.ani_frame_interval_write_count == 1U &&
                fixture.ports.last_ani_frame_interval == 35U,
            "opcode 97 aliases consume at completion, restore interval 35, publish previous and accept an exact window tail"
        );
    }

    Fixture same_call;
    prime_loaded_instruction(same_call, OP_97_WAIT_CUSTOM_ANI_COMPLETE);
    write_u16(same_call.state.window, 2U, OP_95_CLEAR_SCENE_RENDER_BIT1);
    u8 scene_render_flags = 0xA7U;
    same_call.runtime.scene_render_flags = &scene_render_flags;
    same_call.ports.ani_active = false;

    const auto same_call_result = same_call.step();
    test.expect_true(
        same_call_result.status == LegacyWorldStoryVmStatus::yielded &&
            same_call_result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
            same_call_result.executed_instruction_count == 2U &&
            same_call.context.instruction_offset == 4U &&
            same_call.state.previous_opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
            same_call.ports.ani_active_query_count == 1U &&
            same_call.ports.last_ani_frame_interval == 35U &&
            scene_render_flags == 0xA5U,
        "opcode 97 completion restores interval before common-join same-call continuation"
    );
}

void test_consume_four_byte_noop_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        const u16 raw_word =
            static_cast<u16>(OP_98_CONSUME_FOUR_BYTE_NOOP | alias_mask);

        Fixture normal;
        prime_loaded_instruction(normal, raw_word);
        write_u16(normal.state.window, 2U, 0xA55AU);
        write_u16(normal.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        normal.runtime.scene_render_flags = &scene_render_flags;

        const auto normal_result = normal.step();
        test.expect_true(
            normal_result.status == LegacyWorldStoryVmStatus::yielded &&
                normal_result.raw_word == raw_word &&
                normal_result.opcode == OP_98_CONSUME_FOUR_BYTE_NOOP &&
                normal_result.executed_instruction_count == 1U &&
                normal_result.direct_audio_service_count == 1U &&
                normal.context.instruction_offset == 4U &&
                normal.state.previous_opcode == OP_98_CONSUME_FOUR_BYTE_NOOP &&
                scene_render_flags == 0xA7U,
            "opcode 98 aliases consume four bytes, publish previous and yield without executing the next instruction"
        );

        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        write_u16(exact_tail.state.window, 0x7FFCU, raw_word);
        write_u16(exact_tail.state.window, 0x7FFEU, 0x5AA5U);

        const auto exact_tail_result = exact_tail.step();
        test.expect_true(
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
                exact_tail_result.executed_instruction_count == 1U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_98_CONSUME_FOUR_BYTE_NOOP,
            "opcode 98 aliases complete all side effects at an exact four-byte window tail"
        );

        Fixture unread_payload;
        unread_payload.context.talk_data_offset = 0x1111U;
        unread_payload.context.instruction_offset = 0x7FFEU;
        unread_payload.state.loaded_file_number = 1U;
        unread_payload.state.loaded_data_offset = 0x1111U;
        unread_payload.state.window_loaded = true;
        write_u16(unread_payload.state.window, 0x7FFEU, raw_word);

        const auto unread_payload_result = unread_payload.step();
        test.expect_true(
            unread_payload_result.status == LegacyWorldStoryVmStatus::yielded &&
                !unread_payload_result.first_operand_available &&
                unread_payload_result.executed_instruction_count == 1U &&
                unread_payload.context.instruction_offset == 0x8002U &&
                unread_payload.state.previous_opcode ==
                    OP_98_CONSUME_FOUR_BYTE_NOOP,
            "opcode 98 does not read or require its nominal payload word before advancing and yielding"
        );
    }
}

void test_wait_custom_ani_phase_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        const u16 raw_word =
            static_cast<u16>(OP_99_WAIT_CUSTOM_ANI_PHASE | alias_mask);

        Fixture waiting;
        prime_loaded_instruction(waiting, raw_word);
        write_u16(waiting.state.window, 2U, 35U);
        waiting.state.previous_opcode = 0x66U;
        waiting.ports.ani_phase = 35;

        const auto waiting_result = waiting.step();
        test.expect_true(
            waiting_result.status == LegacyWorldStoryVmStatus::yielded &&
                waiting_result.raw_word == raw_word &&
                waiting_result.opcode == OP_99_WAIT_CUSTOM_ANI_PHASE &&
                waiting_result.executed_instruction_count == 1U &&
                waiting_result.direct_audio_service_count == 1U &&
                waiting.context.instruction_offset == 0U &&
                waiting.state.previous_opcode == OP_99_WAIT_CUSTOM_ANI_PHASE &&
                waiting.ports.ani_phase_query_count == 1U,
            "opcode 99 aliases wait at an equal phase threshold and publish previous without advancing"
        );

        Fixture completed;
        prime_loaded_instruction(completed, raw_word);
        write_u16(completed.state.window, 2U, 35U);
        write_u16(completed.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        completed.runtime.scene_render_flags = &scene_render_flags;
        completed.ports.ani_phase = 36;

        const auto completed_result = completed.step();
        test.expect_true(
            completed_result.status == LegacyWorldStoryVmStatus::yielded &&
                completed_result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                completed_result.executed_instruction_count == 2U &&
                completed.context.instruction_offset == 6U &&
                completed.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                completed.ports.ani_phase_query_count == 1U &&
                scene_render_flags == 0xA5U,
            "opcode 99 aliases advance only when phase is strictly greater and continue in the same call"
        );

        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        write_u16(exact_tail.state.window, 0x7FFCU, raw_word);
        write_u16(exact_tail.state.window, 0x7FFEU, 350U);
        exact_tail.ports.ani_phase = 351;

        const auto exact_tail_result = exact_tail.step();
        test.expect_true(
            exact_tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_tail_result.executed_instruction_count == 1U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_99_WAIT_CUSTOM_ANI_PHASE &&
                exact_tail.ports.ani_phase_query_count == 1U,
            "opcode 99 aliases commit completion side effects before the next exact-tail fetch fails"
        );
    }

    Fixture signed_startup;
    prime_loaded_instruction(signed_startup, OP_99_WAIT_CUSTOM_ANI_PHASE);
    write_u16(signed_startup.state.window, 2U, 0U);
    signed_startup.ports.ani_phase = -13;
    const auto signed_startup_result = signed_startup.step();
    test.expect_true(
        signed_startup_result.status == LegacyWorldStoryVmStatus::yielded &&
            signed_startup.context.instruction_offset == 0U &&
            signed_startup.state.previous_opcode == OP_99_WAIT_CUSTOM_ANI_PHASE,
        "opcode 99 compares the signed negative ANI startup phase below every u16 threshold"
    );

    Fixture maximum_threshold;
    prime_loaded_instruction(maximum_threshold, OP_99_WAIT_CUSTOM_ANI_PHASE);
    write_u16(maximum_threshold.state.window, 2U, 0xFFFFU);
    write_u16(
        maximum_threshold.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1
    );
    u8 maximum_scene_flags = 0xA7U;
    maximum_threshold.runtime.scene_render_flags = &maximum_scene_flags;
    maximum_threshold.ports.ani_phase = 0xFFFF;
    const auto equal_maximum_result = maximum_threshold.step();
    maximum_threshold.ports.ani_phase = 0x10000;
    const auto above_maximum_result = maximum_threshold.step();
    test.expect_true(
        equal_maximum_result.status == LegacyWorldStoryVmStatus::yielded &&
            equal_maximum_result.executed_instruction_count == 1U &&
            above_maximum_result.status == LegacyWorldStoryVmStatus::yielded &&
            above_maximum_result.executed_instruction_count == 2U &&
            maximum_threshold.context.instruction_offset == 6U &&
            maximum_threshold.ports.ani_phase_query_count == 2U &&
            maximum_scene_flags == 0xA5U,
        "opcode 99 zero-extends the full u16 threshold before the strict signed comparison"
    );

    Fixture missing_threshold;
    missing_threshold.context.talk_data_offset = 0x1111U;
    missing_threshold.context.instruction_offset = 0x7FFEU;
    missing_threshold.state.loaded_file_number = 1U;
    missing_threshold.state.loaded_data_offset = 0x1111U;
    missing_threshold.state.window_loaded = true;
    missing_threshold.state.previous_opcode = 0x66U;
    missing_threshold.ports.ani_phase = 123;
    write_u16(
        missing_threshold.state.window, 0x7FFEU, OP_99_WAIT_CUSTOM_ANI_PHASE
    );

    const auto missing_threshold_result = missing_threshold.step();
    test.expect_true(
        missing_threshold_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_threshold.context.instruction_offset == 0x7FFEU &&
            missing_threshold.state.previous_opcode == 0x66U &&
            missing_threshold.ports.ani_phase_query_count == 1U,
        "opcode 99 reads the phase owner before stopping at the original missing-threshold access point"
    );
}

void test_set_role_talk_script_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        const u16 raw_word =
            static_cast<u16>(OP_100_SET_ROLE_TALK_SCRIPT | alias_mask);
        fixture.roles[1].talk_script_id = 0x1111U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8001U);
        write_u16(fixture.state.window, 6U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        fixture.runtime.scene_render_flags = &scene_render_flags;

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.roles[1].talk_script_id == 0x8001U &&
                fixture.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                fixture.ports.role_patch_requests.empty() &&
                scene_render_flags == 0xA5U,
            "opcode 100 aliases update the live role Talk script and continue in the same call"
        );
    }

    Fixture current_source;
    current_source.roles[1].talk_script_id = 0x1111U;
    prime_loaded_instruction(current_source, OP_100_SET_ROLE_TALK_SCRIPT);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 0xFFFFU);
    write_u16(current_source.state.window, 6U, kStoryVmTypedStop);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_source_result.executed_instruction_count == 2U &&
            current_source.roles[1].talk_script_id == 0xFFFFU &&
            current_source.roles[0].talk_script_id == 0U &&
            current_source.state.previous_opcode ==
                OP_100_SET_ROLE_TALK_SCRIPT &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 100 translates FFF0 to the current source and does not validate the Talk script number"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].talk_script_id = 0x1111U;
    controlled.roles[2].talk_script_id = 0x2222U;
    prime_loaded_instruction(controlled, OP_100_SET_ROLE_TALK_SCRIPT);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 0x3456U);
    write_u16(controlled.state.window, 6U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[1].talk_script_id == 0x1111U &&
            controlled.roles[2].talk_script_id == 0x3456U &&
            controlled.state.previous_opcode == OP_100_SET_ROLE_TALK_SCRIPT,
        "opcode 100 preserves helper-native FFFE controlled-role selection"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_100_SET_ROLE_TALK_SCRIPT);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x4567U);
    write_u16(missing.state.window, 6U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.action_id == 0xFFFFU &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.tile_x == 0xFFFFU && patch.tile_y == 0xFFFFU &&
            patch.talk_script_id == 0x4567U && patch.path_data_id == 0xFFFFU &&
            patch.flags_or_mask == 0U && patch.flags_and_mask == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == OP_100_SET_ROLE_TALK_SCRIPT,
        "opcode 100 missing role submits the exact Talk-only MAPS patch"
    );

    Fixture truncated;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.roles[1].talk_script_id = 0x1111U;
    write_u16(truncated.state.window, 0x7FFCU, OP_100_SET_ROLE_TALK_SCRIPT);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);

    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.roles[1].talk_script_id == 0x1111U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 100 stages both operands before role lookup or MAPS mutation"
    );

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_100_SET_ROLE_TALK_SCRIPT);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x2468U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.roles[1].talk_script_id == 0x2468U &&
            exact_tail.state.previous_opcode == OP_100_SET_ROLE_TALK_SCRIPT,
        "opcode 100 commits the Talk update and previous before the next exact-tail fetch fails"
    );
}

void test_set_role_status_bit26_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        const u16 raw_word =
            static_cast<u16>(OP_101_SET_ROLE_STATUS_BIT26 | alias_mask);
        fixture.roles[1].flags = 0xA0A50020U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        fixture.runtime.scene_render_flags = &scene_render_flags;

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.roles[1].flags == 0xA4A50020U &&
                fixture.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                fixture.ports.role_patch_requests.empty() &&
                scene_render_flags == 0xA5U,
            "opcode 101 aliases OR role status bit 26 and continue in the same call"
        );
    }

    Fixture current_source;
    current_source.roles[1].flags = 0x80000001U;
    prime_loaded_instruction(current_source, OP_101_SET_ROLE_STATUS_BIT26);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, kStoryVmTypedStop);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_source_result.executed_instruction_count == 2U &&
            current_source.roles[1].flags == 0x84000001U &&
            current_source.roles[0].flags == 0U &&
            current_source.state.previous_opcode ==
                OP_101_SET_ROLE_STATUS_BIT26 &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 101 translates FFF0 only for current-source lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].flags = 0x11110000U;
    controlled.roles[2].flags = 0x22220000U;
    prime_loaded_instruction(controlled, OP_101_SET_ROLE_STATUS_BIT26);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[1].flags == 0x11110000U &&
            controlled.roles[2].flags == 0x26220000U &&
            controlled.state.previous_opcode == OP_101_SET_ROLE_STATUS_BIT26,
        "opcode 101 preserves helper-native FFFE controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit | 0x20U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].flags = 0x40U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].flags = 0x80U;
    prime_loaded_instruction(first_clear_match, OP_101_SET_ROLE_STATUS_BIT26);
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, kStoryVmTypedStop);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_clear_match.roles[0].flags ==
                (openswd3::world_map::kLegacyWorldGuidLookupSkipBit | 0x20U) &&
            first_clear_match.roles[1].flags == 0x04000040U &&
            first_clear_match.roles[2].flags == 0x80U,
        "opcode 101 skips bit-28 roles and uses the first clear GUID match"
    );

    Fixture missing;
    missing.roles[0].flags = 0x10U;
    missing.roles[1].flags = 0x20U;
    missing.roles[2].flags = 0x40U;
    prime_loaded_instruction(missing, OP_101_SET_ROLE_STATUS_BIT26);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 4U &&
            missing.roles[0].flags == 0x10U &&
            missing.roles[1].flags == 0x20U &&
            missing.roles[2].flags == 0x40U &&
            missing.ports.role_patch_requests.empty() &&
            missing.state.previous_opcode == OP_101_SET_ROLE_STATUS_BIT26,
        "opcode 101 silently consumes a missing role without MAPS fallback"
    );

    Fixture truncated;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.roles[1].flags = 0xA0A50020U;
    write_u16(truncated.state.window, 0x7FFEU, OP_101_SET_ROLE_STATUS_BIT26);

    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.roles[1].flags == 0xA0A50020U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 101 stops before lookup when the selector is missing"
    );

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].flags = 0xA0A50020U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_101_SET_ROLE_STATUS_BIT26);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.roles[1].flags == 0xA4A50020U &&
            exact_tail.state.previous_opcode == OP_101_SET_ROLE_STATUS_BIT26,
        "opcode 101 commits role bit 26 and previous before the next exact-tail fetch fails"
    );
}

void test_set_role_status_from_boolean_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        u32 mask;
    };
    constexpr std::array<Variant, 8U> variants{
        Variant{OP_102_SET_ROLE_STATUS_BIT6, 0x00000040U},
        Variant{OP_103_SET_ROLE_STATUS_BIT5, 0x00000020U},
        Variant{OP_117_SET_ROLE_STATUS_BIT4, 0x00000010U},
        Variant{OP_136_SET_ROLE_STATUS_BIT12, 0x00001000U},
        Variant{OP_140_SET_ROLE_STATUS_BIT11, 0x00000800U},
        Variant{OP_145_SET_ROLE_STATUS_BIT13, 0x00002000U},
        Variant{OP_146_SET_ROLE_STATUS_BIT8, 0x00000100U},
        Variant{OP_174_SET_ROLE_STATUS_BIT14, 0x00004000U},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto bind_surface = [](Fixture& fixture,
                                 const u32 role_index,
                                 const std::span<u8> surface,
                                 const u32 selected_guid = 0U) {
        auto& role = fixture.roles[role_index];
        role.map_cell_pointer_32 = 0U;
        role.action.field_2c = 1U;
        role.action.field_30 = 1U;
        fixture.runtime.role_surface = {
            .map_width = 2U,
            .selected_guid = selected_guid,
            .surface_grid = surface,
        };
    };
    const auto expected_surface = [](const u32 flags) {
        u32 mark_mask = (flags & 0x00004000U) != 0U ? 0x30000000U : 0x10000000U;
        if ((flags & 0x00000010U) != 0U) {
            mark_mask |= 0x00800000U;
        }
        return 0xCF7FFFFFU | mark_mask;
    };

    for (const auto variant : variants) {
        for (const u16 alias_mask : alias_masks) {
            for (const bool enabled : {false, true}) {
                Fixture fixture;
                const u16 raw_word =
                    static_cast<u16>(variant.opcode | alias_mask);
                const u32 initial_flags =
                    0x80000001U | (enabled ? 0U : variant.mask);
                const u32 final_flags = (initial_flags & ~variant.mask) |
                    (enabled ? variant.mask : 0U);
                fixture.roles[1].flags = initial_flags;
                std::array<u8, 16U> surface{};
                surface.fill(0xFFU);
                bind_surface(fixture, 1U, surface);
                prime_loaded_instruction(fixture, raw_word);
                write_u16(fixture.state.window, 2U, 0x00F8U);
                write_u16(fixture.state.window, 4U, enabled ? 0x8000U : 0U);
                write_u16(
                    fixture.state.window, 6U, OP_95_CLEAR_SCENE_RENDER_BIT1
                );
                u8 scene_render_flags = 0xA7U;
                fixture.runtime.scene_render_flags = &scene_render_flags;

                const auto result = fixture.step();
                test.expect_true(
                    result.status == LegacyWorldStoryVmStatus::yielded &&
                        result.raw_word == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                        result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                        result.executed_instruction_count == 2U &&
                        fixture.context.instruction_offset == 8U &&
                        fixture.roles[1].flags == final_flags &&
                        read_u32(surface, 0U) ==
                            expected_surface(final_flags) &&
                        fixture.state.previous_opcode ==
                            OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                        fixture.ports.role_patch_requests.empty() &&
                        scene_render_flags == 0xA5U,
                    "shared role-status aliases apply zero/any-nonzero booleans, refresh surface occupancy and continue"
                );
            }
        }
    }

    Fixture current_index_key;
    current_index_key.roles[1].guid = 2U;
    current_index_key.roles[1].flags = 0x80000041U;
    current_index_key.roles[2].guid = 0xAAAAU;
    current_index_key.roles[2].flags = 0x22220000U;
    std::array<u8, 16U> current_index_surface{};
    current_index_surface.fill(0xFFU);
    bind_surface(current_index_key, 1U, current_index_surface);
    prime_loaded_instruction(current_index_key, OP_102_SET_ROLE_STATUS_BIT6);
    write_u16(current_index_key.state.window, 2U, 0xFFF0U);
    write_u16(current_index_key.state.window, 4U, 0U);
    write_u16(current_index_key.state.window, 6U, kStoryVmTypedStop);
    const auto current_index_key_result = current_index_key.step(0, 0, 2U);
    test.expect_true(
        current_index_key_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_index_key.roles[1].flags == 0x80000001U &&
            current_index_key.roles[2].flags == 0x22220000U &&
            read_u32(current_index_surface, 0U) == 0xDF7FFFFFU &&
            read_u16(current_index_key.state.window, 2U) == 0xFFF0U,
        "shared role-status FFF0 uses the controlled index as a GUID lookup key, not current source or direct role"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].flags = 0x11110020U;
    controlled.roles[2].flags = 0x22220000U;
    std::array<u8, 16U> controlled_surface{};
    controlled_surface.fill(0xFFU);
    bind_surface(controlled, 2U, controlled_surface);
    prime_loaded_instruction(controlled, OP_103_SET_ROLE_STATUS_BIT5);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 2U);
    write_u16(controlled.state.window, 6U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[1].flags == 0x11110020U &&
            controlled.roles[2].flags == 0x22220020U &&
            read_u32(controlled_surface, 0U) == 0xDF7FFFFFU,
        "shared role-status preserves helper-native FFFE controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit | 0x1000U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].flags = 0x80000001U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].flags = 0x80U;
    std::array<u8, 16U> first_clear_surface{};
    first_clear_surface.fill(0xFFU);
    bind_surface(first_clear_match, 1U, first_clear_surface);
    prime_loaded_instruction(first_clear_match, OP_136_SET_ROLE_STATUS_BIT12);
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, 1U);
    write_u16(first_clear_match.state.window, 6U, kStoryVmTypedStop);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_clear_match.roles[0].flags ==
                (openswd3::world_map::kLegacyWorldGuidLookupSkipBit |
                 0x1000U) &&
            first_clear_match.roles[1].flags == 0x80001001U &&
            first_clear_match.roles[2].flags == 0x80U,
        "shared role-status skips bit-28 roles and uses the first clear GUID match"
    );

    for (const auto variant : variants) {
        for (const bool enabled : {false, true}) {
            Fixture missing;
            prime_loaded_instruction(missing, variant.opcode);
            write_u16(missing.state.window, 2U, 0x7777U);
            write_u16(missing.state.window, 4U, enabled ? 0xFFFFU : 0U);
            write_u16(missing.state.window, 6U, kStoryVmTypedStop);

            const auto result = missing.step();
            const auto patch = missing.ports.role_patch_requests.empty()
                ? openswd3::world_map::LegacyMapsRolePatchRequest{}
                : missing.ports.role_patch_requests.front();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.executed_instruction_count == 2U &&
                    missing.ports.role_patch_requests.size() == 1U &&
                    patch.guid == 0x7777U && patch.action_id == 0xFFFFU &&
                    patch.base_variant == 0xFFFFU &&
                    patch.variant_delta == 0xFFFFU && patch.tile_x == 0xFFFFU &&
                    patch.tile_y == 0xFFFFU &&
                    patch.talk_script_id == 0xFFFFU &&
                    patch.path_data_id == 0xFFFFU &&
                    patch.flags_or_mask ==
                        (enabled ? static_cast<u16>(variant.mask) : 0U) &&
                    patch.flags_and_mask ==
                        (enabled ? 0xFFFFU
                                 : static_cast<u16>(0xFFFFU - variant.mask)) &&
                    patch.logical_map_id == 0xFFFFU &&
                    missing.context.instruction_offset == 6U &&
                    missing.state.previous_opcode == variant.opcode,
                "shared role-status missing path submits exact boolean MAPS masks"
            );
        }
    }

    Fixture current_index_missing;
    prime_loaded_instruction(
        current_index_missing, OP_140_SET_ROLE_STATUS_BIT11
    );
    write_u16(current_index_missing.state.window, 2U, 0xFFF0U);
    write_u16(current_index_missing.state.window, 4U, 1U);
    write_u16(current_index_missing.state.window, 6U, kStoryVmTypedStop);
    const auto current_index_missing_result =
        current_index_missing.step(0, 0, 2U);
    const auto current_index_patch =
        current_index_missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : current_index_missing.ports.role_patch_requests.front();
    test.expect_true(
        current_index_missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_index_patch.guid == 2U &&
            current_index_patch.flags_or_mask == 0x0800U &&
            current_index_patch.flags_and_mask == 0xFFFFU,
        "shared role-status missing FFF0 patches the controlled index key rather than literal FFF0"
    );

    for (const u16 selector : {0x00F8U, 0x7777U}) {
        Fixture missing_value;
        missing_value.context.talk_data_offset = 0x1111U;
        missing_value.context.instruction_offset = 0x7FFCU;
        missing_value.state.loaded_file_number = 1U;
        missing_value.state.loaded_data_offset = 0x1111U;
        missing_value.state.window_loaded = true;
        missing_value.state.previous_opcode = 0x55U;
        missing_value.roles[1].flags = 0x80000040U;
        write_u16(
            missing_value.state.window, 0x7FFCU, OP_102_SET_ROLE_STATUS_BIT6
        );
        write_u16(missing_value.state.window, 0x7FFEU, selector);

        const auto result = missing_value.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                missing_value.context.instruction_offset == 0x7FFCU &&
                missing_value.state.previous_opcode == 0x55U &&
                missing_value.roles[1].flags == 0x80000040U &&
                missing_value.ports.role_patch_requests.empty(),
            "shared role-status reads the boolean only after lookup and stops before live/MAPS mutation"
        );
    }

    Fixture no_surface;
    no_surface.state.previous_opcode = 0x55U;
    no_surface.roles[1].flags = 0x80000040U;
    prime_loaded_instruction(no_surface, OP_102_SET_ROLE_STATUS_BIT6);
    write_u16(no_surface.state.window, 2U, 0x00F8U);
    write_u16(no_surface.state.window, 4U, 0U);
    const auto no_surface_result = no_surface.step();
    test.expect_true(
        no_surface_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            no_surface.context.instruction_offset == 0U &&
            no_surface.roles[1].flags == 0x80000000U &&
            no_surface.state.previous_opcode == 0x55U,
        "shared role-status commits the live flag before the surface unsafe point"
    );

    Fixture partial_surface_failure;
    partial_surface_failure.state.previous_opcode = 0x55U;
    partial_surface_failure.roles[1].flags = 0x80000040U;
    partial_surface_failure.roles[1].map_cell_pointer_32 = 3U;
    partial_surface_failure.roles[1].action.field_2c = 2U;
    partial_surface_failure.roles[1].action.field_30 = 1U;
    std::array<u8, 16U> partial_surface{};
    partial_surface.fill(0xFFU);
    partial_surface_failure.runtime.role_surface = {
        .map_width = 2U,
        .selected_guid = 0U,
        .surface_grid = partial_surface,
    };
    prime_loaded_instruction(
        partial_surface_failure, OP_102_SET_ROLE_STATUS_BIT6
    );
    write_u16(partial_surface_failure.state.window, 2U, 0x00F8U);
    write_u16(partial_surface_failure.state.window, 4U, 0U);
    const auto partial_surface_result = partial_surface_failure.step();
    test.expect_true(
        partial_surface_result.status ==
                LegacyWorldStoryVmStatus::role_surface_failed &&
            partial_surface_failure.context.instruction_offset == 0U &&
            partial_surface_failure.roles[1].flags == 0x80000000U &&
            read_u32(partial_surface, 12U) == 0xCF7FFFFFU &&
            partial_surface_failure.state.previous_opcode == 0x55U,
        "shared role-status preserves flags and partial clear effects before checked surface failure"
    );

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].flags = 0x80000001U;
    std::array<u8, 16U> exact_tail_surface{};
    exact_tail_surface.fill(0xFFU);
    bind_surface(exact_tail, 1U, exact_tail_surface);
    write_u16(exact_tail.state.window, 0x7FFAU, OP_174_SET_ROLE_STATUS_BIT14);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 1U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.roles[1].flags == 0x80004001U &&
            read_u32(exact_tail_surface, 0U) == 0xFF7FFFFFU &&
            exact_tail.state.previous_opcode == OP_174_SET_ROLE_STATUS_BIT14,
        "shared role-status commits flag, surface and previous before the next exact-tail fetch fails"
    );
}

void test_set_text_layout_pair_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        const u16 raw_word =
            static_cast<u16>(OP_104_SET_TEXT_LAYOUT_PAIR | alias_mask);
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        fixture.state.text_layout_first = 111;
        fixture.state.text_layout_second = -222;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x8000U);
        write_u16(fixture.state.window, 4U, 0x7FFFU);
        write_u16(fixture.state.window, 6U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        fixture.runtime.scene_render_flags = &scene_render_flags;

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.text_control_flags == 0xEFFFFFFFU &&
                fixture.state.text_layout_first == -32768 &&
                fixture.state.text_layout_second == 32767 &&
                fixture.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                scene_render_flags == 0xA5U,
            "opcode 104 aliases clear only text bit 28, sign-extend both values and continue"
        );
    }

    Fixture missing_first;
    missing_first.context.talk_data_offset = 0x1111U;
    missing_first.context.instruction_offset = 0x7FFEU;
    missing_first.state.loaded_file_number = 1U;
    missing_first.state.loaded_data_offset = 0x1111U;
    missing_first.state.window_loaded = true;
    missing_first.state.previous_opcode = 0x55U;
    missing_first.state.text_control_flags = 0xFFFFFFFFU;
    missing_first.state.text_layout_first = 11;
    missing_first.state.text_layout_second = 22;
    write_u16(missing_first.state.window, 0x7FFEU, OP_104_SET_TEXT_LAYOUT_PAIR);

    const auto missing_first_result = missing_first.step();
    test.expect_true(
        missing_first_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_first.context.instruction_offset == 0x7FFEU &&
            missing_first.state.text_control_flags == 0xEFFFFFFFU &&
            missing_first.state.text_layout_first == 11 &&
            missing_first.state.text_layout_second == 22 &&
            missing_first.state.previous_opcode == 0x55U,
        "opcode 104 clears text bit 28 before the first original operand access"
    );

    Fixture missing_second;
    missing_second.context.talk_data_offset = 0x1111U;
    missing_second.context.instruction_offset = 0x7FFCU;
    missing_second.state.loaded_file_number = 1U;
    missing_second.state.loaded_data_offset = 0x1111U;
    missing_second.state.window_loaded = true;
    missing_second.state.previous_opcode = 0x66U;
    missing_second.state.text_control_flags = 0xF5FFFFFFU;
    missing_second.state.text_layout_first = 11;
    missing_second.state.text_layout_second = 22;
    write_u16(
        missing_second.state.window, 0x7FFCU, OP_104_SET_TEXT_LAYOUT_PAIR
    );
    write_u16(missing_second.state.window, 0x7FFEU, 0xFFB0U);

    const auto missing_second_result = missing_second.step();
    test.expect_true(
        missing_second_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_second.context.instruction_offset == 0x7FFCU &&
            missing_second.state.text_control_flags == 0xE5FFFFFFU &&
            missing_second.state.text_layout_first == -80 &&
            missing_second.state.text_layout_second == 22 &&
            missing_second.state.previous_opcode == 0x66U,
        "opcode 104 commits bit clear and first signed value before the second operand access"
    );

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.text_control_flags = 0xFFFFFFFFU;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_104_SET_TEXT_LAYOUT_PAIR);
    write_u16(exact_tail.state.window, 0x7FFCU, 52U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFF88U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.text_control_flags == 0xEFFFFFFFU &&
            exact_tail.state.text_layout_first == 52 &&
            exact_tail.state.text_layout_second == -120 &&
            exact_tail.state.previous_opcode == OP_104_SET_TEXT_LAYOUT_PAIR,
        "opcode 104 commits both signed values and previous before the next exact-tail fetch fails"
    );
}

void test_clear_text_control_bit27_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        const u16 raw_word =
            static_cast<u16>(OP_105_CLEAR_TEXT_CONTROL_BIT27 | alias_mask);
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_95_CLEAR_SCENE_RENDER_BIT1);
        u8 scene_render_flags = 0xA7U;
        fixture.runtime.scene_render_flags = &scene_render_flags;

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.text_control_flags == 0xF7FFFFFFU &&
                fixture.state.previous_opcode ==
                    OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                scene_render_flags == 0xA5U,
            "opcode 105 aliases clear only text-control bit 27 and continue"
        );
    }

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.text_control_flags = 0xAFFFFFFFU;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_105_CLEAR_TEXT_CONTROL_BIT27
    );

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.text_control_flags == 0xA7FFFFFFU &&
            exact_tail.state.previous_opcode == OP_105_CLEAR_TEXT_CONTROL_BIT27,
        "opcode 105 commits bit clear and previous before the next exact-tail fetch fails"
    );
}

void test_clear_text_control_bit26_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_121_CLEAR_TEXT_CONTROL_BIT26 | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0xFBFFFFFFU &&
                fixture.state.previous_opcode ==
                    OP_121_CLEAR_TEXT_CONTROL_BIT26,
            "opcode 121 aliases clear only text-control bit 26 and continue"
        );
    }

    Fixture already_clear;
    already_clear.state.text_control_flags = 0xFBFFFFFFU;
    prime_loaded_instruction(already_clear, OP_121_CLEAR_TEXT_CONTROL_BIT26);
    write_u16(already_clear.state.window, 2U, kStoryVmTypedStop);
    const auto already_clear_result = already_clear.step();

    Fixture exact_tail;
    exact_tail.state.text_control_flags = 0xFFFFFFFFU;
    prime_loaded_instruction(exact_tail, OP_121_CLEAR_TEXT_CONTROL_BIT26);
    exact_tail.context.instruction_offset = 0x7FFEU;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_121_CLEAR_TEXT_CONTROL_BIT26
    );
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        already_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            already_clear.state.text_control_flags == 0xFBFFFFFFU &&
            already_clear.state.previous_opcode ==
                OP_121_CLEAR_TEXT_CONTROL_BIT26 &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.text_control_flags == 0xFBFFFFFFU &&
            exact_tail.state.previous_opcode == OP_121_CLEAR_TEXT_CONTROL_BIT26,
        "opcode 121 is idempotent and commits its exact-tail clear before refetch"
    );
}

void test_clear_speed_mode_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.speed_mode = 0x80000001U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_122_CLEAR_SPEED_MODE | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.speed_mode == 0U &&
                fixture.state.previous_opcode == OP_122_CLEAR_SPEED_MODE,
            "opcode 122 aliases clear the shared speed mode and continue"
        );
    }

    Fixture already_clear;
    prime_loaded_instruction(already_clear, OP_122_CLEAR_SPEED_MODE);
    write_u16(already_clear.state.window, 2U, kStoryVmTypedStop);
    const auto already_clear_result = already_clear.step();

    Fixture unavailable;
    unavailable.speed_mode = 1U;
    unavailable.runtime.speed_mode = nullptr;
    prime_loaded_instruction(unavailable, OP_122_CLEAR_SPEED_MODE);
    const auto unavailable_result = unavailable.step();

    Fixture exact_tail;
    exact_tail.speed_mode = 1U;
    prime_loaded_instruction(exact_tail, OP_122_CLEAR_SPEED_MODE);
    exact_tail.context.instruction_offset = 0x7FFEU;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_122_CLEAR_SPEED_MODE);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        already_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            already_clear.speed_mode == 0U &&
            already_clear.state.previous_opcode == OP_122_CLEAR_SPEED_MODE &&
            unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_result.executed_instruction_count == 1U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.speed_mode == 1U &&
            unavailable.state.previous_opcode == 0U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.speed_mode == 0U &&
            exact_tail.state.previous_opcode == OP_122_CLEAR_SPEED_MODE,
        "opcode 122 preserves the typed owner boundary and exact-tail order"
    );
}

void test_update_scene_music_table_entry_protocol(
    openswd3::test::Context& test
) {
    using Entry = std::array<u16, 4U>;
    const auto bind_table = [](Fixture& fixture,
                               const std::span<const Entry> entries) {
        constexpr u32 kFirstOffset = 0x20U;
        constexpr u32 kSecondOffset = 0x40U;
        constexpr u32 kTableOffset = 0x60U;
        fixture.maps_payload.fill(0xCCU);
        write_u32(fixture.maps_payload, 0x08U, kFirstOffset);
        write_u32(fixture.maps_payload, kFirstOffset + 4U, kSecondOffset);
        write_u32(fixture.maps_payload, kSecondOffset, kTableOffset);
        std::size_t offset = kTableOffset;
        for (const Entry& entry : entries) {
            for (std::size_t field = 0U; field < entry.size(); ++field) {
                write_u16(
                    fixture.maps_payload,
                    offset + field * sizeof(u16),
                    entry[field]
                );
            }
            offset += 8U;
        }
        write_u16(fixture.maps_payload, offset, 0U);
        fixture.runtime.mutable_maps_payload = fixture.maps_payload;
        return static_cast<std::size_t>(kTableOffset);
    };
    const auto write_instruction = [](Fixture& fixture,
                                      const u16 raw_word,
                                      const u16 key,
                                      const u16 value,
                                      const u16 third,
                                      const u16 diagnostic) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, key);
        write_u16(fixture.state.window, 4U, value);
        write_u16(fixture.state.window, 6U, third);
        write_u16(fixture.state.window, 8U, diagnostic);
    };

    constexpr std::array<Entry, 2U> kEntries{
        Entry{0x1234U, 0x1111U, 0x2222U, 0x3333U},
        Entry{0x4321U, 0xAAAAU, 0xBBBBU, 0xCCCCU},
    };
    constexpr std::array<u16, 4U> kAliasMasks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : kAliasMasks) {
        Fixture fixture;
        const std::size_t table = bind_table(fixture, kEntries);
        write_instruction(
            fixture,
            static_cast<u16>(
                OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY | alias_mask
            ),
            0x1234U,
            0x5678U,
            0x9ABCU,
            0xDEF0U
        );
        write_u16(fixture.state.window, 10U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 10U &&
                read_u32(fixture.maps_payload, table) == 0x56781234U &&
                read_u16(fixture.maps_payload, table + 4U) == 0x9ABCU &&
                read_u16(fixture.maps_payload, table + 6U) == 0x3333U &&
                fixture.state.previous_opcode ==
                    OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
            "opcode 123 aliases replace only the matched entry's first six bytes"
        );
    }

    Fixture current_source;
    current_source.context.source_guid = 0x1234U;
    const std::size_t current_table = bind_table(current_source, kEntries);
    write_instruction(
        current_source,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0xFFF0U,
        0x1357U,
        0x2468U,
        0xAAAAU
    );
    write_u16(current_source.state.window, 10U, kStoryVmTypedStop);
    const auto current_result = current_source.step();

    constexpr std::array<Entry, 2U> kDuplicateEntries{
        Entry{0x7777U, 1U, 2U, 3U},
        Entry{0x7777U, 4U, 5U, 6U},
    };
    Fixture first_match;
    const std::size_t duplicate_table =
        bind_table(first_match, kDuplicateEntries);
    write_instruction(
        first_match,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0x7777U,
        0x8888U,
        0x9999U,
        0xAAAAU
    );
    write_u16(first_match.state.window, 10U, kStoryVmTypedStop);
    const auto first_match_result = first_match.step();

    test.expect_true(
        current_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            read_u16(current_source.maps_payload, current_table) == 0xFFF0U &&
            read_u16(current_source.maps_payload, current_table + 2U) ==
                0x1357U &&
            read_u16(current_source.maps_payload, current_table + 4U) ==
                0x2468U &&
            first_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            read_u16(first_match.maps_payload, duplicate_table + 2U) ==
                0x8888U &&
            read_u16(first_match.maps_payload, duplicate_table + 4U) ==
                0x9999U &&
            read_u16(first_match.maps_payload, duplicate_table + 10U) == 4U &&
            read_u16(first_match.maps_payload, duplicate_table + 12U) == 5U,
        "opcode 123 uses FFF0 only for matching and rewrites only the first match"
    );

    Fixture missing;
    const std::size_t missing_table = bind_table(missing, kEntries);
    write_instruction(
        missing,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0x9999U,
        0x1111U,
        0x2222U,
        0x3333U
    );
    write_u16(missing.state.window, 10U, kStoryVmTypedStop);
    const auto missing_result = missing.step();

    Fixture selector_tail;
    bind_table(selector_tail, kEntries);
    prime_loaded_instruction(
        selector_tail, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    selector_tail.context.instruction_offset = 0x7FFEU;
    write_u16(
        selector_tail.state.window,
        0x7FFEU,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    const auto selector_tail_result = selector_tail.step();

    Fixture dword_tail;
    const std::size_t dword_tail_table = bind_table(dword_tail, kEntries);
    prime_loaded_instruction(dword_tail, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY);
    dword_tail.context.instruction_offset = 0x7FFCU;
    write_u16(
        dword_tail.state.window, 0x7FFCU, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    write_u16(dword_tail.state.window, 0x7FFEU, 0x1234U);
    const auto dword_tail_result = dword_tail.step();

    Fixture third_tail;
    const std::size_t third_tail_table = bind_table(third_tail, kEntries);
    prime_loaded_instruction(third_tail, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY);
    third_tail.context.instruction_offset = 0x7FFAU;
    write_u16(
        third_tail.state.window, 0x7FFAU, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    write_u16(third_tail.state.window, 0x7FFCU, 0x1234U);
    write_u16(third_tail.state.window, 0x7FFEU, 0x5678U);
    const auto third_tail_result = third_tail.step();

    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            read_u16(missing.maps_payload, missing_table) == 0x1234U &&
            selector_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_tail.context.instruction_offset == 0x7FFEU &&
            dword_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            read_u32(dword_tail.maps_payload, dword_tail_table) ==
                0x11111234U &&
            third_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            read_u32(third_tail.maps_payload, third_tail_table) ==
                0x56781234U &&
            read_u16(third_tail.maps_payload, third_tail_table + 4U) == 0x2222U,
        "opcode 123 preserves selector, dword and third-word staged access order"
    );

    Fixture unread_tail;
    const std::size_t unread_tail_table = bind_table(unread_tail, kEntries);
    prime_loaded_instruction(
        unread_tail, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    unread_tail.context.instruction_offset = 0x7FF8U;
    write_u16(
        unread_tail.state.window, 0x7FF8U, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    write_u16(unread_tail.state.window, 0x7FFAU, 0x1234U);
    write_u16(unread_tail.state.window, 0x7FFCU, 0x5678U);
    write_u16(unread_tail.state.window, 0x7FFEU, 0x9ABCU);
    const auto unread_tail_result = unread_tail.step();

    Fixture missing_diagnostic_tail;
    bind_table(missing_diagnostic_tail, kEntries);
    prime_loaded_instruction(
        missing_diagnostic_tail, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    missing_diagnostic_tail.context.instruction_offset = 0x7FF8U;
    write_u16(
        missing_diagnostic_tail.state.window,
        0x7FF8U,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
    );
    write_u16(missing_diagnostic_tail.state.window, 0x7FFAU, 0x9999U);
    write_u16(missing_diagnostic_tail.state.window, 0x7FFCU, 0x5678U);
    write_u16(missing_diagnostic_tail.state.window, 0x7FFEU, 0x9ABCU);
    const auto missing_diagnostic_tail_result = missing_diagnostic_tail.step();

    test.expect_true(
        unread_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            unread_tail_result.executed_instruction_count == 1U &&
            unread_tail.context.instruction_offset == 0x8002U &&
            read_u32(unread_tail.maps_payload, unread_tail_table) ==
                0x56781234U &&
            read_u16(unread_tail.maps_payload, unread_tail_table + 4U) ==
                0x9ABCU &&
            unread_tail.state.previous_opcode ==
                OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY &&
            missing_diagnostic_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_diagnostic_tail.context.instruction_offset == 0x7FF8U &&
            missing_diagnostic_tail.state.previous_opcode == 0U,
        "opcode 123 leaves +8 unread on success but requires it for miss diagnostics"
    );

    Fixture missing_payload;
    write_instruction(
        missing_payload,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0x1234U,
        0x5678U,
        0x9ABCU,
        0xDEF0U
    );
    const auto missing_payload_result = missing_payload.step();

    Fixture dword_destination;
    dword_destination.maps_payload.fill(0U);
    write_u32(dword_destination.maps_payload, 0x08U, 0x20U);
    write_u32(dword_destination.maps_payload, 0x24U, 0x40U);
    write_u32(dword_destination.maps_payload, 0x40U, 0xFEU);
    write_u16(dword_destination.maps_payload, 0xFEU, 0x1234U);
    dword_destination.runtime.mutable_maps_payload =
        dword_destination.maps_payload;
    write_instruction(
        dword_destination,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0x1234U,
        0x5678U,
        0x9ABCU,
        0xDEF0U
    );
    const auto dword_destination_result = dword_destination.step();

    Fixture third_destination;
    third_destination.maps_payload.fill(0U);
    write_u32(third_destination.maps_payload, 0x08U, 0x20U);
    write_u32(third_destination.maps_payload, 0x24U, 0x40U);
    write_u32(third_destination.maps_payload, 0x40U, 0xFCU);
    write_u16(third_destination.maps_payload, 0xFCU, 0x1234U);
    write_u16(third_destination.maps_payload, 0xFEU, 0x1111U);
    third_destination.runtime.mutable_maps_payload =
        third_destination.maps_payload;
    write_instruction(
        third_destination,
        OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
        0x1234U,
        0x5678U,
        0x9ABCU,
        0xDEF0U
    );
    const auto third_destination_result = third_destination.step();

    u32 broken_chain_count{};
    for (u32 variant = 0U; variant < 4U; ++variant) {
        Fixture fixture;
        fixture.maps_payload.fill(0U);
        write_u32(fixture.maps_payload, 0x08U, 0x20U);
        write_u32(fixture.maps_payload, 0x24U, 0x40U);
        write_u32(fixture.maps_payload, 0x40U, 0x60U);
        if (variant == 0U) {
            write_u32(fixture.maps_payload, 0x08U, 0xFEU);
        } else if (variant == 1U) {
            write_u32(fixture.maps_payload, 0x24U, 0xFEU);
        } else if (variant == 2U) {
            write_u32(fixture.maps_payload, 0x40U, 0x100U);
        } else {
            write_u32(fixture.maps_payload, 0x40U, 0xF8U);
            write_u16(fixture.maps_payload, 0xF8U, 0x9999U);
        }
        fixture.runtime.mutable_maps_payload = fixture.maps_payload;
        write_instruction(
            fixture,
            OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
            0x1234U,
            0x5678U,
            0x9ABCU,
            0xDEF0U
        );
        const auto result = fixture.step();
        if (result.status ==
                LegacyWorldStoryVmStatus::maps_payload_out_of_range &&
            fixture.context.instruction_offset == 0U &&
            fixture.state.previous_opcode == 0U) {
            ++broken_chain_count;
        }
    }

    test.expect_true(
        missing_payload_result.status ==
                LegacyWorldStoryVmStatus::maps_payload_out_of_range &&
            dword_destination_result.status ==
                LegacyWorldStoryVmStatus::maps_payload_out_of_range &&
            read_u16(dword_destination.maps_payload, 0xFEU) == 0x1234U &&
            third_destination_result.status ==
                LegacyWorldStoryVmStatus::maps_payload_out_of_range &&
            read_u32(third_destination.maps_payload, 0xFCU) == 0x56781234U &&
            broken_chain_count == 4U,
        "opcode 123 stops at each checked MAPS chain and staged write boundary"
    );
}

void test_clear_text_control_bit25_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_124_CLEAR_TEXT_CONTROL_BIT25 | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0xFDFFFFFFU &&
                fixture.state.previous_opcode ==
                    OP_124_CLEAR_TEXT_CONTROL_BIT25,
            "opcode 124 aliases clear only text-control bit 25 and continue"
        );
    }

    Fixture already_clear;
    already_clear.state.text_control_flags = 0xFDFFFFFFU;
    prime_loaded_instruction(already_clear, OP_124_CLEAR_TEXT_CONTROL_BIT25);
    write_u16(already_clear.state.window, 2U, kStoryVmTypedStop);
    const auto already_clear_result = already_clear.step();

    Fixture exact_tail;
    exact_tail.state.text_control_flags = 0xFFFFFFFFU;
    prime_loaded_instruction(exact_tail, OP_124_CLEAR_TEXT_CONTROL_BIT25);
    exact_tail.context.instruction_offset = 0x7FFEU;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_124_CLEAR_TEXT_CONTROL_BIT25
    );
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        already_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            already_clear.state.text_control_flags == 0xFDFFFFFFU &&
            already_clear.state.previous_opcode ==
                OP_124_CLEAR_TEXT_CONTROL_BIT25 &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.text_control_flags == 0xFDFFFFFFU &&
            exact_tail.state.previous_opcode == OP_124_CLEAR_TEXT_CONTROL_BIT25,
        "opcode 124 is idempotent and commits its exact-tail clear before refetch"
    );
}

void test_append_text_allocation_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_125_APPEND_TEXT_ALLOCATION | alias_mask)
        );
        fixture.state.window[2U] = static_cast<u8>('A');
        fixture.state.window[3U] = static_cast<u8>('B');
        fixture.state.window[4U] = static_cast<u8>('%');
        fixture.state.window[5U] = static_cast<u8>('Q');
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);

        const auto result = fixture.step();
        const auto& allocation = fixture.state.text_allocation_chain.back();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_125_APPEND_TEXT_ALLOCATION | alias_mask
                    ) &&
                result.opcode == OP_125_APPEND_TEXT_ALLOCATION &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.text_allocation_chain.size() == 1U &&
                allocation[0U] == static_cast<u8>('A') &&
                allocation[1U] == static_cast<u8>('B') &&
                allocation[2U] == static_cast<u8>('%') &&
                allocation[3U] == static_cast<u8>('Q') &&
                allocation[4U] == 0U && allocation[5U] == 0U &&
                allocation.back() == 0U &&
                fixture.state.previous_opcode == OP_125_APPEND_TEXT_ALLOCATION,
            "opcode 125 aliases append one zero-filled terminated allocation and yield"
        );

        const auto successor = fixture.step();
        test.expect_true(
            successor.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                successor.opcode == kStoryVmTypedStop &&
                successor.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 6U,
            "opcode 125 leaves its successor for the next VM call"
        );
    }

    Fixture persistent_chain;
    prime_loaded_instruction(persistent_chain, OP_125_APPEND_TEXT_ALLOCATION);
    persistent_chain.state.window[2U] = static_cast<u8>('%');
    persistent_chain.state.window[3U] = static_cast<u8>('Q');
    write_u16(persistent_chain.state.window, 4U, OP_125_APPEND_TEXT_ALLOCATION);
    persistent_chain.state.window[6U] = static_cast<u8>('X');
    persistent_chain.state.window[7U] = static_cast<u8>('%');
    persistent_chain.state.window[8U] = static_cast<u8>('Q');
    const auto first_append = persistent_chain.step();
    const auto second_append = persistent_chain.step();
    openswd3::world_map::initialize_legacy_world_story_vm(
        persistent_chain.state
    );
    const auto first_allocation =
        persistent_chain.state.text_allocation_chain.begin();
    const auto second_allocation = std::next(first_allocation);

    test.expect_true(
        first_append.status == LegacyWorldStoryVmStatus::yielded &&
            first_append.executed_instruction_count == 1U &&
            second_append.status == LegacyWorldStoryVmStatus::yielded &&
            second_append.executed_instruction_count == 1U &&
            persistent_chain.context.instruction_offset == 9U &&
            persistent_chain.state.text_allocation_chain.size() == 2U &&
            (*first_allocation)[0U] == static_cast<u8>('%') &&
            (*first_allocation)[1U] == static_cast<u8>('Q') &&
            (*first_allocation)[2U] == 0U &&
            (*second_allocation)[0U] == static_cast<u8>('X') &&
            (*second_allocation)[1U] == static_cast<u8>('%') &&
            (*second_allocation)[2U] == static_cast<u8>('Q') &&
            (*second_allocation)[3U] == 0U,
        "opcode 125 appends in order and the process allocation chain survives VM initialization"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, OP_125_APPEND_TEXT_ALLOCATION);
    missing_terminator.context.instruction_offset = 0x7FFCU;
    write_u16(
        missing_terminator.state.window, 0x7FFCU, OP_125_APPEND_TEXT_ALLOCATION
    );
    missing_terminator.state.window[0x7FFEU] = static_cast<u8>('A');
    missing_terminator.state.window[0x7FFFU] = static_cast<u8>('B');
    const auto missing_result = missing_terminator.step();

    Fixture copy_overflow;
    prime_loaded_instruction(copy_overflow, OP_125_APPEND_TEXT_ALLOCATION);
    std::ranges::fill_n(
        copy_overflow.state.window.begin() + 2U, 257U, static_cast<u8>('A')
    );
    copy_overflow.state.window[259U] = static_cast<u8>('%');
    copy_overflow.state.window[260U] = static_cast<u8>('Q');
    const auto copy_overflow_result = copy_overflow.step();

    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::
                    text_allocation_terminator_not_found &&
            missing_terminator.context.instruction_offset == 0x7FFCU &&
            missing_terminator.state.text_allocation_chain.size() == 1U &&
            missing_terminator.state.text_allocation_chain.back()[0U] ==
                static_cast<u8>('A') &&
            missing_terminator.state.text_allocation_chain.back()[1U] == 0U &&
            missing_terminator.state.previous_opcode == 0U &&
            copy_overflow_result.status ==
                LegacyWorldStoryVmStatus::text_allocation_out_of_range &&
            copy_overflow.context.instruction_offset == 0U &&
            copy_overflow.state.text_allocation_chain.size() == 1U &&
            std::ranges::all_of(
                copy_overflow.state.text_allocation_chain.back(),
                [](const u8 byte) { return byte == static_cast<u8>('A'); }
            ) &&
            copy_overflow.state.previous_opcode == 0U,
        "opcode 125 retains its linked allocation and staged copy on malformed input"
    );

    Fixture suffix_overflow;
    prime_loaded_instruction(suffix_overflow, OP_125_APPEND_TEXT_ALLOCATION);
    std::ranges::fill_n(
        suffix_overflow.state.window.begin() + 2U, 254U, static_cast<u8>('A')
    );
    suffix_overflow.state.window[256U] = static_cast<u8>('%');
    suffix_overflow.state.window[257U] = static_cast<u8>('Q');
    const auto suffix_overflow_result = suffix_overflow.step();
    const auto& suffix_allocation =
        suffix_overflow.state.text_allocation_chain.back();

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_125_APPEND_TEXT_ALLOCATION);
    exact_tail.context.instruction_offset = 0x7FFAU;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_125_APPEND_TEXT_ALLOCATION);
    exact_tail.state.window[0x7FFCU] = static_cast<u8>('A');
    exact_tail.state.window[0x7FFDU] = static_cast<u8>('B');
    exact_tail.state.window[0x7FFEU] = static_cast<u8>('%');
    exact_tail.state.window[0x7FFFU] = static_cast<u8>('Q');
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        suffix_overflow_result.status ==
                LegacyWorldStoryVmStatus::text_allocation_out_of_range &&
            suffix_overflow.context.instruction_offset == 258U &&
            suffix_allocation[253U] == static_cast<u8>('A') &&
            suffix_allocation[254U] == static_cast<u8>('%') &&
            suffix_allocation[255U] == static_cast<u8>('Q') &&
            suffix_overflow.state.previous_opcode == 0U &&
            suffix_overflow.ports.direct_audio_service_count == 0U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.text_allocation_chain.back()[0U] ==
                static_cast<u8>('A') &&
            exact_tail.state.text_allocation_chain.back()[4U] == 0U &&
            exact_tail.state.previous_opcode == OP_125_APPEND_TEXT_ALLOCATION,
        "opcode 125 advances before staged suffix writes and completes an exact-tail record"
    );
}

void test_role_base_variant_reload_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        u16 reload_value;
        u16 sequential_value;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL, 8U, 9U},
        Variant{OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL, 9U, 8U},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr u32 target = 0x12345678U;
    for (const auto variant : variants) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            fixture.roles[1].guid = 0x2222U;
            fixture.roles[1].action.base_variant = 8U;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias_mask)
            );
            write_u16(fixture.state.window, 2U, 0x2222U);
            write_u16(fixture.state.window, 4U, variant.reload_value);
            write_u32(fixture.state.window, 6U, target);
            write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);

            const auto result = fixture.step();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.context.talk_data_offset == target &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.loaded_data_offset == target &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.data_load_count == 1U &&
                    fixture.ports.last_data_offset == target &&
                    !fixture.ports.last_data_clear_before_read &&
                    fixture.ports.story_protocol_events ==
                        std::vector<u32>{2U, 5U},
                "opcodes 126 and 127 aliases apply their inverse reload predicates"
            );
        }

        Fixture sequential;
        sequential.roles[1].guid = 0x2222U;
        sequential.roles[1].action.base_variant = 8U;
        prime_loaded_instruction(sequential, variant.opcode);
        write_u16(sequential.state.window, 2U, 0x2222U);
        write_u16(sequential.state.window, 4U, variant.sequential_value);
        write_u32(sequential.state.window, 6U, target);
        write_u16(sequential.state.window, 10U, kStoryVmTypedStop);

        const auto result = sequential.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                sequential.context.instruction_offset == 10U &&
                sequential.state.previous_opcode == variant.opcode &&
                sequential.ports.data_load_count == 0U,
            "opcodes 126 and 127 consume ten bytes and same-call fetch when their predicate is false"
        );
    }

    Fixture full_width;
    full_width.roles[1].guid = 0x2222U;
    full_width.roles[1].action.base_variant = 0x00010008U;
    prime_loaded_instruction(
        full_width, OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(full_width.state.window, 2U, 0x2222U);
    write_u16(full_width.state.window, 4U, 8U);
    write_u16(full_width.state.window, 10U, kStoryVmTypedStop);
    const auto full_width_result = full_width.step();

    Fixture current_source;
    current_source.roles[1].guid = current_source.context.source_guid;
    current_source.roles[1].action.base_variant = 8U;
    prime_loaded_instruction(
        current_source, OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 8U);
    write_u32(current_source.state.window, 6U, target);
    write_u16(current_source.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto current_source_result = current_source.step();

    Fixture controlled;
    controlled.roles[2].guid = 0x3333U;
    controlled.roles[2].action.base_variant = 8U;
    prime_loaded_instruction(
        controlled, OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL
    );
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 9U);
    write_u32(controlled.state.window, 6U, target);
    write_u16(controlled.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);

    test.expect_true(
        full_width_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            full_width.context.instruction_offset == 10U &&
            full_width.ports.data_load_count == 0U &&
            current_source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_source.ports.data_load_count == 1U &&
            controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.ports.data_load_count == 1U,
        "role base-variant reload compares the full runtime dword and preserves FFF0 and FFFE lookup rules"
    );

    Fixture materialized_equal;
    MapRoleWriteHarness equal_maps{materialized_equal};
    equal_maps.add_source(
        openswd3::world_map::LegacyMapsRoleSourceRecord{
            .logical_map_id = 5U,
            .guid = 0x4444U,
            .action_id = 7U,
            .base_variant = 8U,
        }
    );
    prime_loaded_instruction(
        materialized_equal, OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(materialized_equal.state.window, 2U, 0x4444U);
    write_u16(materialized_equal.state.window, 4U, 8U);
    write_u32(materialized_equal.state.window, 6U, target);
    write_u16(
        materialized_equal.ports.transferred_window, 0U, kStoryVmTypedStop
    );
    const auto materialized_equal_result = materialized_equal.step();

    Fixture materialized_not_equal;
    MapRoleWriteHarness not_equal_maps{materialized_not_equal};
    not_equal_maps.add_source(
        openswd3::world_map::LegacyMapsRoleSourceRecord{
            .logical_map_id = 5U,
            .guid = 0x4444U,
            .action_id = 7U,
            .base_variant = 8U,
        }
    );
    prime_loaded_instruction(
        materialized_not_equal, OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL
    );
    write_u16(materialized_not_equal.state.window, 2U, 0x4444U);
    write_u16(materialized_not_equal.state.window, 4U, 8U);
    write_u16(materialized_not_equal.state.window, 10U, kStoryVmTypedStop);
    const auto materialized_not_equal_result = materialized_not_equal.step();

    test.expect_true(
        materialized_equal_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            materialized_equal.ports.data_load_count == 1U &&
            materialized_not_equal_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            materialized_not_equal.context.instruction_offset == 10U &&
            materialized_not_equal.ports.data_load_count == 0U,
        "opcodes 126 and 127 materialize a missing live role from the mutable MAPS source"
    );

    Fixture missing_database;
    prime_loaded_instruction(
        missing_database, OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(missing_database.state.window, 2U, 0x5555U);
    write_u16(missing_database.state.window, 4U, 8U);
    const auto missing_database_result = missing_database.step();

    Fixture missing_source;
    MapRoleWriteHarness empty_maps{missing_source};
    prime_loaded_instruction(
        missing_source, OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(missing_source.state.window, 2U, 0x5555U);
    const auto missing_source_result = missing_source.step();

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    const auto selector_truncated_result = selector_truncated.step();

    Fixture compare_truncated;
    compare_truncated.context.instruction_offset = 0x7FFCU;
    compare_truncated.context.talk_data_offset = 0x1111U;
    compare_truncated.state.loaded_file_number = 1U;
    compare_truncated.state.loaded_data_offset = 0x1111U;
    compare_truncated.state.window_loaded = true;
    compare_truncated.roles[1].guid = 0x2222U;
    compare_truncated.roles[1].action.base_variant = 8U;
    write_u16(
        compare_truncated.state.window,
        0x7FFCU,
        OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(compare_truncated.state.window, 0x7FFEU, 0x2222U);
    const auto compare_truncated_result = compare_truncated.step();

    test.expect_true(
        missing_database_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_database.context.instruction_offset == 0U &&
            missing_database.state.previous_opcode == 0U &&
            missing_database.ports.data_load_count == 0U &&
            missing_source_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            missing_source.context.instruction_offset == 0U &&
            missing_source.state.previous_opcode == 0U &&
            missing_source.ports.data_load_count == 0U &&
            selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            compare_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            compare_truncated.context.instruction_offset == 0x7FFCU &&
            compare_truncated.state.previous_opcode == 0U,
        "role base-variant reload preserves selector, MAPS materialization, and compare-operand failure order"
    );

    Fixture no_target_equal;
    no_target_equal.context.instruction_offset = 0x7FFAU;
    no_target_equal.context.talk_data_offset = 0x1111U;
    no_target_equal.state.loaded_file_number = 1U;
    no_target_equal.state.loaded_data_offset = 0x1111U;
    no_target_equal.state.window_loaded = true;
    no_target_equal.roles[1].guid = 0x2222U;
    no_target_equal.roles[1].action.base_variant = 8U;
    write_u16(
        no_target_equal.state.window,
        0x7FFAU,
        OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(no_target_equal.state.window, 0x7FFCU, 0x2222U);
    write_u16(no_target_equal.state.window, 0x7FFEU, 9U);
    const auto no_target_equal_result = no_target_equal.step();

    Fixture no_target_not_equal;
    no_target_not_equal.context.instruction_offset = 0x7FFAU;
    no_target_not_equal.context.talk_data_offset = 0x1111U;
    no_target_not_equal.state.loaded_file_number = 1U;
    no_target_not_equal.state.loaded_data_offset = 0x1111U;
    no_target_not_equal.state.window_loaded = true;
    no_target_not_equal.roles[1].guid = 0x2222U;
    no_target_not_equal.roles[1].action.base_variant = 8U;
    write_u16(
        no_target_not_equal.state.window,
        0x7FFAU,
        OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL
    );
    write_u16(no_target_not_equal.state.window, 0x7FFCU, 0x2222U);
    write_u16(no_target_not_equal.state.window, 0x7FFEU, 8U);
    const auto no_target_not_equal_result = no_target_not_equal.step();

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFAU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.roles[1].guid = 0x2222U;
    target_truncated.roles[1].action.base_variant = 8U;
    write_u16(
        target_truncated.state.window,
        0x7FFAU,
        OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL
    );
    write_u16(target_truncated.state.window, 0x7FFCU, 0x2222U);
    write_u16(target_truncated.state.window, 0x7FFEU, 8U);
    const auto target_truncated_result = target_truncated.step();

    test.expect_true(
        no_target_equal_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_equal_result.executed_instruction_count == 1U &&
            no_target_equal.context.instruction_offset == 0x8004U &&
            no_target_equal.state.previous_opcode ==
                OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL &&
            no_target_equal.ports.data_load_count == 0U &&
            no_target_not_equal_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_not_equal_result.executed_instruction_count == 1U &&
            no_target_not_equal.context.instruction_offset == 0x8004U &&
            no_target_not_equal.state.previous_opcode ==
                OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL &&
            no_target_not_equal.ports.data_load_count == 0U &&
            target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFAU &&
            target_truncated.state.previous_opcode == 0U &&
            target_truncated.ports.data_load_count == 0U,
        "role base-variant reload reads its u32 target only on the taken branch"
    );

    Fixture load_failure;
    load_failure.roles[1].guid = 0x2222U;
    load_failure.roles[1].action.base_variant = 8U;
    prime_loaded_instruction(
        load_failure, OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL
    );
    write_u16(load_failure.state.window, 2U, 0x2222U);
    write_u16(load_failure.state.window, 4U, 9U);
    write_u32(load_failure.state.window, 6U, target);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();

    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.opcode ==
                OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "role base-variant reload preserves loader side effects and previous publication on checked I/O failure"
    );
}

void test_adjust_player_item_quantity_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.player_inventory.emplace_back();
        auto& item = fixture.player_inventory.back();
        item.item_id = 0x0123U;
        item.quantity_a = 4U;
        item.quantity_b = 5U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_128_ADJUST_PLAYER_ITEM_QUANTITY | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 2U);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
                fixture.player_inventory.size() == 1U &&
                fixture.player_inventory.front().quantity_a == 4U &&
                fixture.player_inventory.front().quantity_b == 7U &&
                fixture.ports.item_definition_load_count == 0U &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U},
            "opcode 128 aliases update an existing item then publish previous, service audio, and yield"
        );
    }

    Fixture clamp;
    clamp.player_inventory.emplace_back();
    clamp.player_inventory.front().item_id = 0x0200U;
    clamp.player_inventory.front().quantity_a = 9U;
    clamp.player_inventory.front().quantity_b = 98U;
    prime_loaded_instruction(clamp, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(clamp.state.window, 2U, 0x0200U);
    write_u16(clamp.state.window, 4U, 2U);
    const auto clamp_result = clamp.step();

    Fixture transfer;
    transfer.player_inventory.emplace_back();
    transfer.player_inventory.front().item_id = 0x0201U;
    transfer.player_inventory.front().quantity_a = 3U;
    transfer.player_inventory.front().quantity_b = 1U;
    prime_loaded_instruction(transfer, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(transfer.state.window, 2U, 0x0201U);
    write_u16(transfer.state.window, 4U, 0xFFFEU);
    const auto transfer_result = transfer.step();

    Fixture remove;
    remove.player_inventory.emplace_back();
    remove.player_inventory.front().item_id = 0x0202U;
    remove.player_inventory.front().quantity_a = 1U;
    prime_loaded_instruction(remove, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(remove.state.window, 2U, 0x0202U);
    write_u16(remove.state.window, 4U, 0xFFFFU);
    const auto remove_result = remove.step();

    Fixture wrapped_remove;
    wrapped_remove.player_inventory.emplace_back();
    wrapped_remove.player_inventory.front().item_id = 0x0203U;
    wrapped_remove.player_inventory.front().quantity_b = 0x7FFFU;
    prime_loaded_instruction(
        wrapped_remove, OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(wrapped_remove.state.window, 2U, 0x0203U);
    write_u16(wrapped_remove.state.window, 4U, 1U);
    const auto wrapped_remove_result = wrapped_remove.step();

    Fixture sentinel;
    sentinel.player_inventory.emplace_back();
    sentinel.player_inventory.front().item_id =
        openswd3::world_map::kLegacyItemSentinelId;
    sentinel.player_inventory.front().quantity_a = 7U;
    sentinel.player_inventory.front().quantity_b = 1U;
    prime_loaded_instruction(sentinel, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(
        sentinel.state.window, 2U, openswd3::world_map::kLegacyItemSentinelId
    );
    write_u16(sentinel.state.window, 4U, 1U);
    const auto sentinel_result = sentinel.step();

    test.expect_true(
        clamp_result.status == LegacyWorldStoryVmStatus::yielded &&
            clamp.player_inventory.front().quantity_a == 0U &&
            clamp.player_inventory.front().quantity_b == 99U &&
            transfer_result.status == LegacyWorldStoryVmStatus::yielded &&
            transfer.player_inventory.front().quantity_a == 2U &&
            transfer.player_inventory.front().quantity_b == 0U &&
            remove_result.status == LegacyWorldStoryVmStatus::yielded &&
            remove.player_inventory.empty() &&
            wrapped_remove_result.status == LegacyWorldStoryVmStatus::yielded &&
            wrapped_remove.player_inventory.empty() &&
            sentinel_result.status == LegacyWorldStoryVmStatus::yielded &&
            sentinel.player_inventory.front().quantity_a == 1U &&
            sentinel.player_inventory.front().quantity_b == 0U,
        "opcode 128 preserves signed i16 wrapping, the 99 clamp, A/B transfer, deletion, and FFDC normalization"
    );

    Fixture create;
    create.player_inventory.emplace_back();
    create.player_inventory.front().item_id = 0x0010U;
    create.ports.prepared_item_definition.fill(0x22U);
    create.ports.prepared_item_description = {1U, 2U, 3U};
    prime_loaded_instruction(create, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(create.state.window, 2U, 0x0234U);
    write_u16(create.state.window, 4U, 8U);
    const auto create_result = create.step();
    const auto& created = create.player_inventory.front();

    Fixture create_sentinel;
    prime_loaded_instruction(
        create_sentinel, OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(
        create_sentinel.state.window,
        2U,
        openswd3::world_map::kLegacyItemSentinelId
    );
    write_u16(create_sentinel.state.window, 4U, 8U);
    const auto create_sentinel_result = create_sentinel.step();
    const auto& created_sentinel = create_sentinel.player_inventory.front();

    test.expect_true(
        create_result.status == LegacyWorldStoryVmStatus::yielded &&
            create.player_inventory.size() == 2U &&
            created.item_id == 0x0234U && created.selected_count == 0U &&
            created.quantity_a == 0U && created.quantity_b == 8U &&
            created.definition_snapshot[0x20U] == 0x22U &&
            created.definition_snapshot[0x21U] == 0xA2U &&
            created.description == std::vector<u8>{1U, 2U, 3U} &&
            create.ports.item_definition_load_count == 1U &&
            create.ports.last_item_definition_id == 0x0234U &&
            create.ports.story_protocol_events == std::vector<u32>{13U, 2U} &&
            create_sentinel_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            create_sentinel.ports.item_definition_load_count == 0U &&
            created_sentinel.item_id ==
                openswd3::world_map::kLegacyItemSentinelId &&
            created_sentinel.quantity_a == 0U &&
            created_sentinel.quantity_b == 1U &&
            created_sentinel.definition_snapshot[0U] ==
                openswd3::world_map::kLegacyItemSentinelNameBytes[0U] &&
            created_sentinel.definition_snapshot[1U] ==
                openswd3::world_map::kLegacyItemSentinelNameBytes[1U] &&
            created_sentinel.definition_snapshot[0x21U] == 0x80U,
        "opcode 128 prepends a loaded item snapshot and preserves the FFDC construction path"
    );

    Fixture missing_nonpositive;
    prime_loaded_instruction(
        missing_nonpositive, OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(missing_nonpositive.state.window, 2U, 0x0300U);
    write_u16(missing_nonpositive.state.window, 4U, 0xFFFFU);
    const auto missing_nonpositive_result = missing_nonpositive.step();

    Fixture definition_failure;
    definition_failure.ports.item_definition_load_success = false;
    prime_loaded_instruction(
        definition_failure, OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(definition_failure.state.window, 2U, 0x0301U);
    write_u16(definition_failure.state.window, 4U, 1U);
    const auto definition_failure_result = definition_failure.step();

    Fixture missing_owner;
    missing_owner.runtime.player_inventory = nullptr;
    prime_loaded_instruction(missing_owner, OP_128_ADJUST_PLAYER_ITEM_QUANTITY);
    write_u16(missing_owner.state.window, 2U, 0x0302U);
    write_u16(missing_owner.state.window, 4U, 1U);
    const auto missing_owner_result = missing_owner.step();

    test.expect_true(
        missing_nonpositive_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            missing_nonpositive.player_inventory.empty() &&
            missing_nonpositive.ports.item_definition_load_count == 0U &&
            definition_failure_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            definition_failure.player_inventory.empty() &&
            definition_failure.ports.item_definition_load_count == 1U &&
            definition_failure.context.instruction_offset == 6U &&
            definition_failure.state.previous_opcode ==
                OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
            definition_failure.ports.direct_audio_service_count == 1U &&
            missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0U &&
            missing_owner.ports.direct_audio_service_count == 0U,
        "opcode 128 ignores ordinary helper misses but stops at the fixed player-inventory owner boundary"
    );

    Fixture operand_truncated;
    operand_truncated.context.instruction_offset = 0x7FFCU;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.player_inventory.emplace_back();
    operand_truncated.player_inventory.front().item_id = 0x0400U;
    operand_truncated.player_inventory.front().quantity_b = 1U;
    write_u16(
        operand_truncated.state.window,
        0x7FFCU,
        OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(operand_truncated.state.window, 0x7FFEU, 0x0400U);
    const auto operand_truncated_result = operand_truncated.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.player_inventory.emplace_back();
    exact_tail.player_inventory.front().item_id = 0x0401U;
    exact_tail.player_inventory.front().quantity_b = 1U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_128_ADJUST_PLAYER_ITEM_QUANTITY
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0x0401U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFFFU);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFCU &&
            operand_truncated.state.previous_opcode == 0U &&
            operand_truncated.player_inventory.front().quantity_b == 1U &&
            operand_truncated.ports.item_definition_load_count == 0U &&
            operand_truncated.ports.direct_audio_service_count == 0U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
            exact_tail.player_inventory.empty() &&
            exact_tail.ports.direct_audio_service_count == 1U,
        "opcode 128 reads signed delta before item id and completes an exact-tail record before yielding"
    );
}

void test_item_presence_reload_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        bool role_root_only;
        bool inverted;
    };

    constexpr std::array<Variant, 4U> variants{
        Variant{OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM, false, false},
        Variant{OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM, false, true},
        Variant{
            OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM,
            true,
            false,
        },
        Variant{
            OP_168_RELOAD_IF_NO_ROLE_ITEM_ROOT_HAS_ITEM,
            true,
            true,
        },
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr u32 target = 0x12345678U;

    for (const auto variant : variants) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            const u16 item_id =
                variant.opcode == OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM
                ? 0xC123U
                : 0x0123U;
            if (!variant.inverted) {
                if (variant.role_root_only) {
                    fixture.item_lists.role_item_lists[2]->sentinel.item_id =
                        item_id;
                } else {
                    fixture.player_inventory.emplace_back();
                    fixture.player_inventory.front().item_id =
                        static_cast<u16>(item_id | 0xC000U);
                }
            } else if (variant.role_root_only) {
                fixture.player_inventory.emplace_back();
                fixture.player_inventory.front().item_id =
                    static_cast<u16>(item_id | 0xC000U);
            }

            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias_mask)
            );
            write_u16(fixture.state.window, 2U, item_id);
            write_u32(fixture.state.window, 4U, target);
            write_u16(
                fixture.ports.transferred_window,
                0U,
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
            );
            write_u16(fixture.ports.transferred_window, 2U, 16383U);

            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::terminated &&
                    result.executed_instruction_count == 3U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.ports.data_load_count == 1U &&
                    fixture.ports.last_data_file_number == 1U &&
                    fixture.ports.last_data_offset == target &&
                    !fixture.ports.last_data_clear_before_read &&
                    fixture.ports.story_protocol_events ==
                        std::vector<u32>{2U, 5U, 4U} &&
                    fixture.state.previous_opcode ==
                        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL,
                "shared item-presence reload aliases take each positive or inverted predicate and same-call the target window"
            );
        }
    }

    for (const auto variant : variants) {
        Fixture fixture;
        constexpr u16 item_id = 0x0123U;
        if (variant.inverted) {
            if (variant.role_root_only) {
                fixture.item_lists.role_item_lists[0]->sentinel.item_id =
                    item_id;
            } else {
                fixture.player_inventory.emplace_back();
                fixture.player_inventory.front().item_id = 0xC123U;
            }
        } else if (variant.role_root_only) {
            fixture.player_inventory.emplace_back();
            fixture.player_inventory.front().item_id = 0xC123U;
            fixture.item_lists.role_item_lists[0]->sentinel.item_id = 0xC123U;
            fixture.item_lists.role_item_lists[0]->nodes.emplace_back();
            fixture.item_lists.role_item_lists[0]->nodes.front().item_id =
                item_id;
        }

        prime_loaded_instruction(fixture, variant.opcode);
        write_u16(fixture.state.window, 2U, item_id);
        write_u32(fixture.state.window, 4U, target);
        write_u16(fixture.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
        write_u16(fixture.state.window, 10U, 0x0073U);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 12U &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{0x0073U},
            "shared item-presence reload variants invert independently and role-only variants ignore player matches and linked role nodes"
        );
    }

    Fixture missing_player_owner;
    missing_player_owner.runtime.player_inventory = nullptr;
    missing_player_owner.item_lists.role_item_lists[0]->sentinel.item_id =
        0x0222U;
    prime_loaded_instruction(
        missing_player_owner, OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM
    );
    write_u16(missing_player_owner.state.window, 2U, 0x0222U);
    write_u32(missing_player_owner.state.window, 4U, target);
    const auto missing_player_owner_result = missing_player_owner.step();

    Fixture missing_role_owner;
    missing_role_owner.player_inventory.emplace_back();
    missing_role_owner.player_inventory.front().item_id = 0x0222U;
    missing_role_owner.runtime.role_item_lists = nullptr;
    prime_loaded_instruction(
        missing_role_owner, OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM
    );
    write_u16(missing_role_owner.state.window, 2U, 0x0222U);
    write_u32(missing_role_owner.state.window, 4U, target);
    const auto missing_role_owner_result = missing_role_owner.step();

    Fixture missing_role_root;
    missing_role_root.player_inventory.emplace_back();
    missing_role_root.player_inventory.front().item_id = 0x0222U;
    missing_role_root.item_lists.role_item_lists[0].reset();
    prime_loaded_instruction(
        missing_role_root, OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM
    );
    write_u16(missing_role_root.state.window, 2U, 0x0222U);
    write_u32(missing_role_root.state.window, 4U, target);
    const auto missing_role_root_result = missing_role_root.step();

    Fixture early_role_match;
    early_role_match.item_lists.role_item_lists[0]->sentinel.item_id = 0x0222U;
    early_role_match.item_lists.role_item_lists[1].reset();
    prime_loaded_instruction(
        early_role_match, OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM
    );
    write_u16(early_role_match.state.window, 2U, 0x0222U);
    write_u32(early_role_match.state.window, 4U, target);
    write_u16(
        early_role_match.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(early_role_match.ports.transferred_window, 2U, 16383U);
    const auto early_role_match_result = early_role_match.step();

    test.expect_true(
        missing_player_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_player_owner.ports.data_load_count == 0U &&
            missing_role_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_owner.ports.data_load_count == 0U &&
            missing_role_root_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_root.ports.data_load_count == 0U &&
            early_role_match_result.status ==
                LegacyWorldStoryVmStatus::terminated &&
            early_role_match.ports.data_load_count == 1U,
        "item-presence reload preserves player-first owner access and the 64-root helper's first-match return"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFCU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.player_inventory.emplace_back();
    target_truncated.player_inventory.front().item_id = 0xC333U;
    write_u16(
        target_truncated.state.window,
        0x7FFCU,
        OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM
    );
    write_u16(target_truncated.state.window, 0x7FFEU, 0x0333U);
    const auto target_truncated_result = target_truncated.step();

    Fixture no_target_needed;
    no_target_needed.context.instruction_offset = 0x7FFCU;
    no_target_needed.context.talk_data_offset = 0x1111U;
    no_target_needed.state.loaded_file_number = 1U;
    no_target_needed.state.loaded_data_offset = 0x1111U;
    no_target_needed.state.window_loaded = true;
    write_u16(
        no_target_needed.state.window,
        0x7FFCU,
        OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM
    );
    write_u16(no_target_needed.state.window, 0x7FFEU, 0x0333U);
    const auto no_target_needed_result = no_target_needed.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF8U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    write_u16(
        exact_tail.state.window,
        0x7FF8U,
        OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM
    );
    write_u16(exact_tail.state.window, 0x7FFAU, 0x0333U);
    write_u32(exact_tail.state.window, 0x7FFCU, target);
    write_u16(
        exact_tail.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(exact_tail.ports.transferred_window, 2U, 16383U);
    const auto exact_tail_result = exact_tail.step();

    Fixture load_failure;
    prime_loaded_instruction(
        load_failure, OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM
    );
    write_u16(load_failure.state.window, 2U, 0x0333U);
    write_u32(load_failure.state.window, 4U, target);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();

    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFCU &&
            target_truncated.state.previous_opcode == 0U &&
            target_truncated.ports.data_load_count == 0U &&
            no_target_needed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_needed_result.executed_instruction_count == 1U &&
            no_target_needed.context.instruction_offset == 0x8004U &&
            no_target_needed.state.previous_opcode ==
                OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM &&
            no_target_needed.ports.data_load_count == 0U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::terminated &&
            exact_tail_result.executed_instruction_count == 3U &&
            exact_tail.ports.last_data_offset == target &&
            load_failure_result.status ==
                LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "item-presence reload reads its u32 target only when taken and preserves common reload failure publication"
    );
}

void test_add_party_item_if_allowed_protocol(openswd3::test::Context& test) {
    const auto set_restriction = [](LegacyWorldItemNode& item,
                                    const u16 restriction) noexcept {
        item.definition_snapshot[0x3AU] = static_cast<u8>(restriction);
        item.definition_snapshot[0x3BU] = static_cast<u8>(restriction >> 8U);
    };

    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        for (u16 party_index = 0U; party_index < 4U; ++party_index) {
            Fixture fixture;
            const u16 restriction = static_cast<u16>(0x8000U >> party_index);
            fixture.ports.prepared_item_definition.fill(0x22U);
            fixture.ports.prepared_item_definition[0x21U] = 0xFFU;
            fixture.ports.prepared_item_definition[0x3AU] =
                static_cast<u8>(restriction);
            fixture.ports.prepared_item_definition[0x3BU] =
                static_cast<u8>(restriction >> 8U);
            fixture.ports.prepared_item_description = {4U, 5U, 6U};
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_131_ADD_PARTY_ITEM_IF_ALLOWED | alias_mask)
            );
            write_u16(fixture.state.window, 2U, party_index);
            write_u16(fixture.state.window, 4U, 0x0234U);
            write_u16(fixture.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
            write_u16(fixture.state.window, 8U, 0x0073U);

            const auto result = fixture.step();
            const auto& nodes =
                fixture.item_lists.party_item_lists[party_index]->nodes;
            const auto& item = nodes.front();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                    result.executed_instruction_count == 2U &&
                    fixture.context.instruction_offset == 10U &&
                    fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                    nodes.size() == 1U && item.item_id == 0x0234U &&
                    item.quantity_a == 1U && item.quantity_b == 0U &&
                    item.definition_snapshot[0x20U] == 0x22U &&
                    item.definition_snapshot[0x21U] == 0x7FU &&
                    item.description == std::vector<u8>{4U, 5U, 6U} &&
                    fixture.ports.item_definition_load_count == 1U &&
                    fixture.ports.last_item_definition_id == 0x0234U &&
                    fixture.ports.story_protocol_events ==
                        std::vector<u32>{13U, 2U} &&
                    fixture.ports.sound_effect_requests ==
                        std::vector<u16>{0x0073U},
                "opcode 131 aliases retain a newly loaded item only when the selected party restriction bit is set"
            );
        }
    }

    Fixture masked_existing;
    auto& masked_item =
        masked_existing.item_lists.party_item_lists[0]->nodes.emplace_back();
    masked_item.item_id = 0xC123U;
    masked_item.quantity_a = 5U;
    masked_item.definition_snapshot[0x21U] = 0x80U;
    prime_loaded_instruction(masked_existing, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(masked_existing.state.window, 2U, 0U);
    write_u16(masked_existing.state.window, 4U, 0x0123U);
    write_u16(masked_existing.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(masked_existing.state.window, 8U, 0x0073U);
    const auto masked_existing_result = masked_existing.step();

    test.expect_true(
        masked_existing_result.status == LegacyWorldStoryVmStatus::yielded &&
            masked_existing.item_lists.party_item_lists[0]->nodes.size() ==
                1U &&
            masked_item.quantity_a == 5U &&
            masked_item.definition_snapshot[0x21U] == 0x80U &&
            masked_existing.ports.item_definition_load_count == 0U,
        "opcode 131 masked prelookup skips upsert, flag clearing, and restriction checks for an existing item"
    );

    for (const u16 invalid_party_index : {u16{4U}, u16{0xFFFFU}}) {
        Fixture fixture;
        fixture.runtime.party_item_lists = nullptr;
        prime_loaded_instruction(fixture, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
        write_u16(fixture.state.window, 2U, invalid_party_index);
        write_u16(fixture.state.window, 4U, 0x0234U);
        write_u16(fixture.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
        write_u16(fixture.state.window, 8U, 0x0073U);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.ports.item_definition_load_count == 0U,
            "opcode 131 invalid party indices consume after both operands without touching the party owner"
        );
    }

    Fixture unqualified;
    unqualified.ports.prepared_item_definition.fill(0x22U);
    unqualified.ports.prepared_item_definition[0x21U] = 0x80U;
    unqualified.ports.prepared_item_definition[0x3AU] = 0U;
    unqualified.ports.prepared_item_definition[0x3BU] = 0U;
    prime_loaded_instruction(unqualified, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(unqualified.state.window, 2U, 2U);
    write_u16(unqualified.state.window, 4U, 0x0234U);
    write_u16(unqualified.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(unqualified.state.window, 8U, 0x0073U);
    const auto unqualified_result = unqualified.step();

    Fixture sentinel;
    prime_loaded_instruction(sentinel, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(sentinel.state.window, 2U, 0U);
    write_u16(
        sentinel.state.window, 4U, openswd3::world_map::kLegacyItemSentinelId
    );
    write_u16(sentinel.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(sentinel.state.window, 8U, 0x0073U);
    const auto sentinel_result = sentinel.step();

    test.expect_true(
        unqualified_result.status == LegacyWorldStoryVmStatus::yielded &&
            unqualified.item_lists.party_item_lists[2]->nodes.empty() &&
            unqualified.ports.item_definition_load_count == 1U &&
            sentinel_result.status == LegacyWorldStoryVmStatus::yielded &&
            sentinel.item_lists.party_item_lists[0]->nodes.empty() &&
            sentinel.ports.item_definition_load_count == 0U,
        "opcode 131 removes a just-added item whose party restriction bit is clear and preserves the FFDC no-loader path"
    );

    Fixture high_exact_allowed;
    auto& high_allowed =
        high_exact_allowed.item_lists.party_item_lists[1]->nodes.emplace_back();
    high_allowed.item_id = 0xC123U;
    high_allowed.quantity_a = 5U;
    high_allowed.quantity_b = 7U;
    high_allowed.definition_snapshot[0x21U] = 0x80U;
    set_restriction(high_allowed, 0x4000U);
    prime_loaded_instruction(
        high_exact_allowed, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(high_exact_allowed.state.window, 2U, 1U);
    write_u16(high_exact_allowed.state.window, 4U, 0xC123U);
    write_u16(high_exact_allowed.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(high_exact_allowed.state.window, 8U, 0x0073U);
    const auto high_exact_allowed_result = high_exact_allowed.step();

    Fixture high_exact_unqualified;
    auto& high_unqualified =
        high_exact_unqualified.item_lists.party_item_lists[1]
            ->nodes.emplace_back();
    high_unqualified.item_id = 0xC124U;
    high_unqualified.quantity_a = 5U;
    high_unqualified.definition_snapshot[0x21U] = 0x80U;
    prime_loaded_instruction(
        high_exact_unqualified, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(high_exact_unqualified.state.window, 2U, 1U);
    write_u16(high_exact_unqualified.state.window, 4U, 0xC124U);
    write_u16(high_exact_unqualified.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(high_exact_unqualified.state.window, 8U, 0x0073U);
    const auto high_exact_unqualified_result = high_exact_unqualified.step();

    test.expect_true(
        high_exact_allowed_result.status == LegacyWorldStoryVmStatus::yielded &&
            high_allowed.quantity_a == 6U && high_allowed.quantity_b == 7U &&
            high_allowed.definition_snapshot[0x21U] == 0U &&
            high_exact_allowed.ports.item_definition_load_count == 0U &&
            high_exact_unqualified_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            high_unqualified.quantity_a == 5U &&
            high_unqualified.definition_snapshot[0x21U] == 0U &&
            high_exact_unqualified.ports.item_definition_load_count == 0U,
        "opcode 131 preserves exact high-id mode1 upsert and the unqualified add-then-decrement sequence"
    );

    Fixture flagged_duplicate;
    auto& returned =
        flagged_duplicate.item_lists.party_item_lists[0]->nodes.emplace_back();
    returned.item_id = 0xC130U;
    returned.quantity_a = 5U;
    returned.definition_snapshot[0x21U] = 0x80U;
    auto& later_flagged =
        flagged_duplicate.item_lists.party_item_lists[0]->nodes.emplace_back();
    later_flagged.item_id = 0xC130U;
    later_flagged.quantity_a = 0U;
    later_flagged.definition_snapshot[0x21U] = 0x80U;
    prime_loaded_instruction(
        flagged_duplicate, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(flagged_duplicate.state.window, 2U, 0U);
    write_u16(flagged_duplicate.state.window, 4U, 0xC130U);
    write_u16(flagged_duplicate.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(flagged_duplicate.state.window, 8U, 0x0073U);
    const auto flagged_duplicate_result = flagged_duplicate.step();

    test.expect_true(
        flagged_duplicate_result.status == LegacyWorldStoryVmStatus::yielded &&
            flagged_duplicate.item_lists.party_item_lists[0]->nodes.size() ==
                1U &&
            flagged_duplicate.item_lists.party_item_lists[0]
                    ->nodes.front()
                    .quantity_a == 5U &&
            flagged_duplicate.item_lists.party_item_lists[0]
                    ->nodes.front()
                    .definition_snapshot[0x21U] == 0U,
        "opcode 131 preserves sub_44D0F0's negative flagged deletion fall-through into the unflagged scan"
    );

    Fixture wrapped_keep;
    auto& wrapping =
        wrapped_keep.item_lists.party_item_lists[0]->nodes.emplace_back();
    wrapping.item_id = 0xC125U;
    wrapping.quantity_a = 0x7FFFU;
    wrapping.quantity_b = 1U;
    wrapping.definition_snapshot[0x21U] = 0x80U;
    set_restriction(wrapping, 0x8000U);
    prime_loaded_instruction(wrapped_keep, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(wrapped_keep.state.window, 2U, 0U);
    write_u16(wrapped_keep.state.window, 4U, 0xC125U);
    write_u16(wrapped_keep.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(wrapped_keep.state.window, 8U, 0x0073U);
    const auto wrapped_keep_result = wrapped_keep.step();

    Fixture wrapped_delete;
    auto& deleted =
        wrapped_delete.item_lists.party_item_lists[0]->nodes.emplace_back();
    deleted.item_id = 0xC126U;
    deleted.quantity_a = 0xFFFFU;
    prime_loaded_instruction(wrapped_delete, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(wrapped_delete.state.window, 2U, 0U);
    write_u16(wrapped_delete.state.window, 4U, 0xC126U);
    const auto wrapped_delete_result = wrapped_delete.step();

    Fixture clamp;
    auto& clamped = clamp.item_lists.party_item_lists[0]->nodes.emplace_back();
    clamped.item_id = 0xC127U;
    clamped.quantity_a = 99U;
    clamped.quantity_b = 8U;
    set_restriction(clamped, 0x8000U);
    prime_loaded_instruction(clamp, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(clamp.state.window, 2U, 0U);
    write_u16(clamp.state.window, 4U, 0xC127U);
    write_u16(clamp.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(clamp.state.window, 8U, 0x0073U);
    const auto clamp_result = clamp.step();

    test.expect_true(
        wrapped_keep_result.status == LegacyWorldStoryVmStatus::yielded &&
            wrapping.quantity_a == 0U && wrapping.quantity_b == 1U &&
            wrapping.definition_snapshot[0x21U] == 0U &&
            wrapped_delete_result.status ==
                LegacyWorldStoryVmStatus::item_update_failed &&
            wrapped_delete.item_lists.party_item_lists[0]->nodes.empty() &&
            wrapped_delete.context.instruction_offset == 0U &&
            wrapped_delete.state.previous_opcode == 0U &&
            clamp_result.status == LegacyWorldStoryVmStatus::yielded &&
            clamped.quantity_a == 99U && clamped.quantity_b == 0U,
        "opcode 131 preserves mode1 i16 wrapping, null-return unsafe point, and the signed 99 clamp"
    );

    Fixture definition_failure;
    definition_failure.ports.item_definition_load_success = false;
    prime_loaded_instruction(
        definition_failure, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(definition_failure.state.window, 2U, 0U);
    write_u16(definition_failure.state.window, 4U, 0x0234U);
    const auto definition_failure_result = definition_failure.step();

    Fixture missing_owner;
    missing_owner.runtime.party_item_lists = nullptr;
    prime_loaded_instruction(missing_owner, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(missing_owner.state.window, 2U, 0U);
    write_u16(missing_owner.state.window, 4U, 0x0234U);
    const auto missing_owner_result = missing_owner.step();

    Fixture missing_root;
    missing_root.item_lists.party_item_lists[2].reset();
    prime_loaded_instruction(missing_root, OP_131_ADD_PARTY_ITEM_IF_ALLOWED);
    write_u16(missing_root.state.window, 2U, 2U);
    write_u16(missing_root.state.window, 4U, 0x0234U);
    const auto missing_root_result = missing_root.step();

    test.expect_true(
        definition_failure_result.status ==
                LegacyWorldStoryVmStatus::item_update_failed &&
            definition_failure.item_lists.party_item_lists[0]->nodes.empty() &&
            definition_failure.context.instruction_offset == 0U &&
            definition_failure.state.previous_opcode == 0U &&
            missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_root_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable,
        "opcode 131 stops at the original add-result or selected-root unsafe point without publishing completion"
    );

    Fixture group_truncated;
    group_truncated.context.instruction_offset = 0x7FFEU;
    group_truncated.context.talk_data_offset = 0x1111U;
    group_truncated.state.loaded_file_number = 1U;
    group_truncated.state.loaded_data_offset = 0x1111U;
    group_truncated.state.window_loaded = true;
    write_u16(
        group_truncated.state.window, 0x7FFEU, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    const auto group_truncated_result = group_truncated.step();

    Fixture item_truncated;
    item_truncated.context.instruction_offset = 0x7FFCU;
    item_truncated.context.talk_data_offset = 0x1111U;
    item_truncated.state.loaded_file_number = 1U;
    item_truncated.state.loaded_data_offset = 0x1111U;
    item_truncated.state.window_loaded = true;
    item_truncated.runtime.party_item_lists = nullptr;
    write_u16(
        item_truncated.state.window, 0x7FFCU, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(item_truncated.state.window, 0x7FFEU, 4U);
    const auto item_truncated_result = item_truncated.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.runtime.party_item_lists = nullptr;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_131_ADD_PARTY_ITEM_IF_ALLOWED
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 4U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x0234U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        group_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            group_truncated.context.instruction_offset == 0x7FFEU &&
            item_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            item_truncated.context.instruction_offset == 0x7FFCU &&
            item_truncated.ports.item_definition_load_count == 0U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_131_ADD_PARTY_ITEM_IF_ALLOWED,
        "opcode 131 reads both operands before party validation and completes an invalid exact-tail record before the next fetch"
    );
}

void test_swap_player_item_into_role_slot_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        for (u16 role_group = 0U; role_group < 4U; ++role_group) {
            Fixture fixture;
            auto& source = fixture.player_inventory.emplace_back();
            source.item_id = 0xC123U;
            source.selected_count = 9U;
            source.quantity_b = 1U;
            source.definition_snapshot.fill(
                static_cast<u8>(0x20U + role_group)
            );
            source.description = {
                1U,
                2U,
                static_cast<u8>(role_group),
            };
            auto& tail = fixture.player_inventory.emplace_back();
            tail.item_id = 0x0500U;
            tail.quantity_b = 4U;
            const std::size_t root_index =
                static_cast<std::size_t>(role_group) * 16U + 11U;
            auto& selected =
                fixture.item_lists.role_item_lists[root_index]->sentinel;
            selected.item_id = static_cast<u16>(0x0220U + role_group);
            selected.selected_count = 7U;
            selected.quantity_a = 3U;
            selected.quantity_b = 4U;
            selected.definition_snapshot.fill(0x44U);
            selected.description = {9U, static_cast<u8>(role_group)};
            fixture.item_lists.role_item_lists[root_index]
                ->nodes.emplace_back()
                .item_id = 0x0777U;
            fixture.ports.prepared_item_definition.fill(0x55U);
            fixture.ports.prepared_item_description = {7U, 8U};
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(
                    OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT | alias_mask
                )
            );
            write_u16(fixture.state.window, 2U, role_group);
            write_u16(fixture.state.window, 4U, 11U);
            write_u16(fixture.state.window, 6U, 0x0123U);
            write_u16(fixture.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
            write_u16(fixture.state.window, 10U, 0x0073U);

            const auto result = fixture.step();
            const auto& inventory = fixture.player_inventory;
            const auto& equipped =
                fixture.item_lists.role_item_lists[root_index]->sentinel;
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                    result.executed_instruction_count == 2U &&
                    fixture.context.instruction_offset == 12U &&
                    fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                    equipped.item_id == 0xC123U &&
                    equipped.selected_count == 0U &&
                    equipped.quantity_a == 1U && equipped.quantity_b == 0U &&
                    equipped.definition_snapshot[0U] ==
                        static_cast<u8>(0x20U + role_group) &&
                    equipped.description ==
                        std::vector<u8>{
                            1U,
                            2U,
                            static_cast<u8>(role_group),
                        } &&
                    fixture.item_lists.role_item_lists[root_index]
                        ->nodes.empty() &&
                    inventory.size() == 2U &&
                    inventory.front().item_id ==
                        static_cast<u16>(0x0220U + role_group) &&
                    inventory.front().selected_count == 0U &&
                    inventory.front().quantity_a == 0U &&
                    inventory.front().quantity_b == 1U &&
                    inventory.front().definition_snapshot[0U] == 0x55U &&
                    inventory.front().definition_snapshot[0x21U] == 0xD5U &&
                    inventory.front().description == std::vector<u8>{7U, 8U} &&
                    inventory.back().item_id == 0x0500U &&
                    inventory.back().quantity_b == 4U &&
                    fixture.ports.item_definition_load_count == 1U &&
                    fixture.ports.last_item_definition_id ==
                        static_cast<u16>(0x0220U + role_group) &&
                    fixture.ports.sound_effect_requests ==
                        std::vector<u16>{0x0073U},
                "opcode 132 aliases swap a masked player item into slot 11 of each role group and return the old root item"
            );
        }
    }

    Fixture existing_displaced;
    auto& existing_source = existing_displaced.player_inventory.emplace_back();
    existing_source.item_id = 0xC123U;
    existing_source.quantity_b = 2U;
    existing_source.description = {1U};
    auto& existing_old = existing_displaced.player_inventory.emplace_back();
    existing_old.item_id = 0x0222U;
    existing_old.quantity_b = 98U;
    auto& existing_root =
        existing_displaced.item_lists.role_item_lists[0]->sentinel;
    existing_root.item_id = 0x0222U;
    existing_root.description = {9U};
    prime_loaded_instruction(
        existing_displaced, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(existing_displaced.state.window, 2U, 0U);
    write_u16(existing_displaced.state.window, 4U, 0U);
    write_u16(existing_displaced.state.window, 6U, 0x0123U);
    write_u16(existing_displaced.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(existing_displaced.state.window, 10U, 0x0073U);
    const auto existing_displaced_result = existing_displaced.step();

    test.expect_true(
        existing_displaced_result.status == LegacyWorldStoryVmStatus::yielded &&
            existing_root.item_id == 0xC123U &&
            existing_displaced.player_inventory.size() == 2U &&
            existing_old.quantity_b == 99U &&
            existing_source.quantity_b == 1U &&
            existing_displaced.ports.item_definition_load_count == 0U,
        "opcode 132 runs the displaced-item +1 update before the source-item -1 update on existing inventory nodes"
    );

    Fixture loader_failure;
    auto& loader_source = loader_failure.player_inventory.emplace_back();
    loader_source.item_id = 0xC123U;
    loader_source.quantity_b = 1U;
    loader_source.description = {1U, 2U};
    auto& loader_root = loader_failure.item_lists.role_item_lists[0]->sentinel;
    loader_root.item_id = 0x0234U;
    loader_root.description = {9U};
    loader_failure.ports.item_definition_load_success = false;
    prime_loaded_instruction(
        loader_failure, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(loader_failure.state.window, 2U, 0U);
    write_u16(loader_failure.state.window, 4U, 0U);
    write_u16(loader_failure.state.window, 6U, 0x0123U);
    write_u16(loader_failure.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(loader_failure.state.window, 10U, 0x0073U);
    const auto loader_failure_result = loader_failure.step();

    test.expect_true(
        loader_failure_result.status == LegacyWorldStoryVmStatus::yielded &&
            loader_root.item_id == 0xC123U &&
            loader_root.description == std::vector<u8>{1U, 2U} &&
            loader_failure.player_inventory.empty() &&
            loader_failure.ports.item_definition_load_count == 1U &&
            loader_failure.ports.last_item_definition_id == 0x0234U,
        "opcode 132 ignores a displaced-item definition loader miss and still removes the source item"
    );

    Fixture source_deleted_during_add;
    auto& unstable_source =
        source_deleted_during_add.player_inventory.emplace_back();
    unstable_source.item_id = 0xC123U;
    unstable_source.quantity_b = 0x7FFFU;
    unstable_source.description = {1U};
    auto& unstable_root =
        source_deleted_during_add.item_lists.role_item_lists[0]->sentinel;
    unstable_root.item_id = 0xC123U;
    unstable_root.description = {9U};
    prime_loaded_instruction(
        source_deleted_during_add, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(source_deleted_during_add.state.window, 2U, 0U);
    write_u16(source_deleted_during_add.state.window, 4U, 0U);
    write_u16(source_deleted_during_add.state.window, 6U, 0x0123U);
    const auto source_deleted_during_add_result =
        source_deleted_during_add.step();

    test.expect_true(
        source_deleted_during_add_result.status ==
                LegacyWorldStoryVmStatus::item_update_failed &&
            source_deleted_during_add.player_inventory.empty() &&
            unstable_root.item_id == 0xC123U &&
            unstable_root.quantity_a == 1U && unstable_root.quantity_b == 0U &&
            source_deleted_during_add.context.instruction_offset == 0U &&
            source_deleted_during_add.state.previous_opcode == 0U,
        "opcode 132 stops at the original freed-source reread after the displaced-item +1 update deletes that node"
    );

    for (const std::array<u16, 2U> indices : {
             std::array<u16, 2U>{4U, 0U},
             std::array<u16, 2U>{0U, 12U},
         }) {
        Fixture fixture;
        fixture.runtime.player_inventory = nullptr;
        fixture.runtime.role_item_lists = nullptr;
        prime_loaded_instruction(
            fixture, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
        );
        write_u16(fixture.state.window, 2U, indices[0U]);
        write_u16(fixture.state.window, 4U, indices[1U]);
        write_u16(fixture.state.window, 6U, 0x0123U);
        write_u16(fixture.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
        write_u16(fixture.state.window, 10U, 0x0073U);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 12U,
            "opcode 132 invalid role-group or role-slot indices consume without reading the item owner"
        );
    }

    Fixture masked_miss;
    masked_miss.runtime.role_item_lists = nullptr;
    prime_loaded_instruction(
        masked_miss, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(masked_miss.state.window, 2U, 0U);
    write_u16(masked_miss.state.window, 4U, 0U);
    write_u16(masked_miss.state.window, 6U, 0x0123U);
    write_u16(masked_miss.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(masked_miss.state.window, 10U, 0x0073U);
    const auto masked_miss_result = masked_miss.step();

    Fixture missing_player_owner;
    missing_player_owner.runtime.player_inventory = nullptr;
    prime_loaded_instruction(
        missing_player_owner, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(missing_player_owner.state.window, 2U, 0U);
    write_u16(missing_player_owner.state.window, 4U, 0U);
    write_u16(missing_player_owner.state.window, 6U, 0x0123U);
    const auto missing_player_owner_result = missing_player_owner.step();

    Fixture missing_role_owner;
    missing_role_owner.player_inventory.emplace_back().item_id = 0xC123U;
    missing_role_owner.runtime.role_item_lists = nullptr;
    prime_loaded_instruction(
        missing_role_owner, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(missing_role_owner.state.window, 2U, 0U);
    write_u16(missing_role_owner.state.window, 4U, 0U);
    write_u16(missing_role_owner.state.window, 6U, 0x0123U);
    const auto missing_role_owner_result = missing_role_owner.step();

    Fixture missing_role_root;
    missing_role_root.player_inventory.emplace_back().item_id = 0xC123U;
    missing_role_root.item_lists.role_item_lists[16U].reset();
    prime_loaded_instruction(
        missing_role_root, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(missing_role_root.state.window, 2U, 1U);
    write_u16(missing_role_root.state.window, 4U, 0U);
    write_u16(missing_role_root.state.window, 6U, 0x0123U);
    const auto missing_role_root_result = missing_role_root.step();

    test.expect_true(
        masked_miss_result.status == LegacyWorldStoryVmStatus::yielded &&
            masked_miss_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            masked_miss.context.instruction_offset == 12U &&
            missing_player_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_root_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable,
        "opcode 132 preserves player-query-before-role-root owner access and skips the role root on masked miss"
    );

    Fixture first_operand_truncated;
    first_operand_truncated.context.instruction_offset = 0x7FFEU;
    first_operand_truncated.context.talk_data_offset = 0x1111U;
    first_operand_truncated.state.loaded_file_number = 1U;
    first_operand_truncated.state.loaded_data_offset = 0x1111U;
    first_operand_truncated.state.window_loaded = true;
    write_u16(
        first_operand_truncated.state.window,
        0x7FFEU,
        OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    const auto first_operand_truncated_result = first_operand_truncated.step();

    Fixture second_operand_truncated;
    second_operand_truncated.context.instruction_offset = 0x7FFCU;
    second_operand_truncated.context.talk_data_offset = 0x1111U;
    second_operand_truncated.state.loaded_file_number = 1U;
    second_operand_truncated.state.loaded_data_offset = 0x1111U;
    second_operand_truncated.state.window_loaded = true;
    write_u16(
        second_operand_truncated.state.window,
        0x7FFCU,
        OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(second_operand_truncated.state.window, 0x7FFEU, 0U);
    const auto second_operand_truncated_result =
        second_operand_truncated.step();

    Fixture item_truncated;
    item_truncated.context.instruction_offset = 0x7FFAU;
    item_truncated.context.talk_data_offset = 0x1111U;
    item_truncated.state.loaded_file_number = 1U;
    item_truncated.state.loaded_data_offset = 0x1111U;
    item_truncated.state.window_loaded = true;
    write_u16(
        item_truncated.state.window,
        0x7FFAU,
        OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(item_truncated.state.window, 0x7FFCU, 0U);
    write_u16(item_truncated.state.window, 0x7FFEU, 0U);
    const auto item_truncated_result = item_truncated.step();

    Fixture invalid_partial_tail;
    invalid_partial_tail.context.instruction_offset = 0x7FFAU;
    invalid_partial_tail.context.talk_data_offset = 0x1111U;
    invalid_partial_tail.state.loaded_file_number = 1U;
    invalid_partial_tail.state.loaded_data_offset = 0x1111U;
    invalid_partial_tail.state.window_loaded = true;
    invalid_partial_tail.runtime.player_inventory = nullptr;
    invalid_partial_tail.runtime.role_item_lists = nullptr;
    write_u16(
        invalid_partial_tail.state.window,
        0x7FFAU,
        OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(invalid_partial_tail.state.window, 0x7FFCU, 4U);
    write_u16(invalid_partial_tail.state.window, 0x7FFEU, 0U);
    const auto invalid_partial_tail_result = invalid_partial_tail.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF8U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    auto& exact_source = exact_tail.player_inventory.emplace_back();
    exact_source.item_id = 0xC123U;
    exact_source.quantity_b = 1U;
    exact_source.description = {1U};
    exact_tail.item_lists.role_item_lists[0]->sentinel.item_id = 0xFFDCU;
    write_u16(
        exact_tail.state.window, 0x7FF8U, OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT
    );
    write_u16(exact_tail.state.window, 0x7FFAU, 0U);
    write_u16(exact_tail.state.window, 0x7FFCU, 0U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x0123U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        first_operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            second_operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            item_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            invalid_partial_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            invalid_partial_tail_result.executed_instruction_count == 1U &&
            invalid_partial_tail.context.instruction_offset == 0x8002U &&
            invalid_partial_tail.state.previous_opcode ==
                OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT &&
            exact_tail.item_lists.role_item_lists[0]->sentinel.item_id ==
                0xC123U,
        "opcode 132 stages three operands, leaves the item unread for invalid indices, and completes a successful exact-tail swap"
    );
}
