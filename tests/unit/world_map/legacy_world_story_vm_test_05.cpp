#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_request_shop_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.state.shop_item_ids = {1U, 2U, 3U};
        fixture.special_mode_state = 0x11111111U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_133_REQUEST_SHOP | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 501U);
        write_u16(fixture.state.window, 4U, 0xFFFFU);
        write_u16(fixture.state.window, 6U, 0U);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_133_REQUEST_SHOP &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode == OP_133_REQUEST_SHOP &&
                fixture.state.shop_item_ids ==
                    std::vector<u16>{501U, 0xFFFFU} &&
                fixture.special_mode_state == 0x80000002U,
            "opcode 133 aliases replace the process shop item-id buffer, request shop mode 2, publish previous, and yield"
        );
    }

    Fixture empty;
    empty.state.shop_item_ids = {1U};
    prime_loaded_instruction(empty, OP_133_REQUEST_SHOP);
    write_u16(empty.state.window, 2U, 0U);
    const auto empty_result = empty.step();

    Fixture maximum;
    prime_loaded_instruction(maximum, OP_133_REQUEST_SHOP);
    for (std::size_t index = 0U;
         index < openswd3::world_map::kLegacyWorldStoryShopItemCapacity;
         ++index) {
        write_u16(
            maximum.state.window,
            2U + index * sizeof(u16),
            static_cast<u16>(0x0200U + index)
        );
    }
    write_u16(
        maximum.state.window,
        2U +
            openswd3::world_map::kLegacyWorldStoryShopItemCapacity *
                sizeof(u16),
        0U
    );
    const auto maximum_result = maximum.step();

    Fixture overflow;
    overflow.state.shop_item_ids = {9U};
    overflow.special_mode_state = 0x22222222U;
    prime_loaded_instruction(overflow, OP_133_REQUEST_SHOP);
    for (std::size_t index = 0U;
         index <= openswd3::world_map::kLegacyWorldStoryShopItemCapacity;
         ++index) {
        write_u16(
            overflow.state.window,
            2U + index * sizeof(u16),
            static_cast<u16>(0x0300U + index)
        );
    }
    write_u16(
        overflow.state.window,
        2U +
            (openswd3::world_map::kLegacyWorldStoryShopItemCapacity + 1U) *
                sizeof(u16),
        0U
    );
    const auto overflow_result = overflow.step();

    test.expect_true(
        empty_result.status == LegacyWorldStoryVmStatus::yielded &&
            empty.context.instruction_offset == 4U &&
            empty.state.shop_item_ids.empty() &&
            empty.special_mode_state == 0x80000002U &&
            maximum_result.status == LegacyWorldStoryVmStatus::yielded &&
            maximum.context.instruction_offset == 258U &&
            maximum.state.shop_item_ids.size() ==
                openswd3::world_map::kLegacyWorldStoryShopItemCapacity &&
            maximum.state.shop_item_ids.front() == 0x0200U &&
            maximum.state.shop_item_ids.back() == 0x027EU &&
            maximum.special_mode_state == 0x80000002U &&
            overflow_result.status ==
                LegacyWorldStoryVmStatus::shop_item_list_out_of_range &&
            overflow.context.instruction_offset == 0U &&
            overflow.state.previous_opcode == 0U &&
            overflow.state.shop_item_ids.empty() &&
            overflow.special_mode_state == 0x22222222U,
        "opcode 133 accepts zero through 127 item ids and stops after the terminator scan when 128 ids exceed the fixed buffer"
    );

    Fixture missing_terminator;
    missing_terminator.context.instruction_offset = 0x7FFCU;
    missing_terminator.context.talk_data_offset = 0x1111U;
    missing_terminator.state.loaded_file_number = 1U;
    missing_terminator.state.loaded_data_offset = 0x1111U;
    missing_terminator.state.window_loaded = true;
    missing_terminator.state.shop_item_ids = {1U, 2U};
    missing_terminator.special_mode_state = 0x33333333U;
    write_u16(missing_terminator.state.window, 0x7FFCU, OP_133_REQUEST_SHOP);
    write_u16(missing_terminator.state.window, 0x7FFEU, 501U);
    const auto missing_terminator_result = missing_terminator.step();

    Fixture missing_mode_owner;
    missing_mode_owner.runtime.special_mode_state = nullptr;
    prime_loaded_instruction(missing_mode_owner, OP_133_REQUEST_SHOP);
    write_u16(missing_mode_owner.state.window, 2U, 501U);
    write_u16(missing_mode_owner.state.window, 4U, 502U);
    write_u16(missing_mode_owner.state.window, 6U, 0U);
    const auto missing_mode_owner_result = missing_mode_owner.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_133_REQUEST_SHOP);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x0401U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        missing_terminator_result.status ==
                LegacyWorldStoryVmStatus::shop_item_list_terminator_not_found &&
            missing_terminator.context.instruction_offset == 0x7FFCU &&
            missing_terminator.state.previous_opcode == 0U &&
            missing_terminator.state.shop_item_ids.empty() &&
            missing_terminator.special_mode_state == 0x33333333U &&
            missing_mode_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_mode_owner.context.instruction_offset == 0U &&
            missing_mode_owner.state.previous_opcode == 0U &&
            missing_mode_owner.state.shop_item_ids ==
                std::vector<u16>{501U, 502U} &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_133_REQUEST_SHOP &&
            exact_tail.state.shop_item_ids == std::vector<u16>{0x0401U} &&
            exact_tail.special_mode_state == 0x80000002U,
        "opcode 133 preserves replacement-before-scan failures, commits the list before the mode owner, and completes an exact-tail request"
    );
}

void test_adjust_party_member_resources_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        auto& resources = fixture.state.party_member_resources[1U];
        resources.current_first = 10U;
        resources.current_second = 1U;
        resources.current_third = 50U;
        resources.limit_first = 20U;
        resources.limit_second = 100U;
        resources.limit_third = 60U;
        resources.transient_value = 77U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_134_ADJUST_PARTY_MEMBER_RESOURCES | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 2U);
        write_u16(fixture.state.window, 4U, 5U);
        write_u16(fixture.state.window, 6U, 0xFFFFU);
        write_u16(fixture.state.window, 8U, 20U);
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
                fixture.state.previous_opcode ==
                    OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
                resources.current_first == 15U &&
                resources.current_second == 0U &&
                resources.current_third == 60U &&
                resources.limit_first == 20U &&
                resources.limit_second == 100U &&
                resources.limit_third == 60U &&
                resources.transient_value == 0U &&
                read_u16(fixture.state.window, 10U) == kStoryVmTypedStop,
            "opcode 134 aliases wrap three u16 additions, apply signed limits and lower bounds, clear the transient word, publish previous, and same-call"
        );
    }

    Fixture self_modify;
    auto& self_modify_resources = self_modify.state.party_member_resources[0U];
    self_modify_resources.current_first = 0x7FF8U;
    self_modify_resources.current_second = 0xFFFFU;
    self_modify_resources.current_third = 90U;
    self_modify_resources.limit_first = 100U;
    self_modify_resources.limit_second = 100U;
    self_modify_resources.limit_third = 100U;
    self_modify_resources.transient_value = 99U;
    prime_loaded_instruction(self_modify, OP_134_ADJUST_PARTY_MEMBER_RESOURCES);
    write_u16(self_modify.state.window, 2U, 1U);
    write_u16(self_modify.state.window, 4U, 16U);
    write_u16(self_modify.state.window, 6U, 0U);
    write_u16(self_modify.state.window, 8U, 20U);
    write_u16(self_modify.state.window, 10U, 192U);
    const auto self_modify_result = self_modify.step();

    test.expect_true(
        self_modify_result.status == LegacyWorldStoryVmStatus::yielded &&
            self_modify_result.opcode == OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            self_modify_result.executed_instruction_count == 2U &&
            self_modify_result.direct_audio_service_count == 1U &&
            self_modify.context.instruction_offset == 14U &&
            self_modify.state.previous_opcode ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            self_modify_resources.current_first == 0U &&
            self_modify_resources.current_second == 0U &&
            self_modify_resources.current_third == 100U &&
            self_modify_resources.transient_value == 0U &&
            self_modify.special_mode_state == 0x80000004U &&
            self_modify.ports.input_menu_reset_count == 1U &&
            read_u16(self_modify.state.window, 10U) ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5,
        "opcode 134 treats wrapped negative resources as signed, rewrites the next word to opcode 144, and same-calls its mode-four audio-yield path"
    );

    Fixture missing_first;
    missing_first.context.instruction_offset = 0x7FFCU;
    missing_first.context.talk_data_offset = 0x1111U;
    missing_first.state.loaded_file_number = 1U;
    missing_first.state.loaded_data_offset = 0x1111U;
    missing_first.state.window_loaded = true;
    missing_first.state.party_member_resources[0U].current_first = 7U;
    write_u16(
        missing_first.state.window,
        0x7FFCU,
        OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(missing_first.state.window, 0x7FFEU, 1U);
    const auto missing_first_result = missing_first.step();

    Fixture missing_second;
    missing_second.context.instruction_offset = 0x7FFAU;
    missing_second.context.talk_data_offset = 0x1111U;
    missing_second.state.loaded_file_number = 1U;
    missing_second.state.loaded_data_offset = 0x1111U;
    missing_second.state.window_loaded = true;
    missing_second.state.party_member_resources[0U].current_first = 0x7FFFU;
    write_u16(
        missing_second.state.window,
        0x7FFAU,
        OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(missing_second.state.window, 0x7FFCU, 1U);
    write_u16(missing_second.state.window, 0x7FFEU, 1U);
    const auto missing_second_result = missing_second.step();

    Fixture missing_third;
    missing_third.context.instruction_offset = 0x7FF8U;
    missing_third.context.talk_data_offset = 0x1111U;
    missing_third.state.loaded_file_number = 1U;
    missing_third.state.loaded_data_offset = 0x1111U;
    missing_third.state.window_loaded = true;
    missing_third.state.party_member_resources[0U].current_first = 5U;
    missing_third.state.party_member_resources[0U].current_second = 6U;
    write_u16(
        missing_third.state.window,
        0x7FF8U,
        OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(missing_third.state.window, 0x7FFAU, 1U);
    write_u16(missing_third.state.window, 0x7FFCU, 2U);
    write_u16(missing_third.state.window, 0x7FFEU, 3U);
    const auto missing_third_result = missing_third.step();

    test.expect_true(
        missing_first_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_first.context.instruction_offset == 0x7FFCU &&
            missing_first.state.previous_opcode == 0U &&
            missing_first.state.party_member_resources[0U].current_first ==
                7U &&
            missing_second_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_second.context.instruction_offset == 0x7FFAU &&
            missing_second.state.previous_opcode == 0U &&
            missing_second.state.party_member_resources[0U].current_first ==
                0x8000U &&
            missing_third_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_third.context.instruction_offset == 0x7FF8U &&
            missing_third.state.previous_opcode == 0U &&
            missing_third.state.party_member_resources[0U].current_first ==
                7U &&
            missing_third.state.party_member_resources[0U].current_second == 9U,
        "opcode 134 reads and commits each delta independently and does not clamp or publish after a later operand truncation"
    );

    Fixture invalid_selector;
    invalid_selector.context.instruction_offset = 0x7FFCU;
    invalid_selector.context.talk_data_offset = 0x1111U;
    invalid_selector.state.loaded_file_number = 1U;
    invalid_selector.state.loaded_data_offset = 0x1111U;
    invalid_selector.state.window_loaded = true;
    invalid_selector.state.party_member_resources[0U].current_first = 7U;
    write_u16(
        invalid_selector.state.window,
        0x7FFCU,
        OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(invalid_selector.state.window, 0x7FFEU, 5U);
    const auto invalid_selector_result = invalid_selector.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    auto& exact_tail_resources = exact_tail.state.party_member_resources[0U];
    exact_tail_resources.current_first = 1U;
    exact_tail_resources.limit_first = 10U;
    exact_tail_resources.limit_second = 10U;
    exact_tail_resources.limit_third = 10U;
    exact_tail_resources.transient_value = 99U;
    write_u16(
        exact_tail.state.window, 0x7FF6U, OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(exact_tail.state.window, 0x7FF8U, 1U);
    write_u16(exact_tail.state.window, 0x7FFAU, 0U);
    write_u16(exact_tail.state.window, 0x7FFCU, 0U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0U);
    const auto exact_tail_result = exact_tail.step();

    Fixture self_modify_out_of_range;
    self_modify_out_of_range.context.instruction_offset = 0x7FF6U;
    self_modify_out_of_range.context.talk_data_offset = 0x1111U;
    self_modify_out_of_range.state.loaded_file_number = 1U;
    self_modify_out_of_range.state.loaded_data_offset = 0x1111U;
    self_modify_out_of_range.state.window_loaded = true;
    auto& out_of_range_resources =
        self_modify_out_of_range.state.party_member_resources[0U];
    out_of_range_resources.current_second = 0xFFFFU;
    out_of_range_resources.current_third = 0xFFFFU;
    out_of_range_resources.limit_first = 10U;
    out_of_range_resources.limit_second = 10U;
    out_of_range_resources.limit_third = 10U;
    out_of_range_resources.transient_value = 99U;
    write_u16(
        self_modify_out_of_range.state.window,
        0x7FF6U,
        OP_134_ADJUST_PARTY_MEMBER_RESOURCES
    );
    write_u16(self_modify_out_of_range.state.window, 0x7FF8U, 1U);
    write_u16(self_modify_out_of_range.state.window, 0x7FFAU, 0U);
    write_u16(self_modify_out_of_range.state.window, 0x7FFCU, 0U);
    write_u16(self_modify_out_of_range.state.window, 0x7FFEU, 0U);
    const auto self_modify_out_of_range_result =
        self_modify_out_of_range.step();

    test.expect_true(
        invalid_selector_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            invalid_selector_result.executed_instruction_count == 1U &&
            invalid_selector.context.instruction_offset == 0x8006U &&
            invalid_selector.state.previous_opcode ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            invalid_selector.state.party_member_resources[0U].current_first ==
                7U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            exact_tail_resources.current_first == 1U &&
            exact_tail_resources.transient_value == 0U &&
            self_modify_out_of_range_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            self_modify_out_of_range.context.instruction_offset == 0x7FF6U &&
            self_modify_out_of_range.state.previous_opcode == 0U &&
            out_of_range_resources.current_first == 0U &&
            out_of_range_resources.current_second == 0xFFFFU &&
            out_of_range_resources.current_third == 0xFFFFU &&
            out_of_range_resources.transient_value == 99U,
        "opcode 134 invalid selectors consume without operands, exact-tail success continues after publication, and an unsafe next-opcode rewrite preserves prior clamps only"
    );
}

void test_reset_input_menu_state_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.special_input_mode = 0x11111111U;
        fixture.high_priority_submode = 0x22222222U;
        fixture.high_priority_auxiliary = 0x33333333U;
        fixture.high_priority_state = 0x44444444U;
        bool reset_saw_prior_writes = false;
        bool first_audio_saw_committed_ip = false;
        bool common_audio_saw_previous = false;
        fixture.ports.input_menu_reset_callback = [&]() {
            reset_saw_prior_writes = fixture.special_input_mode == 4U &&
                fixture.high_priority_submode == 1U &&
                fixture.high_priority_auxiliary == 0U &&
                fixture.high_priority_state == 3U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0U;
        };
        fixture.ports.audio_service_callback = [&]() {
            if (fixture.ports.direct_audio_service_count == 1U) {
                first_audio_saw_committed_ip =
                    fixture.ports.input_menu_reset_count == 1U &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.previous_opcode == 0U;
            } else if (fixture.ports.direct_audio_service_count == 2U) {
                common_audio_saw_previous =
                    fixture.ports.input_menu_reset_count == 1U &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.previous_opcode ==
                        OP_135_RESET_INPUT_MENU_STATE;
            }
        };
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_135_RESET_INPUT_MENU_STATE | alias_mask)
        );

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_135_RESET_INPUT_MENU_STATE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_135_RESET_INPUT_MENU_STATE &&
                fixture.special_input_mode == 4U &&
                fixture.high_priority_submode == 1U &&
                fixture.high_priority_auxiliary == 0U &&
                fixture.high_priority_state == 3U &&
                fixture.ports.input_menu_reset_count == 1U &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{14U, 2U, 2U} &&
                reset_saw_prior_writes && first_audio_saw_committed_ip &&
                common_audio_saw_previous,
            "opcode 135 aliases write four mode states, reset input/menu/save previews, commit the two-byte record, service handler and common audio, publish previous, and yield"
        );
    }

    Fixture missing_input_mode;
    missing_input_mode.special_input_mode = 0x11U;
    missing_input_mode.high_priority_submode = 0x22U;
    missing_input_mode.high_priority_auxiliary = 0x33U;
    missing_input_mode.high_priority_state = 0x44U;
    missing_input_mode.runtime.special_input_mode = nullptr;
    prime_loaded_instruction(missing_input_mode, OP_135_RESET_INPUT_MENU_STATE);
    const auto missing_input_mode_result = missing_input_mode.step();

    Fixture missing_submode;
    missing_submode.special_input_mode = 0x11U;
    missing_submode.high_priority_submode = 0x22U;
    missing_submode.high_priority_auxiliary = 0x33U;
    missing_submode.high_priority_state = 0x44U;
    missing_submode.runtime.high_priority_submode = nullptr;
    prime_loaded_instruction(missing_submode, OP_135_RESET_INPUT_MENU_STATE);
    const auto missing_submode_result = missing_submode.step();

    Fixture missing_auxiliary;
    missing_auxiliary.special_input_mode = 0x11U;
    missing_auxiliary.high_priority_submode = 0x22U;
    missing_auxiliary.high_priority_auxiliary = 0x33U;
    missing_auxiliary.high_priority_state = 0x44U;
    missing_auxiliary.runtime.high_priority_auxiliary = nullptr;
    prime_loaded_instruction(missing_auxiliary, OP_135_RESET_INPUT_MENU_STATE);
    const auto missing_auxiliary_result = missing_auxiliary.step();

    Fixture missing_high_priority;
    missing_high_priority.special_input_mode = 0x11U;
    missing_high_priority.high_priority_submode = 0x22U;
    missing_high_priority.high_priority_auxiliary = 0x33U;
    missing_high_priority.high_priority_state = 0x44U;
    missing_high_priority.runtime.high_priority_state = nullptr;
    prime_loaded_instruction(
        missing_high_priority, OP_135_RESET_INPUT_MENU_STATE
    );
    const auto missing_high_priority_result = missing_high_priority.step();

    test.expect_true(
        missing_input_mode_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_input_mode.special_input_mode == 0x11U &&
            missing_input_mode.high_priority_submode == 0x22U &&
            missing_input_mode.high_priority_auxiliary == 0x33U &&
            missing_input_mode.high_priority_state == 0x44U &&
            missing_input_mode.ports.input_menu_reset_count == 0U &&
            missing_submode_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_submode.special_input_mode == 4U &&
            missing_submode.high_priority_submode == 0x22U &&
            missing_submode.high_priority_auxiliary == 0x33U &&
            missing_submode.high_priority_state == 0x44U &&
            missing_submode.ports.input_menu_reset_count == 0U &&
            missing_auxiliary_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_auxiliary.special_input_mode == 4U &&
            missing_auxiliary.high_priority_submode == 1U &&
            missing_auxiliary.high_priority_auxiliary == 0x33U &&
            missing_auxiliary.high_priority_state == 0x44U &&
            missing_auxiliary.ports.input_menu_reset_count == 0U &&
            missing_high_priority_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_high_priority.special_input_mode == 4U &&
            missing_high_priority.high_priority_submode == 1U &&
            missing_high_priority.high_priority_auxiliary == 0U &&
            missing_high_priority.high_priority_state == 0x44U &&
            missing_high_priority.ports.input_menu_reset_count == 0U,
        "opcode 135 borrows and writes each mode owner at its original global access point without rolling back earlier writes"
    );

    Fixture deferred_reset;
    deferred_reset.special_input_mode = 0x11U;
    deferred_reset.high_priority_submode = 0x22U;
    deferred_reset.high_priority_auxiliary = 0x33U;
    deferred_reset.high_priority_state = 0x44U;
    deferred_reset.ports.input_menu_reset_success = false;
    prime_loaded_instruction(deferred_reset, OP_135_RESET_INPUT_MENU_STATE);
    const auto deferred_reset_result = deferred_reset.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_135_RESET_INPUT_MENU_STATE);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        deferred_reset_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            deferred_reset.context.instruction_offset == 0U &&
            deferred_reset.state.previous_opcode == 0U &&
            deferred_reset.special_input_mode == 4U &&
            deferred_reset.high_priority_submode == 1U &&
            deferred_reset.high_priority_auxiliary == 0U &&
            deferred_reset.high_priority_state == 3U &&
            deferred_reset.ports.input_menu_reset_count == 1U &&
            deferred_reset.ports.direct_audio_service_count == 0U &&
            deferred_reset.ports.story_protocol_events ==
                std::vector<u32>{14U} &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 2U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_135_RESET_INPUT_MENU_STATE &&
            exact_tail.special_input_mode == 4U &&
            exact_tail.high_priority_submode == 1U &&
            exact_tail.high_priority_auxiliary == 0U &&
            exact_tail.high_priority_state == 3U &&
            exact_tail.ports.input_menu_reset_count == 1U,
        "opcode 135 preserves all four writes when the deferred reset owner fails and completes an exact-tail record without reading operands"
    );
}

