#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_shared_dialog_handler_variants(openswd3::test::Context& test) {
    constexpr std::array<u16, 8U> opcodes{1U, 2U, 3U, 4U, 5U, 6U, 89U, 90U};
    constexpr std::array<u8, 3U> text{'A', '%', 'Q'};
    for (const u16 opcode : opcodes) {
        Fixture fixture;
        const std::size_t end =
            write_dialog_instruction(fixture, opcode, 0x00F8U, text);
        fixture.state.previous_opcode = 0x1234U;
        const auto result = fixture.step();
        const auto& message = fixture.dialogs.messages.front();
        const auto& record = message.record;
        const bool odd_variant = (opcode & 1U) != 0U;
        const u32 expected_flags = 0x00040000U |
            (opcode == 5U || opcode == 6U ? 0x40U : 0U) |
            (odd_variant ? 0x10U : 0U);
        const u16 expected_width = opcode <= 2U ? 176U : 22U;
        const u16 expected_height = opcode <= 2U ? 66U : 33U;
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.dialog_enqueue_count == 1U &&
                result.dialog_text_prepare_count == 1U &&
                result.dialog_text_prepare_success_count == 0U &&
                result.direct_audio_service_count == 2U &&
                result.action_update_count == 1U &&
                fixture.context.instruction_offset == end &&
                fixture.roles[1].interaction_gate == (odd_variant ? 1U : 2U) &&
                fixture.dialogs.close.flagged_dialog_counter ==
                    (odd_variant ? 1U : 0U) &&
                record.role_index == 1U && record.flags == expected_flags &&
                record.width == expected_width &&
                record.height == expected_height &&
                record.character_delay == 4U &&
                message.text == std::vector<u8>(text.begin(), text.end()) &&
                fixture.state.previous_opcode == opcode &&
                fixture.state.dialog_anchor_left == 0x8000U &&
                fixture.state.dialog_anchor_top == 0x8000U &&
                fixture.state.next_dialog_flag18_suppression == 0U &&
                fixture.state.text_control_flags == 0xFFFFFFFFU &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 3U, 4U, 2U},
            "all eight shared dialog opcodes preserve their variant contract"
        );
        if (opcode >= 3U && opcode <= 6U) {
            test.expect_true(
                record.left == 100U && record.top == 120U,
                "mode one retains its explicit left and top words"
            );
        }
    }
}

void test_shared_dialog_raw_aliases(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        90U,
        static_cast<u16>(90U | 0x4000U),
        static_cast<u16>(90U | 0x8000U),
        static_cast<u16>(90U | 0xC000U)
    };
    constexpr std::array<u8, 2U> text{'%', 'Q'};
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        const std::size_t end =
            write_dialog_instruction(fixture, raw_word, 0xFFF0U, text);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == raw_word && result.opcode == 90U &&
                fixture.context.instruction_offset == end &&
                read_u16(fixture.state.window, 2U) == 0x00F8U &&
                fixture.dialogs.messages.front().record.role_index == 1U &&
                fixture.roles[1].interaction_gate == 2U,
            "raw aliases share mode-two semantics and rewrite FFF0 in place"
        );
    }

    Fixture chained;
    const std::size_t end =
        write_dialog_instruction(chained, 90U, 0x00F8U, text);
    write_u16(chained.state.window, end, 194U);
    const auto dialog = chained.step();
    const auto invalid = chained.step();
    test.expect_true(
        dialog.status == LegacyWorldStoryVmStatus::yielded &&
            invalid.status == LegacyWorldStoryVmStatus::yielded &&
            invalid.invalid_opcode_previous == 90U &&
            invalid.invalid_opcode_current == 194U &&
            chained.state.previous_opcode == 194U,
        "dialog common join publishes its opcode before a later default"
    );
}

void test_dialog_flag18_suppression_protocol(openswd3::test::Context& test) {
    constexpr std::array<u8, 2U> text{'%', 'Q'};

    Fixture suppressed;
    write_dialog_instruction(suppressed, 2U, 0x00F8U, text);
    suppressed.state.next_dialog_flag18_suppression = 1U;
    const auto suppressed_result = suppressed.step();
    test.expect_true(
        suppressed_result.status == LegacyWorldStoryVmStatus::yielded &&
            (suppressed.dialogs.messages.front().record.flags & 0x00040000U) ==
                0U &&
            suppressed.state.next_dialog_flag18_suppression == 0U,
        "dialog flag bit 18 is suppressed only when the one-shot value equals one"
    );

    Fixture other_value;
    write_dialog_instruction(other_value, 2U, 0x00F8U, text);
    other_value.state.next_dialog_flag18_suppression = 2U;
    const auto other_value_result = other_value.step();
    test.expect_true(
        other_value_result.status == LegacyWorldStoryVmStatus::yielded &&
            (other_value.dialogs.messages.front().record.flags & 0x00040000U) !=
                0U &&
            other_value.state.next_dialog_flag18_suppression == 0U,
        "dialog flag bit 18 remains set for a nonzero one-shot value other than one"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, 2U);
    write_u16(missing_terminator.state.window, 2U, 0x00F8U);
    write_u16(missing_terminator.state.window, 4U, 0x232DU);
    missing_terminator.state.next_dialog_flag18_suppression = 1U;
    const auto missing_terminator_result = missing_terminator.step();
    test.expect_true(
        missing_terminator_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_terminator_result.direct_audio_service_count == 1U &&
            missing_terminator.dialogs.messages.empty() &&
            missing_terminator.state.next_dialog_flag18_suppression == 1U,
        "dialog failure before queueing preserves the flag bit 18 one-shot value"
    );
}

void test_clear_dialog_control_flag(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0007U, 0x4007U, 0x8007U, 0xC007U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.text_control_flags = 0x92345678U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0x12345678U &&
                fixture.state.previous_opcode == 7U &&
                result.direct_audio_service_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 7 aliases clear bit 31 and continue without audio"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 7U);
    write_u16(chained.state.window, 2U, 194U);
    const auto result = chained.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 2U &&
            result.invalid_opcode_previous == 7U &&
            result.invalid_opcode_current == 194U && result.beep_count == 1U &&
            result.direct_audio_service_count == 1U &&
            chained.context.instruction_offset == 2U &&
            chained.state.previous_opcode == 194U,
        "opcode 7 publishes previous before the same-call next fetch"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 7U);
    write_u16(dialog.state.window, 2U, 2U);
    write_u16(dialog.state.window, 4U, 0x00F8U);
    write_u16(dialog.state.window, 6U, 0x232DU);
    dialog.state.window[8U] = '%';
    dialog.state.window[9U] = 'Q';
    const auto dialog_result = dialog.step();
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog_result.dialog_enqueue_count == 1U &&
            dialog.context.instruction_offset == 10U &&
            (dialog.dialogs.messages.front().record.flags & 0x20U) != 0U &&
            dialog.state.text_control_flags == 0xFFFFFFFFU &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog observes opcode 7 bit 31 clear before resetting it"
    );
}

void test_clear_dialog_control_flag_bit30(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0009U, 0x4009U, 0x8009U, 0xC009U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.text_control_flags = 0xD2345678U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0x92345678U &&
                fixture.state.previous_opcode == 9U &&
                result.direct_audio_service_count == 0U,
            "opcode 9 aliases clear only bit 30 and continue without audio"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 9U);
    write_u16(chained.state.window, 2U, 194U);
    const auto result = chained.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 2U &&
            result.invalid_opcode_previous == 9U &&
            result.invalid_opcode_current == 194U &&
            chained.context.instruction_offset == 2U,
        "opcode 9 publishes previous before the same-call next fetch"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 9U);
    write_u16(dialog.state.window, 2U, 2U);
    write_u16(dialog.state.window, 4U, 0x00F8U);
    write_u16(dialog.state.window, 6U, 0x232DU);
    dialog.state.window[8U] = '%';
    dialog.state.window[9U] = 'Q';
    const auto dialog_result = dialog.step();
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog.context.instruction_offset == 10U &&
            (dialog.dialogs.messages.front().record.flags & 0x400U) != 0U &&
            dialog.state.text_control_flags == 0xFFFFFFFFU &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog observes opcode 9 bit 30 clear before resetting it"
    );
}

void test_stage_dialog_lifetime(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0008U, 0x4008U, 0x8008U, 0xC008U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFFFU);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 4U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.next_text_aux_pending &&
                fixture.state.next_text_aux_value == 0xFFFFU &&
                fixture.state.previous_opcode == 8U &&
                result.direct_audio_service_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 8 aliases stage an unsigned word and continue without audio"
        );
    }

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, 8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            !truncated.state.next_text_aux_pending &&
            truncated.state.next_text_aux_value == 60U &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.direct_audio_service_count == 0U,
        "opcode 8 short operand fails before one-shot state and join effects"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 8U);
    write_u16(dialog.state.window, 2U, 37U);
    write_u16(dialog.state.window, 4U, 2U);
    write_u16(dialog.state.window, 6U, 0x00F8U);
    write_u16(dialog.state.window, 8U, 0x232DU);
    dialog.state.window[10U] = '%';
    dialog.state.window[11U] = 'Q';
    const auto dialog_result = dialog.step();
    const auto& record = dialog.dialogs.messages.front().record;
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog.context.instruction_offset == 12U &&
            (record.flags & 0x08U) != 0U && record.lifetime_limit == 37U &&
            !dialog.state.next_text_aux_pending &&
            dialog.state.next_text_aux_value == 60U &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog consumes and resets opcode 8 one-shot lifetime"
    );
}

void test_dialog_text_preparation_and_mode_zero_metrics(
    openswd3::test::Context& test
) {
    constexpr std::array<u8, 6U> source{'%', 'T', '1', '.', '%', 'Q'};
    const std::array<std::vector<u8>, 2U> prepared{
        std::vector<u8>{'%', 'N', '%', 'N', '%', 'N', '%', 'Q'},
        std::vector<u8>{'%', 'N', '%', 'N', '%', 'N', '%', 'N', '%', 'Q'},
    };
    constexpr std::array<u16, 2U> widths{198U, 220U};
    constexpr std::array<u16, 2U> heights{88U, 110U};
    for (std::size_t index = 0U; index < prepared.size(); ++index) {
        Fixture fixture;
        write_dialog_instruction(fixture, 1U, 0x00F8U, source);
        fixture.ports.dialog_text_prepare_success = true;
        fixture.ports.prepared_dialog_text = prepared[index];
        const auto result = fixture.step();
        const auto& message = fixture.dialogs.messages.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.dialog_text_prepare_success_count == 1U &&
                fixture.ports.last_dialog_text ==
                    std::vector<u8>(source.begin(), source.end()) &&
                message.text == prepared[index] &&
                message.record.width == widths[index] &&
                message.record.height == heights[index],
            "prepared mode-zero text selects the original line-count bucket"
        );
    }

    Fixture measured;
    constexpr std::array<u8, 7U> measured_text{
        'A', 'B', 'C', 'D', 'E', '%', 'Q'
    };
    write_dialog_instruction(measured, 2U, 0x00F8U, measured_text);
    measured.state.text_control_flags &= 0xF7FFFFFFU;
    const auto result = measured.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            measured.dialogs.messages.front().record.width == 55U &&
            measured.dialogs.messages.front().record.height == 22U,
        "bit-27 clear uses measured visible bytes and a two-unit height"
    );
}

void test_dialog_anchor_delay_flag_and_reset(openswd3::test::Context& test) {
    Fixture fixture;
    constexpr std::array<u8, 2U> text{'%', 'Q'};
    write_dialog_instruction(
        fixture, 4U, 0x00F8U, text, 0x232DU, 200U, 120U, 4U, 3U
    );
    fixture.state.dialog_anchor_left = 10U;
    fixture.state.dialog_anchor_top = 20U;
    fixture.state.next_dialog_flag18_suppression = 2U;
    fixture.state.dialog_character_delay_base = 3U;
    fixture.state.text_layout_first = 7;
    fixture.state.text_layout_second = -9;
    fixture.state.speaker_name[0] = 'N';
    fixture.state.speaker_name[1] = 0U;
    fixture.state.speaker_name[2] = 0xAAU;
    const auto result = fixture.step(16, 32);
    const auto& message = fixture.dialogs.messages.front();
    const auto& record = message.record;
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            record.anchor_left == 26U && record.anchor_top == 52U &&
            record.left == 200U && record.top == 120U && record.width == 44U &&
            record.height == 33U && record.character_delay == 6U &&
            message.caption.size() == 1U &&
            fixture.state.speaker_name[0] == 0U &&
            fixture.state.speaker_name[2] == 0xAAU &&
            fixture.state.dialog_anchor_left == 0x8000U &&
            fixture.state.dialog_anchor_top == 0x8000U &&
            fixture.state.next_dialog_flag18_suppression == 0U &&
            (record.flags & 0x00040000U) != 0U &&
            fixture.state.text_layout_first == 0 &&
            fixture.state.text_layout_second == 0 &&
            fixture.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U, 4U, 4U, 2U},
        "anchor, configured delay, flag bit 18 and one-byte name reset are exact"
    );

    Fixture detached;
    write_dialog_instruction(detached, 2U, 0xFFFDU, text);
    detached.context.world_x = 64U;
    detached.context.world_y = 80U;
    const auto detached_result = detached.step();
    const auto& detached_record = detached.dialogs.messages.front().record;
    test.expect_true(
        detached_result.status == LegacyWorldStoryVmStatus::yielded &&
            detached_record.role_index == 0xFFFDU &&
            detached_record.anchor_left == 64U &&
            detached_record.anchor_top == 80U &&
            detached.context.field_26 == 2U &&
            detached.roles[1].interaction_gate == 0U,
        "FFFD uses the detached context anchor and interaction gate"
    );

    Fixture index_zero;
    write_dialog_instruction(index_zero, 90U, 0U, text);
    index_zero.roles[0].guid = 0U;
    const auto zero_result = index_zero.step();
    const auto& zero_record = index_zero.dialogs.messages.front().record;
    test.expect_true(
        zero_result.status == LegacyWorldStoryVmStatus::yielded &&
            zero_record.role_index == 0U && zero_record.left == 30U &&
            zero_record.top == 99U &&
            index_zero.roles[0].interaction_gate == 2U,
        "role index zero preserves the original skipped auto-center branch"
    );
}

void test_dialog_checked_failure_order(openswd3::test::Context& test) {
    constexpr std::array<u8, 3U> text{'A', '%', 'Q'};
    Fixture missing_role;
    write_dialog_instruction(missing_role, 90U, 0x7777U, text);
    missing_role.state.previous_opcode = 0x55U;
    missing_role.state.text_control_flags = 0x12345678U;
    missing_role.state.speaker_name[0] = 'X';
    const auto role_result = missing_role.step();
    test.expect_true(
        role_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            role_result.direct_audio_service_count == 1U &&
            role_result.dialog_text_prepare_count == 1U &&
            missing_role.dialogs.messages.empty() &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.state.text_control_flags == 0x12345678U &&
            missing_role.state.speaker_name[0] == 'X' &&
            missing_role.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U},
        "missing role stops at the caller gate write after audio and prepare"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, 90U);
    write_u16(missing_terminator.state.window, 2U, 0x00F8U);
    write_u16(missing_terminator.state.window, 4U, 0x232DU);
    write_u16(missing_terminator.state.window, 6U, 2U);
    write_u16(missing_terminator.state.window, 8U, 3U);
    missing_terminator.state.window[10U] = 'A';
    const auto terminator_result = missing_terminator.step();
    test.expect_true(
        terminator_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            terminator_result.direct_audio_service_count == 1U &&
            terminator_result.dialog_text_prepare_count == 0U &&
            missing_terminator.dialogs.messages.empty() &&
            missing_terminator.ports.story_protocol_events ==
                std::vector<u32>{2U},
        "missing percent-Q stops after the first original audio service"
    );

    Fixture allocation_failure;
    write_dialog_instruction(allocation_failure, 90U, 0x00F8U, text);
    allocation_failure.ports.throw_on_dialog_text_prepare = true;
    const auto allocation_result = allocation_failure.step();
    test.expect_true(
        allocation_result.status ==
                LegacyWorldStoryVmStatus::dialog_allocation_failed &&
            allocation_result.direct_audio_service_count == 1U &&
            allocation_result.dialog_text_prepare_count == 1U &&
            allocation_failure.dialogs.messages.empty() &&
            allocation_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U},
        "text preparation allocation failure preserves prior effects"
    );
}

void test_default_invalid_opcode_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> opcode_zero_aliases{
        0x0000U, 0x4000U, 0x8000U, 0xC000U
    };
    for (const u16 raw_word : opcode_zero_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        fixture.state.previous_opcode = 0x1234U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == raw_word && result.opcode == 0U &&
                result.instruction_offset == 0U &&
                fixture.context.instruction_offset == 0U &&
                result.executed_instruction_count == 1U &&
                result.invalid_opcode_diagnostic_count == 1U &&
                result.invalid_opcode_current == 0U &&
                result.invalid_opcode_previous == 0x1234U &&
                result.beep_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.previous_opcode == 0U &&
                fixture.ports.beep_count == 1U &&
                fixture.ports.direct_audio_service_count == 1U &&
                fixture.ports.default_protocol_events ==
                    std::vector<u32>{1U, 2U},
            "opcode zero raw aliases beep, diagnose, publish and service"
        );
    }

    constexpr std::array<u16, 4U> default_boundaries{
        194U, 1023U, 1027U, 16382U
    };
    for (const u16 opcode : default_boundaries) {
        Fixture fixture;
        prime_loaded_instruction(fixture, opcode);
        fixture.state.previous_opcode = 0x55AAU;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                fixture.context.instruction_offset == 0U &&
                result.invalid_opcode_current == opcode &&
                result.invalid_opcode_previous == 0x55AAU &&
                fixture.state.previous_opcode == opcode &&
                fixture.ports.default_protocol_events ==
                    std::vector<u32>{1U, 2U},
            "both original default ranges retain the no-advance protocol"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 194U);
    chained.state.previous_opcode = 0x55U;
    const auto first = chained.step();
    write_u16(chained.state.window, 0U, 1023U);
    const auto second = chained.step();
    test.expect_true(
        first.invalid_opcode_previous == 0x55U &&
            first.invalid_opcode_current == 194U &&
            second.invalid_opcode_previous == 194U &&
            second.invalid_opcode_current == 1023U &&
            chained.state.previous_opcode == 1023U &&
            chained.ports.default_protocol_events ==
                std::vector<u32>{1U, 2U, 1U, 2U},
        "default diagnostics observe the prior join value before publishing"
    );
}

void test_initial_flags_and_alignment_gate(openswd3::test::Context& test) {
    Fixture fixture;
    const auto initialized = fixture.state;
    fixture.roles[0].world_x = 17U;
    const auto blocked = fixture.step();
    test.expect_true(
        openswd3::world_map::query_legacy_world_story_flag(initialized, 1U) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 3U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 4U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 10U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 30U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 70U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                initialized, 2U
            ) &&
            initialized.script_variables[0] == 100U &&
            initialized.deferred_map_tile_x == -1 &&
            initialized.deferred_map_tile_y == -1 &&
            initialized.deferred_map_id == 0 &&
            std::ranges::all_of(
                initialized.script_variables.begin() + 1U,
                initialized.script_variables.end(),
                [](const u32 value) { return value == 0U; }
            ) &&
            blocked.status == LegacyWorldStoryVmStatus::yielded &&
            fixture.ports.story_load_count == 0U &&
            fixture.context.talk_data_offset == 0U,
        "sub_40E0B0 flags, initial money and first-load alignment gate are exact"
    );
}

void test_reinitialization_writes_only_owned_vm_fields(
    openswd3::test::Context& test
) {
    LegacyWorldStoryVmState state;
    state.flags.fill(0xFFU);
    state.script_variables.fill(0x12345678U);
    state.window[0] = 0xA5U;
    state.speaker_name[0] = 0x5AU;
    state.text_control_flags = 0x11223344U;
    state.wait_duration = 9U;
    state.wait_started_at = 10U;
    state.loaded_file_number = 11U;
    state.loaded_data_offset = 12U;
    state.window_loaded = true;
    state.world_music_request = 10U;
    state.world_music_first_stream = 11U;
    state.world_music_second_stream = 12U;
    state.music_request = 13U;
    state.music_first_stream = 14U;
    state.music_second_stream = 15U;
    state.music_control_flags = 16U;
    state.current_first_stream = 17U;
    state.current_stream_fade_divisor = 18U;
    state.current_second_stream = 19U;
    state.previous_opcode = 0x1234U;
    state.guid_one_action_override = 0x5678U;
    state.dialog_scale = 13U;
    state.dialog_character_delay_base = 3U;
    state.dialog_anchor_left = 21U;
    state.dialog_anchor_top = 22U;
    state.next_dialog_flag18_suppression = 2U;
    state.mode_texts[0].allocated = true;
    state.mode_texts[0].bytes.fill(0xA5U);
    state.mode_texts[1].allocated = true;
    state.mode_texts[1].bytes.fill(0x5AU);

    openswd3::world_map::initialize_legacy_world_story_vm(state);

    test.expect_true(
        state.script_variables[0] == 100U &&
            state.script_variables[1] == 0x12345678U &&
            state.window[0] == 0xA5U && state.speaker_name[0] == 0x5AU &&
            state.text_control_flags == 0x11223344U &&
            state.wait_duration == 9U && state.wait_started_at == 10U &&
            state.loaded_file_number == 11U &&
            state.loaded_data_offset == 12U && state.window_loaded &&
            state.world_music_request == 0U &&
            state.world_music_first_stream == 0U &&
            state.world_music_second_stream == 0U &&
            state.music_request == 0U && state.music_first_stream == 0U &&
            state.music_second_stream == 0U &&
            state.music_control_flags == 0U &&
            state.current_first_stream == 1U &&
            state.current_stream_fade_divisor == 0U &&
            state.current_second_stream == 0U &&
            state.previous_opcode == 0x1234U &&
            state.guid_one_action_override == 0U && state.dialog_scale == 13U &&
            state.dialog_character_delay_base == 3U &&
            state.dialog_anchor_left == 21U && state.dialog_anchor_top == 22U &&
            state.next_dialog_flag18_suppression == 2U &&
            !state.mode_texts[0].allocated && !state.mode_texts[1].allocated &&
            state.mode_texts[0].bytes.front() == 0xA5U &&
            state.mode_texts[1].bytes.front() == 0x5AU &&
            state.deferred_map_tile_x == -1 &&
            state.deferred_map_tile_y == -1 && state.deferred_map_id == 0 &&
            openswd3::world_map::query_legacy_world_story_flag(state, 70U) &&
            !openswd3::world_map::query_legacy_world_story_flag(state, 2U),
        "sub_40E0B0 rewrites only its VM globals on repeated initialization"
    );
}

