#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_reload_world_session_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_27_RELOAD_WORLD_SESSION,
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0x4000U),
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0x8000U),
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        fixture.runtime.role_surface.selected_guid = 0x00F8U;
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0045U);
        write_u16(fixture.state.window, 6U, 0x0067U);
        write_u16(fixture.state.window, 8U, 0x0089U);
        write_u16(fixture.state.window, 10U, 0x00ABU);
        write_u16(fixture.state.window, 12U, 0x00CDU);
        write_u16(fixture.state.window, 14U, OP_1025);
        const auto result = fixture.step();
        const auto& request = fixture.ports.last_world_load_request;
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 14U &&
                fixture.state.previous_opcode == OP_27_RELOAD_WORLD_SESSION &&
                fixture.ports.world_session_reload_begin_count == 1U &&
                fixture.ports.world_session_reload_count == 1U &&
                request.logical_map_id == 0x0123U &&
                request.tile_x == 0x0045U && request.tile_y == 0x0067U &&
                request.action_id == 0x0089U &&
                request.base_variant == 0x00ABU &&
                request.variant_delta == 0x00CDU &&
                request.selected_guid == 0x00F8U && request.load_flags == 1U,
            "opcode 27 aliases synchronously submit all six load operands"
        );
    }

    Fixture inherited;
    std::array<LegacyWorldRoleRecord, 2U> replacement_roles{};
    replacement_roles[1].guid = 0x0BEEU;
    prime_loaded_instruction(inherited, OP_27_RELOAD_WORLD_SESSION);
    inherited.runtime.role_surface.selected_guid = 0x00F8U;
    inherited.roles[1].action.action_id = 0x12345U;
    inherited.roles[1].action.base_variant = 0x23456U;
    inherited.roles[1].action.variant_delta = 0x34567U;
    inherited.ports.replacement_roles = replacement_roles;
    inherited.ports.replacement_controlled_role_index = 1U;
    inherited.ports.replacement_selected_guid = 0x0BEEU;
    write_u16(inherited.state.window, 2U, 120U);
    write_u16(inherited.state.window, 4U, 31U);
    write_u16(inherited.state.window, 6U, 29U);
    write_u16(inherited.state.window, 8U, 0xFFFFU);
    write_u16(inherited.state.window, 10U, 0xFFFFU);
    write_u16(inherited.state.window, 12U, 0xFFFFU);
    write_u16(inherited.state.window, 14U, OP_10_SET_ROLE_BASE_VARIANT);
    write_u16(inherited.state.window, 16U, 0xFFFEU);
    write_u16(inherited.state.window, 18U, 0x2222U);
    write_u16(inherited.state.window, 20U, OP_1025);
    const auto inherited_result = inherited.step(0, 0, 1U);
    const auto& inherited_request = inherited.ports.last_world_load_request;
    test.expect_true(
        inherited_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            inherited_result.opcode == OP_1025 &&
            inherited_result.executed_instruction_count == 3U &&
            inherited.context.instruction_offset == 20U &&
            inherited.state.previous_opcode == OP_10_SET_ROLE_BASE_VARIANT &&
            inherited_request.action_id == 0x2345U &&
            inherited_request.base_variant == 0x3456U &&
            inherited_request.variant_delta == 0x4567U &&
            inherited_request.selected_guid == 0x00F8U &&
            replacement_roles[1].action.base_variant == 0x2222U &&
            inherited.roles[1].action.base_variant == 0x23456U &&
            inherited.ports.story_protocol_events ==
                std::vector<u32>{6U, 7U, 4U},
        "opcode 27 resolves FFFF through low16 action fields then continues " "against the synchronously rebound world"
    );

    Fixture load_failed;
    prime_loaded_instruction(load_failed, OP_27_RELOAD_WORLD_SESSION);
    load_failed.state.previous_opcode = 0x55U;
    load_failed.ports.world_session_reload_success = false;
    write_u16(load_failed.state.window, 2U, 1U);
    write_u16(load_failed.state.window, 4U, 2U);
    write_u16(load_failed.state.window, 6U, 3U);
    write_u16(load_failed.state.window, 8U, 4U);
    write_u16(load_failed.state.window, 10U, 5U);
    write_u16(load_failed.state.window, 12U, 6U);
    const auto load_failed_result = load_failed.step();
    test.expect_true(
        load_failed_result.status ==
                LegacyWorldStoryVmStatus::world_session_load_failed &&
            load_failed.context.instruction_offset == 0U &&
            load_failed.state.previous_opcode == 0x55U &&
            load_failed.ports.world_session_reload_begin_count == 1U &&
            load_failed.ports.world_session_reload_count == 1U,
        "opcode 27 publishes no IP or previous opcode after reload failure"
    );

    Fixture missing_role;
    prime_loaded_instruction(missing_role, OP_27_RELOAD_WORLD_SESSION);
    missing_role.state.previous_opcode = 0x55U;
    write_u16(missing_role.state.window, 2U, 1U);
    write_u16(missing_role.state.window, 4U, 2U);
    write_u16(missing_role.state.window, 6U, 3U);
    write_u16(missing_role.state.window, 8U, 0xFFFFU);
    write_u16(missing_role.state.window, 10U, 5U);
    write_u16(missing_role.state.window, 12U, 6U);
    const auto missing_role_result = missing_role.step(0, 0, 99U);
    test.expect_true(
        missing_role_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.ports.world_session_reload_begin_count == 0U &&
            missing_role.ports.world_session_reload_count == 0U &&
            missing_role.ports.story_protocol_events.empty(),
        "the existing VM entry guard rejects an invalid controlled role " "before opcode 27 dispatch"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FF4U;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FF4U, OP_27_RELOAD_WORLD_SESSION);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FF4U &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.world_session_reload_begin_count == 0U &&
            truncated.ports.world_session_reload_count == 0U,
        "opcode 27 reads the complete fourteen-byte record before transition " "effects"
    );
}

void test_change_role_path_id_protocol(openswd3::test::Context& test) {
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 selector,
                          const u16 path_id) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, path_id);
    };

    constexpr std::array<u16, 4U> raw_aliases{
        OP_28_CHANGE_ROLE_PATH_ID,
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0x4000U),
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0x8000U),
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].path_word_index = 7U;
        fixture.roles[1].path_data_id = 0x1111U;
        prime(fixture, raw_word, 0x00F8U, 0x2468U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
                fixture.roles[1].path_data_id == 0x2468U &&
                fixture.roles[1].path_word_index == 0U &&
                (fixture.roles[1].flags & 0x1000U) != 0U &&
                fixture.ports.direct_audio_service_count == 1U,
            "opcode 28 aliases update the live role then yield after audio service"
        );
    }

    Fixture missing;
    prime(missing, OP_28_CHANGE_ROLE_PATH_ID, 0x4321U, 0x1357U);
    const auto missing_result = missing.step();
    const auto& patch = missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x4321U && patch.action_id == 0xFFFFU &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.tile_x == 0xFFFFU && patch.tile_y == 0xFFFFU &&
            patch.talk_script_id == 0xFFFFU && patch.path_data_id == 0x1357U &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            missing.ports.direct_audio_service_count == 1U,
        "opcode 28 preserves all other MAPS fields on a missing live role"
    );

    Fixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime(invalid_controlled, OP_28_CHANGE_ROLE_PATH_ID, 0xFFFEU, 0x1357U);
    const auto invalid_controlled_result = invalid_controlled.step(0, 0, 99U);
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.role_patch_requests.empty() &&
            invalid_controlled.ports.direct_audio_service_count == 0U,
        "opcode 28 isolates an invalid controlled-role selector before MAPS patching"
    );

    Fixture type_two;
    type_two.roles[1].path_payload_relation = 0x11111111U;
    type_two.roles[1].path_payload_pointer_32 = 0x22222222U;
    auto& type_two_slot = type_two.active_object_slots[0];
    type_two_slot.bytes.fill(0x5AU);
    write_u16(type_two_slot.bytes, 0U, 1U);
    type_two_slot.bytes[0x1BU] = 2U;
    prime(type_two, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x3456U);
    const auto type_two_result = type_two.step();
    test.expect_true(
        type_two_result.status == LegacyWorldStoryVmStatus::yielded &&
            type_two.ports.role_path_payload_release_count == 1U &&
            type_two.ports.released_role_path_index == 1U &&
            type_two.roles[1].path_payload_relation == 0U &&
            type_two.roles[1].path_payload_pointer_32 == 0U &&
            std::ranges::all_of(
                type_two_slot.bytes.begin() + 8,
                type_two_slot.bytes.begin() + 16,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            type_two_slot.bytes[0x10U] == 0x5AU &&
            type_two_result.active_object_reset_count == 0U,
        "opcode 28 releases role payload then clears only four type-2 link words"
    );

    Fixture aligned;
    auto& aligned_slot = aligned.active_object_slots[0];
    aligned_slot.bytes.fill(0x33U);
    write_u16(aligned_slot.bytes, 0U, 1U);
    aligned_slot.bytes[0x1BU] = 1U;
    prime(aligned, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x4567U);
    const auto aligned_result = aligned.step();
    test.expect_true(
        aligned_result.status == LegacyWorldStoryVmStatus::yielded &&
            aligned_result.active_object_reset_count == 1U &&
            std::ranges::all_of(
                aligned_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ),
        "opcode 28 resets an aligned matching type-1 object without runtime owners"
    );

    const auto test_alignment = [&](const u32 start_x,
                                    const u32 start_y,
                                    const u8 direction,
                                    const u32 flags,
                                    const bool provide_surface) {
        Fixture fixture;
        fixture.roles[1].world_x = start_x;
        fixture.roles[1].world_y = start_y;
        fixture.roles[1].flags = flags;
        StoryPathHarness harness(fixture);
        fixture.runtime.spatial_index = &harness.spatial;
        fixture.runtime.role_surface = provide_surface
            ? harness.runtime.role_surface
            : openswd3::world_map::LegacyWorldRoleSurfaceContext{};
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0U, 1U);
        write_u16(slot.bytes, 2U, 0U);
        slot.bytes[0x1BU] = 1U;
        slot.bytes[0x1CU] = direction;
        prime(fixture, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x5678U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                fixture.roles[1].world_x == 320U &&
                fixture.roles[1].world_y == 240U &&
                result.active_object_reset_count == 1U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 28 aligns type-1 movement backward before spatial reinsertion"
        );
    };
    test_alignment(324U, 244U, 0U, 0U, true);
    test_alignment(316U, 236U, 4U, 0U, true);
    test_alignment(324U, 244U, 0U, 0x4000U, false);

    Fixture invalid_direction;
    invalid_direction.state.previous_opcode = 0x55U;
    invalid_direction.roles[1].world_x = 324U;
    invalid_direction.roles[1].flags = 0x4000U;
    auto& invalid_slot = invalid_direction.active_object_slots[0];
    write_u16(invalid_slot.bytes, 0U, 1U);
    write_u16(invalid_slot.bytes, 2U, 0U);
    invalid_slot.bytes[0x1BU] = 1U;
    invalid_slot.bytes[0x1CU] = 8U;
    prime(invalid_direction, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x6789U);
    const auto invalid_direction_result = invalid_direction.step();
    test.expect_true(
        invalid_direction_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            invalid_direction.context.instruction_offset == 0U &&
            invalid_direction.state.previous_opcode == 0x55U &&
            invalid_direction.roles[1].path_data_id == 0U &&
            invalid_direction.ports.direct_audio_service_count == 0U &&
            invalid_slot.bytes[0x1BU] == 1U,
        "opcode 28 isolates an invalid type-1 direction without publishing the instruction"
    );

    Fixture live_truncated;
    live_truncated.context.instruction_offset = 0x7FFCU;
    live_truncated.context.talk_data_offset = 0x1111U;
    live_truncated.state.loaded_file_number = 1U;
    live_truncated.state.loaded_data_offset = 0x1111U;
    live_truncated.state.window_loaded = true;
    live_truncated.state.previous_opcode = 0x55U;
    live_truncated.roles[1].path_payload_relation = 0x11111111U;
    live_truncated.roles[1].path_payload_pointer_32 = 0x22222222U;
    write_u16(live_truncated.state.window, 0x7FFCU, OP_28_CHANGE_ROLE_PATH_ID);
    write_u16(live_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto live_truncated_result = live_truncated.step();
    test.expect_true(
        live_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            live_truncated.context.instruction_offset == 0x7FFCU &&
            live_truncated.state.previous_opcode == 0x55U &&
            live_truncated.ports.role_path_payload_release_count == 1U &&
            live_truncated.roles[1].path_payload_relation == 0U &&
            live_truncated.roles[1].path_payload_pointer_32 == 0U &&
            live_truncated.roles[1].path_word_index == 0U &&
            live_truncated.roles[1].path_data_id == 0U,
        "opcode 28 reads path id only after live-role payload and object effects"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    missing_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_truncated.state.window, 0x7FFCU, OP_28_CHANGE_ROLE_PATH_ID
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x4321U);
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U &&
            missing_truncated.ports.role_patch_requests.empty() &&
            missing_truncated.ports.direct_audio_service_count == 0U,
        "opcode 28 missing-role fallback reads path id before MAPS patching"
    );
}

void test_global_integer_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 index,
                          const u16 value) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, index);
        write_u16(fixture.state.window, 4U, value);
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000001U;
        prime(
            fixture,
            static_cast<u16>(OP_29_SET_GLOBAL_INTEGER | mask),
            2U,
            0xFFFFU
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_29_SET_GLOBAL_INTEGER &&
                fixture.state.script_variables[0] == 0U &&
                fixture.state.script_variables[2] == 0xFFFFFFFFU,
            "opcode 29 aliases sign-extend the value and clamp variable zero"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[2] = 0xFFFFFFF0U;
        prime(
            fixture, static_cast<u16>(OP_30_ADD_GLOBAL_INTEGER | mask), 2U, 32U
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_30_ADD_GLOBAL_INTEGER &&
                fixture.state.script_variables[2] == 0x10U,
            "opcode 30 aliases add a sign-extended value with u32 wrapping"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[2] = 5U;
        prime(
            fixture,
            static_cast<u16>(OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO | mask),
            2U,
            10U
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO &&
                fixture.state.script_variables[2] == 0U,
            "opcode 31 aliases clamp a sign-bit subtraction result to zero"
        );
    }

    Fixture add_above_path_limit;
    add_above_path_limit.state.script_variables[2] = 990U;
    prime(add_above_path_limit, OP_30_ADD_GLOBAL_INTEGER, 2U, 50U);
    write_u16(add_above_path_limit.state.window, 6U, OP_1025);
    const auto add_above_path_limit_result = add_above_path_limit.step();
    test.expect_true(
        add_above_path_limit_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            add_above_path_limit.state.script_variables[2] == 1040U,
        "story opcode 30 does not inherit the PATH VM 1000-value cap"
    );

    Fixture subtract_negative;
    subtract_negative.state.script_variables[2] = 3U;
    prime(
        subtract_negative, OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO, 2U, 0xFFFBU
    );
    write_u16(subtract_negative.state.window, 6U, OP_1025);
    const auto subtract_negative_result = subtract_negative.step();
    test.expect_true(
        subtract_negative_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            subtract_negative.state.script_variables[2] == 8U,
        "opcode 31 subtracts a negative s16 as its wrapped u32 bit pattern"
    );

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000000U;
        fixture.state.script_variables[2] = 0U;
        prime(
            fixture,
            static_cast<u16>(OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE | mask),
            2U,
            0xFFFFU
        );
        write_u32(fixture.state.window, 6U, 0x12345678U);
        write_u16(fixture.state.window, 10U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
                fixture.state.script_variables[0] == 0U &&
                fixture.ports.data_load_count == 0U,
            "opcode 32 aliases compare against sign-extended threshold bits as unsigned"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000000U;
        fixture.state.script_variables[2] = 0xFFFFFFFFU;
        prime(
            fixture,
            static_cast<u16>(OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE | mask),
            2U,
            0U
        );
        write_u32(fixture.state.window, 6U, 0x12345678U);
        write_u16(fixture.state.window, 10U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
                fixture.state.script_variables[0] == 0U &&
                fixture.ports.data_load_count == 0U,
            "opcode 33 aliases use an unsigned less-or-equal comparison"
        );
    }

    Fixture ge_taken;
    ge_taken.state.script_variables[0] = 0x80000000U;
    ge_taken.state.script_variables[2] = 0xFFFFFFFFU;
    prime(ge_taken, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 2U, 0xFFFFU);
    write_u32(ge_taken.state.window, 6U, 0x2222U);
    write_u16(ge_taken.ports.transferred_window, 0U, OP_1025);
    const auto ge_taken_result = ge_taken.step();
    test.expect_true(
        ge_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            ge_taken_result.executed_instruction_count == 2U &&
            ge_taken.context.talk_data_offset == 0x2222U &&
            ge_taken.context.instruction_offset == 0U &&
            ge_taken.state.previous_opcode ==
                OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            ge_taken.state.script_variables[0] == 0U &&
            ge_taken.ports.data_load_count == 1U &&
            ge_taken.ports.last_data_offset == 0x2222U &&
            ge_taken.ports.direct_audio_service_count == 1U,
        "opcode 32 taken path reloads then clamps variable zero before continuation"
    );

    Fixture le_taken;
    le_taken.state.script_variables[2] = 0U;
    prime(le_taken, OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE, 2U, 0xFFFFU);
    write_u32(le_taken.state.window, 6U, 0x3333U);
    write_u16(le_taken.ports.transferred_window, 0U, OP_1025);
    const auto le_taken_result = le_taken.step();
    test.expect_true(
        le_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            le_taken_result.executed_instruction_count == 2U &&
            le_taken.context.talk_data_offset == 0x3333U &&
            le_taken.state.previous_opcode ==
                OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
            le_taken.ports.data_load_count == 1U &&
            le_taken.ports.direct_audio_service_count == 1U,
        "opcode 33 taken path accepts zero below a sign-extended negative threshold"
    );

    constexpr std::array<u16, 5U> opcodes{
        OP_29_SET_GLOBAL_INTEGER,
        OP_30_ADD_GLOBAL_INTEGER,
        OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO,
        OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE,
        OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE,
    };
    for (const u16 opcode : opcodes) {
        Fixture fixture;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x55U;
        fixture.state.script_variables[0] = 0x80000000U;
        write_u16(fixture.state.window, 0x7FFAU, opcode);
        write_u16(fixture.state.window, 0x7FFCU, 64U);
        write_u16(fixture.state.window, 0x7FFEU, 1U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0x7FFAU &&
                fixture.state.previous_opcode == opcode &&
                fixture.state.script_variables[0] == 0x80000000U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 1U,
            "opcodes 29-33 high index yields without target read or shared clamp"
        );
    }

    Fixture negative_update;
    negative_update.state.previous_opcode = 0x55U;
    prime(negative_update, OP_29_SET_GLOBAL_INTEGER, 0xFFFFU, 1U);
    const auto negative_update_result = negative_update.step();
    test.expect_true(
        negative_update_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_update.context.instruction_offset == 0U &&
            negative_update.state.previous_opcode == 0x55U &&
            negative_update.ports.direct_audio_service_count == 0U,
        "negative story-variable index stops at the original write unsafe point"
    );

    Fixture negative_branch;
    negative_branch.state.previous_opcode = 0x55U;
    prime(
        negative_branch, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 0xFFFFU, 0U
    );
    write_u32(negative_branch.state.window, 6U, 0x4444U);
    const auto negative_branch_result = negative_branch.step();
    test.expect_true(
        negative_branch_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_branch.context.instruction_offset == 0U &&
            negative_branch.state.previous_opcode == 0x55U &&
            negative_branch.ports.data_load_count == 0U,
        "negative conditional index reads target before the original read unsafe point"
    );

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFCU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    write_u16(value_truncated.state.window, 0x7FFCU, OP_29_SET_GLOBAL_INTEGER);
    write_u16(value_truncated.state.window, 0x7FFEU, 2U);
    const auto value_truncated_result = value_truncated.step();
    test.expect_true(
        value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFCU,
        "shared numeric entry requires index and value before dispatch"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFAU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    write_u16(
        target_truncated.state.window,
        0x7FFAU,
        OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE
    );
    write_u16(target_truncated.state.window, 0x7FFCU, 0xFFFFU);
    write_u16(target_truncated.state.window, 0x7FFEU, 0U);
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFAU,
        "negative conditional index still performs the earlier target read"
    );

    Fixture load_failure;
    load_failure.state.script_variables[0] = 0x80000000U;
    load_failure.state.script_variables[2] = 1U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime(load_failure, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 2U, 1U);
    write_u32(load_failure.state.window, 6U, 0x5555U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x5555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            load_failure.state.script_variables[0] == 0U &&
            load_failure.ports.direct_audio_service_count == 1U,
        "taken numeric branch preserves loader then shared-tail failure order"
    );
}