void test_stop_scene_music_stream_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        u32 initial_control_flags;
        u32 initial_transition_mode;
        u32 initial_fade_divisor;
        u32 initial_pending_fade_divisor;
        u32 expected_transition_mode;
        u32 expected_fade_divisor;
        u32 expected_pending_fade_divisor;
        u32 expected_transition_calls;
    };
    constexpr std::array cases{
        TestCase{
            0U,
            0xA58312EFU,
            1U,
            0x11111111U,
            3U,
            0U,
            0U,
            3U,
            1U,
        },
        TestCase{
            0x4000U,
            0x5A80AA55U,
            2U,
            0x22222222U,
            5U,
            2U,
            5U,
            5U,
            1U,
        },
        TestCase{
            0x8000U,
            0x008000FFU,
            0xFFFFFFFFU,
            0x33333333U,
            7U,
            0xFFFFFFFFU,
            0x33333333U,
            7U,
            1U,
        },
        TestCase{
            0xC000U,
            0xFF7FFFFFU,
            2U,
            0x44444444U,
            9U,
            0U,
            0x44444444U,
            0U,
            0U,
        },
    };

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_137_STOP_SCENE_MUSIC_STREAM | test_case.alias_mask
            )
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.world_music_request = 0x11111111U;
        fixture.state.world_music_first_stream = 0x22222222U;
        fixture.state.world_music_second_stream = 0x33333333U;
        fixture.state.music_request = 0x44444444U;
        fixture.state.music_first_stream = 0x55555555U;
        fixture.state.music_second_stream = 0x66666666U;
        fixture.state.music_control_flags = test_case.initial_control_flags;
        fixture.state.current_first_stream = test_case.initial_transition_mode;
        fixture.state.current_stream_fade_divisor =
            test_case.initial_fade_divisor;
        fixture.state.current_second_stream =
            test_case.initial_pending_fade_divisor;
        fixture.state.previous_opcode = 0x66U;
        bool transition_saw_prior_slots = false;
        fixture.ports.music_transition_callback = [&]() {
            transition_saw_prior_slots =
                fixture.state.world_music_request == 0x11111111U &&
                fixture.state.music_request == 0x44444444U &&
                fixture.state.music_first_stream == 0x55555555U &&
                fixture.state.music_second_stream == 0x66666666U &&
                fixture.state.music_control_flags ==
                    test_case.initial_control_flags &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0x66U;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.state.world_music_request == 0x80000001U &&
                fixture.state.world_music_first_stream == 0x22222222U &&
                fixture.state.world_music_second_stream == 0x33333333U &&
                fixture.state.music_request == 0U &&
                fixture.state.music_first_stream == 0U &&
                fixture.state.music_second_stream == 0U &&
                fixture.state.music_control_flags ==
                    (test_case.initial_control_flags & 0xFF5CFF00U) &&
                fixture.state.current_first_stream ==
                    test_case.expected_transition_mode &&
                fixture.state.current_stream_fade_divisor ==
                    test_case.expected_fade_divisor &&
                fixture.state.current_second_stream ==
                    test_case.expected_pending_fade_divisor &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_137_STOP_SCENE_MUSIC_STREAM &&
                fixture.ports.music_transition_apply_count ==
                    test_case.expected_transition_calls &&
                fixture.ports.story_protocol_events ==
                    (test_case.expected_transition_calls != 0U
                         ? std::vector<u32>{11U}
                         : std::vector<u32>{}) &&
                (test_case.expected_transition_calls == 0U ||
                 transition_saw_prior_slots),
            "opcode 137 aliases conditionally synchronize the old scene stream, stage the world-music request, clear the scene slots and flags, publish previous, and continue in the same call without audio"
        );
    }

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.music_control_flags = 0U;
    exact_tail.state.current_first_stream = 2U;
    exact_tail.state.current_stream_fade_divisor = 0x12345678U;
    exact_tail.state.current_second_stream = 15U;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_137_STOP_SCENE_MUSIC_STREAM);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_137_STOP_SCENE_MUSIC_STREAM &&
            exact_tail.state.world_music_request == 0x80000001U &&
            exact_tail.state.current_first_stream == 0U &&
            exact_tail.state.current_stream_fade_divisor == 0x12345678U &&
            exact_tail.state.current_second_stream == 0U &&
            exact_tail.ports.music_transition_apply_count == 0U,
        "opcode 137 completes an exact-tail two-byte record before the next same-call fetch fails without servicing audio"
    );
}

void test_role_distance_reload_protocol(openswd3::test::Context& test) {
    const auto write_record = [](Fixture& fixture,
                                 const std::size_t ip,
                                 const u16 raw_opcode,
                                 const u16 tile_x,
                                 const u16 tile_y,
                                 const u16 selector,
                                 const u16 radius,
                                 const u32 target) {
        write_u16(fixture.state.window, ip, raw_opcode);
        write_u16(fixture.state.window, ip + 2U, tile_x);
        write_u16(fixture.state.window, ip + 4U, tile_y);
        write_u16(fixture.state.window, ip + 6U, selector);
        write_u16(fixture.state.window, ip + 8U, radius);
        write_u32(fixture.state.window, ip + 10U, target);
    };
    struct TestCase {
        u16 alias_mask;
        u16 selector;
        u16 role_guid;
        u16 context_source_guid;
        u32 role_world_x;
        bool should_reload;
    };
    constexpr std::array cases{
        TestCase{0U, 1U, 1U, 0x7777U, 232U, false},
        TestCase{0x4000U, 1U, 1U, 0x7777U, 233U, true},
        TestCase{0x8000U, 0xFFF0U, 1U, 0x7777U, 232U, false},
        TestCase{0xC000U, 0xFFFEU, 0x7777U, 0x8888U, 232U, false},
    };
    constexpr u32 target = 0x12345678U;

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS | test_case.alias_mask
            )
        );
        fixture.context.source_guid = test_case.context_source_guid;
        fixture.roles[0].guid = test_case.role_guid;
        fixture.roles[0].flags = 0x10000000U;
        fixture.roles[1].guid = test_case.role_guid;
        fixture.roles[1].world_x = test_case.role_world_x;
        fixture.roles[1].world_y = 320U;
        fixture.state.previous_opcode = 0x66U;
        write_record(
            fixture,
            0U,
            static_cast<u16>(
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS | test_case.alias_mask
            ),
            10U,
            20U,
            test_case.selector,
            5U,
            target
        );
        write_u16(fixture.state.window, 14U, kStoryVmTypedStop);
        write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);

        const auto result = fixture.step(0, 0, 1U);
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count ==
                    (test_case.should_reload ? 1U : 0U) &&
                fixture.context.talk_data_offset ==
                    (test_case.should_reload ? target : 0x1111U) &&
                fixture.context.instruction_offset ==
                    (test_case.should_reload ? 0U : 14U) &&
                fixture.state.previous_opcode ==
                    OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
                fixture.ports.data_load_count ==
                    (test_case.should_reload ? 1U : 0U) &&
                fixture.ports.story_protocol_events ==
                    (test_case.should_reload ? std::vector<u32>{2U, 5U}
                                             : std::vector<u32>{}),
            "opcode 138 aliases use a strict scaled radius, map FFF0 through the controlled index as a GUID, resolve FFFE directly, and same-call either the sequential or reloaded target"
        );
    }

    Fixture wrapped_negative;
    prime_loaded_instruction(
        wrapped_negative, OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    wrapped_negative.roles[1].guid = 1U;
    wrapped_negative.roles[1].world_x = 32760U;
    wrapped_negative.roles[1].world_y = 32768U;
    write_record(
        wrapped_negative,
        0U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS,
        0U,
        0U,
        1U,
        0U,
        target
    );
    write_u16(wrapped_negative.state.window, 14U, kStoryVmTypedStop);
    const auto wrapped_negative_result = wrapped_negative.step();

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    invalid_controlled.state.previous_opcode = 0x66U;
    write_record(
        invalid_controlled,
        0U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS,
        0U,
        0U,
        0xFFFEU,
        0U,
        target
    );
    const auto invalid_controlled_result = invalid_controlled.step(0, 0, 99U);

    Fixture load_failure;
    prime_loaded_instruction(
        load_failure, OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    load_failure.roles[1].guid = 1U;
    load_failure.roles[1].world_x = 1000U;
    load_failure.roles[1].world_y = 1000U;
    load_failure.state.previous_opcode = 0x66U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    write_record(
        load_failure,
        0U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS,
        0U,
        0U,
        1U,
        0U,
        target
    );
    const auto load_failure_result = load_failure.step();

    test.expect_true(
        wrapped_negative_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            wrapped_negative_result.executed_instruction_count == 2U &&
            wrapped_negative.ports.data_load_count == 0U &&
            wrapped_negative.context.instruction_offset == 14U &&
            wrapped_negative.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x66U &&
            invalid_controlled.ports.data_load_count == 0U &&
            load_failure_result.status ==
                LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 138 preserves the x87 negative-square low-dword zero, isolates an invalid controlled index at role access, and publishes previous after reload failure"
    );

    const auto prime_tail = [](Fixture& fixture, const u16 ip) {
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = ip;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        fixture.roles[1].guid = 1U;
    };

    Fixture missing_short;
    prime_tail(missing_short, 0x7FF8U);
    write_u16(
        missing_short.state.window,
        0x7FF8U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    write_u16(missing_short.state.window, 0x7FFEU, 0x7777U);
    const auto missing_short_result = missing_short.step();

    Fixture threshold_truncated;
    prime_tail(threshold_truncated, 0x7FF8U);
    write_u16(
        threshold_truncated.state.window,
        0x7FF8U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    write_u16(threshold_truncated.state.window, 0x7FFEU, 1U);
    const auto threshold_truncated_result = threshold_truncated.step();

    Fixture target_truncated;
    prime_tail(target_truncated, 0x7FF6U);
    target_truncated.roles[1].world_x = 1000U;
    target_truncated.roles[1].world_y = 1000U;
    write_u16(
        target_truncated.state.window,
        0x7FF6U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    write_u16(target_truncated.state.window, 0x7FF8U, 0U);
    write_u16(target_truncated.state.window, 0x7FFAU, 0U);
    write_u16(target_truncated.state.window, 0x7FFCU, 1U);
    write_u16(target_truncated.state.window, 0x7FFEU, 0U);
    const auto target_truncated_result = target_truncated.step();

    Fixture target_unread;
    prime_tail(target_unread, 0x7FF6U);
    target_unread.roles[1].world_x = 8U;
    target_unread.roles[1].world_y = 0U;
    write_u16(
        target_unread.state.window,
        0x7FF6U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS
    );
    write_u16(target_unread.state.window, 0x7FF8U, 1U);
    write_u16(target_unread.state.window, 0x7FFAU, 0U);
    write_u16(target_unread.state.window, 0x7FFCU, 1U);
    write_u16(target_unread.state.window, 0x7FFEU, 0U);
    const auto target_unread_result = target_unread.step();

    Fixture exact_tail;
    prime_tail(exact_tail, 0x7FF2U);
    exact_tail.roles[1].world_x = 8U;
    exact_tail.roles[1].world_y = 0U;
    write_record(
        exact_tail,
        0x7FF2U,
        OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS,
        1U,
        0U,
        1U,
        0U,
        target
    );
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        missing_short_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            missing_short_result.executed_instruction_count == 1U &&
            missing_short.context.instruction_offset == 0x8006U &&
            missing_short.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            threshold_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            threshold_truncated.context.instruction_offset == 0x7FF8U &&
            threshold_truncated.state.previous_opcode == 0x66U &&
            target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FF6U &&
            target_truncated.state.previous_opcode == 0x66U &&
            target_unread_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            target_unread.context.instruction_offset == 0x8004U &&
            target_unread.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            missing_short.ports.data_load_count == 0U &&
            threshold_truncated.ports.data_load_count == 0U &&
            target_truncated.ports.data_load_count == 0U &&
            target_unread.ports.data_load_count == 0U &&
            exact_tail.ports.data_load_count == 0U,
        "opcode 138 reads the selector first, skips all remaining fields on a miss, checks radius only for a live role, reads the target only when taken, and preserves exact-tail same-call behavior"
    );
}

void test_configure_music_stream_transition_protocol(
    openswd3::test::Context& test
) {
    struct TestCase {
        u16 alias_mask;
        u16 mode;
        u16 pending_divisor;
    };
    constexpr std::array cases{
        TestCase{0U, 0U, 0U},
        TestCase{0x4000U, 1U, 25U},
        TestCase{0x8000U, 2U, 45U},
        TestCase{0xC000U, 0xFFFFU, 0xFFFFU},
    };

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION | test_case.alias_mask
            )
        );
        write_u16(fixture.state.window, 2U, test_case.mode);
        write_u16(fixture.state.window, 4U, test_case.pending_divisor);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.current_first_stream = 0x11111111U;
        fixture.state.current_stream_fade_divisor = 0x22222222U;
        fixture.state.current_second_stream = 0x33333333U;
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.state.current_first_stream == test_case.mode &&
                fixture.state.current_stream_fade_divisor == 0x22222222U &&
                fixture.state.current_second_stream ==
                    test_case.pending_divisor &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION &&
                fixture.ports.music_transition_apply_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 141 aliases zero-extend both transition parameters, preserve the current fade divisor, publish previous, and continue without applying or servicing audio"
        );
    }

    Fixture mode_truncated;
    mode_truncated.context.instruction_offset = 0x7FFEU;
    mode_truncated.context.talk_data_offset = 0x1111U;
    mode_truncated.state.loaded_file_number = 1U;
    mode_truncated.state.loaded_data_offset = 0x1111U;
    mode_truncated.state.window_loaded = true;
    mode_truncated.state.current_first_stream = 0x11111111U;
    mode_truncated.state.current_second_stream = 0x22222222U;
    mode_truncated.state.previous_opcode = 0x66U;
    write_u16(
        mode_truncated.state.window,
        0x7FFEU,
        OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION
    );
    const auto mode_truncated_result = mode_truncated.step();

    Fixture pending_truncated;
    pending_truncated.context.instruction_offset = 0x7FFCU;
    pending_truncated.context.talk_data_offset = 0x1111U;
    pending_truncated.state.loaded_file_number = 1U;
    pending_truncated.state.loaded_data_offset = 0x1111U;
    pending_truncated.state.window_loaded = true;
    pending_truncated.state.current_first_stream = 0x11111111U;
    pending_truncated.state.current_stream_fade_divisor = 0x33333333U;
    pending_truncated.state.current_second_stream = 0x22222222U;
    pending_truncated.state.previous_opcode = 0x66U;
    write_u16(
        pending_truncated.state.window,
        0x7FFCU,
        OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION
    );
    write_u16(pending_truncated.state.window, 0x7FFEU, 0xABCDU);
    const auto pending_truncated_result = pending_truncated.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.current_stream_fade_divisor = 0x44444444U;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FFAU,
        OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 2U);
    write_u16(exact_tail.state.window, 0x7FFEU, 90U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        mode_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            mode_truncated.state.current_first_stream == 0x11111111U &&
            mode_truncated.state.current_second_stream == 0x22222222U &&
            mode_truncated.context.instruction_offset == 0x7FFEU &&
            mode_truncated.state.previous_opcode == 0x66U &&
            pending_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            pending_truncated.state.current_first_stream == 0xABCDU &&
            pending_truncated.state.current_stream_fade_divisor ==
                0x33333333U &&
            pending_truncated.state.current_second_stream == 0x22222222U &&
            pending_truncated.context.instruction_offset == 0x7FFCU &&
            pending_truncated.state.previous_opcode == 0x66U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.state.current_first_stream == 2U &&
            exact_tail.state.current_stream_fade_divisor == 0x44444444U &&
            exact_tail.state.current_second_stream == 90U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION,
        "opcode 141 commits mode before a missing pending divisor and completes an exact-tail record before the next same-call fetch"
    );
}

void test_initialize_primary_countdown_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        u16 minutes;
        u16 seconds;
        u16 primary_transition_value;
    };
    constexpr std::array cases{
        TestCase{0U, 0U, 0U, 0U},
        TestCase{0x4000U, 5U, 0U, 728U},
        TestCase{0x8000U, 1U, 59U, 0x8000U},
        TestCase{0xC000U, 0xFFFFU, 0xFFFFU, 0xFFFFU},
    };

    for (const auto& test_case : cases) {
        Fixture fixture;
        openswd3::rendering::LegacyCountdownState countdown{
            .primary_ticks = 0x11111111U,
            .secondary_ticks = 0x22222222U,
            .primary_transition_value = 0x33333333U,
            .primary_value_004c97e8 = 0x44444444U,
            .primary_value_004c97ec = 0x55555555U,
            .secondary_value_004bab78 = 0x66666666U,
            .secondary_value_004bab7c = 0x77777777U,
        };
        fixture.runtime.countdown = &countdown;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_142_INITIALIZE_PRIMARY_COUNTDOWN | test_case.alias_mask
            )
        );
        write_u16(fixture.state.window, 2U, test_case.minutes);
        write_u16(fixture.state.window, 4U, test_case.seconds);
        write_u16(fixture.state.window, 6U, test_case.primary_transition_value);
        write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        const u32 expected_ticks = 30U *
            (60U * static_cast<u32>(test_case.minutes) +
             static_cast<u32>(test_case.seconds));
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_142_INITIALIZE_PRIMARY_COUNTDOWN &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                countdown.primary_ticks == expected_ticks &&
                countdown.secondary_ticks == 0x22222222U &&
                countdown.primary_transition_value ==
                    test_case.primary_transition_value &&
                countdown.primary_value_004c97e8 == 0U &&
                countdown.primary_value_004c97ec == 0U &&
                countdown.secondary_value_004bab78 == 0x66666666U &&
                countdown.secondary_value_004bab7c == 0x77777777U &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x10U
                ) &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x12U
                ) &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_142_INITIALIZE_PRIMARY_COUNTDOWN,
            "opcode 142 aliases initialize the existing primary countdown owner with wrapping 30 Hz arithmetic, two flags, previous, audio maintenance, and yield"
        );
    }

    Fixture truncated;
    openswd3::rendering::LegacyCountdownState truncated_countdown{
        .primary_ticks = 0x11111111U,
        .primary_transition_value = 0x22222222U,
        .primary_value_004c97e8 = 0x33333333U,
        .primary_value_004c97ec = 0x44444444U,
    };
    truncated.runtime.countdown = &truncated_countdown;
    truncated.context.instruction_offset = 0x7FFAU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(
        truncated.state.window, 0x7FFAU, OP_142_INITIALIZE_PRIMARY_COUNTDOWN
    );
    write_u16(truncated.state.window, 0x7FFCU, 5U);
    write_u16(truncated.state.window, 0x7FFEU, 0U);
    const auto truncated_result = truncated.step();

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_142_INITIALIZE_PRIMARY_COUNTDOWN);
    write_u16(unavailable.state.window, 2U, 5U);
    write_u16(unavailable.state.window, 4U, 0U);
    write_u16(unavailable.state.window, 6U, 728U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();

    Fixture exact_tail;
    openswd3::rendering::LegacyCountdownState exact_countdown{
        .primary_value_004c97e8 = 1U,
        .primary_value_004c97ec = 2U,
    };
    exact_tail.runtime.countdown = &exact_countdown;
    exact_tail.context.instruction_offset = 0x7FF8U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FF8U, OP_142_INITIALIZE_PRIMARY_COUNTDOWN
    );
    write_u16(exact_tail.state.window, 0x7FFAU, 5U);
    write_u16(exact_tail.state.window, 0x7FFCU, 0U);
    write_u16(exact_tail.state.window, 0x7FFEU, 1111U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_countdown.primary_ticks == 0x11111111U &&
            truncated_countdown.primary_transition_value == 0x22222222U &&
            truncated_countdown.primary_value_004c97e8 == 0x33333333U &&
            truncated_countdown.primary_value_004c97ec == 0x44444444U &&
            truncated.context.instruction_offset == 0x7FFAU &&
            truncated.state.previous_opcode == 0x66U &&
            unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_countdown.primary_ticks == 9000U &&
            exact_countdown.primary_transition_value == 1111U &&
            exact_countdown.primary_value_004c97e8 == 0U &&
            exact_countdown.primary_value_004c97ec == 0U &&
            openswd3::world_map::query_legacy_world_story_flag(
                exact_tail.state, 0x10U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                exact_tail.state, 0x12U
            ) &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_142_INITIALIZE_PRIMARY_COUNTDOWN,
        "opcode 142 rejects a missing final operand before mutation, typed-stops at a missing countdown owner, and completes an exact-tail call before audio maintenance and yield"
    );
}