void test_dialog_enqueue_and_wait_protocol(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 89U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x232DU);
    write_u16(script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 8U, 8U);
    script[10U] = 'A';
    script[11U] = '%';
    script[12U] = 'Q';
    write_u16(script, 13U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 15U, 0x00F8U);
    write_u16(script, 17U, 0xFFFFU);
    fixture.state.speaker_name[0] = 'N';
    fixture.state.speaker_name[1] = 0U;

    const auto enqueued = fixture.step(16, 32);
    const auto& message = fixture.dialogs.messages.front();
    test.expect_equal(
        enqueued.status, LegacyWorldStoryVmStatus::yielded, "opcode 89 yields"
    );
    test.expect_equal(
        enqueued.executed_instruction_count,
        1U,
        "opcode 89 counts one instruction"
    );
    test.expect_equal(
        enqueued.dialog_enqueue_count, 1U, "opcode 89 enqueues one dialog"
    );
    test.expect_equal(
        enqueued.action_update_count,
        3U,
        "initial load, frame and caption update three actions"
    );
    test.expect_equal(
        fixture.context.instruction_offset,
        u16{13U},
        "opcode 89 advances behind %Q"
    );
    test.expect_equal(
        fixture.roles[1].interaction_gate,
        u16{1U},
        "opcode 89 leaves the owner gate at one"
    );
    test.expect_true(
        (fixture.roles[1].flags & 0x00080000U) != 0U,
        "initial load marks the source role"
    );
    test.expect_equal(
        message.record.width,
        u16{154U},
        "dialog width is column count times eleven"
    );
    test.expect_equal(
        message.record.height,
        u16{88U},
        "dialog height is row count times eleven"
    );
    test.expect_equal(
        message.record.left,
        u16{227U},
        "dialog left uses role, camera and facing offset"
    );
    test.expect_equal(
        message.record.top,
        u16{260U},
        "dialog top uses role, camera and facing offset"
    );
    test.expect_equal(
        message.record.flags,
        u32{0x00040010U},
        "opcode 89 adds its default bit 18 and odd-variant flags"
    );
    test.expect_equal(
        message.record.character_delay,
        u16{4U},
        "dialog delay is twice the initialized base delay"
    );
    test.expect_true(
        message.record.saved_foreground_index == 0U &&
            message.record.saved_secondary_index == 0U &&
            message.record.text_style == 4U &&
            message.record.saved_text_style == 0U,
        "calloc leaves saved text attributes zero while sub_40AFF0 sets style four"
    );
    test.expect_equal(
        message.caption.size(),
        std::size_t{1U},
        "speaker name becomes the caption"
    );
    test.expect_equal(
        message.text.size(),
        std::size_t{3U},
        "dialog text includes its %Q terminator"
    );
    test.expect_equal(
        fixture.state.speaker_name[0],
        u8{0U},
        "speaker buffer is cleared after enqueue"
    );
    test.expect_equal(
        fixture.dialogs.close.flagged_dialog_counter,
        u32{0x8001U},
        "dialog count increments without losing story lock"
    );

    const auto waiting = fixture.step();
    const u16 waiting_instruction_offset = fixture.context.instruction_offset;
    fixture.roles[1].interaction_gate = 0U;
    const auto released = fixture.step();
    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.executed_instruction_count == 1U &&
            waiting_instruction_offset == 13U &&
            released.status == LegacyWorldStoryVmStatus::yielded &&
            released.executed_instruction_count == 1U &&
            fixture.context.instruction_offset == 17U,
        "opcode 14 stalls on gate one and advances then yields at zero"
    );
}

void test_dialog_role_overlap_avoidance(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 89U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x232DU);
    write_u16(script, 6U, 8U);
    write_u16(script, 8U, 4U);
    script[10U] = '%';
    script[11U] = 'Q';
    fixture.roles[1].world_x = 320U;
    fixture.roles[1].world_y = 30U;
    fixture.roles[1].action.variant_delta = 1U;

    const auto result = fixture.step();
    const auto& record = fixture.dialogs.messages.front().record;
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            record.left == 276U && record.top == 32U,
        "sub_40AFF0 repeats the facing offset when the first panel still overlaps its role"
    );
}

void test_dialog_explicit_layout_pair(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 104U);
    write_u16(script, 2U, 5U);
    write_u16(script, 4U, static_cast<u16>(-7));
    write_u16(script, 6U, 89U);
    write_u16(script, 8U, 0x00F8U);
    write_u16(script, 10U, 0x232DU);
    write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 14U, 8U);
    script[16U] = '%';
    script[17U] = 'Q';

    const auto result = fixture.step();
    const auto& record = fixture.dialogs.messages.front().record;

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 89U && result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 18U && record.left == 248U &&
            record.top == 189U,
        "opcode 104 replaces the second role-facing offset with its signed pair"
    );
    test.expect_true(
        fixture.state.text_control_flags == 0xFFFFFFFFU &&
            fixture.state.text_layout_first == 0 &&
            fixture.state.text_layout_second == 0,
        "dialog enqueue resets opcode 104 text globals to their legacy defaults"
    );
}

void test_story_transfer_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_161_TRANSFER_STORY | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 2042U);
        fixture.state.window[300U] = 0xA5U;
        write_u16(
            fixture.ports.transferred_window,
            0U,
            OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
        );
        write_u16(fixture.ports.transferred_window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.ports.story_load_callback = [&fixture]() {
            fixture.ports.story_protocol_events.push_back(15U);
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.raw_word == kStoryVmTypedStop &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 3U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 4U &&
                fixture.ports.direct_audio_service_count == 4U &&
                fixture.ports.story_load_count == 1U &&
                fixture.ports.last_story_id == 2042 &&
                !fixture.ports.last_story_clear_before_read &&
                fixture.context.talk_data_offset == 0x2222U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.loaded_file_number == 2U &&
                fixture.state.loaded_data_offset == 0x2222U &&
                fixture.state.window_loaded &&
                fixture.state.window[300U] == 0xA5U &&
                fixture.state.previous_opcode ==
                    OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 2U, 15U, 2U, 2U},
            "opcode 161 aliases service audio four times, preserve the unread window tail, publish previous, and same-call the transferred window"
        );
    }

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_161_TRANSFER_STORY);
    write_u16(exact_tail.state.window, 0x7FFEU, 2042U);
    write_u16(
        exact_tail.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(exact_tail.ports.transferred_window, 2U, kStoryVmTypedStop);
    exact_tail.ports.story_load_callback = [&exact_tail]() {
        exact_tail.ports.story_protocol_events.push_back(15U);
    };

    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            exact_tail_result.opcode == kStoryVmTypedStop &&
            exact_tail_result.executed_instruction_count == 3U &&
            exact_tail_result.direct_audio_service_count == 4U &&
            exact_tail.context.talk_data_offset == 0x2222U &&
            exact_tail.context.instruction_offset == 2U &&
            exact_tail.state.previous_opcode ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
            exact_tail.ports.story_protocol_events ==
                std::vector<u32>{2U, 2U, 15U, 2U, 2U},
        "opcode 161 transfers from the final complete four-byte source record before same-calling the new window"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_161_TRANSFER_STORY);

    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.opcode == OP_161_TRANSFER_STORY &&
            truncated_result.executed_instruction_count == 1U &&
            truncated_result.direct_audio_service_count == 1U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U &&
            truncated.ports.story_load_count == 0U &&
            truncated.ports.story_protocol_events == std::vector<u32>{2U},
        "opcode 161 services audio before the original operand access and typed-stops a truncated operand"
    );

    struct Failure {
        u16 story_word;
        i32 expected_story_id;
        LegacyTalkWindowStatus status;
    };
    constexpr std::array<Failure, 2U> failures{
        Failure{0xFFFFU, -1, LegacyTalkWindowStatus::invalid_story_id},
        Failure{2042U, 2042, LegacyTalkWindowStatus::open_failed},
    };
    for (const auto failure : failures) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_161_TRANSFER_STORY);
        write_u16(fixture.state.window, 2U, failure.story_word);
        fixture.state.previous_opcode = 0x66U;
        fixture.ports.story_load_status = failure.status;
        fixture.ports.story_load_callback = [&fixture]() {
            fixture.ports.story_protocol_events.push_back(15U);
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::load_failed &&
                result.opcode == OP_161_TRANSFER_STORY &&
                result.executed_instruction_count == 1U &&
                result.load_status == failure.status &&
                result.direct_audio_service_count == 2U &&
                fixture.ports.direct_audio_service_count == 2U &&
                fixture.ports.story_load_count == 1U &&
                fixture.ports.last_story_id == failure.expected_story_id &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.loaded_file_number == 1U &&
                fixture.state.window_loaded &&
                fixture.state.previous_opcode == 0x66U &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 2U, 15U},
            "opcode 161 sign-extends the story id and preserves two prior audio services at the checked load failure boundary"
        );
    }
}

void test_current_map_reload_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<u16, 2U> opcodes{
        OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL,
        OP_164_RELOAD_IF_CURRENT_MAP_EQUAL,
    };

    for (const u16 opcode : opcodes) {
        for (std::size_t index = 0U; index < alias_masks.size(); ++index) {
            Fixture fixture;
            const i32 map_id =
                opcode == OP_164_RELOAD_IF_CURRENT_MAP_EQUAL && index == 3U
                ? -1
                : 21;
            const u32 map_bits = std::bit_cast<u32>(map_id);
            fixture.runtime.current_logical_map_id =
                opcode == OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL
                ? (index == 1U ? 0x00010015U : map_bits + 1U)
                : map_bits;
            const u32 target = 0x12345670U + static_cast<u32>(index);
            prime_loaded_instruction(
                fixture, static_cast<u16>(opcode | alias_masks[index])
            );
            write_u16(fixture.state.window, 2U, static_cast<u16>(map_id));
            write_u32(fixture.state.window, 4U, target);
            fixture.state.window[300U] = 0xA5U;
            fixture.state.previous_opcode = 0x66U;
            write_u16(
                fixture.ports.transferred_window,
                0U,
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
            );
            write_u16(fixture.ports.transferred_window, 2U, kStoryVmTypedStop);

            const auto result = fixture.step();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 3U &&
                    result.load_status == LegacyTalkWindowStatus::ready &&
                    result.direct_audio_service_count == 1U &&
                    fixture.ports.direct_audio_service_count == 1U &&
                    fixture.ports.data_load_count == 1U &&
                    fixture.ports.last_data_file_number == 1U &&
                    fixture.ports.last_data_offset == target &&
                    !fixture.ports.last_data_clear_before_read &&
                    fixture.context.talk_data_offset == target &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.loaded_file_number == 1U &&
                    fixture.state.loaded_data_offset == target &&
                    fixture.state.window_loaded &&
                    fixture.state.window[300U] == 0xA5U &&
                    fixture.state.previous_opcode ==
                        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
                    fixture.ports.story_protocol_events ==
                        std::vector<u32>{2U, 5U},
                "opcodes 163 and 164 cover every raw alias, compare the signed map operand against the full current-map dword, preserve the unread window tail, and same-call the target"
            );
        }
    }

    struct SequentialCase {
        u16 opcode;
        u32 current_map_id;
    };
    constexpr std::array sequential_cases{
        SequentialCase{OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL, 21U},
        SequentialCase{OP_164_RELOAD_IF_CURRENT_MAP_EQUAL, 22U},
    };
    for (const auto test_case : sequential_cases) {
        Fixture fixture;
        fixture.runtime.current_logical_map_id = test_case.current_map_id;
        prime_loaded_instruction(fixture, test_case.opcode);
        write_u16(fixture.state.window, 2U, 21U);
        write_u32(fixture.state.window, 4U, 0xDEADBEEFU);
        write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.talk_data_offset == 0x1111U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode == test_case.opcode &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcodes 163 and 164 invert the equality predicate and same-call the eight-byte sequential successor without audio"
        );
    }

    const auto prime_tail =
        [](Fixture& fixture, const u16 ip, const u16 opcode) {
            fixture.context.talk_data_offset = 0x1111U;
            fixture.context.instruction_offset = ip;
            fixture.state.loaded_file_number = 1U;
            fixture.state.loaded_data_offset = 0x1111U;
            fixture.state.window_loaded = true;
            fixture.state.previous_opcode = 0x66U;
            write_u16(fixture.state.window, ip, opcode);
        };

    Fixture operand_truncated;
    prime_tail(
        operand_truncated, 0x7FFEU, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL
    );
    const auto operand_truncated_result = operand_truncated.step();

    Fixture target_truncated;
    prime_tail(
        target_truncated, 0x7FFAU, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL
    );
    target_truncated.runtime.current_logical_map_id = 22U;
    write_u16(target_truncated.state.window, 0x7FFCU, 21U);
    write_u16(target_truncated.state.window, 0x7FFEU, 0x5678U);
    const auto target_truncated_result = target_truncated.step();

    Fixture target_unread;
    prime_tail(target_unread, 0x7FFCU, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL);
    target_unread.runtime.current_logical_map_id = 22U;
    write_u16(target_unread.state.window, 0x7FFEU, 21U);
    const auto target_unread_result = target_unread.step();

    Fixture exact_tail;
    prime_tail(exact_tail, 0x7FF8U, OP_164_RELOAD_IF_CURRENT_MAP_EQUAL);
    exact_tail.runtime.current_logical_map_id = 0xFFFFFFFFU;
    exact_tail.state.window[300U] = 0xA5U;
    write_u16(exact_tail.state.window, 0x7FFAU, 0xFFFFU);
    write_u32(exact_tail.state.window, 0x7FFCU, 0x12345678U);
    write_u16(
        exact_tail.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(exact_tail.ports.transferred_window, 2U, kStoryVmTypedStop);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated_result.executed_instruction_count == 1U &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U &&
            target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated_result.executed_instruction_count == 1U &&
            target_truncated.context.instruction_offset == 0x7FFAU &&
            target_truncated.state.previous_opcode == 0x66U &&
            target_unread_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            target_unread_result.executed_instruction_count == 1U &&
            target_unread.context.instruction_offset == 0x8004U &&
            target_unread.state.previous_opcode ==
                OP_164_RELOAD_IF_CURRENT_MAP_EQUAL &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            exact_tail_result.opcode == kStoryVmTypedStop &&
            exact_tail_result.executed_instruction_count == 3U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.talk_data_offset == 0x12345678U &&
            exact_tail.context.instruction_offset == 2U &&
            exact_tail.state.previous_opcode ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
            exact_tail.state.window[300U] == 0xA5U &&
            operand_truncated.ports.data_load_count == 0U &&
            target_truncated.ports.data_load_count == 0U &&
            target_unread.ports.data_load_count == 0U &&
            exact_tail.ports.data_load_count == 1U,
        "opcodes 163 and 164 stage operand and target reads, leave the not-taken target unread, and preserve exact-tail same-call behavior"
    );

    Fixture load_failure;
    load_failure.runtime.current_logical_map_id = 22U;
    prime_loaded_instruction(
        load_failure, OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL
    );
    write_u16(load_failure.state.window, 2U, 21U);
    write_u32(load_failure.state.window, 4U, 0x12345678U);
    load_failure.state.previous_opcode = 0x66U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;

    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x12345678U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL &&
            !load_failure.state.window_loaded &&
            load_failure.state.loaded_file_number == 1U &&
            load_failure.state.loaded_data_offset == 0x1111U &&
            load_failure.ports.data_load_count == 1U &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcodes 163 and 164 preserve audio, context writes, previous publication, and stale loaded ownership at the checked same-file load failure boundary"
    );
}