void test_wide_global_integer_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 index,
                          const u32 value) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, index);
        write_u32(fixture.state.window, 4U, value);
    };

    for (const u16 mask : alias_masks) {
        Fixture set;
        set.state.script_variables[0] = 0x80000001U;
        prime(
            set,
            static_cast<u16>(OP_181_SET_GLOBAL_INTEGER_WIDE | mask),
            2U,
            0x89ABCDEFU
        );
        write_u16(set.state.window, 8U, OP_1025);
        const auto result = set.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                set.context.instruction_offset == 8U &&
                set.state.previous_opcode == OP_181_SET_GLOBAL_INTEGER_WIDE &&
                set.state.script_variables[0] == 0U &&
                set.state.script_variables[2] == 0x89ABCDEFU,
            "opcode 181 aliases assign all 32 value bits and apply the shared variable-zero clamp"
        );

        Fixture add;
        add.state.script_variables[2] = 0xFFFFFFF0U;
        prime(
            add,
            static_cast<u16>(OP_182_ADD_GLOBAL_INTEGER_WIDE | mask),
            2U,
            32U
        );
        write_u16(add.state.window, 8U, OP_1025);
        const auto add_result = add.step();
        test.expect_true(
            add_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                add_result.executed_instruction_count == 2U &&
                add.context.instruction_offset == 8U &&
                add.state.previous_opcode == OP_182_ADD_GLOBAL_INTEGER_WIDE &&
                add.state.script_variables[2] == 0x10U,
            "opcode 182 aliases perform full-width u32 wrapping addition"
        );

        Fixture subtract;
        subtract.state.script_variables[2] = 5U;
        prime(
            subtract,
            static_cast<u16>(
                OP_183_SUBTRACT_GLOBAL_INTEGER_WIDE_CLAMP_ZERO | mask
            ),
            2U,
            10U
        );
        write_u16(subtract.state.window, 8U, OP_1025);
        const auto subtract_result = subtract.step();
        test.expect_true(
            subtract_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                subtract_result.executed_instruction_count == 2U &&
                subtract.context.instruction_offset == 8U &&
                subtract.state.previous_opcode ==
                    OP_183_SUBTRACT_GLOBAL_INTEGER_WIDE_CLAMP_ZERO &&
                subtract.state.script_variables[2] == 0U,
            "opcode 183 aliases clamp a full-width subtraction result whose sign bit is set"
        );

        Fixture ge_not_taken;
        ge_not_taken.state.script_variables[2] = 0x7FFFFFFFU;
        prime(
            ge_not_taken,
            static_cast<u16>(
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE | mask
            ),
            2U,
            0x80000000U
        );
        write_u32(ge_not_taken.state.window, 8U, 0x12345678U);
        write_u16(ge_not_taken.state.window, 12U, OP_1025);
        const auto ge_result = ge_not_taken.step();
        test.expect_true(
            ge_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                ge_result.executed_instruction_count == 2U &&
                ge_not_taken.context.instruction_offset == 12U &&
                ge_not_taken.state.previous_opcode ==
                    OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
                ge_not_taken.ports.data_load_count == 0U,
            "opcode 184 aliases use an unsigned full-width greater-or-equal comparison"
        );

        Fixture le_not_taken;
        le_not_taken.state.script_variables[2] = 0xFFFFFFFFU;
        prime(
            le_not_taken,
            static_cast<u16>(
                OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE | mask
            ),
            2U,
            0U
        );
        write_u32(le_not_taken.state.window, 8U, 0x12345678U);
        write_u16(le_not_taken.state.window, 12U, OP_1025);
        const auto le_result = le_not_taken.step();
        test.expect_true(
            le_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                le_result.executed_instruction_count == 2U &&
                le_not_taken.context.instruction_offset == 12U &&
                le_not_taken.state.previous_opcode ==
                    OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE &&
                le_not_taken.ports.data_load_count == 0U,
            "opcode 185 aliases use an unsigned full-width less-or-equal comparison"
        );
    }

    Fixture subtract_negative_bits;
    subtract_negative_bits.state.script_variables[2] = 3U;
    prime(
        subtract_negative_bits,
        OP_183_SUBTRACT_GLOBAL_INTEGER_WIDE_CLAMP_ZERO,
        2U,
        0xFFFFFFFBU
    );
    write_u16(subtract_negative_bits.state.window, 8U, OP_1025);
    const auto subtract_negative_result = subtract_negative_bits.step();
    test.expect_true(
        subtract_negative_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            subtract_negative_bits.state.script_variables[2] == 8U,
        "opcode 183 subtracts the supplied full u32 bit pattern before testing the result sign bit"
    );

    Fixture ge_taken;
    ge_taken.state.script_variables[0] = 0x80000000U;
    ge_taken.state.script_variables[2] = 0xFFFFFFFFU;
    prime(
        ge_taken,
        OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE,
        2U,
        0x80000000U
    );
    write_u32(ge_taken.state.window, 8U, 0x2222U);
    write_u16(ge_taken.ports.transferred_window, 0U, OP_1025);
    const auto ge_taken_result = ge_taken.step();
    test.expect_true(
        ge_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            ge_taken_result.executed_instruction_count == 2U &&
            ge_taken.context.talk_data_offset == 0x2222U &&
            ge_taken.context.instruction_offset == 0U &&
            ge_taken.state.previous_opcode ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            ge_taken.state.script_variables[0] == 0U &&
            ge_taken.ports.data_load_count == 1U &&
            ge_taken.ports.direct_audio_service_count == 1U,
        "opcode 184 taken reloads from its +8 target then applies the shared clamp and same-calls"
    );

    Fixture le_taken;
    le_taken.state.script_variables[2] = 0U;
    prime(le_taken, OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE, 2U, 0U);
    write_u32(le_taken.state.window, 8U, 0x3333U);
    write_u16(le_taken.ports.transferred_window, 0U, OP_1025);
    const auto le_taken_result = le_taken.step();
    test.expect_true(
        le_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            le_taken_result.executed_instruction_count == 2U &&
            le_taken.context.talk_data_offset == 0x3333U &&
            le_taken.state.previous_opcode ==
                OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE &&
            le_taken.ports.data_load_count == 1U &&
            le_taken.ports.direct_audio_service_count == 1U,
        "opcode 185 taken accepts equality, reloads and same-calls"
    );

    constexpr std::array<u16, 5U> opcodes{
        OP_181_SET_GLOBAL_INTEGER_WIDE,
        OP_182_ADD_GLOBAL_INTEGER_WIDE,
        OP_183_SUBTRACT_GLOBAL_INTEGER_WIDE_CLAMP_ZERO,
        OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE,
        OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE,
    };
    for (const u16 opcode : opcodes) {
        Fixture high_index;
        high_index.context.instruction_offset = 0x7FF8U;
        high_index.context.talk_data_offset = 0x1111U;
        high_index.state.loaded_file_number = 1U;
        high_index.state.loaded_data_offset = 0x1111U;
        high_index.state.window_loaded = true;
        high_index.state.previous_opcode = 0x55U;
        high_index.state.script_variables[0] = 0x80000000U;
        write_u16(high_index.state.window, 0x7FF8U, opcode);
        write_u16(high_index.state.window, 0x7FFAU, 64U);
        write_u32(high_index.state.window, 0x7FFCU, 0x12345678U);
        const auto result = high_index.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                high_index.context.instruction_offset == 0x7FF8U &&
                high_index.state.previous_opcode == opcode &&
                high_index.state.script_variables[0] == 0x80000000U &&
                high_index.ports.data_load_count == 0U,
            "opcodes 181-185 high index reads the full value but not a conditional target, then publishes and yields without shared clamp"
        );
    }

    Fixture negative_update;
    negative_update.state.previous_opcode = 0x55U;
    prime(
        negative_update, OP_181_SET_GLOBAL_INTEGER_WIDE, 0xFFFFU, 0x12345678U
    );
    const auto negative_update_result = negative_update.step();

    Fixture negative_branch_truncated;
    negative_branch_truncated.context.instruction_offset = 0x7FF8U;
    negative_branch_truncated.context.talk_data_offset = 0x1111U;
    negative_branch_truncated.state.loaded_file_number = 1U;
    negative_branch_truncated.state.loaded_data_offset = 0x1111U;
    negative_branch_truncated.state.window_loaded = true;
    write_u16(
        negative_branch_truncated.state.window,
        0x7FF8U,
        OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE
    );
    write_u16(negative_branch_truncated.state.window, 0x7FFAU, 0xFFFFU);
    write_u32(negative_branch_truncated.state.window, 0x7FFCU, 0U);
    const auto negative_branch_truncated_result =
        negative_branch_truncated.step();

    Fixture negative_branch;
    negative_branch.state.previous_opcode = 0x55U;
    prime(
        negative_branch,
        OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE,
        0xFFFFU,
        0U
    );
    write_u32(negative_branch.state.window, 8U, 0x4444U);
    const auto negative_branch_result = negative_branch.step();
    test.expect_true(
        negative_update_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_update.context.instruction_offset == 0U &&
            negative_update.state.previous_opcode == 0x55U &&
            negative_update.ports.direct_audio_service_count == 0U &&
            negative_branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            negative_branch_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_branch.context.instruction_offset == 0U &&
            negative_branch.state.previous_opcode == 0x55U &&
            negative_branch.ports.data_load_count == 0U,
        "wide negative update stops at its first array write, while a conditional reads its target before the first unsafe array read"
    );

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFAU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    write_u16(
        value_truncated.state.window, 0x7FFAU, OP_181_SET_GLOBAL_INTEGER_WIDE
    );
    write_u16(value_truncated.state.window, 0x7FFCU, 2U);
    write_u16(value_truncated.state.window, 0x7FFEU, 0x5678U);
    const auto value_truncated_result = value_truncated.step();
    test.expect_true(
        value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFAU,
        "wide shared numeric entry requires all four value bytes"
    );

    Fixture exact_update;
    exact_update.context.instruction_offset = 0x7FF8U;
    exact_update.context.talk_data_offset = 0x1111U;
    exact_update.state.loaded_file_number = 1U;
    exact_update.state.loaded_data_offset = 0x1111U;
    exact_update.state.window_loaded = true;
    exact_update.state.previous_opcode = 0x55U;
    write_u16(
        exact_update.state.window, 0x7FF8U, OP_181_SET_GLOBAL_INTEGER_WIDE
    );
    write_u16(exact_update.state.window, 0x7FFAU, 2U);
    write_u32(exact_update.state.window, 0x7FFCU, 0x12345678U);
    const auto exact_update_result = exact_update.step();

    Fixture exact_branch;
    exact_branch.context.instruction_offset = 0x7FF4U;
    exact_branch.context.talk_data_offset = 0x1111U;
    exact_branch.state.loaded_file_number = 1U;
    exact_branch.state.loaded_data_offset = 0x1111U;
    exact_branch.state.window_loaded = true;
    exact_branch.state.previous_opcode = 0x55U;
    exact_branch.state.script_variables[2] = 0U;
    write_u16(
        exact_branch.state.window,
        0x7FF4U,
        OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE
    );
    write_u16(exact_branch.state.window, 0x7FF6U, 2U);
    write_u32(exact_branch.state.window, 0x7FF8U, 1U);
    write_u32(exact_branch.state.window, 0x7FFCU, 0x5555U);
    const auto exact_branch_result = exact_branch.step();
    test.expect_true(
        exact_update_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_update.context.instruction_offset == 0x8000U &&
            exact_update.state.previous_opcode ==
                OP_181_SET_GLOBAL_INTEGER_WIDE &&
            exact_update.state.script_variables[2] == 0x12345678U,
        "wide update commits its exact-tail side effects before the same-call successor fetch fails"
    );
    test.expect_true(
        exact_branch_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_branch.context.instruction_offset == 0x8000U &&
            exact_branch.state.previous_opcode ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            exact_branch.ports.data_load_count == 0U,
        "wide not-taken branch commits its exact-tail side effects before the same-call successor fetch fails"
    );

    Fixture load_failure;
    load_failure.state.script_variables[0] = 0x80000000U;
    load_failure.state.script_variables[2] = 1U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime(load_failure, OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE, 2U, 1U);
    write_u32(load_failure.state.window, 8U, 0x6666U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x6666U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE &&
            load_failure.state.script_variables[0] == 0U &&
            load_failure.ports.direct_audio_service_count == 1U,
        "wide taken branch preserves loader then shared-clamp and previous failure order"
    );
}

void test_set_bounded_script_clock_protocol(openswd3::test::Context& test) {
    struct Sample {
        u16 value;
        u32 expected;
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<Sample, 4U> samples{
        Sample{0U, 0U},
        Sample{1000U, 1000U},
        Sample{1001U, 0U},
        Sample{0xFFFFU, 0U},
    };

    for (const u16 mask : alias_masks) {
        for (const auto sample : samples) {
            Fixture fixture;
            fixture.state.script_clock = 77U;
            fixture.state.script_clock_frame_counter = 9U;
            fixture.state.script_clock_origin = 88U;
            prime_loaded_instruction(
                fixture, static_cast<u16>(OP_34_SET_BOUNDED_SCRIPT_CLOCK | mask)
            );
            write_u16(fixture.state.window, 2U, sample.value);
            write_u16(fixture.state.window, 4U, OP_1025);
            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                    result.opcode == OP_1025 &&
                    result.executed_instruction_count == 2U &&
                    fixture.context.instruction_offset == 4U &&
                    fixture.state.previous_opcode ==
                        OP_34_SET_BOUNDED_SCRIPT_CLOCK &&
                    fixture.state.script_clock == sample.expected &&
                    fixture.state.script_clock_frame_counter == 9U &&
                    fixture.state.script_clock_origin == 88U &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcode 34 aliases set or reset the bounded script clock"
            );
        }
    }

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.state.script_clock = 77U;
    write_u16(truncated.state.window, 0x7FFEU, OP_34_SET_BOUNDED_SCRIPT_CLOCK);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.state.script_clock == 77U,
        "opcode 34 rejects a missing u16 before writing the script clock"
    );
}