void test_disable_primary_countdown_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        openswd3::rendering::LegacyCountdownState countdown{
            .primary_ticks = 0x11111111U,
            .secondary_ticks = 0x22222222U,
            .primary_transition_value = 0x33333333U,
            .primary_value_004c97e8 = 0x44444444U,
            .primary_value_004c97ec = 0x55555555U,
            .secondary_value_004bab78 = 0x66666666U,
            .secondary_value_004bab7c = 0x77777777U,
        };
        fixture.runtime.countdown = &countdown;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_143_DISABLE_PRIMARY_COUNTDOWN | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x10U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x11U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x12U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x4CU);
        fixture.state.previous_opcode = 0x66U;
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            const bool primary_active =
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x10U
                );
            const bool primary_companion =
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x12U
                );
            const bool suppressed =
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x4CU
                );

            committed_before_audio = countdown.primary_ticks == 0xFFFFFFFFU &&
                !primary_active && !primary_companion && !suppressed &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_143_DISABLE_PRIMARY_COUNTDOWN;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_143_DISABLE_PRIMARY_COUNTDOWN &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                countdown.primary_ticks == 0xFFFFFFFFU &&
                countdown.secondary_ticks == 0x22222222U &&
                countdown.primary_transition_value == 0x33333333U &&
                countdown.primary_value_004c97e8 == 0x44444444U &&
                countdown.primary_value_004c97ec == 0x55555555U &&
                countdown.secondary_value_004bab78 == 0x66666666U &&
                countdown.secondary_value_004bab7c == 0x77777777U &&
                !openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x10U
                ) &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x11U
                ) &&
                !openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x12U
                ) &&
                !openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x4CU
                ) &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_143_DISABLE_PRIMARY_COUNTDOWN &&
                committed_before_audio,
            "opcode 143 aliases disable only the primary countdown and three flags before audio maintenance and yield"
        );
    }

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_143_DISABLE_PRIMARY_COUNTDOWN);
    write_u16(unavailable.state.window, 2U, kStoryVmTypedStop);
    openswd3::world_map::set_legacy_world_story_flag(unavailable.state, 0x10U);
    openswd3::world_map::set_legacy_world_story_flag(unavailable.state, 0x12U);
    openswd3::world_map::set_legacy_world_story_flag(unavailable.state, 0x4CU);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();

    Fixture exact_tail;
    openswd3::rendering::LegacyCountdownState exact_countdown{
        .primary_ticks = 1234U,
    };
    exact_tail.runtime.countdown = &exact_countdown;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    openswd3::world_map::set_legacy_world_story_flag(exact_tail.state, 0x10U);
    openswd3::world_map::set_legacy_world_story_flag(exact_tail.state, 0x12U);
    openswd3::world_map::set_legacy_world_story_flag(exact_tail.state, 0x4CU);
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_143_DISABLE_PRIMARY_COUNTDOWN
    );
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_result.direct_audio_service_count == 0U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U &&
            openswd3::world_map::query_legacy_world_story_flag(
                unavailable.state, 0x10U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                unavailable.state, 0x12U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                unavailable.state, 0x4CU
            ) &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_countdown.primary_ticks == 0xFFFFFFFFU &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_143_DISABLE_PRIMARY_COUNTDOWN,
        "opcode 143 typed-stops at its first countdown write and completes an exact-tail record before audio maintenance and yield"
    );
}

void test_request_special_mode_four_or_five_protocol(
    openswd3::test::Context& test
) {
    struct TestCase {
        u16 alias_mask;
        u8 selector;
        u8 unread_padding;
        u32 expected_mode;
    };
    constexpr std::array cases{
        TestCase{0U, 0U, 0x11U, 0x80000004U},
        TestCase{0x4000U, 1U, 0x22U, 0x80000005U},
        TestCase{0x8000U, 2U, 0x33U, 0x80000004U},
        TestCase{0xC000U, 0xFFU, 0x44U, 0x80000004U},
    };

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5 | test_case.alias_mask
            )
        );
        fixture.state.window[2U] = test_case.selector;
        fixture.state.window[3U] = test_case.unread_padding;
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        fixture.special_mode_state = 0x11111111U;
        fixture.high_priority_state = 0x22222222U;
        fixture.high_priority_submode = 0x33333333U;
        fixture.high_priority_auxiliary = 0x44444444U;
        fixture.special_input_mode = 0x55555555U;
        fixture.state.previous_opcode = 0x66U;
        bool reset_saw_pre_state = false;
        bool audio_saw_committed_state = false;
        fixture.ports.input_menu_reset_callback = [&]() {
            reset_saw_pre_state =
                fixture.special_mode_state == test_case.expected_mode &&
                fixture.high_priority_state == 0U &&
                fixture.high_priority_submode == 0x33333333U &&
                fixture.high_priority_auxiliary == 0x44444444U &&
                fixture.special_input_mode == 0x55555555U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0x66U;
        };
        fixture.ports.audio_service_callback = [&]() {
            audio_saw_committed_state =
                fixture.special_mode_state == test_case.expected_mode &&
                fixture.high_priority_state == 0U &&
                fixture.high_priority_submode == 0U &&
                fixture.high_priority_auxiliary == 0U &&
                fixture.special_input_mode == 0U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_144_REQUEST_SPECIAL_MODE_4_OR_5;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.special_mode_state == test_case.expected_mode &&
                fixture.high_priority_state == 0U &&
                fixture.high_priority_submode == 0U &&
                fixture.high_priority_auxiliary == 0U &&
                fixture.special_input_mode == 0U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
                fixture.ports.input_menu_reset_count == 1U &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{14U, 2U} &&
                reset_saw_pre_state && audio_saw_committed_state,
            "opcode 144 aliases select only mode five for selector byte one, ignore padding, reset in machine order, publish previous, service audio, and yield"
        );
    }

    Fixture missing_mode_owner;
    missing_mode_owner.runtime.special_mode_state = nullptr;
    prime_loaded_instruction(
        missing_mode_owner, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    missing_mode_owner.state.window[2U] = 1U;
    missing_mode_owner.state.previous_opcode = 0x66U;
    const auto missing_mode_owner_result = missing_mode_owner.step();

    Fixture truncated_selector;
    truncated_selector.context.instruction_offset = 0x7FFEU;
    truncated_selector.context.talk_data_offset = 0x1111U;
    truncated_selector.state.loaded_file_number = 1U;
    truncated_selector.state.loaded_data_offset = 0x1111U;
    truncated_selector.state.window_loaded = true;
    truncated_selector.special_mode_state = 0x11111111U;
    truncated_selector.high_priority_state = 0x22222222U;
    truncated_selector.state.previous_opcode = 0x66U;
    write_u16(
        truncated_selector.state.window,
        0x7FFEU,
        OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    const auto truncated_selector_result = truncated_selector.step();

    Fixture missing_high_state;
    missing_high_state.runtime.high_priority_state = nullptr;
    prime_loaded_instruction(
        missing_high_state, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    missing_high_state.state.window[2U] = 1U;
    missing_high_state.high_priority_state = 0x22222222U;
    const auto missing_high_state_result = missing_high_state.step();

    Fixture failed_reset;
    prime_loaded_instruction(failed_reset, OP_144_REQUEST_SPECIAL_MODE_4_OR_5);
    failed_reset.state.window[2U] = 0U;
    failed_reset.high_priority_state = 0x22222222U;
    failed_reset.high_priority_submode = 0x33333333U;
    failed_reset.high_priority_auxiliary = 0x44444444U;
    failed_reset.special_input_mode = 0x55555555U;
    failed_reset.ports.input_menu_reset_success = false;
    const auto failed_reset_result = failed_reset.step();

    Fixture missing_submode;
    missing_submode.runtime.high_priority_submode = nullptr;
    prime_loaded_instruction(
        missing_submode, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    missing_submode.state.window[2U] = 0U;
    missing_submode.high_priority_submode = 0x33333333U;
    missing_submode.high_priority_auxiliary = 0x44444444U;
    missing_submode.special_input_mode = 0x55555555U;
    const auto missing_submode_result = missing_submode.step();

    Fixture missing_auxiliary;
    missing_auxiliary.runtime.high_priority_auxiliary = nullptr;
    prime_loaded_instruction(
        missing_auxiliary, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    missing_auxiliary.state.window[2U] = 0U;
    missing_auxiliary.high_priority_submode = 0x33333333U;
    missing_auxiliary.high_priority_auxiliary = 0x44444444U;
    missing_auxiliary.special_input_mode = 0x55555555U;
    const auto missing_auxiliary_result = missing_auxiliary.step();

    Fixture missing_input;
    missing_input.runtime.special_input_mode = nullptr;
    prime_loaded_instruction(missing_input, OP_144_REQUEST_SPECIAL_MODE_4_OR_5);
    missing_input.state.window[2U] = 0U;
    missing_input.high_priority_submode = 0x33333333U;
    missing_input.high_priority_auxiliary = 0x44444444U;
    missing_input.special_input_mode = 0x55555555U;
    const auto missing_input_result = missing_input.step();

    test.expect_true(
        missing_mode_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_mode_owner.ports.input_menu_reset_count == 0U &&
            truncated_selector_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_selector.special_mode_state == 0x80000004U &&
            truncated_selector.high_priority_state == 0x22222222U &&
            truncated_selector.context.instruction_offset == 0x7FFEU &&
            truncated_selector.state.previous_opcode == 0x66U &&
            truncated_selector.ports.input_menu_reset_count == 0U &&
            missing_high_state_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_high_state.special_mode_state == 0x80000005U &&
            missing_high_state.high_priority_state == 0x22222222U &&
            missing_high_state.ports.input_menu_reset_count == 0U &&
            failed_reset_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            failed_reset.special_mode_state == 0x80000004U &&
            failed_reset.high_priority_state == 0U &&
            failed_reset.high_priority_submode == 0x33333333U &&
            failed_reset.high_priority_auxiliary == 0x44444444U &&
            failed_reset.special_input_mode == 0x55555555U &&
            failed_reset.ports.input_menu_reset_count == 1U &&
            missing_submode_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_submode.high_priority_state == 0U &&
            missing_submode.high_priority_submode == 0x33333333U &&
            missing_submode.high_priority_auxiliary == 0x44444444U &&
            missing_submode.special_input_mode == 0x55555555U &&
            missing_submode.ports.input_menu_reset_count == 1U &&
            missing_auxiliary_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_auxiliary.high_priority_state == 0U &&
            missing_auxiliary.high_priority_submode == 0U &&
            missing_auxiliary.high_priority_auxiliary == 0x44444444U &&
            missing_auxiliary.special_input_mode == 0x55555555U &&
            missing_auxiliary.ports.input_menu_reset_count == 1U &&
            missing_input_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_input.high_priority_state == 0U &&
            missing_input.high_priority_submode == 0U &&
            missing_input.high_priority_auxiliary == 0U &&
            missing_input.special_input_mode == 0x55555555U &&
            missing_input.ports.input_menu_reset_count == 1U,
        "opcode 144 preserves every committed write across selector, binding, and reset-helper failure boundaries"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FFCU, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    exact_tail.state.window[0x7FFEU] = 1U;
    exact_tail.state.window[0x7FFFU] = 0xA5U;
    const auto exact_tail_result = exact_tail.step();

    Fixture unread_padding;
    unread_padding.context.instruction_offset = 0x7FFDU;
    unread_padding.context.talk_data_offset = 0x1111U;
    unread_padding.state.loaded_file_number = 1U;
    unread_padding.state.loaded_data_offset = 0x1111U;
    unread_padding.state.window_loaded = true;
    unread_padding.state.previous_opcode = 0x66U;
    write_u16(
        unread_padding.state.window, 0x7FFDU, OP_144_REQUEST_SPECIAL_MODE_4_OR_5
    );
    unread_padding.state.window[0x7FFFU] = 1U;
    const auto unread_padding_result = unread_padding.step();

    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.special_mode_state == 0x80000005U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            unread_padding_result.status == LegacyWorldStoryVmStatus::yielded &&
            unread_padding_result.executed_instruction_count == 1U &&
            unread_padding_result.direct_audio_service_count == 1U &&
            unread_padding.special_mode_state == 0x80000005U &&
            unread_padding.context.instruction_offset == 0x8001U &&
            unread_padding.state.previous_opcode ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5,
        "opcode 144 completes at the exact tail and also ignores a missing nominal padding byte"
    );
}

void test_set_story_flag_70_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        bool initially_set;
    };
    constexpr std::array cases{
        TestCase{0U, false},
        TestCase{0x4000U, true},
        TestCase{0x8000U, false},
        TestCase{0xC000U, true},
    };

    for (const auto test_case : cases) {
        Fixture fixture;
        fixture.state.flags.fill(test_case.initially_set ? 0xFFU : 0U);
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_147_SET_STORY_FLAG_70 | test_case.alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        auto expected_flags = fixture.state.flags;
        expected_flags[70U >> 3U] |= static_cast<u8>(1U << (70U & 7U));
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio = fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_147_SET_STORY_FLAG_70;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_147_SET_STORY_FLAG_70 | test_case.alias_mask
                    ) &&
                result.opcode == OP_147_SET_STORY_FLAG_70 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_147_SET_STORY_FLAG_70 &&
                committed_before_audio,
            "opcode 147 aliases set only story flag 70, publish previous, service audio, and yield"
        );
    }

    Fixture exact_tail;
    exact_tail.state.flags.fill(0x1AU);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    auto expected_tail_flags = exact_tail.state.flags;
    expected_tail_flags[70U >> 3U] |= static_cast<u8>(1U << (70U & 7U));
    write_u16(exact_tail.state.window, 0x7FFEU, OP_147_SET_STORY_FLAG_70);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_147_SET_STORY_FLAG_70 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.state.flags == expected_tail_flags &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_147_SET_STORY_FLAG_70,
        "opcode 147 completes its fixed flag write before audio maintenance and yield at the exact window tail"
    );
}

void test_set_story_flag_19_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        bool initially_set;
    };
    constexpr std::array cases{
        TestCase{0U, false},
        TestCase{0x4000U, true},
        TestCase{0x8000U, false},
        TestCase{0xC000U, true},
    };

    for (const auto test_case : cases) {
        Fixture fixture;
        fixture.state.flags.fill(test_case.initially_set ? 0xFFU : 0U);
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_148_SET_STORY_FLAG_19 | test_case.alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        auto expected_flags = fixture.state.flags;
        expected_flags[19U >> 3U] |= static_cast<u8>(1U << (19U & 7U));
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio = fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_148_SET_STORY_FLAG_19;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_148_SET_STORY_FLAG_19 | test_case.alias_mask
                    ) &&
                result.opcode == OP_148_SET_STORY_FLAG_19 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_148_SET_STORY_FLAG_19 &&
                committed_before_audio,
            "opcode 148 aliases set only story flag 19, publish previous, service audio, and yield"
        );
    }

    Fixture exact_tail;
    exact_tail.state.flags.fill(0xA5U);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    auto expected_tail_flags = exact_tail.state.flags;
    expected_tail_flags[19U >> 3U] |= static_cast<u8>(1U << (19U & 7U));
    write_u16(exact_tail.state.window, 0x7FFEU, OP_148_SET_STORY_FLAG_19);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_148_SET_STORY_FLAG_19 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.state.flags == expected_tail_flags &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_148_SET_STORY_FLAG_19,
        "opcode 148 completes its fixed flag write before audio maintenance and yield at the exact window tail"
    );
}

void test_clear_story_flag_19_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        bool initially_set;
    };
    constexpr std::array cases{
        TestCase{0U, true},
        TestCase{0x4000U, false},
        TestCase{0x8000U, true},
        TestCase{0xC000U, false},
    };

    for (const auto test_case : cases) {
        Fixture fixture;
        fixture.state.flags.fill(test_case.initially_set ? 0xFFU : 0U);
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_149_CLEAR_STORY_FLAG_19 | test_case.alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        auto expected_flags = fixture.state.flags;
        expected_flags[19U >> 3U] &=
            static_cast<u8>(~static_cast<u8>(1U << (19U & 7U)));
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio = fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_149_CLEAR_STORY_FLAG_19;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_149_CLEAR_STORY_FLAG_19 | test_case.alias_mask
                    ) &&
                result.opcode == OP_149_CLEAR_STORY_FLAG_19 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.flags == expected_flags &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_149_CLEAR_STORY_FLAG_19 &&
                committed_before_audio,
            "opcode 149 aliases clear only story flag 19, publish previous, service audio, and yield"
        );
    }

    Fixture exact_tail;
    exact_tail.state.flags.fill(0xADU);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    auto expected_tail_flags = exact_tail.state.flags;
    expected_tail_flags[19U >> 3U] &=
        static_cast<u8>(~static_cast<u8>(1U << (19U & 7U)));
    write_u16(exact_tail.state.window, 0x7FFEU, OP_149_CLEAR_STORY_FLAG_19);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_149_CLEAR_STORY_FLAG_19 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.state.flags == expected_tail_flags &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_149_CLEAR_STORY_FLAG_19,
        "opcode 149 completes its fixed flag clear before audio maintenance and yield at the exact window tail"
    );
}