void test_item_total_reload_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<u16, 2U> opcodes{
        OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST,
        OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST,
    };
    constexpr u16 item_id = 0x0123U;
    constexpr u32 target = 0x12345678U;

    for (const u16 opcode : opcodes) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            auto& player_item = fixture.player_inventory.emplace_back();
            player_item.item_id = 0xC123U;
            player_item.selected_count = 0xFFFFU;
            player_item.quantity_a = 5U;
            player_item.quantity_b = 0xFFFEU;
            auto& role_item = fixture.item_lists.role_item_lists[2U]->sentinel;
            role_item.item_id = item_id;
            role_item.selected_count = 0xFFFFU;
            role_item.quantity_a = 4U;
            role_item.quantity_b = 5U;
            prime_loaded_instruction(
                fixture, static_cast<u16>(opcode | alias_mask)
            );
            write_u16(fixture.state.window, 2U, item_id);
            write_u16(fixture.state.window, 4U, 12U);
            write_u32(fixture.state.window, 6U, target);
            fixture.state.window[300U] = 0xA5U;
            fixture.state.previous_opcode = 0x66U;
            write_u16(
                fixture.ports.transferred_window,
                0U,
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
            );
            write_u16(fixture.ports.transferred_window, 2U, kStoryVmTypedStop);

            const auto result = fixture.step();
            test.expect_true(
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
                    fixture.ports.story_protocol_events ==
                        std::vector<u32>{2U, 5U},
                "opcodes 165 and 166 cover every raw alias, sum signed player and role-root quantities, ignore selected-count fields, and take equality at both threshold boundaries"
            );
        }
    }

    Fixture at_least_miss;
    auto& low_player = at_least_miss.player_inventory.emplace_back();
    low_player.item_id = 0xC123U;
    low_player.quantity_b = 0xFFFEU;
    prime_loaded_instruction(
        at_least_miss, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(at_least_miss.state.window, 2U, item_id);
    write_u16(at_least_miss.state.window, 4U, 0xFFFFU);
    write_u32(at_least_miss.state.window, 6U, 0xDEADBEEFU);
    write_u16(at_least_miss.state.window, 10U, kStoryVmTypedStop);
    const auto at_least_miss_result = at_least_miss.step();

    Fixture at_most_miss;
    auto& high_player = at_most_miss.player_inventory.emplace_back();
    high_player.item_id = 0xC123U;
    high_player.quantity_a = 13U;
    prime_loaded_instruction(at_most_miss, OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST);
    write_u16(at_most_miss.state.window, 2U, item_id);
    write_u16(at_most_miss.state.window, 4U, 12U);
    write_u32(at_most_miss.state.window, 6U, 0xDEADBEEFU);
    write_u16(at_most_miss.state.window, 10U, kStoryVmTypedStop);
    const auto at_most_miss_result = at_most_miss.step();

    test.expect_true(
        at_least_miss_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            at_least_miss_result.executed_instruction_count == 2U &&
            at_least_miss.context.instruction_offset == 10U &&
            at_least_miss.state.previous_opcode ==
                OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST &&
            at_least_miss.ports.data_load_count == 0U &&
            at_most_miss_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            at_most_miss_result.executed_instruction_count == 2U &&
            at_most_miss.context.instruction_offset == 10U &&
            at_most_miss.state.previous_opcode ==
                OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST &&
            at_most_miss.ports.data_load_count == 0U,
        "opcodes 165 and 166 compare nonzero totals as signed at-least and at-most predicates and same-call the ten-byte sequential successor"
    );

    Fixture zero_at_least;
    auto& flagged_role_root =
        zero_at_least.item_lists.role_item_lists[0U]->sentinel;
    flagged_role_root.item_id = 0xC123U;
    flagged_role_root.quantity_a = 99U;
    auto& linked_role_item =
        zero_at_least.item_lists.role_item_lists[0U]->nodes.emplace_back();
    linked_role_item.item_id = item_id;
    linked_role_item.quantity_a = 99U;
    prime_loaded_instruction(
        zero_at_least, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(zero_at_least.state.window, 2U, item_id);
    write_u16(zero_at_least.state.window, 4U, 0x8000U);
    write_u16(zero_at_least.state.window, 10U, kStoryVmTypedStop);
    const auto zero_at_least_result = zero_at_least.step();

    Fixture zero_at_most;
    auto& cancelling_player = zero_at_most.player_inventory.emplace_back();
    cancelling_player.item_id = 0xC123U;
    cancelling_player.quantity_a = 5U;
    cancelling_player.quantity_b = 0xFFFBU;
    prime_loaded_instruction(zero_at_most, OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST);
    write_u16(zero_at_most.state.window, 2U, item_id);
    write_u16(zero_at_most.state.window, 4U, 0x8000U);
    write_u32(zero_at_most.state.window, 6U, target);
    write_u16(
        zero_at_most.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(zero_at_most.ports.transferred_window, 2U, kStoryVmTypedStop);
    const auto zero_at_most_result = zero_at_most.step();

    test.expect_true(
        zero_at_least_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_at_least_result.executed_instruction_count == 2U &&
            zero_at_least.context.instruction_offset == 10U &&
            zero_at_least.ports.data_load_count == 0U &&
            zero_at_most_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_at_most_result.executed_instruction_count == 3U &&
            zero_at_most_result.direct_audio_service_count == 1U &&
            zero_at_most.context.talk_data_offset == target &&
            zero_at_most.ports.data_load_count == 1U &&
            zero_at_most.state.previous_opcode ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL,
        "zero totals preserve the original opcode-165 sequential and opcode-166 reload special cases regardless of signed threshold, while role roots use exact IDs and ignore linked nodes"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    selector_truncated.state.previous_opcode = 0x66U;
    selector_truncated.runtime.player_inventory = nullptr;
    selector_truncated.runtime.role_item_lists = nullptr;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    const auto selector_truncated_result = selector_truncated.step();

    Fixture missing_player_owner;
    missing_player_owner.runtime.player_inventory = nullptr;
    prime_loaded_instruction(
        missing_player_owner, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(missing_player_owner.state.window, 2U, item_id);
    const auto missing_player_owner_result = missing_player_owner.step();

    Fixture missing_role_owner;
    missing_role_owner.player_inventory.emplace_back().item_id = 0xC123U;
    missing_role_owner.runtime.role_item_lists = nullptr;
    prime_loaded_instruction(
        missing_role_owner, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(missing_role_owner.state.window, 2U, item_id);
    const auto missing_role_owner_result = missing_role_owner.step();

    Fixture missing_role_root;
    missing_role_root.item_lists.role_item_lists[0U].reset();
    missing_role_root.item_lists.role_item_lists[1U]->sentinel.item_id =
        item_id;
    prime_loaded_instruction(
        missing_role_root, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(missing_role_root.state.window, 2U, item_id);
    const auto missing_role_root_result = missing_role_root.step();

    Fixture early_role_match;
    auto& early_item =
        early_role_match.item_lists.role_item_lists[0U]->sentinel;
    early_item.item_id = item_id;
    early_item.quantity_a = 1U;
    early_role_match.item_lists.role_item_lists[1U].reset();
    prime_loaded_instruction(
        early_role_match, OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(early_role_match.state.window, 2U, item_id);
    write_u16(early_role_match.state.window, 4U, 1U);
    write_u32(early_role_match.state.window, 6U, target);
    write_u16(
        early_role_match.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(early_role_match.ports.transferred_window, 2U, kStoryVmTypedStop);
    const auto early_role_match_result = early_role_match.step();

    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.state.previous_opcode == 0x66U &&
            missing_player_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_root_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            early_role_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            early_role_match_result.executed_instruction_count == 3U &&
            early_role_match.ports.data_load_count == 1U,
        "opcodes 165 and 166 read the item operand before owners, query player before role roots, typed-stop at the first missing root, and return before later missing roots after a match"
    );

    Fixture threshold_truncated;
    threshold_truncated.context.instruction_offset = 0x7FFCU;
    threshold_truncated.context.talk_data_offset = 0x1111U;
    threshold_truncated.state.loaded_file_number = 1U;
    threshold_truncated.state.loaded_data_offset = 0x1111U;
    threshold_truncated.state.window_loaded = true;
    threshold_truncated.state.previous_opcode = 0x66U;
    write_u16(
        threshold_truncated.state.window,
        0x7FFCU,
        OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(threshold_truncated.state.window, 0x7FFEU, item_id);
    const auto threshold_truncated_result = threshold_truncated.step();

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFAU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.previous_opcode = 0x66U;
    write_u16(
        target_truncated.state.window,
        0x7FFAU,
        OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST
    );
    write_u16(target_truncated.state.window, 0x7FFCU, item_id);
    write_u16(target_truncated.state.window, 0x7FFEU, 0x8000U);
    const auto target_truncated_result = target_truncated.step();

    Fixture target_unread;
    target_unread.context.instruction_offset = 0x7FFAU;
    target_unread.context.talk_data_offset = 0x1111U;
    target_unread.state.loaded_file_number = 1U;
    target_unread.state.loaded_data_offset = 0x1111U;
    target_unread.state.window_loaded = true;
    target_unread.state.previous_opcode = 0x66U;
    write_u16(
        target_unread.state.window,
        0x7FFAU,
        OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST
    );
    write_u16(target_unread.state.window, 0x7FFCU, item_id);
    write_u16(target_unread.state.window, 0x7FFEU, 0x8000U);
    const auto target_unread_result = target_unread.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    exact_tail.state.window[300U] = 0xA5U;
    write_u16(
        exact_tail.state.window, 0x7FF6U, OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST
    );
    write_u16(exact_tail.state.window, 0x7FF8U, item_id);
    write_u16(exact_tail.state.window, 0x7FFAU, 0x8000U);
    write_u32(exact_tail.state.window, 0x7FFCU, target);
    write_u16(
        exact_tail.ports.transferred_window,
        0U,
        OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL
    );
    write_u16(exact_tail.ports.transferred_window, 2U, kStoryVmTypedStop);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        threshold_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            threshold_truncated.context.instruction_offset == 0x7FFCU &&
            threshold_truncated.state.previous_opcode == 0x66U &&
            target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFAU &&
            target_truncated.state.previous_opcode == 0x66U &&
            target_unread_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            target_unread.context.instruction_offset == 0x8004U &&
            target_unread.state.previous_opcode ==
                OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            exact_tail_result.opcode == kStoryVmTypedStop &&
            exact_tail_result.executed_instruction_count == 3U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.talk_data_offset == target &&
            exact_tail.context.instruction_offset == 2U &&
            exact_tail.state.previous_opcode ==
                OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL &&
            exact_tail.state.window[300U] == 0xA5U &&
            threshold_truncated.ports.data_load_count == 0U &&
            target_truncated.ports.data_load_count == 0U &&
            target_unread.ports.data_load_count == 0U &&
            exact_tail.ports.data_load_count == 1U,
        "opcodes 165 and 166 stage threshold and taken-only target reads, leave the zero-at-least target unread, and preserve exact-tail same-call behavior"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST);
    write_u16(load_failure.state.window, 2U, item_id);
    write_u16(load_failure.state.window, 4U, 0x8000U);
    write_u32(load_failure.state.window, 6U, target);
    load_failure.state.previous_opcode = 0x66U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();

    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST &&
            !load_failure.state.window_loaded &&
            load_failure.state.loaded_data_offset == 0x1111U &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcodes 165 and 166 preserve audio, context writes, previous publication, and stale loaded ownership at the checked same-file load failure boundary"
    );
}

void test_mode_text_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        std::size_t text_index;
        u16 flag_index;
        bool stores_text;
    };
    constexpr std::array variants{
        Variant{OP_170_CLEAR_MODE17_TEXT, 0U, 77U, false},
        Variant{OP_171_SET_MODE17_TEXT, 0U, 77U, true},
        Variant{OP_172_CLEAR_MODE18_TEXT, 1U, 78U, false},
        Variant{OP_173_SET_MODE18_TEXT, 1U, 78U, true},
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
            fixture.state.mode_texts[0].allocated = true;
            fixture.state.mode_texts[0].bytes.fill(0x11U);
            fixture.state.mode_texts[1].allocated = true;
            fixture.state.mode_texts[1].bytes.fill(0x22U);
            openswd3::world_map::set_legacy_world_story_flag(
                fixture.state, 77U
            );
            openswd3::world_map::set_legacy_world_story_flag(
                fixture.state, 78U
            );
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias_mask)
            );

            const std::size_t successor_offset = variant.stores_text ? 7U : 2U;
            if (variant.stores_text) {
                openswd3::world_map::clear_legacy_world_story_flag(
                    fixture.state, variant.flag_index
                );
                fixture.state.window[2U] = static_cast<u8>('A');
                fixture.state.window[3U] = 0U;
                fixture.state.window[4U] = static_cast<u8>('B');
                fixture.state.window[5U] = static_cast<u8>('%');
                fixture.state.window[6U] = static_cast<u8>('Q');
            }
            write_u16(
                fixture.state.window, successor_offset, kStoryVmTypedStop
            );
            fixture.state.previous_opcode = 0x66U;

            const auto result = fixture.step();
            const auto& selected = fixture.state.mode_texts[variant.text_index];
            const auto& other =
                fixture.state.mode_texts[variant.text_index ^ 1U];
            const auto successor = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.executed_instruction_count == 1U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.context.instruction_offset == successor_offset &&
                    selected.allocated == variant.stores_text &&
                    (!variant.stores_text ||
                     (selected.bytes[0U] == static_cast<u8>('A') &&
                      selected.bytes[1U] == 0U && selected.bytes[2U] == 0U &&
                      selected.bytes.back() == 0U)) &&
                    other.allocated &&
                    other.bytes.front() ==
                        (variant.text_index == 0U ? 0x22U : 0x11U) &&
                    openswd3::world_map::query_legacy_world_story_flag(
                        fixture.state, variant.flag_index
                    ) == variant.stores_text &&
                    openswd3::world_map::query_legacy_world_story_flag(
                        fixture.state, static_cast<u16>(variant.flag_index ^ 3U)
                    ) &&
                    fixture.state.previous_opcode == variant.opcode &&
                    successor.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    successor.opcode == kStoryVmTypedStop &&
                    successor.executed_instruction_count == 1U,
                "opcodes 170 through 173 cover every raw alias, isolate the mode-17 and mode-18 slots, preserve lstrcpyA embedded-NUL truncation, publish previous, service audio, and yield before the successor"
            );
        }
    }

    Fixture empty_text;
    prime_loaded_instruction(empty_text, OP_171_SET_MODE17_TEXT);
    empty_text.state.window[2U] = static_cast<u8>('%');
    empty_text.state.window[3U] = static_cast<u8>('Q');
    const auto empty_text_result = empty_text.step();

    Fixture long_after_nul;
    prime_loaded_instruction(long_after_nul, OP_173_SET_MODE18_TEXT);
    std::ranges::fill_n(
        long_after_nul.state.window.begin() + 2U, 60U, static_cast<u8>('A')
    );
    long_after_nul.state.window[3U] = 0U;
    long_after_nul.state.window[62U] = static_cast<u8>('%');
    long_after_nul.state.window[63U] = static_cast<u8>('Q');
    const auto long_after_nul_result = long_after_nul.step();

    test.expect_true(
        empty_text_result.status == LegacyWorldStoryVmStatus::yielded &&
            empty_text.context.instruction_offset == 4U &&
            empty_text.state.mode_texts[0].allocated &&
            std::ranges::all_of(
                empty_text.state.mode_texts[0].bytes,
                [](const u8 byte) { return byte == 0U; }
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                empty_text.state, 77U
            ) &&
            long_after_nul_result.status == LegacyWorldStoryVmStatus::yielded &&
            long_after_nul.context.instruction_offset == 64U &&
            long_after_nul.state.mode_texts[1].allocated &&
            long_after_nul.state.mode_texts[1].bytes[0U] ==
                static_cast<u8>('A') &&
            std::ranges::all_of(
                long_after_nul.state.mode_texts[1].bytes.begin() + 1U,
                long_after_nul.state.mode_texts[1].bytes.end(),
                [](const u8 byte) { return byte == 0U; }
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                long_after_nul.state, 78U
            ),
        "odd mode-text variants accept an empty percent-Q record and scan through bytes after an embedded NUL while lstrcpyA copies only the prefix"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, OP_171_SET_MODE17_TEXT);
    missing_terminator.context.instruction_offset = 0x7FFDU;
    missing_terminator.state.previous_opcode = 0x66U;
    missing_terminator.state.mode_texts[0].allocated = true;
    missing_terminator.state.mode_texts[0].bytes.fill(0xA5U);
    openswd3::world_map::set_legacy_world_story_flag(
        missing_terminator.state, 77U
    );
    write_u16(missing_terminator.state.window, 0x7FFDU, OP_171_SET_MODE17_TEXT);
    missing_terminator.state.window[0x7FFFU] = static_cast<u8>('A');
    const auto missing_terminator_result = missing_terminator.step();

    Fixture copy_overflow;
    prime_loaded_instruction(copy_overflow, OP_171_SET_MODE17_TEXT);
    copy_overflow.state.previous_opcode = 0x66U;
    copy_overflow.state.mode_texts[0].allocated = true;
    copy_overflow.state.mode_texts[0].bytes.fill(0xCCU);
    std::ranges::fill_n(
        copy_overflow.state.window.begin() + 2U, 52U, static_cast<u8>('X')
    );
    copy_overflow.state.window[54U] = static_cast<u8>('%');
    copy_overflow.state.window[55U] = static_cast<u8>('Q');
    const auto copy_overflow_result = copy_overflow.step();

    test.expect_true(
        missing_terminator_result.status ==
                LegacyWorldStoryVmStatus::mode_text_terminator_not_found &&
            missing_terminator.context.instruction_offset == 0x7FFDU &&
            missing_terminator.state.previous_opcode == 0x66U &&
            missing_terminator.state.mode_texts[0].allocated &&
            missing_terminator.state.mode_texts[0].bytes.front() == 0xA5U &&
            openswd3::world_map::query_legacy_world_story_flag(
                missing_terminator.state, 77U
            ) &&
            missing_terminator_result.direct_audio_service_count == 0U,
        "odd mode-text variants leave the old owner, flag, IP, previous, and audio state unchanged when the terminator read fails"
    );
    test.expect_true(
        copy_overflow_result.status ==
                LegacyWorldStoryVmStatus::mode_text_out_of_range &&
            copy_overflow.context.instruction_offset == 56U &&
            copy_overflow.state.previous_opcode == 0x66U &&
            copy_overflow.state.mode_texts[0].allocated &&
            std::ranges::all_of(
                copy_overflow.state.mode_texts[0].bytes,
                [](const u8 byte) { return byte == static_cast<u8>('X'); }
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                copy_overflow.state, 77U
            ) &&
            copy_overflow_result.direct_audio_service_count == 0U,
        "odd mode-text variants commit IP, replacement allocation, zero-fill, and 52 copied bytes before the lstrcpyA terminator overflow typed-stop"
    );

    Fixture clear_exact_tail;
    prime_loaded_instruction(clear_exact_tail, OP_172_CLEAR_MODE18_TEXT);
    clear_exact_tail.context.instruction_offset = 0x7FFEU;
    clear_exact_tail.state.previous_opcode = 0x66U;
    clear_exact_tail.state.mode_texts[1].allocated = true;
    openswd3::world_map::set_legacy_world_story_flag(
        clear_exact_tail.state, 78U
    );
    write_u16(clear_exact_tail.state.window, 0x7FFEU, OP_172_CLEAR_MODE18_TEXT);
    const auto clear_exact_tail_result = clear_exact_tail.step();

    Fixture set_exact_tail;
    prime_loaded_instruction(set_exact_tail, OP_173_SET_MODE18_TEXT);
    set_exact_tail.context.instruction_offset = 0x7FFCU;
    set_exact_tail.state.previous_opcode = 0x66U;
    write_u16(set_exact_tail.state.window, 0x7FFCU, OP_173_SET_MODE18_TEXT);
    set_exact_tail.state.window[0x7FFEU] = static_cast<u8>('%');
    set_exact_tail.state.window[0x7FFFU] = static_cast<u8>('Q');
    const auto set_exact_tail_result = set_exact_tail.step();

    test.expect_true(
        clear_exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            clear_exact_tail_result.executed_instruction_count == 1U &&
            clear_exact_tail_result.direct_audio_service_count == 1U &&
            clear_exact_tail.context.instruction_offset == 0x8000U &&
            clear_exact_tail.state.previous_opcode ==
                OP_172_CLEAR_MODE18_TEXT &&
            !clear_exact_tail.state.mode_texts[1].allocated &&
            !openswd3::world_map::query_legacy_world_story_flag(
                clear_exact_tail.state, 78U
            ),
        "fixed mode-text clear completes owner, flag, previous, audio, and yield at the exact window tail"
    );
    test.expect_true(
        set_exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            set_exact_tail_result.executed_instruction_count == 1U &&
            set_exact_tail_result.direct_audio_service_count == 1U &&
            set_exact_tail.context.instruction_offset == 0x8000U &&
            set_exact_tail.state.previous_opcode == OP_173_SET_MODE18_TEXT &&
            set_exact_tail.state.mode_texts[1].allocated &&
            openswd3::world_map::query_legacy_world_story_flag(
                set_exact_tail.state, 78U
            ),
        "variable mode-text set completes owner, flag, previous, audio, and yield at the exact window tail"
    );
}

void test_suspend_story_ani_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_175_SUSPEND_STORY_ANI | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.ports.ani_control_flags = 0xA5A50001U;
        u32 flags_at_write{};
        u16 ip_at_write{0xFFFFU};
        u32 previous_at_write{};
        fixture.ports.story_ani_suspend_callback = [&] {
            flags_at_write = fixture.ports.ani_control_flags;
            ip_at_write = fixture.context.instruction_offset;
            previous_at_write = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_175_SUSPEND_STORY_ANI &&
                fixture.ports.ani_suspend_write_count == 1U &&
                fixture.ports.ani_control_flags == 0xA5A50011U &&
                flags_at_write == 0xA5A50011U && ip_at_write == 0U &&
                previous_at_write == 0x66U,
            "opcode 175 covers every raw alias, ORs only ANI control bit4 across the full dword, writes before IP and previous, and same-calls without audio"
        );
    }

    Fixture idempotent;
    prime_loaded_instruction(idempotent, OP_175_SUSPEND_STORY_ANI);
    write_u16(idempotent.state.window, 2U, kStoryVmTypedStop);
    idempotent.ports.ani_control_flags = 0xFFFFFFFFU;
    const auto idempotent_result = idempotent.step();

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_175_SUSPEND_STORY_ANI);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.state.previous_opcode = 0x66U;
    exact_tail.ports.ani_control_flags = 0x80000000U;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_175_SUSPEND_STORY_ANI);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        idempotent_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            idempotent.ports.ani_control_flags == 0xFFFFFFFFU &&
            idempotent.ports.ani_suspend_write_count == 1U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_175_SUSPEND_STORY_ANI &&
            exact_tail.ports.ani_control_flags == 0x80000010U &&
            exact_tail.ports.ani_suspend_write_count == 1U,
        "opcode 175 is idempotent and completes its full-dword flag write, IP advance, and previous publication before an exact-tail successor fetch failure"
    );
}

void test_resume_story_ani_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_176_RESUME_STORY_ANI | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.ports.ani_control_flags = 0xA5A50011U;
        u32 flags_at_write{};
        u16 ip_at_write{0xFFFFU};
        u32 previous_at_write{};
        u32 audio_at_write{};
        u32 flags_at_audio{};
        u16 ip_at_audio{};
        u32 previous_at_audio{};
        fixture.ports.story_ani_suspend_callback = [&] {
            flags_at_write = fixture.ports.ani_control_flags;
            ip_at_write = fixture.context.instruction_offset;
            previous_at_write = fixture.state.previous_opcode;
            audio_at_write = fixture.ports.direct_audio_service_count;
        };
        fixture.ports.audio_service_callback = [&] {
            flags_at_audio = fixture.ports.ani_control_flags;
            ip_at_audio = fixture.context.instruction_offset;
            previous_at_audio = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_176_RESUME_STORY_ANI &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_176_RESUME_STORY_ANI &&
                fixture.ports.ani_suspend_write_count == 1U &&
                fixture.ports.ani_control_flags == 0xA5A50001U &&
                flags_at_write == 0xA5A50001U && ip_at_write == 0U &&
                previous_at_write == 0x66U && audio_at_write == 0U &&
                flags_at_audio == 0xA5A50001U && ip_at_audio == 2U &&
                previous_at_audio == OP_176_RESUME_STORY_ANI,
            "opcode 176 covers every raw alias, clears only ANI control bit4 across the full dword before IP and previous, then services audio and yields without fetching the successor"
        );
    }

    Fixture idempotent;
    prime_loaded_instruction(idempotent, OP_176_RESUME_STORY_ANI);
    idempotent.ports.ani_control_flags = 0xA5A50001U;
    const auto idempotent_result = idempotent.step();

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_176_RESUME_STORY_ANI);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.state.previous_opcode = 0x66U;
    exact_tail.ports.ani_control_flags = 0xFFFFFFFFU;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_176_RESUME_STORY_ANI);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        idempotent_result.status == LegacyWorldStoryVmStatus::yielded &&
            idempotent.ports.ani_control_flags == 0xA5A50001U &&
            idempotent.ports.ani_suspend_write_count == 1U &&
            idempotent.ports.direct_audio_service_count == 1U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_176_RESUME_STORY_ANI &&
            exact_tail.ports.ani_control_flags == 0xFFFFFFEFU &&
            exact_tail.ports.ani_suspend_write_count == 1U,
        "opcode 176 is idempotent and yields after its full-dword flag clear, IP advance, previous publication, and audio at the exact window tail"
    );
}

void test_gather_party_at_player_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 2U> setup_alias_masks{0U, 0x4000U};
    for (const u16 alias_mask : setup_alias_masks) {
        Fixture fixture;
        fixture.roles.resize(4U);
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE0005U;
        fixture.roles[0].world_x = 0x12345678U;
        fixture.roles[0].world_y = 0x9ABCDEF0U;
        fixture.roles[0].action.base_variant = 77U;
        fixture.roles[1].flags = 0x80000081U;
        fixture.roles[1].action.wait_override = 7U;
        fixture.roles[2].flags = 0x12345600U;
        fixture.roles[2].action.wait_override = 8U;
        fixture.roles[3].flags = 0x04000080U;
        fixture.roles[3].action.wait_override = 9U;
        fixture.player_post_frame.world_x_history.fill(0x11111111U);
        fixture.player_post_frame.world_y_history.fill(0x22222222U);
        bool audio_after_setup{};
        fixture.ports.audio_service_callback = [&] {
            audio_after_setup =
                read_u16(fixture.state.window, 0U) ==
                    static_cast<u16>(
                        OP_177_GATHER_PARTY_AT_PLAYER | alias_mask | 0x8000U
                    ) &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_177_GATHER_PARTY_AT_PLAYER &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE8005U &&
                fixture.roles[0].action.base_variant == 0U &&
                fixture.roles[1].flags == 0x84000081U &&
                fixture.roles[1].action.wait_override == 0x8000U;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_177_GATHER_PARTY_AT_PLAYER &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_177_GATHER_PARTY_AT_PLAYER &&
                read_u16(fixture.state.window, 0U) ==
                    static_cast<u16>(
                        OP_177_GATHER_PARTY_AT_PLAYER | alias_mask | 0x8000U
                    ) &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE8005U &&
                fixture.roles[0].action.base_variant == 0U &&
                std::ranges::all_of(
                    fixture.player_post_frame.world_x_history,
                    [](const u32 value) { return value == 0x12345678U; }
                ) &&
                std::ranges::all_of(
                    fixture.player_post_frame.world_y_history,
                    [](const u32 value) { return value == 0x9ABCDEF0U; }
                ) &&
                fixture.roles[1].flags == 0x84000081U &&
                fixture.roles[1].action.wait_override == 0x8000U &&
                fixture.roles[2].flags == 0x12345600U &&
                fixture.roles[2].action.wait_override == 8U &&
                fixture.roles[3].flags == 0x04000080U &&
                fixture.roles[3].action.wait_override == 0x8000U &&
                audio_after_setup,
            "opcode 177 base aliases self-mark setup, preserve alias bit14, reset the player action and histories, mark only secondary party roles, then publish previous and audio-yield at the same IP"
        );
    }

    constexpr std::array<u16, 2U> poll_alias_masks{0x8000U, 0xC000U};
    for (const u16 alias_mask : poll_alias_masks) {
        Fixture fixture;
        fixture.roles.resize(4U);
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | alias_mask)
        );
        fixture.state.previous_opcode = 0x66U;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE0005U;
        fixture.roles[0].world_x = 100U;
        fixture.roles[0].world_y = 200U;
        fixture.roles[0].action.base_variant = 77U;
        fixture.roles[1].world_x = 100U;
        fixture.roles[1].world_y = 200U;
        fixture.roles[1].flags = 0x84000080U;
        fixture.roles[1].action.wait_override = 0x8000U;
        fixture.roles[2].world_x = 101U;
        fixture.roles[2].world_y = 200U;
        fixture.roles[2].flags = 0x04000080U;
        fixture.roles[2].action.wait_override = 0x8000U;
        fixture.roles[3].world_x = 100U;
        fixture.roles[3].world_y = 200U;
        fixture.roles[3].flags = 0x04000000U;
        fixture.roles[3].action.wait_override = 0x8000U;
        fixture.live_party_role_count = 3U;
        fixture.runtime.player_post_frame = nullptr;
        bool audio_after_poll{};
        fixture.ports.audio_service_callback = [&] {
            audio_after_poll = fixture.roles[1].flags == 0x80000080U &&
                fixture.roles[1].action.wait_override == 0U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == OP_177_GATHER_PARTY_AT_PLAYER;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                read_u16(fixture.state.window, 0U) ==
                    static_cast<u16>(
                        OP_177_GATHER_PARTY_AT_PLAYER | alias_mask
                    ) &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE0005U &&
                fixture.roles[0].action.base_variant == 77U &&
                fixture.roles[1].flags == 0x80000080U &&
                fixture.roles[1].action.wait_override == 0U &&
                fixture.roles[2].flags == 0x04000080U &&
                fixture.roles[2].action.wait_override == 0x8000U &&
                fixture.roles[3].flags == 0x04000000U &&
                fixture.roles[3].action.wait_override == 0x8000U &&
                audio_after_poll,
            "opcode 177 high aliases poll without setup owners, clear only matching party-role bit26 and wait override, retain the self marker while party count disagrees, then audio-yield"
        );
    }

    Fixture completion;
    completion.roles.resize(2U);
    prime_loaded_instruction(
        completion, static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0xC000U)
    );
    completion.roles[0].world_x = 100U;
    completion.roles[0].world_y = 200U;
    completion.roles[1].world_x = 100U;
    completion.roles[1].world_y = 200U;
    completion.roles[1].flags = 0x04000080U;
    completion.roles[1].action.wait_override = 0x8000U;
    completion.live_party_role_count = 2U;
    completion.runtime.player_post_frame = nullptr;
    const auto completion_result = completion.step();

    Fixture exact_tail;
    exact_tail.roles.resize(1U);
    prime_loaded_instruction(exact_tail, OP_177_GATHER_PARTY_AT_PLAYER);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.live_party_role_count = 1U;
    exact_tail.runtime.player_post_frame = nullptr;
    write_u16(
        exact_tail.state.window,
        0x7FFEU,
        static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x8000U)
    );
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        completion_result.status == LegacyWorldStoryVmStatus::yielded &&
            completion_result.direct_audio_service_count == 1U &&
            completion.context.instruction_offset == 2U &&
            read_u16(completion.state.window, 0U) ==
                static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x4000U) &&
            completion.roles[1].flags == 0x00000080U &&
            completion.roles[1].action.wait_override == 0U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_177_GATHER_PARTY_AT_PLAYER &&
            read_u16(exact_tail.state.window, 0x7FFEU) ==
                OP_177_GATHER_PARTY_AT_PLAYER,
        "opcode 177 completes only when matched party roles plus the player equal live count, clears raw bit15 while preserving bit14, and audio-yields without a successor fetch at the exact tail"
    );

    Fixture missing_history;
    missing_history.roles.resize(2U);
    prime_loaded_instruction(missing_history, OP_177_GATHER_PARTY_AT_PLAYER);
    missing_history.state.previous_opcode = 0x66U;
    missing_history.dialogs.close.flagged_dialog_counter = 0x12340005U;
    missing_history.roles[0].action.base_variant = 9U;
    missing_history.roles[1].flags = 0x00000080U;
    missing_history.roles[1].action.wait_override = 7U;
    missing_history.runtime.player_post_frame = nullptr;
    const auto missing_history_result = missing_history.step();

    Fixture missing_party_count;
    missing_party_count.roles.resize(2U);
    prime_loaded_instruction(
        missing_party_count,
        static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x8000U)
    );
    missing_party_count.state.previous_opcode = 0x66U;
    missing_party_count.roles[0].world_x = 100U;
    missing_party_count.roles[0].world_y = 200U;
    missing_party_count.roles[1].world_x = 100U;
    missing_party_count.roles[1].world_y = 200U;
    missing_party_count.roles[1].flags = 0x04000080U;
    missing_party_count.roles[1].action.wait_override = 0x8000U;
    missing_party_count.runtime.player_post_frame = nullptr;
    missing_party_count.runtime.live_party_role_count = nullptr;
    const auto missing_party_count_result = missing_party_count.step();

    test.expect_true(
        missing_history_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            read_u16(missing_history.state.window, 0U) ==
                static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x8000U) &&
            missing_history.dialogs.close.flagged_dialog_counter ==
                0x12348005U &&
            missing_history.roles[0].action.base_variant == 0U &&
            missing_history.roles[1].flags == 0x00000080U &&
            missing_history.roles[1].action.wait_override == 7U &&
            missing_history.context.instruction_offset == 0U &&
            missing_history.state.previous_opcode == 0x66U &&
            missing_history.ports.direct_audio_service_count == 0U &&
            missing_party_count_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_party_count.roles[1].flags == 0x00000080U &&
            missing_party_count.roles[1].action.wait_override == 0U &&
            read_u16(missing_party_count.state.window, 0U) ==
                static_cast<u16>(OP_177_GATHER_PARTY_AT_PLAYER | 0x8000U) &&
            missing_party_count.context.instruction_offset == 0U &&
            missing_party_count.state.previous_opcode == 0x66U &&
            missing_party_count.ports.direct_audio_service_count == 0U,
        "opcode 177 typed-stops at missing setup history after self/dialog/base writes and at missing poll party count after matching-role cleanup, without previous or audio"
    );
}