void test_jump_if_byte_le_script_clock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock = 0x12340001U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK | mask)
        );
        fixture.state.window[2] = 2U;
        fixture.state.window[3] = 0xA5U;
        write_u32(fixture.state.window, 4U, 0x2222U);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
                fixture.state.script_clock == 0x12340001U &&
                fixture.state.window[3] == 0xA5U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 35 aliases compare the byte against only clock low16"
        );
    }

    Fixture equality_taken;
    equality_taken.state.script_clock = 0x123400FFU;
    prime_loaded_instruction(
        equality_taken, OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    equality_taken.state.window[2] = 0xFFU;
    equality_taken.state.window[3] = 0xA5U;
    write_u32(equality_taken.state.window, 4U, 0x3333U);
    write_u16(equality_taken.ports.transferred_window, 0U, OP_1025);
    const auto equality_taken_result = equality_taken.step();
    test.expect_true(
        equality_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            equality_taken_result.executed_instruction_count == 2U &&
            equality_taken.context.talk_data_offset == 0x3333U &&
            equality_taken.context.instruction_offset == 0U &&
            equality_taken.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            equality_taken.ports.data_load_count == 1U &&
            equality_taken.ports.last_data_offset == 0x3333U &&
            equality_taken.ports.direct_audio_service_count == 1U,
        "opcode 35 equality takes the same-file branch and continues"
    );

    Fixture no_target_needed;
    no_target_needed.context.instruction_offset = 0x7FFDU;
    no_target_needed.context.talk_data_offset = 0x1111U;
    no_target_needed.state.loaded_file_number = 1U;
    no_target_needed.state.loaded_data_offset = 0x1111U;
    no_target_needed.state.window_loaded = true;
    no_target_needed.state.script_clock = 0U;
    write_u16(
        no_target_needed.state.window,
        0x7FFDU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    no_target_needed.state.window[0x7FFFU] = 0xFFU;
    const auto no_target_needed_result = no_target_needed.step();
    test.expect_true(
        no_target_needed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_needed.context.instruction_offset == 0x8005U &&
            no_target_needed.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            no_target_needed.ports.data_load_count == 0U &&
            no_target_needed.ports.direct_audio_service_count == 0U,
        "opcode 35 not-taken path neither reads padding nor requires target"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFDU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.previous_opcode = 0x55U;
    target_truncated.state.script_clock = 0xFFU;
    write_u16(
        target_truncated.state.window,
        0x7FFDU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    target_truncated.state.window[0x7FFFU] = 0xFFU;
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFDU &&
            target_truncated.state.previous_opcode == 0x55U &&
            target_truncated.ports.data_load_count == 0U,
        "opcode 35 taken path reads target only after the comparison"
    );

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFEU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    value_truncated.state.previous_opcode = 0x55U;
    write_u16(
        value_truncated.state.window,
        0x7FFEU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    const auto value_truncated_result = value_truncated.step();
    test.expect_true(
        value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFEU &&
            value_truncated.state.previous_opcode == 0x55U,
        "opcode 35 requires the value byte before comparing"
    );

    Fixture load_failure;
    load_failure.state.script_clock = 1U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime_loaded_instruction(load_failure, OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK);
    load_failure.state.window[2] = 1U;
    load_failure.state.window[3] = 0xA5U;
    write_u32(load_failure.state.window, 4U, 0x4444U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x4444U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            load_failure.ports.direct_audio_service_count == 1U,
        "opcode 35 taken path preserves the checked loader failure order"
    );
}

void test_jump_if_script_clock_exceeds_origin_delta_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock_origin = 0xFFFFFFF0U;
        fixture.state.script_clock = 0x10U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA | mask
            )
        );
        write_u16(fixture.state.window, 2U, 0x20U);
        write_u32(fixture.state.window, 4U, 0x2222U);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
                fixture.state.script_clock_origin == 0xFFFFFFF0U &&
                fixture.state.script_clock == 0x10U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 36 aliases use wrapped threshold and strict comparison"
        );
    }

    Fixture taken;
    taken.state.script_clock_origin = 0xFFFFFFF0U;
    taken.state.script_clock = 0x11U;
    prime_loaded_instruction(
        taken, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(taken.state.window, 2U, 0x20U);
    write_u32(taken.state.window, 4U, 0x3333U);
    write_u16(taken.ports.transferred_window, 8U, OP_1025);
    const auto taken_result = taken.step();
    test.expect_true(
        taken_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            taken_result.executed_instruction_count == 2U &&
            taken.context.talk_data_offset == 0x3333U &&
            taken.context.instruction_offset == 8U &&
            taken.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            taken.ports.data_load_count == 1U &&
            taken.ports.last_data_offset == 0x3333U &&
            taken.ports.direct_audio_service_count == 1U &&
            taken.ports.beep_count == 0U,
        "opcode 36 taken path reloads then continues at new-window offset 8"
    );

    Fixture full_width_clock;
    full_width_clock.state.script_clock_origin = 0U;
    full_width_clock.state.script_clock = 0x10000U;
    prime_loaded_instruction(
        full_width_clock, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(full_width_clock.state.window, 2U, 1U);
    write_u32(full_width_clock.state.window, 4U, 0x3535U);
    write_u16(full_width_clock.ports.transferred_window, 8U, OP_1025);
    const auto full_width_clock_result = full_width_clock.step();
    test.expect_true(
        full_width_clock_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            full_width_clock.context.talk_data_offset == 0x3535U &&
            full_width_clock.context.instruction_offset == 8U &&
            full_width_clock.ports.data_load_count == 1U,
        "opcode 36 compares the full 32-bit script clock"
    );

    Fixture no_target_needed;
    no_target_needed.context.instruction_offset = 0x7FFCU;
    no_target_needed.context.talk_data_offset = 0x1111U;
    no_target_needed.state.loaded_file_number = 1U;
    no_target_needed.state.loaded_data_offset = 0x1111U;
    no_target_needed.state.window_loaded = true;
    no_target_needed.state.script_clock_origin = 0U;
    no_target_needed.state.script_clock = 1U;
    write_u16(
        no_target_needed.state.window,
        0x7FFCU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(no_target_needed.state.window, 0x7FFEU, 1U);
    const auto no_target_needed_result = no_target_needed.step();
    test.expect_true(
        no_target_needed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_needed.context.instruction_offset == 0x8004U &&
            no_target_needed.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            no_target_needed.ports.data_load_count == 0U,
        "opcode 36 not-taken path does not require the target"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFCU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.previous_opcode = 0x55U;
    target_truncated.state.script_clock_origin = 0U;
    target_truncated.state.script_clock = 2U;
    write_u16(
        target_truncated.state.window,
        0x7FFCU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(target_truncated.state.window, 0x7FFEU, 1U);
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFCU &&
            target_truncated.state.previous_opcode == 0x55U &&
            target_truncated.ports.data_load_count == 0U,
        "opcode 36 taken path reads target only after comparison"
    );

    Fixture delta_truncated;
    delta_truncated.context.instruction_offset = 0x7FFEU;
    delta_truncated.context.talk_data_offset = 0x1111U;
    delta_truncated.state.loaded_file_number = 1U;
    delta_truncated.state.loaded_data_offset = 0x1111U;
    delta_truncated.state.window_loaded = true;
    delta_truncated.state.previous_opcode = 0x55U;
    write_u16(
        delta_truncated.state.window,
        0x7FFEU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    const auto delta_truncated_result = delta_truncated.step();
    test.expect_true(
        delta_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            delta_truncated.context.instruction_offset == 0x7FFEU &&
            delta_truncated.state.previous_opcode == 0x55U,
        "opcode 36 requires the complete u16 delta before comparison"
    );

    Fixture load_failure;
    load_failure.state.script_clock_origin = 0U;
    load_failure.state.script_clock = 2U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime_loaded_instruction(
        load_failure, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(load_failure.state.window, 2U, 1U);
    write_u32(load_failure.state.window, 4U, 0x4444U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x4444U &&
            load_failure.context.instruction_offset == 8U &&
            load_failure.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            load_failure.ports.direct_audio_service_count == 1U,
        "opcode 36 failure preserves loader then post-load +8 ordering"
    );
}

void test_snapshot_script_clock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock = 0x89ABCDEFU;
        fixture.state.script_clock_origin = 0x01234567U;
        fixture.state.script_clock_frame_counter = 20U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_37_SNAPSHOT_SCRIPT_CLOCK | mask)
        );
        write_u16(fixture.state.window, 2U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.script_clock_origin == 0x89ABCDEFU &&
                fixture.state.script_clock == 0x89ABCDEFU &&
                fixture.state.script_clock_frame_counter == 20U &&
                fixture.state.previous_opcode == OP_37_SNAPSHOT_SCRIPT_CLOCK &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 37 aliases snapshot the full clock and continue"
        );
    }

    Fixture window_tail;
    window_tail.context.instruction_offset = 0x7FFEU;
    window_tail.context.talk_data_offset = 0x1111U;
    window_tail.state.loaded_file_number = 1U;
    window_tail.state.loaded_data_offset = 0x1111U;
    window_tail.state.window_loaded = true;
    window_tail.state.script_clock = 0xFEDCBA98U;
    window_tail.state.script_clock_origin = 0x01234567U;
    write_u16(window_tail.state.window, 0x7FFEU, OP_37_SNAPSHOT_SCRIPT_CLOCK);
    const auto window_tail_result = window_tail.step();
    test.expect_true(
        window_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            window_tail.context.instruction_offset == 0x8000U &&
            window_tail.state.script_clock_origin == 0xFEDCBA98U &&
            window_tail.state.script_clock == 0xFEDCBA98U &&
            window_tail.state.previous_opcode == OP_37_SNAPSHOT_SCRIPT_CLOCK,
        "opcode 37 needs no bytes beyond the two-byte opcode"
    );
}

void test_clear_role_from_scene_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_38_CLEAR_ROLE_FROM_SCENE | mask)
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0xFFFFU &&
                request.tile_y == 0xFFFFU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0U &&
                request.flags_and_mask == 0x7FFFU &&
                request.logical_map_id == 0xFFFFU &&
                result.active_object_reset_count == 0U,
            "opcode 38 aliases patch only MAPS role flags on ordinary miss"
        );
    }

    Fixture raw_current_source_fallback;
    raw_current_source_fallback.context.source_guid = 0x4321U;
    prime_loaded_instruction(
        raw_current_source_fallback, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(raw_current_source_fallback.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_source_fallback.state.window, 4U, OP_1025);
    const auto raw_current_source_result = raw_current_source_fallback.step();
    test.expect_true(
        raw_current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_source_fallback.ports.role_patch_requests.size() ==
                1U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .guid == 0xFFF0U &&
            raw_current_source_fallback.context.instruction_offset == 4U &&
            raw_current_source_fallback.state.previous_opcode ==
                OP_38_CLEAR_ROLE_FROM_SCENE,
        "opcode 38 missing FFF0 lookup patches the original raw selector"
    );

    Fixture controlled_out_of_range;
    controlled_out_of_range.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        controlled_out_of_range, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(controlled_out_of_range.state.window, 2U, 0xFFFEU);
    const auto controlled_out_of_range_result =
        controlled_out_of_range.step(0, 0, 99U);
    test.expect_true(
        controlled_out_of_range_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            controlled_out_of_range_result.opcode == 0U &&
            controlled_out_of_range_result.executed_instruction_count == 0U &&
            controlled_out_of_range.context.instruction_offset == 0U &&
            controlled_out_of_range.state.previous_opcode == 0x55U &&
            controlled_out_of_range.ports.role_patch_requests.empty(),
        "opcode 38 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live_current_source_without_surface;
    live_current_source_without_surface.state.previous_opcode = 0x55U;
    live_current_source_without_surface.roles[1].flags = 0xE0009234U;
    prime_loaded_instruction(
        live_current_source_without_surface, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(live_current_source_without_surface.state.window, 2U, 0xFFF0U);
    const auto live_current_source_without_surface_result =
        live_current_source_without_surface.step();
    test.expect_true(
        live_current_source_without_surface_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            live_current_source_without_surface.context.instruction_offset ==
                0U &&
            live_current_source_without_surface.roles[1].flags == 0x1234U &&
            live_current_source_without_surface.state.previous_opcode ==
                0x55U &&
            live_current_source_without_surface.ports.role_patch_requests
                .empty(),
        "opcode 38 clears live FFF0 role flags before the surface unsafe point"
    );

    Fixture live;
    live.roles[0].guid = 0x00F8U;
    live.roles[0].flags = 0U;
    live.roles[1].guid = 0x00F8U;
    live.roles[1].flags = 0xE0009234U;
    live.roles[1].map_cell_pointer_32 = 0U;
    live.roles[1].action.field_2c = 1U;
    live.roles[1].action.field_30 = 1U;
    std::array<u8, 16U> surface{};
    surface.fill(0xFFU);
    live.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = surface,
        };
    live.active_object_slots[0].bytes.fill(0x22U);
    write_u16(live.active_object_slots[0].bytes, 0U, 0U);
    live.active_object_slots[1].bytes.fill(0x33U);
    write_u16(live.active_object_slots[1].bytes, 0U, 1U);
    live.active_object_slots.back().bytes.fill(0x44U);
    write_u16(live.active_object_slots.back().bytes, 0U, 0U);
    prime_loaded_instruction(live, OP_38_CLEAR_ROLE_FROM_SCENE);
    write_u16(live.state.window, 2U, 0xFFFEU);
    write_u16(live.state.window, 4U, OP_1025);
    const auto live_result = live.step(0, 0, 1U);
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 4U &&
            live.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
            live.roles[1].flags == 0x1234U &&
            read_u32(surface, 0U) == 0xCF7FFFFFU &&
            live_result.active_object_reset_count == 2U &&
            std::ranges::all_of(
                live.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.active_object_slots[1].bytes[2U] == 0x33U &&
            std::ranges::all_of(
                live.active_object_slots.back().bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.ports.role_patch_requests.empty(),
        "opcode 38 relooks up the first same-GUID role and scans all 72 slots"
    );

    Fixture wide_role_index;
    std::vector<LegacyWorldRoleRecord> wide_roles(0x10001U);
    auto& last_wide_role = wide_roles.back();
    last_wide_role.guid = 0x00F8U;
    last_wide_role.flags = 0xE0009234U;
    last_wide_role.map_cell_pointer_32 = 0U;
    last_wide_role.action.field_2c = 1U;
    last_wide_role.action.field_30 = 1U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        wide_slots{};
    wide_slots[0].bytes.fill(0x22U);
    write_u16(wide_slots[0].bytes, 0U, 0U);
    std::array<u8, 16U> wide_surface{};
    wide_surface.fill(0xFFU);
    wide_role_index.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = wide_surface,
        };
    prime_loaded_instruction(wide_role_index, OP_38_CLEAR_ROLE_FROM_SCENE);
    write_u16(wide_role_index.state.window, 2U, 0xFFFEU);
    write_u16(wide_role_index.state.window, 4U, OP_1025);
    const auto wide_role_index_result =
        openswd3::world_map::step_legacy_world_story_vm(
            wide_role_index.context,
            wide_role_index.state,
            wide_roles,
            0x10000U,
            wide_slots,
            wide_role_index.maps_payload,
            wide_role_index.dialogs,
            wide_role_index.dialog_resources,
            wide_role_index.first_name,
            wide_role_index.second_name,
            wide_role_index.runtime,
            wide_role_index.ports
        );
    test.expect_true(
        wide_role_index_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            wide_role_index_result.executed_instruction_count == 2U &&
            wide_role_index_result.active_object_reset_count == 0U &&
            last_wide_role.flags == 0x1234U &&
            wide_slots[0].bytes[2U] == 0x22U &&
            wide_role_index.ports.role_patch_requests.empty(),
        "opcode 38 compares object u16 index with full replacement u32"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FFCU;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window, 0x7FFCU, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(no_following_bytes.state.window, 0x7FFEU, 0x1234U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_38_CLEAR_ROLE_FROM_SCENE &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 38 requires no bytes after its four-byte record"
    );

    Fixture operand_truncated;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x55U;
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x55U &&
            operand_truncated.ports.role_patch_requests.empty(),
        "opcode 38 requires the complete u16 selector before side effects"
    );
}

void test_set_role_flag_8000_and_clear_one_shots_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS | mask
            )
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0xFFFFU &&
                request.tile_y == 0xFFFFU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0x8000U &&
                request.flags_and_mask == 0xFFFFU &&
                request.logical_map_id == 0xFFFFU &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 39 aliases patch only the MAPS role flag on ordinary miss"
        );
    }

    Fixture raw_current_source_fallback;
    raw_current_source_fallback.context.source_guid = 0x4321U;
    prime_loaded_instruction(
        raw_current_source_fallback,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(raw_current_source_fallback.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_source_fallback.state.window, 4U, OP_1025);
    const auto raw_current_source_result = raw_current_source_fallback.step();
    test.expect_true(
        raw_current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_source_fallback.ports.role_patch_requests.size() ==
                1U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .guid == 0xFFF0U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .flags_or_mask == 0x8000U &&
            raw_current_source_fallback.context.instruction_offset == 4U &&
            raw_current_source_fallback.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS,
        "opcode 39 missing FFF0 lookup patches the original raw selector"
    );

    Fixture controlled_out_of_range;
    controlled_out_of_range.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        controlled_out_of_range, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(controlled_out_of_range.state.window, 2U, 0xFFFEU);
    const auto controlled_out_of_range_result =
        controlled_out_of_range.step(0, 0, 99U);
    test.expect_true(
        controlled_out_of_range_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            controlled_out_of_range_result.opcode == 0U &&
            controlled_out_of_range_result.executed_instruction_count == 0U &&
            controlled_out_of_range.context.instruction_offset == 0U &&
            controlled_out_of_range.state.previous_opcode == 0x55U &&
            controlled_out_of_range.ports.role_patch_requests.empty(),
        "opcode 39 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live_without_surface;
    live_without_surface.state.previous_opcode = 0x55U;
    live_without_surface.roles[1].flags = 0xA5A50001U;
    live_without_surface.roles[1].action.one_shot_base_variant = 0x11111111U;
    live_without_surface.roles[1].action.one_shot_variant_delta = 0x22222222U;
    prime_loaded_instruction(
        live_without_surface, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(live_without_surface.state.window, 2U, 0xFFF0U);
    const auto live_without_surface_result = live_without_surface.step();
    test.expect_true(
        live_without_surface_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            live_without_surface.context.instruction_offset == 0U &&
            live_without_surface.roles[1].flags == 0xA5A58001U &&
            live_without_surface.roles[1].action.one_shot_base_variant ==
                0x11111111U &&
            live_without_surface.roles[1].action.one_shot_variant_delta ==
                0x22222222U &&
            live_without_surface.state.previous_opcode == 0x55U &&
            live_without_surface.ports.role_patch_requests.empty(),
        "opcode 39 sets the live flag before the surface unsafe point"
    );

    Fixture partial_surface_failure;
    partial_surface_failure.state.previous_opcode = 0x55U;
    partial_surface_failure.roles[1].flags = 0xA5A50001U;
    partial_surface_failure.roles[1].map_cell_pointer_32 = 3U;
    partial_surface_failure.roles[1].action.field_2c = 2U;
    partial_surface_failure.roles[1].action.field_30 = 1U;
    partial_surface_failure.roles[1].action.one_shot_base_variant = 0x11111111U;
    partial_surface_failure.roles[1].action.one_shot_variant_delta =
        0x22222222U;
    std::array<u8, 16U> partial_surface{};
    partial_surface.fill(0xFFU);
    partial_surface_failure.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = partial_surface,
        };
    prime_loaded_instruction(
        partial_surface_failure, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(partial_surface_failure.state.window, 2U, 0xFFF0U);
    const auto partial_surface_failure_result = partial_surface_failure.step();
    test.expect_true(
        partial_surface_failure_result.status ==
                LegacyWorldStoryVmStatus::role_surface_failed &&
            partial_surface_failure.context.instruction_offset == 0U &&
            partial_surface_failure.roles[1].flags == 0xA5A58001U &&
            read_u32(partial_surface, 12U) == 0xCF7FFFFFU &&
            partial_surface_failure.roles[1].action.one_shot_base_variant ==
                0x11111111U &&
            partial_surface_failure.roles[1].action.one_shot_variant_delta ==
                0x22222222U &&
            partial_surface_failure.state.previous_opcode == 0x55U,
        "opcode 39 preserves partial surface effects before checked failure"
    );

    Fixture live;
    live.roles[1].flags = 0xA5A50001U;
    live.roles[1].map_cell_pointer_32 = 0U;
    live.roles[1].action.field_2c = 1U;
    live.roles[1].action.field_30 = 1U;
    live.roles[1].action.one_shot_base_variant = 0x11111111U;
    live.roles[1].action.one_shot_variant_delta = 0x22222222U;
    std::array<u8, 16U> surface{};
    surface.fill(0xFFU);
    live.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = surface,
        };
    prime_loaded_instruction(
        live, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(live.state.window, 2U, 0xFFFEU);
    write_u16(live.state.window, 4U, OP_1025);
    const auto live_result = live.step(0, 0, 1U);
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 4U &&
            live.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            live.roles[1].flags == 0xA5A58001U &&
            read_u32(surface, 0U) == 0xCF7FFFFFU &&
            live.roles[1].action.one_shot_base_variant == 0xFFFFFFFFU &&
            live.roles[1].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            live.ports.role_patch_requests.empty() &&
            live.ports.direct_audio_service_count == 0U,
        "opcode 39 clears surface before setting both one-shot fields to -1"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FFCU;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window,
        0x7FFCU,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(no_following_bytes.state.window, 0x7FFEU, 0x1234U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 39 requires no bytes after its four-byte record"
    );

    Fixture operand_truncated;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x55U;
    write_u16(
        operand_truncated.state.window,
        0x7FFEU,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x55U &&
            operand_truncated.ports.role_patch_requests.empty(),
        "opcode 39 requires the complete u16 selector before side effects"
    );
}

void test_relocate_role_and_complete_path_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH | mask)
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, 0x1015U);
        write_u16(fixture.state.window, 6U, 0x100FU);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0x1015U &&
                request.tile_y == 0x100FU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0U &&
                request.flags_and_mask == 0xFFFFU &&
                request.logical_map_id == 0xFFFFU &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 40 aliases patch raw MAPS coordinates on ordinary miss"
        );
    }

    Fixture raw_current_token;
    prime_loaded_instruction(
        raw_current_token, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_token.state.window, 4U, 21U);
    write_u16(raw_current_token.state.window, 6U, 15U);
    write_u16(raw_current_token.state.window, 8U, OP_1025);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_token.ports.role_patch_requests.size() == 1U &&
            raw_current_token.ports.role_patch_requests.front().guid ==
                0xFFF0U &&
            raw_current_token.roles[1].world_x == 320U &&
            raw_current_token.roles[1].world_y == 240U,
        "opcode 40 treats FFF0 as a literal GUID rather than current source"
    );

    Fixture literal_current_token;
    literal_current_token.roles[2].guid = 0xFFF0U;
    prime_loaded_instruction(
        literal_current_token, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(literal_current_token.state.window, 2U, 0xFFF0U);
    write_u16(literal_current_token.state.window, 4U, 21U);
    write_u16(literal_current_token.state.window, 6U, 15U);
    const auto literal_current_token_result = literal_current_token.step();
    test.expect_true(
        literal_current_token_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            literal_current_token.context.instruction_offset == 0U &&
            literal_current_token.ports.role_patch_requests.empty(),
        "opcode 40 resolves a real FFF0 GUID without source substitution"
    );

    Fixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        invalid_controlled, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 21U);
    write_u16(invalid_controlled.state.window, 6U, 15U);
    const auto invalid_controlled_result = invalid_controlled.step(0, 0, 99U);
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 40 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live;
    live.roles[1].flags = 0x82000001U;
    live.roles[1].action.cached_base_variant = 7U;
    live.roles[1].action.cached_variant_delta = 8U;
    live.roles[1].action.wait_remaining = 9U;
    StoryPathHarness live_paths{live};
    auto& completed_slot = live.active_object_slots[0];
    write_u16(completed_slot.bytes, 0x00U, 1U);
    completed_slot.bytes[0x1BU] = 2U;
    prime_loaded_instruction(live, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    write_u16(live.state.window, 2U, 0x00F8U);
    write_u16(live.state.window, 4U, 0x1015U);
    write_u16(live.state.window, 6U, 0x100FU);
    write_u16(live.state.window, 8U, OP_1025);
    const auto live_result = live.step();
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 8U &&
            live.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            live.roles[1].world_x == 336U && live.roles[1].world_y == 240U &&
            live.roles[1].map_cell_pointer_32 == 771U &&
            live.roles[1].flags == 1U &&
            live.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
            live.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
            live.roles[1].action.wait_remaining == 9U &&
            std::ranges::all_of(
                completed_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.ports.role_patch_requests.empty() &&
            live.ports.direct_audio_service_count == 0U,
        "opcode 40 schedules, completes type2 ownership, then clears caller state"
    );

    Fixture controlled;
    controlled.roles[1].action.cached_base_variant = 7U;
    controlled.roles[1].action.cached_variant_delta = 8U;
    StoryPathHarness controlled_paths{controlled, 1U};
    prime_loaded_instruction(controlled, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 21U);
    write_u16(controlled.state.window, 6U, 15U);
    write_u16(controlled.state.window, 8U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.context.instruction_offset == 8U &&
            controlled.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            controlled.roles[1].action.cached_base_variant == 7U &&
            controlled.roles[1].action.cached_variant_delta == 8U &&
            controlled.ports.role_patch_requests.empty(),
        "opcode 40 compares the raw selector to source GUID for cache reset"
    );

    Fixture helper_failure;
    openswd3::world_map::LegacyWorldStoryPathRuntime incomplete_paths{};
    incomplete_paths.roles = helper_failure.roles;
    incomplete_paths.active_object_slots = helper_failure.active_object_slots;
    helper_failure.runtime.story_paths = &incomplete_paths;
    helper_failure.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        helper_failure, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(helper_failure.state.window, 2U, 0x00F8U);
    write_u16(helper_failure.state.window, 4U, 21U);
    write_u16(helper_failure.state.window, 6U, 15U);
    const auto helper_failure_result = helper_failure.step();
    test.expect_true(
        helper_failure_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            helper_failure.context.instruction_offset == 0U &&
            helper_failure.state.previous_opcode == 0x55U &&
            helper_failure.roles[1].world_x == 320U &&
            helper_failure.roles[1].world_y == 240U,
        "opcode 40 stops at a checked sub_42DAF0 runtime failure"
    );

    Fixture found_truncated;
    found_truncated.context.instruction_offset = 0x7FFCU;
    found_truncated.context.talk_data_offset = 0x1111U;
    found_truncated.state.loaded_file_number = 1U;
    found_truncated.state.loaded_data_offset = 0x1111U;
    found_truncated.state.window_loaded = true;
    found_truncated.state.previous_opcode = 0x55U;
    write_u16(
        found_truncated.state.window,
        0x7FFCU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(found_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto found_truncated_result = found_truncated.step();
    test.expect_true(
        found_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_truncated_result.executed_instruction_count == 1U &&
            found_truncated.context.instruction_offset == 0x7FFCU &&
            found_truncated.state.previous_opcode == 0x55U &&
            found_truncated.ports.role_patch_requests.empty(),
        "opcode 40 reads destination operands only after a successful lookup"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    missing_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_truncated.state.window,
        0x7FFCU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated_result.executed_instruction_count == 1U &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U &&
            missing_truncated.ports.role_patch_requests.empty(),
        "opcode 40 missing-role MAPS operands are read after lookup failure"
    );

    Fixture missing_y_truncated;
    missing_y_truncated.context.instruction_offset = 0x7FFAU;
    missing_y_truncated.context.talk_data_offset = 0x1111U;
    missing_y_truncated.state.loaded_file_number = 1U;
    missing_y_truncated.state.loaded_data_offset = 0x1111U;
    missing_y_truncated.state.window_loaded = true;
    missing_y_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_y_truncated.state.window,
        0x7FFAU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(missing_y_truncated.state.window, 0x7FFCU, 0x7777U);
    write_u16(missing_y_truncated.state.window, 0x7FFEU, 21U);
    const auto missing_y_truncated_result = missing_y_truncated.step();
    test.expect_true(
        missing_y_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_y_truncated.context.instruction_offset == 0x7FFAU &&
            missing_y_truncated.state.previous_opcode == 0x55U &&
            missing_y_truncated.ports.role_patch_requests.empty(),
        "opcode 40 reads the +6 coordinate before using the available +4 word"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FF8U;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window,
        0x7FF8U,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(no_following_bytes.state.window, 0x7FFAU, 0x7777U);
    write_u16(no_following_bytes.state.window, 0x7FFCU, 21U);
    write_u16(no_following_bytes.state.window, 0x7FFEU, 15U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 40 requires no bytes after its eight-byte record"
    );
}

void test_reload_indexed_target_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_41_RELOAD_INDEXED_TARGET,
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0x4000U),
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0x8000U),
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0xC000U),
    };
    constexpr u32 first_target = 0x11112222U;
    constexpr u32 second_target = 0x33334444U;
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.indexed_target_selector = 1U;
        prime_loaded_instruction(fixture, raw_word);
        write_u32(fixture.state.window, 2U, first_target);
        write_u32(fixture.state.window, 6U, second_target);
        write_u32(fixture.state.window, 10U, 0xFF00FF00U);
        write_u16(fixture.ports.transferred_window, 0U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.indexed_target_selector == 0U &&
                fixture.context.talk_data_offset == second_target &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.loaded_file_number == 1U &&
                fixture.state.loaded_data_offset == second_target &&
                fixture.state.window_loaded &&
                fixture.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.last_data_file_number == 1U &&
                fixture.ports.last_data_offset == second_target &&
                !fixture.ports.last_data_clear_before_read &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
            "opcode 41 aliases select one dword target and reload in same call"
        );
    }

    Fixture out_of_range;
    out_of_range.indexed_target_selector = 0x00010002U;
    prime_loaded_instruction(out_of_range, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(out_of_range.state.window, 2U, first_target);
    write_u32(out_of_range.state.window, 6U, second_target);
    write_u32(out_of_range.state.window, 10U, 0xFF00FF00U);
    write_u16(out_of_range.ports.transferred_window, 0U, OP_1025);
    const auto out_of_range_result = out_of_range.step();
    test.expect_true(
        out_of_range_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            out_of_range.ports.last_data_offset == first_target &&
            out_of_range.indexed_target_selector == 0U &&
            out_of_range.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "opcode 41 compares the full u32 selector and falls back to index zero"
    );

    Fixture sentinel_selected;
    sentinel_selected.indexed_target_selector = 2U;
    prime_loaded_instruction(sentinel_selected, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(sentinel_selected.state.window, 2U, first_target);
    write_u32(sentinel_selected.state.window, 6U, second_target);
    write_u32(sentinel_selected.state.window, 10U, 0xFF00FF00U);
    write_u16(sentinel_selected.ports.transferred_window, 0U, OP_1025);
    const auto sentinel_selected_result = sentinel_selected.step();
    test.expect_true(
        sentinel_selected_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            sentinel_selected.ports.last_data_offset == 0xFF00FF00U &&
            sentinel_selected.context.talk_data_offset == 0xFF00FF00U &&
            sentinel_selected.indexed_target_selector == 0U,
        "opcode 41 preserves the selector-equals-count sentinel target bug"
    );

    Fixture load_failure;
    load_failure.indexed_target_selector = 1U;
    prime_loaded_instruction(load_failure, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(load_failure.state.window, 2U, first_target);
    write_u32(load_failure.state.window, 6U, second_target);
    write_u32(load_failure.state.window, 10U, 0xFF00FF00U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.indexed_target_selector == 0U &&
            load_failure.context.talk_data_offset == second_target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 41 checked load failure preserves audio, target, reset, and previous"
    );

    Fixture missing_owner;
    missing_owner.indexed_target_selector = 9U;
    missing_owner.runtime.indexed_target_selector = nullptr;
    prime_loaded_instruction(missing_owner, OP_41_RELOAD_INDEXED_TARGET);
    missing_owner.state.previous_opcode = 0x55U;
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.indexed_target_selector == 9U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U &&
            missing_owner.ports.data_load_count == 0U &&
            missing_owner.ports.direct_audio_service_count == 0U,
        "opcode 41 stops at the missing indexed-selector owner"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFAU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.indexed_target_selector = 1U;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFAU, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(truncated.state.window, 0x7FFCU, first_target);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.indexed_target_selector == 1U &&
            truncated.context.instruction_offset == 0x7FFAU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U &&
            truncated.ports.direct_audio_service_count == 0U,
        "opcode 41 checked scan stops when the FF00FF00 terminator is absent"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.indexed_target_selector = 0U;
    write_u16(exact_tail.state.window, 0x7FF6U, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(exact_tail.state.window, 0x7FF8U, first_target);
    write_u32(exact_tail.state.window, 0x7FFCU, 0xFF00FF00U);
    write_u16(exact_tail.ports.transferred_window, 0U, OP_1025);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            exact_tail_result.executed_instruction_count == 2U &&
            exact_tail.ports.last_data_offset == first_target &&
            exact_tail.indexed_target_selector == 0U &&
            exact_tail.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "opcode 41 accepts a target table ending at the window boundary"
    );
}

void test_interaction_lock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> set_aliases{
        OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0x4000U
        ),
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0x8000U
        ),
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0xC000U
        ),
    };
    for (const u16 raw_word : set_aliases) {
        Fixture fixture;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE0005U;
        fixture.roles[0].action.base_variant = 7U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.ports.action_update_count == 1U &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE8005U &&
                fixture.roles[0].action.base_variant == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 42 aliases set shared lock, reset base variant, and continue"
        );
    }

    Fixture update_failure;
    update_failure.dialogs.close.flagged_dialog_counter = 0x12340002U;
    update_failure.roles[0].action.base_variant = 9U;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(
        update_failure, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    write_u16(update_failure.state.window, 2U, OP_1025);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.dialogs.close.flagged_dialog_counter ==
                0x12348002U &&
            update_failure.roles[0].action.base_variant == 0U &&
            update_failure.context.instruction_offset == 2U &&
            update_failure.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        "opcode 42 action-update failure is diagnostic-only after state writes"
    );

    constexpr std::array<u16, 4U> clear_aliases{
        OP_43_CLEAR_INTERACTION_LOCK,
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0x4000U),
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0x8000U),
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0xC000U),
    };
    for (const u16 raw_word : clear_aliases) {
        Fixture fixture;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
        fixture.roles[0].action.base_variant = 7U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 0U &&
                fixture.ports.action_update_count == 0U &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE0005U &&
                fixture.roles[0].action.base_variant == 7U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 43 aliases clear only shared interaction-lock bit fifteen"
        );
    }

    Fixture chained;
    chained.dialogs.close.flagged_dialog_counter = 0x12340003U;
    chained.roles[0].action.base_variant = 11U;
    prime_loaded_instruction(
        chained, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    write_u16(chained.state.window, 2U, OP_43_CLEAR_INTERACTION_LOCK);
    write_u16(chained.state.window, 4U, OP_1025);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            chained_result.executed_instruction_count == 3U &&
            chained.dialogs.close.flagged_dialog_counter == 0x12340003U &&
            chained.roles[0].action.base_variant == 0U &&
            chained.ports.action_update_count == 1U &&
            chained.context.instruction_offset == 4U &&
            chained.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK,
        "opcodes 42 and 43 share one lock owner across same-call continuation"
    );

    Fixture invalid_controlled;
    invalid_controlled.dialogs.close.flagged_dialog_counter = 0x1234U;
    prime_loaded_instruction(
        invalid_controlled, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.dialogs.close.flagged_dialog_counter ==
                0x1234U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 42 invalid controlled owner stops at the VM session boundary"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.dialogs.close.flagged_dialog_counter = 0x1234U;
    exact_tail.roles[0].action.base_variant = 7U;
    write_u16(
        exact_tail.state.window,
        0x7FFEU,
        OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.dialogs.close.flagged_dialog_counter == 0x9234U &&
            exact_tail.roles[0].action.base_variant == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        "opcode 42 needs no following byte before its effects and previous publish"
    );

    Fixture clear_invalid_controlled;
    clear_invalid_controlled.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
    prime_loaded_instruction(
        clear_invalid_controlled, OP_43_CLEAR_INTERACTION_LOCK
    );
    clear_invalid_controlled.state.previous_opcode = 0x55U;
    const auto clear_invalid_controlled_result = clear_invalid_controlled.step(
        0, 0, static_cast<u32>(clear_invalid_controlled.roles.size())
    );
    test.expect_true(
        clear_invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            clear_invalid_controlled_result.opcode == 0U &&
            clear_invalid_controlled_result.executed_instruction_count == 0U &&
            clear_invalid_controlled.dialogs.close.flagged_dialog_counter ==
                0xCAFE8005U &&
            clear_invalid_controlled.context.instruction_offset == 0U &&
            clear_invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 43 invalid controlled owner stops at the VM session boundary"
    );

    Fixture clear_exact_tail;
    clear_exact_tail.context.instruction_offset = 0x7FFEU;
    clear_exact_tail.context.talk_data_offset = 0x1111U;
    clear_exact_tail.state.loaded_file_number = 1U;
    clear_exact_tail.state.loaded_data_offset = 0x1111U;
    clear_exact_tail.state.window_loaded = true;
    clear_exact_tail.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
    write_u16(
        clear_exact_tail.state.window, 0x7FFEU, OP_43_CLEAR_INTERACTION_LOCK
    );
    const auto clear_exact_tail_result = clear_exact_tail.step();
    test.expect_true(
        clear_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            clear_exact_tail_result.executed_instruction_count == 1U &&
            clear_exact_tail_result.action_update_count == 0U &&
            clear_exact_tail.dialogs.close.flagged_dialog_counter ==
                0xCAFE0005U &&
            clear_exact_tail.context.instruction_offset == 0x8000U &&
            clear_exact_tail.state.previous_opcode ==
                OP_43_CLEAR_INTERACTION_LOCK,
        "opcode 43 needs no following byte before clear and previous publish"
    );
}