void test_configure_ani_follower_position_protocol(
    openswd3::test::Context& test
) {
    struct TestCase {
        u16 alias_mask;
        i16 raw_x;
        i16 raw_y;
        i32 expected_x;
        i32 expected_y;
    };
    constexpr std::array cases{
        TestCase{0U, 20, 15, 320, 240},
        TestCase{0x4000U, 30, 10, 432, 208},
        TestCase{0x8000U, 5, 30, 208, 272},
        TestCase{0xC000U, -1, -1, 208, 208},
    };

    for (const auto test_case : cases) {
        Fixture fixture;
        fixture.ani_follower = {
            .current_x = 1,
            .current_y = 2,
            .target_x = 3,
            .target_y = 777,
            .velocity_x = 4,
            .velocity_y = 5,
        };
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_150_CONFIGURE_ANI_FOLLOWER_POSITION | test_case.alias_mask
            )
        );
        write_u16(
            fixture.state.window, 2U, std::bit_cast<u16>(test_case.raw_x)
        );
        write_u16(
            fixture.state.window, 4U, std::bit_cast<u16>(test_case.raw_y)
        );
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio =
                fixture.ani_follower.current_x == test_case.expected_x &&
                fixture.ani_follower.current_y == test_case.expected_y &&
                fixture.ani_follower.target_x == test_case.expected_x &&
                fixture.ani_follower.target_y == 777 &&
                fixture.ani_follower.velocity_x == 0 &&
                fixture.ani_follower.velocity_y == 0 &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_150_CONFIGURE_ANI_FOLLOWER_POSITION;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_150_CONFIGURE_ANI_FOLLOWER_POSITION |
                        test_case.alias_mask
                    ) &&
                result.opcode == OP_150_CONFIGURE_ANI_FOLLOWER_POSITION &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.ani_follower.current_x == test_case.expected_x &&
                fixture.ani_follower.current_y == test_case.expected_y &&
                fixture.ani_follower.target_x == test_case.expected_x &&
                fixture.ani_follower.target_y == 777 &&
                fixture.ani_follower.velocity_x == 0 &&
                fixture.ani_follower.velocity_y == 0 &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_150_CONFIGURE_ANI_FOLLOWER_POSITION &&
                committed_before_audio,
            "opcode 150 scales and clamps both follower coordinates, preserves target y, clears both velocities, publishes previous, services audio, and yields"
        );
    }

    Fixture missing_x;
    missing_x.ani_follower = {
        .current_x = 1,
        .current_y = 2,
        .target_x = 3,
        .target_y = 4,
        .velocity_x = 5,
        .velocity_y = 6,
    };
    missing_x.context.instruction_offset = 0x7FFEU;
    missing_x.context.talk_data_offset = 0x1111U;
    missing_x.state.loaded_file_number = 1U;
    missing_x.state.loaded_data_offset = 0x1111U;
    missing_x.state.window_loaded = true;
    write_u16(
        missing_x.state.window, 0x7FFEU, OP_150_CONFIGURE_ANI_FOLLOWER_POSITION
    );
    const auto missing_x_result = missing_x.step();
    test.expect_true(
        missing_x_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_x.ani_follower.current_x == 1 &&
            missing_x.ani_follower.current_y == 2 &&
            missing_x.context.instruction_offset == 0x7FFEU,
        "opcode 150 missing x stops before the first follower write"
    );

    Fixture missing_y;
    missing_y.ani_follower = {
        .current_x = 1,
        .current_y = 2,
        .target_x = 3,
        .target_y = 4,
        .velocity_x = 5,
        .velocity_y = 6,
    };
    missing_y.context.instruction_offset = 0x7FFCU;
    missing_y.context.talk_data_offset = 0x1111U;
    missing_y.state.loaded_file_number = 1U;
    missing_y.state.loaded_data_offset = 0x1111U;
    missing_y.state.window_loaded = true;
    write_u16(
        missing_y.state.window, 0x7FFCU, OP_150_CONFIGURE_ANI_FOLLOWER_POSITION
    );
    write_u16(missing_y.state.window, 0x7FFEU, std::bit_cast<u16>(i16{30}));
    const auto missing_y_result = missing_y.step();
    test.expect_true(
        missing_y_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_y.ani_follower.current_x == 480 &&
            missing_y.ani_follower.current_y == 2 &&
            missing_y.ani_follower.target_x == 3 &&
            missing_y.ani_follower.target_y == 4 &&
            missing_y.ani_follower.velocity_x == 5 &&
            missing_y.ani_follower.velocity_y == 6 &&
            missing_y.context.instruction_offset == 0x7FFCU,
        "opcode 150 missing y preserves the committed unclamped x and all later state"
    );

    Fixture missing_owner;
    prime_loaded_instruction(
        missing_owner, OP_150_CONFIGURE_ANI_FOLLOWER_POSITION
    );
    write_u16(missing_owner.state.window, 2U, 20U);
    missing_owner.runtime.ani_follower = nullptr;
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0U,
        "opcode 150 missing follower owner stops at the first global write before reading y"
    );

    Fixture exact_tail;
    exact_tail.ani_follower.target_y = 999;
    exact_tail.ani_follower.velocity_x = -7;
    exact_tail.ani_follower.velocity_y = 8;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_150_CONFIGURE_ANI_FOLLOWER_POSITION
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 13U);
    write_u16(exact_tail.state.window, 0x7FFEU, 17U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode ==
                OP_150_CONFIGURE_ANI_FOLLOWER_POSITION &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.ani_follower.current_x == 208 &&
            exact_tail.ani_follower.current_y == 272 &&
            exact_tail.ani_follower.target_x == 208 &&
            exact_tail.ani_follower.target_y == 999 &&
            exact_tail.ani_follower.velocity_x == 0 &&
            exact_tail.ani_follower.velocity_y == 0 &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_150_CONFIGURE_ANI_FOLLOWER_POSITION,
        "opcode 150 completes all follower writes before audio maintenance and yield at the exact window tail"
    );
}

void test_configure_ani_follower_target_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.ani_follower = {
            .current_x = 11,
            .current_y = 22,
            .target_x = 33,
            .target_y = 44,
            .velocity_x = 55,
            .velocity_y = 66,
        };
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_151_CONFIGURE_ANI_FOLLOWER_TARGET | alias_mask)
        );
        write_u16(fixture.state.window, 2U, std::bit_cast<u16>(i16{-1}));
        write_u16(fixture.state.window, 4U, 0x1234U);
        write_u16(
            fixture.state.window,
            6U,
            std::bit_cast<u16>(std::numeric_limits<i16>::min())
        );
        write_u16(
            fixture.state.window,
            8U,
            std::bit_cast<u16>(std::numeric_limits<i16>::max())
        );
        write_u16(fixture.state.window, 10U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio = fixture.ani_follower.current_x == 11 &&
                fixture.ani_follower.current_y == 22 &&
                fixture.ani_follower.target_x == -16 &&
                fixture.ani_follower.target_y == 0x12340 &&
                fixture.ani_follower.velocity_x ==
                    std::numeric_limits<i16>::min() &&
                fixture.ani_follower.velocity_y ==
                    std::numeric_limits<i16>::max() &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_151_CONFIGURE_ANI_FOLLOWER_TARGET;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_151_CONFIGURE_ANI_FOLLOWER_TARGET | alias_mask
                    ) &&
                result.opcode == OP_151_CONFIGURE_ANI_FOLLOWER_TARGET &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.ani_follower.current_x == 11 &&
                fixture.ani_follower.current_y == 22 &&
                fixture.ani_follower.target_x == -16 &&
                fixture.ani_follower.target_y == 0x12340 &&
                fixture.ani_follower.velocity_x ==
                    std::numeric_limits<i16>::min() &&
                fixture.ani_follower.velocity_y ==
                    std::numeric_limits<i16>::max() &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_151_CONFIGURE_ANI_FOLLOWER_TARGET &&
                committed_before_audio,
            "opcode 151 aliases scale both signed targets, sign-extend both velocities, preserve current coordinates, publish previous, service audio, and yield"
        );
    }

    struct TruncationCase {
        u16 instruction_offset;
        std::size_t available_operand_count;
    };
    constexpr std::array truncation_cases{
        TruncationCase{0x7FFEU, 0U},
        TruncationCase{0x7FFCU, 1U},
        TruncationCase{0x7FFAU, 2U},
        TruncationCase{0x7FF8U, 3U},
    };
    constexpr std::array<u16, 4U> operands{
        std::bit_cast<u16>(i16{-1}),
        0x1234U,
        std::bit_cast<u16>(i16{-7}),
        std::bit_cast<u16>(i16{8}),
    };

    for (const auto truncation : truncation_cases) {
        Fixture fixture;
        fixture.ani_follower = {
            .current_x = 11,
            .current_y = 22,
            .target_x = 33,
            .target_y = 44,
            .velocity_x = 55,
            .velocity_y = 66,
        };
        fixture.context.instruction_offset = truncation.instruction_offset;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        write_u16(
            fixture.state.window,
            truncation.instruction_offset,
            OP_151_CONFIGURE_ANI_FOLLOWER_TARGET
        );
        for (std::size_t index = 0U; index < truncation.available_operand_count;
             ++index) {
            write_u16(
                fixture.state.window,
                static_cast<std::size_t>(truncation.instruction_offset) + 2U +
                    index * 2U,
                operands[index]
            );
        }

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                fixture.ani_follower.current_x == 11 &&
                fixture.ani_follower.current_y == 22 &&
                fixture.ani_follower.target_x ==
                    (truncation.available_operand_count >= 1U ? -16 : 33) &&
                fixture.ani_follower.target_y ==
                    (truncation.available_operand_count >= 2U ? 0x12340 : 44) &&
                fixture.ani_follower.velocity_x ==
                    (truncation.available_operand_count >= 3U ? -7 : 55) &&
                fixture.ani_follower.velocity_y == 66 &&
                fixture.context.instruction_offset ==
                    truncation.instruction_offset &&
                fixture.state.previous_opcode == 0U,
            "opcode 151 truncation preserves every earlier staged follower write and no later write"
        );
    }

    Fixture missing_owner;
    prime_loaded_instruction(
        missing_owner, OP_151_CONFIGURE_ANI_FOLLOWER_TARGET
    );
    write_u16(missing_owner.state.window, 2U, 20U);
    missing_owner.runtime.ani_follower = nullptr;
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0U,
        "opcode 151 missing follower owner stops at the first global write before reading target y"
    );

    Fixture exact_tail;
    exact_tail.ani_follower = {
        .current_x = 11,
        .current_y = 22,
        .target_x = 33,
        .target_y = 44,
        .velocity_x = 55,
        .velocity_y = 66,
    };
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FF6U, OP_151_CONFIGURE_ANI_FOLLOWER_TARGET
    );
    write_u16(
        exact_tail.state.window,
        0x7FF8U,
        std::bit_cast<u16>(std::numeric_limits<i16>::min())
    );
    write_u16(
        exact_tail.state.window,
        0x7FFAU,
        std::bit_cast<u16>(std::numeric_limits<i16>::max())
    );
    write_u16(exact_tail.state.window, 0x7FFCU, std::bit_cast<u16>(i16{-1}));
    write_u16(exact_tail.state.window, 0x7FFEU, 2U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_151_CONFIGURE_ANI_FOLLOWER_TARGET &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.ani_follower.current_x == 11 &&
            exact_tail.ani_follower.current_y == 22 &&
            exact_tail.ani_follower.target_x == -0x80000 &&
            exact_tail.ani_follower.target_y == 0x7FFF0 &&
            exact_tail.ani_follower.velocity_x == -1 &&
            exact_tail.ani_follower.velocity_y == 2 &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_151_CONFIGURE_ANI_FOLLOWER_TARGET,
        "opcode 151 completes all four staged follower writes before audio maintenance and yield at the exact window tail"
    );
}

void test_wait_ani_follower_target_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 alias_mask;
        i32 current_x;
        i32 current_y;
        i32 target_x;
        i32 target_y;
        bool complete;
    };
    constexpr std::array cases{
        TestCase{0U, 10, 30, 20, 30, false},
        TestCase{0x4000U, 10, 30, 10, 40, false},
        TestCase{0x8000U, -1, 2, -1, 2, true},
        TestCase{
            0xC000U, 0x7FFFFFFF, -0x7FFFFFFF, 0x7FFFFFFF, -0x7FFFFFFF, true
        },
    };

    for (const auto test_case : cases) {
        Fixture fixture;
        fixture.ani_follower = {
            .current_x = test_case.current_x,
            .current_y = test_case.current_y,
            .target_x = test_case.target_x,
            .target_y = test_case.target_y,
            .velocity_x = -7,
            .velocity_y = 8,
        };
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_152_WAIT_ANI_FOLLOWER_TARGET | test_case.alias_mask
            )
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            committed_before_audio = fixture.context.instruction_offset ==
                    (test_case.complete ? 2U : 0U) &&
                fixture.state.previous_opcode ==
                    OP_152_WAIT_ANI_FOLLOWER_TARGET;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_152_WAIT_ANI_FOLLOWER_TARGET | test_case.alias_mask
                    ) &&
                result.opcode == OP_152_WAIT_ANI_FOLLOWER_TARGET &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.ani_follower.current_x == test_case.current_x &&
                fixture.ani_follower.current_y == test_case.current_y &&
                fixture.ani_follower.target_x == test_case.target_x &&
                fixture.ani_follower.target_y == test_case.target_y &&
                fixture.ani_follower.velocity_x == -7 &&
                fixture.ani_follower.velocity_y == 8 &&
                fixture.context.instruction_offset ==
                    (test_case.complete ? 2U : 0U) &&
                fixture.state.previous_opcode ==
                    OP_152_WAIT_ANI_FOLLOWER_TARGET &&
                committed_before_audio,
            "opcode 152 aliases wait on x then y, preserve follower state, publish previous, service audio, and yield on both waiting and complete paths"
        );
    }

    Fixture missing_owner;
    prime_loaded_instruction(missing_owner, OP_152_WAIT_ANI_FOLLOWER_TARGET);
    missing_owner.state.previous_opcode = 0x66U;
    missing_owner.runtime.ani_follower = nullptr;
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.direct_audio_service_count == 0U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x66U,
        "opcode 152 missing follower owner stops at the first state read without publishing previous or servicing audio"
    );

    for (const bool complete : {false, true}) {
        Fixture fixture;
        fixture.ani_follower = {
            .current_x = 10,
            .current_y = 20,
            .target_x = complete ? 10 : 11,
            .target_y = 20,
            .velocity_x = -7,
            .velocity_y = 8,
        };
        fixture.context.instruction_offset = 0x7FFEU;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        write_u16(
            fixture.state.window, 0x7FFEU, OP_152_WAIT_ANI_FOLLOWER_TARGET
        );

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_152_WAIT_ANI_FOLLOWER_TARGET &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset ==
                    (complete ? 0x8000U : 0x7FFEU) &&
                fixture.state.previous_opcode ==
                    OP_152_WAIT_ANI_FOLLOWER_TARGET,
            "opcode 152 exact-tail waiting and completion both publish previous, service audio, and yield without fetching a successor"
        );
    }
}

void test_reload_current_world_session_protocol(openswd3::test::Context& test) {
    constexpr u32 kWideLogicalMapId = 0x01230015U;
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.runtime.current_logical_map_id = kWideLogicalMapId;
        fixture.runtime.role_surface.selected_guid = 0x00F8U;
        fixture.roles[0].world_x = 0xFFFEDCBAU;
        fixture.roles[0].world_y = 0x81234567U;
        fixture.roles[0].action.action_id = 0x00012345U;
        fixture.state.deferred_map_tile_x = -1;
        fixture.state.deferred_map_tile_y = -1;
        fixture.state.deferred_map_id = 0;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_155_RELOAD_CURRENT_WORLD_SESSION | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        bool committed_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            const auto& request = fixture.ports.last_world_load_request;
            committed_before_audio =
                fixture.state.deferred_map_tile_x == 0x0FFFEDCB &&
                fixture.state.deferred_map_tile_y == 0x08123456 &&
                fixture.state.deferred_map_id ==
                    std::bit_cast<i32>(kWideLogicalMapId) &&
                fixture.ports.world_session_reload_begin_count == 1U &&
                fixture.ports.world_session_reload_count == 1U &&
                request.logical_map_id == 22U && request.tile_x == 59U &&
                request.tile_y == 59U && request.action_id == 0x2345U &&
                request.base_variant == 0U && request.variant_delta == 1U &&
                request.selected_guid == 0x00F8U && request.load_flags == 1U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_155_RELOAD_CURRENT_WORLD_SESSION;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_155_RELOAD_CURRENT_WORLD_SESSION | alias_mask
                    ) &&
                result.opcode == OP_155_RELOAD_CURRENT_WORLD_SESSION &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_155_RELOAD_CURRENT_WORLD_SESSION &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{6U, 7U, 2U} &&
                committed_before_audio,
            "opcode 155 aliases retain the full logical-map dword and shifted role coordinates, submit the fixed map-22 reload, publish previous, service audio, and yield"
        );
    }

    Fixture map_twenty_two;
    map_twenty_two.runtime.current_logical_map_id = 22U;
    map_twenty_two.state.deferred_map_tile_x = 111;
    map_twenty_two.state.deferred_map_tile_y = 222;
    map_twenty_two.state.deferred_map_id = 333;
    prime_loaded_instruction(
        map_twenty_two, OP_155_RELOAD_CURRENT_WORLD_SESSION
    );
    const auto map_twenty_two_result = map_twenty_two.step();
    test.expect_true(
        map_twenty_two_result.status == LegacyWorldStoryVmStatus::yielded &&
            map_twenty_two_result.direct_audio_service_count == 1U &&
            map_twenty_two.state.deferred_map_tile_x == 111 &&
            map_twenty_two.state.deferred_map_tile_y == 222 &&
            map_twenty_two.state.deferred_map_id == 333 &&
            map_twenty_two.ports.world_session_reload_begin_count == 0U &&
            map_twenty_two.ports.world_session_reload_count == 0U &&
            map_twenty_two.context.instruction_offset == 2U &&
            map_twenty_two.state.previous_opcode ==
                OP_155_RELOAD_CURRENT_WORLD_SESSION &&
            map_twenty_two.ports.story_protocol_events == std::vector<u32>{2U},
        "opcode 155 map 22 follows the debug-only no-op path and preserves deferred map state"
    );

    Fixture load_failure;
    load_failure.runtime.current_logical_map_id = 21U;
    load_failure.roles[0].world_x = 0x12340U;
    load_failure.roles[0].world_y = 0x56780U;
    load_failure.roles[0].action.action_id = 0x12345U;
    prime_loaded_instruction(load_failure, OP_155_RELOAD_CURRENT_WORLD_SESSION);
    load_failure.state.previous_opcode = 0x66U;
    load_failure.ports.world_session_reload_success = false;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status ==
                LegacyWorldStoryVmStatus::world_session_load_failed &&
            load_failure_result.direct_audio_service_count == 0U &&
            load_failure.state.deferred_map_tile_x == 0x1234 &&
            load_failure.state.deferred_map_tile_y == 0x5678 &&
            load_failure.state.deferred_map_id == 21 &&
            load_failure.ports.world_session_reload_begin_count == 1U &&
            load_failure.ports.world_session_reload_count == 1U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == 0x66U &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{6U, 7U},
        "opcode 155 checked reload failure preserves all deferred writes but not IP, previous, or audio"
    );

    Fixture exact_tail;
    exact_tail.runtime.current_logical_map_id = 21U;
    exact_tail.runtime.role_surface.selected_guid = 0x00F8U;
    exact_tail.roles[0].world_x = 0x12340U;
    exact_tail.roles[0].world_y = 0x56780U;
    exact_tail.roles[0].action.action_id = 0x12345U;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_155_RELOAD_CURRENT_WORLD_SESSION
    );

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_155_RELOAD_CURRENT_WORLD_SESSION &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.state.deferred_map_tile_x == 0x1234 &&
            exact_tail.state.deferred_map_tile_y == 0x5678 &&
            exact_tail.state.deferred_map_id == 21 &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_155_RELOAD_CURRENT_WORLD_SESSION,
        "opcode 155 completes its synchronous reload before previous, audio maintenance, and yield at the exact window tail"
    );
}

void test_reload_deferred_world_session_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.runtime.role_surface.selected_guid = 0x00F8U;
        fixture.roles[0].action.action_id = 0x00012345U;
        fixture.state.deferred_map_tile_x = -2;
        fixture.state.deferred_map_tile_y = -3;
        fixture.state.deferred_map_id = 0x0001FFFF;
        fixture.state.previous_opcode = 0x66U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_156_RELOAD_DEFERRED_WORLD_SESSION | alias_mask)
        );
        bool deferred_visible_during_reload = false;
        fixture.ports.world_session_reload_callback = [&]() {
            deferred_visible_during_reload =
                fixture.state.deferred_map_tile_x == -2 &&
                fixture.state.deferred_map_tile_y == -3 &&
                fixture.state.deferred_map_id == 0x0001FFFF;
        };
        bool cleared_before_audio = false;
        fixture.ports.audio_service_callback = [&]() {
            cleared_before_audio = fixture.state.deferred_map_tile_x == -1 &&
                fixture.state.deferred_map_tile_y == -1 &&
                fixture.state.deferred_map_id == 0 &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_156_RELOAD_DEFERRED_WORLD_SESSION;
        };

        const auto result = fixture.step();
        const auto& request = fixture.ports.last_world_load_request;
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word ==
                    static_cast<u16>(
                        OP_156_RELOAD_DEFERRED_WORLD_SESSION | alias_mask
                    ) &&
                result.opcode == OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                request.logical_map_id == 0x0001FFFFU &&
                request.tile_x == 0xFFFFFFFEU &&
                request.tile_y == 0xFFFFFFFDU && request.action_id == 0x2345U &&
                request.base_variant == 0U && request.variant_delta == 1U &&
                request.selected_guid == 0x00F8U && request.load_flags == 1U &&
                fixture.ports.world_session_reload_begin_count == 1U &&
                fixture.ports.world_session_reload_count == 1U &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{6U, 7U, 2U} &&
                deferred_visible_during_reload && cleared_before_audio,
            "opcode 156 aliases reload the full positive deferred map dword before clearing it, publish previous, service audio, and yield"
        );
    }

    for (const i32 inactive_map_id : std::array<i32, 2U>{0, -1}) {
        Fixture fixture;
        fixture.state.deferred_map_tile_x = 111;
        fixture.state.deferred_map_tile_y = 222;
        fixture.state.deferred_map_id = inactive_map_id;
        prime_loaded_instruction(fixture, OP_156_RELOAD_DEFERRED_WORLD_SESSION);

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.direct_audio_service_count == 1U &&
                fixture.state.deferred_map_tile_x == 111 &&
                fixture.state.deferred_map_tile_y == 222 &&
                fixture.state.deferred_map_id == inactive_map_id &&
                fixture.ports.world_session_reload_begin_count == 0U &&
                fixture.ports.world_session_reload_count == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U},
            "opcode 156 nonpositive deferred map ids follow the debug-only no-op path and preserve deferred state"
        );
    }

    Fixture load_failure;
    load_failure.state.deferred_map_tile_x = 0x1234;
    load_failure.state.deferred_map_tile_y = 0x5678;
    load_failure.state.deferred_map_id = 21;
    load_failure.state.previous_opcode = 0x66U;
    load_failure.ports.world_session_reload_success = false;
    prime_loaded_instruction(
        load_failure, OP_156_RELOAD_DEFERRED_WORLD_SESSION
    );
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status ==
                LegacyWorldStoryVmStatus::world_session_load_failed &&
            load_failure_result.direct_audio_service_count == 0U &&
            load_failure.state.deferred_map_tile_x == 0x1234 &&
            load_failure.state.deferred_map_tile_y == 0x5678 &&
            load_failure.state.deferred_map_id == 21 &&
            load_failure.ports.world_session_reload_begin_count == 1U &&
            load_failure.ports.world_session_reload_count == 1U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == 0x66U &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{6U, 7U},
        "opcode 156 checked reload failure preserves deferred state but not IP, previous, or audio"
    );

    Fixture exact_tail;
    exact_tail.runtime.role_surface.selected_guid = 0x00F8U;
    exact_tail.roles[0].action.action_id = 0x12345U;
    exact_tail.state.deferred_map_tile_x = 0x1234;
    exact_tail.state.deferred_map_tile_y = 0x5678;
    exact_tail.state.deferred_map_id = 21;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_156_RELOAD_DEFERRED_WORLD_SESSION
    );

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.state.deferred_map_tile_x == -1 &&
            exact_tail.state.deferred_map_tile_y == -1 &&
            exact_tail.state.deferred_map_id == 0 &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_156_RELOAD_DEFERRED_WORLD_SESSION,
        "opcode 156 completes reload and deferred clearing before previous, audio maintenance, and yield at the exact window tail"
    );
}