void test_set_role_collision_bypass_protocol(openswd3::test::Context& test) {
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
            static_cast<u16>(OP_178_SET_ROLE_COLLISION_BYPASS | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.roles[1].flags = 0x80000001U;

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_178_SET_ROLE_COLLISION_BYPASS &&
                fixture.roles[1].flags == 0x80040001U,
            "opcode 178 covers every raw alias, ORs only role collision-bypass bit18, advances four, publishes normalized previous, and same-calls without audio"
        );
    }

    Fixture idempotent;
    prime_loaded_instruction(idempotent, OP_178_SET_ROLE_COLLISION_BYPASS);
    write_u16(idempotent.state.window, 2U, 0x00F8U);
    write_u16(idempotent.state.window, 4U, kStoryVmTypedStop);
    idempotent.roles[1].flags = 0xA5A40001U;
    const auto idempotent_result = idempotent.step();

    Fixture missing;
    prime_loaded_instruction(missing, OP_178_SET_ROLE_COLLISION_BYPASS);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, kStoryVmTypedStop);
    missing.roles[1].flags = 0x12345678U;
    const auto missing_result = missing.step();

    test.expect_true(
        idempotent_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            idempotent.roles[1].flags == 0xA5A40001U &&
            missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_178_SET_ROLE_COLLISION_BYPASS &&
            missing.roles[1].flags == 0x12345678U &&
            missing.ports.direct_audio_service_count == 0U,
        "opcode 178 is idempotent and silently consumes an ordinary lookup miss"
    );

    Fixture literal_fff0;
    literal_fff0.roles[1].guid = 2U;
    literal_fff0.roles[2].guid = 0x9999U;
    prime_loaded_instruction(literal_fff0, OP_178_SET_ROLE_COLLISION_BYPASS);
    write_u16(literal_fff0.state.window, 2U, 0xFFF0U);
    write_u16(literal_fff0.state.window, 4U, kStoryVmTypedStop);
    const auto literal_fff0_result = literal_fff0.step(0, 0, 2U);

    Fixture controlled_fffe;
    controlled_fffe.roles[2].guid = 0x9999U;
    prime_loaded_instruction(controlled_fffe, OP_178_SET_ROLE_COLLISION_BYPASS);
    write_u16(controlled_fffe.state.window, 2U, 0xFFFEU);
    write_u16(controlled_fffe.state.window, 4U, kStoryVmTypedStop);
    const auto controlled_fffe_result = controlled_fffe.step(0, 0, 2U);

    test.expect_true(
        literal_fff0_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            (literal_fff0.roles[1].flags & 0x00040000U) != 0U &&
            (literal_fff0.roles[2].flags & 0x00040000U) == 0U &&
            controlled_fffe_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            (controlled_fffe.roles[2].flags & 0x00040000U) != 0U,
        "opcode 178 replaces FFF0 with the controlled index low word then performs ordinary GUID lookup, while helper-native FFFE selects the controlled role directly"
    );

    Fixture skipped_first;
    skipped_first.roles[1].guid = 0x1234U;
    skipped_first.roles[1].flags = 0x10000001U;
    skipped_first.roles[2].guid = 0x1234U;
    skipped_first.roles[2].flags = 0x80000001U;
    prime_loaded_instruction(skipped_first, OP_178_SET_ROLE_COLLISION_BYPASS);
    write_u16(skipped_first.state.window, 2U, 0x1234U);
    write_u16(skipped_first.state.window, 4U, kStoryVmTypedStop);
    const auto skipped_first_result = skipped_first.step();

    test.expect_true(
        skipped_first_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            skipped_first.roles[1].flags == 0x10000001U &&
            skipped_first.roles[2].flags == 0x80040001U,
        "opcode 178 preserves lookup bit28 filtering and modifies only the first eligible duplicate GUID"
    );

    Fixture truncated;
    prime_loaded_instruction(truncated, OP_178_SET_ROLE_COLLISION_BYPASS);
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.state.previous_opcode = 0x66U;
    truncated.roles[1].flags = 0x12345678U;
    write_u16(
        truncated.state.window, 0x7FFEU, OP_178_SET_ROLE_COLLISION_BYPASS
    );
    const auto truncated_result = truncated.step();

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_178_SET_ROLE_COLLISION_BYPASS);
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.state.previous_opcode = 0x66U;
    exact_tail.roles[1].flags = 0x80000001U;
    write_u16(
        exact_tail.state.window, 0x7FFCU, OP_178_SET_ROLE_COLLISION_BYPASS
    );
    write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U &&
            truncated.roles[1].flags == 0x12345678U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_178_SET_ROLE_COLLISION_BYPASS &&
            exact_tail.roles[1].flags == 0x80040001U,
        "opcode 178 stops before lookup on a truncated selector and commits role flags, IP, and previous before an exact-tail same-call successor fetch failure"
    );
}

void test_enqueue_frame_deformation_protocol(openswd3::test::Context& test) {
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
            static_cast<u16>(OP_179_ENQUEUE_FRAME_DEFORMATION | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 100U);
        write_u16(fixture.state.window, 4U, 200U);
        write_u16(fixture.state.window, 6U, 30U);
        write_u16(fixture.state.window, 8U, 5U);
        write_u16(fixture.state.window, 10U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        const auto* node = fixture.frame_deformations.front();
        const auto center = std::size_t{30U + 30U * 60U};
        const auto radius_edge = std::size_t{54U + 30U * 60U};
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_179_ENQUEUE_FRAME_DEFORMATION &&
                fixture.frame_deformations.size() == 1U && node != nullptr &&
                node->state().framebuffer_width == 640U &&
                node->state().framebuffer_height == 480U &&
                node->state().origin_x == 70 && node->state().origin_y == 170 &&
                node->state().field_width == 60U &&
                node->state().field_height == 60U &&
                node->state().damping_shift == 4U &&
                node->state().active_field_index == 0U &&
                node->source_snapshot().size() == 640U * 480U &&
                node->field(0U)[center] == 120 &&
                node->field(0U)[center + 1U] == 115 &&
                node->field(0U)[radius_edge] == 0,
            "opcode 179 covers every raw alias, constructs and head-inserts the 640x480 deformation with signed origin and doubled field radius, injects fixed radius24 strength, then advances ten and same-calls without audio"
        );
    }

    Fixture signed_operands;
    prime_loaded_instruction(signed_operands, OP_179_ENQUEUE_FRAME_DEFORMATION);
    write_u16(
        signed_operands.state.window, 2U, std::bit_cast<u16>(i16{-32768})
    );
    write_u16(signed_operands.state.window, 4U, std::bit_cast<u16>(i16{32767}));
    write_u16(signed_operands.state.window, 6U, 25U);
    write_u16(signed_operands.state.window, 8U, std::bit_cast<u16>(i16{-3}));
    write_u16(signed_operands.state.window, 10U, kStoryVmTypedStop);
    const auto signed_result = signed_operands.step();
    const auto* signed_node = signed_operands.frame_deformations.front();
    const auto signed_center = std::size_t{25U + 25U * 50U};

    test.expect_true(
        signed_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            signed_node != nullptr && signed_node->state().origin_x == -32793 &&
            signed_node->state().origin_y == 32742 &&
            signed_node->state().field_width == 50U &&
            signed_node->state().field_height == 50U &&
            signed_node->field(0U)[signed_center] == -72 &&
            signed_operands.crt_rng.state() == 1U,
        "opcode 179 sign-extends all four operands, uses script x/y only for origin, and does not consume CRT RNG for a nonnegative field center"
    );

    Fixture missing_y;
    prime_loaded_instruction(missing_y, OP_179_ENQUEUE_FRAME_DEFORMATION);
    missing_y.context.instruction_offset = 0x7FFCU;
    missing_y.state.previous_opcode = 0x66U;
    write_u16(
        missing_y.state.window, 0x7FFCU, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    write_u16(missing_y.state.window, 0x7FFEU, 100U);
    const auto missing_y_result = missing_y.step();

    Fixture missing_radius;
    prime_loaded_instruction(missing_radius, OP_179_ENQUEUE_FRAME_DEFORMATION);
    missing_radius.context.instruction_offset = 0x7FFAU;
    missing_radius.state.previous_opcode = 0x66U;
    write_u16(
        missing_radius.state.window, 0x7FFAU, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    write_u16(missing_radius.state.window, 0x7FFCU, 100U);
    write_u16(missing_radius.state.window, 0x7FFEU, 200U);
    const auto missing_radius_result = missing_radius.step();

    Fixture missing_strength;
    prime_loaded_instruction(
        missing_strength, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    missing_strength.context.instruction_offset = 0x7FF8U;
    missing_strength.state.previous_opcode = 0x66U;
    write_u16(
        missing_strength.state.window, 0x7FF8U, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    write_u16(missing_strength.state.window, 0x7FFAU, 100U);
    write_u16(missing_strength.state.window, 0x7FFCU, 200U);
    write_u16(missing_strength.state.window, 0x7FFEU, 30U);
    const auto missing_strength_result = missing_strength.step();

    test.expect_true(
        missing_y_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_y.context.instruction_offset == 0x7FFCU &&
            missing_y.state.previous_opcode == 0x66U &&
            missing_y.frame_deformations.empty() &&
            missing_radius_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_radius.context.instruction_offset == 0x7FFAU &&
            missing_radius.state.previous_opcode == 0x66U &&
            missing_radius.frame_deformations.empty() &&
            missing_strength_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_strength.context.instruction_offset == 0x7FF8U &&
            missing_strength.state.previous_opcode == 0x66U &&
            missing_strength.frame_deformations.empty(),
        "opcode 179 preserves the machine read order y, x, radius, strength and stops before allocation on each truncated stage"
    );

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_179_ENQUEUE_FRAME_DEFORMATION);
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window, 0x7FF6U, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    write_u16(exact_tail.state.window, 0x7FF8U, 100U);
    write_u16(exact_tail.state.window, 0x7FFAU, 200U);
    write_u16(exact_tail.state.window, 0x7FFCU, 30U);
    write_u16(exact_tail.state.window, 0x7FFEU, 5U);
    const auto exact_tail_result = exact_tail.step();

    Fixture owner_missing;
    prime_loaded_instruction(owner_missing, OP_179_ENQUEUE_FRAME_DEFORMATION);
    write_u16(owner_missing.state.window, 2U, 100U);
    write_u16(owner_missing.state.window, 4U, 200U);
    write_u16(owner_missing.state.window, 6U, 30U);
    write_u16(owner_missing.state.window, 8U, 5U);
    owner_missing.state.previous_opcode = 0x66U;
    owner_missing.runtime.frame_deformations = nullptr;
    const auto owner_missing_result = owner_missing.step();

    Fixture invalid_geometry;
    prime_loaded_instruction(
        invalid_geometry, OP_179_ENQUEUE_FRAME_DEFORMATION
    );
    write_u16(invalid_geometry.state.window, 2U, 100U);
    write_u16(invalid_geometry.state.window, 4U, 200U);
    write_u16(invalid_geometry.state.window, 6U, 0U);
    write_u16(invalid_geometry.state.window, 8U, 5U);
    invalid_geometry.state.previous_opcode = 0x66U;
    const auto invalid_geometry_result = invalid_geometry.step();

    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_179_ENQUEUE_FRAME_DEFORMATION &&
            exact_tail.frame_deformations.size() == 1U &&
            owner_missing_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            owner_missing.context.instruction_offset == 0U &&
            owner_missing.state.previous_opcode == 0x66U &&
            owner_missing.frame_deformations.empty() &&
            invalid_geometry_result.status ==
                LegacyWorldStoryVmStatus::frame_deformation_injection_failed &&
            invalid_geometry.context.instruction_offset == 0U &&
            invalid_geometry.state.previous_opcode == 0x66U &&
            invalid_geometry.frame_deformations.empty() &&
            invalid_geometry.crt_rng.state() == 1U,
        "opcode 179 commits the complete node before exact-tail successor fetch failure, while missing actual owners and unsafe zero geometry typed-stop before publication, IP, previous, or RNG"
    );
}

void test_clear_frame_execution_gate_protocol(openswd3::test::Context& test) {
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
            static_cast<u16>(OP_180_CLEAR_FRAME_EXECUTION_GATE | alias_mask)
        );
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        fixture.frame_execution_gate = 0xA5A50001U;
        u32 gate_at_audio = 0xFFFFFFFFU;
        u16 ip_at_audio{};
        u32 previous_at_audio{};
        fixture.ports.audio_service_callback = [&] {
            gate_at_audio = fixture.frame_execution_gate;
            ip_at_audio = fixture.context.instruction_offset;
            previous_at_audio = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_180_CLEAR_FRAME_EXECUTION_GATE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.frame_execution_gate == 0U && gate_at_audio == 0U &&
                fixture.context.instruction_offset == 2U && ip_at_audio == 2U &&
                fixture.state.previous_opcode ==
                    OP_180_CLEAR_FRAME_EXECUTION_GATE &&
                previous_at_audio == OP_180_CLEAR_FRAME_EXECUTION_GATE,
            "opcode 180 covers every raw alias, clears the full frame-execution gate before IP and previous, then services audio and yields without fetching the successor"
        );
    }

    Fixture idempotent;
    prime_loaded_instruction(idempotent, OP_180_CLEAR_FRAME_EXECUTION_GATE);
    write_u16(idempotent.state.window, 2U, kStoryVmTypedStop);
    idempotent.frame_execution_gate = 0U;
    const auto idempotent_result = idempotent.step();

    Fixture exact_tail;
    prime_loaded_instruction(exact_tail, OP_180_CLEAR_FRAME_EXECUTION_GATE);
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.state.previous_opcode = 0x66U;
    exact_tail.frame_execution_gate = 0xFFFFFFFFU;
    write_u16(
        exact_tail.state.window,
        0x7FFEU,
        static_cast<u16>(OP_180_CLEAR_FRAME_EXECUTION_GATE | 0xC000U)
    );
    const auto exact_tail_result = exact_tail.step();

    Fixture owner_missing;
    prime_loaded_instruction(owner_missing, OP_180_CLEAR_FRAME_EXECUTION_GATE);
    owner_missing.state.previous_opcode = 0x66U;
    owner_missing.frame_execution_gate = 0x12345678U;
    owner_missing.runtime.frame_execution_gate = nullptr;
    const auto owner_missing_result = owner_missing.step();

    test.expect_true(
        idempotent_result.status == LegacyWorldStoryVmStatus::yielded &&
            idempotent_result.executed_instruction_count == 1U &&
            idempotent_result.direct_audio_service_count == 1U &&
            idempotent.frame_execution_gate == 0U &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_180_CLEAR_FRAME_EXECUTION_GATE &&
            exact_tail.frame_execution_gate == 0U &&
            owner_missing_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            owner_missing_result.executed_instruction_count == 1U &&
            owner_missing_result.direct_audio_service_count == 0U &&
            owner_missing.context.instruction_offset == 0U &&
            owner_missing.state.previous_opcode == 0x66U &&
            owner_missing.frame_execution_gate == 0x12345678U,
        "opcode 180 is idempotent and yields after its full-dword gate clear at the exact window tail, while a missing actual owner stops at the original write point"
    );
}