void test_set_role_action_wait_override_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> aliases{
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0x4000U),
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0x8000U),
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0xC000U),
    };
    for (const u16 raw_word : aliases) {
        Fixture fixture;
        fixture.roles[1].action.wait_remaining = 0xCAFEU;
        fixture.roles[1].action.wait_default = 0xBEEFU;
        fixture.roles[1].action.wait_override = 0x1234U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8003U);
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.ports.action_update_count == 1U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[1].action.wait_default == 0xBEEFU &&
                fixture.roles[1].action.wait_override == 0x8003U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 44 aliases write wait override, clear remaining, and continue"
        );
    }

    Fixture current_source;
    current_source.roles[1].action.wait_remaining = 9U;
    prime_loaded_instruction(
        current_source, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 0xFFFFU);
    write_u16(current_source.state.window, 6U, OP_1025);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            current_source.roles[1].action.wait_override == 0xFFFFU &&
            current_source.roles[1].action.wait_remaining == 0U &&
            current_source.roles[0].action.wait_override == 0U &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 44 translates FFF0 to the context GUID before lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.wait_override = 0x1111U;
    controlled.roles[2].action.wait_remaining = 7U;
    prime_loaded_instruction(controlled, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 0x8123U);
    write_u16(controlled.state.window, 6U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.roles[2].action.wait_override == 0x8123U &&
            controlled.roles[2].action.wait_remaining == 0U &&
            controlled.roles[1].action.wait_override == 0x1111U,
        "opcode 44 passes FFFE to the helper for direct controlled-index selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[1].action.wait_remaining = 5U;
    first_clear_match.roles[2].action.wait_remaining = 7U;
    prime_loaded_instruction(
        first_clear_match, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, 0x8004U);
    write_u16(first_clear_match.state.window, 6U, OP_1025);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            first_clear_match.roles[0].action.wait_override == 0U &&
            first_clear_match.roles[1].action.wait_override == 0x8004U &&
            first_clear_match.roles[1].action.wait_remaining == 0U &&
            first_clear_match.roles[2].action.wait_override == 0U &&
            first_clear_match.roles[2].action.wait_remaining == 7U,
        "opcode 44 lookup skips bit-28 roles and uses the first clear GUID match"
    );

    Fixture update_failure;
    update_failure.roles[1].action.wait_remaining = 9U;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(
        update_failure, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(update_failure.state.window, 2U, 0x00F8U);
    write_u16(update_failure.state.window, 4U, 0x8005U);
    write_u16(update_failure.state.window, 6U, OP_1025);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.roles[1].action.wait_override == 0x8005U &&
            update_failure.roles[1].action.wait_remaining == 0U &&
            update_failure.context.instruction_offset == 6U &&
            update_failure.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        "opcode 44 refresh failure is diagnostic-only after both word writes"
    );

    Fixture missing;
    missing.roles[0].action.wait_override = 0x1111U;
    missing.roles[1].action.wait_override = 0x2222U;
    missing.roles[2].action.wait_override = 0x3333U;
    prime_loaded_instruction(missing, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x8006U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            missing_result.executed_instruction_count == 1U &&
            missing_result.action_update_count == 0U &&
            missing.roles[0].action.wait_override == 0x1111U &&
            missing.roles[1].action.wait_override == 0x2222U &&
            missing.roles[2].action.wait_override == 0x3333U &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 44 missing role stops at the first unsafe action access"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
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
        "opcode 44 stops at the first unsafe selector-word access"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    write_u16(
        missing_truncated.state.window,
        0x7FFCU,
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    missing_truncated.state.previous_opcode = 0x55U;
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated_result.executed_instruction_count == 1U &&
            missing_truncated_result.action_update_count == 0U &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U,
        "opcode 44 reads the value after lookup and before unsafe role access"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8007U);
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
        "opcode 44 invalid controlled owner stops at the VM session boundary"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].action.wait_remaining = 9U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x8008U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.action_update_count == 1U &&
            exact_tail.roles[1].action.wait_override == 0x8008U &&
            exact_tail.roles[1].action.wait_remaining == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        "opcode 44 exact-tail record completes before the next fetch fails"
    );
}

void test_shared_picture_action_enqueue_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    struct Variant {
        u16 opcode;
        bool primary;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION, true},
        Variant{OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION, false},
    };
    const auto prime_instruction = [](Fixture& fixture, const u16 raw_word) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFEDCU);
        write_u16(fixture.state.window, 4U, 0x8001U);
        write_u16(fixture.state.window, 6U, 0xFFFFU);
        write_u16(fixture.state.window, 8U, 0x8000U);
        fixture.state.previous_opcode = 0x66U;
    };

    for (const Variant variant : variants) {
        for (const u16 mask : alias_masks) {
            Fixture fixture;
            openswd3::world_map::LegacyPictureActionLists picture_actions;
            picture_actions.primary.emplace_back();
            picture_actions.primary.back().screen_x = 0x1111U;
            picture_actions.secondary.emplace_back();
            picture_actions.secondary.back().screen_x = 0x2222U;
            fixture.runtime.picture_actions = &picture_actions;
            prime_instruction(fixture, static_cast<u16>(variant.opcode | mask));

            const auto result = fixture.step();
            const auto& destination = variant.primary
                ? picture_actions.primary
                : picture_actions.secondary;
            const auto& other = variant.primary ? picture_actions.secondary
                                                : picture_actions.primary;
            const auto& node = destination.front();
            const u16 prior_x = variant.primary ? 0x1111U : 0x2222U;
            const u16 other_x = variant.primary ? 0x2222U : 0x1111U;

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == variant.opcode &&
                    result.executed_instruction_count == 1U &&
                    destination.size() == 2U && other.size() == 1U &&
                    std::next(destination.begin())->screen_x == prior_x &&
                    other.front().screen_x == other_x &&
                    node.screen_x == 0xFEDCU && node.screen_y == 0x8001U &&
                    node.field_04 == 0U && node.field_06 == 0U &&
                    node.action.action_id == 0xFFFFU &&
                    node.action.base_variant == 0x8000U &&
                    node.action.field_1c == 0xFFFFFFFFU &&
                    node.action.one_shot_base_variant == 0xFFFFFFFFU &&
                    node.action.one_shot_variant_delta == 0xFFFFFFFFU &&
                    node.action.wait_override == 0U &&
                    node.action.wait_default == 0U &&
                    node.action.wait_remaining == 0U &&
                    node.action.command_cursor == 0U &&
                    node.action.external_mode == 0U &&
                    node.next_pointer_32 == 0U &&
                    fixture.context.instruction_offset == 10U &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.direct_audio_service_count == 1U,
                "opcodes 58 and 153 aliases initialize and prepend the selected picture-action list"
            );
        }
    }

    struct BoundaryCase {
        u16 instruction_offset;
        u32 available_operands;
    };
    constexpr std::array<BoundaryCase, 4U> boundaries{
        BoundaryCase{0x7FFEU, 0U},
        BoundaryCase{0x7FFCU, 1U},
        BoundaryCase{0x7FFAU, 2U},
        BoundaryCase{0x7FF8U, 3U},
    };
    for (const BoundaryCase boundary : boundaries) {
        Fixture fixture;
        openswd3::world_map::LegacyPictureActionLists picture_actions;
        picture_actions.primary.emplace_back();
        picture_actions.secondary.emplace_back();
        fixture.runtime.picture_actions = &picture_actions;
        fixture.context.instruction_offset = boundary.instruction_offset;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        write_u16(
            fixture.state.window,
            boundary.instruction_offset,
            OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION
        );
        for (u32 operand = 0U; operand < boundary.available_operands;
             ++operand) {
            write_u16(
                fixture.state.window,
                static_cast<std::size_t>(boundary.instruction_offset) + 2U +
                    2U * operand,
                static_cast<u16>(0x1000U + operand)
            );
        }

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                result.opcode == OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION &&
                result.executed_instruction_count == 1U &&
                picture_actions.primary.size() == 1U &&
                picture_actions.secondary.size() == 1U &&
                fixture.context.instruction_offset ==
                    boundary.instruction_offset &&
                fixture.state.previous_opcode == 0x66U,
            "shared picture-action handler keeps an incomplete staged node unlinked"
        );
    }

    Fixture missing_owner;
    prime_instruction(missing_owner, OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION);
    const auto owner_result = missing_owner.step();
    test.expect_true(
        owner_result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            owner_result.opcode == OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x66U,
        "shared picture-action handler reaches the list owner only after all operands"
    );

    Fixture exact_tail;
    openswd3::world_map::LegacyPictureActionLists tail_actions;
    exact_tail.runtime.picture_actions = &tail_actions;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FF6U,
        OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION
    );
    write_u16(exact_tail.state.window, 0x7FF8U, 1U);
    write_u16(exact_tail.state.window, 0x7FFAU, 2U);
    write_u16(exact_tail.state.window, 0x7FFCU, 3U);
    write_u16(exact_tail.state.window, 0x7FFEU, 4U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            tail_result.opcode == OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            tail_result.executed_instruction_count == 1U &&
            tail_actions.primary.empty() &&
            tail_actions.secondary.size() == 1U &&
            tail_actions.secondary.front().screen_x == 1U &&
            tail_actions.secondary.front().screen_y == 2U &&
            tail_actions.secondary.front().action.action_id == 3U &&
            tail_actions.secondary.front().action.base_variant == 4U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION,
        "shared picture-action handler exact tail links the node and yields at the window end"
    );
}

void test_request_battle_after_clearing_overlay_lists(
    openswd3::test::Context& test
) {
    Fixture fixture;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows(2U);
    openswd3::world_map::LegacyRoleHeadActionList role_heads(3U);
    u32 battle_request{};
    fixture.runtime.packed_row_effects = &packed_rows;
    fixture.runtime.role_head_actions = &role_heads;
    fixture.runtime.battle_request_value = &battle_request;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 88U);
    write_u16(script, 2U, 0xFFFEU);

    const auto result = fixture.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 88U && result.executed_instruction_count == 1U &&
            fixture.context.instruction_offset == 4U,
        "opcode 88 consumes four bytes and yields immediately"
    );
    test.expect_true(
        packed_rows.empty() && role_heads.empty() &&
            battle_request == 0xFFFFFFFEU,
        "opcode 88 clears only its two overlay owners and tags a sign-extended battle id"
    );
}

void test_play_sound_effect_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<u16, 4U> sound_ids{
        0U,
        1U,
        0x1234U,
        0xFFFFU,
    };
    for (std::size_t index = 0U; index < alias_masks.size(); ++index) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_59_PLAY_SOUND_EFFECT | alias_masks[index])
        );
        write_u16(fixture.state.window, 2U, sound_ids[index]);
        write_u16(fixture.state.window, 4U, OP_14_WAIT_ROLE_ACTION_STATUS);
        write_u16(fixture.state.window, 6U, 0x00F8U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_59_PLAY_SOUND_EFFECT &&
                result.executed_instruction_count == 1U &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{sound_ids[index]} &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.direct_audio_service_count == 1U,
            "opcode 59 aliases submit one u16 sound request, publish previous and audio-yield"
        );
    }

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_59_PLAY_SOUND_EFFECT);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.ports.sound_effect_requests.empty() &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 59 truncated operand stops before the sound request"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_59_PLAY_SOUND_EFFECT);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFFFU);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            tail_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            tail_result.executed_instruction_count == 1U &&
            exact_tail.ports.sound_effect_requests ==
                std::vector<u16>{0xFFFFU} &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT,
        "opcode 59 exact tail completes the request and yields at the window end"
    );
}

void test_shared_scene_render_control_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<u16, 2U> opcodes{
        OP_60_RESUME_WORLD_SCENE_RENDERING,
        OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING,
    };
    for (const u16 opcode : opcodes) {
        for (const u16 alias_mask : alias_masks) {
            Fixture fixture;
            u8 scene_render_flags{0xA5U};
            std::vector<u8> flags_during_clear;
            fixture.runtime.scene_render_flags = &scene_render_flags;
            fixture.ports.framebuffer_clear_callback = [&]() noexcept {
                flags_during_clear.push_back(scene_render_flags);
            };
            prime_loaded_instruction(
                fixture, static_cast<u16>(opcode | alias_mask)
            );
            fixture.state.previous_opcode = 0x66U;

            const auto result = fixture.step();
            const bool suspends =
                opcode == OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING;

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == opcode &&
                    result.executed_instruction_count == 1U &&
                    result.direct_audio_service_count == 1U &&
                    scene_render_flags == (suspends ? 0xA5U : 0xA4U) &&
                    flags_during_clear ==
                        (suspends ? std::vector<u8>{0xA4U}
                                  : std::vector<u8>{}) &&
                    fixture.ports.framebuffer_clear_count ==
                        (suspends ? 1U : 0U) &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.previous_opcode == opcode,
                "shared opcodes 60 and 61 preserve aliases, clear bit zero before the optional framebuffer clear, publish previous and yield"
            );
        }
    }

    for (const u16 opcode : opcodes) {
        Fixture unavailable;
        prime_loaded_instruction(unavailable, opcode);
        unavailable.state.previous_opcode = 0x66U;

        const auto result = unavailable.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                unavailable.ports.framebuffer_clear_count == 0U &&
                unavailable.context.instruction_offset == 0U &&
                unavailable.state.previous_opcode == 0x66U,
            "shared opcodes 60 and 61 typed-stop before effects when the scene flag owner is unavailable"
        );
    }

    for (const u16 opcode : opcodes) {
        Fixture exact_tail;
        u8 scene_render_flags{0xA5U};
        std::vector<u8> flags_during_clear;
        exact_tail.runtime.scene_render_flags = &scene_render_flags;
        exact_tail.ports.framebuffer_clear_callback = [&]() noexcept {
            flags_during_clear.push_back(scene_render_flags);
        };
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(exact_tail.state.window, 0x7FFEU, opcode);

        const auto result = exact_tail.step();
        const bool suspends =
            opcode == OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING;

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                scene_render_flags == (suspends ? 0xA5U : 0xA4U) &&
                flags_during_clear ==
                    (suspends ? std::vector<u8>{0xA4U} : std::vector<u8>{}) &&
                exact_tail.ports.framebuffer_clear_count ==
                    (suspends ? 1U : 0U) &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == opcode,
            "shared opcodes 60 and 61 complete effects and publication at the exact window tail"
        );
    }
}