void test_configure_deferred_world_session_protocol(
    openswd3::test::Context& test
) {
    struct SignedCase {
        u16 map_id;
        u16 tile_x;
        u16 tile_y;
        i32 expected_map_id;
        i32 expected_tile_x;
        i32 expected_tile_y;
    };
    constexpr std::array<SignedCase, 4U> cases{
        SignedCase{0x0000U, 0x8000U, 0xFFFFU, 0, -32768, -1},
        SignedCase{0x7FFFU, 0x0000U, 0x0001U, 32767, 0, 1},
        SignedCase{0x8000U, 0x7FFFU, 0x8000U, -32768, 32767, -32768},
        SignedCase{0xFFFFU, 0xFFFFU, 0x7FFFU, -1, -1, 32767},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (std::size_t index = 0U; index < cases.size(); ++index) {
        Fixture fixture;
        const auto& value = cases[index];
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_157_CONFIGURE_DEFERRED_WORLD_SESSION | alias_masks[index]
            )
        );
        write_u16(fixture.state.window, 2U, value.map_id);
        write_u16(fixture.state.window, 4U, value.tile_x);
        write_u16(fixture.state.window, 6U, value.tile_y);
        write_u16(fixture.state.window, 8U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.state.deferred_map_id == value.expected_map_id &&
                fixture.state.deferred_map_tile_x == value.expected_tile_x &&
                fixture.state.deferred_map_tile_y == value.expected_tile_y &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_157_CONFIGURE_DEFERRED_WORLD_SESSION &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 157 aliases sign-extend all three deferred words, publish previous, and continue in the same call"
        );
    }

    Fixture map_twenty_two;
    map_twenty_two.state.deferred_map_id = 111;
    map_twenty_two.state.deferred_map_tile_x = 222;
    map_twenty_two.state.deferred_map_tile_y = 333;
    map_twenty_two.context.instruction_offset = 0x7FFCU;
    map_twenty_two.context.talk_data_offset = 0x1111U;
    map_twenty_two.state.loaded_file_number = 1U;
    map_twenty_two.state.loaded_data_offset = 0x1111U;
    map_twenty_two.state.window_loaded = true;
    map_twenty_two.state.previous_opcode = 0x66U;
    write_u16(
        map_twenty_two.state.window,
        0x7FFCU,
        OP_157_CONFIGURE_DEFERRED_WORLD_SESSION
    );
    write_u16(map_twenty_two.state.window, 0x7FFEU, 22U);

    const auto map_twenty_two_result = map_twenty_two.step();
    test.expect_true(
        map_twenty_two_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            map_twenty_two_result.executed_instruction_count == 1U &&
            map_twenty_two_result.direct_audio_service_count == 0U &&
            map_twenty_two.state.deferred_map_id == 111 &&
            map_twenty_two.state.deferred_map_tile_x == 222 &&
            map_twenty_two.state.deferred_map_tile_y == 333 &&
            map_twenty_two.context.instruction_offset == 0x8004U &&
            map_twenty_two.state.previous_opcode ==
                OP_157_CONFIGURE_DEFERRED_WORLD_SESSION &&
            map_twenty_two.ports.story_protocol_events.empty(),
        "opcode 157 map 22 preserves all deferred state, does not read tile operands, advances eight, and same-calls the next fetch"
    );

    Fixture missing_map;
    missing_map.state.deferred_map_id = 111;
    missing_map.state.deferred_map_tile_x = 222;
    missing_map.state.deferred_map_tile_y = 333;
    missing_map.context.instruction_offset = 0x7FFEU;
    missing_map.context.talk_data_offset = 0x1111U;
    missing_map.state.loaded_file_number = 1U;
    missing_map.state.loaded_data_offset = 0x1111U;
    missing_map.state.window_loaded = true;
    missing_map.state.previous_opcode = 0x66U;
    write_u16(
        missing_map.state.window,
        0x7FFEU,
        OP_157_CONFIGURE_DEFERRED_WORLD_SESSION
    );

    const auto missing_map_result = missing_map.step();
    test.expect_true(
        missing_map_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_map.state.deferred_map_id == 111 &&
            missing_map.state.deferred_map_tile_x == 222 &&
            missing_map.state.deferred_map_tile_y == 333 &&
            missing_map.context.instruction_offset == 0x7FFEU &&
            missing_map.state.previous_opcode == 0x66U,
        "opcode 157 missing map stops before the first deferred write"
    );

    Fixture missing_x;
    missing_x.state.deferred_map_id = 111;
    missing_x.state.deferred_map_tile_x = 222;
    missing_x.state.deferred_map_tile_y = 333;
    missing_x.context.instruction_offset = 0x7FFCU;
    missing_x.context.talk_data_offset = 0x1111U;
    missing_x.state.loaded_file_number = 1U;
    missing_x.state.loaded_data_offset = 0x1111U;
    missing_x.state.window_loaded = true;
    missing_x.state.previous_opcode = 0x66U;
    write_u16(
        missing_x.state.window, 0x7FFCU, OP_157_CONFIGURE_DEFERRED_WORLD_SESSION
    );
    write_u16(missing_x.state.window, 0x7FFEU, 0x8000U);

    const auto missing_x_result = missing_x.step();
    test.expect_true(
        missing_x_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_x.state.deferred_map_id == -32768 &&
            missing_x.state.deferred_map_tile_x == 222 &&
            missing_x.state.deferred_map_tile_y == 333 &&
            missing_x.context.instruction_offset == 0x7FFCU &&
            missing_x.state.previous_opcode == 0x66U,
        "opcode 157 missing X preserves the committed signed map and all older coordinates"
    );

    Fixture missing_y;
    missing_y.state.deferred_map_id = 111;
    missing_y.state.deferred_map_tile_x = 222;
    missing_y.state.deferred_map_tile_y = 333;
    missing_y.context.instruction_offset = 0x7FFAU;
    missing_y.context.talk_data_offset = 0x1111U;
    missing_y.state.loaded_file_number = 1U;
    missing_y.state.loaded_data_offset = 0x1111U;
    missing_y.state.window_loaded = true;
    missing_y.state.previous_opcode = 0x66U;
    write_u16(
        missing_y.state.window, 0x7FFAU, OP_157_CONFIGURE_DEFERRED_WORLD_SESSION
    );
    write_u16(missing_y.state.window, 0x7FFCU, 0x7FFFU);
    write_u16(missing_y.state.window, 0x7FFEU, 0xFFFFU);

    const auto missing_y_result = missing_y.step();
    test.expect_true(
        missing_y_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_y.state.deferred_map_id == 32767 &&
            missing_y.state.deferred_map_tile_x == -1 &&
            missing_y.state.deferred_map_tile_y == 333 &&
            missing_y.context.instruction_offset == 0x7FFAU &&
            missing_y.state.previous_opcode == 0x66U,
        "opcode 157 missing Y preserves the committed signed map and X"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF8U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FF8U,
        OP_157_CONFIGURE_DEFERRED_WORLD_SESSION
    );
    write_u16(exact_tail.state.window, 0x7FFAU, 0x7FFFU);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x8000U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFFFU);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.state.deferred_map_id == 32767 &&
            exact_tail.state.deferred_map_tile_x == -32768 &&
            exact_tail.state.deferred_map_tile_y == -1 &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_157_CONFIGURE_DEFERRED_WORLD_SESSION &&
            exact_tail.ports.story_protocol_events.empty(),
        "opcode 157 commits all signed fields, publishes previous, and same-calls the next fetch at the exact window tail"
    );
}

void test_story_file_operations_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        LegacyWorldStoryFileOperation operation;
        LegacyWorldStoryFileDirectory directory;
        u16 own_prefix;
        u16 lookahead_directory;
        std::string_view filename;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{
            OP_158_COPY_STORY_FILE,
            LegacyWorldStoryFileOperation::copy,
            LegacyWorldStoryFileDirectory::video,
            1U,
            0U,
            "clip.bin",
        },
        Variant{
            OP_159_DELETE_STORY_FILE,
            LegacyWorldStoryFileOperation::remove,
            LegacyWorldStoryFileDirectory::music,
            0U,
            1U,
            "song.ogg",
        },
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const auto& variant : variants) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            fixture.state.previous_opcode = 0x66U;
            fixture.ports.story_file_operation_success = false;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias_mask)
            );
            write_u16(fixture.state.window, 2U, variant.own_prefix);
            std::ranges::copy(
                variant.filename, fixture.state.window.begin() + 4U
            );
            const std::size_t terminator = 4U + variant.filename.size();
            fixture.state.window[terminator] = static_cast<u8>('%');
            fixture.state.window[terminator + 1U] = static_cast<u8>('Q');
            const std::size_t next_instruction = terminator + 2U;
            write_u16(
                fixture.state.window,
                next_instruction,
                kStoryVmLookaheadTypedStop
            );
            write_u16(
                fixture.state.window,
                next_instruction + 2U,
                variant.lookahead_directory
            );
            bool suspend_order = false;
            fixture.ports.story_host_frame_suspend_callback = [&]() {
                suspend_order = fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode == 0x66U;
            };
            bool operation_order = false;
            fixture.ports.story_file_operation_callback = [&]() {
                operation_order =
                    fixture.context.instruction_offset == next_instruction &&
                    fixture.state.previous_opcode == 0x66U &&
                    fixture.ports.story_host_frame_suspend_count == 1U;
            };

            const auto result = fixture.step();
            const std::string actual_filename{
                fixture.ports.last_story_file_name.begin(),
                fixture.ports.last_story_file_name.end(),
            };
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            frame_deformation_injection_failed &&
                    result.opcode == kStoryVmLookaheadTypedStop &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 0U &&
                    fixture.ports.story_host_frame_suspend_count == 1U &&
                    fixture.ports.story_file_operation_count == 1U &&
                    fixture.ports.last_story_file_operation ==
                        variant.operation &&
                    fixture.ports.last_story_file_directory ==
                        variant.directory &&
                    actual_filename == variant.filename && suspend_order &&
                    operation_order &&
                    fixture.context.instruction_offset == next_instruction &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.story_protocol_events.empty(),
                "opcodes 158 and 159 aliases ignore their own prefix, select the directory from the next instruction operand, ignore file API failure, publish previous, and same-call"
            );
        }
    }

    Fixture embedded_nul;
    embedded_nul.state.previous_opcode = 0x66U;
    prime_loaded_instruction(embedded_nul, OP_158_COPY_STORY_FILE);
    write_u16(embedded_nul.state.window, 2U, 0U);
    embedded_nul.state.window[4U] = static_cast<u8>('A');
    embedded_nul.state.window[5U] = 0U;
    embedded_nul.state.window[6U] = static_cast<u8>('B');
    embedded_nul.state.window[7U] = static_cast<u8>('%');
    embedded_nul.state.window[8U] = static_cast<u8>('Q');
    write_u16(embedded_nul.state.window, 9U, kStoryVmLookaheadTypedStop);
    write_u16(embedded_nul.state.window, 11U, 0xFFFFU);

    const auto embedded_nul_result = embedded_nul.step();
    test.expect_true(
        embedded_nul_result.status ==
                LegacyWorldStoryVmStatus::frame_deformation_injection_failed &&
            embedded_nul.ports.story_host_frame_suspend_count == 1U &&
            embedded_nul.ports.story_file_operation_count == 1U &&
            embedded_nul.ports.last_story_file_directory ==
                LegacyWorldStoryFileDirectory::root &&
            embedded_nul.ports.last_story_file_name ==
                std::vector<u8>{static_cast<u8>('A')} &&
            embedded_nul.context.instruction_offset == 9U &&
            embedded_nul.state.previous_opcode == OP_158_COPY_STORY_FILE,
        "opcode 158 scans through embedded NUL to percent-Q but passes only the legacy C-string prefix to the file operation"
    );

    Fixture missing_terminator;
    missing_terminator.state.previous_opcode = 0x66U;
    prime_loaded_instruction(missing_terminator, OP_159_DELETE_STORY_FILE);
    write_u16(missing_terminator.state.window, 2U, 0xABCDU);
    const auto missing_terminator_result = missing_terminator.step();
    test.expect_true(
        missing_terminator_result.status ==
                LegacyWorldStoryVmStatus::story_filename_terminator_not_found &&
            missing_terminator.ports.story_host_frame_suspend_count == 0U &&
            missing_terminator.ports.story_file_operation_count == 0U &&
            missing_terminator.context.instruction_offset == 0U &&
            missing_terminator.state.previous_opcode == 0x66U,
        "opcode 159 missing percent-Q stops before IP, previous, and file operation"
    );

    Fixture missing_lookahead;
    missing_lookahead.context.instruction_offset = 0x7FF8U;
    missing_lookahead.context.talk_data_offset = 0x1111U;
    missing_lookahead.state.loaded_file_number = 1U;
    missing_lookahead.state.loaded_data_offset = 0x1111U;
    missing_lookahead.state.window_loaded = true;
    missing_lookahead.state.previous_opcode = 0x66U;
    write_u16(missing_lookahead.state.window, 0x7FF8U, OP_158_COPY_STORY_FILE);
    write_u16(missing_lookahead.state.window, 0x7FFAU, 0xFFFFU);
    missing_lookahead.state.window[0x7FFCU] = static_cast<u8>('%');
    missing_lookahead.state.window[0x7FFDU] = static_cast<u8>('Q');

    const auto missing_lookahead_result = missing_lookahead.step();
    test.expect_true(
        missing_lookahead_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_lookahead.ports.story_host_frame_suspend_count == 1U &&
            missing_lookahead.ports.story_file_operation_count == 0U &&
            missing_lookahead.context.instruction_offset == 0x7FFEU &&
            missing_lookahead.state.previous_opcode == 0x66U,
        "opcode 158 consumes percent-Q before the next-instruction directory lookahead fails"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FF6U, OP_159_DELETE_STORY_FILE);
    write_u16(exact_tail.state.window, 0x7FF8U, 0x1234U);
    exact_tail.state.window[0x7FFAU] = static_cast<u8>('%');
    exact_tail.state.window[0x7FFBU] = static_cast<u8>('Q');
    write_u16(exact_tail.state.window, 0x7FFCU, kStoryVmLookaheadTypedStop);
    write_u16(exact_tail.state.window, 0x7FFEU, 2U);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            exact_tail_result.opcode == kStoryVmLookaheadTypedStop &&
            exact_tail_result.executed_instruction_count == 2U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.ports.story_host_frame_suspend_count == 1U &&
            exact_tail.ports.story_file_operation_count == 1U &&
            exact_tail.ports.last_story_file_operation ==
                LegacyWorldStoryFileOperation::remove &&
            exact_tail.ports.last_story_file_directory ==
                LegacyWorldStoryFileDirectory::root &&
            exact_tail.ports.last_story_file_name.empty() &&
            exact_tail.context.instruction_offset == 0x7FFCU &&
            exact_tail.state.previous_opcode == OP_159_DELETE_STORY_FILE &&
            exact_tail.ports.story_protocol_events.empty(),
        "opcode 159 reads its next-instruction directory selector from the final window word, performs the delete, publishes previous, and same-calls the successor"
    );
}

void test_suppress_next_dialog_flag18_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.state.next_dialog_flag18_suppression = 0xA5A5A5A5U;
        fixture.state.previous_opcode = 0x66U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_160_SUPPRESS_NEXT_DIALOG_FLAG18 | alias_mask)
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
                fixture.state.next_dialog_flag18_suppression == 1U &&
                fixture.state.previous_opcode ==
                    OP_160_SUPPRESS_NEXT_DIALOG_FLAG18 &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 160 aliases overwrite the full one-shot value, publish previous, and same-call the successor"
        );
    }

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.next_dialog_flag18_suppression = 2U;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_160_SUPPRESS_NEXT_DIALOG_FLAG18
    );

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.opcode == OP_160_SUPPRESS_NEXT_DIALOG_FLAG18 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.next_dialog_flag18_suppression == 1U &&
            exact_tail.state.previous_opcode ==
                OP_160_SUPPRESS_NEXT_DIALOG_FLAG18 &&
            exact_tail.ports.story_protocol_events.empty(),
        "opcode 160 commits the one-shot value, IP and previous before the exact-tail next fetch fails"
    );

    Fixture dialog;
    dialog.state.previous_opcode = 0x66U;
    prime_loaded_instruction(dialog, OP_160_SUPPRESS_NEXT_DIALOG_FLAG18);
    write_u16(dialog.state.window, 2U, 2U);
    write_u16(dialog.state.window, 4U, 0U);
    write_u16(dialog.state.window, 6U, 0x232DU);
    dialog.state.window[8U] = static_cast<u8>('%');
    dialog.state.window[9U] = static_cast<u8>('Q');

    const auto dialog_result = dialog.step();
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.opcode == 2U &&
            dialog_result.executed_instruction_count == 2U &&
            dialog_result.dialog_enqueue_count == 1U &&
            dialog_result.direct_audio_service_count == 2U &&
            dialog.context.instruction_offset == 10U &&
            dialog.dialogs.messages.size() == 1U &&
            (dialog.dialogs.messages.front().record.flags & 0x00040000U) ==
                0U &&
            dialog.state.next_dialog_flag18_suppression == 0U &&
            dialog.state.previous_opcode == 2U,
        "opcode 160 same-calls a dialog that suppresses flag bit 18 once and clears the one-shot value after queueing"
    );

    Fixture failed_dialog;
    failed_dialog.state.previous_opcode = 0x66U;
    prime_loaded_instruction(failed_dialog, OP_160_SUPPRESS_NEXT_DIALOG_FLAG18);
    write_u16(failed_dialog.state.window, 2U, 2U);
    write_u16(failed_dialog.state.window, 4U, 0U);
    write_u16(failed_dialog.state.window, 6U, 0x232DU);

    const auto failed_dialog_result = failed_dialog.step();
    test.expect_true(
        failed_dialog_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            failed_dialog_result.opcode == 2U &&
            failed_dialog_result.executed_instruction_count == 2U &&
            failed_dialog_result.direct_audio_service_count == 1U &&
            failed_dialog.context.instruction_offset == 2U &&
            failed_dialog.dialogs.messages.empty() &&
            failed_dialog.state.next_dialog_flag18_suppression == 1U &&
            failed_dialog.state.previous_opcode ==
                OP_160_SUPPRESS_NEXT_DIALOG_FLAG18,
        "a failed dialog after opcode 160 preserves the one-shot value and opcode 160 previous publication"
    );
}