void test_party_member_field_reload_protocol(openswd3::test::Context& test) {
    const auto set_field = [](auto& resources,
                              const i32 selector,
                              const u32 value) {
        switch (selector) {
        case 0:
            resources.current_first = static_cast<u16>(value);
            break;

        case 1:
            resources.current_second = static_cast<u16>(value);
            break;

        case 2:
            resources.current_third = static_cast<u16>(value);
            break;

        case 3:
            resources.limit_first = static_cast<u16>(value);
            break;

        case 4:
            resources.limit_second = static_cast<u16>(value);
            break;

        case 5:
            resources.limit_third = static_cast<u16>(value);
            break;

        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            resources.fields_10_to_1e[static_cast<std::size_t>(selector - 6)] =
                static_cast<u16>(value);
            break;

        case 14:
            resources.field_20 = value;
            break;

        case 15:
            resources.field_00 = value;
            break;

        case 16:
            resources.field_2c = static_cast<u8>(value);
            break;
        }
    };
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 selector,
                          const u16 threshold,
                          const u32 target) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, threshold);
        write_u32(fixture.state.window, 6U, target);
    };

    for (i32 selector = 0; selector <= 16; ++selector) {
        Fixture fixture;
        const u16 value = static_cast<u16>(100 + selector);
        set_field(
            fixture.state.party_member_resources[0U], selector, value + 1U
        );
        set_field(fixture.state.party_member_resources[1U], selector, value);
        prime(
            fixture,
            OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE,
            static_cast<u16>(selector),
            value,
            static_cast<u32>(0x2000 + selector)
        );
        write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.talk_data_offset ==
                    static_cast<u32>(0x2000 + selector) &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.direct_audio_service_count == 2U,
            "opcode 186 maps all seventeen getter selectors to the second party-member record and reloads on equality"
        );
    }

    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        for (const u16 opcode : {
                 OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE,
                 OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE,
             }) {
            Fixture fixture;
            fixture.state.party_member_resources[1U].field_00 = 10U;
            prime(fixture, static_cast<u16>(opcode | mask), 15U, 10U, 0x3333U);
            write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == opcode &&
                    result.executed_instruction_count == 1U &&
                    result.direct_audio_service_count == 2U &&
                    fixture.context.talk_data_offset == 0x3333U &&
                    fixture.state.previous_opcode == opcode &&
                    fixture.ports.data_load_count == 1U,
                "opcodes 186-187 cover every raw alias and take their inclusive signed equality branch"
            );
        }
    }

    Fixture signed_word;
    signed_word.state.party_member_resources[1U].limit_third = 0xFFFFU;
    prime(signed_word, OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE, 5U, 0U, 0x4444U);
    write_u16(signed_word.state.window, 10U, kStoryVmTypedStop);
    const auto signed_word_result = signed_word.step();

    Fixture unsigned_word;
    unsigned_word.state.party_member_resources[1U].fields_10_to_1e[0U] =
        0xFFFFU;
    prime(
        unsigned_word,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE,
        6U,
        0xFFFFU,
        0x4444U
    );
    write_u16(unsigned_word.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto unsigned_word_result = unsigned_word.step();

    Fixture signed_dword_20;
    signed_dword_20.state.party_member_resources[1U].field_20 = 0x80000000U;
    prime(
        signed_dword_20,
        OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE,
        14U,
        0U,
        0x5555U
    );
    write_u16(signed_dword_20.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto signed_dword_20_result = signed_dword_20.step();

    Fixture signed_dword_00;
    signed_dword_00.state.party_member_resources[1U].field_00 = 0xFFFFFFFFU;
    prime(
        signed_dword_00,
        OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE,
        15U,
        0U,
        0x6666U
    );
    write_u16(signed_dword_00.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto signed_dword_00_result = signed_dword_00.step();

    test.expect_true(
        signed_word_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            signed_word.context.instruction_offset == 10U &&
            signed_word.ports.data_load_count == 0U &&
            unsigned_word_result.status == LegacyWorldStoryVmStatus::yielded &&
            unsigned_word.context.talk_data_offset == 0x4444U &&
            signed_dword_20_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            signed_dword_20.context.talk_data_offset == 0x5555U &&
            signed_dword_00_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            signed_dword_00.context.talk_data_offset == 0x6666U,
        "party-member getter sign-extends fields zero through five, zero-extends fields six through thirteen, and preserves signed i32 bit patterns for fields fourteen and fifteen"
    );

    Fixture ge_not_taken;
    ge_not_taken.state.party_member_resources[1U].field_00 = 9U;
    prime(
        ge_not_taken, OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE, 15U, 10U, 0x7777U
    );
    write_u16(ge_not_taken.state.window, 10U, kStoryVmTypedStop);
    const auto ge_not_taken_result = ge_not_taken.step();

    Fixture le_not_taken;
    le_not_taken.state.party_member_resources[1U].field_00 = 11U;
    prime(
        le_not_taken, OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE, 15U, 10U, 0x7777U
    );
    write_u16(le_not_taken.state.window, 10U, kStoryVmTypedStop);
    const auto le_not_taken_result = le_not_taken.step();
    test.expect_true(
        ge_not_taken_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            ge_not_taken_result.executed_instruction_count == 2U &&
            ge_not_taken.context.instruction_offset == 10U &&
            ge_not_taken.state.previous_opcode ==
                OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
            ge_not_taken.ports.data_load_count == 0U &&
            ge_not_taken.ports.direct_audio_service_count == 0U &&
            le_not_taken_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            le_not_taken_result.executed_instruction_count == 2U &&
            le_not_taken.context.instruction_offset == 10U &&
            le_not_taken.state.previous_opcode ==
                OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE &&
            le_not_taken.ports.data_load_count == 0U &&
            le_not_taken.ports.direct_audio_service_count == 0U,
        "opcodes 186-187 use opposite inclusive signed predicates and same-call their ten-byte not-taken records without audio"
    );

    for (const u16 opcode : {
             OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE,
             OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE,
         }) {
        Fixture high_selector;
        high_selector.context.instruction_offset = 0x7FFCU;
        high_selector.context.talk_data_offset = 0x1111U;
        high_selector.state.loaded_file_number = 1U;
        high_selector.state.loaded_data_offset = 0x1111U;
        high_selector.state.window_loaded = true;
        high_selector.state.previous_opcode = 0x55U;
        write_u16(high_selector.state.window, 0x7FFCU, opcode);
        write_u16(high_selector.state.window, 0x7FFEU, 17U);
        const auto result = high_selector.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                high_selector.context.instruction_offset == 0x7FFCU &&
                high_selector.state.previous_opcode == opcode &&
                high_selector.ports.data_load_count == 0U,
            "opcodes 186-187 selector above sixteen does not read threshold or target, does not advance, and audio-yields for retry"
        );
    }

    Fixture negative_ge;
    prime(
        negative_ge,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE,
        0xFFFFU,
        1U,
        0x8888U
    );
    write_u16(negative_ge.state.window, 10U, kStoryVmTypedStop);
    const auto negative_ge_result = negative_ge.step();

    Fixture negative_le;
    prime(
        negative_le,
        OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE,
        0x8000U,
        0U,
        0x9999U
    );
    write_u16(negative_le.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto negative_le_result = negative_le.step();
    test.expect_true(
        negative_ge_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_ge.context.instruction_offset == 10U &&
            negative_ge.ports.data_load_count == 0U &&
            negative_le_result.status == LegacyWorldStoryVmStatus::yielded &&
            negative_le.context.talk_data_offset == 0x9999U &&
            negative_le.ports.data_load_count == 1U,
        "negative selectors use the original getter default value zero instead of indexing the record"
    );

    Fixture threshold_truncated;
    threshold_truncated.context.instruction_offset = 0x7FFCU;
    threshold_truncated.context.talk_data_offset = 0x1111U;
    threshold_truncated.state.loaded_file_number = 1U;
    threshold_truncated.state.loaded_data_offset = 0x1111U;
    threshold_truncated.state.window_loaded = true;
    write_u16(
        threshold_truncated.state.window,
        0x7FFCU,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE
    );
    write_u16(threshold_truncated.state.window, 0x7FFEU, 15U);
    const auto threshold_truncated_result = threshold_truncated.step();

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFAU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.party_member_resources[1U].field_00 = 10U;
    write_u16(
        target_truncated.state.window,
        0x7FFAU,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE
    );
    write_u16(target_truncated.state.window, 0x7FFCU, 15U);
    write_u16(target_truncated.state.window, 0x7FFEU, 10U);
    const auto target_truncated_result = target_truncated.step();

    Fixture unread_target;
    unread_target.context.instruction_offset = 0x7FFAU;
    unread_target.context.talk_data_offset = 0x1111U;
    unread_target.state.loaded_file_number = 1U;
    unread_target.state.loaded_data_offset = 0x1111U;
    unread_target.state.window_loaded = true;
    unread_target.state.party_member_resources[1U].field_00 = 9U;
    write_u16(
        unread_target.state.window,
        0x7FFAU,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE
    );
    write_u16(unread_target.state.window, 0x7FFCU, 15U);
    write_u16(unread_target.state.window, 0x7FFEU, 10U);
    const auto unread_target_result = unread_target.step();
    test.expect_true(
        threshold_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            threshold_truncated.context.instruction_offset == 0x7FFCU &&
            target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFAU &&
            target_truncated.state.previous_opcode == 0U &&
            unread_target_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            unread_target.context.instruction_offset == 0x8004U &&
            unread_target.state.previous_opcode ==
                OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
            unread_target.ports.data_load_count == 0U,
        "valid selectors read threshold after the getter, taken requires target, and not-taken leaves target bytes unread before its fixed ten-byte same-call tail"
    );

    Fixture exact_taken;
    exact_taken.context.instruction_offset = 0x7FF6U;
    exact_taken.context.talk_data_offset = 0x1111U;
    exact_taken.state.loaded_file_number = 1U;
    exact_taken.state.loaded_data_offset = 0x1111U;
    exact_taken.state.window_loaded = true;
    exact_taken.state.party_member_resources[1U].field_00 = 10U;
    write_u16(
        exact_taken.state.window,
        0x7FF6U,
        OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE
    );
    write_u16(exact_taken.state.window, 0x7FF8U, 15U);
    write_u16(exact_taken.state.window, 0x7FFAU, 10U);
    write_u32(exact_taken.state.window, 0x7FFCU, 0xAAAAU);
    write_u16(exact_taken.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto exact_taken_result = exact_taken.step();

    Fixture load_failure;
    load_failure.state.party_member_resources[1U].field_00 = 10U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime(
        load_failure, OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE, 15U, 10U, 0xBBBBU
    );
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        exact_taken_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_taken_result.executed_instruction_count == 1U &&
            exact_taken_result.direct_audio_service_count == 2U &&
            exact_taken.context.talk_data_offset == 0xAAAAU &&
            exact_taken.context.instruction_offset == 0U &&
            exact_taken.state.previous_opcode ==
                OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
            load_failure_result.status ==
                LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 2U &&
            load_failure.context.talk_data_offset == 0xBBBBU &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE &&
            load_failure.ports.direct_audio_service_count == 2U,
        "taken exact-tail and checked load failure both preserve loader audio, previous publication, common-join audio and yield ordering"
    );
}

void test_party_member_field_write_protocol(openswd3::test::Context& test) {
    const auto field_bits = [](const auto& resources,
                               const i32 selector) -> u32 {
        switch (selector) {
        case 0:
            return resources.current_first;

        case 1:
            return resources.current_second;

        case 2:
            return resources.current_third;

        case 3:
            return resources.limit_first;

        case 4:
            return resources.limit_second;

        case 5:
            return resources.limit_third;

        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            return resources
                .fields_10_to_1e[static_cast<std::size_t>(selector - 6)];

        case 14:
            return resources.field_20;

        case 15:
            return resources.field_00;

        case 16:
            return resources.field_2c;

        default:
            return 0U;
        }
    };
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 selector,
                          const u16 operand) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, operand);
    };

    for (i32 selector = 0; selector <= 16; ++selector) {
        Fixture fixture;
        fixture.state.party_member_resources[0U].field_00 = 0xAAAAAAAAU;
        fixture.state.party_member_resources[1U].field_20 = 0xBBBBBBBBU;
        prime(
            fixture,
            OP_188_SET_PARTY_MEMBER_FIELD,
            static_cast<u16>(selector),
            0x1234U
        );
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        const auto result = fixture.step();
        const u32 expected = selector == 16 ? 0x34U : 0x1234U;
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_188_SET_PARTY_MEMBER_FIELD &&
                field_bits(
                    fixture.state.party_member_resources[1U], selector
                ) == expected &&
                fixture.state.party_member_resources[0U].field_00 ==
                    0xAAAAAAAAU &&
                fixture.ports.direct_audio_service_count == 0U &&
                fixture.ports.party_member_level_load_count ==
                    (selector == 16 ? 1U : 0U),
            "opcode 188 maps all seventeen setter selectors to the second party-member record and truncates only at the destination width"
        );
    }

    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture set;
        prime(
            set, static_cast<u16>(OP_188_SET_PARTY_MEMBER_FIELD | mask), 15U, 5U
        );
        write_u16(set.state.window, 6U, kStoryVmTypedStop);
        const auto set_result = set.step();

        Fixture add;
        add.state.party_member_resources[1U].field_00 = 7U;
        prime(
            add, static_cast<u16>(OP_189_ADD_PARTY_MEMBER_FIELD | mask), 15U, 5U
        );
        write_u16(add.state.window, 6U, kStoryVmTypedStop);
        const auto add_result = add.step();

        Fixture subtract;
        subtract.state.party_member_resources[1U].field_00 = 7U;
        prime(
            subtract,
            static_cast<u16>(OP_190_SUBTRACT_PARTY_MEMBER_FIELD | mask),
            15U,
            5U
        );
        write_u16(subtract.state.window, 6U, kStoryVmTypedStop);
        const auto subtract_result = subtract.step();
        test.expect_true(
            set_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                set.state.party_member_resources[1U].field_00 == 5U &&
                set.state.previous_opcode == OP_188_SET_PARTY_MEMBER_FIELD &&
                add_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                add.state.party_member_resources[1U].field_00 == 12U &&
                add.state.previous_opcode == OP_189_ADD_PARTY_MEMBER_FIELD &&
                subtract_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                subtract.state.party_member_resources[1U].field_00 == 2U &&
                subtract.state.previous_opcode ==
                    OP_190_SUBTRACT_PARTY_MEMBER_FIELD,
            "opcodes 188-190 cover every raw alias and refine to set, wrapping add, or wrapping subtract"
        );
    }

    Fixture full_add;
    full_add.state.party_member_resources[1U].field_00 = 0x7FFFFFFFU;
    prime(full_add, OP_189_ADD_PARTY_MEMBER_FIELD, 15U, 1U);
    write_u16(full_add.state.window, 6U, kStoryVmTypedStop);
    const auto full_add_result = full_add.step();

    Fixture full_subtract_negative;
    full_subtract_negative.state.party_member_resources[1U].field_20 = 0U;
    prime(
        full_subtract_negative, OP_190_SUBTRACT_PARTY_MEMBER_FIELD, 14U, 0xFFFFU
    );
    write_u16(full_subtract_negative.state.window, 6U, kStoryVmTypedStop);
    const auto full_subtract_result = full_subtract_negative.step();

    Fixture signed_word_add;
    signed_word_add.state.party_member_resources[1U].current_first = 0xFFFFU;
    prime(signed_word_add, OP_189_ADD_PARTY_MEMBER_FIELD, 0U, 1U);
    write_u16(signed_word_add.state.window, 6U, kStoryVmTypedStop);
    const auto signed_word_result = signed_word_add.step();

    Fixture unsigned_word_add;
    unsigned_word_add.state.party_member_resources[1U].fields_10_to_1e[0U] =
        0xFFFFU;
    prime(unsigned_word_add, OP_189_ADD_PARTY_MEMBER_FIELD, 6U, 1U);
    write_u16(unsigned_word_add.state.window, 6U, kStoryVmTypedStop);
    const auto unsigned_word_result = unsigned_word_add.step();
    test.expect_true(
        full_add_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            full_add.state.party_member_resources[1U].field_00 == 0x80000000U &&
            full_subtract_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            full_subtract_negative.state.party_member_resources[1U].field_20 ==
                1U &&
            signed_word_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            signed_word_add.state.party_member_resources[1U].current_first ==
                0U &&
            unsigned_word_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            unsigned_word_add.state.party_member_resources[1U]
                    .fields_10_to_1e[0U] == 0U,
        "opcodes 189-190 use getter extension and i32 wrapping before the destination setter truncates word fields"
    );

    Fixture level_success;
    level_success.state.party_member_resources[1U].field_2c = 0xFFU;
    level_success.state.party_member_resources[1U].field_20 = 0x11111111U;
    level_success.ports.party_member_level_load_success = true;
    level_success.ports.party_member_level_output = 0xCAFEBABEU;
    prime(level_success, OP_189_ADD_PARTY_MEMBER_FIELD, 16U, 1U);
    write_u16(level_success.state.window, 6U, kStoryVmTypedStop);
    const auto level_success_result = level_success.step();

    Fixture level_failure;
    level_failure.state.party_member_resources[1U].field_2c = 0U;
    level_failure.state.party_member_resources[1U].field_20 = 0x22222222U;
    prime(level_failure, OP_188_SET_PARTY_MEMBER_FIELD, 16U, 0xFFFFU);
    write_u16(level_failure.state.window, 6U, kStoryVmTypedStop);
    const auto level_failure_result = level_failure.step();
    test.expect_true(
        level_success_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            level_success.state.party_member_resources[1U].field_2c == 0U &&
            level_success.state.party_member_resources[1U].field_20 ==
                0xCAFEBABEU &&
            level_success.ports.party_member_level_load_count == 1U &&
            level_success.ports.last_party_member_level_group == 2U &&
            level_success.ports.last_party_member_level == 257U &&
            level_failure_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            level_failure.state.party_member_resources[1U].field_2c == 0xFFU &&
            level_failure.state.party_member_resources[1U].field_20 ==
                0x22222222U &&
            level_failure.ports.party_member_level_load_count == 1U &&
            level_failure.ports.last_party_member_level_group == 2U &&
            level_failure.ports.last_party_member_level == 0U,
        "field sixteen writes its low byte before LEVEL group two lookup at wrapped result plus one, updating field fourteen only on helper success"
    );

    for (const u16 opcode : {
             OP_188_SET_PARTY_MEMBER_FIELD,
             OP_189_ADD_PARTY_MEMBER_FIELD,
             OP_190_SUBTRACT_PARTY_MEMBER_FIELD,
         }) {
        Fixture high_selector;
        high_selector.context.instruction_offset = 0x7FFAU;
        high_selector.context.talk_data_offset = 0x1111U;
        high_selector.state.loaded_file_number = 1U;
        high_selector.state.loaded_data_offset = 0x1111U;
        high_selector.state.window_loaded = true;
        high_selector.state.previous_opcode = 0x55U;
        high_selector.state.party_member_resources[1U].field_00 = 7U;
        write_u16(high_selector.state.window, 0x7FFAU, opcode);
        write_u16(high_selector.state.window, 0x7FFCU, 17U);
        write_u16(high_selector.state.window, 0x7FFEU, 0x1234U);
        const auto result = high_selector.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                high_selector.context.instruction_offset == 0x7FFAU &&
                high_selector.state.previous_opcode == opcode &&
                high_selector.state.party_member_resources[1U].field_00 == 7U &&
                high_selector.ports.party_member_level_load_count == 0U,
            "opcodes 188-190 selector above sixteen reads the value but does not write, advance, or call LEVEL before audio-yield retry"
        );
    }

    for (const u16 opcode : {
             OP_188_SET_PARTY_MEMBER_FIELD,
             OP_189_ADD_PARTY_MEMBER_FIELD,
             OP_190_SUBTRACT_PARTY_MEMBER_FIELD,
         }) {
        Fixture negative_selector;
        negative_selector.state.party_member_resources[1U].field_00 =
            0x12345678U;
        prime(negative_selector, opcode, 0xFFFFU, 5U);
        write_u16(negative_selector.state.window, 6U, kStoryVmTypedStop);
        const auto result = negative_selector.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.executed_instruction_count == 2U &&
                negative_selector.context.instruction_offset == 6U &&
                negative_selector.state.previous_opcode == opcode &&
                negative_selector.state.party_member_resources[1U].field_00 ==
                    0x12345678U &&
                negative_selector.ports.party_member_level_load_count == 0U,
            "negative field selectors follow getter and setter defaults without touching the record"
        );
    }

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window, 0x7FFEU, OP_188_SET_PARTY_MEMBER_FIELD
    );
    const auto selector_truncated_result = selector_truncated.step();

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFCU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    write_u16(
        value_truncated.state.window, 0x7FFCU, OP_188_SET_PARTY_MEMBER_FIELD
    );
    write_u16(value_truncated.state.window, 0x7FFEU, 17U);
    const auto value_truncated_result = value_truncated.step();

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_188_SET_PARTY_MEMBER_FIELD);
    write_u16(exact_tail.state.window, 0x7FFCU, 15U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x4321U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFCU &&
            value_truncated.state.previous_opcode == 0U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_188_SET_PARTY_MEMBER_FIELD &&
            exact_tail.state.party_member_resources[1U].field_00 == 0x4321U,
        "opcodes 188-190 require selector then value, while an exact six-byte record commits its write, IP and previous before the same-call successor fetch fails"
    );
}

void test_transfer_flags_and_terminal_cleanup(openswd3::test::Context& test) {
    Fixture fixture;
    auto first = std::span<u8>{fixture.ports.initial_window};
    write_u16(first, 0U, OP_161_TRANSFER_STORY);
    write_u16(first, 2U, 2042U);
    auto second = std::span<u8>{fixture.ports.transferred_window};
    write_u16(second, 0U, 25U);
    write_u16(second, 2U, 123U);
    write_u16(second, 4U, 26U);
    write_u16(second, 6U, 3U);
    write_u16(second, 8U, 0xFFFFU);
    fixture.roles[1].flags |= 0x00000800U;
    fixture.roles[1].action.base_variant = 1U;
    fixture.roles[1].action.variant_delta = 2U;
    fixture.roles[1].action.one_shot_base_variant = 7U;
    fixture.roles[1].action.one_shot_variant_delta = 6U;
    fixture.roles[2].path_data_id = 9U;
    fixture.roles[2].path_word_index = 17U;
    fixture.roles[2].action.one_shot_base_variant = 8U;
    fixture.roles[2].action.one_shot_variant_delta = 5U;
    fixture.active_object_slots[0].bytes[0] = 2U;
    fixture.active_object_slots[0].bytes[1] = 0U;
    fixture.active_object_slots[0].bytes[0x1BU] = 2U;

    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::terminated &&
            result.executed_instruction_count == 4U &&
            fixture.ports.story_load_count == 2U &&
            fixture.ports.last_story_id == 2042 &&
            result.direct_audio_service_count == 4U &&
            fixture.ports.direct_audio_service_count == 4U &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 123U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 3U
            ) &&
            fixture.context.source_guid == 0xFFFFU &&
            fixture.context.talk_data_offset == 0xFFFFFFFFU &&
            !fixture.state.window_loaded &&
            (fixture.dialogs.close.flagged_dialog_counter & 0x8000U) == 0U &&
            (fixture.roles[1].flags & 0x00080000U) == 0U &&
            fixture.roles[1].action.base_variant == 7U &&
            fixture.roles[1].action.variant_delta == 6U &&
            fixture.roles[1].action.one_shot_base_variant == 0xFFFFFFFFU &&
            fixture.roles[1].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            fixture.roles[2].action.one_shot_base_variant == 0xFFFFFFFFU &&
            fixture.roles[2].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            fixture.roles[2].path_data_id == 0U &&
            fixture.roles[2].path_word_index == 0U &&
            std::ranges::all_of(
                fixture.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            result.role_one_shot_clear_count == fixture.roles.size() &&
            result.active_object_reset_count == 1U &&
            result.action_update_count == 2U,
        "opcode 161 replaces the window, 25/26 mutate flags, and FFFF restores and releases the source role"
    );
}

void test_same_file_branch(openswd3::test::Context& test) {
    Fixture fixture;
    auto first = std::span<u8>{fixture.ports.initial_window};
    write_u16(first, 0U, 21U);
    write_u16(first, 2U, 1U);
    write_u32(first, 4U, 0x3333U);
    auto branch = std::span<u8>{fixture.ports.transferred_window};
    write_u16(branch, 0U, 0xFFFFU);

    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::terminated &&
            result.executed_instruction_count == 2U &&
            fixture.ports.data_load_count == 1U &&
            result.action_update_count == 2U,
        "opcode 21 branches within the current TALK file before TalkEnd"
    );
}

void test_role_action_operand_extension(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x8000U);
    write_u16(script, 6U, 0xFFFEU);
    write_u16(script, 8U, 0x8000U);
    write_u16(script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 12U, 0x00F8U);

    const auto result = fixture.step();

    Fixture missing;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(missing_script, 2U, 0x7777U);
    write_u16(missing_script, 4U, 0x8000U);
    write_u16(missing_script, 6U, 0xFFFEU);
    write_u16(missing_script, 8U, 0x8123U);
    write_u16(missing_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing_script, 12U, 0x00F8U);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();

    Fixture preserved;
    preserved.roles[1].action.action_id = 0x12345678U;
    preserved.roles[1].action.base_variant = 0x87654321U;
    preserved.roles[1].action.variant_delta = 0x10203040U;
    preserved.roles[1].action.wait_remaining = 7U;
    auto preserved_script = std::span<u8>{preserved.ports.initial_window};
    write_u16(preserved_script, 0U, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(preserved_script, 2U, 0x00F8U);
    write_u16(preserved_script, 4U, 0xFFFFU);
    write_u16(preserved_script, 6U, 0xFFFFU);
    write_u16(preserved_script, 8U, 0xFFFFU);
    write_u16(preserved_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(preserved_script, 12U, 0x00F8U);
    const auto preserved_result = preserved.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            fixture.roles[1].action.action_id == 0xFFFF8000U &&
            fixture.roles[1].action.base_variant == 0xFFFFFFFEU &&
            fixture.roles[1].action.variant_delta == 0x00008000U,
        "opcode 120 sign-extends action and base while zero-extending variant"
    );
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            missing_result.executed_instruction_count == 2U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.action_id == 0x8000U &&
            patch.base_variant == 0xFFFEU && patch.variant_delta == 0x8123U &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 120 patches the MAPS role source and consumes ten bytes when " "the runtime role is absent"
    );
    test.expect_true(
        preserved_result.status == LegacyWorldStoryVmStatus::yielded &&
            preserved_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            preserved.roles[1].action.action_id == 0x12345678U &&
            preserved.roles[1].action.base_variant == 0x87654321U &&
            preserved.roles[1].action.variant_delta == 0x10203040U &&
            preserved.roles[1].action.wait_remaining == 0U &&
            (preserved.roles[1].flags & 0x1000U) != 0U &&
            preserved.ports.action_update_count == 2U,
        "opcode 120 preserves FFFF action operands while refreshing and " "marking the resolved role"
    );
}