void test_write_map_role_patch_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        MapRoleWriteHarness harness{fixture};
        harness.add_source({
            .logical_map_id = 1U,
            .guid = 0x2222U,
            .action_id = 2U,
            .base_variant = 3U,
            .variant_delta = 4U,
            .tile_x = 5U,
            .tile_y = 6U,
            .talk_script_id = 7U,
            .path_data_id = 8U,
            .path_word_index = 9,
            .flags = 0x0100U,
        });
        harness.prime(
            static_cast<u16>(OP_62_WRITE_MAP_ROLE | alias_mask),
            0x2222U,
            9U,
            0x0044U,
            7U,
            8U,
            0x0055U,
            0x0066U,
            0x0077U
        );
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        const auto& source = harness.database.role_sources.front();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 67U &&
                result.executed_instruction_count == 2U &&
                result.role_materialization_count == 0U &&
                result.action_update_count == 0U &&
                fixture.context.instruction_offset == 18U &&
                fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK &&
                fixture.roles.size() == 3U && source.logical_map_id == 9U &&
                source.guid == 0x2222U && source.action_id == 0x0055U &&
                source.base_variant == 0x0066U &&
                source.variant_delta == 0x0077U && source.tile_x == 7U &&
                source.tile_y == 8U && source.talk_script_id == 7U &&
                source.path_data_id == 0x0044U && source.path_word_index == 0 &&
                source.flags == 0x0100U,
            "opcode 62 aliases patch the complete MAPS source, skip runtime materialization for another map and fetch the next instruction in the same call"
        );
    }

    Fixture missing;
    MapRoleWriteHarness missing_harness{missing};
    missing_harness.prime(
        OP_62_WRITE_MAP_ROLE, 0x3456U, 9U, 1U, 2U, 3U, 4U, 5U, 6U
    );
    missing.state.previous_opcode = 0x66U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.role_source_patch_failure_count == 1U &&
            missing_result.role_materialization_count == 0U &&
            missing.context.instruction_offset == 18U &&
            missing.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 62 preserves the original diagnostic-only missing-source path and still advances"
    );
}

void test_write_map_role_materialization_protocol(
    openswd3::test::Context& test
) {
    Fixture replaced;
    MapRoleWriteHarness replacement{replaced, 0x00010005U};
    auto& old_role = replaced.roles[1];
    old_role.guid = 0x00F8U;
    old_role.world_x = 4U << 4U;
    old_role.world_y = 4U << 4U;
    old_role.flags = 0x0100C201U;
    old_role.talk_script_id = 0x4444U;
    old_role.action.action_id = 1U;
    old_role.action.field_2c = 1U;
    old_role.action.field_30 = 1U;
    replacement.insert_runtime_role(1U);
    write_u32(
        replacement.surface,
        static_cast<std::size_t>(old_role.map_cell_pointer_32) * sizeof(u32),
        0xFFFFFFFFU
    );
    auto& occupied_slot = replaced.active_object_slots[7U];
    occupied_slot.bytes.fill(0xA5U);
    write_u16(occupied_slot.bytes, 0U, 1U);
    replacement.add_source({
        .logical_map_id = 5U,
        .guid = 0x00F8U,
        .action_id = 9U,
        .base_variant = 10U,
        .variant_delta = 11U,
        .tile_x = 3U,
        .tile_y = 4U,
        .talk_script_id = 0x2222U,
        .path_data_id = 0x3333U,
        .path_word_index = 7,
        .flags = 0x0100U,
    });
    const std::size_t old_cell_offset =
        static_cast<std::size_t>(old_role.map_cell_pointer_32) * sizeof(u32);
    const std::size_t new_cell_offset =
        static_cast<std::size_t>(MapRoleWriteHarness::kMapWidth + 1U) *
        sizeof(u32);
    write_u32(replacement.surface, new_cell_offset, 0x0000A800U);
    auto& replacement_emitters = replacement.particles.emitters();
    for (std::size_t index = 0U; index < replacement_emitters.size(); ++index) {
        replacement_emitters[index].head_token = static_cast<u32>(10U + index);
    }
    replacement_emitters[0].role_selector = 0x7777;
    bool update_observed_order{};
    replaced.ports.action_update_callback = [&](auto& action, const u32) {
        action.field_2c = 1U;
        action.field_30 = 1U;
        update_observed_order = replaced.roles[1].flags == 0x11000201U &&
            std::ranges::all_of(occupied_slot.bytes,
                                [](const u8 value) {
                                    return value == 0xFFU;
                                }) &&
            read_u32(replacement.surface, old_cell_offset) == 0xCF7FFFFFU &&
            replacement.database.role_sources.front().talk_script_id == 0x4444U;
    };
    replacement.prime(
        OP_62_WRITE_MAP_ROLE,
        0xFFF0U,
        0xFFFFU,
        0xFFFFU,
        0xFFFFU,
        0xFFFFU,
        0x0020U,
        0x0030U,
        0x0040U
    );
    replaced.state.previous_opcode = 0x66U;

    const auto replaced_result = replaced.step();
    const auto& replaced_role = replaced.roles[1];
    const auto& replaced_source = replacement.database.role_sources.front();
    const std::size_t replacement_row =
        openswd3::world_map::kLegacySpatialRowPadding + 1U;

    test.expect_true(
        replaced_result.status == LegacyWorldStoryVmStatus::yielded &&
            replaced_result.action_update_count == 1U &&
            replaced_result.action_update_failure_count == 0U &&
            replaced_result.active_object_reset_count == 1U &&
            replaced_result.role_materialization_count == 1U &&
            replaced_result.role_particle_emitter_write_count == 3U &&
            update_observed_order && replaced.roles.size() == 3U &&
            replaced_role.guid == 0x00F8U && replaced_role.world_x == 16U &&
            replaced_role.world_y == 16U &&
            replaced_role.map_cell_pointer_32 == 33U &&
            replaced_role.action.action_id == 0x0020U &&
            replaced_role.action.base_variant == 0x0030U &&
            replaced_role.action.variant_delta == 0x0040U &&
            replaced_role.action.field_1c == 0U &&
            replaced_role.talk_script_id == 0x4444U &&
            replaced_role.path_data_id == 0x3333U &&
            replaced_role.path_word_index == 0U &&
            replaced_role.flags == 0x20A0C301U &&
            replaced_source.logical_map_id == 5U &&
            replaced_source.tile_x == 1U && replaced_source.tile_y == 1U &&
            replaced_source.talk_script_id == 0x4444U &&
            replaced_source.path_data_id == 0x3333U &&
            replaced_source.path_word_index == 7 &&
            replaced_source.flags == 0xC301U &&
            replacement.spatial.row_heads[1U][replacement_row] == 1U &&
            replacement_emitters[0].role_selector == 0x7777 &&
            replacement_emitters[0].head_token == 10U &&
            replacement_emitters[1].role_selector == 0x00F8 &&
            replacement_emitters[2].role_selector == 0x00F8 &&
            replacement_emitters[3].role_selector == 0x00F8 &&
            replacement_emitters[1].world_x == 16 &&
            replacement_emitters[1].world_y == 16 &&
            replacement_emitters[1].head_token == 11U &&
            replaced.context.instruction_offset == 18U &&
            replaced.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 62 cleans the old role, inherits its low flags and talk id, replaces through the spatial chain, then fills every empty particle emitter"
    );

    Fixture appended;
    MapRoleWriteHarness append{appended};
    append.add_source({
        .logical_map_id = 5U,
        .guid = 0x3333U,
        .action_id = 1U,
        .base_variant = 2U,
        .variant_delta = 3U,
        .tile_x = 2U,
        .tile_y = 3U,
        .talk_script_id = 4U,
        .path_data_id = 5U,
        .path_word_index = 6,
        .flags = 0x0202U,
    });
    auto& append_emitters = append.particles.emitters();
    for (std::size_t index = 0U; index < append_emitters.size(); ++index) {
        append_emitters[index].head_token = static_cast<u32>(20U + index);
    }
    appended.ports.action_update_result = 0U;
    appended.ports.action_update_callback = [](auto& action, const u32) {
        action.field_2c = 1U;
        action.field_30 = 1U;
    };
    append.prime(OP_62_WRITE_MAP_ROLE, 0x3333U, 5U, 5U, 2U, 3U, 1U, 2U, 3U);
    const auto appended_result = appended.step();
    const std::size_t append_row =
        openswd3::world_map::kLegacySpatialRowPadding + 3U;

    test.expect_true(
        appended_result.status == LegacyWorldStoryVmStatus::yielded &&
            appended_result.action_update_count == 1U &&
            appended_result.action_update_failure_count == 1U &&
            appended_result.role_materialization_count == 1U &&
            appended_result.role_particle_emitter_write_count == 4U &&
            appended.roles.size() == 4U && appended.roles[3].guid == 0x3333U &&
            appended.roles[3].world_x == 32U &&
            appended.roles[3].world_y == 48U &&
            appended.roles[3].flags == 0x0202U &&
            append.spatial.row_heads[2U][append_row] == 3U &&
            std::ranges::all_of(
                append_emitters,
                [](const auto& emitter) {
                    return emitter.role_selector == 0x3333 &&
                        emitter.world_x == 32 && emitter.world_y == 48;
                }
            ) &&
            append_emitters[0].head_token == 20U &&
            append_emitters[3].head_token == 23U,
        "opcode 62 appends a missing current-map role, ignores action-update failure, inserts it spatially and preserves emitter head links"
    );
}

void test_write_map_role_failure_ordering(openswd3::test::Context& test) {
    Fixture truncated;
    MapRoleWriteHarness truncation{truncated};
    auto& truncated_role = truncated.roles[1];
    truncated_role.guid = 0x00F8U;
    truncated_role.world_x = 2U << 4U;
    truncated_role.world_y = 2U << 4U;
    truncated_role.flags = 0x0100C001U;
    truncated_role.action.field_2c = 1U;
    truncated_role.action.field_30 = 1U;
    truncated_role.map_cell_pointer_32 =
        2U * MapRoleWriteHarness::kMapWidth + 2U;
    const std::size_t truncated_cell_offset =
        static_cast<std::size_t>(truncated_role.map_cell_pointer_32) *
        sizeof(u32);
    write_u32(truncation.surface, truncated_cell_offset, 0xFFFFFFFFU);
    auto& truncated_slot = truncated.active_object_slots[0U];
    truncated_slot.bytes.fill(0xA5U);
    write_u16(truncated_slot.bytes, 0U, 1U);
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFCU, OP_62_WRITE_MAP_ROLE);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);

    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.active_object_reset_count == 1U &&
            std::ranges::all_of(
                truncated_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            truncated.roles[1].flags == 0x11000001U &&
            read_u32(truncation.surface, truncated_cell_offset) ==
                0xCF7FFFFFU &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 62 performs old-role cleanup after the selector read before a truncated operand tail stops"
    );

    Fixture unavailable;
    MapRoleWriteHarness unavailable_runtime{unavailable};
    unavailable.roles[1].guid = 0x00F8U;
    unavailable.roles[1].flags = 0x0000C000U;
    unavailable.roles[1].map_cell_pointer_32 = 0U;
    unavailable.roles[1].action.field_2c = 1U;
    unavailable.roles[1].action.field_30 = 1U;
    unavailable_runtime.add_source({.logical_map_id = 5U, .guid = 0x00F8U});
    unavailable_runtime.prime(
        OP_62_WRITE_MAP_ROLE, 0x00F8U, 5U, 0U, 1U, 1U, 1U, 0U, 0U
    );
    unavailable.runtime.maps_database = nullptr;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.roles[1].flags == 0x10000000U &&
            unavailable.context.instruction_offset == 0U,
        "opcode 62 preserves old-role cleanup before a missing MAPS runtime typed-stop"
    );

    Fixture exact_tail;
    MapRoleWriteHarness tail{exact_tail};
    tail.add_source({.logical_map_id = 1U, .guid = 0x2222U});
    exact_tail.context.instruction_offset = 0x7FEEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    const std::array<u16, 9U> words{
        OP_62_WRITE_MAP_ROLE,
        0x2222U,
        9U,
        0U,
        1U,
        1U,
        1U,
        0U,
        0U,
    };
    for (std::size_t index = 0U; index < words.size(); ++index) {
        write_u16(exact_tail.state.window, 0x7FEEU + index * 2U, words[index]);
    }
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            tail_result.opcode == OP_62_WRITE_MAP_ROLE &&
            tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_62_WRITE_MAP_ROLE &&
            tail.database.role_sources.front().logical_map_id == 9U,
        "opcode 62 completes MAPS publication and previous-opcode publication at the exact window tail"
    );
}

void test_set_selection_scroll_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.selection_words.fill(std::bit_cast<i16>(u16{0x1234U}));
        fixture.selection_scroll = {
            .cursor_word_index = 7U,
            .frames_remaining = 11,
            .frame_interval = 12,
            .saved_left = 13U,
            .saved_top = 14U,
        };
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_63_SET_SELECTION_SCROLL | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 0xFFFFU);
        write_u16(fixture.state.window, 4U, 0x0001U);
        write_u16(fixture.state.window, 6U, 0xFFFEU);
        write_u16(fixture.state.window, 8U, 0x8000U);
        write_u16(fixture.state.window, 10U, 0xFF00U);
        write_u16(fixture.state.window, 12U, 67U);
        write_u16(fixture.state.window, 14U, 0U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step(100, 200);

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 67U &&
                result.executed_instruction_count == 2U &&
                result.selection_overflow_diagnostic_count == 0U &&
                std::bit_cast<u16>(fixture.selection_words[0]) == 0x0001U &&
                std::bit_cast<u16>(fixture.selection_words[1]) == 0xFFFEU &&
                std::bit_cast<u16>(fixture.selection_words[2]) == 0x8000U &&
                std::ranges::all_of(
                    fixture.selection_words.begin() + 3,
                    fixture.selection_words.end(),
                    [](const i16 value) {
                        return std::bit_cast<u16>(value) ==
                            openswd3::world_map::kLegacyWorldSelectionSentinel;
                    }
                ) &&
                fixture.selection_scroll.cursor_word_index == 7U &&
                fixture.selection_scroll.frame_interval == 65535 &&
                fixture.selection_scroll.frames_remaining == 65535 &&
                fixture.selection_scroll.saved_left == 100U &&
                fixture.selection_scroll.saved_top == 200U &&
                fixture.context.instruction_offset == 12U &&
                fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
            "opcode 63 aliases replace the selection table, zero-extend the interval, preserve the cursor, snapshot the viewport and fetch the next instruction"
        );
    }

    Fixture empty;
    empty.selection_words.fill(std::bit_cast<i16>(u16{0x1111U}));
    prime_loaded_instruction(empty, OP_63_SET_SELECTION_SCROLL);
    write_u16(empty.state.window, 2U, 0U);
    write_u16(empty.state.window, 4U, 0xFF00U);
    write_u16(empty.state.window, 6U, 67U);
    write_u16(empty.state.window, 8U, 0U);
    const auto empty_result = empty.step(0x1234, 0x5678);
    test.expect_true(
        empty_result.status == LegacyWorldStoryVmStatus::yielded &&
            empty_result.opcode == 67U &&
            empty_result.executed_instruction_count == 2U &&
            std::ranges::all_of(
                empty.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) ==
                        openswd3::world_map::kLegacyWorldSelectionSentinel;
                }
            ) &&
            empty.selection_scroll.frame_interval == 0 &&
            empty.selection_scroll.frames_remaining == 0 &&
            empty.selection_scroll.saved_left == 0x1234U &&
            empty.selection_scroll.saved_top == 0x5678U &&
            empty.context.instruction_offset == 6U &&
            empty.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 63 accepts an immediate terminator, clears all 64 words and advances six bytes"
    );

    Fixture maximum;
    prime_loaded_instruction(maximum, OP_63_SET_SELECTION_SCROLL);
    write_u16(maximum.state.window, 2U, 1U);
    for (u16 index = 0U; index < 56U; ++index) {
        write_u16(
            maximum.state.window,
            4U + static_cast<std::size_t>(index) * 2U,
            static_cast<u16>(0x1000U + index)
        );
    }
    write_u16(maximum.state.window, 116U, 0xFF00U);
    write_u16(maximum.state.window, 118U, 67U);
    write_u16(maximum.state.window, 120U, 0U);
    const auto maximum_result = maximum.step(10, 20);
    test.expect_true(
        maximum_result.status == LegacyWorldStoryVmStatus::yielded &&
            maximum_result.opcode == 67U &&
            maximum_result.executed_instruction_count == 2U &&
            std::bit_cast<u16>(maximum.selection_words[0]) == 0x1000U &&
            std::bit_cast<u16>(maximum.selection_words[55]) == 0x1037U &&
            std::bit_cast<u16>(maximum.selection_words[56]) ==
                openswd3::world_map::kLegacyWorldSelectionSentinel &&
            maximum.context.instruction_offset == 118U &&
            maximum.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 63 accepts exactly 56 words, retains eight CFCF tail words and continues in the same call"
    );

    Fixture overflow;
    overflow.selection_words.fill(std::bit_cast<i16>(u16{0x2222U}));
    prime_loaded_instruction(overflow, OP_63_SET_SELECTION_SCROLL);
    write_u16(overflow.state.window, 2U, 4U);
    for (u16 index = 0U; index < 57U; ++index) {
        write_u16(
            overflow.state.window,
            4U + static_cast<std::size_t>(index) * 2U,
            index
        );
    }
    write_u16(overflow.state.window, 118U, 0xFF00U);
    overflow.runtime.selection_words = nullptr;
    overflow.runtime.selection_scroll = nullptr;
    overflow.runtime.camera = nullptr;
    overflow.state.previous_opcode = 0x66U;
    const auto overflow_result = overflow.step();
    test.expect_true(
        overflow_result.status == LegacyWorldStoryVmStatus::yielded &&
            overflow_result.opcode == OP_63_SET_SELECTION_SCROLL &&
            overflow_result.executed_instruction_count == 1U &&
            overflow_result.selection_overflow_diagnostic_count == 1U &&
            overflow_result.direct_audio_service_count == 1U &&
            std::ranges::all_of(
                overflow.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) == 0x2222U;
                }
            ) &&
            overflow.context.instruction_offset == 0U &&
            overflow.state.previous_opcode == OP_63_SET_SELECTION_SCROLL,
        "opcode 63 rejects 57 words before every runtime owner, publishes previous and yields on the same instruction"
    );

    Fixture unterminated;
    unterminated.selection_words.fill(std::bit_cast<i16>(u16{0x3333U}));
    unterminated.context.instruction_offset = 0x7FFCU;
    unterminated.context.talk_data_offset = 0x1111U;
    unterminated.state.loaded_file_number = 1U;
    unterminated.state.loaded_data_offset = 0x1111U;
    unterminated.state.window_loaded = true;
    unterminated.state.previous_opcode = 0x66U;
    write_u16(unterminated.state.window, 0x7FFCU, OP_63_SET_SELECTION_SCROLL);
    write_u16(unterminated.state.window, 0x7FFEU, 4U);
    const auto unterminated_result = unterminated.step();
    test.expect_true(
        unterminated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            std::ranges::all_of(
                unterminated.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) == 0x3333U;
                }
            ) &&
            unterminated.context.instruction_offset == 0x7FFCU &&
            unterminated.state.previous_opcode == 0x66U,
        "opcode 63 typed-stops an unterminated scan before clearing the destination or publishing previous"
    );

    Fixture missing_words;
    missing_words.selection_words.fill(std::bit_cast<i16>(u16{0x6666U}));
    missing_words.selection_scroll = {
        .cursor_word_index = 3U,
        .frames_remaining = 4,
        .frame_interval = 5,
        .saved_left = 6U,
        .saved_top = 7U,
    };
    prime_loaded_instruction(missing_words, OP_63_SET_SELECTION_SCROLL);
    write_u16(missing_words.state.window, 2U, 2U);
    write_u16(missing_words.state.window, 4U, 0xFF00U);
    missing_words.runtime.selection_words = nullptr;
    missing_words.state.previous_opcode = 0x66U;
    const auto missing_words_result = missing_words.step(10, 20);
    test.expect_true(
        missing_words_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            std::ranges::all_of(
                missing_words.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) == 0x6666U;
                }
            ) &&
            missing_words.selection_scroll.cursor_word_index == 3U &&
            missing_words.selection_scroll.frame_interval == 5 &&
            missing_words.context.instruction_offset == 0U &&
            missing_words.state.previous_opcode == 0x66U,
        "opcode 63 missing-table typed-stop occurs after terminator validation but before destination clearing"
    );

    Fixture missing_camera;
    missing_camera.selection_words.fill(std::bit_cast<i16>(u16{0x4444U}));
    prime_loaded_instruction(missing_camera, OP_63_SET_SELECTION_SCROLL);
    write_u16(missing_camera.state.window, 2U, 2U);
    write_u16(missing_camera.state.window, 4U, 0x1111U);
    write_u16(missing_camera.state.window, 6U, 0x2222U);
    write_u16(missing_camera.state.window, 8U, 0x3333U);
    write_u16(missing_camera.state.window, 10U, 0xFF00U);
    missing_camera.runtime.camera = nullptr;
    missing_camera.state.previous_opcode = 0x66U;
    const auto missing_camera_result = missing_camera.step();
    test.expect_true(
        missing_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            std::bit_cast<u16>(missing_camera.selection_words[0]) == 0x1111U &&
            std::bit_cast<u16>(missing_camera.selection_words[1]) == 0x2222U &&
            std::bit_cast<u16>(missing_camera.selection_words[2]) ==
                openswd3::world_map::kLegacyWorldSelectionSentinel &&
            missing_camera.selection_scroll.frame_interval == 0 &&
            missing_camera.context.instruction_offset == 0U &&
            missing_camera.state.previous_opcode == 0x66U,
        "opcode 63 missing-camera typed-stop preserves the prior fill and paired-word copy but not the odd tail"
    );

    Fixture missing_scroll;
    missing_scroll.selection_words.fill(std::bit_cast<i16>(u16{0x5555U}));
    prime_loaded_instruction(missing_scroll, OP_63_SET_SELECTION_SCROLL);
    write_u16(missing_scroll.state.window, 2U, 2U);
    write_u16(missing_scroll.state.window, 4U, 0x1111U);
    write_u16(missing_scroll.state.window, 6U, 0x2222U);
    write_u16(missing_scroll.state.window, 8U, 0x3333U);
    write_u16(missing_scroll.state.window, 10U, 0xFF00U);
    missing_scroll.runtime.selection_scroll = nullptr;
    missing_scroll.state.previous_opcode = 0x66U;
    const auto missing_scroll_result = missing_scroll.step(10, 20);
    test.expect_true(
        missing_scroll_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            std::bit_cast<u16>(missing_scroll.selection_words[0]) == 0x1111U &&
            std::bit_cast<u16>(missing_scroll.selection_words[1]) == 0x2222U &&
            std::bit_cast<u16>(missing_scroll.selection_words[2]) == 0x3333U &&
            missing_scroll.context.instruction_offset == 0U &&
            missing_scroll.state.previous_opcode == 0x66U,
        "opcode 63 missing-scroll typed-stop preserves the complete word copy after reading the camera top"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF8U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FF8U, OP_63_SET_SELECTION_SCROLL);
    write_u16(exact_tail.state.window, 0x7FFAU, 3U);
    write_u16(exact_tail.state.window, 0x7FFCU, 0xFFFEU);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFF00U);
    const auto exact_tail_result = exact_tail.step(30, 40);
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.opcode == OP_63_SET_SELECTION_SCROLL &&
            exact_tail_result.executed_instruction_count == 1U &&
            std::bit_cast<u16>(exact_tail.selection_words[0]) == 0xFFFEU &&
            std::bit_cast<u16>(exact_tail.selection_words[1]) ==
                openswd3::world_map::kLegacyWorldSelectionSentinel &&
            exact_tail.selection_scroll.frame_interval == 3 &&
            exact_tail.selection_scroll.frames_remaining == 3 &&
            exact_tail.selection_scroll.saved_left == 30U &&
            exact_tail.selection_scroll.saved_top == 40U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_63_SET_SELECTION_SCROLL,
        "opcode 63 exact tail completes table, timing, viewport and previous publication before the next fetch fails"
    );
}