void test_wait_picture_action_byte_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        bool primary;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE, true},
        Variant{OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE, false},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const auto variant : variants) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            openswd3::world_map::LegacyPictureActionLists picture_actions;
            picture_actions.primary.emplace_back();
            picture_actions.secondary.emplace_back();
            auto& selected = variant.primary ? picture_actions.primary
                                             : picture_actions.secondary;
            auto& other = variant.primary ? picture_actions.secondary
                                          : picture_actions.primary;
            selected.front().action.packed_ap_state = 0x0500U;
            other.front().action.packed_ap_state = 0x0A00U;
            fixture.runtime.picture_actions = &picture_actions;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias_mask)
            );
            write_u16(fixture.state.window, 2U, 5U);
            write_u16(fixture.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1);
            u8 scene_render_flags = 0xA7U;
            fixture.runtime.scene_render_flags = &scene_render_flags;

            const auto waiting = fixture.step();
            test.expect_true(
                waiting.status == LegacyWorldStoryVmStatus::yielded &&
                    waiting.raw_word ==
                        static_cast<u16>(variant.opcode | alias_mask) &&
                    waiting.opcode == variant.opcode &&
                    waiting.executed_instruction_count == 1U &&
                    waiting.direct_audio_service_count == 1U &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode == variant.opcode &&
                    scene_render_flags == 0xA7U,
                "shared picture-action wait aliases select their own head and stall at equality"
            );

            selected.front().action.packed_ap_state = 0x0600U;
            const auto completed = fixture.step();
            test.expect_true(
                completed.status == LegacyWorldStoryVmStatus::yielded &&
                    completed.opcode == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                    completed.executed_instruction_count == 2U &&
                    fixture.context.instruction_offset == 6U &&
                    fixture.state.previous_opcode ==
                        OP_95_CLEAR_SCENE_RENDER_BIT1 &&
                    scene_render_flags == 0xA5U,
                "shared picture-action wait advances only when the selected byte is strictly above"
            );
        }
    }

    for (const auto variant : variants) {
        Fixture wide_threshold;
        openswd3::world_map::LegacyPictureActionLists picture_actions;
        auto& selected = variant.primary ? picture_actions.primary
                                         : picture_actions.secondary;
        selected.emplace_back();
        selected.front().action.packed_ap_state = 0xFF00U;
        wide_threshold.runtime.picture_actions = &picture_actions;
        prime_loaded_instruction(wide_threshold, variant.opcode);
        write_u16(wide_threshold.state.window, 2U, 256U);

        const auto result = wide_threshold.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                wide_threshold.context.instruction_offset == 0U &&
                wide_threshold.state.previous_opcode == variant.opcode,
            "picture-action wait cannot complete above a u16 threshold greater than byte range"
        );

        Fixture empty_selected;
        openswd3::world_map::LegacyPictureActionLists empty_actions;
        auto& other =
            variant.primary ? empty_actions.secondary : empty_actions.primary;
        other.emplace_back();
        other.front().action.packed_ap_state = 0U;
        empty_selected.runtime.picture_actions = &empty_actions;
        prime_loaded_instruction(empty_selected, variant.opcode);
        write_u16(empty_selected.state.window, 2U, 0U);
        write_u16(
            empty_selected.state.window, 4U, OP_95_CLEAR_SCENE_RENDER_BIT1
        );
        u8 scene_render_flags = 0xA7U;
        empty_selected.runtime.scene_render_flags = &scene_render_flags;

        const auto empty_result = empty_selected.step();
        test.expect_true(
            empty_result.status == LegacyWorldStoryVmStatus::yielded &&
                empty_result.executed_instruction_count == 2U &&
                empty_selected.context.instruction_offset == 6U &&
                scene_render_flags == 0xA5U,
            "picture-action wait completes on an empty selected chain without reading the other chain"
        );
    }

    Fixture missing_operand;
    missing_operand.context.talk_data_offset = 0x1111U;
    missing_operand.context.instruction_offset = 0x7FFEU;
    missing_operand.state.loaded_file_number = 1U;
    missing_operand.state.loaded_data_offset = 0x1111U;
    missing_operand.state.window_loaded = true;
    missing_operand.state.previous_opcode = 0x55U;
    write_u16(
        missing_operand.state.window,
        0x7FFEU,
        OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE
    );

    const auto missing_operand_result = missing_operand.step();
    test.expect_true(
        missing_operand_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_operand.context.instruction_offset == 0x7FFEU &&
            missing_operand.state.previous_opcode == 0x55U,
        "picture-action wait reads its threshold before selecting the runtime owner"
    );

    Fixture missing_runtime;
    prime_loaded_instruction(
        missing_runtime, OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE
    );
    write_u16(missing_runtime.state.window, 2U, 5U);
    missing_runtime.state.previous_opcode = 0x66U;

    const auto missing_runtime_result = missing_runtime.step();
    test.expect_true(
        missing_runtime_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_runtime.context.instruction_offset == 0U &&
            missing_runtime.state.previous_opcode == 0x66U,
        "picture-action wait stops at the typed parent owner when runtime binding is absent"
    );

    Fixture exact_tail;
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    exact_tail.runtime.picture_actions = &picture_actions;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    write_u16(
        exact_tail.state.window,
        0x7FFCU,
        OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE
    );
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFFFU);

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE,
        "empty secondary picture-action wait commits IP and previous before exact-tail fetch failure"
    );
}

void test_enqueue_moving_action_protocol(openswd3::test::Context& test) {
    const auto write_record = [](const std::span<u8> bytes,
                                 const std::size_t offset,
                                 const u16 raw_opcode) {
        write_u16(bytes, offset, raw_opcode);
        write_u16(bytes, offset + 2U, 0x1234U);
        write_u16(bytes, offset + 4U, 2U);
        write_u16(bytes, offset + 6U, 1U);
        write_u16(bytes, offset + 8U, 2U);
        write_u16(bytes, offset + 10U, 4U);
        write_u16(bytes, offset + 12U, 6U);
        write_u16(bytes, offset + 14U, 5U);
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture exact_tail;
        openswd3::world_map::LegacyMovingActionList moving_actions;
        exact_tail.runtime.moving_actions = &moving_actions;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FF0U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_record(
            exact_tail.state.window,
            0x7FF0U,
            static_cast<u16>(OP_79_ENQUEUE_MOVING_ACTION | alias_mask)
        );

        const auto exact_result = exact_tail.step();
        const auto& node = moving_actions.front();

        test.expect_true(
            exact_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_result.opcode == OP_79_ENQUEUE_MOVING_ACTION &&
                exact_result.executed_instruction_count == 1U &&
                moving_actions.size() == 1U &&
                node.action.action_id == 0x1234U &&
                node.action.base_variant == 2U &&
                node.action.variant_delta == 0U && node.start_x == 16 &&
                node.start_y == 32 && node.target_x == 64 &&
                node.target_y == 96 && node.velocity_x == 3.0F &&
                node.velocity_y == 4.0F && node.position_x == 16.0F &&
                node.position_y == 32.0F && node.next_pointer_32 == 0U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == OP_79_ENQUEUE_MOVING_ACTION,
            "opcode 79 aliases initialize and prepend the moving action before exact-tail next fetch failure"
        );
    }

    Fixture ordinary;
    openswd3::world_map::LegacyMovingActionList moving_actions;
    moving_actions.emplace_back();
    moving_actions.back().action.action_id = 0x9999U;
    ordinary.runtime.moving_actions = &moving_actions;
    auto script = std::span<u8>{ordinary.ports.initial_window};
    write_record(script, 0U, OP_79_ENQUEUE_MOVING_ACTION);
    write_u16(script, 16U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 18U, 0x00F8U);

    const auto ordinary_result = ordinary.step();

    test.expect_true(
        ordinary_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ordinary_result.executed_instruction_count == 2U &&
            moving_actions.size() == 2U &&
            moving_actions.front().action.action_id == 0x1234U &&
            moving_actions.back().action.action_id == 0x9999U &&
            ordinary.context.instruction_offset == 20U &&
            ordinary.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 79 prepends to the real list and continues in the same call"
    );
}

void test_enqueue_moving_action_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u16, 7U> operands{
        0x1234U,
        2U,
        1U,
        2U,
        4U,
        6U,
        5U,
    };
    for (std::size_t available = 0U; available < operands.size(); ++available) {
        Fixture truncated;
        openswd3::world_map::LegacyMovingActionList moving_actions;
        truncated.runtime.moving_actions = &moving_actions;
        const u16 offset =
            static_cast<u16>(0x8000U - (available + 1U) * sizeof(u16));
        truncated.context.talk_data_offset = 0x1111U;
        truncated.context.instruction_offset = offset;
        truncated.state.loaded_file_number = 1U;
        truncated.state.loaded_data_offset = 0x1111U;
        truncated.state.window_loaded = true;
        truncated.state.previous_opcode = 0x66U;
        write_u16(truncated.state.window, offset, OP_79_ENQUEUE_MOVING_ACTION);
        for (std::size_t index = 0U; index < available; ++index) {
            write_u16(
                truncated.state.window,
                static_cast<std::size_t>(offset) + 2U + index * 2U,
                operands[index]
            );
        }

        const auto truncated_result = truncated.step();

        test.expect_true(
            truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                moving_actions.empty() &&
                truncated.context.instruction_offset == offset &&
                truncated.state.previous_opcode == 0x66U,
            "opcode 79 truncations release the temporary unlinked node and preserve IP/previous"
        );
    }

    Fixture unavailable;
    auto unavailable_script = std::span<u8>{unavailable.ports.initial_window};
    write_u16(unavailable_script, 0U, OP_79_ENQUEUE_MOVING_ACTION);
    for (std::size_t index = 0U; index < operands.size(); ++index) {
        write_u16(unavailable_script, 2U + index * 2U, operands[index]);
    }
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 79 owner absence occurs after full node initialization but before list insertion"
    );

    const auto run_exact = [&](const u16 start_x,
                               const u16 start_y,
                               const u16 target_x,
                               const u16 target_y,
                               const u16 movement) {
        Fixture fixture;
        openswd3::world_map::LegacyMovingActionList moving_actions;
        fixture.runtime.moving_actions = &moving_actions;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FF0U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        write_u16(fixture.state.window, 0x7FF0U, OP_79_ENQUEUE_MOVING_ACTION);
        write_u16(fixture.state.window, 0x7FF2U, 1U);
        write_u16(fixture.state.window, 0x7FF4U, 2U);
        write_u16(fixture.state.window, 0x7FF6U, start_x);
        write_u16(fixture.state.window, 0x7FF8U, start_y);
        write_u16(fixture.state.window, 0x7FFAU, target_x);
        write_u16(fixture.state.window, 0x7FFCU, target_y);
        write_u16(fixture.state.window, 0x7FFEU, movement);
        const auto result = fixture.step();
        return std::tuple{
            result,
            moving_actions.front().velocity_x,
            moving_actions.front().velocity_y,
        };
    };

    const auto [zero_result, zero_velocity_x, zero_velocity_y] =
        run_exact(1U, 2U, 1U, 2U, 5U);
    const auto [negative_result, negative_velocity_x, negative_velocity_y] =
        run_exact(1U, 2U, 4U, 6U, 0xFFFBU);
    const auto [overflow_result, overflow_velocity_x, overflow_velocity_y] =
        run_exact(0x0800U, 0U, 0x07FFU, 0U, 5U);
    test.expect_true(
        zero_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            std::isnan(zero_velocity_x) && std::isnan(zero_velocity_y) &&
            negative_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            negative_velocity_x == -3.0F && negative_velocity_y == -4.0F &&
            overflow_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            std::isnan(overflow_velocity_x) && std::isnan(overflow_velocity_y),
        "opcode 79 preserves x87 zero-distance NaN, signed movement and wrapping 32-bit squared-distance behavior"
    );
}

void test_real_clear_dialog_control_flag_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004518);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 7U,
        "real opcode 7 physical record is a two-byte instruction"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 7U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
    const auto result = fixture.step();
    test.expect_true(
        result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.text_control_flags == 0x7FFFFFFFU &&
            fixture.state.previous_opcode == 7U,
        "real opcode 7 record replays the clear-and-continue contract"
    );
}

void test_real_change_role_base_variant_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A24);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 10U &&
            read_u16(instruction, 2U) == 1U && read_u16(instruction, 4U) == 0U,
        "real opcode 10 physical record has the expected six-byte payload"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 10U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
    fixture.roles[1].guid = 1U;
    fixture.roles[1].action.base_variant = 77U;
    fixture.roles[1].action.wait_remaining = 9U;
    const auto result = fixture.step();
    test.expect_true(
        result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            result.action_update_count == 1U &&
            fixture.state.previous_opcode == 10U,
        "real opcode 10 record replays the live-role update contract"
    );
}

void test_real_change_role_variant_delta_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A2E);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 11U &&
            read_u16(instruction, 2U) == 1U && read_u16(instruction, 4U) == 0U,
        "real opcode 11 physical record has the expected six-byte payload"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 11U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
    fixture.roles[1].guid = 1U;
    fixture.roles[1].flags = 1U;
    fixture.roles[1].action.variant_delta = 77U;
    fixture.roles[1].action.wait_remaining = 9U;
    const auto result = fixture.step();
    test.expect_true(
        result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U &&
            fixture.roles[1].action.variant_delta == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            fixture.roles[1].flags == 0x00001001U &&
            result.action_update_count == 1U &&
            fixture.state.previous_opcode == 11U,
        "real opcode 11 record replays the live-role update contract"
    );
}

void test_real_clear_dialog_control_flag_bit30_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000451E);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 9U,
        "real opcode 9 physical record is a two-byte instruction"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 9U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
    const auto result = fixture.step();
    test.expect_true(
        result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.text_control_flags == 0xBFFFFFFFU &&
            fixture.state.previous_opcode == 9U,
        "real opcode 9 record replays the bit-30 clear contract"
    );
}

void test_real_stage_dialog_lifetime_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000451A);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 8U &&
            read_u16(instruction, 2U) == 0xFFFFU,
        "real opcode 8 physical record carries an unsigned FFFF word"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 8U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
    const auto result = fixture.step();
    test.expect_true(
        result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.next_text_aux_pending &&
            fixture.state.next_text_aux_value == 0xFFFFU &&
            fixture.state.previous_opcode == 8U,
        "real opcode 8 record replays the staged-lifetime contract"
    );
}

void test_real_wait_role_action_status_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000471F);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_14_WAIT_ROLE_ACTION_STATUS &&
            read_u16(instruction, 2U) == 0x00F8U,
        "real opcode 14 record is a four-byte wait for role F8"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_14_WAIT_ROLE_ACTION_STATUS);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].interaction_gate = 1U;
    const auto waiting = fixture.step();
    const u16 waiting_offset = fixture.context.instruction_offset;
    fixture.roles[1].interaction_gate = 0U;
    const auto completed = fixture.step();
    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            waiting.direct_audio_service_count == 1U && waiting_offset == 0U &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            completed.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "real opcode 14 record replays wait, audio service, and completion"
    );
}

void test_real_continue_common_join_same_call_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::string_view file;
        std::streamoff offset;
    };
    constexpr std::array samples{
        Sample{"TALK1.DAT", 0x000014C8U},
        Sample{"TALK1.DAT", 0x00011FCDU},
        Sample{"TALK1.DAT", 0x0005AAF2U},
        Sample{"TALK2.DAT", 0x00000CF4U},
        Sample{"TALK2.DAT", 0x0003305EU},
        Sample{"TALK2.DAT", 0x00033078U},
        Sample{"TALK2.DAT", 0x000330D2U},
        Sample{"TALK2.DAT", 0x0003312EU},
        Sample{"TALK3.DAT", 0x000021BEU},
        Sample{"TALK3.DAT", 0x0001D882U},
        Sample{"TALK3.DAT", 0x0001D884U},
        Sample{"TALK3.DAT", 0x00033490U},
        Sample{"TALK4.DAT", 0x00001334U},
        Sample{"TALK4.DAT", 0x000364E4U},
        Sample{"TALK4.DAT", 0x000364FEU},
        Sample{"TALK4.DAT", 0x00036558U},
        Sample{"TALK4.DAT", 0x000365B4U},
    };

    bool all_records_valid = true;
    for (const auto sample : samples) {
        std::ifstream input{
            root / sample.file, std::ios::binary | std::ios::in
        };
        std::array<u8, 2U> instruction{};
        input.seekg(sample.offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        all_records_valid = all_records_valid && static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL;
    }

    test.expect_true(
        all_records_valid,
        "real opcode 1026 boundary and multi-probe records in all four TALK files retain the raw 0402 word"
    );
}

void test_real_story_transfer_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00007505);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 1U;
    state.loaded_data_offset = 0x00007305U;
    state.window_loaded = true;
    state.previous_opcode = 0x66U;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00F8U;
    context.talk_script_id = 248U;
    context.talk_data_offset = 0x00007305U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_161_TRANSFER_STORY &&
            read_u16(instruction, 2U) == 2037U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            result.executed_instruction_count == 3U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 5U &&
            context.talk_data_offset == 0x00006CE9U &&
            context.instruction_offset == 6U &&
            state.loaded_file_number == 2U &&
            state.loaded_data_offset == 0x00006CE9U &&
            state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(state.window, 0U) ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
            read_u16(state.window, 2U) == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(state.window, 4U) == 193U &&
            ports.sound_effect_requests == std::vector<u16>{193U},
        "real opcode 161 transfers from TALK1 to TALK2 and same-calls the 1026 prefix and sound request"
    );
}