void test_update_role_action_fields_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_120_UPDATE_ROLE_ACTION_FIELDS,
        static_cast<u16>(OP_120_UPDATE_ROLE_ACTION_FIELDS | 0x4000U),
        static_cast<u16>(OP_120_UPDATE_ROLE_ACTION_FIELDS | 0x8000U),
        static_cast<u16>(OP_120_UPDATE_ROLE_ACTION_FIELDS | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8000U);
        write_u16(fixture.state.window, 6U, 0x7FFFU);
        write_u16(fixture.state.window, 8U, 0x8000U);
        write_u16(fixture.state.window, 10U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_120_UPDATE_ROLE_ACTION_FIELDS &&
                fixture.roles[1].action.action_id == 0xFFFF8000U &&
                fixture.roles[1].action.base_variant == 0x00007FFFU &&
                fixture.roles[1].action.variant_delta == 0x00008000U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                (fixture.roles[1].flags & 0x00001000U) != 0U,
            "opcode 120 aliases update all three fields and continue"
        );
    }

    Fixture source_lookup;
    source_lookup.roles[1].flags = 0x10000000U;
    source_lookup.roles[2].guid = 0x00F8U;
    source_lookup.roles[2].flags = 0U;
    prime_loaded_instruction(source_lookup, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(source_lookup.state.window, 2U, 0xFFF0U);
    write_u16(source_lookup.state.window, 4U, 1U);
    write_u16(source_lookup.state.window, 6U, 2U);
    write_u16(source_lookup.state.window, 8U, 3U);
    write_u16(source_lookup.state.window, 10U, kStoryVmTypedStop);
    const auto source_lookup_result = source_lookup.step();
    test.expect_true(
        source_lookup_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            source_lookup.roles[1].action.action_id == 0U &&
            source_lookup.roles[2].action.action_id == 1U &&
            source_lookup.roles[2].action.base_variant == 2U &&
            source_lookup.roles[2].action.variant_delta == 3U &&
            source_lookup.state.previous_opcode ==
                OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "opcode 120 resolves FFF0 and skips matching bit28 roles"
    );

    Fixture controlled;
    prime_loaded_instruction(controlled, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 4U);
    write_u16(controlled.state.window, 6U, 5U);
    write_u16(controlled.state.window, 8U, 6U);
    write_u16(controlled.state.window, 10U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[2].action.action_id == 4U &&
            controlled.roles[2].action.base_variant == 5U &&
            controlled.roles[2].action.variant_delta == 6U &&
            controlled.state.previous_opcode ==
                OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "opcode 120 FFFE selects the controlled role directly"
    );

    Fixture selector_truncated;
    prime_loaded_instruction(
        selector_truncated, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.state.previous_opcode = 0x66U;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    const auto selector_truncated_result = selector_truncated.step();

    Fixture action_truncated;
    action_truncated.roles[1].action.action_id = 0x11111111U;
    action_truncated.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(
        action_truncated, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    action_truncated.context.instruction_offset = 0x7FFCU;
    action_truncated.state.previous_opcode = 0x66U;
    write_u16(
        action_truncated.state.window, 0x7FFCU, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(action_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto action_truncated_result = action_truncated.step();

    Fixture base_truncated;
    base_truncated.roles[1].action.action_id = 0x11111111U;
    base_truncated.roles[1].action.base_variant = 0x22222222U;
    base_truncated.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(base_truncated, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    base_truncated.context.instruction_offset = 0x7FFAU;
    base_truncated.state.previous_opcode = 0x66U;
    write_u16(
        base_truncated.state.window, 0x7FFAU, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(base_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(base_truncated.state.window, 0x7FFEU, 0x8000U);
    const auto base_truncated_result = base_truncated.step();

    Fixture variant_truncated;
    variant_truncated.roles[1].action.action_id = 0x11111111U;
    variant_truncated.roles[1].action.base_variant = 0x22222222U;
    variant_truncated.roles[1].action.variant_delta = 0x33333333U;
    variant_truncated.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(
        variant_truncated, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    variant_truncated.context.instruction_offset = 0x7FF8U;
    variant_truncated.state.previous_opcode = 0x66U;
    write_u16(
        variant_truncated.state.window,
        0x7FF8U,
        OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(variant_truncated.state.window, 0x7FFAU, 0x00F8U);
    write_u16(variant_truncated.state.window, 0x7FFCU, 0x8000U);
    write_u16(variant_truncated.state.window, 0x7FFEU, 0x7FFFU);
    const auto variant_truncated_result = variant_truncated.step();

    Fixture missing_variant_truncated;
    prime_loaded_instruction(
        missing_variant_truncated, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    missing_variant_truncated.context.instruction_offset = 0x7FF8U;
    missing_variant_truncated.state.previous_opcode = 0x66U;
    write_u16(
        missing_variant_truncated.state.window,
        0x7FF8U,
        OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(missing_variant_truncated.state.window, 0x7FFAU, 0x7777U);
    write_u16(missing_variant_truncated.state.window, 0x7FFCU, 1U);
    write_u16(missing_variant_truncated.state.window, 0x7FFEU, 2U);
    const auto missing_variant_truncated_result =
        missing_variant_truncated.step();

    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.ports.action_update_count == 0U &&
            selector_truncated.ports.role_patch_requests.empty() &&
            selector_truncated.state.previous_opcode == 0x66U &&
            action_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            action_truncated.roles[1].action.action_id == 0x11111111U &&
            action_truncated.roles[1].action.wait_remaining == 7U &&
            action_truncated.ports.action_update_count == 0U &&
            action_truncated.state.previous_opcode == 0x66U &&
            base_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            base_truncated.roles[1].action.action_id == 0xFFFF8000U &&
            base_truncated.roles[1].action.base_variant == 0x22222222U &&
            base_truncated.roles[1].action.wait_remaining == 7U &&
            base_truncated.ports.action_update_count == 0U &&
            base_truncated.state.previous_opcode == 0x66U &&
            variant_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            variant_truncated.roles[1].action.action_id == 0xFFFF8000U &&
            variant_truncated.roles[1].action.base_variant == 0x00007FFFU &&
            variant_truncated.roles[1].action.variant_delta == 0x33333333U &&
            variant_truncated.roles[1].action.wait_remaining == 7U &&
            variant_truncated.ports.action_update_count == 0U &&
            variant_truncated.state.previous_opcode == 0x66U &&
            missing_variant_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_variant_truncated.ports.action_update_count == 0U &&
            missing_variant_truncated.ports.role_patch_requests.empty() &&
            missing_variant_truncated.state.previous_opcode == 0x66U,
        "opcode 120 preserves each staged write before the next operand fault"
    );

    Fixture update_failure;
    update_failure.roles[1].action.action_id = 0x11111111U;
    update_failure.roles[1].action.base_variant = 0x22222222U;
    update_failure.roles[1].action.variant_delta = 0x33333333U;
    update_failure.roles[1].action.wait_remaining = 7U;
    update_failure.roles[1].flags = 0x200U;
    update_failure.ports.action_update_result = 0U;
    bool callback_observed_order = false;
    update_failure.ports.action_update_callback = [&](const auto& action,
                                                      const u32) {
        callback_observed_order = action.action_id == 0x11111111U &&
            action.base_variant == 0x22222222U &&
            action.variant_delta == 0x33333333U &&
            action.wait_remaining == 0U &&
            (update_failure.roles[1].flags & 0x00001000U) == 0U;
    };
    prime_loaded_instruction(update_failure, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(update_failure.state.window, 2U, 0x00F8U);
    write_u16(update_failure.state.window, 4U, 0xFFFFU);
    write_u16(update_failure.state.window, 6U, 0xFFFFU);
    write_u16(update_failure.state.window, 8U, 0xFFFFU);
    write_u16(update_failure.state.window, 10U, kStoryVmTypedStop);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            callback_observed_order &&
            (update_failure.roles[1].flags & 0x00001000U) != 0U &&
            update_failure.context.instruction_offset == 10U &&
            update_failure.state.previous_opcode ==
                OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "opcode 120 clears wait before refresh and marks flags after failure"
    );

    Fixture missing;
    missing.context.source_guid = 0x7777U;
    prime_loaded_instruction(missing, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    write_u16(missing.state.window, 2U, 0xFFF0U);
    write_u16(missing.state.window, 4U, 0x8000U);
    write_u16(missing.state.window, 6U, 0xFFFEU);
    write_u16(missing.state.window, 8U, 0x8123U);
    write_u16(missing.state.window, 10U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    const auto missing_patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing_result.action_update_count == 0U &&
            missing_result.direct_audio_service_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            missing_patch.guid == 0x7777U &&
            missing_patch.action_id == 0x8000U &&
            missing_patch.base_variant == 0xFFFEU &&
            missing_patch.variant_delta == 0x8123U &&
            missing_patch.flags_or_mask == 0x1000U &&
            missing_patch.flags_and_mask == 0xFFFFU &&
            missing.context.instruction_offset == 10U &&
            missing.state.previous_opcode == OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "opcode 120 missing role patch keeps raw words after FFF0 replacement"
    );

    Fixture found_tail;
    prime_loaded_instruction(found_tail, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    found_tail.context.instruction_offset = 0x7FF6U;
    write_u16(
        found_tail.state.window, 0x7FF6U, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(found_tail.state.window, 0x7FF8U, 0x00F8U);
    write_u16(found_tail.state.window, 0x7FFAU, 1U);
    write_u16(found_tail.state.window, 0x7FFCU, 2U);
    write_u16(found_tail.state.window, 0x7FFEU, 3U);
    const auto found_tail_result = found_tail.step();

    Fixture missing_tail;
    prime_loaded_instruction(missing_tail, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    missing_tail.context.instruction_offset = 0x7FF6U;
    write_u16(
        missing_tail.state.window, 0x7FF6U, OP_120_UPDATE_ROLE_ACTION_FIELDS
    );
    write_u16(missing_tail.state.window, 0x7FF8U, 0x7777U);
    write_u16(missing_tail.state.window, 0x7FFAU, 4U);
    write_u16(missing_tail.state.window, 0x7FFCU, 5U);
    write_u16(missing_tail.state.window, 0x7FFEU, 6U);
    const auto missing_tail_result = missing_tail.step();

    test.expect_true(
        found_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            found_tail_result.executed_instruction_count == 1U &&
            found_tail.context.instruction_offset == 0x8000U &&
            found_tail.state.previous_opcode ==
                OP_120_UPDATE_ROLE_ACTION_FIELDS &&
            found_tail.roles[1].action.action_id == 1U &&
            found_tail.roles[1].action.base_variant == 2U &&
            found_tail.roles[1].action.variant_delta == 3U &&
            (found_tail.roles[1].flags & 0x00001000U) != 0U &&
            missing_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            missing_tail_result.executed_instruction_count == 1U &&
            missing_tail.context.instruction_offset == 0x8000U &&
            missing_tail.state.previous_opcode ==
                OP_120_UPDATE_ROLE_ACTION_FIELDS &&
            missing_tail.ports.role_patch_requests.size() == 1U,
        "opcode 120 commits found and missing exact tails before refetch"
    );
}

void test_missing_role_position_patch(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 40U);
    write_u16(script, 2U, 0x7777U);
    write_u16(script, 4U, 0x8123U);
    write_u16(script, 6U, 0xFEDCU);
    write_u16(script, 8U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 10U, 0x00F8U);

    const auto result = fixture.step();
    const auto patch = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 12U &&
            fixture.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.tile_x == 0x8123U &&
            patch.tile_y == 0xFEDCU && patch.flags_or_mask == 0U &&
            patch.flags_and_mask == 0xFFFFU && patch.action_id == 0xFFFFU &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU,
        "opcode 40 preserves raw tile words in the MAPS fallback and consumes " "eight bytes when the role is absent"
    );
}

void test_change_role_base_variant_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x000AU, 0x400AU, 0x800AU, 0xC00AU
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFF0U);
        write_u16(fixture.state.window, 4U, 0x1234U);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.roles[1].action.wait_remaining = 9U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 6U &&
                fixture.context.instruction_offset == 6U &&
                fixture.roles[1].action.base_variant == 0x1234U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                result.action_update_count == 1U &&
                fixture.state.previous_opcode == 10U &&
                fixture.ports.role_patch_requests.empty(),
            "opcode 10 aliases update the live role and continue"
        );
    }

    Fixture missing;
    prime_loaded_instruction(missing, 10U);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x0333U);
    write_u16(missing.state.window, 6U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == 10U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.base_variant == 0x0333U &&
            patch.action_id == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 10 patches the MAPS source when the live role is absent"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, 10U);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x1234U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled_result.action_update_count == 0U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 10 stops at an invalid controlled-role live access"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, 10U);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.action_update_count == 0U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 10 short payload fails before role or MAPS mutation"
    );
}

void test_change_role_variant_delta_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x000BU, 0x400BU, 0x800BU, 0xC00BU
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFF0U);
        write_u16(fixture.state.window, 4U, 0x8123U);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.roles[1].flags = 0xA5000001U;
        fixture.roles[1].action.wait_remaining = 9U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.roles[1].action.variant_delta == 0x8123U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[1].flags == 0xA5001001U &&
                result.action_update_count == 1U &&
                fixture.state.previous_opcode == 11U &&
                fixture.ports.role_patch_requests.empty(),
            "opcode 11 aliases update the live role and continue"
        );
    }

    Fixture missing;
    prime_loaded_instruction(missing, 11U);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x8123U);
    write_u16(missing.state.window, 6U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == 11U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.variant_delta == 0x8123U &&
            patch.action_id == 0xFFFFU && patch.base_variant == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 11 patches the MAPS source when the live role is absent"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, 11U);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8123U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled_result.action_update_count == 0U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 11 stops at an invalid controlled-role live access"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, 11U);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.action_update_count == 0U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 11 short payload fails before role or MAPS mutation"
    );

    Fixture chained;
    prime_loaded_instruction(chained, 11U);
    write_u16(chained.state.window, 2U, 0x00F8U);
    write_u16(chained.state.window, 4U, 3U);
    write_u16(chained.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 8U, 0x00F8U);
    write_u16(chained.state.window, 10U, 0x0222U);
    write_u16(chained.state.window, 12U, kStoryVmTypedStop);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            chained_result.executed_instruction_count == 3U &&
            chained_result.action_update_count == 1U &&
            chained.roles[1].action.variant_delta == 3U &&
            chained.roles[1].action.action_id == 0x0222U &&
            (chained.roles[1].flags & 0x00001000U) != 0U,
        "opcode 11 defers its action update across a same-role opcode 45"
    );
}

void test_set_role_position_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_12_SET_ROLE_POSITION,
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0x4000U),
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0x8000U),
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0x02080001U;
        fixture.roles[1].action.cached_base_variant = 7U;
        fixture.roles[1].action.cached_variant_delta = 8U;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x1015U);
        write_u16(fixture.state.window, 6U, 0x100FU);
        write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        const auto slot =
            std::span<const u8>{fixture.active_object_slots[0].bytes};
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode == OP_12_SET_ROLE_POSITION &&
                fixture.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
                fixture.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
                (fixture.roles[1].flags & 0x02080000U) == 0U &&
                read_u16(slot, 0x00U) == 1U && read_u16(slot, 0x04U) == 336U &&
                read_u16(slot, 0x06U) == 240U && (slot[0x1BU] & 0x0FU) == 2U &&
                fixture.dialogs.close.flagged_dialog_counter == 0U,
            "opcode 12 aliases schedule wrapped coordinates and continue"
        );
    }

    Fixture current_alias;
    current_alias.roles[1].flags = 0x00080000U;
    current_alias.roles[1].action.cached_base_variant = 7U;
    current_alias.roles[1].action.cached_variant_delta = 8U;
    StoryPathHarness alias_paths{current_alias};
    prime_loaded_instruction(current_alias, OP_12_SET_ROLE_POSITION);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    write_u16(current_alias.state.window, 4U, 22U);
    write_u16(current_alias.state.window, 6U, 15U);
    write_u16(current_alias.state.window, 8U, kStoryVmTypedStop);
    const auto alias_result = current_alias.step();
    test.expect_true(
        alias_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_alias.roles[1].action.cached_base_variant == 7U &&
            current_alias.roles[1].action.cached_variant_delta == 8U &&
            (current_alias.roles[1].flags & 0x00080000U) != 0U,
        "opcode 12 substitutes FFF0 for lookup but not for raw cache reset"
    );

    Fixture controlled;
    StoryPathHarness controlled_paths{controlled, 1U};
    prime_loaded_instruction(controlled, OP_12_SET_ROLE_POSITION);
    write_u16(controlled.state.window, 2U, 0x00F8U);
    write_u16(controlled.state.window, 4U, 21U);
    write_u16(controlled.state.window, 6U, 15U);
    write_u16(controlled.state.window, 8U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.dialogs.close.flagged_dialog_counter == 0x8000U &&
            controlled.context.instruction_offset == 8U &&
            controlled.state.previous_opcode == OP_12_SET_ROLE_POSITION,
        "opcode 12 marks dialog state when the target is controlled"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_12_SET_ROLE_POSITION);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 21U);
    write_u16(missing.state.window, 6U, 15U);
    write_u16(missing.state.window, 8U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 8U &&
            missing.state.previous_opcode == OP_12_SET_ROLE_POSITION &&
            missing.dialogs.close.flagged_dialog_counter == 0U,
        "opcode 12 consumes an ordinary missing role without scheduling"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_12_SET_ROLE_POSITION);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 21U);
    write_u16(invalid_controlled.state.window, 6U, 15U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 12 stops before an invalid controlled-role schedule"
    );

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_12_SET_ROLE_POSITION);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    write_u16(unavailable.state.window, 4U, 21U);
    write_u16(unavailable.state.window, 6U, 15U);
    unavailable.roles[1].flags = 0x00080000U;
    unavailable.roles[1].action.cached_base_variant = 7U;
    unavailable.roles[1].action.cached_variant_delta = 8U;
    unavailable.state.previous_opcode = 0x55U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x55U &&
            unavailable.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
            unavailable.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
            (unavailable.roles[1].flags & 0x00080000U) == 0U,
        "opcode 12 preserves cache reset before a missing path runtime"
    );

    Fixture found_truncated;
    found_truncated.context.instruction_offset = 0x7FFCU;
    found_truncated.context.talk_data_offset = 0x1111U;
    found_truncated.state.loaded_file_number = 1U;
    found_truncated.state.loaded_data_offset = 0x1111U;
    found_truncated.state.window_loaded = true;
    write_u16(found_truncated.state.window, 0x7FFCU, OP_12_SET_ROLE_POSITION);
    write_u16(found_truncated.state.window, 0x7FFEU, 0x00F8U);
    found_truncated.roles[1].flags = 0x00080000U;
    found_truncated.roles[1].action.cached_base_variant = 7U;
    found_truncated.state.previous_opcode = 0x55U;
    const auto found_truncated_result = found_truncated.step();
    test.expect_true(
        found_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_truncated.context.instruction_offset == 0x7FFCU &&
            found_truncated.state.previous_opcode == 0x55U &&
            found_truncated.roles[1].action.cached_base_variant ==
                0xFFFFFFFFU &&
            (found_truncated.roles[1].flags & 0x00080000U) == 0U,
        "opcode 12 checks found-role operands after the original cache reset"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    write_u16(missing_truncated.state.window, 0x7FFCU, OP_12_SET_ROLE_POSITION);
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    missing_truncated.state.previous_opcode = 0x55U;
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
            LegacyWorldStoryVmStatus::operand_out_of_range,
        "opcode 12 missing short status"
    );
    test.expect_true(
        missing_truncated.context.instruction_offset == 0x8004U,
        "opcode 12 missing short instruction offset"
    );
    test.expect_true(
        missing_truncated.state.previous_opcode == OP_12_SET_ROLE_POSITION,
        "opcode 12 missing short previous"
    );
}

void test_step_role_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_13_STEP_ROLE,
        static_cast<u16>(OP_13_STEP_ROLE | 0x4000U),
        static_cast<u16>(OP_13_STEP_ROLE | 0x8000U),
        static_cast<u16>(OP_13_STEP_ROLE | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        const auto scheduled =
            openswd3::world_map::schedule_legacy_world_story_path(
                paths.runtime,
                openswd3::world_map::LegacyWorldStoryPathRequest{
                    .role_index = 1U,
                    .destination_x = 336U,
                    .destination_y = 240U,
                }
            );
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status ==
                    openswd3::world_map::LegacyWorldStoryPathStatus::
                        completed &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_13_STEP_ROLE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_13_STEP_ROLE &&
                read_u16(slot, 0x02U) == 0U && read_u16(slot, 0x16U) == 4U &&
                read_u16(slot, 0x18U) == 0U &&
                (fixture.roles[1].flags & 0x40000000U) != 0U,
            "opcode 13 aliases arm one path step, service audio, and yield"
        );
    }

    Fixture current_alias;
    current_alias.roles[1].flags = 0x02000000U;
    prime_loaded_instruction(current_alias, OP_13_STEP_ROLE);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    const auto current_alias_result = current_alias.step();
    test.expect_true(
        current_alias_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_alias_result.direct_audio_service_count == 1U &&
            current_alias.context.instruction_offset == 4U &&
            current_alias.state.previous_opcode == OP_13_STEP_ROLE &&
            current_alias.roles[1].flags == 0x02000000U,
        "opcode 13 substitutes FFF0 and skips an already stepped role"
    );

    Fixture no_slot;
    StoryPathHarness no_slot_paths{no_slot};
    prime_loaded_instruction(no_slot, OP_13_STEP_ROLE);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::yielded &&
            no_slot_result.direct_audio_service_count == 1U &&
            no_slot.context.instruction_offset == 4U &&
            no_slot.state.previous_opcode == OP_13_STEP_ROLE &&
            std::ranges::all_of(
                no_slot.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ),
        "opcode 13 ignores the helper's no matching slot return"
    );

    Fixture arrived;
    StoryPathHarness arrived_paths{arrived};
    auto& arrived_slot = arrived.active_object_slots[0].bytes;
    arrived_slot.fill(0xFFU);
    arrived_slot[0x00U] = 1U;
    arrived_slot[0x01U] = 0U;
    arrived_slot[0x02U] = 0U;
    arrived_slot[0x03U] = 0U;
    arrived_slot[0x1BU] = 2U;
    arrived_slot[0x1CU] = 0xFFU;
    arrived.roles[1].path_wait_remaining = 7U;
    arrived.roles[1].flags = 0x44000000U;
    prime_loaded_instruction(arrived, OP_13_STEP_ROLE);
    write_u16(arrived.state.window, 2U, 0x00F8U);
    const auto arrived_result = arrived.step();
    test.expect_true(
        arrived_result.status == LegacyWorldStoryVmStatus::yielded &&
            arrived_result.direct_audio_service_count == 1U &&
            arrived.roles[1].path_wait_remaining == 0U &&
            arrived.roles[1].flags == 0U &&
            arrived.context.instruction_offset == 4U &&
            arrived.state.previous_opcode == OP_13_STEP_ROLE,
        "opcode 13 ignores the helper's arrived return after its side effects"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_13_STEP_ROLE);
    write_u16(missing.state.window, 2U, 0x7777U);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.direct_audio_service_count == 1U &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_13_STEP_ROLE,
        "opcode 13 consumes an ordinary missing role and yields"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_13_STEP_ROLE);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.direct_audio_service_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 13 stops before an invalid controlled-role flag read"
    );

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_13_STEP_ROLE);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    unavailable.state.previous_opcode = 0x55U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_result.direct_audio_service_count == 0U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x55U,
        "opcode 13 stops at the helper call when path runtime is unavailable"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, OP_13_STEP_ROLE);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U,
        "opcode 13 checks its selector before any side effect"
    );
}