void test_clear_selection_scroll_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.selection_words.size();
             ++index) {
            fixture.selection_words[index] =
                std::bit_cast<i16>(static_cast<u16>(index));
        }
        fixture.selection_scroll = {
            .cursor_word_index = 7U,
            .frames_remaining = 8,
            .frame_interval = 9,
            .saved_left = 10U,
            .saved_top = 11U,
        };
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_64_CLEAR_SELECTION_SCROLL | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 67U);
        write_u16(fixture.state.window, 4U, 0U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step(100, 200);

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 67U &&
                result.executed_instruction_count == 2U &&
                std::ranges::all_of(
                    fixture.selection_words,
                    [](const i16 value) {
                        return std::bit_cast<u16>(value) ==
                            openswd3::world_map::kLegacyWorldSelectionSentinel;
                    }
                ) &&
                fixture.selection_scroll.cursor_word_index == 7U &&
                fixture.selection_scroll.frames_remaining == 8 &&
                fixture.selection_scroll.frame_interval == 9 &&
                fixture.selection_scroll.saved_left == 10U &&
                fixture.selection_scroll.saved_top == 11U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
            "opcode 64 aliases clear all 64 selection words, preserve timing/cursor/snapshot state and fetch the next instruction"
        );
    }

    Fixture unavailable;
    unavailable.selection_words.fill(std::bit_cast<i16>(u16{0x1234U}));
    unavailable.selection_scroll = {
        .cursor_word_index = 1U,
        .frames_remaining = 2,
        .frame_interval = 3,
        .saved_left = 4U,
        .saved_top = 5U,
    };
    prime_loaded_instruction(unavailable, OP_64_CLEAR_SELECTION_SCROLL);
    unavailable.runtime.selection_words = nullptr;
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step(100, 200);
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            std::ranges::all_of(
                unavailable.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) == 0x1234U;
                }
            ) &&
            unavailable.selection_scroll.cursor_word_index == 1U &&
            unavailable.selection_scroll.frames_remaining == 2 &&
            unavailable.selection_scroll.frame_interval == 3 &&
            unavailable.selection_scroll.saved_left == 4U &&
            unavailable.selection_scroll.saved_top == 5U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 64 missing-table typed-stop occurs before every effect and publication"
    );

    Fixture exact_tail;
    exact_tail.selection_words.fill(std::bit_cast<i16>(u16{0x5678U}));
    exact_tail.selection_scroll = {
        .cursor_word_index = 6U,
        .frames_remaining = 7,
        .frame_interval = 8,
        .saved_left = 9U,
        .saved_top = 10U,
    };
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFEU, OP_64_CLEAR_SELECTION_SCROLL);
    const auto exact_tail_result = exact_tail.step(100, 200);
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.opcode == OP_64_CLEAR_SELECTION_SCROLL &&
            exact_tail_result.executed_instruction_count == 1U &&
            std::ranges::all_of(
                exact_tail.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) ==
                        openswd3::world_map::kLegacyWorldSelectionSentinel;
                }
            ) &&
            exact_tail.selection_scroll.cursor_word_index == 6U &&
            exact_tail.selection_scroll.frames_remaining == 7 &&
            exact_tail.selection_scroll.frame_interval == 8 &&
            exact_tail.selection_scroll.saved_left == 9U &&
            exact_tail.selection_scroll.saved_top == 10U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_64_CLEAR_SELECTION_SCROLL,
        "opcode 64 exact tail clears the table and publishes previous before the next fetch fails"
    );
}

void test_transfer_role_to_party_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.roles[1].guid = 9U;
        fixture.roles[1].flags = 0x00004082U;
        fixture.roles[1].talk_script_id = 0x0033U;
        fixture.roles[1].path_data_id = 0U;
        fixture.live_party_object_slots[1].bytes.fill(0x44U);
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_65_TRANSFER_ROLE_TO_PARTY | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 9U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_65_TRANSFER_ROLE_TO_PARTY &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                result.role_transfer_status ==
                    openswd3::world_map::LegacyWorldRoleTransferStatus::ready &&
                fixture.role_transfer_state.party_role_count == 2U &&
                fixture.role_transfer_state.party_role_indices[1] == 1U &&
                fixture.role_transfer_state.roles_transferred == 1U &&
                fixture.live_party_role_count == 2U &&
                std::ranges::all_of(
                    fixture.live_party_object_slots[1].bytes,
                    [](const u8 value) { return value == 0xFFU; }
                ) &&
                fixture.roles[1].talk_script_id == 0U &&
                fixture.roles[1].flags == 0x00000082U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_65_TRANSFER_ROLE_TO_PARTY,
            "opcode 65 aliases append a pathless role to transfer bookkeeping, apply Talk/flag state, publish previous and yield"
        );
    }

    Fixture raw_fff0;
    raw_fff0.context.source_guid = 9U;
    raw_fff0.roles[1].guid = 9U;
    raw_fff0.runtime.role_transfer_state = nullptr;
    prime_loaded_instruction(raw_fff0, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(raw_fff0.state.window, 2U, 0xFFF0U);
    raw_fff0.state.previous_opcode = 0x66U;
    const auto raw_fff0_result = raw_fff0.step();
    test.expect_true(
        raw_fff0_result.status == LegacyWorldStoryVmStatus::yielded &&
            raw_fff0.role_transfer_state.party_role_count == 1U &&
            raw_fff0.roles[1].talk_script_id == 0U &&
            raw_fff0.context.instruction_offset == 4U &&
            raw_fff0.state.previous_opcode == OP_65_TRANSFER_ROLE_TO_PARTY,
        "opcode 65 does not translate FFF0 to the Talk source GUID and silently consumes a missing literal selector"
    );

    Fixture controlled;
    controlled.roles[1].guid = 9U;
    controlled.roles[1].flags = 0x00004001U;
    controlled.roles[1].talk_script_id = 0x0044U;
    controlled.roles[1].path_data_id = 0U;
    prime_loaded_instruction(controlled, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled.role_transfer_state.party_role_indices[1] == 1U &&
            controlled.live_party_role_count == 2U &&
            controlled.roles[1].talk_script_id == 0U &&
            controlled.roles[1].flags == 0x00000081U,
        "opcode 65 retains the shared FFFE controlled-role lookup contract"
    );

    Fixture aligned;
    MapRoleWriteHarness aligned_runtime{aligned};
    aligned.roles[1].guid = 9U;
    aligned.roles[1].flags = 0x00004080U;
    aligned.roles[1].talk_script_id = 0x0033U;
    aligned.roles[1].path_data_id = 4U;
    aligned.roles[1].world_x = 0x20U;
    aligned.roles[1].world_y = 0x30U;
    aligned_runtime.add_source(
        {.logical_map_id = 5U, .guid = 9U, .flags = 0x0100U}
    );
    auto& aligned_slot = aligned.active_object_slots[5U];
    aligned_slot.bytes.fill(0x55U);
    aligned.live_party_object_slots[1].bytes.fill(0x44U);
    write_u16(aligned_slot.bytes, 0U, 1U);
    prime_loaded_instruction(aligned, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(aligned.state.window, 2U, 9U);

    const auto aligned_result = aligned.step();
    test.expect_true(
        aligned_result.status == LegacyWorldStoryVmStatus::yielded &&
            aligned_result.role_transfer_status ==
                openswd3::world_map::LegacyWorldRoleTransferStatus::ready &&
            aligned_runtime.database.role_sources.front().flags == 0x0180U &&
            std::ranges::all_of(
                aligned_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            aligned.role_transfer_state.active_object_slots_reset == 1U &&
            aligned.role_transfer_state.party_role_indices[1] == 1U &&
            aligned.live_party_role_count == 2U &&
            std::ranges::all_of(
                aligned.live_party_object_slots[1].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            aligned.roles[1].talk_script_id == 0U &&
            aligned.roles[1].flags == 0x00000080U,
        "opcode 65 wires aligned active-object MAPS patch, full slot reset and final transfer append through the shared helper"
    );

    Fixture missing_maps;
    MapRoleWriteHarness missing_maps_runtime{missing_maps};
    missing_maps.roles[1].guid = 9U;
    missing_maps.roles[1].flags = 0x00004080U;
    missing_maps.roles[1].talk_script_id = 0x0033U;
    missing_maps.roles[1].path_data_id = 4U;
    missing_maps.roles[1].world_x = 0x20U;
    missing_maps.roles[1].world_y = 0x30U;
    missing_maps_runtime.add_source(
        {.logical_map_id = 5U, .guid = 9U, .flags = 0x0100U}
    );
    auto& missing_maps_slot = missing_maps.active_object_slots[0U];
    missing_maps_slot.bytes.fill(0x55U);
    write_u16(missing_maps_slot.bytes, 0U, 1U);
    prime_loaded_instruction(missing_maps, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(missing_maps.state.window, 2U, 9U);
    missing_maps.runtime.maps_database = nullptr;
    missing_maps.state.previous_opcode = 0x66U;

    const auto missing_maps_result = missing_maps.step();
    test.expect_true(
        missing_maps_result.status ==
                LegacyWorldStoryVmStatus::role_transfer_failed &&
            missing_maps_result.role_transfer_status ==
                openswd3::world_map::LegacyWorldRoleTransferStatus::
                    role_source_patch_failed &&
            missing_maps_runtime.database.role_sources.front().flags ==
                0x0100U &&
            missing_maps_slot.bytes[0] == 1U &&
            missing_maps.role_transfer_state.active_object_slots_reset == 0U &&
            missing_maps.role_transfer_state.party_role_count == 1U &&
            missing_maps.live_party_role_count == 1U &&
            missing_maps.roles[1].talk_script_id == 0x0033U &&
            missing_maps.roles[1].flags == 0x00004080U &&
            missing_maps.context.instruction_offset == 0U &&
            missing_maps.state.previous_opcode == 0x66U,
        "opcode 65 nullable MAPS owner failure occurs at the patch point before object reset, transfer append, IP and previous publication"
    );

    Fixture missing_live;
    missing_live.roles[1].guid = 9U;
    missing_live.roles[1].flags = 0x00004080U;
    missing_live.roles[1].talk_script_id = 0x0033U;
    missing_live.roles[1].path_data_id = 0U;
    missing_live.live_party_object_slots[1].bytes.fill(0x44U);
    prime_loaded_instruction(missing_live, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(missing_live.state.window, 2U, 9U);
    missing_live.runtime.live_party_object_slots = nullptr;
    missing_live.runtime.live_party_role_count = nullptr;
    missing_live.state.previous_opcode = 0x66U;
    const auto missing_live_result = missing_live.step();
    test.expect_true(
        missing_live_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_live_result.role_transfer_status ==
                openswd3::world_map::LegacyWorldRoleTransferStatus::ready &&
            missing_live.role_transfer_state.party_role_count == 2U &&
            missing_live.role_transfer_state.party_role_indices[1] == 1U &&
            missing_live.role_transfer_state.roles_transferred == 1U &&
            missing_live.live_party_role_count == 1U &&
            missing_live.live_party_object_slots[1].bytes[0] == 0x44U &&
            missing_live.roles[1].talk_script_id == 0U &&
            missing_live.roles[1].flags == 0x00000080U &&
            missing_live.context.instruction_offset == 0U &&
            missing_live.state.previous_opcode == 0x66U,
        "opcode 65 missing live-party slots typed-stops after helper effects but before live publication, IP and previous"
    );

    Fixture full_party;
    full_party.roles[1].guid = 9U;
    full_party.roles[1].flags = 0x00004080U;
    full_party.roles[1].talk_script_id = 0x0033U;
    full_party.roles[1].path_data_id = 0U;
    full_party.role_transfer_state.party_role_count = 8U;
    prime_loaded_instruction(full_party, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(full_party.state.window, 2U, 9U);
    full_party.state.previous_opcode = 0x66U;
    const auto full_party_result = full_party.step();
    test.expect_true(
        full_party_result.status ==
                LegacyWorldStoryVmStatus::role_transfer_failed &&
            full_party_result.role_transfer_status ==
                openswd3::world_map::LegacyWorldRoleTransferStatus::
                    party_capacity_exceeded &&
            full_party.roles[1].talk_script_id == 0x0033U &&
            full_party.roles[1].flags == 0x00004080U &&
            full_party.live_party_role_count == 1U &&
            full_party.context.instruction_offset == 0U &&
            full_party.state.previous_opcode == 0x66U,
        "opcode 65 full-party typed-stop preserves common role state and publication"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_65_TRANSFER_ROLE_TO_PARTY);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.role_transfer_state.party_role_count == 1U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 65 truncated selector typed-stops before lookup and transfer state"
    );

    Fixture exact_tail;
    exact_tail.runtime.role_transfer_state = nullptr;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_65_TRANSFER_ROLE_TO_PARTY);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x7777U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_65_TRANSFER_ROLE_TO_PARTY &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_65_TRANSFER_ROLE_TO_PARTY,
        "opcode 65 missing-role exact tail consumes, publishes previous and yields without another fetch"
    );
}

void test_update_role_map_state_protocol(openswd3::test::Context& test) {
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 selector,
                          const u16 path,
                          const u16 talk,
                          const u16 action,
                          const u16 base,
                          const u16 variant,
                          const u16 flags) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, path);
        write_u16(fixture.state.window, 6U, talk);
        write_u16(fixture.state.window, 8U, action);
        write_u16(fixture.state.window, 10U, base);
        write_u16(fixture.state.window, 12U, variant);
        write_u16(fixture.state.window, 14U, flags);
    };
    const auto add_source = [](MapRoleWriteHarness& runtime, const u16 guid) {
        runtime.add_source({
            .logical_map_id = 81U,
            .guid = guid,
            .action_id = 100U,
            .base_variant = 101U,
            .variant_delta = 102U,
            .tile_x = 2U,
            .tile_y = 2U,
            .talk_script_id = 103U,
            .path_data_id = 104U,
            .path_word_index = 7,
            .flags = 0x0181U,
        });
    };
    const auto prepare_party = [](Fixture& fixture) {
        fixture.role_transfer_state.party_role_count = 5U;
        fixture.live_party_role_count = 5U;
        fixture.role_transfer_state.party_role_indices = {
            10U,
            11U,
            1U,
            13U,
            14U,
            15U,
            16U,
            17U,
        };
        for (std::size_t index = 0U;
             index < fixture.live_party_object_slots.size();
             ++index) {
            fixture.live_party_object_slots[index].bytes.fill(
                static_cast<u8>(index)
            );
            write_u16(
                fixture.live_party_object_slots[index].bytes, 2U, 0x7FFFU
            );
        }
    };

    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        MapRoleWriteHarness runtime{fixture};
        fixture.roles[1].guid = 8U;
        add_source(runtime, 9U);
        fixture.runtime.role_transfer_state = nullptr;
        fixture.runtime.live_party_object_slots = nullptr;
        fixture.runtime.live_party_role_count = nullptr;
        prime(
            fixture,
            static_cast<u16>(OP_66_UPDATE_ROLE_MAP_STATE | alias_mask),
            9U,
            0x8001U,
            0x8002U,
            0x8003U,
            0x8004U,
            0x8005U,
            0xC000U
        );
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        const auto& source = runtime.database.role_sources.front();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_66_UPDATE_ROLE_MAP_STATE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                result.role_map_update.status ==
                    openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                        ready &&
                !result.role_map_update.runtime_role_found &&
                result.role_map_update.maps_source_patched &&
                source.flags == 0x0101U && source.path_data_id == 104U &&
                source.talk_script_id == 103U &&
                fixture.context.instruction_offset == 16U &&
                fixture.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
            "opcode 66 aliases patch only missing-role MAPS bit seven without party owners, publish previous and yield"
        );
    }

    Fixture raw_fff0;
    MapRoleWriteHarness raw_fff0_runtime{raw_fff0};
    raw_fff0.context.source_guid = 9U;
    raw_fff0.roles[1].guid = 9U;
    add_source(raw_fff0_runtime, 0xFFF0U);
    raw_fff0.runtime.role_transfer_state = nullptr;
    raw_fff0.runtime.live_party_object_slots = nullptr;
    raw_fff0.runtime.live_party_role_count = nullptr;
    prime(
        raw_fff0, OP_66_UPDATE_ROLE_MAP_STATE, 0xFFF0U, 1U, 2U, 3U, 4U, 5U, 6U
    );
    const auto raw_fff0_result = raw_fff0.step();
    test.expect_true(
        raw_fff0_result.status == LegacyWorldStoryVmStatus::yielded &&
            !raw_fff0_result.role_map_update.runtime_role_found &&
            raw_fff0_runtime.database.role_sources.front().flags == 0x0101U &&
            raw_fff0.roles[1].guid == 9U,
        "opcode 66 does not translate FFF0 and patches the literal MAPS GUID"
    );

    Fixture controlled;
    MapRoleWriteHarness controlled_runtime{controlled};
    controlled.roles[1].guid = 9U;
    controlled.roles[1].world_x = 0x20U;
    controlled.roles[1].world_y = 0x20U;
    controlled.roles[1].map_cell_pointer_32 = 2U;
    controlled.roles[1].flags = 0x80U;
    controlled.roles[1].action.field_2c = 1U;
    controlled.roles[1].action.field_30 = 1U;
    add_source(controlled_runtime, 9U);
    controlled.role_transfer_state.party_role_count = 2U;
    controlled.live_party_role_count = 2U;
    controlled.role_transfer_state.party_role_indices = {
        10U,
        1U,
        12U,
        13U,
        14U,
        15U,
        16U,
        17U,
    };
    for (auto& object : controlled.live_party_object_slots) {
        write_u16(object.bytes, 2U, 0x7FFFU);
    }
    prime(
        controlled, OP_66_UPDATE_ROLE_MAP_STATE, 0xFFFEU, 1U, 2U, 3U, 4U, 5U, 6U
    );
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled_result.role_map_update.runtime_role_found &&
            controlled_result.role_map_update.resolved_role_index == 1U &&
            controlled_result.role_map_update.party_role_removed &&
            controlled.role_transfer_state.party_role_count == 1U &&
            controlled.live_party_role_count == 1U,
        "opcode 66 independently retains the FFFE controlled-role lookup contract"
    );

    Fixture aligned;
    MapRoleWriteHarness aligned_runtime{aligned};
    aligned.roles[1].guid = 9U;
    aligned.roles[1].world_x = 0x20U;
    aligned.roles[1].world_y = 0x20U;
    aligned.roles[1].map_cell_pointer_32 = 2U;
    aligned.roles[1].flags = 0x80U;
    aligned.roles[1].action.field_2c = 1U;
    aligned.roles[1].action.field_30 = 1U;
    add_source(aligned_runtime, 9U);
    prepare_party(aligned);
    prime(
        aligned,
        OP_66_UPDATE_ROLE_MAP_STATE,
        9U,
        0x8001U,
        0x8002U,
        0x8003U,
        0x8004U,
        0x8005U,
        0xC000U
    );

    const auto aligned_result = aligned.step();
    const auto& aligned_role = aligned.roles[1];
    const auto& aligned_source = aligned_runtime.database.role_sources.front();
    test.expect_true(
        aligned_result.status == LegacyWorldStoryVmStatus::yielded &&
            aligned_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::ready &&
            aligned_result.role_map_update.runtime_role_found &&
            aligned_result.role_map_update.physical_party_index == 2U &&
            aligned_result.role_map_update.party_role_removed &&
            aligned.role_transfer_state.party_role_count == 4U &&
            aligned.live_party_role_count == 4U &&
            aligned.role_transfer_state.party_role_indices ==
                std::array<u32, 8U>{10U, 11U, 13U, 14U, 15U, 16U, 17U, 17U} &&
            aligned.live_party_object_slots[2].bytes[0] == 3U &&
            aligned.role_transfer_state.party_object_slots[2].bytes[0] == 3U &&
            aligned_role.path_data_id == 0x8001U &&
            aligned_role.path_word_index == 0U &&
            aligned_role.talk_script_id == 0x8002U &&
            aligned_role.action.action_id == 0x8003U &&
            aligned_role.action.base_variant == 0x8004U &&
            aligned_role.action.variant_delta == 0x8005U &&
            aligned_role.flags == 0xC000U && aligned_source.action_id == 100U &&
            aligned_source.base_variant == 101U &&
            aligned_source.variant_delta == 102U &&
            aligned_source.talk_script_id == 0x8002U &&
            aligned_source.path_data_id == 0x8001U &&
            aligned_source.path_word_index == 0 &&
            aligned_source.flags == 0x0101U &&
            read_u32(aligned_runtime.surface, 2U * sizeof(u32)) ==
                0x30000000U &&
            aligned.context.instruction_offset == 16U &&
            aligned.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
        "opcode 66 zero-extends all seven operands, updates role/MAPS/surface, removes the physical party slot and synchronizes post/live owners"
    );

    Fixture not_party;
    MapRoleWriteHarness not_party_runtime{not_party};
    not_party.roles[1].guid = 9U;
    not_party.roles[1].flags = 0x80U;
    add_source(not_party_runtime, 9U);
    not_party.role_transfer_state.party_role_count = 5U;
    not_party.live_party_role_count = 5U;
    not_party.role_transfer_state.party_role_indices.fill(99U);
    prime(not_party, OP_66_UPDATE_ROLE_MAP_STATE, 9U, 1U, 2U, 3U, 4U, 5U, 6U);
    const auto not_party_result = not_party.step();
    test.expect_true(
        not_party_result.status == LegacyWorldStoryVmStatus::yielded &&
            not_party_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                    active_role_not_in_physical_party &&
            !not_party_result.role_map_update.party_role_removed &&
            not_party.role_transfer_state.party_role_count == 5U &&
            not_party.live_party_role_count == 5U &&
            not_party.context.instruction_offset == 16U &&
            not_party.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
        "opcode 66 consumes the helper diagnostic when an active role is absent from all eight physical party slots"
    );

    Fixture missing_source;
    MapRoleWriteHarness missing_source_runtime{missing_source};
    missing_source.roles[1].guid = 9U;
    missing_source.roles[1].world_x = 0x20U;
    missing_source.roles[1].world_y = 0x20U;
    missing_source.roles[1].map_cell_pointer_32 = 2U;
    missing_source.roles[1].flags = 0x80U;
    missing_source.roles[1].action.field_2c = 1U;
    missing_source.roles[1].action.field_30 = 1U;
    prepare_party(missing_source);
    prime(
        missing_source,
        OP_66_UPDATE_ROLE_MAP_STATE,
        9U,
        0x8001U,
        0x8002U,
        0x8003U,
        0x8004U,
        0x8005U,
        0xC000U
    );
    const auto missing_source_result = missing_source.step();
    test.expect_true(
        missing_source_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_source_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                    maps_patch_failed &&
            missing_source_result.role_map_update.party_role_removed &&
            missing_source.role_transfer_state.party_role_count == 4U &&
            missing_source.live_party_role_count == 4U &&
            missing_source.roles[1].path_data_id == 0x8001U &&
            missing_source.context.instruction_offset == 16U &&
            missing_source.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
        "opcode 66 ignores MAPS patch diagnostics after preserving all later runtime and party-removal side effects"
    );

    Fixture spatial_miss;
    MapRoleWriteHarness spatial_miss_runtime{spatial_miss};
    spatial_miss.roles[1].guid = 9U;
    spatial_miss.roles[1].world_x = 0x24U;
    spatial_miss.roles[1].world_y = 0x20U;
    spatial_miss.roles[1].map_cell_pointer_32 = 0U;
    spatial_miss.roles[1].flags = 0x80U;
    spatial_miss.roles[1].action.field_2c = 1U;
    spatial_miss.roles[1].action.field_30 = 1U;
    add_source(spatial_miss_runtime, 9U);
    prepare_party(spatial_miss);
    write_u16(spatial_miss.live_party_object_slots[2].bytes, 2U, 0U);
    spatial_miss.live_party_object_slots[2].bytes[0x1CU] = 7U;
    prime(
        spatial_miss, OP_66_UPDATE_ROLE_MAP_STATE, 9U, 1U, 2U, 3U, 4U, 5U, 6U
    );
    const auto spatial_miss_result = spatial_miss.step();
    test.expect_true(
        spatial_miss_result.status == LegacyWorldStoryVmStatus::yielded &&
            spatial_miss_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                    role_spatial_relocation_failed &&
            spatial_miss_result.role_map_update.party_role_removed &&
            spatial_miss.role_transfer_state.party_role_count == 4U &&
            spatial_miss.live_party_role_count == 4U &&
            spatial_miss.context.instruction_offset == 16U,
        "opcode 66 consumes an ignored spatial miss after the helper completes later MAPS, surface and party effects"
    );

    Fixture bad_direction;
    MapRoleWriteHarness bad_direction_runtime{bad_direction};
    bad_direction.roles[1].guid = 9U;
    bad_direction.roles[1].world_x = 0x24U;
    bad_direction.roles[1].world_y = 0x20U;
    bad_direction.roles[1].map_cell_pointer_32 = 0U;
    bad_direction.roles[1].flags = 0x80U;
    bad_direction.roles[1].action.field_2c = 1U;
    bad_direction.roles[1].action.field_30 = 1U;
    add_source(bad_direction_runtime, 9U);
    prepare_party(bad_direction);
    bad_direction_runtime.surface.assign(
        bad_direction_runtime.surface.size(), 0xFFU
    );
    write_u16(bad_direction.live_party_object_slots[2].bytes, 2U, 0U);
    bad_direction.live_party_object_slots[2].bytes[0x1CU] = 8U;
    prime(
        bad_direction, OP_66_UPDATE_ROLE_MAP_STATE, 9U, 1U, 2U, 3U, 4U, 5U, 6U
    );
    bad_direction.state.previous_opcode = 0x66U;
    const auto bad_direction_result = bad_direction.step();
    test.expect_true(
        bad_direction_result.status ==
                LegacyWorldStoryVmStatus::role_map_update_failed &&
            bad_direction_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                    path_direction_out_of_range &&
            bad_direction_result.role_map_update.role_surface_cleared &&
            bad_direction.role_transfer_state.party_role_count == 5U &&
            bad_direction.live_party_role_count == 5U &&
            read_u32(bad_direction_runtime.surface, 0U) == 0xCF7FFFFFU &&
            bad_direction.context.instruction_offset == 0U &&
            bad_direction.state.previous_opcode == 0x66U,
        "opcode 66 invalid-direction typed-stop preserves the original prior surface clear and prevents publication"
    );

    Fixture missing_live_count;
    MapRoleWriteHarness missing_live_runtime{missing_live_count};
    missing_live_count.roles[1].guid = 9U;
    missing_live_count.roles[1].world_x = 0x20U;
    missing_live_count.roles[1].world_y = 0x20U;
    missing_live_count.roles[1].map_cell_pointer_32 = 2U;
    missing_live_count.roles[1].flags = 0x80U;
    missing_live_count.roles[1].action.field_2c = 1U;
    missing_live_count.roles[1].action.field_30 = 1U;
    add_source(missing_live_runtime, 9U);
    prepare_party(missing_live_count);
    prime(
        missing_live_count,
        OP_66_UPDATE_ROLE_MAP_STATE,
        9U,
        1U,
        2U,
        3U,
        4U,
        5U,
        6U
    );
    missing_live_count.runtime.live_party_role_count = nullptr;
    missing_live_count.state.previous_opcode = 0x66U;
    const auto missing_live_result = missing_live_count.step();
    test.expect_true(
        missing_live_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_live_result.role_map_update.party_role_removed &&
            missing_live_count.role_transfer_state.party_role_count == 4U &&
            missing_live_count.live_party_role_count == 5U &&
            missing_live_count.context.instruction_offset == 0U &&
            missing_live_count.state.previous_opcode == 0x66U,
        "opcode 66 missing live-count owner typed-stops after helper and post-slot synchronization but before publication"
    );

    Fixture missing_runtime;
    missing_runtime.runtime.maps_database = nullptr;
    missing_runtime.runtime.role_transfer_state = nullptr;
    missing_runtime.runtime.live_party_object_slots = nullptr;
    missing_runtime.runtime.live_party_role_count = nullptr;
    prime(
        missing_runtime, OP_66_UPDATE_ROLE_MAP_STATE, 9U, 1U, 2U, 3U, 4U, 5U, 6U
    );
    missing_runtime.state.previous_opcode = 0x66U;
    const auto missing_runtime_result = missing_runtime.step();
    test.expect_true(
        missing_runtime_result.status ==
                LegacyWorldStoryVmStatus::role_map_update_failed &&
            missing_runtime_result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::
                    maps_runtime_required &&
            missing_runtime.context.instruction_offset == 0U &&
            missing_runtime.state.previous_opcode == 0x66U,
        "opcode 66 missing MAPS runtime typed-stops before lookup effects and publication"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FF2U;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FF2U, OP_66_UPDATE_ROLE_MAP_STATE);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FF2U &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 66 truncated 16-byte record typed-stops before all helper effects"
    );

    Fixture exact_tail;
    MapRoleWriteHarness exact_tail_runtime{exact_tail};
    exact_tail.roles[1].guid = 8U;
    add_source(exact_tail_runtime, 9U);
    exact_tail.runtime.role_transfer_state = nullptr;
    exact_tail.runtime.live_party_object_slots = nullptr;
    exact_tail.runtime.live_party_role_count = nullptr;
    exact_tail.context.instruction_offset = 0x7FF0U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    const std::array<u16, 8U> tail_words{
        OP_66_UPDATE_ROLE_MAP_STATE,
        9U,
        1U,
        2U,
        3U,
        4U,
        5U,
        6U,
    };
    for (std::size_t index = 0U; index < tail_words.size(); ++index) {
        write_u16(
            exact_tail.state.window, 0x7FF0U + index * 2U, tail_words[index]
        );
    }
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_66_UPDATE_ROLE_MAP_STATE &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_runtime.database.role_sources.front().flags == 0x0101U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
        "opcode 66 exact tail completes MAPS fallback and publication then yields without another fetch"
    );
}