void test_real_current_map_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        u32 file_number;
        std::streamoff file_offset;
        u16 opcode;
    };
    constexpr std::array<Sample, 27U> samples{
        Sample{1U, 0x0000BCCDU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{1U, 0x0000BD7FU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{1U, 0x0000C761U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{1U, 0x0001BEA3U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{1U, 0x00038DDDU, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL},
        Sample{1U, 0x0003EDE3U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{1U, 0x0003EE5FU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{2U, 0x00031AA8U, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL},
        Sample{3U, 0x00002616U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x00003028U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x000030A2U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x00003122U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x00003196U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x0000320AU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x0000327EU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x00003322U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x000033C6U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x0000662CU, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL},
        Sample{3U, 0x000066A0U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x0001CD77U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x0001CE0DU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{3U, 0x000324DCU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{4U, 0x00026FD9U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{4U, 0x00026FE1U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{4U, 0x0002708BU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{4U, 0x000270FFU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
        Sample{4U, 0x00027173U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL},
    };
    constexpr std::array<std::string_view, 4U> talk_files{
        "TALK1.DAT",
        "TALK2.DAT",
        "TALK3.DAT",
        "TALK4.DAT",
    };

    bool all_records_valid = true;
    u32 opcode_163_count = 0U;
    u32 opcode_164_count = 0U;
    for (const auto sample : samples) {
        std::ifstream input{
            root / talk_files[sample.file_number - 1U],
            std::ios::binary | std::ios::in,
        };
        std::array<u8, 8U> instruction{};
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        if (!input) {
            all_records_valid = false;
            continue;
        }

        const u32 target = read_u32(instruction, 4U);
        std::array<u8, 2U> target_instruction{};
        input.seekg(static_cast<std::streamoff>(target) + 0x200);
        input.read(
            reinterpret_cast<char*>(target_instruction.data()),
            static_cast<std::streamsize>(target_instruction.size())
        );
        all_records_valid = all_records_valid && static_cast<bool>(input) &&
            (read_u16(instruction, 0U) & 0x3FFFU) == sample.opcode &&
            (read_u16(target_instruction, 0U) & 0x3FFFU) ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL;
        opcode_163_count +=
            sample.opcode == OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL ? 1U : 0U;
        opcode_164_count +=
            sample.opcode == OP_164_RELOAD_IF_CURRENT_MAP_EQUAL ? 1U : 0U;
    }
    test.expect_true(
        all_records_valid && opcode_163_count == 3U && opcode_164_count == 24U,
        "all 27 real opcode 163 and 164 records have eight readable bytes and a valid same-file target beginning with opcode 1026"
    );

    struct Replay {
        std::streamoff file_offset;
        u16 opcode;
        u32 current_map_id;
    };
    constexpr std::array replays{
        Replay{0x00038DDDU, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL, 21U},
        Replay{0x0000BCCDU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL, 98U},
    };
    for (const auto replay : replays) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        std::array<u8, 8U> instruction{};
        input.seekg(replay.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        if (!input) {
            test.expect_true(
                false, "real opcode 163 or 164 record is readable"
            );
            continue;
        }

        Fixture fixture;
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.runtime.current_logical_map_id = replay.current_map_id;
        fixture.state.window[300U] = 0xA5U;
        write_u16(
            fixture.ports.transferred_window,
            0U,
            OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
        );
        write_u16(fixture.ports.transferred_window, 2U, kStoryVmTypedStop);
        const u32 target = read_u32(instruction, 4U);
        const i32 map_id = static_cast<i16>(read_u16(instruction, 2U));
        const bool map_matches =
            replay.current_map_id == std::bit_cast<u32>(map_id);

        const auto result = fixture.step();
        test.expect_true(
            (read_u16(instruction, 0U) & 0x3FFFU) == replay.opcode &&
                (replay.opcode == OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL
                     ? !map_matches
                     : map_matches) &&
                result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 3U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.last_data_file_number == 1U &&
                fixture.ports.last_data_offset == target &&
                !fixture.ports.last_data_clear_before_read &&
                fixture.context.talk_data_offset == target &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.loaded_data_offset == target &&
                fixture.state.window[300U] == 0xA5U &&
                fixture.state.previous_opcode ==
                    OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
            "real opcode 163 and 164 records take their inverted map predicates and same-call the loaded 1026 target"
        );
    }
}

void test_real_item_total_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        u16 item_id;
        u32 target;
        u16 target_opcode;
    };
    constexpr std::array samples{
        Sample{
            0x00004F16U,
            795U,
            0x00004D26U,
            OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST,
        },
        Sample{
            0x00005470U,
            798U,
            0x000052EFU,
            OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
        },
        Sample{
            0x000057B5U,
            798U,
            0x00005662U,
            OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
        },
    };

    bool all_records_valid = true;
    for (const auto sample : samples) {
        std::ifstream input{
            root / "TALK4.DAT", std::ios::binary | std::ios::in
        };
        std::array<u8, 10U> instruction{};
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        if (!input) {
            all_records_valid = false;
            continue;
        }

        std::array<u8, 2U> target_instruction{};
        input.seekg(static_cast<std::streamoff>(sample.target) + 0x200);
        input.read(
            reinterpret_cast<char*>(target_instruction.data()),
            static_cast<std::streamsize>(target_instruction.size())
        );
        all_records_valid = all_records_valid && static_cast<bool>(input) &&
            (read_u16(instruction, 0U) & 0x3FFFU) ==
                OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST &&
            read_u16(instruction, 2U) == sample.item_id &&
            std::bit_cast<i16>(read_u16(instruction, 4U)) == 1 &&
            read_u32(instruction, 6U) == sample.target &&
            (read_u16(target_instruction, 0U) & 0x3FFFU) ==
                sample.target_opcode;
    }
    test.expect_true(
        all_records_valid,
        "all three real opcode 165 records preserve item, signed threshold, same-file target, and target-opcode bytes"
    );

    std::ifstream input{root / "TALK4.DAT", std::ios::binary | std::ios::in};
    std::array<u8, 10U> instruction{};
    input.seekg(0x00005470U);
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_file_number = 4U;
    fixture.state.loaded_data_offset = 0x00005270U;
    fixture.context.talk_data_offset = 0x00005270U;
    auto& player_item = fixture.player_inventory.emplace_back();
    player_item.item_id = 0xC31EU;
    player_item.quantity_a = 1U;
    fixture.state.window[300U] = 0xA5U;
    write_u16(
        fixture.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(fixture.ports.transferred_window, 2U, kStoryVmTypedStop);

    const auto result = fixture.step();
    test.expect_true(
        instruction_read &&
            (read_u16(instruction, 0U) & 0x3FFFU) ==
                OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST &&
            read_u16(instruction, 2U) == 798U &&
            std::bit_cast<i16>(read_u16(instruction, 4U)) == 1 &&
            read_u32(instruction, 6U) == 0x000052EFU &&
            result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.opcode == kStoryVmTypedStop &&
            result.executed_instruction_count == 3U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 1U &&
            fixture.ports.data_load_count == 1U &&
            fixture.ports.last_data_file_number == 4U &&
            fixture.ports.last_data_offset == 0x000052EFU &&
            !fixture.ports.last_data_clear_before_read &&
            fixture.context.talk_data_offset == 0x000052EFU &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.loaded_file_number == 4U &&
            fixture.state.loaded_data_offset == 0x000052EFU &&
            fixture.state.window[300U] == 0xA5U &&
            fixture.state.previous_opcode ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
            fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
        "real opcode 165 item 798 takes total-one equality and same-calls its TALK4 target"
    );
}

void test_real_mode_text_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        u32 file_number;
        std::streamoff file_offset;
        u16 opcode;
        std::size_t length;
    };
    constexpr std::array samples{
        Sample{1U, 0x000233D7U, OP_171_SET_MODE17_TEXT, 38U},
        Sample{1U, 0x000233FDU, OP_173_SET_MODE18_TEXT, 38U},
        Sample{1U, 0x00038FA3U, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{1U, 0x00038FA5U, OP_172_CLEAR_MODE18_TEXT, 2U},
        Sample{1U, 0x00044656U, OP_171_SET_MODE17_TEXT, 30U},
        Sample{1U, 0x00044674U, OP_173_SET_MODE18_TEXT, 30U},
        Sample{1U, 0x00052771U, OP_172_CLEAR_MODE18_TEXT, 2U},
        Sample{1U, 0x00052773U, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{2U, 0x000150EBU, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{2U, 0x000150EDU, OP_172_CLEAR_MODE18_TEXT, 2U},
        Sample{2U, 0x000320F4U, OP_171_SET_MODE17_TEXT, 40U},
        Sample{2U, 0x0003211CU, OP_173_SET_MODE18_TEXT, 40U},
        Sample{3U, 0x00010AAEU, OP_173_SET_MODE18_TEXT, 14U},
        Sample{3U, 0x00017BFBU, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{3U, 0x00027487U, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{3U, 0x0002D1A0U, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{3U, 0x0002D1B4U, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{4U, 0x0000510CU, OP_172_CLEAR_MODE18_TEXT, 2U},
        Sample{4U, 0x0000510EU, OP_170_CLEAR_MODE17_TEXT, 2U},
        Sample{4U, 0x000052F3U, OP_172_CLEAR_MODE18_TEXT, 2U},
        Sample{4U, 0x000052F5U, OP_170_CLEAR_MODE17_TEXT, 2U},
    };

    std::array<std::size_t, 4U> opcode_counts{};
    std::array<std::size_t, 4U> file_counts{};
    bool all_records_valid = true;
    for (const auto sample : samples) {
        std::ifstream input{
            root / ("TALK" + std::to_string(sample.file_number) + ".DAT"),
            std::ios::binary | std::ios::in
        };
        std::vector<u8> instruction(sample.length);
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool stores_text = (sample.opcode & 1U) != 0U;
        all_records_valid = all_records_valid && static_cast<bool>(input) &&
            (read_u16(instruction, 0U) & 0x3FFFU) == sample.opcode &&
            (!stores_text ||
             (sample.length >= 4U &&
              read_u16(instruction, sample.length - 2U) == 0x5125U &&
              sample.length - 4U <
                  openswd3::world_map::kLegacyWorldStoryModeTextSize));
        ++opcode_counts[sample.opcode - OP_170_CLEAR_MODE17_TEXT];
        ++file_counts[sample.file_number - 1U];
    }
    test.expect_true(
        all_records_valid && samples.size() == 21U &&
            opcode_counts == std::array<std::size_t, 4U>{9U, 3U, 5U, 4U} &&
            file_counts == std::array<std::size_t, 4U>{8U, 4U, 5U, 4U},
        "all 21 real mode-text records preserve base raw words, fixed or percent-Q lengths, per-opcode counts, and TALK-file distribution"
    );

    constexpr std::array replay_indices{2U, 0U, 17U, 12U};
    for (const std::size_t replay_index : replay_indices) {
        const auto sample = samples[replay_index];
        std::ifstream input{
            root / ("TALK" + std::to_string(sample.file_number) + ".DAT"),
            std::ios::binary | std::ios::in
        };
        std::vector<u8> instruction(sample.length);
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);
        const bool stores_text = (sample.opcode & 1U) != 0U;
        const std::size_t text_index =
            sample.opcode <= OP_171_SET_MODE17_TEXT ? 0U : 1U;
        const u16 flag_index = static_cast<u16>(77U + text_index);

        Fixture fixture;
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.loaded_file_number = sample.file_number;
        fixture.state.loaded_data_offset =
            static_cast<u32>(sample.file_offset - 0x200);
        fixture.context.talk_data_offset =
            static_cast<u32>(sample.file_offset - 0x200);
        fixture.state.mode_texts[text_index].allocated = true;
        fixture.state.mode_texts[text_index].bytes.fill(0xA5U);
        if (stores_text) {
            openswd3::world_map::clear_legacy_world_story_flag(
                fixture.state, flag_index
            );
        } else {
            openswd3::world_map::set_legacy_world_story_flag(
                fixture.state, flag_index
            );
        }

        const auto result = fixture.step();
        const auto& text = fixture.state.mode_texts[text_index];
        const std::size_t payload_length =
            stores_text ? sample.length - 4U : 0U;
        const bool payload_matches = !stores_text ||
            (std::ranges::equal(
                 std::span<const u8>{instruction}.subspan(2U, payload_length),
                 std::span<const u8>{text.bytes}.first(payload_length)
             ) &&
             text.bytes[payload_length] == 0U);
        test.expect_true(
            instruction_read &&
                (read_u16(instruction, 0U) & 0x3FFFU) == sample.opcode &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == sample.opcode &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == sample.length &&
                text.allocated == stores_text && payload_matches &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, flag_index
                ) == stores_text &&
                fixture.state.previous_opcode == sample.opcode,
            "real opcodes 170 through 173 clear or replace their selected persisted mode text, synchronize the paired flag, and yield after audio"
        );
    }
}

void test_real_gather_party_at_player_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        u32 file_number;
        std::streamoff file_offset;
    };
    constexpr std::array samples{
        Sample{3U, 0x000066FEU}, Sample{4U, 0x00009842U},
        Sample{4U, 0x00009970U}, Sample{4U, 0x00009A60U},
        Sample{4U, 0x00009B50U}, Sample{4U, 0x00009C40U},
        Sample{4U, 0x00009D30U}, Sample{4U, 0x00009E20U},
        Sample{4U, 0x00009F10U}, Sample{4U, 0x0001A9DBU},
        Sample{4U, 0x0001B7E9U}, Sample{4U, 0x0001B8CDU},
        Sample{4U, 0x0001B9B9U}, Sample{4U, 0x0001BA9DU},
        Sample{4U, 0x0001BB81U}, Sample{4U, 0x0001BC65U},
        Sample{4U, 0x0001BD49U}, Sample{4U, 0x0001BE2DU},
        Sample{4U, 0x0001BEE1U}, Sample{4U, 0x0001BFC5U},
        Sample{4U, 0x0001C0A9U}, Sample{4U, 0x0001C18DU},
        Sample{4U, 0x0001C279U}, Sample{4U, 0x0001C35DU},
        Sample{4U, 0x0001C441U}, Sample{4U, 0x0001C525U},
        Sample{4U, 0x0001C609U}, Sample{4U, 0x00021832U},
        Sample{4U, 0x0002B5FCU},
    };

    std::array<std::size_t, 4U> file_counts{};
    bool all_records_valid = true;
    for (const auto sample : samples) {
        std::ifstream input{
            root / ("TALK" + std::to_string(sample.file_number) + ".DAT"),
            std::ios::binary | std::ios::in
        };
        std::array<u8, 2U> instruction{};
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        all_records_valid = all_records_valid && static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_177_GATHER_PARTY_AT_PLAYER;
        ++file_counts[sample.file_number - 1U];
    }

    test.expect_true(
        all_records_valid && samples.size() == 29U &&
            file_counts == std::array<std::size_t, 4U>{0U, 0U, 1U, 28U},
        "all 29 real opcode 177 records are base raw two-byte instructions with the locked TALK3 and TALK4 distribution"
    );

    for (const std::size_t replay_index : {0U, 1U}) {
        const auto sample = samples[replay_index];
        std::ifstream input{
            root / ("TALK" + std::to_string(sample.file_number) + ".DAT"),
            std::ios::binary | std::ios::in
        };
        std::array<u8, 2U> instruction{};
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_177_GATHER_PARTY_AT_PLAYER);
        fixture.context.instruction_offset = 0x7FFEU;
        std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFEU);
        fixture.roles[0].action.base_variant = 9U;
        fixture.dialogs.close.flagged_dialog_counter = 0x12340005U;
        fixture.player_post_frame.world_x_history.fill(0x11111111U);
        fixture.player_post_frame.world_y_history.fill(0x22222222U);

        const auto setup = fixture.step();
        const u16 setup_raw = read_u16(fixture.state.window, 0x7FFEU);
        const u16 setup_ip = fixture.context.instruction_offset;
        const u32 setup_previous = fixture.state.previous_opcode;
        const u32 setup_audio = fixture.ports.direct_audio_service_count;
        const auto completed = fixture.step();

        test.expect_true(
            instruction_read &&
                read_u16(instruction, 0U) == OP_177_GATHER_PARTY_AT_PLAYER &&
                setup.status == LegacyWorldStoryVmStatus::yielded &&
                setup.executed_instruction_count == 1U &&
                setup.direct_audio_service_count == 1U &&
                setup_raw ==
                    static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x8000U) &&
                setup_ip == 0x7FFEU &&
                setup_previous == OP_177_GATHER_PARTY_AT_PLAYER &&
                setup_audio == 1U &&
                fixture.roles[0].action.base_variant == 0U &&
                fixture.dialogs.close.flagged_dialog_counter == 0x12348005U &&
                std::ranges::all_of(
                    fixture.player_post_frame.world_x_history,
                    [](const u32 value) { return value == 16U; }
                ) &&
                std::ranges::all_of(
                    fixture.player_post_frame.world_y_history,
                    [](const u32 value) { return value == 16U; }
                ) &&
                completed.status == LegacyWorldStoryVmStatus::yielded &&
                completed.executed_instruction_count == 1U &&
                completed.direct_audio_service_count == 1U &&
                fixture.ports.direct_audio_service_count == 2U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode ==
                    OP_177_GATHER_PARTY_AT_PLAYER &&
                read_u16(fixture.state.window, 0x7FFEU) ==
                    OP_177_GATHER_PARTY_AT_PLAYER,
            "real opcode 177 performs setup at the same exact-tail IP, then its self-marked poll completes the one-role party and audio-yields at IP 0x8000"
        );
    }
}

void test_real_set_role_collision_bypass_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        u32 file_number;
        std::streamoff file_offset;
        u16 selector;
    };
    constexpr std::array samples{
        Sample{1U, 0x0003FB3DU, 0x0143U},
        Sample{3U, 0x0001B471U, 0x0002U},
        Sample{3U, 0x0001B485U, 0x0002U},
        Sample{3U, 0x00033812U, 0x0397U},
        Sample{4U, 0x00018AF1U, 0x0068U},
        Sample{4U, 0x00018B80U, 0x0068U},
    };

    std::array<std::size_t, 4U> file_counts{};
    bool all_records_valid = true;
    bool all_replays_valid = true;
    for (const auto sample : samples) {
        std::ifstream input{
            root / ("TALK" + std::to_string(sample.file_number) + ".DAT"),
            std::ios::binary | std::ios::in
        };
        std::array<u8, 4U> instruction{};
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_valid = static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_178_SET_ROLE_COLLISION_BYPASS &&
            read_u16(instruction, 2U) == sample.selector;
        all_records_valid = all_records_valid && instruction_valid;
        ++file_counts[sample.file_number - 1U];

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_178_SET_ROLE_COLLISION_BYPASS);
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.previous_opcode = 0x66U;
        fixture.roles[1].guid = sample.selector;
        fixture.roles[1].flags = 0x80000001U;
        std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFCU);
        const auto result = fixture.step();
        all_replays_valid = all_replays_valid &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_178_SET_ROLE_COLLISION_BYPASS &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 0U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_178_SET_ROLE_COLLISION_BYPASS &&
            fixture.roles[1].flags == 0x80040001U;
    }

    test.expect_true(
        all_records_valid && all_replays_valid && samples.size() == 6U &&
            file_counts == std::array<std::size_t, 4U>{1U, 0U, 3U, 2U},
        "all six real opcode 178 records preserve base raw words and locked selectors, set role collision bypass, and complete before exact-tail successor fetch failure"
    );
}

void test_real_jump_same_file_offset_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00008A85);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 1U;
    state.loaded_data_offset = 0x00008885U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00F8U;
    context.talk_script_id = 248U;
    context.talk_data_offset = 0x00008885U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_15_JUMP_SAME_FILE_OFFSET &&
            read_u32(instruction, 2U) == 0x000088CFU &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 2U &&
            context.talk_data_offset == 0x000088CFU &&
            context.instruction_offset == 4U &&
            state.loaded_file_number == 1U &&
            state.loaded_data_offset == 0x000088CFU &&
            state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            ports.sound_effect_requests == std::vector<u16>{0x003AU},
        "real opcode 15 jumps within TALK1 and same-call executes opcode 59"
    );
}

void test_real_jump_if_role_path_unprepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000F963);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 2U;
    state.loaded_data_offset = 0x0000F763U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00E4U;
    context.talk_script_id = 2200U;
    context.talk_data_offset = 0x0000F763U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x00E4U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    write_u16(active_object_slots[0].bytes, 0U, 1U);
    active_object_slots[0].bytes[0x1BU] = 2U;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {.current_tick = 0U},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            read_u16(instruction, 2U) == 0x00E4U &&
            read_u32(instruction, 4U) == 0x0000F787U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 67U && result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 2U &&
            context.talk_data_offset == 0x0000F787U &&
            context.instruction_offset == 0U &&
            state.loaded_file_number == 2U &&
            state.loaded_data_offset == 0x0000F787U &&
            state.previous_opcode == OP_67_WAIT_FRAME_CLOCK &&
            state.wait_duration == 0x012CU &&
            read_u16(state.window, 2U) == 0x812CU,
        "real opcode 16 jumps within TALK2 and same-call starts opcode 67 wait"
    );
}

void test_real_jump_if_role_path_prepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000074A6);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 2U;
    state.loaded_data_offset = 0x000072A6U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00DAU;
    context.talk_script_id = 2200U;
    context.talk_data_offset = 0x000072A6U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x00DAU;
    roles[1].flags = 0x40040000U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    write_u16(active_object_slots[0].bytes, 0U, 1U);
    write_u16(active_object_slots[0].bytes, 2U, 0U);
    active_object_slots[0].bytes[0x1BU] = 2U;
    active_object_slots[0].bytes[0x1CU] = 0U;
    std::vector<u8> surface(4U, 0U);
    openswd3::world_map::LegacyRoleSpatialIndex spatial;
    openswd3::world_map::LegacyWorldPathNodePool node_pool;
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::world_map::LegacyWorldCameraRect camera{};
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = roles,
        .active_object_slots = active_object_slots,
        .spatial_index = &spatial,
        .role_surface =
            {
                .map_width = 1U,
                .surface_grid = surface,
            },
        .node_pool = &node_pool,
        .movement = &movement,
        .camera = &camera,
        .selected_arrival_bytes = {},
        .selected_role_index = 0U,
        .map_height = 1U,
        .scene_render_flags = &scene_render_flags,
    };
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{};
    runtime.story_paths = &story_paths;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        runtime,
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            read_u32(instruction, 4U) == 0x00007296U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_109_STEP_ROLES &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 2U &&
            context.talk_data_offset == 0x00007296U &&
            context.instruction_offset == 8U &&
            state.previous_opcode == OP_109_STEP_ROLES,
        "real opcode 17 jumps within TALK2 and executes opcode 109"
    );
}

void test_real_release_role_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00054136);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_18_RELEASE_ROLE_PATH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].guid = 0x000BU;
    fixture.roles[1].flags = 0x20000000U;
    fixture.roles[1].interaction_gate = 1U;
    fixture.roles[1].action.wait_remaining = 9U;
    write_u16(fixture.state.window, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(fixture.state.window, 12U, 0x000BU);
    const auto result = fixture.step();
    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_18_RELEASE_ROLE_PATH &&
            read_u16(instruction, 2U) == 0x000BU &&
            read_u16(instruction, 4U) ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 3U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 10U &&
            fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            fixture.roles[1].flags == 0x20000000U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            fixture.ports.data_load_count == 0U,
        "real opcode 18 releases role 11 before opcode 111 skips its target"
    );
}

void test_real_release_all_role_paths_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00010C93);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_19_RELEASE_ROLE_PATHS);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].flags = 0x20000000U;
    fixture.roles[1].action.wait_remaining = 7U;
    const auto result = fixture.step();
    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_19_RELEASE_ROLE_PATHS &&
            read_u16(instruction, 2U) == OP_28_CHANGE_ROLE_PATH_ID &&
            read_u16(instruction, 4U) == 102U &&
            read_u16(instruction, 6U) == 102U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            fixture.roles[1].flags == 0x20000000U &&
            fixture.roles[1].action.wait_remaining == 7U &&
            fixture.ports.role_patch_requests.size() == 1U &&
            fixture.ports.role_patch_requests.front().guid == 102U &&
            fixture.ports.role_patch_requests.front().path_data_id == 102U &&
            fixture.ports.role_patch_requests.front().flags_or_mask ==
                0x1000U &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 19 skips released roles then opcode 28 patches MAPS and yields"
    );
}