void test_wait_role_action_status_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_14_WAIT_ROLE_ACTION_STATUS,
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0x4000U),
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0x8000U),
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        fixture.roles[1].interaction_gate = 2U;
        fixture.state.previous_opcode = 0x55U;
        const auto waiting = fixture.step();
        const u16 waiting_offset = fixture.context.instruction_offset;
        fixture.roles[1].interaction_gate = 0U;
        const auto completed = fixture.step();
        test.expect_true(
            waiting.status == LegacyWorldStoryVmStatus::yielded &&
                waiting.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
                waiting.executed_instruction_count == 1U &&
                waiting.direct_audio_service_count == 1U &&
                waiting_offset == 0U &&
                completed.status == LegacyWorldStoryVmStatus::yielded &&
                completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
                completed.executed_instruction_count == 1U &&
                completed.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
            "opcode 14 aliases yield until role action status clears"
        );
    }

    Fixture current_alias;
    prime_loaded_instruction(current_alias, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    current_alias.roles[1].interaction_gate = 0U;
    const auto current_alias_result = current_alias.step();
    test.expect_true(
        current_alias_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_alias_result.direct_audio_service_count == 1U &&
            current_alias.context.instruction_offset == 4U &&
            current_alias.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 resolves FFF0 before reading role action status"
    );

    Fixture context_alias;
    context_alias.context.source_guid = 0xFFFDU;
    context_alias.context.field_26 = 1U;
    prime_loaded_instruction(context_alias, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(context_alias.state.window, 2U, 0xFFF0U);
    const auto context_waiting = context_alias.step();
    const u16 context_waiting_offset = context_alias.context.instruction_offset;
    context_alias.context.field_26 = 0U;
    const auto context_completed = context_alias.step();
    test.expect_true(
        context_waiting.status == LegacyWorldStoryVmStatus::yielded &&
            context_waiting.direct_audio_service_count == 1U &&
            context_waiting_offset == 0U &&
            context_completed.status == LegacyWorldStoryVmStatus::yielded &&
            context_completed.direct_audio_service_count == 1U &&
            context_alias.context.instruction_offset == 4U &&
            context_alias.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 recognizes FFFD after FFF0 source substitution"
    );

    Fixture direct_context;
    direct_context.context.field_26 = 0U;
    prime_loaded_instruction(direct_context, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(direct_context.state.window, 2U, 0xFFFDU);
    const auto direct_context_result = direct_context.step();
    test.expect_true(
        direct_context_result.status == LegacyWorldStoryVmStatus::yielded &&
            direct_context_result.direct_audio_service_count == 1U &&
            direct_context.context.instruction_offset == 4U &&
            direct_context.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 reads direct FFFD context action status"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing.state.window, 2U, 0x7777U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.direct_audio_service_count == 0U &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 14 stops at an ordinary missing-role status read"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.direct_audio_service_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 14 stops before an invalid controlled-role status read"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, OP_14_WAIT_ROLE_ACTION_STATUS);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U,
        "opcode 14 checks its selector before any side effect"
    );
}

void test_jump_same_file_offset_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_15_JUMP_SAME_FILE_OFFSET,
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0x4000U),
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0x8000U),
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0xC000U)
    };
    constexpr u32 target = 0x12345678U;
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u32(fixture.state.window, 2U, target);
        write_u16(
            fixture.ports.transferred_window, 0U, OP_59_PLAY_SOUND_EFFECT
        );
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 2U &&
                fixture.context.talk_data_offset == target &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.loaded_file_number == 1U &&
                fixture.state.loaded_data_offset == target &&
                fixture.state.window_loaded &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.last_data_file_number == 1U &&
                fixture.ports.last_data_offset == target &&
                !fixture.ports.last_data_clear_before_read &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{0x1234U} &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 5U, 2U},
            "opcode 15 aliases service audio, load, publish, and same-call fetch"
        );
    }

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u32(load_failure.state.window, 2U, 0x87654321U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x87654321U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 15 checked I/O failure preserves earlier legacy side effects"
    );

    Fixture boundary;
    boundary.context.instruction_offset = 0x7FFAU;
    boundary.context.talk_data_offset = 0x1111U;
    boundary.state.loaded_file_number = 1U;
    boundary.state.loaded_data_offset = 0x1111U;
    boundary.state.window_loaded = true;
    write_u16(boundary.state.window, 0x7FFAU, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u32(boundary.state.window, 0x7FFCU, 0x33445566U);
    write_u16(boundary.ports.transferred_window, 0U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(boundary.ports.transferred_window, 2U, 0x4321U);
    const auto boundary_result = boundary.step();
    test.expect_true(
        boundary_result.status == LegacyWorldStoryVmStatus::yielded &&
            boundary_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            boundary_result.executed_instruction_count == 2U &&
            boundary.context.talk_data_offset == 0x33445566U &&
            boundary.context.instruction_offset == 4U,
        "opcode 15 accepts an exact six-byte window-tail payload"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u16(truncated.state.window, 0x7FFEU, 0x5566U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.talk_data_offset == 0x1111U &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U,
        "opcode 15 checks its u32 target before helper side effects"
    );
}

void test_jump_if_role_path_unprepared_protocol(openswd3::test::Context& test) {
    const auto configure_slot = [](Fixture& fixture, const u16 role_index) {
        write_u16(fixture.active_object_slots[0].bytes, 0U, role_index);
        fixture.active_object_slots[0].bytes[0x1BU] = 2U;
    };
    const auto configure_jump = [](Fixture& fixture, const u32 target) {
        write_u32(fixture.state.window, 4U, target);
        write_u16(
            fixture.ports.transferred_window, 0U, OP_59_PLAY_SOUND_EFFECT
        );
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
    };

    constexpr std::array<u16, 4U> raw_aliases{
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED,
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0x4000U),
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0x8000U),
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        configure_jump(fixture, 0x12345678U);
        configure_slot(fixture, 1U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 5U, 2U},
            "opcode 16 aliases jump when a type-2 role path is unprepared"
        );
    }

    Fixture no_slot;
    prime_loaded_instruction(no_slot, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    write_u32(no_slot.state.window, 4U, 0x12345678U);
    write_u16(no_slot.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(no_slot.state.window, 10U, 0x2345U);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::yielded &&
            no_slot_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            no_slot_result.executed_instruction_count == 2U &&
            no_slot_result.direct_audio_service_count == 1U &&
            no_slot.context.talk_data_offset == 0x1111U &&
            no_slot.context.instruction_offset == 12U &&
            no_slot.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            no_slot.ports.data_load_count == 0U,
        "opcode 16 advances eight bytes when no type-2 role path exists"
    );

    Fixture prepared;
    prepared.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(prepared, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(prepared.state.window, 2U, 0x00F8U);
    write_u32(prepared.state.window, 4U, 0x12345678U);
    write_u16(prepared.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(prepared.state.window, 10U, 0x3456U);
    configure_slot(prepared, 1U);
    const auto prepared_result = prepared.step();
    test.expect_true(
        prepared_result.status == LegacyWorldStoryVmStatus::yielded &&
            prepared_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            prepared_result.direct_audio_service_count == 1U &&
            prepared.context.instruction_offset == 12U &&
            prepared.ports.data_load_count == 0U,
        "opcode 16 does not jump after the role path step is prepared"
    );

    Fixture ordinary_missing;
    prime_loaded_instruction(
        ordinary_missing, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(ordinary_missing.state.window, 2U, 0x7777U);
    write_u32(ordinary_missing.state.window, 4U, 0x22223333U);
    write_u16(ordinary_missing.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(ordinary_missing.state.window, 10U, 0x2345U);
    configure_slot(ordinary_missing, 0U);
    const auto ordinary_missing_result = ordinary_missing.step();
    test.expect_true(
        ordinary_missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_missing_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            ordinary_missing.context.talk_data_offset == 0x1111U &&
            ordinary_missing.context.instruction_offset == 12U &&
            ordinary_missing.ports.data_load_count == 0U,
        "opcode 16 preserves resolver miss output FFFFFFFF"
    );

    Fixture raw_current_token;
    prime_loaded_instruction(
        raw_current_token, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u32(raw_current_token.state.window, 4U, 0x33334444U);
    write_u16(raw_current_token.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(raw_current_token.state.window, 10U, 0x3456U);
    configure_slot(raw_current_token, 1U);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status == LegacyWorldStoryVmStatus::yielded &&
            raw_current_token_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            raw_current_token.context.talk_data_offset == 0x1111U &&
            raw_current_token.context.instruction_offset == 12U &&
            raw_current_token.ports.data_load_count == 0U,
        "opcode 16 passes FFF0 raw to the resolver without source substitution"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u32(invalid_controlled.state.window, 4U, 0x44445555U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.data_load_count == 0U,
        "opcode 16 obeys the VM controlled-role entry safety boundary"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(load_failure.state.window, 2U, 0x00F8U);
    configure_jump(load_failure, 0x66667777U);
    configure_slot(load_failure, 1U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_seek_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x66667777U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            !load_failure.state.window_loaded,
        "opcode 16 branch preserves helper effects before checked I/O failure"
    );

    Fixture branch_truncated;
    branch_truncated.context.instruction_offset = 0x7FFCU;
    branch_truncated.context.talk_data_offset = 0x1111U;
    branch_truncated.state.loaded_file_number = 1U;
    branch_truncated.state.loaded_data_offset = 0x1111U;
    branch_truncated.state.window_loaded = true;
    write_u16(
        branch_truncated.state.window,
        0x7FFCU,
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(branch_truncated, 1U);
    branch_truncated.state.previous_opcode = 0x55U;
    const auto branch_truncated_result = branch_truncated.step();
    test.expect_true(
        branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            branch_truncated.context.instruction_offset == 0x7FFCU &&
            branch_truncated.state.previous_opcode == 0x55U &&
            branch_truncated.ports.data_load_count == 0U,
        "opcode 16 branch reads the u32 target only after its slot predicate"
    );

    Fixture no_branch_truncated;
    no_branch_truncated.context.instruction_offset = 0x7FFCU;
    no_branch_truncated.context.talk_data_offset = 0x1111U;
    no_branch_truncated.state.loaded_file_number = 1U;
    no_branch_truncated.state.loaded_data_offset = 0x1111U;
    no_branch_truncated.state.window_loaded = true;
    write_u16(
        no_branch_truncated.state.window,
        0x7FFCU,
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(no_branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto no_branch_truncated_result = no_branch_truncated.step();
    test.expect_true(
        no_branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_branch_truncated_result.executed_instruction_count == 1U &&
            no_branch_truncated.context.instruction_offset == 0x8004U &&
            no_branch_truncated.state.previous_opcode ==
                OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            no_branch_truncated.ports.data_load_count == 0U,
        "opcode 16 no-branch path advances without reading an absent target"
    );
}

void test_jump_if_role_path_prepared_protocol(openswd3::test::Context& test) {
    const auto configure_slot = [](Fixture& fixture) {
        write_u16(fixture.active_object_slots[0].bytes, 0U, 1U);
        fixture.active_object_slots[0].bytes[0x1BU] = 2U;
    };
    constexpr std::array<u16, 4U> raw_aliases{
        OP_17_JUMP_IF_ROLE_PATH_PREPARED,
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0x4000U),
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0x8000U),
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0x40000000U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u32(fixture.state.window, 4U, 0x12345678U);
        write_u16(
            fixture.ports.transferred_window, 0U, OP_59_PLAY_SOUND_EFFECT
        );
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
        configure_slot(fixture);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.data_load_count == 1U,
            "opcode 17 aliases jump when a type-2 role path is prepared"
        );
    }

    Fixture unprepared;
    prime_loaded_instruction(unprepared, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(unprepared.state.window, 2U, 0x00F8U);
    write_u32(unprepared.state.window, 4U, 0x22223333U);
    write_u16(unprepared.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(unprepared.state.window, 10U, 0x2345U);
    configure_slot(unprepared);
    const auto unprepared_result = unprepared.step();
    test.expect_true(
        unprepared_result.status == LegacyWorldStoryVmStatus::yielded &&
            unprepared_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            unprepared_result.direct_audio_service_count == 1U &&
            unprepared.context.instruction_offset == 12U &&
            unprepared.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            unprepared.ports.data_load_count == 0U,
        "opcode 17 advances when the matching role path is unprepared"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u32(missing.state.window, 4U, 0x33334444U);
    write_u16(missing.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(missing.state.window, 10U, 0x3456U);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            missing.context.instruction_offset == 12U &&
            missing.ports.data_load_count == 0U,
        "opcode 17 resolver miss output FFFFFFFF takes no-branch"
    );

    Fixture raw_current_token;
    raw_current_token.context.source_guid = 0x00F8U;
    raw_current_token.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(
        raw_current_token, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u32(raw_current_token.state.window, 4U, 0x33334444U);
    write_u16(raw_current_token.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(raw_current_token.state.window, 10U, 0x3456U);
    configure_slot(raw_current_token);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status == LegacyWorldStoryVmStatus::yielded &&
            raw_current_token_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            raw_current_token.context.instruction_offset == 12U &&
            raw_current_token.ports.data_load_count == 0U,
        "opcode 17 passes FFF0 raw without source substitution"
    );

    Fixture wrong_type;
    wrong_type.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(wrong_type, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(wrong_type.state.window, 2U, 0x00F8U);
    write_u32(wrong_type.state.window, 4U, 0x33334444U);
    write_u16(wrong_type.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(wrong_type.state.window, 10U, 0x3456U);
    configure_slot(wrong_type);
    wrong_type.active_object_slots[0].bytes[0x1BU] = 3U;
    const auto wrong_type_result = wrong_type.step();
    test.expect_true(
        wrong_type_result.status == LegacyWorldStoryVmStatus::yielded &&
            wrong_type_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            wrong_type.context.instruction_offset == 12U &&
            wrong_type.ports.data_load_count == 0U,
        "opcode 17 requires active-slot type low nibble two"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.data_load_count == 0U,
        "opcode 17 obeys the VM controlled-role entry safety boundary"
    );

    Fixture load_failure;
    load_failure.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(load_failure, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(load_failure.state.window, 2U, 0x00F8U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    configure_slot(load_failure);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.state.previous_opcode ==
                OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            !load_failure.state.window_loaded,
        "opcode 17 preserves branch helper effects before I/O failure"
    );

    Fixture truncated;
    truncated.roles[1].flags = 0x40000000U;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(
        truncated.state.window, 0x7FFCU, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(truncated);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U,
        "opcode 17 prepared branch reads the target at its original danger point"
    );

    Fixture no_branch_truncated;
    no_branch_truncated.context.instruction_offset = 0x7FFCU;
    no_branch_truncated.context.talk_data_offset = 0x1111U;
    no_branch_truncated.state.loaded_file_number = 1U;
    no_branch_truncated.state.loaded_data_offset = 0x1111U;
    no_branch_truncated.state.window_loaded = true;
    write_u16(
        no_branch_truncated.state.window,
        0x7FFCU,
        OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(no_branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(no_branch_truncated);
    no_branch_truncated.state.previous_opcode = 0x55U;
    const auto no_branch_truncated_result = no_branch_truncated.step();
    test.expect_true(
        no_branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_branch_truncated_result.executed_instruction_count == 1U &&
            no_branch_truncated.context.instruction_offset == 0x8004U &&
            no_branch_truncated.state.previous_opcode ==
                OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            no_branch_truncated.ports.data_load_count == 0U,
        "opcode 17 unprepared no-branch advances before the next fetch fails"
    );
}

void test_role_action_chain_update_gate(openswd3::test::Context& test) {
    const auto run_chain = [](const u16 second_opcode) {
        Fixture fixture;
        auto script = std::span<u8>{fixture.ports.initial_window};
        write_u16(script, 0U, 10U);
        write_u16(script, 2U, 0x00F8U);
        write_u16(script, 4U, 2U);
        write_u16(script, 6U, second_opcode);
        write_u16(script, 8U, 0x00F8U);
        write_u16(script, 10U, 3U);
        write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
        write_u16(script, 14U, 0x00F8U);
        const auto result = fixture.step();
        return std::tuple{result, fixture.roles[1]};
    };

    const auto [plain_result, plain_role] = run_chain(11U);
    const auto [flagged_result, flagged_role] = run_chain(0x400BU);
    test.expect_true(
        plain_result.status == LegacyWorldStoryVmStatus::yielded &&
            plain_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            plain_result.action_update_count == 2U &&
            plain_role.action.base_variant == 2U &&
            plain_role.action.variant_delta == 3U &&
            (plain_role.flags & 0x00001000U) != 0U,
        "opcodes 10 and 11 coalesce a same-role raw action chain"
    );
    test.expect_true(
        flagged_result.status == LegacyWorldStoryVmStatus::yielded &&
            flagged_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            flagged_result.action_update_count == 3U &&
            flagged_role.action.base_variant == 2U &&
            flagged_role.action.variant_delta == 3U,
        "sub_42E740 compares the next raw opcode without masking flag bits"
    );

    Fixture next_opcode_truncated;
    next_opcode_truncated.context.instruction_offset = 0x7FFAU;
    next_opcode_truncated.context.talk_data_offset = 0x1111U;
    next_opcode_truncated.state.loaded_file_number = 1U;
    next_opcode_truncated.state.loaded_data_offset = 0x1111U;
    next_opcode_truncated.state.window_loaded = true;
    next_opcode_truncated.roles[1].action.base_variant = 9U;
    next_opcode_truncated.roles[1].action.wait_remaining = 7U;
    write_u16(next_opcode_truncated.state.window, 0x7FFAU, 10U);
    write_u16(next_opcode_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(next_opcode_truncated.state.window, 0x7FFEU, 2U);
    next_opcode_truncated.state.previous_opcode = 0x55U;
    const auto next_opcode_truncated_result = next_opcode_truncated.step();
    test.expect_true(
        next_opcode_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_opcode_truncated_result.executed_instruction_count == 1U &&
            next_opcode_truncated_result.action_update_count == 0U &&
            next_opcode_truncated.roles[1].action.base_variant == 2U &&
            next_opcode_truncated.roles[1].action.wait_remaining == 0U &&
            next_opcode_truncated.context.instruction_offset == 0x7FFAU &&
            next_opcode_truncated.state.previous_opcode == 0x55U,
        "opcode 10 writes action fields before mandatory lookahead opcode access"
    );

    Fixture next_selector_truncated;
    next_selector_truncated.context.instruction_offset = 0x7FF8U;
    next_selector_truncated.context.talk_data_offset = 0x1111U;
    next_selector_truncated.state.loaded_file_number = 1U;
    next_selector_truncated.state.loaded_data_offset = 0x1111U;
    next_selector_truncated.state.window_loaded = true;
    next_selector_truncated.roles[1].action.variant_delta = 9U;
    next_selector_truncated.roles[1].action.wait_remaining = 7U;
    next_selector_truncated.roles[1].flags = 0x20U;
    write_u16(next_selector_truncated.state.window, 0x7FF8U, 11U);
    write_u16(next_selector_truncated.state.window, 0x7FFAU, 0x00F8U);
    write_u16(next_selector_truncated.state.window, 0x7FFCU, 3U);
    write_u16(
        next_selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    next_selector_truncated.state.previous_opcode = 0x55U;
    const auto next_selector_truncated_result = next_selector_truncated.step();
    test.expect_true(
        next_selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_selector_truncated_result.action_update_count == 0U &&
            next_selector_truncated.roles[1].action.variant_delta == 3U &&
            next_selector_truncated.roles[1].action.wait_remaining == 0U &&
            next_selector_truncated.roles[1].flags == 0x20U &&
            next_selector_truncated.context.instruction_offset == 0x7FF8U &&
            next_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 11 reads a recognized next selector before refresh and flags"
    );
}

void test_change_requested_action_id(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> aliases{
        OP_45_SET_ROLE_ACTION_ID,
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x4000U),
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x8000U),
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0xC000U),
    };
    for (const u16 raw_word : aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0xA4A50020U;
        fixture.roles[1].action.action_id = 0xDEADBEEFU;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8001U);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.roles[1].action.action_id == 0x00008001U &&
                fixture.roles[1].flags == 0xA4A51020U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 45 aliases write a zero-extended action id and set bit 12"
        );
    }

    Fixture current_source;
    current_source.roles[1].action.action_id = 0xDEADBEEFU;
    prime_loaded_instruction(current_source, OP_45_SET_ROLE_ACTION_ID);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 0xFFFFU);
    write_u16(current_source.state.window, 6U, kStoryVmTypedStop);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_source.roles[1].action.action_id == 0x0000FFFFU &&
            current_source.roles[0].action.action_id == 0U &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 45 translates FFF0 only for the current role lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.action_id = 0x1111U;
    controlled.roles[2].action.action_id = 0x2222U;
    prime_loaded_instruction(controlled, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 0x8123U);
    write_u16(controlled.state.window, 6U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[2].action.action_id == 0x8123U &&
            controlled.roles[1].action.action_id == 0x1111U &&
            (controlled.roles[2].flags & 0x1000U) != 0U,
        "opcode 45 passes FFFE through for controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[1].action.action_id = 0x1111U;
    first_clear_match.roles[2].action.action_id = 0x2222U;
    prime_loaded_instruction(first_clear_match, OP_45_SET_ROLE_ACTION_ID);
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, 0x8004U);
    write_u16(first_clear_match.state.window, 6U, kStoryVmTypedStop);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_clear_match.roles[0].action.action_id == 0U &&
            first_clear_match.roles[1].action.action_id == 0x8004U &&
            first_clear_match.roles[2].action.action_id == 0x2222U,
        "opcode 45 skips bit-28 roles and uses the first clear GUID match"
    );

    Fixture chained;
    prime_loaded_instruction(chained, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 2U, 0x00F8U);
    write_u16(chained.state.window, 4U, 0x0222U);
    write_u16(chained.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 8U, 0x00F8U);
    write_u16(chained.state.window, 10U, 0U);
    write_u16(chained.state.window, 12U, kStoryVmTypedStop);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            chained_result.executed_instruction_count == 3U &&
            chained_result.action_update_count == 1U &&
            chained.roles[1].action.action_id == 0U &&
            (chained.roles[1].flags & 0x1000U) != 0U &&
            chained.context.instruction_offset == 12U &&
            chained.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 coalesces an exact raw same-role chain and writes zero"
    );

    const auto run_field_chain = [](const u16 next_opcode) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_45_SET_ROLE_ACTION_ID);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x0111U);
        write_u16(fixture.state.window, 6U, next_opcode);
        write_u16(fixture.state.window, 8U, 0x00F8U);
        write_u16(fixture.state.window, 10U, 3U);
        write_u16(fixture.state.window, 12U, kStoryVmTypedStop);
        const auto result = fixture.step();
        return std::tuple{result, fixture.roles[1]};
    };
    const auto [base_chain_result, base_chain_role] =
        run_field_chain(OP_10_SET_ROLE_BASE_VARIANT);
    const auto [delta_chain_result, delta_chain_role] = run_field_chain(11U);
    test.expect_true(
        base_chain_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            base_chain_result.executed_instruction_count == 3U &&
            base_chain_result.action_update_count == 1U &&
            base_chain_role.action.action_id == 0x0111U &&
            base_chain_role.action.base_variant == 3U,
        "opcode 45 coalesces a same-role raw opcode 10 successor"
    );
    test.expect_true(
        delta_chain_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            delta_chain_result.executed_instruction_count == 3U &&
            delta_chain_result.action_update_count == 1U &&
            delta_chain_role.action.action_id == 0x0111U &&
            delta_chain_role.action.variant_delta == 3U,
        "opcode 45 coalesces a same-role raw opcode 11 successor"
    );

    Fixture aliased_next;
    prime_loaded_instruction(aliased_next, OP_45_SET_ROLE_ACTION_ID);
    write_u16(aliased_next.state.window, 2U, 0x00F8U);
    write_u16(aliased_next.state.window, 4U, 0x0111U);
    write_u16(
        aliased_next.state.window,
        6U,
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x4000U)
    );
    write_u16(aliased_next.state.window, 8U, 0x00F8U);
    write_u16(aliased_next.state.window, 10U, 0x0222U);
    write_u16(aliased_next.state.window, 12U, kStoryVmTypedStop);
    const auto aliased_next_result = aliased_next.step();
    test.expect_true(
        aliased_next_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            aliased_next_result.action_update_count == 2U &&
            aliased_next.roles[1].action.action_id == 0x0222U,
        "opcode 45 lookahead compares the next raw opcode without alias masking"
    );

    Fixture untranslated_next;
    prime_loaded_instruction(untranslated_next, OP_45_SET_ROLE_ACTION_ID);
    write_u16(untranslated_next.state.window, 2U, 0x00F8U);
    write_u16(untranslated_next.state.window, 4U, 0x0111U);
    write_u16(untranslated_next.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(untranslated_next.state.window, 8U, 0xFFF0U);
    write_u16(untranslated_next.state.window, 10U, 0x0222U);
    write_u16(untranslated_next.state.window, 12U, kStoryVmTypedStop);
    const auto untranslated_next_result = untranslated_next.step();
    test.expect_true(
        untranslated_next_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            untranslated_next_result.action_update_count == 2U &&
            untranslated_next.roles[1].action.action_id == 0x0222U,
        "opcode 45 lookahead does not translate the next FFF0 selector"
    );

    Fixture controlled_chain;
    prime_loaded_instruction(controlled_chain, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled_chain.state.window, 2U, 0xFFFEU);
    write_u16(controlled_chain.state.window, 4U, 0x0111U);
    write_u16(controlled_chain.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled_chain.state.window, 8U, 0xFFFEU);
    write_u16(controlled_chain.state.window, 10U, 0x0222U);
    write_u16(controlled_chain.state.window, 12U, kStoryVmTypedStop);
    const auto controlled_chain_result = controlled_chain.step();
    test.expect_true(
        controlled_chain_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled_chain_result.action_update_count == 1U &&
            controlled_chain.roles[0].action.action_id == 0x0222U,
        "opcode 45 lookahead preserves FFFE controlled-role selection"
    );

    Fixture update_failure;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(update_failure, OP_45_SET_ROLE_ACTION_ID);
    write_u16(update_failure.state.window, 2U, 0x00F8U);
    write_u16(update_failure.state.window, 4U, 0x8005U);
    write_u16(update_failure.state.window, 6U, kStoryVmTypedStop);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.roles[1].action.action_id == 0x8005U &&
            (update_failure.roles[1].flags & 0x1000U) != 0U &&
            update_failure.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 refresh failure is diagnostic-only before bit 12 is set"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_45_SET_ROLE_ACTION_ID);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x0333U);
    write_u16(missing.state.window, 6U, kStoryVmTypedStop);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.action_id == 0x0333U &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.tile_x == 0xFFFFU && patch.tile_y == 0xFFFFU &&
            patch.talk_script_id == 0xFFFFU && patch.path_data_id == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 missing role uses the exact MAPS action-and-flag patch"
    );
}

void test_change_requested_action_id_failure_ordering(
    openswd3::test::Context& test
) {
    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    selector_truncated.state.previous_opcode = 0x55U;
    const auto selector_truncated_result = selector_truncated.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated_result.executed_instruction_count == 1U &&
            selector_truncated_result.action_update_count == 0U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x55U,
        "opcode 45 stops at the first unsafe selector access"
    );

    Fixture found_value_truncated;
    found_value_truncated.context.instruction_offset = 0x7FFCU;
    found_value_truncated.context.talk_data_offset = 0x1111U;
    found_value_truncated.state.loaded_file_number = 1U;
    found_value_truncated.state.loaded_data_offset = 0x1111U;
    found_value_truncated.state.window_loaded = true;
    found_value_truncated.roles[1].action.action_id = 0xDEADBEEFU;
    write_u16(
        found_value_truncated.state.window, 0x7FFCU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(found_value_truncated.state.window, 0x7FFEU, 0x00F8U);
    found_value_truncated.state.previous_opcode = 0x55U;
    const auto found_value_truncated_result = found_value_truncated.step();
    test.expect_true(
        found_value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_value_truncated.roles[1].action.action_id == 0xDEADBEEFU &&
            found_value_truncated_result.action_update_count == 0U &&
            found_value_truncated.context.instruction_offset == 0x7FFCU &&
            found_value_truncated.state.previous_opcode == 0x55U,
        "opcode 45 finds the role before the unsafe action-id access"
    );

    Fixture missing_value_truncated;
    missing_value_truncated.context.instruction_offset = 0x7FFCU;
    missing_value_truncated.context.talk_data_offset = 0x1111U;
    missing_value_truncated.state.loaded_file_number = 1U;
    missing_value_truncated.state.loaded_data_offset = 0x1111U;
    missing_value_truncated.state.window_loaded = true;
    write_u16(
        missing_value_truncated.state.window, 0x7FFCU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(missing_value_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto missing_value_truncated_result = missing_value_truncated.step();
    test.expect_true(
        missing_value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_value_truncated.ports.role_patch_requests.empty() &&
            missing_value_truncated.context.instruction_offset == 0x7FFCU,
        "opcode 45 missing-role patch waits for the action-id read"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].flags = 0x20U;
    exact_tail.roles[1].action.action_id = 0xDEADBEEFU;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_45_SET_ROLE_ACTION_ID);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x8008U);
    exact_tail.state.previous_opcode = 0x55U;
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.action_update_count == 0U &&
            exact_tail.roles[1].action.action_id == 0x8008U &&
            exact_tail.roles[1].flags == 0x20U &&
            exact_tail.context.instruction_offset == 0x7FFAU &&
            exact_tail.state.previous_opcode == 0x55U,
        "opcode 45 writes action id before the mandatory next-opcode access"
    );

    Fixture next_selector_truncated;
    next_selector_truncated.context.instruction_offset = 0x7FF8U;
    next_selector_truncated.context.talk_data_offset = 0x1111U;
    next_selector_truncated.state.loaded_file_number = 1U;
    next_selector_truncated.state.loaded_data_offset = 0x1111U;
    next_selector_truncated.state.window_loaded = true;
    next_selector_truncated.roles[1].flags = 0x20U;
    write_u16(
        next_selector_truncated.state.window, 0x7FF8U, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(next_selector_truncated.state.window, 0x7FFAU, 0x00F8U);
    write_u16(next_selector_truncated.state.window, 0x7FFCU, 0x8009U);
    write_u16(
        next_selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    next_selector_truncated.state.previous_opcode = 0x55U;
    const auto next_selector_truncated_result = next_selector_truncated.step();
    test.expect_true(
        next_selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_selector_truncated_result.action_update_count == 0U &&
            next_selector_truncated.roles[1].action.action_id == 0x8009U &&
            next_selector_truncated.roles[1].flags == 0x20U &&
            next_selector_truncated.context.instruction_offset == 0x7FF8U &&
            next_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 45 reads a recognized next selector before refresh and bit 12"
    );

    Fixture unrecognized_tail;
    unrecognized_tail.context.instruction_offset = 0x7FF8U;
    unrecognized_tail.context.talk_data_offset = 0x1111U;
    unrecognized_tail.state.loaded_file_number = 1U;
    unrecognized_tail.state.loaded_data_offset = 0x1111U;
    unrecognized_tail.state.window_loaded = true;
    write_u16(
        unrecognized_tail.state.window, 0x7FF8U, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(unrecognized_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(unrecognized_tail.state.window, 0x7FFCU, 0x8010U);
    write_u16(unrecognized_tail.state.window, 0x7FFEU, kStoryVmTypedStop);
    const auto unrecognized_tail_result = unrecognized_tail.step();
    test.expect_true(
        unrecognized_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            unrecognized_tail_result.executed_instruction_count == 2U &&
            unrecognized_tail_result.action_update_count == 1U &&
            unrecognized_tail.roles[1].action.action_id == 0x8010U &&
            (unrecognized_tail.roles[1].flags & 0x1000U) != 0U &&
            unrecognized_tail.context.instruction_offset == 0x7FFEU &&
            unrecognized_tail.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 unrecognized lookahead needs no following selector"
    );

    Fixture missing_exact_tail;
    missing_exact_tail.context.instruction_offset = 0x7FFAU;
    missing_exact_tail.context.talk_data_offset = 0x1111U;
    missing_exact_tail.state.loaded_file_number = 1U;
    missing_exact_tail.state.loaded_data_offset = 0x1111U;
    missing_exact_tail.state.window_loaded = true;
    write_u16(
        missing_exact_tail.state.window, 0x7FFAU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(missing_exact_tail.state.window, 0x7FFCU, 0x7777U);
    write_u16(missing_exact_tail.state.window, 0x7FFEU, 0x0333U);
    const auto missing_exact_tail_result = missing_exact_tail.step();
    test.expect_true(
        missing_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            missing_exact_tail_result.executed_instruction_count == 1U &&
            missing_exact_tail_result.action_update_count == 0U &&
            missing_exact_tail.ports.role_patch_requests.size() == 1U &&
            missing_exact_tail.context.instruction_offset == 0x8000U &&
            missing_exact_tail.state.previous_opcode ==
                OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 missing-role exact tail patches before the next fetch"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_45_SET_ROLE_ACTION_ID);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8011U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 45 invalid controlled owner stops at the VM session boundary"
    );
}

void test_restore_role_action_overrides_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime_role_instruction = [](Fixture& fixture,
                                           const u16 raw_word) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x55U;
    };

    for (const u16 mask : alias_masks) {
        Fixture restore_all;
        auto& action = restore_all.roles[1].action;
        action.action_id = 0xA0A0A0A0U;
        action.cached_action_id = 0x41414141U;
        action.base_variant = 0xB0B0B0B0U;
        action.cached_base_variant = 0x42424242U;
        action.variant_delta = 0xC0C0C0C0U;
        action.cached_variant_delta = 0x43434343U;
        action.mode_flags = 0x44444444U;
        action.field_1c = 0x11111111U;
        action.one_shot_base_variant = 0x22222222U;
        action.one_shot_variant_delta = 0x33333333U;
        action.packed_ap_state = 0x4545U;
        action.command_cursor = 0x4646U;
        action.wait_remaining = 0x4747U;
        action.wait_default = 0x4848U;
        action.wait_override = 0x4949U;
        action.field_4a = 0x4A4AU;
        action.field_8c = 0x4B4B4B4BU;
        action.external_mode = 0x4C4C4C4CU;
        prime_role_instruction(
            restore_all,
            static_cast<u16>(OP_46_RESTORE_ROLE_ACTION_OVERRIDES | mask)
        );
        const auto result = restore_all.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                action.action_id == 0x11111111U &&
                action.cached_action_id == 0x41414141U &&
                action.base_variant == 0x22222222U &&
                action.cached_base_variant == 0x42424242U &&
                action.variant_delta == 0x33333333U &&
                action.cached_variant_delta == 0x43434343U &&
                action.mode_flags == 0x44444444U &&
                action.field_1c == 0xFFFFFFFFU &&
                action.one_shot_base_variant == 0xFFFFFFFFU &&
                action.one_shot_variant_delta == 0xFFFFFFFFU &&
                action.packed_ap_state == 0x4545U &&
                action.command_cursor == 0U && action.wait_remaining == 0U &&
                action.wait_default == 0U && action.wait_override == 0U &&
                action.field_4a == 0x1111U && action.field_8c == 0x4B4B4B4BU &&
                action.external_mode == 0U &&
                restore_all.context.instruction_offset == 4U &&
                restore_all.state.previous_opcode ==
                    OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
                restore_all.ports.direct_audio_service_count == 0U,
            "opcode 46 aliases restore all targets then reset exact action fields"
        );

        Fixture base_override;
        auto& base_action = base_override.roles[1].action;
        base_action.base_variant = 0x11111111U;
        base_action.one_shot_base_variant = 0x89ABCDEFU;
        base_action.one_shot_variant_delta = 0x76543210U;
        base_action.wait_remaining = 0x1111U;
        base_action.wait_default = 0x2222U;
        base_action.wait_override = 0x3333U;
        prime_role_instruction(
            base_override,
            static_cast<u16>(OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE | mask)
        );
        const auto base_result = base_override.step();
        test.expect_true(
            base_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                base_result.executed_instruction_count == 2U &&
                base_result.action_update_count == 1U &&
                base_action.base_variant == 0x89ABCDEFU &&
                base_action.one_shot_base_variant == 0xFFFFFFFFU &&
                base_action.one_shot_variant_delta == 0x76543210U &&
                base_action.wait_remaining == 0x1111U &&
                base_action.wait_default == 0x2222U &&
                base_action.wait_override == 0x3333U &&
                base_override.context.instruction_offset == 4U &&
                base_override.state.previous_opcode ==
                    OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
                base_override.ports.direct_audio_service_count == 0U,
            "opcode 47 aliases apply the full pending base variant only"
        );

        Fixture delta_override;
        auto& delta_action = delta_override.roles[1].action;
        delta_action.variant_delta = 0x11111111U;
        delta_action.one_shot_base_variant = 0x76543210U;
        delta_action.one_shot_variant_delta = 0x89ABCDEFU;
        delta_action.wait_remaining = 0x1111U;
        delta_action.wait_default = 0x2222U;
        delta_action.wait_override = 0x3333U;
        prime_role_instruction(
            delta_override,
            static_cast<u16>(OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE | mask)
        );
        const auto delta_result = delta_override.step();
        test.expect_true(
            delta_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                delta_result.executed_instruction_count == 2U &&
                delta_result.action_update_count == 1U &&
                delta_action.variant_delta == 0x89ABCDEFU &&
                delta_action.one_shot_variant_delta == 0xFFFFFFFFU &&
                delta_action.one_shot_base_variant == 0x76543210U &&
                delta_action.wait_remaining == 0x1111U &&
                delta_action.wait_default == 0x2222U &&
                delta_action.wait_override == 0x3333U &&
                delta_override.context.instruction_offset == 4U &&
                delta_override.state.previous_opcode ==
                    OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
                delta_override.ports.direct_audio_service_count == 0U,
            "opcode 48 aliases apply the full pending variant delta only"
        );

        Fixture wait_override;
        auto& wait_action = wait_override.roles[1].action;
        wait_action.one_shot_base_variant = 0x11111111U;
        wait_action.one_shot_variant_delta = 0x22222222U;
        wait_action.wait_remaining = 0x3333U;
        wait_action.wait_default = 0x4444U;
        wait_action.wait_override = 0x5555U;
        prime_role_instruction(
            wait_override,
            static_cast<u16>(OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF | mask)
        );
        const auto wait_result = wait_override.step();
        test.expect_true(
            wait_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                wait_result.executed_instruction_count == 2U &&
                wait_result.action_update_count == 1U &&
                wait_action.one_shot_base_variant == 0x11111111U &&
                wait_action.one_shot_variant_delta == 0x22222222U &&
                wait_action.wait_remaining == 0x3333U &&
                wait_action.wait_default == 0x4444U &&
                wait_action.wait_override == 0xFFFFU &&
                wait_override.context.instruction_offset == 4U &&
                wait_override.state.previous_opcode ==
                    OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
                wait_override.ports.direct_audio_service_count == 0U,
            "opcode 49 aliases write only the wait-override word"
        );
    }

    constexpr std::array<u16, 3U> conditional_opcodes{
        OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE,
        OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE,
        OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF,
    };
    for (const u16 opcode : conditional_opcodes) {
        Fixture no_pending_value;
        auto& action = no_pending_value.roles[1].action;
        action.base_variant = 0x11111111U;
        action.variant_delta = 0x22222222U;
        action.one_shot_base_variant = 0xFFFFFFFFU;
        action.one_shot_variant_delta = 0xFFFFFFFFU;
        action.wait_remaining = 0x3333U;
        action.wait_default = 0x4444U;
        action.wait_override = 0xFFFFU;
        prime_role_instruction(no_pending_value, opcode);
        const auto result = no_pending_value.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.action_update_count == 1U &&
                action.base_variant == 0x11111111U &&
                action.variant_delta == 0x22222222U &&
                action.one_shot_base_variant == 0xFFFFFFFFU &&
                action.one_shot_variant_delta == 0xFFFFFFFFU &&
                action.wait_remaining == 0x3333U &&
                action.wait_default == 0x4444U &&
                action.wait_override == 0xFFFFU &&
                no_pending_value.context.instruction_offset == 4U &&
                no_pending_value.state.previous_opcode == opcode,
            "opcodes 47-49 refresh once even when their conditional write skips"
        );
    }

    Fixture restore_absent_values;
    auto& absent_action = restore_absent_values.roles[1].action;
    absent_action.action_id = 0x11111111U;
    absent_action.base_variant = 0x22222222U;
    absent_action.variant_delta = 0x33333333U;
    absent_action.field_1c = 0xFFFFFFFFU;
    absent_action.one_shot_base_variant = 0xFFFFFFFFU;
    absent_action.one_shot_variant_delta = 0xFFFFFFFFU;
    prime_role_instruction(
        restore_absent_values, OP_46_RESTORE_ROLE_ACTION_OVERRIDES
    );
    const auto restore_absent_result = restore_absent_values.step();
    test.expect_true(
        restore_absent_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            restore_absent_result.action_update_count == 1U &&
            absent_action.action_id == 0xFFFFFFFFU &&
            absent_action.base_variant == 0xFFFFFFFFU &&
            absent_action.variant_delta == 0xFFFFFFFFU,
        "opcode 46 unconditionally copies absent pending values"
    );

    Fixture literal_fff0;
    literal_fff0.roles[1].action.base_variant = 0x11111111U;
    literal_fff0.roles[1].action.one_shot_base_variant = 0x22222222U;
    literal_fff0.roles[2].guid = 0xFFF0U;
    literal_fff0.roles[2].action.base_variant = 0x33333333U;
    literal_fff0.roles[2].action.one_shot_base_variant = 0x44444444U;
    prime_loaded_instruction(
        literal_fff0, OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE
    );
    write_u16(literal_fff0.state.window, 2U, 0xFFF0U);
    write_u16(literal_fff0.state.window, 4U, kStoryVmTypedStop);
    const auto literal_fff0_result = literal_fff0.step();
    test.expect_true(
        literal_fff0_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            literal_fff0.roles[1].action.base_variant == 0x11111111U &&
            literal_fff0.roles[1].action.one_shot_base_variant == 0x22222222U &&
            literal_fff0.roles[2].action.base_variant == 0x44444444U &&
            literal_fff0.roles[2].action.one_shot_base_variant == 0xFFFFFFFFU,
        "opcodes 46-49 treat FFF0 as a literal GUID"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.one_shot_variant_delta = 0x11111111U;
    controlled.roles[2].action.one_shot_variant_delta = 0x22222222U;
    prime_loaded_instruction(
        controlled, OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE
    );
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, kStoryVmTypedStop);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.roles[1].action.variant_delta == 0U &&
            controlled.roles[1].action.one_shot_variant_delta == 0x11111111U &&
            controlled.roles[2].action.variant_delta == 0x22222222U &&
            controlled.roles[2].action.one_shot_variant_delta == 0xFFFFFFFFU,
        "opcodes 46-49 pass FFFE through for controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[0].action.wait_override = 0x1111U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].action.wait_override = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].action.wait_override = 0x3333U;
    prime_loaded_instruction(
        first_clear_match, OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF
    );
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, kStoryVmTypedStop);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_clear_match.roles[0].action.wait_override == 0x1111U &&
            first_clear_match.roles[1].action.wait_override == 0xFFFFU &&
            first_clear_match.roles[2].action.wait_override == 0x3333U,
        "opcodes 46-49 skip bit-28 roles and use the first clear GUID match"
    );

    constexpr std::array<u16, 4U> opcodes{
        OP_46_RESTORE_ROLE_ACTION_OVERRIDES,
        OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE,
        OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE,
        OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF,
    };
    for (const u16 opcode : opcodes) {
        Fixture missing;
        prime_loaded_instruction(missing, opcode);
        write_u16(missing.state.window, 2U, 0x7777U);
        missing.state.previous_opcode = 0x55U;
        const auto result = missing.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::role_not_found &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.action_update_count == 0U &&
                missing.context.instruction_offset == 0U &&
                missing.state.previous_opcode == 0x55U &&
                missing.ports.role_patch_requests.empty(),
            "opcodes 46-49 stop at the first unsafe missing-role action access"
        );

        Fixture selector_truncated;
        selector_truncated.context.instruction_offset = 0x7FFEU;
        selector_truncated.context.talk_data_offset = 0x1111U;
        selector_truncated.state.loaded_file_number = 1U;
        selector_truncated.state.loaded_data_offset = 0x1111U;
        selector_truncated.state.window_loaded = true;
        write_u16(selector_truncated.state.window, 0x7FFEU, opcode);
        selector_truncated.state.previous_opcode = 0x55U;
        const auto truncated_result = selector_truncated.step();
        test.expect_true(
            truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                truncated_result.opcode == opcode &&
                truncated_result.executed_instruction_count == 1U &&
                truncated_result.action_update_count == 0U &&
                selector_truncated.context.instruction_offset == 0x7FFEU &&
                selector_truncated.state.previous_opcode == 0x55U,
            "opcodes 46-49 stop at the unsafe selector-word access"
        );

        Fixture exact_tail;
        auto& tail_action = exact_tail.roles[1].action;
        tail_action.action_id = 0x10U;
        tail_action.field_1c = 0x101U;
        tail_action.base_variant = 0x20U;
        tail_action.one_shot_base_variant = 0x202U;
        tail_action.variant_delta = 0x30U;
        tail_action.one_shot_variant_delta = 0x303U;
        tail_action.wait_override = 0x404U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        write_u16(exact_tail.state.window, 0x7FFCU, opcode);
        write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto tail_result = exact_tail.step();
        const bool tail_effect =
            (opcode == OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
             tail_action.action_id == 0x101U &&
             tail_action.base_variant == 0x202U &&
             tail_action.variant_delta == 0x303U &&
             tail_action.wait_override == 0U) ||
            (opcode == OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
             tail_action.base_variant == 0x202U &&
             tail_action.one_shot_base_variant == 0xFFFFFFFFU) ||
            (opcode == OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
             tail_action.variant_delta == 0x303U &&
             tail_action.one_shot_variant_delta == 0xFFFFFFFFU) ||
            (opcode == OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
             tail_action.wait_override == 0xFFFFU);
        test.expect_true(
            tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                tail_result.opcode == opcode &&
                tail_result.executed_instruction_count == 1U &&
                tail_result.action_update_count == 1U && tail_effect &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == opcode &&
                exact_tail.ports.direct_audio_service_count == 0U,
            "opcodes 46-49 exact-tail records complete before next fetch"
        );

        Fixture update_failure;
        auto& failed_action = update_failure.roles[1].action;
        failed_action.action_id = 0x10U;
        failed_action.field_1c = 0x101U;
        failed_action.base_variant = 0x20U;
        failed_action.one_shot_base_variant = 0x202U;
        failed_action.variant_delta = 0x30U;
        failed_action.one_shot_variant_delta = 0x303U;
        failed_action.wait_override = 0x404U;
        update_failure.ports.action_update_result = 0U;
        prime_role_instruction(update_failure, opcode);
        const auto failed_result = update_failure.step();
        const bool failed_effect =
            (opcode == OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
             failed_action.action_id == 0x101U &&
             failed_action.base_variant == 0x202U &&
             failed_action.variant_delta == 0x303U) ||
            (opcode == OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
             failed_action.base_variant == 0x202U) ||
            (opcode == OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
             failed_action.variant_delta == 0x303U) ||
            (opcode == OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
             failed_action.wait_override == 0xFFFFU);
        test.expect_true(
            failed_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                failed_result.action_update_count == 1U &&
                failed_result.action_update_failure_count == 1U &&
                failed_effect &&
                update_failure.context.instruction_offset == 4U &&
                update_failure.state.previous_opcode == opcode,
            "opcodes 46-49 refresh failure is diagnostic-only after effects"
        );

        Fixture invalid_controlled;
        prime_loaded_instruction(invalid_controlled, opcode);
        write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
        invalid_controlled.state.previous_opcode = 0x55U;
        const auto invalid_result = invalid_controlled.step(
            0, 0, static_cast<u32>(invalid_controlled.roles.size())
        );
        test.expect_true(
            invalid_result.status == LegacyWorldStoryVmStatus::role_not_found &&
                invalid_result.opcode == 0U &&
                invalid_result.executed_instruction_count == 0U &&
                invalid_result.action_update_count == 0U &&
                invalid_controlled.context.instruction_offset == 0U &&
                invalid_controlled.state.previous_opcode == 0x55U,
            "opcodes 46-49 invalid controlled owner stops before opcode fetch"
        );
    }
}