void test_frame_clock_wait_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_67_WAIT_FRAME_CLOCK | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        fixture.runtime.current_tick = 0xABCDEF01U;
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_67_WAIT_FRAME_CLOCK &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.wait_duration == 0x1234U &&
                fixture.state.wait_started_at == 0xABCDEF01U &&
                read_u16(fixture.state.window, 2U) == 0x9234U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
            "opcode 67 aliases snapshot duration/start, set script bit15, publish previous and yield in place"
        );
    }

    Fixture strict;
    prime_loaded_instruction(strict, OP_67_WAIT_FRAME_CLOCK);
    write_u16(strict.state.window, 2U, 0x800AU);
    write_u16(strict.state.window, 4U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(strict.state.window, 6U, 1U);
    strict.state.wait_duration = 10U;
    strict.state.wait_started_at = 100U;
    strict.runtime.current_tick = 110U;
    strict.state.previous_opcode = 0x66U;
    const auto equal_result = strict.step();
    test.expect_true(
        equal_result.status == LegacyWorldStoryVmStatus::yielded &&
            equal_result.opcode == OP_67_WAIT_FRAME_CLOCK &&
            equal_result.executed_instruction_count == 1U &&
            read_u16(strict.state.window, 2U) == 0x800AU &&
            strict.context.instruction_offset == 0U &&
            strict.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK &&
            strict.ports.sound_effect_requests.empty(),
        "opcode 67 waits when unsigned elapsed equals duration"
    );

    strict.runtime.current_tick = 111U;
    const auto greater_result = strict.step();
    test.expect_true(
        greater_result.status == LegacyWorldStoryVmStatus::yielded &&
            greater_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            greater_result.executed_instruction_count == 2U &&
            read_u16(strict.state.window, 2U) == 10U &&
            strict.context.instruction_offset == 8U &&
            strict.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            strict.ports.sound_effect_requests == std::vector<u16>{1U},
        "opcode 67 completes only when elapsed is strictly greater, clears bit15 and fetches the next instruction in the same call"
    );

    Fixture wrapping;
    prime_loaded_instruction(wrapping, OP_67_WAIT_FRAME_CLOCK);
    write_u16(wrapping.state.window, 2U, 0x8020U);
    write_u16(wrapping.state.window, 4U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(wrapping.state.window, 6U, 2U);
    wrapping.state.wait_duration = 0x20U;
    wrapping.state.wait_started_at = 0xFFFFFFF0U;
    wrapping.runtime.current_tick = 0x10U;
    const auto wrapping_equal = wrapping.step();
    wrapping.runtime.current_tick = 0x11U;
    const auto wrapping_greater = wrapping.step();
    test.expect_true(
        wrapping_equal.status == LegacyWorldStoryVmStatus::yielded &&
            wrapping_equal.opcode == OP_67_WAIT_FRAME_CLOCK &&
            wrapping.state.wait_duration == 0x20U &&
            wrapping_greater.status == LegacyWorldStoryVmStatus::yielded &&
            wrapping_greater.opcode == OP_59_PLAY_SOUND_EFFECT &&
            wrapping.ports.sound_effect_requests == std::vector<u16>{2U},
        "opcode 67 elapsed subtraction wraps in u32 before the strict comparison"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_67_WAIT_FRAME_CLOCK);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 67 truncated operand typed-stops before self-modification and previous publication"
    );

    Fixture start_tail;
    start_tail.context.instruction_offset = 0x7FFCU;
    start_tail.context.talk_data_offset = 0x1111U;
    start_tail.state.loaded_file_number = 1U;
    start_tail.state.loaded_data_offset = 0x1111U;
    start_tail.state.window_loaded = true;
    start_tail.runtime.current_tick = 123U;
    start_tail.state.previous_opcode = 0x66U;
    write_u16(start_tail.state.window, 0x7FFCU, OP_67_WAIT_FRAME_CLOCK);
    write_u16(start_tail.state.window, 0x7FFEU, 5U);
    const auto start_tail_result = start_tail.step();
    test.expect_true(
        start_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            read_u16(start_tail.state.window, 0x7FFEU) == 0x8005U &&
            start_tail.context.instruction_offset == 0x7FFCU &&
            start_tail.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 67 phase-one exact tail self-modifies and publishes previous while yielding in place"
    );

    Fixture complete_tail;
    complete_tail.context.instruction_offset = 0x7FFCU;
    complete_tail.context.talk_data_offset = 0x1111U;
    complete_tail.state.loaded_file_number = 1U;
    complete_tail.state.loaded_data_offset = 0x1111U;
    complete_tail.state.window_loaded = true;
    complete_tail.state.wait_duration = 5U;
    complete_tail.state.wait_started_at = 100U;
    complete_tail.runtime.current_tick = 106U;
    complete_tail.state.previous_opcode = 0x66U;
    write_u16(complete_tail.state.window, 0x7FFCU, OP_67_WAIT_FRAME_CLOCK);
    write_u16(complete_tail.state.window, 0x7FFEU, 0x8005U);
    const auto complete_tail_result = complete_tail.step();
    test.expect_true(
        complete_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            complete_tail_result.opcode == OP_67_WAIT_FRAME_CLOCK &&
            complete_tail_result.executed_instruction_count == 1U &&
            read_u16(complete_tail.state.window, 0x7FFEU) == 5U &&
            complete_tail.context.instruction_offset == 0x8000U &&
            complete_tail.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "opcode 67 completion exact tail clears bit15, advances and publishes previous before the next fetch fails"
    );
}

void test_clear_role_flag_0400_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.roles[1].guid = 9U;
        fixture.roles[1].flags = 0xA5A5FFFFU;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_68_CLEAR_ROLE_FLAG_0400 | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 9U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_68_CLEAR_ROLE_FLAG_0400 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.roles[1].flags == 0xA5A5FBFFU &&
                fixture.ports.role_patch_requests.empty() &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_68_CLEAR_ROLE_FLAG_0400,
            "opcode 68 aliases clear only role flag 0400, publish previous and yield"
        );
    }

    Fixture current_source;
    current_source.context.source_guid = 9U;
    current_source.roles[1].guid = 9U;
    current_source.roles[1].flags = 0x00000401U;
    prime_loaded_instruction(current_source, OP_68_CLEAR_ROLE_FLAG_0400);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_source.roles[1].flags == 0x00000001U &&
            current_source.ports.role_patch_requests.empty(),
        "opcode 68 resolves FFF0 through the Talk source before runtime lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 9U;
    controlled.roles[1].flags = 0x00000402U;
    prime_loaded_instruction(controlled, OP_68_CLEAR_ROLE_FLAG_0400);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled.roles[1].flags == 0x00000002U,
        "opcode 68 independently retains the FFFE controlled-role lookup contract"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_68_CLEAR_ROLE_FLAG_0400);
    write_u16(missing.state.window, 2U, 0x1234U);
    missing.state.previous_opcode = 0x66U;
    const auto missing_result = missing.step();
    const auto& missing_request = missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing.ports.role_patch_requests.size() == 1U &&
            missing_request.guid == 0x1234U &&
            missing_request.action_id == 0xFFFFU &&
            missing_request.base_variant == 0xFFFFU &&
            missing_request.variant_delta == 0xFFFFU &&
            missing_request.tile_x == 0xFFFFU &&
            missing_request.tile_y == 0xFFFFU &&
            missing_request.talk_script_id == 0xFFFFU &&
            missing_request.path_data_id == 0xFFFFU &&
            missing_request.flags_or_mask == 0U &&
            missing_request.flags_and_mask == 0xFBFFU &&
            missing_request.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_68_CLEAR_ROLE_FLAG_0400,
        "opcode 68 missing-role fallback submits only MAPS flags OR zero and AND FBFF then consumes"
    );

    Fixture missing_source;
    missing_source.context.source_guid = 0x1234U;
    prime_loaded_instruction(missing_source, OP_68_CLEAR_ROLE_FLAG_0400);
    write_u16(missing_source.state.window, 2U, 0xFFF0U);
    const auto missing_source_result = missing_source.step();
    test.expect_true(
        missing_source_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_source.ports.role_patch_requests.size() == 1U &&
            missing_source.ports.role_patch_requests.front().guid == 0x1234U,
        "opcode 68 missing FFF0 target patches the resolved source GUID rather than raw FFF0"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_68_CLEAR_ROLE_FLAG_0400);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.ports.role_patch_requests.empty() &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 68 truncated selector typed-stops before role or MAPS effects"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_68_CLEAR_ROLE_FLAG_0400);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x1234U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_68_CLEAR_ROLE_FLAG_0400 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.ports.role_patch_requests.size() == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_68_CLEAR_ROLE_FLAG_0400,
        "opcode 68 missing-role exact tail submits MAPS patch, advances, publishes previous and yields without another fetch"
    );
}

void test_set_role_flag_0400_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        fixture.roles[1].guid = 9U;
        fixture.roles[1].flags = 0xA5A50001U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_69_SET_ROLE_FLAG_0400 | alias_mask)
        );
        write_u16(fixture.state.window, 2U, 9U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_69_SET_ROLE_FLAG_0400 &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.roles[1].flags == 0xA5A50401U &&
                fixture.ports.role_patch_requests.empty() &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_69_SET_ROLE_FLAG_0400,
            "opcode 69 aliases set only role flag 0400, publish previous and yield"
        );
    }

    Fixture current_source;
    current_source.context.source_guid = 9U;
    current_source.roles[1].guid = 9U;
    current_source.roles[1].flags = 1U;
    prime_loaded_instruction(current_source, OP_69_SET_ROLE_FLAG_0400);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_source.roles[1].flags == 0x00000401U &&
            current_source.ports.role_patch_requests.empty(),
        "opcode 69 resolves FFF0 through the Talk source before runtime lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 9U;
    controlled.roles[1].flags = 2U;
    prime_loaded_instruction(controlled, OP_69_SET_ROLE_FLAG_0400);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled.roles[1].flags == 0x00000402U,
        "opcode 69 independently retains the FFFE controlled-role lookup contract"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_69_SET_ROLE_FLAG_0400);
    write_u16(missing.state.window, 2U, 0x1234U);
    missing.state.previous_opcode = 0x66U;
    const auto missing_result = missing.step();
    const auto& missing_request = missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing.ports.role_patch_requests.size() == 1U &&
            missing_request.guid == 0x1234U &&
            missing_request.action_id == 0xFFFFU &&
            missing_request.base_variant == 0xFFFFU &&
            missing_request.variant_delta == 0xFFFFU &&
            missing_request.tile_x == 0xFFFFU &&
            missing_request.tile_y == 0xFFFFU &&
            missing_request.talk_script_id == 0xFFFFU &&
            missing_request.path_data_id == 0xFFFFU &&
            missing_request.flags_or_mask == 0x0400U &&
            missing_request.flags_and_mask == 0xFFFFU &&
            missing_request.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_69_SET_ROLE_FLAG_0400,
        "opcode 69 missing-role fallback submits only MAPS flags OR 0400 and AND FFFF then consumes"
    );

    Fixture missing_source;
    missing_source.context.source_guid = 0x1234U;
    prime_loaded_instruction(missing_source, OP_69_SET_ROLE_FLAG_0400);
    write_u16(missing_source.state.window, 2U, 0xFFF0U);
    const auto missing_source_result = missing_source.step();
    test.expect_true(
        missing_source_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_source.ports.role_patch_requests.size() == 1U &&
            missing_source.ports.role_patch_requests.front().guid == 0x1234U,
        "opcode 69 missing FFF0 target patches the resolved source GUID rather than raw FFF0"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_69_SET_ROLE_FLAG_0400);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.ports.role_patch_requests.empty() &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 69 truncated selector typed-stops before role or MAPS effects"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_69_SET_ROLE_FLAG_0400);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x1234U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.opcode == OP_69_SET_ROLE_FLAG_0400 &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.ports.role_patch_requests.size() == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_69_SET_ROLE_FLAG_0400,
        "opcode 69 missing-role exact tail submits MAPS patch, advances, publishes previous and yields without another fetch"
    );
}