void test_real_schedule_role_path_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        std::size_t instruction_size;
        u16 opcode;
        u16 expected_action_id;
        u16 expected_base_variant;
        u16 expected_variant_delta;
    };
    constexpr std::array<Sample, 2U> samples{
        Sample{
            0x000049F6,
            10U,
            OP_20_SCHEDULE_ROLE_PATHS,
            0xFFFFU,
            0xFFFFU,
            0xFFFFU,
        },
        Sample{
            0x0002A56D,
            16U,
            OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS,
            0xFFFFU,
            0U,
            7U,
        },
    };

    for (const auto sample : samples) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(sample.file_offset);
        std::vector<u8> instruction(sample.instruction_size + 2U);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.source_guid = 1U;
        fixture.roles[1].guid = 1U;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, sample.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        write_u16(
            fixture.state.window, sample.instruction_size, kStoryVmTypedStop
        );
        const auto staged = fixture.step();
        const u16 staged_count = read_u16(fixture.state.window, 2U);
        const auto& slot = fixture.active_object_slots[0].bytes;
        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();

        test.expect_true(
            instruction_read && read_u16(instruction, 0U) == sample.opcode &&
                (read_u16(instruction, 2U) & 0x4000U) == 0U &&
                staged.status == LegacyWorldStoryVmStatus::yielded &&
                staged.opcode == sample.opcode && staged_count == 0x4001U &&
                read_u16(slot, 0x10U) == sample.expected_action_id &&
                read_u16(slot, 0x12U) == sample.expected_base_variant &&
                read_u16(slot, 0x14U) == sample.expected_variant_delta &&
                completed.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                completed.opcode == kStoryVmTypedStop &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == sample.instruction_size &&
                fixture.state.previous_opcode == sample.opcode &&
                read_u16(fixture.state.window, 2U) == 1U,
            "real opcode 20/169 records stage paths then complete in-call"
        );
    }
}

void test_real_jump_if_global_bit_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        u16 opcode;
        u16 bit_index;
        u32 target;
        bool set_bit;
    };
    constexpr std::array<Sample, 2U> samples{
        Sample{
            0x000014CE,
            OP_21_JUMP_IF_GLOBAL_BIT_SET,
            0x01BDU,
            0x000012F0U,
            true,
        },
        Sample{
            0x00002560,
            OP_22_JUMP_IF_GLOBAL_BIT_CLEAR,
            0x0064U,
            0x00002386U,
            false,
        },
    };

    std::array<std::array<u8, 8U>, samples.size()> instructions{};
    std::array<bool, samples.size()> instruction_reads{};
    for (std::size_t index = 0U; index < samples.size(); ++index) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(samples[index].file_offset);
        input.read(
            reinterpret_cast<char*>(instructions[index].data()),
            static_cast<std::streamsize>(instructions[index].size())
        );
        instruction_reads[index] = static_cast<bool>(input);
    }

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    for (std::size_t index = 0U; index < samples.size(); ++index) {
        const auto sample = samples[index];
        const auto& instruction = instructions[index];
        const bool instruction_read = instruction_reads[index];
        Fixture fixture;
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset =
            static_cast<u32>(sample.file_offset) - 0x200U;
        fixture.state.window_loaded = true;
        fixture.context.talk_data_offset = fixture.state.loaded_data_offset;
        if (sample.set_bit) {
            openswd3::world_map::set_legacy_world_story_flag(
                fixture.state, sample.bit_index
            );
        }
        RealPorts ports{databases};
        const auto result = openswd3::world_map::step_legacy_world_story_vm(
            fixture.context,
            fixture.state,
            fixture.roles,
            0U,
            fixture.active_object_slots,
            fixture.maps_payload,
            fixture.dialogs,
            fixture.dialog_resources,
            fixture.first_name,
            fixture.second_name,
            fixture.runtime,
            ports
        );

        const bool opcode21_tail =
            result.status == LegacyWorldStoryVmStatus::role_not_found &&
            result.opcode == OP_76_TURN_AND_SUSPEND_STORY_ROLE &&
            result.executed_instruction_count == 3U &&
            fixture.context.talk_data_offset == sample.target &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.loaded_file_number == 1U &&
            fixture.state.loaded_data_offset == sample.target;
        const bool opcode22_tail =
            result.status == LegacyWorldStoryVmStatus::terminated &&
            result.opcode == 0x3FFFU &&
            result.executed_instruction_count == 3U &&
            fixture.context.talk_data_offset == 0xFFFFFFFFU &&
            fixture.context.instruction_offset == 0xFFFFU;
        test.expect_true(
            instruction_read &&
                initialized.status ==
                    openswd3::resource_io::LegacyResourceDatabaseStatus::
                        ready &&
                read_u16(instruction, 0U) == sample.opcode &&
                read_u16(instruction, 2U) == sample.bit_index &&
                read_u32(instruction, 4U) == sample.target &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.state.previous_opcode ==
                    OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL,
            "real opcode 21/22 records execute the audited first branch"
        );
        if (sample.opcode == OP_21_JUMP_IF_GLOBAL_BIT_SET) {
            test.expect_true(
                opcode21_tail,
                "real opcode 21 target follows the expected prefix chain"
            );
        } else {
            test.expect_true(
                opcode22_tail,
                "real opcode 22 target follows the expected prefix chain"
            );
        }
    }
}

void test_real_jump_if_all_global_bits_set_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00008AD3);
    std::array<u8, 12U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    Fixture fixture;
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x000088D3U;
    fixture.state.window_loaded = true;
    fixture.context.talk_data_offset = 0x000088D3U;
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 295U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 264U);
    RealPorts ports{databases};
    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        fixture.context,
        fixture.state,
        fixture.roles,
        0U,
        fixture.active_object_slots,
        fixture.maps_payload,
        fixture.dialogs,
        fixture.dialog_resources,
        fixture.first_name,
        fixture.second_name,
        fixture.runtime,
        ports
    );

    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            read_u16(instruction, 2U) == 295U &&
            read_u16(instruction, 4U) == 264U &&
            read_u16(instruction, 6U) == 0xFF00U &&
            read_u32(instruction, 8U) == 0x000088E1U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 2U &&
            fixture.context.talk_data_offset == 0x000088E1U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.loaded_file_number == 1U &&
            fixture.state.loaded_data_offset == 0x000088E1U &&
            fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            ports.sound_effect_requests == std::vector<u16>{0x0039U},
        "real opcode 23 branches then same-call opcode 59 plays sound and yields"
    );
}

void test_real_set_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000074C1);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_25_SET_GLOBAL_BIT);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_data_offset = 0x000072C1U;
    fixture.context.talk_data_offset = 0x000072C1U;
    const auto result = fixture.step();

    test.expect_true(
        instruction_read && read_u16(instruction, 0U) == OP_25_SET_GLOBAL_BIT &&
            read_u16(instruction, 2U) == 7080U &&
            read_u16(instruction, 4U) == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(instruction, 6U) == 0x00ABU &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            result.executed_instruction_count == 2U &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 7080U
            ) &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            fixture.ports.sound_effect_requests == std::vector<u16>{0x00ABU},
        "real opcode 25 sets bit 7080 then same-call opcode 59 plays sound"
    );
}

void test_real_clear_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000265FE);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_26_CLEAR_GLOBAL_BIT);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_data_offset = 0x000263FEU;
    fixture.context.talk_data_offset = 0x000263FEU;
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 614U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 615U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 616U);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_26_CLEAR_GLOBAL_BIT &&
            read_u16(instruction, 2U) == 615U &&
            read_u16(instruction, 4U) == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(instruction, 6U) == 0x0038U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            result.executed_instruction_count == 2U &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 614U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 615U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 616U
            ) &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            fixture.ports.sound_effect_requests == std::vector<u16>{0x0038U},
        "real opcode 26 clears bit 615 then same-call opcode 59 plays sound"
    );
}

void test_real_reload_world_session_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK3.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00016095);
    std::array<u8, 14U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_27_RELOAD_WORLD_SESSION);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 14U, kStoryVmTypedStop);
    fixture.runtime.role_surface.selected_guid = 0x00F8U;
    fixture.roles[1].action.action_id = 0x12345U;
    fixture.roles[1].action.base_variant = 0x23456U;
    fixture.roles[1].action.variant_delta = 0x34567U;
    const auto result = fixture.step(0, 0, 1U);
    const auto& request = fixture.ports.last_world_load_request;

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_27_RELOAD_WORLD_SESSION &&
            read_u16(instruction, 2U) == 161U &&
            read_u16(instruction, 4U) == 23U &&
            read_u16(instruction, 6U) == 22U &&
            read_u16(instruction, 8U) == 0xFFFFU &&
            read_u16(instruction, 10U) == 0xFFFFU &&
            read_u16(instruction, 12U) == 0xFFFFU &&
            result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.opcode == kStoryVmTypedStop &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 14U &&
            fixture.state.previous_opcode == OP_27_RELOAD_WORLD_SESSION &&
            request.logical_map_id == 161U && request.tile_x == 23U &&
            request.tile_y == 22U && request.action_id == 0x2345U &&
            request.base_variant == 0x3456U &&
            request.variant_delta == 0x4567U &&
            request.selected_guid == 0x00F8U && request.load_flags == 1U,
        "real opcode 27 record inherits all three controlled-role action " "fields before synchronous world reload"
    );
}

void test_real_reload_deferred_world_session_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00038E29);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_156_RELOAD_DEFERRED_WORLD_SESSION);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.runtime.role_surface.selected_guid = 0x00F8U;
    fixture.roles[0].action.action_id = 0x12345U;
    fixture.state.deferred_map_tile_x = 23;
    fixture.state.deferred_map_tile_y = 22;
    fixture.state.deferred_map_id = 161;
    const auto result = fixture.step();
    const auto& request = fixture.ports.last_world_load_request;

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode ==
                OP_156_RELOAD_DEFERRED_WORLD_SESSION &&
            fixture.state.deferred_map_tile_x == -1 &&
            fixture.state.deferred_map_tile_y == -1 &&
            fixture.state.deferred_map_id == 0 &&
            request.logical_map_id == 161U && request.tile_x == 23U &&
            request.tile_y == 22U && request.action_id == 0x2345U &&
            request.base_variant == 0U && request.variant_delta == 1U &&
            request.selected_guid == 0x00F8U && request.load_flags == 1U &&
            fixture.ports.story_protocol_events == std::vector<u32>{6U, 7U, 2U},
        "real opcode 156 record reloads deferred world state, clears it, and yields"
    );
}

void test_real_change_role_path_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0001938D);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_28_CHANGE_ROLE_PATH_ID);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].guid = 2U;
    fixture.roles[1].path_data_id = 0x1111U;
    fixture.roles[1].path_word_index = 7U;
    fixture.roles[1].flags = 0x20U;
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_28_CHANGE_ROLE_PATH_ID &&
            read_u16(instruction, 2U) == 2U &&
            read_u16(instruction, 4U) == 30U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            fixture.roles[1].path_data_id == 30U &&
            fixture.roles[1].path_word_index == 0U &&
            fixture.roles[1].flags == 0x1020U &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 28 record replaces live role path id then yields"
    );
}

void test_real_global_integer_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream add_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    add_input.seekg(0x00007FAF);
    std::array<u8, 6U> add_instruction{};
    add_input.read(
        reinterpret_cast<char*>(add_instruction.data()),
        static_cast<std::streamsize>(add_instruction.size())
    );
    const bool add_read = static_cast<bool>(add_input);

    Fixture add_fixture;
    prime_loaded_instruction(add_fixture, OP_30_ADD_GLOBAL_INTEGER);
    std::ranges::copy(add_instruction, add_fixture.state.window.begin());
    write_u16(add_fixture.state.window, 6U, kStoryVmTypedStop);
    const auto add_result = add_fixture.step();
    test.expect_true(
        add_read && read_u16(add_instruction, 0U) == OP_30_ADD_GLOBAL_INTEGER &&
            read_u16(add_instruction, 2U) == 0U &&
            read_u16(add_instruction, 4U) == 50U &&
            add_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            add_result.executed_instruction_count == 2U &&
            add_fixture.context.instruction_offset == 6U &&
            add_fixture.state.previous_opcode == OP_30_ADD_GLOBAL_INTEGER &&
            add_fixture.state.script_variables[0] == 150U,
        "real opcode 30 record adds 50 to initialized variable zero"
    );

    std::ifstream chain_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    chain_input.seekg(0x0000FF97);
    std::array<u8, 28U> chain{};
    chain_input.read(
        reinterpret_cast<char*>(chain.data()),
        static_cast<std::streamsize>(chain.size())
    );
    const bool chain_read = static_cast<bool>(chain_input);

    Fixture chain_fixture;
    prime_loaded_instruction(
        chain_fixture, OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE
    );
    std::ranges::copy(chain, chain_fixture.state.window.begin());
    chain_fixture.state.script_variables[62] = 3U;
    const auto chain_result = chain_fixture.step();
    test.expect_true(
        chain_read &&
            read_u16(chain, 0U) == OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
            read_u16(chain, 2U) == 62U && read_u16(chain, 4U) == 2U &&
            read_u16(chain, 10U) == OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            read_u16(chain, 12U) == 62U && read_u16(chain, 14U) == 4U &&
            read_u16(chain, 20U) == OP_29_SET_GLOBAL_INTEGER &&
            read_u16(chain, 22U) == 62U && read_u16(chain, 24U) == 4U &&
            read_u16(chain, 26U) == 0xFFFFU &&
            chain_result.status == LegacyWorldStoryVmStatus::terminated &&
            chain_result.executed_instruction_count == 4U &&
            chain_fixture.context.instruction_offset == 0xFFFFU &&
            chain_fixture.state.previous_opcode == OP_29_SET_GLOBAL_INTEGER &&
            chain_fixture.state.script_variables[62] == 4U &&
            chain_fixture.ports.data_load_count == 0U,
        "real opcode 33 to 32 to 29 chain falls through then terminates"
    );

    std::ifstream wide_input{
        root / "TALK4.DAT", std::ios::binary | std::ios::in
    };
    wide_input.seekg(0x00031BF1);
    std::array<u8, 12U> wide_instruction{};
    wide_input.read(
        reinterpret_cast<char*>(wide_instruction.data()),
        static_cast<std::streamsize>(wide_instruction.size())
    );
    const bool wide_read = static_cast<bool>(wide_input);

    Fixture wide_not_taken;
    prime_loaded_instruction(
        wide_not_taken, OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE
    );
    std::ranges::copy(wide_instruction, wide_not_taken.state.window.begin());
    write_u16(wide_not_taken.state.window, 12U, kStoryVmTypedStop);
    const auto wide_not_taken_result = wide_not_taken.step();

    Fixture wide_taken;
    prime_loaded_instruction(
        wide_taken, OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE
    );
    std::ranges::copy(wide_instruction, wide_taken.state.window.begin());
    wide_taken.state.script_variables[0] = 30000000U;
    write_u16(wide_taken.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto wide_taken_result = wide_taken.step();
    test.expect_true(
        wide_read &&
            read_u16(wide_instruction, 0U) ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            read_u16(wide_instruction, 2U) == 0U &&
            read_u32(wide_instruction, 4U) == 30000000U &&
            read_u32(wide_instruction, 8U) == 0x00031A59U &&
            wide_not_taken_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            wide_not_taken_result.executed_instruction_count == 2U &&
            wide_not_taken.context.instruction_offset == 12U &&
            wide_not_taken.state.previous_opcode ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            wide_not_taken.ports.data_load_count == 0U &&
            wide_taken_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            wide_taken_result.executed_instruction_count == 2U &&
            wide_taken.context.talk_data_offset == 0x00031A59U &&
            wide_taken.context.instruction_offset == 0U &&
            wide_taken.state.previous_opcode ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            wide_taken.ports.data_load_count == 1U &&
            wide_taken.ports.last_data_offset == 0x00031A59U &&
            wide_taken.ports.direct_audio_service_count == 1U,
        "real opcode 184 compares variable zero against 30000000 and takes its +8 target only at the unsigned threshold"
    );
}

void test_real_relocate_role_and_complete_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000464E);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            read_u16(instruction, 2U) == 1U &&
            read_u16(instruction, 4U) == 0x24U &&
            read_u16(instruction, 6U) == 0x21U &&
            result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.opcode == kStoryVmTypedStop &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.tile_x == 0x24U &&
            request.tile_y == 0x21U && request.flags_or_mask == 0U &&
            request.flags_and_mask == 0xFFFFU,
        "real opcode 40 record patches missing role coordinates then continues"
    );
}

void test_real_reload_indexed_target_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000042E6);
    std::array<u8, 26U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.indexed_target_selector = 3U;
    prime_loaded_instruction(fixture, OP_41_RELOAD_INDEXED_TARGET);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_41_RELOAD_INDEXED_TARGET &&
            read_u32(instruction, 2U) == 0x00004100U &&
            read_u32(instruction, 6U) == 0x00004118U &&
            read_u32(instruction, 10U) == 0x00004124U &&
            read_u32(instruction, 14U) == 0x0000410CU &&
            read_u32(instruction, 18U) == 0x00004130U &&
            read_u32(instruction, 22U) == 0xFF00FF00U &&
            result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.opcode == kStoryVmTypedStop &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 1U &&
            fixture.ports.last_data_file_number == 1U &&
            fixture.ports.last_data_offset == 0x0000410CU &&
            fixture.context.talk_data_offset == 0x0000410CU &&
            fixture.indexed_target_selector == 0U &&
            fixture.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "real opcode 41 record selects target three, resets selector, and reloads"
    );
}

void test_real_interaction_lock_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000046EE);
    std::array<u8, 2U> set_instruction{};
    input.read(
        reinterpret_cast<char*>(set_instruction.data()),
        static_cast<std::streamsize>(set_instruction.size())
    );
    input.seekg(0x0000A164);
    std::array<u8, 2U> clear_instruction{};
    input.read(
        reinterpret_cast<char*>(clear_instruction.data()),
        static_cast<std::streamsize>(clear_instruction.size())
    );
    const bool instructions_read = static_cast<bool>(input);

    Fixture set_fixture;
    set_fixture.dialogs.close.flagged_dialog_counter = 0x12340005U;
    set_fixture.roles[0].action.base_variant = 7U;
    prime_loaded_instruction(
        set_fixture, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    std::ranges::copy(set_instruction, set_fixture.state.window.begin());
    write_u16(set_fixture.state.window, 2U, kStoryVmTypedStop);
    const auto set_result = set_fixture.step();

    Fixture clear_fixture;
    clear_fixture.dialogs.close.flagged_dialog_counter = 0x12348005U;
    prime_loaded_instruction(clear_fixture, OP_43_CLEAR_INTERACTION_LOCK);
    std::ranges::copy(clear_instruction, clear_fixture.state.window.begin());
    write_u16(clear_fixture.state.window, 2U, kStoryVmTypedStop);
    const auto clear_result = clear_fixture.step();

    test.expect_true(
        instructions_read &&
            read_u16(set_instruction, 0U) ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
            read_u16(clear_instruction, 0U) == OP_43_CLEAR_INTERACTION_LOCK &&
            set_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            set_result.executed_instruction_count == 2U &&
            set_fixture.dialogs.close.flagged_dialog_counter == 0x12348005U &&
            set_fixture.roles[0].action.base_variant == 0U &&
            set_fixture.ports.action_update_count == 1U &&
            set_fixture.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
            clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            clear_result.executed_instruction_count == 2U &&
            clear_fixture.dialogs.close.flagged_dialog_counter == 0x12340005U &&
            clear_fixture.ports.action_update_count == 0U &&
            clear_fixture.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK,
        "real opcodes 42 and 43 set and clear the shared interaction lock"
    );
}

void test_real_set_role_action_wait_override_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00041D04);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[1].guid = 0x027FU;
    fixture.roles[1].action.wait_remaining = 9U;
    fixture.roles[1].action.wait_override = 0x8123U;
    prime_loaded_instruction(fixture, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            read_u16(instruction, 2U) == 0x027FU &&
            read_u16(instruction, 4U) == 0U &&
            result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 1U &&
            fixture.roles[1].action.wait_override == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 44 writes the role wait override and clears remaining wait"
    );
}