void test_wait_for_frame_color_transition(openswd3::test::Context& test) {
    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{
        .countdown = 1,
    };
    fixture.runtime.frame_color = &frame_color;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 53U);
    write_u16(script, 2U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 4U, 0x00F8U);

    const auto waiting = fixture.step();
    frame_color.countdown = 0;
    const auto completed = fixture.step();

    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == 53U && waiting.executed_instruction_count == 1U &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            completed.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U,
        "opcode 53 waits in place only while the signed color countdown is positive"
    );

    Fixture cancel_fixture;
    openswd3::rendering::LegacyFrameColorTransitionState cancel_color{
        .countdown = 7,
        .current_red = 1.0F,
        .current_green = 2.0F,
        .current_blue = 3.0F,
        .target_red = 4.0F,
        .target_green = 5.0F,
        .target_blue = 6.0F,
        .step_red = 7.0F,
        .step_green = 8.0F,
        .step_blue = 9.0F,
    };
    cancel_fixture.runtime.frame_color = &cancel_color;
    auto cancel_script = std::span<u8>{cancel_fixture.ports.initial_window};
    write_u16(cancel_script, 0U, OP_74_CANCEL_FRAME_COLOR_TRANSITION);
    write_u16(cancel_script, 2U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(cancel_script, 4U, 0x00F8U);

    const auto cancelled = cancel_fixture.step();

    test.expect_true(
        cancelled.status == LegacyWorldStoryVmStatus::yielded &&
            cancelled.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            cancelled.executed_instruction_count == 2U &&
            cancel_fixture.context.instruction_offset == 6U &&
            cancel_color.countdown == 0 && cancel_color.step_red == 0.0F &&
            cancel_color.step_green == 0.0F && cancel_color.step_blue == 0.0F &&
            cancel_color.current_red == 1.0F &&
            cancel_color.current_green == 2.0F &&
            cancel_color.current_blue == 3.0F &&
            cancel_color.target_red == 4.0F &&
            cancel_color.target_green == 5.0F &&
            cancel_color.target_blue == 6.0F &&
            cancel_fixture.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 74 clears only the three color steps and countdown then continues"
    );

    constexpr std::array<u16, 4U> cancel_alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : cancel_alias_masks) {
        Fixture exact_tail;
        openswd3::rendering::LegacyFrameColorTransitionState exact_color{
            .countdown = 7,
            .current_red = 1.0F,
            .current_green = 2.0F,
            .current_blue = 3.0F,
            .target_red = 4.0F,
            .target_green = 5.0F,
            .target_blue = 6.0F,
            .step_red = 7.0F,
            .step_green = 8.0F,
            .step_blue = 9.0F,
        };
        exact_tail.runtime.frame_color = &exact_color;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_74_CANCEL_FRAME_COLOR_TRANSITION | alias_mask)
        );

        const auto exact_result = exact_tail.step();

        test.expect_true(
            exact_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_result.opcode == OP_74_CANCEL_FRAME_COLOR_TRANSITION &&
                exact_result.executed_instruction_count == 1U &&
                exact_color.countdown == 0 && exact_color.step_red == 0.0F &&
                exact_color.step_green == 0.0F &&
                exact_color.step_blue == 0.0F &&
                exact_color.current_red == 1.0F &&
                exact_color.target_blue == 6.0F &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_74_CANCEL_FRAME_COLOR_TRANSITION,
            "opcode 74 aliases complete all clears and publication before an exact-tail next fetch fails"
        );
    }

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_74_CANCEL_FRAME_COLOR_TRANSITION);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 74 platform owner absence stops before the first color-state write"
    );
}

void test_suspend_story_role_protocol(openswd3::test::Context& test) {
    Fixture exact_tail;
    exact_tail.roles[1].guid = 0x00F8U;
    exact_tail.roles[1].world_x = 0x20U;
    exact_tail.roles[1].world_y = 0x30U;
    StoryPathHarness path_harness{exact_tail};
    exact_tail.runtime.story_paths = &path_harness.runtime;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FFCU,
        static_cast<u16>(OP_75_SUSPEND_STORY_ROLE | 0xC000U)
    );
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFFEU);

    const auto exact_result = exact_tail.step(0, 0, 1U);

    test.expect_true(
        exact_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_result.opcode == OP_75_SUSPEND_STORY_ROLE &&
            exact_result.executed_instruction_count == 1U &&
            (exact_tail.roles[1].flags & 0x80000000U) != 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_75_SUSPEND_STORY_ROLE,
        "opcode 75 alias uses the FFFE controlled role and publishes before exact-tail next fetch failure"
    );

    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture missing;
        prime_loaded_instruction(
            missing, static_cast<u16>(OP_75_SUSPEND_STORY_ROLE | alias_mask)
        );
        write_u16(missing.state.window, 2U, 0xFFF0U);
        missing.state.previous_opcode = 0x66U;
        const auto missing_result = missing.step();
        test.expect_true(
            missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
                missing_result.opcode == OP_75_SUSPEND_STORY_ROLE &&
                missing.context.instruction_offset == 0U &&
                missing.state.previous_opcode == 0x66U,
            "opcode 75 aliases keep literal FFF0 untranslated and typed-stop the original index-minus-one overrun"
        );
    }

    Fixture unavailable;
    unavailable.roles[1].guid = 0x00F8U;
    prime_loaded_instruction(unavailable, OP_75_SUSPEND_STORY_ROLE);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U &&
            (unavailable.roles[1].flags & 0x80000000U) == 0U,
        "opcode 75 owner absence stops after lookup but before suspend effects"
    );

    Fixture truncated;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x66U;
    write_u16(truncated.state.window, 0x7FFEU, OP_75_SUSPEND_STORY_ROLE);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U,
        "opcode 75 typed-stops before lookup when the selector is truncated"
    );
}

void test_turn_role_toward_role(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].action.base_variant = 7U;
    fixture.roles[1].action.variant_delta = 6U;
    fixture.roles[1].action.wait_remaining = 9U;
    fixture.roles[1].action.field_2c = 0U;
    fixture.roles[1].action.field_30 = 0U;
    fixture.roles[1].world_x = 100U;
    fixture.roles[1].world_y = 100U;
    fixture.roles[2].guid = 0x00F9U;
    fixture.roles[2].flags = 0U;
    fixture.roles[2].world_x = 200U;
    fixture.roles[2].world_y = 100U;

    StoryPathHarness path_harness{fixture};
    fixture.runtime.story_paths = &path_harness.runtime;

    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, OP_76_TURN_AND_SUSPEND_STORY_ROLE);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x00F9U);
    write_u16(script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 8U, 0x00F8U);
    const auto result = fixture.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 2U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.variant_delta == 3U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            (fixture.roles[1].flags & 0x80000000U) != 0U &&
            fixture.context.instruction_offset == 10U &&
            fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 76 turns the first role toward the second and suspends it"
    );
}

void test_turn_role_toward_role_lookup_boundaries(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture missing_first;
        missing_first.context.talk_data_offset = 0x1111U;
        missing_first.context.instruction_offset = 0x7FFCU;
        missing_first.state.loaded_file_number = 1U;
        missing_first.state.loaded_data_offset = 0x1111U;
        missing_first.state.window_loaded = true;
        missing_first.state.previous_opcode = 0x66U;
        write_u16(
            missing_first.state.window,
            0x7FFCU,
            static_cast<u16>(OP_76_TURN_AND_SUSPEND_STORY_ROLE | alias_mask)
        );
        write_u16(missing_first.state.window, 0x7FFEU, 0xFFFFU);
        const auto missing_first_result = missing_first.step();
        test.expect_true(
            missing_first_result.status ==
                    LegacyWorldStoryVmStatus::role_not_found &&
                missing_first_result.opcode ==
                    OP_76_TURN_AND_SUSPEND_STORY_ROLE &&
                missing_first.context.instruction_offset == 0x7FFCU &&
                missing_first.state.previous_opcode == 0x66U,
            "opcode 76 aliases typed-stop a missing first role before reading the absent second selector"
        );
    }

    Fixture second_truncated;
    second_truncated.context.talk_data_offset = 0x1111U;
    second_truncated.context.instruction_offset = 0x7FFCU;
    second_truncated.state.loaded_file_number = 1U;
    second_truncated.state.loaded_data_offset = 0x1111U;
    second_truncated.state.window_loaded = true;
    second_truncated.state.previous_opcode = 0x66U;
    write_u16(
        second_truncated.state.window,
        0x7FFCU,
        OP_76_TURN_AND_SUSPEND_STORY_ROLE
    );
    write_u16(second_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto second_truncated_result = second_truncated.step();
    test.expect_true(
        second_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            second_truncated.context.instruction_offset == 0x7FFCU &&
            second_truncated.state.previous_opcode == 0x66U,
        "opcode 76 reads the second selector only after the first lookup succeeds"
    );

    Fixture second_literal;
    second_literal.roles[1].action.base_variant = 7U;
    second_literal.roles[1].action.variant_delta = 6U;
    second_literal.roles[1].action.wait_remaining = 9U;
    prime_loaded_instruction(second_literal, OP_76_TURN_AND_SUSPEND_STORY_ROLE);
    write_u16(second_literal.state.window, 2U, 0xFFF0U);
    write_u16(second_literal.state.window, 4U, 0xFFF0U);
    second_literal.state.previous_opcode = 0x66U;
    const auto second_literal_result = second_literal.step();
    test.expect_true(
        second_literal_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            second_literal_result.action_update_count == 0U &&
            second_literal.roles[1].action.base_variant == 7U &&
            second_literal.roles[1].action.variant_delta == 6U &&
            second_literal.roles[1].action.wait_remaining == 9U &&
            second_literal.context.instruction_offset == 0U &&
            second_literal.state.previous_opcode == 0x66U,
        "opcode 76 translates FFF0 only for the first selector and stops before action writes when the second literal is missing"
    );
}

void test_turn_role_toward_role_owner_and_exact_tail(
    openswd3::test::Context& test
) {
    Fixture unavailable;
    unavailable.roles[1].action.base_variant = 7U;
    unavailable.roles[1].action.variant_delta = 6U;
    unavailable.roles[1].action.wait_remaining = 9U;
    unavailable.roles[1].action.field_2c = 0U;
    unavailable.roles[1].action.field_30 = 0U;
    unavailable.roles[1].world_x = 100U;
    unavailable.roles[1].world_y = 100U;
    unavailable.roles[2].guid = 0x00F9U;
    unavailable.roles[2].world_x = 200U;
    unavailable.roles[2].world_y = 100U;
    prime_loaded_instruction(unavailable, OP_76_TURN_AND_SUSPEND_STORY_ROLE);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    write_u16(unavailable.state.window, 4U, 0x00F9U);
    unavailable.state.previous_opcode = 0x66U;

    const auto unavailable_result = unavailable.step();

    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_result.action_update_count == 1U &&
            unavailable.roles[1].action.base_variant == 0U &&
            unavailable.roles[1].action.variant_delta == 3U &&
            unavailable.roles[1].action.wait_remaining == 0U &&
            (unavailable.roles[1].flags & 0x80000000U) == 0U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 76 performs action writes and refresh before the suspend owner is first accessed"
    );

    Fixture exact_tail;
    exact_tail.roles[1].action.base_variant = 7U;
    exact_tail.roles[1].action.variant_delta = 6U;
    exact_tail.roles[1].action.wait_remaining = 9U;
    exact_tail.roles[1].action.field_2c = 0U;
    exact_tail.roles[1].action.field_30 = 0U;
    exact_tail.roles[1].world_x = 100U;
    exact_tail.roles[1].world_y = 100U;
    exact_tail.roles[2].guid = 0x00F9U;
    exact_tail.roles[2].world_x = 200U;
    exact_tail.roles[2].world_y = 100U;
    StoryPathHarness path_harness{exact_tail};
    exact_tail.runtime.story_paths = &path_harness.runtime;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FFAU,
        static_cast<u16>(OP_76_TURN_AND_SUSPEND_STORY_ROLE | 0xC000U)
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0xFFF0U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x00F9U);

    const auto exact_result = exact_tail.step();

    test.expect_true(
        exact_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_result.opcode == OP_76_TURN_AND_SUSPEND_STORY_ROLE &&
            exact_result.executed_instruction_count == 1U &&
            exact_result.action_update_count == 1U &&
            exact_tail.roles[1].action.base_variant == 0U &&
            exact_tail.roles[1].action.variant_delta == 3U &&
            exact_tail.roles[1].action.wait_remaining == 0U &&
            (exact_tail.roles[1].flags & 0x80000000U) != 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_76_TURN_AND_SUSPEND_STORY_ROLE,
        "opcode 76 alias translates only the first FFF0 selector and completes action, suspend and publication before exact-tail next fetch failure"
    );
}

void test_set_role_head_sign_action(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<u16, 4U> slots{0U, 1U, 3U, 0xFFFFU};
    for (std::size_t index = 0U; index < alias_masks.size(); ++index) {
        Fixture fixture;
        fixture.roles[1].field_3c = 0x12345678U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_71_SET_ROLE_HEAD_SIGN | alias_masks[index])
        );
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, slots[index]);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_71_SET_ROLE_HEAD_SIGN &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.roles[1].field_3c ==
                    openswd3::world_map::legacy_world_head_sign_action_token(
                        slots[index]
                    ) &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_71_SET_ROLE_HEAD_SIGN,
            "opcode 71 aliases zero-extend the slot without a range check, assign the head-sign token, publish previous and yield"
        );
    }

    Fixture missing_tail;
    missing_tail.roles[1].field_3c = 0x12345678U;
    missing_tail.context.instruction_offset = 0x7FFCU;
    missing_tail.context.talk_data_offset = 0x1111U;
    missing_tail.state.loaded_file_number = 1U;
    missing_tail.state.loaded_data_offset = 0x1111U;
    missing_tail.state.window_loaded = true;
    missing_tail.state.previous_opcode = 0x66U;
    write_u16(missing_tail.state.window, 0x7FFCU, OP_71_SET_ROLE_HEAD_SIGN);
    write_u16(missing_tail.state.window, 0x7FFEU, 0xFFF0U);
    const auto missing_result = missing_tail.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.opcode == OP_71_SET_ROLE_HEAD_SIGN &&
            missing_result.executed_instruction_count == 1U &&
            missing_tail.roles[1].field_3c == 0x12345678U &&
            missing_tail.context.instruction_offset == 0x8002U &&
            missing_tail.state.previous_opcode == OP_71_SET_ROLE_HEAD_SIGN,
        "opcode 71 unresolved literal FFF0 consumes six bytes without reading the missing slot operand"
    );

    Fixture found_truncated;
    found_truncated.roles[1].field_3c = 0x12345678U;
    found_truncated.context.instruction_offset = 0x7FFCU;
    found_truncated.context.talk_data_offset = 0x1111U;
    found_truncated.state.loaded_file_number = 1U;
    found_truncated.state.loaded_data_offset = 0x1111U;
    found_truncated.state.window_loaded = true;
    found_truncated.state.previous_opcode = 0x66U;
    write_u16(found_truncated.state.window, 0x7FFCU, OP_71_SET_ROLE_HEAD_SIGN);
    write_u16(found_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto found_truncated_result = found_truncated.step();
    test.expect_true(
        found_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_truncated.roles[1].field_3c == 0x12345678U &&
            found_truncated.context.instruction_offset == 0x7FFCU &&
            found_truncated.state.previous_opcode == 0x66U,
        "opcode 71 found-role path reads the slot only after lookup and typed-stops a truncated tail"
    );

    Fixture controlled;
    controlled.roles[1].field_3c = 0U;
    prime_loaded_instruction(controlled, OP_71_SET_ROLE_HEAD_SIGN);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 2U);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled.roles[1].field_3c ==
                openswd3::world_map::legacy_world_head_sign_action_token(2U),
        "opcode 71 independently retains the FFFE controlled-role lookup contract"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_71_SET_ROLE_HEAD_SIGN);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 3U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail.roles[1].field_3c ==
                openswd3::world_map::legacy_world_head_sign_action_token(3U) &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_71_SET_ROLE_HEAD_SIGN,
        "opcode 71 exact tail assigns the token, advances, publishes previous and yields without another fetch"
    );

    Fixture cleared;
    cleared.roles[1].field_3c =
        openswd3::world_map::legacy_world_head_sign_action_token(2U);
    auto clear_script = std::span<u8>{cleared.ports.initial_window};
    write_u16(clear_script, 0U, OP_72_CLEAR_ROLE_HEAD_SIGN);
    write_u16(clear_script, 2U, 0x00F8U);
    const auto removed = cleared.step();
    test.expect_true(
        removed.status == LegacyWorldStoryVmStatus::yielded &&
            removed.opcode == OP_72_CLEAR_ROLE_HEAD_SIGN &&
            removed.executed_instruction_count == 1U &&
            removed.direct_audio_service_count == 1U &&
            cleared.roles[1].field_3c == 0U &&
            cleared.context.instruction_offset == 4U &&
            cleared.state.previous_opcode == OP_72_CLEAR_ROLE_HEAD_SIGN,
        "opcode 72 clears the head sign, publishes previous and yields"
    );

    for (const u16 alias_mask : alias_masks) {
        Fixture alias;
        alias.roles[1].field_3c = 0x12345678U;
        prime_loaded_instruction(
            alias, static_cast<u16>(OP_72_CLEAR_ROLE_HEAD_SIGN | alias_mask)
        );
        write_u16(alias.state.window, 2U, 0x00F8U);
        alias.state.previous_opcode = 0x66U;
        const auto alias_result = alias.step();
        test.expect_true(
            alias_result.status == LegacyWorldStoryVmStatus::yielded &&
                alias_result.opcode == OP_72_CLEAR_ROLE_HEAD_SIGN &&
                alias_result.executed_instruction_count == 1U &&
                alias.roles[1].field_3c == 0U &&
                alias.context.instruction_offset == 4U &&
                alias.state.previous_opcode == OP_72_CLEAR_ROLE_HEAD_SIGN,
            "opcode 72 aliases clear the head sign, publish previous and yield"
        );
    }

    Fixture missing;
    missing.roles[1].field_3c = 0x12345678U;
    prime_loaded_instruction(missing, OP_72_CLEAR_ROLE_HEAD_SIGN);
    write_u16(missing.state.window, 2U, 0xFFF0U);
    missing.state.previous_opcode = 0x66U;
    const auto clear_missing_result = missing.step();
    test.expect_true(
        clear_missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing.roles[1].field_3c == 0x12345678U &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_72_CLEAR_ROLE_HEAD_SIGN,
        "opcode 72 keeps literal FFF0 untranslated and silently consumes a missing role"
    );

    Fixture controlled_clear;
    controlled_clear.roles[1].field_3c = 0x12345678U;
    prime_loaded_instruction(controlled_clear, OP_72_CLEAR_ROLE_HEAD_SIGN);
    write_u16(controlled_clear.state.window, 2U, 0xFFFEU);
    const auto controlled_clear_result = controlled_clear.step(0, 0, 1U);
    test.expect_true(
        controlled_clear_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled_clear.roles[1].field_3c == 0U,
        "opcode 72 retains the shared FFFE controlled-role lookup contract"
    );

    Fixture clear_truncated;
    clear_truncated.context.instruction_offset = 0x7FFEU;
    clear_truncated.context.talk_data_offset = 0x1111U;
    clear_truncated.state.loaded_file_number = 1U;
    clear_truncated.state.loaded_data_offset = 0x1111U;
    clear_truncated.state.window_loaded = true;
    clear_truncated.state.previous_opcode = 0x66U;
    write_u16(
        clear_truncated.state.window, 0x7FFEU, OP_72_CLEAR_ROLE_HEAD_SIGN
    );
    const auto clear_truncated_result = clear_truncated.step();
    test.expect_true(
        clear_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            clear_truncated.context.instruction_offset == 0x7FFEU &&
            clear_truncated.state.previous_opcode == 0x66U,
        "opcode 72 typed-stops before lookup when the selector is truncated"
    );

    Fixture clear_exact_tail;
    clear_exact_tail.context.instruction_offset = 0x7FFCU;
    clear_exact_tail.context.talk_data_offset = 0x1111U;
    clear_exact_tail.state.loaded_file_number = 1U;
    clear_exact_tail.state.loaded_data_offset = 0x1111U;
    clear_exact_tail.state.window_loaded = true;
    clear_exact_tail.state.previous_opcode = 0x66U;
    clear_exact_tail.roles[1].field_3c = 0x12345678U;
    write_u16(
        clear_exact_tail.state.window, 0x7FFCU, OP_72_CLEAR_ROLE_HEAD_SIGN
    );
    write_u16(clear_exact_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto clear_exact_tail_result = clear_exact_tail.step();
    test.expect_true(
        clear_exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            clear_exact_tail.roles[1].field_3c == 0U &&
            clear_exact_tail.context.instruction_offset == 0x8000U &&
            clear_exact_tail.state.previous_opcode ==
                OP_72_CLEAR_ROLE_HEAD_SIGN,
        "opcode 72 exact tail clears, advances, publishes previous and yields without another fetch"
    );
}
