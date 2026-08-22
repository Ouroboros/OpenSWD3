#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_start_camera_move_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        CameraMoveFixture relative;
        relative.camera.right = 800U;
        relative.camera.bottom = 800U;
        relative.camera_pan.remaining_x = 0x11111111;
        relative.camera_pan.remaining_y = 0x22222222;
        relative.camera_pan.step_x = 0x33333333;
        relative.camera_pan.step_y = 0x44444444;
        relative.state.previous_opcode = 0x55U;
        prime_long_camera_move(
            relative,
            static_cast<u16>(OP_50_START_RELATIVE_CAMERA_MOVE | mask),
            -3,
            2,
            8U,
            7U
        );
        const auto relative_result = relative.step(160, 320);
        test.expect_true(
            relative_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                relative_result.opcode == kStoryVmTypedStop &&
                relative_result.executed_instruction_count == 2U &&
                relative.camera_pan.remaining_x == -48 &&
                relative.camera_pan.remaining_y == 32 &&
                relative.camera_pan.step_x == -8 &&
                relative.camera_pan.step_y == 4 &&
                relative.context.instruction_offset == 10U &&
                relative.state.previous_opcode ==
                    OP_50_START_RELATIVE_CAMERA_MOVE &&
                relative.ports.direct_audio_service_count == 0U,
            "opcode 50 aliases replace active motion and derive signed steps"
        );

        CameraMoveFixture absolute;
        absolute.camera.right = 800U;
        absolute.camera.bottom = 560U;
        absolute.camera_pan.remaining_x = 0x11111111;
        absolute.camera_pan.remaining_y = 0x22222222;
        absolute.camera_pan.step_x = 0x33333333;
        absolute.camera_pan.step_y = 0x44444444;
        absolute.state.previous_opcode = 0x55U;
        prime_long_camera_move(
            absolute,
            static_cast<u16>(OP_70_START_ABSOLUTE_CAMERA_MOVE | mask),
            7,
            9,
            6U,
            8U
        );
        const auto absolute_result = absolute.step(160, 80);
        test.expect_true(
            absolute_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                absolute_result.opcode == kStoryVmTypedStop &&
                absolute_result.executed_instruction_count == 2U &&
                absolute.camera_pan.remaining_x == -48 &&
                absolute.camera_pan.remaining_y == 64 &&
                absolute.camera_pan.step_x == -6 &&
                absolute.camera_pan.step_y == 8 &&
                absolute.context.instruction_offset == 10U &&
                absolute.state.previous_opcode ==
                    OP_70_START_ABSOLUTE_CAMERA_MOVE &&
                absolute.ports.direct_audio_service_count == 0U,
            "opcode 70 aliases derive displacement from viewport tile origin"
        );

        CameraMoveFixture role_target;
        role_target.roles[1].world_x = 800U;
        role_target.roles[1].world_y = 640U;
        role_target.camera_pan.remaining_x = 0x11111111;
        role_target.camera_pan.remaining_y = 0x22222222;
        role_target.camera_pan.step_x = 0x33333333;
        role_target.camera_pan.step_y = 0x44444444;
        role_target.state.previous_opcode = 0x55U;
        prime_role_camera_move(
            role_target,
            static_cast<u16>(OP_73_START_CAMERA_MOVE_TO_ROLE | mask),
            0x00F8U,
            16U,
            6U
        );
        const auto role_result = role_target.step();
        test.expect_true(
            role_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                role_result.opcode == kStoryVmTypedStop &&
                role_result.executed_instruction_count == 2U &&
                role_target.camera_pan.remaining_x == 480 &&
                role_target.camera_pan.remaining_y == 400 &&
                role_target.camera_pan.step_x == 16 &&
                role_target.camera_pan.step_y == 4 &&
                role_target.context.instruction_offset == 8U &&
                role_target.state.previous_opcode ==
                    OP_73_START_CAMERA_MOVE_TO_ROLE &&
                role_target.ports.direct_audio_service_count == 0U,
            "opcode 73 aliases center a clamped viewport on the selected role"
        );
    }

    CameraMoveFixture zero_axes;
    zero_axes.camera.right = 640U;
    zero_axes.camera.bottom = 480U;
    zero_axes.camera_pan.remaining_x = 11;
    zero_axes.camera_pan.remaining_y = 22;
    zero_axes.camera_pan.step_x = 33;
    zero_axes.camera_pan.step_y = 44;
    prime_long_camera_move(
        zero_axes, OP_50_START_RELATIVE_CAMERA_MOVE, 0, 0, 0U, 0U
    );
    const auto zero_axes_result = zero_axes.step();
    test.expect_true(
        zero_axes_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_axes.camera_pan.remaining_x == 0 &&
            zero_axes.camera_pan.remaining_y == 0 &&
            zero_axes.camera_pan.step_x == 0 &&
            zero_axes.camera_pan.step_y == 0 &&
            zero_axes.context.instruction_offset == 10U,
        "zero camera displacement accepts zero requested steps"
    );

    CameraMoveFixture literal_fff0;
    literal_fff0.roles[1].world_x = 320U;
    literal_fff0.roles[1].world_y = 240U;
    literal_fff0.roles[2].guid = 0xFFF0U;
    literal_fff0.roles[2].world_x = 960U;
    literal_fff0.roles[2].world_y = 720U;
    prime_role_camera_move(
        literal_fff0, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFF0U, 16U, 16U
    );
    const auto literal_fff0_result = literal_fff0.step();
    test.expect_true(
        literal_fff0_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            literal_fff0.camera_pan.remaining_x == 640 &&
            literal_fff0.camera_pan.remaining_y == 480 &&
            literal_fff0.camera_pan.step_x == 16 &&
            literal_fff0.camera_pan.step_y == 16,
        "opcode 73 treats FFF0 as a literal role GUID"
    );

    CameraMoveFixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].world_x = 320U;
    controlled.roles[1].world_y = 240U;
    controlled.roles[2].world_x = 960U;
    controlled.roles[2].world_y = 720U;
    prime_role_camera_move(
        controlled, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFFEU, 16U, 16U
    );
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled.camera_pan.remaining_x == 640 &&
            controlled.camera_pan.remaining_y == 480,
        "opcode 73 passes FFFE through for controlled-role selection"
    );

    CameraMoveFixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[0].world_x = 320U;
    first_clear_match.roles[0].world_y = 240U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].world_x = 960U;
    first_clear_match.roles[1].world_y = 720U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].world_x = 1200U;
    first_clear_match.roles[2].world_y = 960U;
    prime_role_camera_move(
        first_clear_match, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x2222U, 16U, 16U
    );
    const auto first_clear_result = first_clear_match.step();
    test.expect_true(
        first_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_clear_match.camera_pan.remaining_x == 640 &&
            first_clear_match.camera_pan.remaining_y == 480,
        "opcode 73 skips bit-28 roles and uses the first clear GUID match"
    );

    CameraMoveFixture far_role;
    far_role.roles[1].world_x = 2000U;
    far_role.roles[1].world_y = 1600U;
    prime_role_camera_move(
        far_role, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x00F8U, 16U, 16U
    );
    const auto far_role_result = far_role.step();
    test.expect_true(
        far_role_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            far_role.camera_pan.remaining_x == 960 &&
            far_role.camera_pan.remaining_y == 800 &&
            far_role.camera_pan.step_x == 16 &&
            far_role.camera_pan.step_y == 16,
        "opcode 73 centers then clamps the target viewport to map bounds"
    );

    CameraMoveFixture zero_map;
    zero_map.runtime.role_surface.map_width = 0U;
    zero_map.runtime.map_height = 0U;
    zero_map.roles[1].world_x = 320U;
    zero_map.roles[1].world_y = 240U;
    prime_role_camera_move(
        zero_map, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x00F8U, 16U, 16U
    );
    const auto zero_map_result = zero_map.step();
    test.expect_true(
        zero_map_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_map.camera_pan.remaining_x == -640 &&
            zero_map.camera_pan.remaining_y == -480 &&
            zero_map.camera_pan.step_x == -16 &&
            zero_map.camera_pan.step_y == -16 &&
            zero_map.context.instruction_offset == 8U,
        "zero-sized maps preserve original wrapping camera clamping"
    );

    CameraMoveFixture high_clamp;
    high_clamp.runtime.role_surface.map_width = 50U;
    high_clamp.runtime.map_height = 40U;
    high_clamp.camera.right = 800U;
    high_clamp.camera.bottom = 640U;
    prime_long_camera_move(
        high_clamp, OP_50_START_RELATIVE_CAMERA_MOVE, 10, 10, 16U, 16U
    );
    const auto high_clamp_result = high_clamp.step();
    test.expect_true(
        high_clamp_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            high_clamp.camera_pan.remaining_x == 0 &&
            high_clamp.camera_pan.remaining_y == 0 &&
            high_clamp.camera_pan.step_x == 16 &&
            high_clamp.camera_pan.step_y == 16,
        "camera max clamping occurs after step derivation without recomputing steps"
    );

    CameraMoveFixture low_clamp;
    low_clamp.camera.right = 800U;
    low_clamp.camera.bottom = 800U;
    prime_long_camera_move(
        low_clamp, OP_50_START_RELATIVE_CAMERA_MOVE, -20, -20, 16U, 16U
    );
    const auto low_clamp_result = low_clamp.step(160, 160);
    test.expect_true(
        low_clamp_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            low_clamp.camera_pan.remaining_x == -160 &&
            low_clamp.camera_pan.remaining_y == -160 &&
            low_clamp.camera_pan.step_x == -16 &&
            low_clamp.camera_pan.step_y == -16,
        "camera minimum clamping preserves already-signed step magnitudes"
    );
}

void test_start_camera_move_failure_ordering(openswd3::test::Context& test) {
    CameraMoveFixture x_divide_by_zero;
    x_divide_by_zero.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        x_divide_by_zero, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 2, 0U, 3U
    );
    const auto x_divide_result = x_divide_by_zero.step();
    test.expect_true(
        x_divide_result.status ==
                LegacyWorldStoryVmStatus::camera_step_divide_by_zero &&
            x_divide_result.opcode == OP_50_START_RELATIVE_CAMERA_MOVE &&
            x_divide_result.executed_instruction_count == 1U &&
            x_divide_by_zero.camera_pan.remaining_x == 16 &&
            x_divide_by_zero.camera_pan.remaining_y == 32 &&
            x_divide_by_zero.camera_pan.step_x == 0 &&
            x_divide_by_zero.camera_pan.step_y == 3 &&
            x_divide_by_zero.context.instruction_offset == 0U &&
            x_divide_by_zero.state.previous_opcode == 0x55U &&
            x_divide_by_zero.ports.direct_audio_service_count == 0U,
        "X step divide-by-zero preserves shifted displacements and raw steps"
    );

    CameraMoveFixture y_divide_by_zero;
    y_divide_by_zero.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        y_divide_by_zero, OP_50_START_RELATIVE_CAMERA_MOVE, -1, -2, 3U, 0U
    );
    const auto y_divide_result = y_divide_by_zero.step();
    test.expect_true(
        y_divide_result.status ==
                LegacyWorldStoryVmStatus::camera_step_divide_by_zero &&
            y_divide_by_zero.camera_pan.remaining_x == -16 &&
            y_divide_by_zero.camera_pan.remaining_y == -32 &&
            y_divide_by_zero.camera_pan.step_x == 4 &&
            y_divide_by_zero.camera_pan.step_y == 0 &&
            y_divide_by_zero.context.instruction_offset == 0U &&
            y_divide_by_zero.state.previous_opcode == 0x55U,
        "Y step divide-by-zero retains X fallback before sign application"
    );

    CameraMoveFixture relative_without_camera;
    relative_without_camera.runtime.camera = nullptr;
    relative_without_camera.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        relative_without_camera, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 0, 1U, 0U
    );
    const auto relative_without_camera_result = relative_without_camera.step();
    test.expect_true(
        relative_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            relative_without_camera.camera_pan.remaining_x == 16 &&
            relative_without_camera.camera_pan.remaining_y == 0 &&
            relative_without_camera.camera_pan.step_x == 1 &&
            relative_without_camera.camera_pan.step_y == 0 &&
            relative_without_camera.context.instruction_offset == 0U &&
            relative_without_camera.state.previous_opcode == 0x55U,
        "opcode 50 reaches the camera owner only after step preparation"
    );

    CameraMoveFixture absolute_without_camera;
    absolute_without_camera.runtime.camera = nullptr;
    absolute_without_camera.camera_pan.remaining_x = 0x11111111;
    absolute_without_camera.camera_pan.remaining_y = 0x22222222;
    absolute_without_camera.camera_pan.step_x = 0x33333333;
    absolute_without_camera.camera_pan.step_y = 0x44444444;
    prime_long_camera_move(
        absolute_without_camera, OP_70_START_ABSOLUTE_CAMERA_MOVE, 7, 9, 4U, 4U
    );
    const auto absolute_without_camera_result = absolute_without_camera.step();
    test.expect_true(
        absolute_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            absolute_without_camera.camera_pan.remaining_x == 0x11111111 &&
            absolute_without_camera.camera_pan.remaining_y == 0x22222222 &&
            absolute_without_camera.camera_pan.step_x == 0x33333333 &&
            absolute_without_camera.camera_pan.step_y == 0x44444444 &&
            absolute_without_camera.context.instruction_offset == 0U,
        "opcode 70 accesses the camera after its first target operand"
    );

    CameraMoveFixture missing_role;
    missing_role.camera_pan.remaining_x = 11;
    missing_role.camera_pan.remaining_y = 22;
    missing_role.camera_pan.step_x = 33;
    missing_role.camera_pan.step_y = 44;
    missing_role.state.previous_opcode = 0x55U;
    prime_role_camera_move(
        missing_role, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7777U, 4U, 4U
    );
    const auto missing_role_result = missing_role.step();
    test.expect_true(
        missing_role_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            missing_role_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            missing_role_result.executed_instruction_count == 1U &&
            missing_role.camera_pan.remaining_x == 11 &&
            missing_role.camera_pan.remaining_y == 22 &&
            missing_role.camera_pan.step_x == 33 &&
            missing_role.camera_pan.step_y == 44 &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.ports.role_patch_requests.empty(),
        "opcode 73 missing role stops at the unchecked coordinate access"
    );

    CameraMoveFixture missing_role_without_camera;
    missing_role_without_camera.runtime.camera = nullptr;
    prime_role_camera_move(
        missing_role_without_camera,
        OP_73_START_CAMERA_MOVE_TO_ROLE,
        0x7777U,
        4U,
        4U
    );
    const auto missing_without_camera_result =
        missing_role_without_camera.step();
    test.expect_true(
        missing_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_without_camera.context.instruction_offset == 0U,
        "opcode 73 copies the camera owner before its unsafe missing-role read"
    );

    Fixture missing_pan;
    prime_long_camera_move(
        missing_pan, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 1, 1U, 1U
    );
    const auto missing_pan_result = missing_pan.step();
    test.expect_true(
        missing_pan_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_pan_result.opcode == OP_50_START_RELATIVE_CAMERA_MOVE &&
            missing_pan_result.executed_instruction_count == 1U &&
            missing_pan.context.instruction_offset == 0U,
        "camera move handlers require the process camera-pan owner first"
    );

    CameraMoveFixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime_role_camera_move(
        invalid_controlled, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFFEU, 4U, 4U
    );
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.camera_pan.remaining_x == 0 &&
            invalid_controlled.camera_pan.remaining_y == 0 &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 73 invalid controlled owner stops before opcode fetch"
    );
}

void test_start_camera_move_window_boundaries(openswd3::test::Context& test) {
    const auto prime_at =
        [](Fixture& fixture, const u16 raw_word, const u16 offset) {
            fixture.context.talk_data_offset = 0x1111U;
            fixture.context.instruction_offset = offset;
            fixture.state.loaded_file_number = 1U;
            fixture.state.loaded_data_offset = 0x1111U;
            fixture.state.window_loaded = true;
            fixture.state.previous_opcode = 0x55U;
            write_u16(fixture.state.window, offset, raw_word);
        };

    constexpr std::array<u16, 2U> long_opcodes{
        OP_50_START_RELATIVE_CAMERA_MOVE,
        OP_70_START_ABSOLUTE_CAMERA_MOVE,
    };
    for (const u16 opcode : long_opcodes) {
        const i16 first = opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? -2 : 5;
        const i16 second = opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? 3 : 7;
        const i32 expected_tile_x =
            opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? -2 : 1;
        constexpr i32 expected_tile_y = 3;

        CameraMoveFixture first_operand_truncated;
        first_operand_truncated.camera_pan.remaining_x = 11;
        first_operand_truncated.camera_pan.remaining_y = 22;
        first_operand_truncated.camera_pan.step_x = 33;
        first_operand_truncated.camera_pan.step_y = 44;
        prime_at(first_operand_truncated, opcode, 0x7FFEU);
        const auto first_truncated_result =
            first_operand_truncated.step(64, 64);
        test.expect_true(
            first_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                first_truncated_result.opcode == opcode &&
                first_truncated_result.executed_instruction_count == 1U &&
                first_operand_truncated.camera_pan.remaining_x == 11 &&
                first_operand_truncated.camera_pan.remaining_y == 22 &&
                first_operand_truncated.camera_pan.step_x == 33 &&
                first_operand_truncated.camera_pan.step_y == 44 &&
                first_operand_truncated.context.instruction_offset == 0x7FFEU &&
                first_operand_truncated.state.previous_opcode == 0x55U,
            "long camera moves stop before the first operand word"
        );

        CameraMoveFixture second_operand_truncated;
        second_operand_truncated.camera_pan.remaining_x = 11;
        second_operand_truncated.camera_pan.remaining_y = 22;
        second_operand_truncated.camera_pan.step_x = 33;
        second_operand_truncated.camera_pan.step_y = 44;
        prime_at(second_operand_truncated, opcode, 0x7FFCU);
        write_u16(
            second_operand_truncated.state.window,
            0x7FFEU,
            static_cast<u16>(first)
        );
        const auto second_truncated_result =
            second_operand_truncated.step(64, 64);
        test.expect_true(
            second_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                second_operand_truncated.camera_pan.remaining_x ==
                    (opcode == OP_50_START_RELATIVE_CAMERA_MOVE
                         ? expected_tile_x
                         : 11) &&
                second_operand_truncated.camera_pan.remaining_y == 22 &&
                second_operand_truncated.camera_pan.step_x == 33 &&
                second_operand_truncated.camera_pan.step_y == 44 &&
                second_operand_truncated.context.instruction_offset ==
                    0x7FFCU &&
                second_operand_truncated.state.previous_opcode == 0x55U,
            "camera move variants preserve their distinct Y-target truncation order"
        );

        CameraMoveFixture x_step_truncated;
        x_step_truncated.camera_pan.step_x = 33;
        x_step_truncated.camera_pan.step_y = 44;
        prime_at(x_step_truncated, opcode, 0x7FFAU);
        write_u16(
            x_step_truncated.state.window, 0x7FFCU, static_cast<u16>(first)
        );
        write_u16(
            x_step_truncated.state.window, 0x7FFEU, static_cast<u16>(second)
        );
        const auto x_step_truncated_result = x_step_truncated.step(64, 64);
        test.expect_true(
            x_step_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                x_step_truncated.camera_pan.remaining_x == expected_tile_x &&
                x_step_truncated.camera_pan.remaining_y == expected_tile_y &&
                x_step_truncated.camera_pan.step_x == 33 &&
                x_step_truncated.camera_pan.step_y == 44 &&
                x_step_truncated.context.instruction_offset == 0x7FFAU &&
                x_step_truncated.state.previous_opcode == 0x55U,
            "long camera moves retain both tile displacements before X-step truncation"
        );

        CameraMoveFixture y_step_truncated;
        y_step_truncated.camera_pan.step_x = 33;
        y_step_truncated.camera_pan.step_y = 44;
        prime_at(y_step_truncated, opcode, 0x7FF8U);
        write_u16(
            y_step_truncated.state.window, 0x7FFAU, static_cast<u16>(first)
        );
        write_u16(
            y_step_truncated.state.window, 0x7FFCU, static_cast<u16>(second)
        );
        write_u16(y_step_truncated.state.window, 0x7FFEU, 8U);
        const auto y_step_truncated_result = y_step_truncated.step(64, 64);
        test.expect_true(
            y_step_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                y_step_truncated.camera_pan.remaining_x == expected_tile_x &&
                y_step_truncated.camera_pan.remaining_y == expected_tile_y &&
                y_step_truncated.camera_pan.step_x == 8 &&
                y_step_truncated.camera_pan.step_y == 44 &&
                y_step_truncated.context.instruction_offset == 0x7FF8U &&
                y_step_truncated.state.previous_opcode == 0x55U,
            "long camera moves retain raw X step before Y-step truncation"
        );

        CameraMoveFixture exact_tail;
        exact_tail.camera.right = 704U;
        exact_tail.camera.bottom = 544U;
        prime_at(exact_tail, opcode, 0x7FF6U);
        write_u16(exact_tail.state.window, 0x7FF8U, static_cast<u16>(first));
        write_u16(exact_tail.state.window, 0x7FFAU, static_cast<u16>(second));
        write_u16(exact_tail.state.window, 0x7FFCU, 8U);
        write_u16(exact_tail.state.window, 0x7FFEU, 4U);
        const auto exact_tail_result = exact_tail.step(64, 64);
        const i32 expected_pixel_x = expected_tile_x * 16;
        test.expect_true(
            exact_tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_tail_result.opcode == opcode &&
                exact_tail_result.executed_instruction_count == 1U &&
                exact_tail.camera_pan.remaining_x == expected_pixel_x &&
                exact_tail.camera_pan.remaining_y == 48 &&
                exact_tail.camera_pan.step_x ==
                    (expected_pixel_x < 0 ? -8 : 8) &&
                exact_tail.camera_pan.step_y == 4 &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == opcode,
            "complete long camera records finish effects before next fetch"
        );
    }

    CameraMoveFixture role_selector_truncated;
    role_selector_truncated.camera_pan.remaining_x = 11;
    role_selector_truncated.camera_pan.remaining_y = 22;
    role_selector_truncated.camera_pan.step_x = 33;
    role_selector_truncated.camera_pan.step_y = 44;
    prime_at(role_selector_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFEU);
    const auto role_selector_result = role_selector_truncated.step();
    test.expect_true(
        role_selector_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_selector_truncated.camera_pan.remaining_x == 11 &&
            role_selector_truncated.camera_pan.remaining_y == 22 &&
            role_selector_truncated.camera_pan.step_x == 33 &&
            role_selector_truncated.camera_pan.step_y == 44 &&
            role_selector_truncated.context.instruction_offset == 0x7FFEU &&
            role_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 73 stops before the selector word"
    );

    CameraMoveFixture role_x_step_truncated;
    role_x_step_truncated.roles[1].world_x = 800U;
    role_x_step_truncated.roles[1].world_y = 640U;
    role_x_step_truncated.camera_pan.step_x = 33;
    role_x_step_truncated.camera_pan.step_y = 44;
    prime_at(role_x_step_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFCU);
    write_u16(role_x_step_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto role_x_step_result = role_x_step_truncated.step();
    test.expect_true(
        role_x_step_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_x_step_truncated.camera_pan.remaining_x == 30 &&
            role_x_step_truncated.camera_pan.remaining_y == 25 &&
            role_x_step_truncated.camera_pan.step_x == 33 &&
            role_x_step_truncated.camera_pan.step_y == 44 &&
            role_x_step_truncated.context.instruction_offset == 0x7FFCU &&
            role_x_step_truncated.state.previous_opcode == 0x55U,
        "opcode 73 retains role-derived tile displacements before X-step truncation"
    );

    CameraMoveFixture role_y_step_truncated;
    role_y_step_truncated.roles[1].world_x = 800U;
    role_y_step_truncated.roles[1].world_y = 640U;
    role_y_step_truncated.camera_pan.step_x = 33;
    role_y_step_truncated.camera_pan.step_y = 44;
    prime_at(role_y_step_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFAU);
    write_u16(role_y_step_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(role_y_step_truncated.state.window, 0x7FFEU, 8U);
    const auto role_y_step_result = role_y_step_truncated.step();
    test.expect_true(
        role_y_step_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_y_step_truncated.camera_pan.remaining_x == 30 &&
            role_y_step_truncated.camera_pan.remaining_y == 25 &&
            role_y_step_truncated.camera_pan.step_x == 8 &&
            role_y_step_truncated.camera_pan.step_y == 44 &&
            role_y_step_truncated.context.instruction_offset == 0x7FFAU &&
            role_y_step_truncated.state.previous_opcode == 0x55U,
        "opcode 73 retains raw X step before Y-step truncation"
    );

    CameraMoveFixture role_exact_tail;
    role_exact_tail.roles[1].world_x = 800U;
    role_exact_tail.roles[1].world_y = 640U;
    prime_at(role_exact_tail, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FF8U);
    write_u16(role_exact_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(role_exact_tail.state.window, 0x7FFCU, 8U);
    write_u16(role_exact_tail.state.window, 0x7FFEU, 8U);
    const auto role_exact_tail_result = role_exact_tail.step();
    test.expect_true(
        role_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            role_exact_tail_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            role_exact_tail_result.executed_instruction_count == 1U &&
            role_exact_tail.camera_pan.remaining_x == 480 &&
            role_exact_tail.camera_pan.remaining_y == 400 &&
            role_exact_tail.camera_pan.step_x == 8 &&
            role_exact_tail.camera_pan.step_y == 8 &&
            role_exact_tail.context.instruction_offset == 0x8000U &&
            role_exact_tail.state.previous_opcode ==
                OP_73_START_CAMERA_MOVE_TO_ROLE,
        "opcode 73 exact tail completes when clamp diagnostics are skipped"
    );

    CameraMoveFixture clamped_diagnostic_tail;
    clamped_diagnostic_tail.runtime.role_surface.map_width = 50U;
    clamped_diagnostic_tail.camera.right = 900U;
    clamped_diagnostic_tail.roles[1].world_x = 320U;
    clamped_diagnostic_tail.roles[1].world_y = 240U;
    prime_at(clamped_diagnostic_tail, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FF8U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFCU, 4U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFEU, 4U);
    const auto clamped_tail_result = clamped_diagnostic_tail.step();
    test.expect_true(
        clamped_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            clamped_tail_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            clamped_tail_result.executed_instruction_count == 1U &&
            clamped_diagnostic_tail.camera_pan.remaining_x == -100 &&
            clamped_diagnostic_tail.camera_pan.remaining_y == 0 &&
            clamped_diagnostic_tail.camera_pan.step_x == 0 &&
            clamped_diagnostic_tail.camera_pan.step_y == 0 &&
            clamped_diagnostic_tail.context.instruction_offset == 0x7FF8U &&
            clamped_diagnostic_tail.state.previous_opcode == 0x55U,
        "opcode 73 clamp diagnostics preserve the original next-word overread"
    );

    CameraMoveFixture clamped_diagnostic_available;
    clamped_diagnostic_available.runtime.role_surface.map_width = 50U;
    clamped_diagnostic_available.camera.right = 900U;
    clamped_diagnostic_available.roles[1].world_x = 320U;
    clamped_diagnostic_available.roles[1].world_y = 240U;
    prime_role_camera_move(
        clamped_diagnostic_available,
        OP_73_START_CAMERA_MOVE_TO_ROLE,
        0x00F8U,
        4U,
        4U
    );
    const auto clamped_available_result = clamped_diagnostic_available.step();
    test.expect_true(
        clamped_available_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            clamped_available_result.opcode == kStoryVmTypedStop &&
            clamped_available_result.executed_instruction_count == 2U &&
            clamped_diagnostic_available.camera_pan.remaining_x == -100 &&
            clamped_diagnostic_available.camera_pan.remaining_y == 0 &&
            clamped_diagnostic_available.context.instruction_offset == 8U &&
            clamped_diagnostic_available.state.previous_opcode ==
                OP_73_START_CAMERA_MOVE_TO_ROLE,
        "opcode 73 clamp diagnostics consume an available next word only diagnostically"
    );
}

void test_wait_for_camera_move_protocol(openswd3::test::Context& test) {
    struct Variant {
        i32 remaining_x;
        i32 remaining_y;
        i32 step_x;
        i32 step_y;
    };
    constexpr std::array<Variant, 4U> variants{
        Variant{16, 0, 0, 0},
        Variant{0, -16, 0, 0},
        Variant{0, 0, 4, 0},
        Variant{0, 0, 0, -4},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        for (const Variant variant : variants) {
            CameraMoveFixture waiting;
            waiting.camera_pan.remaining_x = variant.remaining_x;
            waiting.camera_pan.remaining_y = variant.remaining_y;
            waiting.camera_pan.step_x = variant.step_x;
            waiting.camera_pan.step_y = variant.step_y;
            waiting.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                waiting,
                static_cast<u16>(OP_51_WAIT_CAMERA_MOVE_COMPLETE | mask)
            );
            write_u16(waiting.state.window, 2U, kStoryVmTypedStop);

            const auto result = waiting.step();

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                    result.executed_instruction_count == 1U &&
                    waiting.camera_pan.remaining_x == variant.remaining_x &&
                    waiting.camera_pan.remaining_y == variant.remaining_y &&
                    waiting.camera_pan.step_x == variant.step_x &&
                    waiting.camera_pan.step_y == variant.step_y &&
                    waiting.context.instruction_offset == 0U &&
                    waiting.state.previous_opcode ==
                        OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                    result.direct_audio_service_count == 1U &&
                    waiting.ports.direct_audio_service_count == 1U,
                "opcode 51 aliases wait on each camera movement field through the common audio-yield exit"
            );
        }

        CameraMoveFixture completed;
        completed.state.previous_opcode = 0x55U;
        prime_loaded_instruction(
            completed, static_cast<u16>(OP_51_WAIT_CAMERA_MOVE_COMPLETE | mask)
        );
        write_u16(completed.state.window, 2U, kStoryVmTypedStop);

        const auto result = completed.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                completed.context.instruction_offset == 2U &&
                completed.state.previous_opcode ==
                    OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                completed.ports.direct_audio_service_count == 0U,
            "opcode 51 aliases advance and continue when all movement fields are zero"
        );
    }

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(missing_owner, OP_51_WAIT_CAMERA_MOVE_COMPLETE);
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 51 requires the camera-pan owner before reading movement state"
    );

    CameraMoveFixture completed_tail;
    completed_tail.context.talk_data_offset = 0x1111U;
    completed_tail.context.instruction_offset = 0x7FFEU;
    completed_tail.state.loaded_file_number = 1U;
    completed_tail.state.loaded_data_offset = 0x1111U;
    completed_tail.state.window_loaded = true;
    completed_tail.state.previous_opcode = 0x55U;
    write_u16(
        completed_tail.state.window, 0x7FFEU, OP_51_WAIT_CAMERA_MOVE_COMPLETE
    );
    const auto completed_tail_result = completed_tail.step();
    test.expect_true(
        completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_51_WAIT_CAMERA_MOVE_COMPLETE,
        "opcode 51 exact tail publishes completion before the next fetch fails"
    );

    CameraMoveFixture waiting_tail;
    waiting_tail.context.talk_data_offset = 0x1111U;
    waiting_tail.context.instruction_offset = 0x7FFEU;
    waiting_tail.state.loaded_file_number = 1U;
    waiting_tail.state.loaded_data_offset = 0x1111U;
    waiting_tail.state.window_loaded = true;
    waiting_tail.state.previous_opcode = 0x55U;
    waiting_tail.camera_pan.step_y = -4;
    write_u16(
        waiting_tail.state.window, 0x7FFEU, OP_51_WAIT_CAMERA_MOVE_COMPLETE
    );
    const auto waiting_tail_result = waiting_tail.step();
    test.expect_true(
        waiting_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_tail_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_tail_result.executed_instruction_count == 1U &&
            waiting_tail.context.instruction_offset == 0x7FFEU &&
            waiting_tail.state.previous_opcode ==
                OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_tail.camera_pan.step_y == -4 &&
            waiting_tail_result.direct_audio_service_count == 1U &&
            waiting_tail.ports.direct_audio_service_count == 1U,
        "opcode 51 exact-tail wait publishes and audio-yields without advancing"
    );
}

void test_wait_for_camera_top_while_moving_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        CameraMoveFixture matching_x;
        matching_x.camera_pan.remaining_x = 16;
        prime_loaded_instruction(
            matching_x,
            static_cast<u16>(OP_191_WAIT_CAMERA_TOP_WHILE_MOVING | mask)
        );
        write_u16(matching_x.state.window, 2U, 0xFF10U);
        write_u16(matching_x.state.window, 4U, kStoryVmTypedStop);
        const auto matching_x_result = matching_x.step(0, -240);

        CameraMoveFixture mismatching_y;
        mismatching_y.camera_pan.remaining_y = -16;
        prime_loaded_instruction(
            mismatching_y,
            static_cast<u16>(OP_191_WAIT_CAMERA_TOP_WHILE_MOVING | mask)
        );
        write_u16(mismatching_y.state.window, 2U, 0xFF10U);
        write_u16(mismatching_y.state.window, 4U, kStoryVmTypedStop);
        const auto mismatching_y_result = mismatching_y.step(0, -239);

        test.expect_true(
            matching_x_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                matching_x_result.opcode == kStoryVmTypedStop &&
                matching_x_result.executed_instruction_count == 2U &&
                matching_x.context.instruction_offset == 4U &&
                matching_x.state.previous_opcode ==
                    OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
                matching_x.ports.direct_audio_service_count == 0U &&
                matching_x.camera_pan.remaining_x == 16 &&
                mismatching_y_result.status ==
                    LegacyWorldStoryVmStatus::yielded &&
                mismatching_y_result.opcode ==
                    OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
                mismatching_y_result.executed_instruction_count == 1U &&
                mismatching_y_result.direct_audio_service_count == 1U &&
                mismatching_y.context.instruction_offset == 0U &&
                mismatching_y.state.previous_opcode ==
                    OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
                mismatching_y.ports.direct_audio_service_count == 1U &&
                mismatching_y.camera_pan.remaining_y == -16,
            "opcode 191 aliases compare signed viewport top only while either camera displacement remains, continuing on equality and audio-yielding on mismatch"
        );
    }

    CameraMoveFixture step_only;
    step_only.camera_pan.step_x = 4;
    step_only.camera_pan.step_y = -4;
    step_only.runtime.camera = nullptr;
    prime_loaded_instruction(step_only, OP_191_WAIT_CAMERA_TOP_WHILE_MOVING);
    write_u16(step_only.state.window, 4U, kStoryVmTypedStop);
    const auto step_only_result = step_only.step();
    test.expect_true(
        step_only_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            step_only_result.executed_instruction_count == 2U &&
            step_only.context.instruction_offset == 4U &&
            step_only.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            step_only.camera_pan.step_x == 4 &&
            step_only.camera_pan.step_y == -4 &&
            step_only.ports.direct_audio_service_count == 0U,
        "opcode 191 ignores camera steps and does not access the operand or viewport owner when both displacement fields are zero"
    );

    Fixture missing_pan_owner;
    missing_pan_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        missing_pan_owner, OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    const auto missing_pan_result = missing_pan_owner.step();

    CameraMoveFixture missing_camera_owner;
    missing_camera_owner.camera_pan.remaining_x = 1;
    missing_camera_owner.runtime.camera = nullptr;
    missing_camera_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        missing_camera_owner, OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    write_u16(missing_camera_owner.state.window, 2U, 0U);
    const auto missing_camera_result = missing_camera_owner.step();
    test.expect_true(
        missing_pan_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_pan_owner.context.instruction_offset == 0U &&
            missing_pan_owner.state.previous_opcode == 0x55U &&
            missing_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_camera_owner.context.instruction_offset == 0U &&
            missing_camera_owner.state.previous_opcode == 0x55U &&
            missing_camera_owner.ports.direct_audio_service_count == 0U,
        "opcode 191 accesses the pan owner before its conditional operand and the viewport owner only after that operand"
    );

    CameraMoveFixture active_truncated;
    active_truncated.context.talk_data_offset = 0x1111U;
    active_truncated.context.instruction_offset = 0x7FFEU;
    active_truncated.state.loaded_file_number = 1U;
    active_truncated.state.loaded_data_offset = 0x1111U;
    active_truncated.state.window_loaded = true;
    active_truncated.state.previous_opcode = 0x55U;
    active_truncated.camera_pan.remaining_y = 1;
    write_u16(
        active_truncated.state.window,
        0x7FFEU,
        OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    const auto active_truncated_result = active_truncated.step();

    CameraMoveFixture inactive_unread_tail;
    inactive_unread_tail.context.talk_data_offset = 0x1111U;
    inactive_unread_tail.context.instruction_offset = 0x7FFEU;
    inactive_unread_tail.state.loaded_file_number = 1U;
    inactive_unread_tail.state.loaded_data_offset = 0x1111U;
    inactive_unread_tail.state.window_loaded = true;
    inactive_unread_tail.state.previous_opcode = 0x55U;
    inactive_unread_tail.runtime.camera = nullptr;
    write_u16(
        inactive_unread_tail.state.window,
        0x7FFEU,
        OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    const auto inactive_tail_result = inactive_unread_tail.step();
    test.expect_true(
        active_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            active_truncated.context.instruction_offset == 0x7FFEU &&
            active_truncated.state.previous_opcode == 0x55U &&
            active_truncated.ports.direct_audio_service_count == 0U &&
            inactive_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            inactive_unread_tail.context.instruction_offset == 0x8002U &&
            inactive_unread_tail.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            inactive_unread_tail.ports.direct_audio_service_count == 0U,
        "opcode 191 requires the Y operand only while displacement remains, while the inactive path consumes four bytes without reading the absent operand"
    );

    CameraMoveFixture matching_exact_tail;
    matching_exact_tail.context.talk_data_offset = 0x1111U;
    matching_exact_tail.context.instruction_offset = 0x7FFCU;
    matching_exact_tail.state.loaded_file_number = 1U;
    matching_exact_tail.state.loaded_data_offset = 0x1111U;
    matching_exact_tail.state.window_loaded = true;
    matching_exact_tail.state.previous_opcode = 0x55U;
    matching_exact_tail.camera_pan.remaining_x = 1;
    write_u16(
        matching_exact_tail.state.window,
        0x7FFCU,
        OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    write_u16(matching_exact_tail.state.window, 0x7FFEU, 0x8000U);
    const auto matching_tail_result = matching_exact_tail.step(0, -32768);

    CameraMoveFixture mismatching_exact_tail;
    mismatching_exact_tail.context.talk_data_offset = 0x1111U;
    mismatching_exact_tail.context.instruction_offset = 0x7FFCU;
    mismatching_exact_tail.state.loaded_file_number = 1U;
    mismatching_exact_tail.state.loaded_data_offset = 0x1111U;
    mismatching_exact_tail.state.window_loaded = true;
    mismatching_exact_tail.state.previous_opcode = 0x55U;
    mismatching_exact_tail.camera_pan.remaining_y = 1;
    write_u16(
        mismatching_exact_tail.state.window,
        0x7FFCU,
        OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
    );
    write_u16(mismatching_exact_tail.state.window, 0x7FFEU, 0x8000U);
    const auto mismatching_tail_result = mismatching_exact_tail.step(0, 32767);
    test.expect_true(
        matching_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            matching_exact_tail.context.instruction_offset == 0x8000U &&
            matching_exact_tail.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            matching_exact_tail.ports.direct_audio_service_count == 0U &&
            mismatching_tail_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            mismatching_exact_tail.context.instruction_offset == 0x7FFCU &&
            mismatching_exact_tail.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            mismatching_tail_result.direct_audio_service_count == 1U &&
            mismatching_exact_tail.ports.direct_audio_service_count == 1U,
        "opcode 191 exact four-byte tail commits equality before successor fetch failure but keeps mismatch in place through the common audio-yield exit"
    );
}

void test_wait_for_music_stream_transition_protocol(
    openswd3::test::Context& test
) {
    struct Case {
        u32 mode;
        u32 current_divisor;
        u16 expected_offset;
    };
    constexpr std::array<Case, 4U> cases{
        Case{2U, 0U, 0U},
        Case{1U, 9U, 0U},
        Case{1U, 0U, 2U},
        Case{0x00010002U, 0U, 2U},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        for (const Case test_case : cases) {
            Fixture fixture;
            fixture.state.current_first_stream = test_case.mode;
            fixture.state.current_stream_fade_divisor =
                test_case.current_divisor;
            fixture.state.current_second_stream = 0x55667788U;
            fixture.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_192_WAIT_MUSIC_STREAM_TRANSITION | mask)
            );
            write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
            u16 offset_at_audio{};
            u32 previous_at_audio{};
            fixture.ports.audio_service_callback = [&] {
                offset_at_audio = fixture.context.instruction_offset;
                previous_at_audio = fixture.state.previous_opcode;
            };

            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_192_WAIT_MUSIC_STREAM_TRANSITION &&
                    result.executed_instruction_count == 1U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.context.instruction_offset ==
                        test_case.expected_offset &&
                    offset_at_audio == test_case.expected_offset &&
                    fixture.state.previous_opcode ==
                        OP_192_WAIT_MUSIC_STREAM_TRANSITION &&
                    previous_at_audio == OP_192_WAIT_MUSIC_STREAM_TRANSITION &&
                    fixture.state.current_first_stream == test_case.mode &&
                    fixture.state.current_stream_fade_divisor ==
                        test_case.current_divisor &&
                    fixture.state.current_second_stream == 0x55667788U &&
                    fixture.ports.direct_audio_service_count == 1U,
                "opcode 192 aliases wait on full mode two or a nonzero current fade divisor, otherwise advancing before the common audio-yield exit"
            );
        }
    }

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    exact_tail.state.current_first_stream = 0U;
    exact_tail.state.current_stream_fade_divisor = 0U;
    write_u16(
        exact_tail.state.window, 0x7FFEU, OP_192_WAIT_MUSIC_STREAM_TRANSITION
    );
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_192_WAIT_MUSIC_STREAM_TRANSITION &&
            exact_tail.ports.direct_audio_service_count == 1U,
        "opcode 192 exact tail advances but audio-yields without fetching its successor"
    );
}

void test_wait_for_story_video_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<i32, 3U> active_values{
        0,
        1,
        std::numeric_limits<i32>::max(),
    };
    constexpr std::array<i32, 2U> completed_values{
        -1,
        std::numeric_limits<i32>::min(),
    };

    for (const u16 mask : alias_masks) {
        for (const i32 progress : active_values) {
            Fixture active;
            active.ports.video_progress = progress;
            active.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                active, static_cast<u16>(OP_193_WAIT_STORY_VIDEO | mask)
            );
            write_u16(active.state.window, 2U, kStoryVmTypedStop);
            u16 offset_at_query = 0xFFFFU;
            u32 previous_at_query{};
            u16 offset_at_audio = 0xFFFFU;
            u32 previous_at_audio{};
            active.ports.video_progress_callback = [&] {
                offset_at_query = active.context.instruction_offset;
                previous_at_query = active.state.previous_opcode;
            };
            active.ports.audio_service_callback = [&] {
                offset_at_audio = active.context.instruction_offset;
                previous_at_audio = active.state.previous_opcode;
            };

            const auto result = active.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_193_WAIT_STORY_VIDEO &&
                    result.executed_instruction_count == 1U &&
                    result.direct_audio_service_count == 1U &&
                    active.context.instruction_offset == 0U &&
                    active.state.previous_opcode == OP_193_WAIT_STORY_VIDEO &&
                    active.ports.video_progress_query_count == 1U &&
                    active.ports.direct_audio_service_count == 1U &&
                    offset_at_query == 0U && previous_at_query == 0x55U &&
                    offset_at_audio == 0U &&
                    previous_at_audio == OP_193_WAIT_STORY_VIDEO,
                "opcode 193 aliases query video progress before publication, then keep every nonnegative result in place through previous and one audio service"
            );
        }

        for (const i32 progress : completed_values) {
            Fixture completed;
            completed.ports.video_progress = progress;
            completed.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                completed, static_cast<u16>(OP_193_WAIT_STORY_VIDEO | mask)
            );
            write_u16(completed.state.window, 2U, kStoryVmTypedStop);
            u16 offset_at_query = 0xFFFFU;
            u32 previous_at_query{};
            completed.ports.video_progress_callback = [&] {
                offset_at_query = completed.context.instruction_offset;
                previous_at_query = completed.state.previous_opcode;
            };

            const auto result = completed.step();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 0U &&
                    completed.context.instruction_offset == 2U &&
                    completed.state.previous_opcode ==
                        OP_193_WAIT_STORY_VIDEO &&
                    completed.ports.video_progress_query_count == 1U &&
                    completed.ports.direct_audio_service_count == 0U &&
                    offset_at_query == 0U && previous_at_query == 0x55U,
                "opcode 193 aliases advance and same-call without audio for every negative legacy video progress result"
            );
        }
    }

    Fixture active_tail;
    active_tail.context.talk_data_offset = 0x1111U;
    active_tail.context.instruction_offset = 0x7FFEU;
    active_tail.state.loaded_file_number = 1U;
    active_tail.state.loaded_data_offset = 0x1111U;
    active_tail.state.window_loaded = true;
    active_tail.state.previous_opcode = 0x55U;
    active_tail.ports.video_progress = 0;
    write_u16(active_tail.state.window, 0x7FFEU, OP_193_WAIT_STORY_VIDEO);
    const auto active_tail_result = active_tail.step();

    Fixture completed_tail;
    completed_tail.context.talk_data_offset = 0x1111U;
    completed_tail.context.instruction_offset = 0x7FFEU;
    completed_tail.state.loaded_file_number = 1U;
    completed_tail.state.loaded_data_offset = 0x1111U;
    completed_tail.state.window_loaded = true;
    completed_tail.state.previous_opcode = 0x55U;
    completed_tail.ports.video_progress = -1;
    write_u16(completed_tail.state.window, 0x7FFEU, OP_193_WAIT_STORY_VIDEO);
    const auto completed_tail_result = completed_tail.step();
    test.expect_true(
        active_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            active_tail_result.executed_instruction_count == 1U &&
            active_tail_result.direct_audio_service_count == 1U &&
            active_tail.context.instruction_offset == 0x7FFEU &&
            active_tail.state.previous_opcode == OP_193_WAIT_STORY_VIDEO &&
            completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail_result.direct_audio_service_count == 0U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode == OP_193_WAIT_STORY_VIDEO,
        "opcode 193 exact tail audio-yields active video in place, while completed video commits IP and previous before same-call successor fetch failure"
    );
}

void test_latch_common_join_same_call_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.previous_opcode = 0x55U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_1024_LATCH_COMMON_JOIN_SAME_CALL | mask)
        );
        write_u16(fixture.state.window, 2U, OP_59_PLAY_SOUND_EFFECT);
        write_u16(fixture.state.window, 4U, 0x1234U);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 3U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{0x1234U} &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 1024 aliases advance, publish themselves, latch common joins to same-call, and suppress the following sound handler common audio"
        );
    }

    Fixture persistent;
    prime_loaded_instruction(persistent, OP_1024_LATCH_COMMON_JOIN_SAME_CALL);
    write_u16(persistent.state.window, 2U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(persistent.state.window, 4U, 0x1111U);
    write_u16(persistent.state.window, 6U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(persistent.state.window, 8U, 0x2222U);
    write_u16(persistent.state.window, 10U, kStoryVmTypedStop);
    const auto persistent_result = persistent.step();
    test.expect_true(
        persistent_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            persistent_result.executed_instruction_count == 4U &&
            persistent_result.direct_audio_service_count == 0U &&
            persistent.context.instruction_offset == 10U &&
            persistent.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            persistent.ports.sound_effect_requests ==
                std::vector<u16>{0x1111U, 0x2222U} &&
            persistent.ports.direct_audio_service_count == 0U,
        "opcode 1024 call-local latch survives multiple common joins instead of behaving as one-shot ESI"
    );

    persistent.context.instruction_offset = 2U;
    const auto next_call_result = persistent.step();
    test.expect_true(
        next_call_result.status == LegacyWorldStoryVmStatus::yielded &&
            next_call_result.opcode == OP_59_PLAY_SOUND_EFFECT &&
            next_call_result.executed_instruction_count == 1U &&
            next_call_result.direct_audio_service_count == 1U &&
            persistent.context.instruction_offset == 6U &&
            persistent.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            persistent.ports.sound_effect_requests ==
                std::vector<u16>{0x1111U, 0x2222U, 0x1111U} &&
            persistent.ports.direct_audio_service_count == 1U,
        "opcode 1024 latch expires with the interpreter call and does not leak into a later step"
    );

    Fixture internal_audio;
    prime_loaded_instruction(
        internal_audio, OP_1024_LATCH_COMMON_JOIN_SAME_CALL
    );
    write_u16(internal_audio.state.window, 2U, OP_135_RESET_INPUT_MENU_STATE);
    write_u16(internal_audio.state.window, 4U, kStoryVmTypedStop);
    const auto internal_audio_result = internal_audio.step();
    test.expect_true(
        internal_audio_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            internal_audio_result.executed_instruction_count == 3U &&
            internal_audio_result.direct_audio_service_count == 1U &&
            internal_audio.context.instruction_offset == 4U &&
            internal_audio.state.previous_opcode ==
                OP_135_RESET_INPUT_MENU_STATE &&
            internal_audio.ports.input_menu_reset_count == 1U &&
            internal_audio.ports.story_protocol_events ==
                std::vector<u32>{14U, 2U},
        "opcode 1024 suppresses only opcode 135 common audio while preserving its handler-internal audio"
    );

    Fixture stationary_wait;
    openswd3::rendering::LegacyFrameColorTransitionState color{};
    color.countdown = 1;
    stationary_wait.runtime.frame_color = &color;
    prime_loaded_instruction(
        stationary_wait, OP_1024_LATCH_COMMON_JOIN_SAME_CALL
    );
    write_u16(
        stationary_wait.state.window, 2U, OP_53_WAIT_FRAME_COLOR_TRANSITION
    );
    const auto stationary_result = stationary_wait.step();
    test.expect_true(
        stationary_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            stationary_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            stationary_result.executed_instruction_count == 4096U &&
            stationary_result.direct_audio_service_count == 0U &&
            stationary_wait.context.instruction_offset == 2U &&
            stationary_wait.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            stationary_wait.ports.direct_audio_service_count == 0U,
        "opcode 1024 keeps a nonadvancing common-wait handler in the same call until the modern dispatch guard stops the original infinite loop domain"
    );

    for (const u16 mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x55U;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_1024_LATCH_COMMON_JOIN_SAME_CALL | mask)
        );

        const auto result = exact_tail.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.opcode == OP_1024_LATCH_COMMON_JOIN_SAME_CALL &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 0U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_1024_LATCH_COMMON_JOIN_SAME_CALL &&
                exact_tail.ports.direct_audio_service_count == 0U,
            "opcode 1024 exact tail commits IP, previous, and the call-local latch before successor fetch failure"
        );
    }
}

void test_clear_common_join_latch_and_yield_protocol(
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
        fixture.state.previous_opcode = 0x55U;
        const u16 raw_word =
            static_cast<u16>(OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD | mask);
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_59_PLAY_SOUND_EFFECT);
        write_u16(fixture.state.window, 4U, 0x2222U);
        u16 offset_at_audio{};
        u16 previous_at_audio{};
        fixture.ports.audio_service_callback = [&] {
            offset_at_audio = fixture.context.instruction_offset;
            previous_at_audio = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == raw_word &&
                result.opcode == OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
                fixture.ports.sound_effect_requests.empty() &&
                fixture.ports.direct_audio_service_count == 1U &&
                offset_at_audio == 2U &&
                previous_at_audio == OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD,
            "opcode 1025 aliases advance and publish before one audio service, yield, and leave the successor unread"
        );
    }

    Fixture latched;
    prime_loaded_instruction(latched, OP_1024_LATCH_COMMON_JOIN_SAME_CALL);
    write_u16(latched.state.window, 2U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(latched.state.window, 4U, 0x1111U);
    write_u16(
        latched.state.window, 6U, OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD
    );
    write_u16(latched.state.window, 8U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(latched.state.window, 10U, 0x2222U);
    u16 offset_at_audio{};
    u16 previous_at_audio{};
    latched.ports.audio_service_callback = [&] {
        offset_at_audio = latched.context.instruction_offset;
        previous_at_audio = latched.state.previous_opcode;
    };

    const auto latched_result = latched.step();
    test.expect_true(
        latched_result.status == LegacyWorldStoryVmStatus::yielded &&
            latched_result.opcode ==
                OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
            latched_result.executed_instruction_count == 3U &&
            latched_result.direct_audio_service_count == 1U &&
            latched.context.instruction_offset == 8U &&
            latched.state.previous_opcode ==
                OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
            latched.ports.sound_effect_requests == std::vector<u16>{0x1111U} &&
            latched.ports.direct_audio_service_count == 1U &&
            offset_at_audio == 8U &&
            previous_at_audio == OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD,
        "opcode 1025 clears the persistent opcode 1024 latch, services common audio once, yields, and leaves its successor unread"
    );

    for (const u16 mask : alias_masks) {
        Fixture exact_tail;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x55U;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            static_cast<u16>(OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD | mask)
        );

        const auto result = exact_tail.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode ==
                    OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD &&
                exact_tail.ports.direct_audio_service_count == 1U,
            "opcode 1025 exact tail commits IP and previous, services audio, and yields without fetching a successor"
        );
    }
}

void test_start_frame_color_transition_protocol(openswd3::test::Context& test) {
    constexpr std::array<i16, 6U> components{-30, 5, 17, 0, -25, 2};
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        openswd3::rendering::LegacyFrameColorTransitionState color{};
        fixture.runtime.frame_color = &color;
        fixture.state.previous_opcode = 0x55U;
        prime_frame_color_transition(
            fixture,
            static_cast<u16>(OP_52_START_FRAME_COLOR_TRANSITION | mask),
            components,
            6U
        );

        const auto result = fixture.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                color.current_red == -30.0F && color.current_green == 5.0F &&
                color.current_blue == 17.0F && color.target_red == 0.0F &&
                color.target_green == -25.0F && color.target_blue == 2.0F &&
                color.countdown == 6 && color.step_red == 5.0F &&
                color.step_green == -5.0F && color.step_blue == -2.5F &&
                fixture.context.instruction_offset == 16U &&
                fixture.state.previous_opcode ==
                    OP_52_START_FRAME_COLOR_TRANSITION &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 52 aliases initialize all color-transition values and continue"
        );
    }

    Fixture maximum_duration;
    openswd3::rendering::LegacyFrameColorTransitionState maximum_color{};
    maximum_duration.runtime.frame_color = &maximum_color;
    prime_frame_color_transition(
        maximum_duration,
        OP_52_START_FRAME_COLOR_TRANSITION,
        std::array<i16, 6U>{-32768, -32768, -32768, 32767, 32767, 32767},
        0xFFFFU
    );
    const auto maximum_result = maximum_duration.step();
    test.expect_true(
        maximum_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            maximum_color.countdown == 65535 &&
            maximum_color.step_red == 1.0F &&
            maximum_color.step_green == 1.0F && maximum_color.step_blue == 1.0F,
        "opcode 52 zero-extends duration and preserves full signed component range"
    );

    Fixture zero_duration;
    openswd3::rendering::LegacyFrameColorTransitionState zero_color{};
    zero_duration.runtime.frame_color = &zero_color;
    prime_frame_color_transition(
        zero_duration,
        OP_52_START_FRAME_COLOR_TRANSITION,
        std::array<i16, 6U>{0, 0, 5, 1, -1, 5},
        0U
    );
    const auto zero_result = zero_duration.step();
    test.expect_true(
        zero_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            zero_color.countdown == 0 &&
            std::bit_cast<u32>(zero_color.step_red) == 0x7F800000U &&
            std::bit_cast<u32>(zero_color.step_green) == 0xFF800000U &&
            std::bit_cast<u32>(zero_color.step_blue) == 0xFFC00000U &&
            zero_duration.context.instruction_offset == 16U &&
            zero_duration.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION,
        "opcode 52 zero duration reproduces x87 infinity and indefinite-NaN bits"
    );

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_frame_color_transition(
        missing_owner, OP_52_START_FRAME_COLOR_TRANSITION, components, 6U
    );
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 52 accesses the color owner after reading the first component"
    );
}

void test_start_frame_color_transition_window_boundaries(
    openswd3::test::Context& test
) {
    constexpr std::array<i16, 6U> components{-30, 5, 17, 0, -25, 2};

    for (std::size_t available = 0U; available < 7U; ++available) {
        Fixture fixture;
        openswd3::rendering::LegacyFrameColorTransitionState color{
            .countdown = 77,
            .current_red = 101.0F,
            .current_green = 102.0F,
            .current_blue = 103.0F,
            .target_red = 201.0F,
            .target_green = 202.0F,
            .target_blue = 203.0F,
            .step_red = 301.0F,
            .step_green = 302.0F,
            .step_blue = 303.0F,
        };
        fixture.runtime.frame_color = &color;
        const u16 offset = static_cast<u16>(0x7FFEU - available * 2U);
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = offset;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x55U;
        write_u16(
            fixture.state.window, offset, OP_52_START_FRAME_COLOR_TRANSITION
        );
        for (std::size_t index = 0U;
             index < available && index < components.size();
             ++index) {
            write_u16(
                fixture.state.window,
                offset + 2U + index * 2U,
                static_cast<u16>(components[index])
            );
        }

        const auto result = fixture.step();
        const bool all_targets_available = available >= components.size();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
                result.executed_instruction_count == 1U &&
                color.current_red == (available >= 1U ? -30.0F : 101.0F) &&
                color.current_green == (available >= 2U ? 5.0F : 102.0F) &&
                color.current_blue == (available >= 3U ? 17.0F : 103.0F) &&
                color.target_red == (all_targets_available ? 0.0F : 201.0F) &&
                color.target_green ==
                    (all_targets_available ? -25.0F : 202.0F) &&
                color.target_blue == (all_targets_available ? 2.0F : 203.0F) &&
                color.countdown == 77 && color.step_red == 301.0F &&
                color.step_green == 302.0F && color.step_blue == 303.0F &&
                fixture.context.instruction_offset == offset &&
                fixture.state.previous_opcode == 0x55U,
            "opcode 52 truncations preserve staged current and grouped target writes"
        );
    }

    Fixture exact_tail;
    openswd3::rendering::LegacyFrameColorTransitionState exact_color{};
    exact_tail.runtime.frame_color = &exact_color;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FF0U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    write_u16(
        exact_tail.state.window, 0x7FF0U, OP_52_START_FRAME_COLOR_TRANSITION
    );
    for (std::size_t index = 0U; index < components.size(); ++index) {
        write_u16(
            exact_tail.state.window,
            0x7FF2U + index * 2U,
            static_cast<u16>(components[index])
        );
    }
    write_u16(exact_tail.state.window, 0x7FFEU, 6U);

    const auto exact_result = exact_tail.step();

    test.expect_true(
        exact_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
            exact_result.executed_instruction_count == 1U &&
            exact_color.countdown == 6 && exact_color.step_red == 5.0F &&
            exact_color.step_green == -5.0F && exact_color.step_blue == -2.5F &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION,
        "opcode 52 exact tail completes all effects before the next fetch fails"
    );
}

void test_wait_for_frame_color_transition_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<i32, 2U> waiting_values{1, 0x7FFFFFFF};
    constexpr std::array<i32, 3U> completed_values{
        0,
        -1,
        -2147483647 - 1,
    };

    for (const u16 mask : alias_masks) {
        for (const i32 countdown : waiting_values) {
            Fixture fixture;
            openswd3::rendering::LegacyFrameColorTransitionState color{
                .countdown = countdown,
                .current_red = 101.0F,
                .current_green = 102.0F,
                .current_blue = 103.0F,
                .target_red = 201.0F,
                .target_green = 202.0F,
                .target_blue = 203.0F,
                .step_red = 301.0F,
                .step_green = 302.0F,
                .step_blue = 303.0F,
            };
            fixture.runtime.frame_color = &color;
            fixture.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_53_WAIT_FRAME_COLOR_TRANSITION | mask)
            );

            const auto result = fixture.step();

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    result.executed_instruction_count == 1U &&
                    color.countdown == countdown &&
                    color.current_red == 101.0F &&
                    color.current_green == 102.0F &&
                    color.current_blue == 103.0F &&
                    color.target_red == 201.0F &&
                    color.target_green == 202.0F &&
                    color.target_blue == 203.0F && color.step_red == 301.0F &&
                    color.step_green == 302.0F && color.step_blue == 303.0F &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode ==
                        OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    fixture.ports.direct_audio_service_count == 1U,
                "opcode 53 aliases wait in place for every positive signed countdown"
            );
        }

        for (const i32 countdown : completed_values) {
            Fixture fixture;
            openswd3::rendering::LegacyFrameColorTransitionState color{
                .countdown = countdown,
                .current_red = 101.0F,
                .current_green = 102.0F,
                .current_blue = 103.0F,
                .target_red = 201.0F,
                .target_green = 202.0F,
                .target_blue = 203.0F,
                .step_red = 301.0F,
                .step_green = 302.0F,
                .step_blue = 303.0F,
            };
            fixture.runtime.frame_color = &color;
            fixture.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_53_WAIT_FRAME_COLOR_TRANSITION | mask)
            );
            write_u16(fixture.state.window, 2U, kStoryVmTypedStop);

            const auto result = fixture.step();

            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 2U &&
                    color.countdown == countdown &&
                    color.current_red == 101.0F &&
                    color.current_green == 102.0F &&
                    color.current_blue == 103.0F &&
                    color.target_red == 201.0F &&
                    color.target_green == 202.0F &&
                    color.target_blue == 203.0F && color.step_red == 301.0F &&
                    color.step_green == 302.0F && color.step_blue == 303.0F &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.previous_opcode ==
                        OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcode 53 aliases complete and continue for every non-positive signed countdown"
            );
        }
    }

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(missing_owner, OP_53_WAIT_FRAME_COLOR_TRANSITION);
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 53 missing color owner stops at the original countdown access"
    );

    Fixture waiting_tail;
    openswd3::rendering::LegacyFrameColorTransitionState waiting_color{};
    waiting_color.countdown = 1;
    waiting_tail.runtime.frame_color = &waiting_color;
    waiting_tail.context.talk_data_offset = 0x1111U;
    waiting_tail.context.instruction_offset = 0x7FFEU;
    waiting_tail.state.loaded_file_number = 1U;
    waiting_tail.state.loaded_data_offset = 0x1111U;
    waiting_tail.state.window_loaded = true;
    waiting_tail.state.previous_opcode = 0x55U;
    write_u16(
        waiting_tail.state.window, 0x7FFEU, OP_53_WAIT_FRAME_COLOR_TRANSITION
    );
    const auto waiting_tail_result = waiting_tail.step();
    test.expect_true(
        waiting_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_tail_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            waiting_tail_result.executed_instruction_count == 1U &&
            waiting_tail.context.instruction_offset == 0x7FFEU &&
            waiting_tail.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION,
        "opcode 53 waiting tail publishes previous and remains at 0x7FFE"
    );

    Fixture completed_tail;
    openswd3::rendering::LegacyFrameColorTransitionState completed_color{};
    completed_tail.runtime.frame_color = &completed_color;
    completed_tail.context.talk_data_offset = 0x1111U;
    completed_tail.context.instruction_offset = 0x7FFEU;
    completed_tail.state.loaded_file_number = 1U;
    completed_tail.state.loaded_data_offset = 0x1111U;
    completed_tail.state.window_loaded = true;
    completed_tail.state.previous_opcode = 0x55U;
    write_u16(
        completed_tail.state.window, 0x7FFEU, OP_53_WAIT_FRAME_COLOR_TRANSITION
    );
    const auto completed_tail_result = completed_tail.step();
    test.expect_true(
        completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION,
        "opcode 53 completed tail publishes previous before the next fetch fails"
    );
}

void test_repeat_role_action_refresh_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime_instruction = [](Fixture& fixture,
                                      const u16 raw_word,
                                      const u16 selector,
                                      const i16 repeat_count) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, static_cast<u16>(repeat_count));
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x55U;
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        auto& action = fixture.roles[1].action;
        action.command_cursor = 0x1111U;
        action.wait_remaining = 0x2222U;
        action.field_58 = 0x3333U;
        std::vector<std::tuple<u16, u16, u16>> snapshots;
        fixture.ports.action_update_callback =
            [&snapshots](
                openswd3::asset_runtime::LegacyActionRecord& updated,
                const u32 update_index
            ) {
                snapshots.emplace_back(
                    updated.wait_remaining,
                    updated.command_cursor,
                    updated.field_58
                );
                updated.wait_remaining =
                    static_cast<u16>(0x4000U + update_index);
                updated.command_cursor =
                    static_cast<u16>(0x5000U + update_index);
                updated.field_58 = static_cast<u16>(0x6000U + update_index);
            };
        prime_instruction(
            fixture,
            static_cast<u16>(OP_54_REPEAT_ROLE_ACTION_REFRESH | mask),
            0x00F8U,
            2
        );

        const auto result = fixture.step();

        const std::vector<std::tuple<u16, u16, u16>> expected{
            {0U, 0U, 0x3333U},
            {0U, 0x5001U, 0x6001U},
            {0U, 0x5002U, 0U},
        };
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 3U &&
                result.action_update_failure_count == 0U &&
                snapshots == expected && action.wait_remaining == 0x4003U &&
                action.command_cursor == 0x5003U && action.field_58 == 0U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_54_REPEAT_ROLE_ACTION_REFRESH &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 54 aliases preserve initial and repeated refresh ordering"
        );
    }

    constexpr std::array<i16, 3U> non_positive_repeats{
        static_cast<i16>(0x8000U),
        -1,
        0,
    };
    for (const i16 repeat_count : non_positive_repeats) {
        Fixture fixture;
        auto& action = fixture.roles[1].action;
        action.command_cursor = 0x1111U;
        action.wait_remaining = 0x2222U;
        action.field_58 = 0x3333U;
        std::tuple<u16, u16, u16> snapshot{};
        fixture.ports.action_update_callback =
            [&snapshot](
                openswd3::asset_runtime::LegacyActionRecord& updated, const u32
            ) {
                snapshot = {
                    updated.wait_remaining,
                    updated.command_cursor,
                    updated.field_58,
                };
                updated.wait_remaining = 0x4444U;
                updated.command_cursor = 0x5555U;
                updated.field_58 = 0x6666U;
            };
        prime_instruction(
            fixture, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x00F8U, repeat_count
        );

        const auto result = fixture.step();

        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                snapshot == std::tuple<u16, u16, u16>{0U, 0U, 0x3333U} &&
                action.command_cursor == 0x5555U &&
                action.wait_remaining == 0x4444U &&
                action.field_58 == 0x6666U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_54_REPEAT_ROLE_ACTION_REFRESH,
            "opcode 54 non-positive signed repeat keeps the initial refresh writeback"
        );
    }

    Fixture update_failure;
    auto& failed_action = update_failure.roles[1].action;
    failed_action.command_cursor = 0x1111U;
    failed_action.wait_remaining = 0x2222U;
    failed_action.field_58 = 0x3333U;
    update_failure.ports.action_update_result = 0U;
    update_failure.ports.action_update_callback =
        [](openswd3::asset_runtime::LegacyActionRecord& updated,
           const u32 update_index) {
            updated.wait_remaining = static_cast<u16>(0x7000U + update_index);
            updated.command_cursor = static_cast<u16>(0x7100U + update_index);
            updated.field_58 = static_cast<u16>(0x7200U + update_index);
        };
    prime_instruction(
        update_failure, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x00F8U, 2
    );
    const auto failure_result = update_failure.step();
    test.expect_true(
        failure_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            failure_result.action_update_count == 3U &&
            failure_result.action_update_failure_count == 3U &&
            failed_action.command_cursor == 0x7103U &&
            failed_action.wait_remaining == 0x7003U &&
            failed_action.field_58 == 0U &&
            update_failure.context.instruction_offset == 6U &&
            update_failure.state.previous_opcode ==
                OP_54_REPEAT_ROLE_ACTION_REFRESH,
        "opcode 54 refresh failures are diagnostic-only across every iteration"
    );

    Fixture source_selector;
    source_selector.roles[1].action.command_cursor = 0x1111U;
    source_selector.roles[2].guid = 0xFFF0U;
    source_selector.roles[2].action.command_cursor = 0x2222U;
    prime_instruction(
        source_selector, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0xFFF0U, 0
    );
    const auto source_result = source_selector.step();
    test.expect_true(
        source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            source_result.action_update_count == 1U &&
            source_selector.roles[1].action.command_cursor == 0U &&
            source_selector.roles[2].action.command_cursor == 0x2222U,
        "opcode 54 translates FFF0 to the talk source before lookup"
    );

    Fixture controlled_selector;
    controlled_selector.roles[1].guid = 0xFFFEU;
    controlled_selector.roles[1].action.command_cursor = 0x1111U;
    controlled_selector.roles[2].action.command_cursor = 0x2222U;
    prime_instruction(
        controlled_selector, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0xFFFEU, 0
    );
    const auto controlled_result = controlled_selector.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled_result.action_update_count == 1U &&
            controlled_selector.roles[1].action.command_cursor == 0x1111U &&
            controlled_selector.roles[2].action.command_cursor == 0U,
        "opcode 54 passes FFFE through for controlled-role selection"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    selector_truncated.state.previous_opcode = 0x55U;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    const auto selector_truncated_result = selector_truncated.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated_result.action_update_count == 0U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x55U,
        "opcode 54 stops at the unsafe selector-word access"
    );

    Fixture repeat_truncated;
    repeat_truncated.context.instruction_offset = 0x7FFCU;
    repeat_truncated.context.talk_data_offset = 0x1111U;
    repeat_truncated.state.loaded_file_number = 1U;
    repeat_truncated.state.loaded_data_offset = 0x1111U;
    repeat_truncated.state.window_loaded = true;
    repeat_truncated.state.previous_opcode = 0x55U;
    write_u16(
        repeat_truncated.state.window, 0x7FFCU, OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    write_u16(repeat_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto repeat_truncated_result = repeat_truncated.step();
    test.expect_true(
        repeat_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            repeat_truncated_result.action_update_count == 0U &&
            repeat_truncated.context.instruction_offset == 0x7FFCU &&
            repeat_truncated.state.previous_opcode == 0x55U,
        "opcode 54 performs missing-role lookup before the unsafe repeat read"
    );

    Fixture missing_role;
    missing_role.roles[0].action.command_cursor = 0x1111U;
    prime_instruction(
        missing_role, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x7777U, 2
    );
    const auto missing_result = missing_role.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            missing_result.executed_instruction_count == 1U &&
            missing_result.action_update_count == 0U &&
            missing_role.roles[0].action.command_cursor == 0x1111U &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U,
        "opcode 54 missing role stops at the first unsafe action-field write"
    );

    Fixture exact_tail;
    auto& tail_action = exact_tail.roles[1].action;
    tail_action.command_cursor = 0x1111U;
    tail_action.wait_remaining = 0x2222U;
    tail_action.field_58 = 0x3333U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 1U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            tail_result.opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            tail_result.executed_instruction_count == 1U &&
            tail_result.action_update_count == 2U &&
            tail_action.command_cursor == 0U &&
            tail_action.wait_remaining == 0U && tail_action.field_58 == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_54_REPEAT_ROLE_ACTION_REFRESH,
        "opcode 54 exact tail completes every refresh before next-fetch failure"
    );
}

void test_shared_role_spatial_group_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    struct GroupCase {
        u16 opcode;
        u32 old_group;
        u32 target_group;
    };
    constexpr std::array<GroupCase, 3U> group_cases{
        GroupCase{OP_55_SET_ROLE_SPATIAL_GROUP_1, 0U, 1U},
        GroupCase{OP_56_SET_ROLE_SPATIAL_GROUP_0, 2U, 0U},
        GroupCase{OP_57_SET_ROLE_SPATIAL_GROUP_2, 1U, 2U},
    };
    constexpr std::size_t role_row =
        openswd3::world_map::kLegacySpatialRowPadding + 2U;
    const auto reset_spatial =
        [](openswd3::world_map::LegacyRoleSpatialIndex& spatial) {
            spatial.map_height = 4U;
            for (auto& group : spatial.row_heads) {
                group.assign(44U, 0U);
            }
        };
    const auto prime_instruction =
        [](Fixture& fixture, const u16 raw_word, const u16 selector) {
            prime_loaded_instruction(fixture, raw_word);
            write_u16(fixture.state.window, 2U, selector);
            fixture.state.previous_opcode = 0x66U;
        };

    for (const GroupCase group_case : group_cases) {
        for (const u16 mask : alias_masks) {
            Fixture fixture;
            openswd3::world_map::LegacyRoleSpatialIndex spatial;
            reset_spatial(spatial);
            auto& role = fixture.roles[1];
            role.world_y = 32U;
            role.flags = 0xA5A50000U | group_case.old_group;
            const bool inserted =
                openswd3::world_map::insert_legacy_role_spatially(
                    spatial, fixture.roles, 1U, group_case.old_group
                );
            fixture.runtime.spatial_index = &spatial;
            prime_instruction(
                fixture, static_cast<u16>(group_case.opcode | mask), 0x00F8U
            );

            const auto result = fixture.step();

            test.expect_true(
                inserted &&
                    result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == group_case.opcode &&
                    result.executed_instruction_count == 1U &&
                    role.flags == (0xA5A50000U | group_case.target_group) &&
                    spatial.row_heads[group_case.old_group][role_row] == 0U &&
                    spatial.row_heads[group_case.target_group][role_row] ==
                        1U &&
                    role.spatial_next_link_32 == 0U &&
                    fixture.context.instruction_offset == 4U &&
                    fixture.state.previous_opcode == group_case.opcode &&
                    fixture.ports.direct_audio_service_count == 1U,
                "opcodes 55-57 aliases move the role from its old group to the requested group"
            );
        }
    }

    Fixture source_selector;
    openswd3::world_map::LegacyRoleSpatialIndex source_spatial;
    reset_spatial(source_spatial);
    source_selector.roles[1].world_y = 32U;
    source_selector.roles[1].flags = 0U;
    source_selector.roles[2].guid = 0xFFF0U;
    source_selector.roles[2].world_y = 32U;
    source_selector.roles[2].flags = 0U;
    const bool source_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            source_spatial, source_selector.roles, 1U, 0U
        );
    source_selector.runtime.spatial_index = &source_spatial;
    prime_instruction(source_selector, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0xFFF0U);
    const auto source_result = source_selector.step();
    test.expect_true(
        source_inserted &&
            source_result.status == LegacyWorldStoryVmStatus::yielded &&
            (source_selector.roles[1].flags & 3U) == 1U &&
            (source_selector.roles[2].flags & 3U) == 0U &&
            source_spatial.row_heads[1U][role_row] == 1U,
        "shared role group handler translates FFF0 to the talk source"
    );

    Fixture controlled_selector;
    openswd3::world_map::LegacyRoleSpatialIndex controlled_spatial;
    reset_spatial(controlled_spatial);
    controlled_selector.roles[1].guid = 0xFFFEU;
    controlled_selector.roles[1].world_y = 32U;
    controlled_selector.roles[1].flags = 0U;
    controlled_selector.roles[2].world_y = 32U;
    controlled_selector.roles[2].flags = 0U;
    const bool controlled_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            controlled_spatial, controlled_selector.roles, 2U, 0U
        );
    controlled_selector.runtime.spatial_index = &controlled_spatial;
    prime_instruction(
        controlled_selector, OP_57_SET_ROLE_SPATIAL_GROUP_2, 0xFFFEU
    );
    const auto controlled_result = controlled_selector.step(0, 0, 2U);
    test.expect_true(
        controlled_inserted &&
            controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            (controlled_selector.roles[1].flags & 3U) == 0U &&
            (controlled_selector.roles[2].flags & 3U) == 2U &&
            controlled_spatial.row_heads[2U][role_row] == 2U,
        "shared role group handler passes FFFE through for controlled-role selection"
    );

    Fixture absent_from_chain;
    openswd3::world_map::LegacyRoleSpatialIndex empty_spatial;
    reset_spatial(empty_spatial);
    absent_from_chain.roles[1].world_y = 32U;
    absent_from_chain.roles[1].flags = 0U;
    absent_from_chain.runtime.spatial_index = &empty_spatial;
    prime_instruction(
        absent_from_chain, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto absent_result = absent_from_chain.step();
    test.expect_true(
        absent_result.status == LegacyWorldStoryVmStatus::yielded &&
            (absent_from_chain.roles[1].flags & 3U) == 1U &&
            empty_spatial.row_heads[0U][role_row] == 0U &&
            empty_spatial.row_heads[1U][role_row] == 0U &&
            absent_from_chain.context.instruction_offset == 4U &&
            absent_from_chain.state.previous_opcode ==
                OP_55_SET_ROLE_SPATIAL_GROUP_1,
        "shared role group handler treats a missing spatial-chain node as diagnostic-only"
    );

    Fixture logical_y;
    openswd3::world_map::LegacyRoleSpatialIndex logical_spatial;
    reset_spatial(logical_spatial);
    logical_y.roles[1].world_y = 0xFFFFFFFFU;
    logical_y.roles[1].flags = 0U;
    const bool logical_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            logical_spatial, logical_y.roles, 1U, 0U
        );
    logical_y.runtime.spatial_index = &logical_spatial;
    prime_instruction(logical_y, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U);
    const auto logical_result = logical_y.step();
    const std::size_t zero_row = openswd3::world_map::kLegacySpatialRowPadding;
    test.expect_true(
        logical_inserted &&
            logical_result.status == LegacyWorldStoryVmStatus::yielded &&
            (logical_y.roles[1].flags & 3U) == 1U &&
            logical_spatial.row_heads[0U][zero_row] == 1U &&
            logical_spatial.row_heads[1U][zero_row] == 0U,
        "shared role group handler preserves logical Y shift before the diagnostic-only miss"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    selector_truncated.state.previous_opcode = 0x66U;
    selector_truncated.roles[1].flags = 0x12345678U;
    write_u16(
        selector_truncated.state.window, 0x7FFEU, OP_55_SET_ROLE_SPATIAL_GROUP_1
    );
    const auto truncated_result = selector_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.roles[1].flags == 0x12345678U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x66U,
        "shared role group handler stops at the unsafe selector read"
    );

    Fixture missing_role;
    missing_role.context.instruction_offset = 0x7FFCU;
    missing_role.context.talk_data_offset = 0x1111U;
    missing_role.state.loaded_file_number = 1U;
    missing_role.state.loaded_data_offset = 0x1111U;
    missing_role.state.window_loaded = true;
    missing_role.state.previous_opcode = 0x66U;
    missing_role.roles[0].flags = 0x12345678U;
    write_u16(
        missing_role.state.window, 0x7FFCU, OP_55_SET_ROLE_SPATIAL_GROUP_1
    );
    write_u16(missing_role.state.window, 0x7FFEU, 0x7777U);
    const auto missing_result = missing_role.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_55_SET_ROLE_SPATIAL_GROUP_1 &&
            missing_result.executed_instruction_count == 1U &&
            missing_role.roles[0].flags == 0x12345678U &&
            missing_role.context.instruction_offset == 0x7FFCU &&
            missing_role.state.previous_opcode == 0x66U,
        "shared role group handler isolates the original negative role index before flags access"
    );

    Fixture missing_owner;
    missing_owner.roles[1].world_y = 32U;
    missing_owner.roles[1].flags = 0xA5A50000U;
    prime_instruction(missing_owner, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U);
    const auto owner_result = missing_owner.step();
    test.expect_true(
        owner_result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner.roles[1].flags == 0xA5A50001U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x66U,
        "shared role group handler updates flags before the missing spatial owner boundary"
    );

    Fixture invalid_old_group;
    openswd3::world_map::LegacyRoleSpatialIndex invalid_group_spatial;
    reset_spatial(invalid_group_spatial);
    invalid_old_group.roles[1].world_y = 32U;
    invalid_old_group.roles[1].flags = 0xA5A50003U;
    invalid_old_group.runtime.spatial_index = &invalid_group_spatial;
    prime_instruction(
        invalid_old_group, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto invalid_group_result = invalid_old_group.step();
    test.expect_true(
        invalid_group_result.status ==
                LegacyWorldStoryVmStatus::role_spatial_relocation_failed &&
            invalid_old_group.roles[1].flags == 0xA5A50001U &&
            invalid_old_group.context.instruction_offset == 0U &&
            invalid_old_group.state.previous_opcode == 0x66U,
        "shared role group handler preserves the flags write before invalid old-group isolation"
    );

    Fixture reinsertion_failure;
    openswd3::world_map::LegacyRoleSpatialIndex broken_spatial;
    reset_spatial(broken_spatial);
    reinsertion_failure.roles[1].world_y = 32U;
    reinsertion_failure.roles[1].flags = 0U;
    const bool broken_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            broken_spatial, reinsertion_failure.roles, 1U, 0U
        );
    broken_spatial.row_heads[1U].clear();
    reinsertion_failure.runtime.spatial_index = &broken_spatial;
    prime_instruction(
        reinsertion_failure, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto reinsertion_result = reinsertion_failure.step();
    test.expect_true(
        broken_inserted &&
            reinsertion_result.status ==
                LegacyWorldStoryVmStatus::role_spatial_relocation_failed &&
            (reinsertion_failure.roles[1].flags & 3U) == 1U &&
            broken_spatial.row_heads[0U][role_row] == 0U &&
            reinsertion_failure.roles[1].spatial_next_link_32 == 0U &&
            reinsertion_failure.context.instruction_offset == 0U &&
            reinsertion_failure.state.previous_opcode == 0x66U,
        "shared role group handler keeps completed unlink effects when reinsertion is isolated"
    );

    Fixture exact_tail;
    openswd3::world_map::LegacyRoleSpatialIndex tail_spatial;
    reset_spatial(tail_spatial);
    exact_tail.roles[1].world_y = 32U;
    exact_tail.roles[1].flags = 0U;
    const bool tail_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            tail_spatial, exact_tail.roles, 1U, 0U
        );
    exact_tail.runtime.spatial_index = &tail_spatial;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_57_SET_ROLE_SPATIAL_GROUP_2);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_inserted &&
            tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            tail_result.opcode == OP_57_SET_ROLE_SPATIAL_GROUP_2 &&
            tail_result.executed_instruction_count == 1U &&
            (exact_tail.roles[1].flags & 3U) == 2U &&
            tail_spatial.row_heads[2U][role_row] == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_57_SET_ROLE_SPATIAL_GROUP_2,
        "shared role group handler exact tail completes relocation and yields at the window end"
    );
}

void test_wait_for_role_action_index_threshold(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_107_WAIT_ROLE_ACTION_INDEX,
        static_cast<u16>(OP_107_WAIT_ROLE_ACTION_INDEX | 0x4000U),
        static_cast<u16>(OP_107_WAIT_ROLE_ACTION_INDEX | 0x8000U),
        static_cast<u16>(OP_107_WAIT_ROLE_ACTION_INDEX | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture waiting;
        prime_loaded_instruction(waiting, raw_word);
        write_u16(waiting.state.window, 2U, 0xFFF0U);
        write_u16(waiting.state.window, 4U, 5U);
        waiting.roles[1].action.packed_ap_state = 0x0405U;
        waiting.state.previous_opcode = 0x66U;

        const auto result = waiting.step();
        test.expect_true(
            result.raw_word == raw_word &&
                result.opcode == OP_107_WAIT_ROLE_ACTION_INDEX &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                waiting.context.instruction_offset == 0U &&
                waiting.state.previous_opcode == OP_107_WAIT_ROLE_ACTION_INDEX,
            "opcode 107 aliases publish previous and wait at the inclusive item-count boundary"
        );
    }

    const auto prime_consuming = [](Fixture& fixture,
                                    const u16 selector,
                                    const u16 threshold,
                                    const u16 packed_state) {
        prime_loaded_instruction(fixture, OP_107_WAIT_ROLE_ACTION_INDEX);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, threshold);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.roles[1].action.packed_ap_state = packed_state;
        fixture.state.previous_opcode = 0x66U;
    };

    Fixture completed;
    prime_consuming(completed, 0xFFF0U, 5U, 0x0505U);
    const auto completed_result = completed.step();
    Fixture invalid_threshold;
    prime_consuming(invalid_threshold, 0x00F8U, 5U, 0x0104U);
    const auto invalid_result = invalid_threshold.step();
    Fixture missing_role;
    prime_consuming(missing_role, 0x7777U, 5U, 0x0000U);
    const auto missing_result = missing_role.step();
    Fixture controlled_role;
    prime_consuming(controlled_role, 0xFFFEU, 0U, 0x0000U);
    const auto controlled_result = controlled_role.step(0, 0, 1U);

    test.expect_true(
        completed_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            completed_result.opcode == kStoryVmTypedStop &&
            completed_result.executed_instruction_count == 2U &&
            completed.context.instruction_offset == 6U &&
            completed.state.previous_opcode == OP_107_WAIT_ROLE_ACTION_INDEX,
        "opcode 107 consumes a reached action index in-call"
    );
    test.expect_true(
        invalid_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            invalid_result.opcode == kStoryVmTypedStop &&
            invalid_result.executed_instruction_count == 2U &&
            invalid_threshold.context.instruction_offset == 6U &&
            invalid_threshold.state.previous_opcode ==
                OP_107_WAIT_ROLE_ACTION_INDEX,
        "opcode 107 consumes a threshold above the item limit in-call"
    );
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.opcode == kStoryVmTypedStop &&
            missing_result.executed_instruction_count == 2U &&
            missing_role.context.instruction_offset == 6U &&
            missing_role.state.previous_opcode == OP_107_WAIT_ROLE_ACTION_INDEX,
        "opcode 107 consumes a missing role in-call"
    );
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled_result.opcode == kStoryVmTypedStop &&
            controlled_result.executed_instruction_count == 2U &&
            controlled_role.context.instruction_offset == 6U &&
            controlled_role.state.previous_opcode ==
                OP_107_WAIT_ROLE_ACTION_INDEX,
        "opcode 107 preserves the helper-native controlled-role selector"
    );

    const auto prime_exact_tail = [](Fixture& fixture, const u16 offset) {
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = offset;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        write_u16(fixture.state.window, offset, OP_107_WAIT_ROLE_ACTION_INDEX);
    };

    Fixture completed_tail;
    prime_exact_tail(completed_tail, 0x7FFAU);
    write_u16(completed_tail.state.window, 0x7FFCU, 0xFFF0U);
    write_u16(completed_tail.state.window, 0x7FFEU, 5U);
    completed_tail.roles[1].action.packed_ap_state = 0x0505U;
    const auto completed_tail_result = completed_tail.step();

    Fixture missing_selector;
    prime_exact_tail(missing_selector, 0x7FFEU);
    const auto missing_selector_result = missing_selector.step();

    Fixture missing_threshold;
    prime_exact_tail(missing_threshold, 0x7FFCU);
    write_u16(missing_threshold.state.window, 0x7FFEU, 0xFFF0U);
    const auto missing_threshold_result = missing_threshold.step();

    test.expect_true(
        completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_107_WAIT_ROLE_ACTION_INDEX &&
            missing_selector_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_selector.context.instruction_offset == 0x7FFEU &&
            missing_selector.state.previous_opcode == 0x66U &&
            missing_threshold_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_threshold.context.instruction_offset == 0x7FFCU &&
            missing_threshold.state.previous_opcode == 0x66U,
        "opcode 107 preserves staged operands and exact-tail completion"
    );
}

void test_set_next_dialog_anchor_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_108_SET_NEXT_DIALOG_ANCHOR,
        static_cast<u16>(OP_108_SET_NEXT_DIALOG_ANCHOR | 0x4000U),
        static_cast<u16>(OP_108_SET_NEXT_DIALOG_ANCHOR | 0x8000U),
        static_cast<u16>(OP_108_SET_NEXT_DIALOG_ANCHOR | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 639U);
        write_u16(fixture.state.window, 4U, 479U);
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        test.expect_true(
            result.raw_word == kStoryVmTypedStop &&
                result.opcode == kStoryVmTypedStop &&
                result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.dialog_anchor_left == 639U &&
                fixture.state.dialog_anchor_top == 479U &&
                fixture.state.previous_opcode == OP_108_SET_NEXT_DIALOG_ANCHOR,
            "opcode 108 aliases preserve inclusive anchor bounds and continue in-call"
        );
    }

    Fixture both_fallback;
    prime_loaded_instruction(both_fallback, OP_108_SET_NEXT_DIALOG_ANCHOR);
    write_u16(both_fallback.state.window, 2U, 640U);
    write_u16(both_fallback.state.window, 4U, 480U);
    write_u16(both_fallback.state.window, 6U, kStoryVmTypedStop);
    const auto both_result = both_fallback.step();

    Fixture top_fallback;
    prime_loaded_instruction(top_fallback, OP_108_SET_NEXT_DIALOG_ANCHOR);
    write_u16(top_fallback.state.window, 2U, 639U);
    write_u16(top_fallback.state.window, 4U, 0xFFFFU);
    write_u16(top_fallback.state.window, 6U, kStoryVmTypedStop);
    const auto top_result = top_fallback.step();

    test.expect_true(
        both_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            both_fallback.state.dialog_anchor_left == 16U &&
            both_fallback.state.dialog_anchor_top == 16U &&
            both_fallback.state.previous_opcode ==
                OP_108_SET_NEXT_DIALOG_ANCHOR &&
            top_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            top_fallback.state.dialog_anchor_left == 639U &&
            top_fallback.state.dialog_anchor_top == 16U &&
            top_fallback.state.previous_opcode == OP_108_SET_NEXT_DIALOG_ANCHOR,
        "opcode 108 independently replaces unsigned out-of-range anchors with 16"
    );

    const auto prime_exact_tail = [](Fixture& fixture, const u16 offset) {
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = offset;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.dialog_anchor_left = 10U;
        fixture.state.dialog_anchor_top = 20U;
        fixture.state.previous_opcode = 0x66U;
        write_u16(fixture.state.window, offset, OP_108_SET_NEXT_DIALOG_ANCHOR);
    };

    Fixture missing_left;
    prime_exact_tail(missing_left, 0x7FFEU);
    const auto missing_left_result = missing_left.step();

    Fixture missing_top;
    prime_exact_tail(missing_top, 0x7FFCU);
    write_u16(missing_top.state.window, 0x7FFEU, 700U);
    const auto missing_top_result = missing_top.step();

    Fixture exact_tail;
    prime_exact_tail(exact_tail, 0x7FFAU);
    write_u16(exact_tail.state.window, 0x7FFCU, 640U);
    write_u16(exact_tail.state.window, 0x7FFEU, 479U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        missing_left_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_left.state.dialog_anchor_left == 10U &&
            missing_left.state.dialog_anchor_top == 20U &&
            missing_left.state.previous_opcode == 0x66U &&
            missing_top_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_top.state.dialog_anchor_left == 700U &&
            missing_top.state.dialog_anchor_top == 20U &&
            missing_top.state.previous_opcode == 0x66U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.dialog_anchor_left == 16U &&
            exact_tail.state.dialog_anchor_top == 479U &&
            exact_tail.state.previous_opcode == OP_108_SET_NEXT_DIALOG_ANCHOR,
        "opcode 108 preserves staged writes and completes an exact-tail record"
    );

    Fixture dialog_template;
    constexpr std::array<u8, 2U> text{'%', 'Q'};
    const std::size_t dialog_length =
        write_dialog_instruction(dialog_template, 2U, 0x00F8U, text);
    Fixture chained;
    prime_loaded_instruction(chained, OP_108_SET_NEXT_DIALOG_ANCHOR);
    write_u16(chained.state.window, 2U, 10U);
    write_u16(chained.state.window, 4U, 20U);
    std::ranges::copy_n(
        dialog_template.state.window.begin(),
        static_cast<std::ptrdiff_t>(dialog_length),
        chained.state.window.begin() + 6U
    );
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status == LegacyWorldStoryVmStatus::yielded &&
            chained_result.opcode == 2U &&
            chained_result.executed_instruction_count == 2U &&
            chained.context.instruction_offset == 6U + dialog_length &&
            !chained.dialogs.messages.empty() &&
            chained.state.dialog_anchor_left == 0x8000U &&
            chained.state.dialog_anchor_top == 0x8000U &&
            chained.state.previous_opcode == 2U,
        "opcode 108 stages the one-shot anchor consumed by the next dialog"
    );
}

void test_step_role_list_protocol(openswd3::test::Context& test) {
    const auto prime_slot = [](Fixture& fixture,
                               const std::size_t slot_index,
                               const u16 role_index,
                               const u8 direction) {
        auto& slot = fixture.active_object_slots[slot_index].bytes;
        slot.fill(0xFFU);
        write_u16(slot, 0x00U, role_index);
        write_u16(slot, 0x02U, 0U);
        slot[0x1BU] = 2U;
        slot[0x1CU] = direction;
    };

    constexpr std::array<u16, 4U> raw_aliases{
        OP_109_STEP_ROLES,
        static_cast<u16>(OP_109_STEP_ROLES | 0x4000U),
        static_cast<u16>(OP_109_STEP_ROLES | 0x8000U),
        static_cast<u16>(OP_109_STEP_ROLES | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        fixture.roles[1].flags |= 0x00040000U;
        prime_slot(fixture, 0U, 1U, 0U);
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 2U);
        write_u16(fixture.state.window, 4U, 0x7777U);
        write_u16(fixture.state.window, 6U, 0x00F8U);
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_109_STEP_ROLES &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode == OP_109_STEP_ROLES &&
                (fixture.roles[1].flags & 0x40000000U) != 0U,
            "opcode 109 aliases skip missing selectors, step found roles, and yield"
        );
    }

    Fixture no_slot;
    StoryPathHarness no_slot_paths{no_slot};
    prime_loaded_instruction(no_slot, OP_109_STEP_ROLES);
    write_u16(no_slot.state.window, 2U, 1U);
    write_u16(no_slot.state.window, 4U, 0x00F8U);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::yielded &&
            no_slot_result.direct_audio_service_count == 1U &&
            no_slot.context.instruction_offset == 6U &&
            no_slot.state.previous_opcode == OP_109_STEP_ROLES &&
            std::ranges::all_of(
                no_slot.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ),
        "opcode 109 ignores the helper's no matching slot return"
    );

    Fixture literal_fff0;
    StoryPathHarness literal_fff0_paths{literal_fff0};
    literal_fff0.roles[2].guid = 0xFFF0U;
    literal_fff0.roles[2].flags = 0x00040000U;
    prime_slot(literal_fff0, 0U, 2U, 0U);
    prime_loaded_instruction(literal_fff0, OP_109_STEP_ROLES);
    write_u16(literal_fff0.state.window, 2U, 1U);
    write_u16(literal_fff0.state.window, 4U, 0xFFF0U);
    const auto literal_fff0_result = literal_fff0.step();

    Fixture controlled_fffe;
    StoryPathHarness controlled_fffe_paths{controlled_fffe};
    controlled_fffe.roles[0].flags |= 0x00040000U;
    prime_slot(controlled_fffe, 0U, 0U, 0U);
    prime_loaded_instruction(controlled_fffe, OP_109_STEP_ROLES);
    write_u16(controlled_fffe.state.window, 2U, 1U);
    write_u16(controlled_fffe.state.window, 4U, 0xFFFEU);
    const auto controlled_fffe_result = controlled_fffe.step();

    test.expect_true(
        literal_fff0_result.status == LegacyWorldStoryVmStatus::yielded &&
            (literal_fff0.roles[2].flags & 0x40000000U) != 0U &&
            (literal_fff0.roles[1].flags & 0x40000000U) == 0U &&
            controlled_fffe_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            (controlled_fffe.roles[0].flags & 0x40000000U) != 0U,
        "opcode 109 keeps FFF0 literal and preserves helper-native FFFE lookup"
    );

    Fixture failed_second;
    StoryPathHarness failed_second_paths{failed_second};
    failed_second.roles[1].flags |= 0x00040000U;
    failed_second.roles[2].guid = 0x1234U;
    failed_second.roles[2].flags = 0x00040000U;
    prime_slot(failed_second, 0U, 1U, 0U);
    prime_slot(failed_second, 1U, 2U, 8U);
    prime_loaded_instruction(failed_second, OP_109_STEP_ROLES);
    write_u16(failed_second.state.window, 2U, 2U);
    write_u16(failed_second.state.window, 4U, 0x00F8U);
    write_u16(failed_second.state.window, 6U, 0x1234U);
    failed_second.state.previous_opcode = 0x66U;
    const auto failed_second_result = failed_second.step();
    test.expect_true(
        failed_second_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            failed_second_result.direct_audio_service_count == 0U &&
            failed_second.context.instruction_offset == 0U &&
            failed_second.state.previous_opcode == 0x66U &&
            (failed_second.roles[1].flags & 0x40000000U) != 0U &&
            (failed_second.roles[2].flags & 0x40000000U) == 0U,
        "opcode 109 preserves earlier role side effects when a later helper fails"
    );

    Fixture missing_count;
    missing_count.context.talk_data_offset = 0x1111U;
    missing_count.context.instruction_offset = 0x7FFEU;
    missing_count.state.loaded_file_number = 1U;
    missing_count.state.loaded_data_offset = 0x1111U;
    missing_count.state.window_loaded = true;
    missing_count.state.previous_opcode = 0x66U;
    write_u16(missing_count.state.window, 0x7FFEU, OP_109_STEP_ROLES);
    const auto missing_count_result = missing_count.step();

    Fixture missing_second;
    StoryPathHarness missing_second_paths{missing_second};
    prime_slot(missing_second, 0U, 1U, 0xFFU);
    missing_second.roles[1].path_wait_remaining = 7U;
    missing_second.roles[1].flags = 0x44000000U;
    missing_second.context.talk_data_offset = 0x1111U;
    missing_second.context.instruction_offset = 0x7FFAU;
    missing_second.state.loaded_file_number = 1U;
    missing_second.state.loaded_data_offset = 0x1111U;
    missing_second.state.window_loaded = true;
    missing_second.state.previous_opcode = 0x66U;
    write_u16(missing_second.state.window, 0x7FFAU, OP_109_STEP_ROLES);
    write_u16(missing_second.state.window, 0x7FFCU, 2U);
    write_u16(missing_second.state.window, 0x7FFEU, 0x00F8U);
    const auto missing_second_result = missing_second.step();

    test.expect_true(
        missing_count_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_count.context.instruction_offset == 0x7FFEU &&
            missing_count.state.previous_opcode == 0x66U &&
            missing_count_result.direct_audio_service_count == 0U &&
            missing_second_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_second.context.instruction_offset == 0x7FFAU &&
            missing_second.state.previous_opcode == 0x66U &&
            missing_second_result.direct_audio_service_count == 0U &&
            missing_second.roles[1].path_wait_remaining == 0U &&
            missing_second.roles[1].flags == 0U,
        "opcode 109 reads its list incrementally and retains prior helper effects"
    );

    Fixture zero_tail;
    zero_tail.context.talk_data_offset = 0x1111U;
    zero_tail.context.instruction_offset = 0x7FFCU;
    zero_tail.state.loaded_file_number = 1U;
    zero_tail.state.loaded_data_offset = 0x1111U;
    zero_tail.state.window_loaded = true;
    write_u16(zero_tail.state.window, 0x7FFCU, OP_109_STEP_ROLES);
    write_u16(zero_tail.state.window, 0x7FFEU, 0U);
    const auto zero_tail_result = zero_tail.step();

    Fixture one_tail;
    one_tail.context.talk_data_offset = 0x1111U;
    one_tail.context.instruction_offset = 0x7FFAU;
    one_tail.state.loaded_file_number = 1U;
    one_tail.state.loaded_data_offset = 0x1111U;
    one_tail.state.window_loaded = true;
    write_u16(one_tail.state.window, 0x7FFAU, OP_109_STEP_ROLES);
    write_u16(one_tail.state.window, 0x7FFCU, 1U);
    write_u16(one_tail.state.window, 0x7FFEU, 0x7777U);
    const auto one_tail_result = one_tail.step();

    test.expect_true(
        zero_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            zero_tail.context.instruction_offset == 0x8000U &&
            zero_tail.state.previous_opcode == OP_109_STEP_ROLES &&
            zero_tail_result.direct_audio_service_count == 1U &&
            one_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            one_tail.context.instruction_offset == 0x8000U &&
            one_tail.state.previous_opcode == OP_109_STEP_ROLES &&
            one_tail_result.direct_audio_service_count == 1U,
        "opcode 109 accepts zero count and exact-tail counted lists"
    );

    const auto prime_maximum_list = [](Fixture& fixture, const u16 count) {
        prime_loaded_instruction(fixture, OP_109_STEP_ROLES);
        write_u16(fixture.state.window, 2U, count);
        for (std::size_t offset = 4U; offset < fixture.state.window.size();
             offset += 2U) {
            write_u16(fixture.state.window, offset, 0x7777U);
        }
        fixture.state.previous_opcode = 0x66U;
    };

    Fixture wrapped;
    prime_maximum_list(wrapped, 0x3FFEU);
    const auto wrapped_result = wrapped.step();

    Fixture beyond_window;
    prime_maximum_list(beyond_window, 0x3FFFU);
    const auto beyond_window_result = beyond_window.step();

    test.expect_true(
        wrapped_result.status == LegacyWorldStoryVmStatus::yielded &&
            wrapped.context.instruction_offset == 0x8000U &&
            wrapped.state.previous_opcode == OP_109_STEP_ROLES &&
            wrapped_result.direct_audio_service_count == 1U &&
            beyond_window_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            beyond_window.context.instruction_offset == 0U &&
            beyond_window.state.previous_opcode == 0x66U &&
            beyond_window_result.direct_audio_service_count == 0U,
        "opcode 109 preserves the full-window IP and stops at the first missing selector"
    );
}

void test_secondary_role_bit30_reload_protocol(openswd3::test::Context& test) {
    constexpr u32 target = 0x12345678U;
    constexpr std::array<u16, 2U> opcodes{
        OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30,
        OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30,
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 opcode : opcodes) {
        for (const u16 alias_mask : alias_masks) {
            for (const bool secondary_role_has_bit30 : {false, true}) {
                Fixture fixture;
                prime_loaded_instruction(
                    fixture, static_cast<u16>(opcode | alias_mask)
                );
                write_u32(fixture.state.window, 2U, target);
                write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
                write_u16(
                    fixture.ports.transferred_window, 0U, kStoryVmTypedStop
                );
                fixture.roles[1].flags =
                    secondary_role_has_bit30 ? 0x40000000U : 0U;
                fixture.state.previous_opcode = 0x66U;

                const auto result = fixture.step();
                const bool should_reload = secondary_role_has_bit30 ==
                    (opcode == OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30);
                test.expect_true(
                    result.status ==
                            LegacyWorldStoryVmStatus::
                                script_variable_index_out_of_range &&
                        result.opcode == kStoryVmTypedStop &&
                        result.executed_instruction_count == 2U &&
                        result.direct_audio_service_count ==
                            (should_reload ? 1U : 0U) &&
                        fixture.context.talk_data_offset ==
                            (should_reload ? target : 0x1111U) &&
                        fixture.context.instruction_offset ==
                            (should_reload ? 0U : 6U) &&
                        fixture.state.previous_opcode == opcode &&
                        fixture.ports.data_load_count ==
                            (should_reload ? 1U : 0U) &&
                        fixture.ports.story_protocol_events ==
                            (should_reload ? std::vector<u32>{2U, 5U}
                                           : std::vector<u32>{}),
                    "opcodes 110 and 111 invert the secondary-role bit30 condition"
                );
            }
        }
    }

    Fixture role_zero_only;
    role_zero_only.roles.resize(1U);
    role_zero_only.roles[0].flags = 0x40000000U;
    prime_loaded_instruction(
        role_zero_only, OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30
    );
    write_u32(role_zero_only.state.window, 2U, target);
    write_u16(role_zero_only.state.window, 6U, kStoryVmTypedStop);
    role_zero_only.state.previous_opcode = 0x66U;
    const auto role_zero_result = role_zero_only.step();
    test.expect_true(
        role_zero_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            role_zero_result.executed_instruction_count == 2U &&
            role_zero_only.context.instruction_offset == 6U &&
            role_zero_only.state.previous_opcode ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            role_zero_only.ports.data_load_count == 0U,
        "opcodes 110 and 111 never scan role index zero"
    );

    Fixture load_failure;
    prime_loaded_instruction(
        load_failure, OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30
    );
    write_u32(load_failure.state.window, 2U, 0x87654321U);
    load_failure.roles[1].flags = 0x40000000U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x66U;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.opcode ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x87654321U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 111 checked I/O failure preserves legacy transfer side effects"
    );

    Fixture sequential_truncated;
    sequential_truncated.context.talk_data_offset = 0x1111U;
    sequential_truncated.context.instruction_offset = 0x7FFEU;
    sequential_truncated.state.loaded_file_number = 1U;
    sequential_truncated.state.loaded_data_offset = 0x1111U;
    sequential_truncated.state.window_loaded = true;
    sequential_truncated.state.previous_opcode = 0x66U;
    write_u16(
        sequential_truncated.state.window,
        0x7FFEU,
        OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30
    );
    const auto sequential_truncated_result = sequential_truncated.step();

    Fixture reload_truncated;
    reload_truncated.context.talk_data_offset = 0x1111U;
    reload_truncated.context.instruction_offset = 0x7FFEU;
    reload_truncated.state.loaded_file_number = 1U;
    reload_truncated.state.loaded_data_offset = 0x1111U;
    reload_truncated.state.window_loaded = true;
    reload_truncated.state.previous_opcode = 0x66U;
    write_u16(
        reload_truncated.state.window,
        0x7FFEU,
        OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30
    );
    const auto reload_truncated_result = reload_truncated.step();

    test.expect_true(
        sequential_truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_truncated_result.executed_instruction_count == 1U &&
            sequential_truncated.context.instruction_offset == 0x8004U &&
            sequential_truncated.state.previous_opcode ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            sequential_truncated.ports.data_load_count == 0U &&
            reload_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            reload_truncated.context.instruction_offset == 0x7FFEU &&
            reload_truncated.state.previous_opcode == 0x66U &&
            reload_truncated.ports.data_load_count == 0U,
        "opcodes 110 and 111 read the target only on the reload path"
    );

    Fixture reload_tail;
    reload_tail.context.talk_data_offset = 0x1111U;
    reload_tail.context.instruction_offset = 0x7FFAU;
    reload_tail.state.loaded_file_number = 1U;
    reload_tail.state.loaded_data_offset = 0x1111U;
    reload_tail.state.window_loaded = true;
    write_u16(
        reload_tail.state.window,
        0x7FFAU,
        OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30
    );
    write_u32(reload_tail.state.window, 0x7FFCU, target);
    write_u16(reload_tail.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto reload_tail_result = reload_tail.step();

    Fixture sequential_tail;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.context.instruction_offset = 0x7FFAU;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    write_u16(
        sequential_tail.state.window,
        0x7FFAU,
        OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30
    );
    write_u32(sequential_tail.state.window, 0x7FFCU, target);
    const auto sequential_tail_result = sequential_tail.step();

    test.expect_true(
        reload_tail_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            reload_tail_result.executed_instruction_count == 2U &&
            reload_tail_result.direct_audio_service_count == 1U &&
            reload_tail.context.talk_data_offset == target &&
            reload_tail.context.instruction_offset == 0U &&
            reload_tail.state.previous_opcode ==
                OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30 &&
            reload_tail.ports.data_load_count == 1U &&
            sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail_result.direct_audio_service_count == 0U &&
            sequential_tail.context.instruction_offset == 0x8000U &&
            sequential_tail.state.previous_opcode ==
                OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            sequential_tail.ports.data_load_count == 0U,
        "opcodes 110 and 111 preserve reload and sequential exact tails"
    );
}

void test_wait_overlay_action_lists_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 alias_mask : alias_masks) {
        Fixture fixture;
        std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows(1U);
        openswd3::world_map::LegacyMovingActionList moving_actions(1U);
        fixture.runtime.packed_row_effects = &packed_rows;
        fixture.runtime.moving_actions = &moving_actions;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS | alias_mask
            )
        );
        fixture.state.previous_opcode = 0x66U;
        u32 previous_seen_by_audio{};
        u16 ip_seen_by_audio{};
        fixture.ports.audio_service_callback = [&] {
            previous_seen_by_audio = fixture.state.previous_opcode;
            ip_seen_by_audio = fixture.context.instruction_offset;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
                previous_seen_by_audio ==
                    OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
                ip_seen_by_audio == 0U && packed_rows.size() == 1U &&
                moving_actions.size() == 1U &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U},
            "opcode 112 aliases short-circuit on the packed-row list"
        );
    }

    Fixture role_head_wait;
    std::list<openswd3::rendering::LegacyPackedRowEffect> empty_packed_rows;
    openswd3::world_map::LegacyRoleHeadActionList role_head_actions(1U);
    openswd3::world_map::LegacyMovingActionList waiting_moving_actions(1U);
    role_head_wait.runtime.packed_row_effects = &empty_packed_rows;
    role_head_wait.runtime.role_head_actions = &role_head_actions;
    role_head_wait.runtime.moving_actions = &waiting_moving_actions;
    prime_loaded_instruction(
        role_head_wait, OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
    );
    role_head_wait.state.previous_opcode = 0x66U;
    const auto role_head_result = role_head_wait.step();
    test.expect_true(
        role_head_result.status == LegacyWorldStoryVmStatus::yielded &&
            role_head_result.executed_instruction_count == 1U &&
            role_head_result.direct_audio_service_count == 1U &&
            role_head_wait.context.instruction_offset == 0U &&
            role_head_wait.state.previous_opcode ==
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            role_head_actions.size() == 1U &&
            waiting_moving_actions.size() == 1U,
        "opcode 112 waits when only the role-head action list is nonempty"
    );

    Fixture completed;
    std::list<openswd3::rendering::LegacyPackedRowEffect> completed_rows;
    openswd3::world_map::LegacyRoleHeadActionList completed_heads;
    openswd3::world_map::LegacyMovingActionList retained_moving_actions(1U);
    completed.runtime.packed_row_effects = &completed_rows;
    completed.runtime.role_head_actions = &completed_heads;
    completed.runtime.moving_actions = &retained_moving_actions;
    prime_loaded_instruction(
        completed, OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
    );
    write_u16(completed.state.window, 2U, kStoryVmTypedStop);
    completed.state.previous_opcode = 0x66U;
    const auto completed_result = completed.step();
    test.expect_true(
        completed_result.status == LegacyWorldStoryVmStatus::yielded &&
            completed_result.opcode ==
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            completed_result.executed_instruction_count == 1U &&
            completed_result.direct_audio_service_count == 1U &&
            completed.context.instruction_offset == 2U &&
            completed.state.previous_opcode ==
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            retained_moving_actions.size() == 1U,
        "opcode 112 completes but still yields and ignores moving actions"
    );

    Fixture packed_owner_missing;
    openswd3::world_map::LegacyRoleHeadActionList untouched_heads(1U);
    packed_owner_missing.runtime.role_head_actions = &untouched_heads;
    prime_loaded_instruction(
        packed_owner_missing, OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
    );
    packed_owner_missing.state.previous_opcode = 0x66U;
    const auto packed_missing_result = packed_owner_missing.step();

    Fixture head_owner_missing;
    std::list<openswd3::rendering::LegacyPackedRowEffect> available_rows;
    head_owner_missing.runtime.packed_row_effects = &available_rows;
    prime_loaded_instruction(
        head_owner_missing, OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
    );
    head_owner_missing.state.previous_opcode = 0x66U;
    const auto head_missing_result = head_owner_missing.step();

    test.expect_true(
        packed_missing_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            packed_owner_missing.context.instruction_offset == 0U &&
            packed_owner_missing.state.previous_opcode == 0x66U &&
            packed_missing_result.direct_audio_service_count == 0U &&
            untouched_heads.size() == 1U &&
            head_missing_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            head_owner_missing.context.instruction_offset == 0U &&
            head_owner_missing.state.previous_opcode == 0x66U &&
            head_missing_result.direct_audio_service_count == 0U,
        "opcode 112 checks typed list owners in machine access order"
    );

    for (const bool wait_at_tail : {false, true}) {
        Fixture exact_tail;
        std::list<openswd3::rendering::LegacyPackedRowEffect> tail_rows;
        if (wait_at_tail) {
            tail_rows.emplace_back();
        }
        openswd3::world_map::LegacyRoleHeadActionList tail_heads;
        openswd3::world_map::LegacyMovingActionList tail_moving(1U);
        exact_tail.runtime.packed_row_effects = &tail_rows;
        exact_tail.runtime.role_head_actions = &tail_heads;
        exact_tail.runtime.moving_actions = &tail_moving;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.context.instruction_offset = 0x7FFEU;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        exact_tail.state.previous_opcode = 0x66U;
        write_u16(
            exact_tail.state.window,
            0x7FFEU,
            OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
        );

        const auto tail_result = exact_tail.step();
        test.expect_true(
            tail_result.status == LegacyWorldStoryVmStatus::yielded &&
                tail_result.executed_instruction_count == 1U &&
                tail_result.direct_audio_service_count == 1U &&
                exact_tail.context.instruction_offset ==
                    (wait_at_tail ? 0x7FFEU : 0x8000U) &&
                exact_tail.state.previous_opcode ==
                    OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
                tail_moving.size() == 1U,
            "opcode 112 preserves waiting and completed exact tails"
        );
    }
}

void test_play_sound_effect_with_unread_padding_protocol(
    openswd3::test::Context& test
) {
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
            static_cast<u16>(
                OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING |
                alias_masks[index]
            )
        );
        write_u16(fixture.state.window, 2U, sound_ids[index]);
        write_u16(
            fixture.state.window,
            4U,
            static_cast<u16>(0xA500U | static_cast<u16>(index))
        );
        write_u16(fixture.state.window, 6U, kStoryVmTypedStop);
        fixture.state.previous_opcode = 0x66U;
        std::size_t requests_seen_by_audio{};
        u16 sound_seen_by_audio{};
        u16 ip_seen_by_audio{};
        u32 previous_seen_by_audio{};
        fixture.ports.audio_service_callback = [&] {
            requests_seen_by_audio = fixture.ports.sound_effect_requests.size();
            sound_seen_by_audio = fixture.ports.sound_effect_requests.back();
            ip_seen_by_audio = fixture.context.instruction_offset;
            previous_seen_by_audio = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{sound_ids[index]} &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING &&
                requests_seen_by_audio == 1U &&
                sound_seen_by_audio == sound_ids[index] &&
                ip_seen_by_audio == 6U &&
                previous_seen_by_audio ==
                    OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U},
            "opcode 113 aliases consume six bytes and ignore playback results"
        );
    }

    Fixture operand_truncated;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x66U;
    write_u16(
        operand_truncated.state.window,
        0x7FFEU,
        OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING
    );
    const auto truncated_result = operand_truncated.step();

    Fixture unread_padding;
    unread_padding.context.talk_data_offset = 0x1111U;
    unread_padding.context.instruction_offset = 0x7FFCU;
    unread_padding.state.loaded_file_number = 1U;
    unread_padding.state.loaded_data_offset = 0x1111U;
    unread_padding.state.window_loaded = true;
    unread_padding.state.previous_opcode = 0x66U;
    write_u16(
        unread_padding.state.window,
        0x7FFCU,
        OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING
    );
    write_u16(unread_padding.state.window, 0x7FFEU, 0x4321U);
    const auto unread_padding_result = unread_padding.step();

    Fixture exact_tail;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FFAU,
        OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0xFFFFU);
    write_u16(exact_tail.state.window, 0x7FFEU, 0xBEEFU);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.ports.sound_effect_requests.empty() &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U &&
            truncated_result.direct_audio_service_count == 0U &&
            unread_padding_result.status == LegacyWorldStoryVmStatus::yielded &&
            unread_padding_result.executed_instruction_count == 1U &&
            unread_padding_result.direct_audio_service_count == 1U &&
            unread_padding.ports.sound_effect_requests ==
                std::vector<u16>{0x4321U} &&
            unread_padding.context.instruction_offset == 0x8002U &&
            unread_padding.state.previous_opcode ==
                OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING &&
            exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.ports.sound_effect_requests ==
                std::vector<u16>{0xFFFFU} &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING,
        "opcode 113 reads only the sound word and preserves both tail forms"
    );
}

void test_stage_scene_music_stream_request_protocol(
    openswd3::test::Context& test
) {
    struct TestCase {
        u16 alias_mask;
        u16 first_stream;
        u16 second_stream;
        u16 flags;
        u32 initial_transition_mode;
        u32 initial_fade_divisor;
        u32 initial_pending_fade_divisor;
        u32 initial_control_flags;
        u32 applied_transition_mode;
        u32 final_transition_mode;
        u32 final_fade_divisor;
        u32 final_control_flags;
    };
    constexpr std::array cases{
        TestCase{
            0U,
            25U,
            26U,
            0x2000U,
            0U,
            0x11111111U,
            3U,
            0x12345678U,
            1U,
            0U,
            0U,
            0x12B65600U,
        },
        TestCase{
            0x4000U,
            0U,
            0xFFFFU,
            0x4000U,
            1U,
            0x22222222U,
            4U,
            0xA5C3D4EFU,
            1U,
            0U,
            0U,
            0xA5C3D400U,
        },
        TestCase{
            0x8000U,
            0x1234U,
            7U,
            0x6000U,
            2U,
            0x33333333U,
            5U,
            0x5A03AA55U,
            2U,
            2U,
            5U,
            0x5A83AA00U,
        },
        TestCase{
            0xC000U,
            0xFFFFU,
            8U,
            0x8000U,
            0xFFFFFFFFU,
            0x89ABCDEFU,
            8U,
            0xFFFFFFFFU,
            0xFFFFFFFFU,
            0xFFFFFFFFU,
            0x89ABCDEFU,
            0xFFFCFF00U,
        },
    };

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST | test_case.alias_mask
            )
        );
        write_u16(fixture.state.window, 2U, test_case.first_stream);
        write_u16(fixture.state.window, 4U, test_case.second_stream);
        write_u16(fixture.state.window, 6U, test_case.flags);
        write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
        fixture.state.music_request = 0x01020304U;
        fixture.state.music_first_stream = 0x11111111U;
        fixture.state.music_second_stream = 0x22222222U;
        fixture.state.current_first_stream = test_case.initial_transition_mode;
        fixture.state.current_stream_fade_divisor =
            test_case.initial_fade_divisor;
        fixture.state.current_second_stream =
            test_case.initial_pending_fade_divisor;
        fixture.state.music_control_flags = test_case.initial_control_flags;
        fixture.state.previous_opcode = 0x66U;
        u32 request_seen_by_transition{};
        u32 first_seen_by_transition{};
        u32 second_seen_by_transition{};
        u32 control_seen_by_transition{};
        u16 ip_seen_by_transition{};
        u32 previous_seen_by_transition{};
        fixture.ports.music_transition_callback = [&] {
            request_seen_by_transition = fixture.state.music_request;
            first_seen_by_transition = fixture.state.music_first_stream;
            second_seen_by_transition = fixture.state.music_second_stream;
            control_seen_by_transition = fixture.state.music_control_flags;
            ip_seen_by_transition = fixture.context.instruction_offset;
            previous_seen_by_transition = fixture.state.previous_opcode;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.state.music_request == 0x80000001U &&
                fixture.state.music_first_stream == test_case.first_stream &&
                fixture.state.music_second_stream == test_case.second_stream &&
                fixture.state.current_first_stream ==
                    test_case.final_transition_mode &&
                fixture.state.current_stream_fade_divisor ==
                    test_case.final_fade_divisor &&
                fixture.state.current_second_stream ==
                    test_case.initial_pending_fade_divisor &&
                fixture.state.music_control_flags ==
                    test_case.final_control_flags &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST &&
                fixture.ports.music_transition_apply_count == 1U &&
                fixture.ports.last_music_transition_mode ==
                    test_case.applied_transition_mode &&
                fixture.ports.last_music_current_fade_divisor ==
                    test_case.initial_fade_divisor &&
                fixture.ports.last_music_pending_fade_divisor ==
                    test_case.initial_pending_fade_divisor &&
                fixture.ports.story_protocol_events == std::vector<u32>{11U} &&
                request_seen_by_transition == 0x80000001U &&
                first_seen_by_transition == test_case.first_stream &&
                second_seen_by_transition == test_case.second_stream &&
                control_seen_by_transition == test_case.initial_control_flags &&
                ip_seen_by_transition == 0U &&
                previous_seen_by_transition == 0x66U,
            "opcode 114 aliases stage streams, synchronize and continue in order"
        );
    }

    const auto prime_tail = [](Fixture& fixture, const u16 ip) {
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = ip;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.music_request = 0x01020304U;
        fixture.state.music_first_stream = 0x11111111U;
        fixture.state.music_second_stream = 0x22222222U;
        fixture.state.current_first_stream = 0U;
        fixture.state.current_stream_fade_divisor = 9U;
        fixture.state.current_second_stream = 17U;
        fixture.state.music_control_flags = 0x11223344U;
        fixture.state.previous_opcode = 0x66U;
        write_u16(
            fixture.state.window, ip, OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST
        );
    };

    Fixture first_truncated;
    prime_tail(first_truncated, 0x7FFEU);
    const auto first_truncated_result = first_truncated.step();

    Fixture second_truncated;
    prime_tail(second_truncated, 0x7FFCU);
    write_u16(second_truncated.state.window, 0x7FFEU, 0x1234U);
    const auto second_truncated_result = second_truncated.step();

    Fixture flags_truncated;
    prime_tail(flags_truncated, 0x7FFAU);
    write_u16(flags_truncated.state.window, 0x7FFCU, 0x1234U);
    write_u16(flags_truncated.state.window, 0x7FFEU, 0x5678U);
    const auto flags_truncated_result = flags_truncated.step();

    Fixture exact_tail;
    prime_tail(exact_tail, 0x7FF8U);
    exact_tail.state.current_first_stream = 2U;
    write_u16(exact_tail.state.window, 0x7FFAU, 0x1234U);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x5678U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x4000U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        first_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            first_truncated_result.executed_instruction_count == 1U &&
            first_truncated.state.music_request == 0x80000001U &&
            first_truncated.state.music_first_stream == 0x11111111U &&
            first_truncated.state.music_second_stream == 0x22222222U &&
            first_truncated.state.current_first_stream == 0U &&
            first_truncated.state.current_stream_fade_divisor == 9U &&
            first_truncated.state.music_control_flags == 0x11223344U &&
            first_truncated.state.previous_opcode == 0x66U &&
            first_truncated.ports.music_transition_apply_count == 0U &&
            second_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            second_truncated_result.executed_instruction_count == 1U &&
            second_truncated.state.music_request == 0x80000001U &&
            second_truncated.state.music_first_stream == 0x1234U &&
            second_truncated.state.music_second_stream == 0x22222222U &&
            second_truncated.state.current_first_stream == 0U &&
            second_truncated.state.current_stream_fade_divisor == 9U &&
            second_truncated.state.music_control_flags == 0x11223344U &&
            second_truncated.state.previous_opcode == 0x66U &&
            second_truncated.ports.music_transition_apply_count == 0U &&
            flags_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            flags_truncated_result.executed_instruction_count == 1U &&
            flags_truncated.state.music_request == 0x80000001U &&
            flags_truncated.state.music_first_stream == 0x1234U &&
            flags_truncated.state.music_second_stream == 0x5678U &&
            flags_truncated.state.current_first_stream == 0U &&
            flags_truncated.state.current_stream_fade_divisor == 0U &&
            flags_truncated.state.music_control_flags == 0x11A23344U &&
            flags_truncated.state.previous_opcode == 0x66U &&
            flags_truncated.ports.music_transition_apply_count == 1U &&
            exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.state.music_request == 0x80000001U &&
            exact_tail.state.music_first_stream == 0x1234U &&
            exact_tail.state.music_second_stream == 0x5678U &&
            exact_tail.state.current_first_stream == 2U &&
            exact_tail.state.current_stream_fade_divisor == 17U &&
            exact_tail.state.music_control_flags == 0x11A33300U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST &&
            exact_tail.ports.music_transition_apply_count == 1U,
        "opcode 114 preserves every staged unsafe point and exact-tail effect"
    );
}

void test_set_music_stream_volume_protocol(openswd3::test::Context& test) {
    struct TestCase {
        u16 raw_opcode;
        u16 raw_level;
        u32 expected_level;
    };
    constexpr std::array<TestCase, 6U> cases{{
        {OP_115_SET_MUSIC_STREAM_VOLUME, 0U, 0U},
        {
            static_cast<u16>(OP_115_SET_MUSIC_STREAM_VOLUME | 0x4000U),
            10U,
            10U,
        },
        {
            static_cast<u16>(OP_115_SET_MUSIC_STREAM_VOLUME | 0x8000U),
            11U,
            11U,
        },
        {
            static_cast<u16>(OP_115_SET_MUSIC_STREAM_VOLUME | 0xC000U),
            12U,
            11U,
        },
        {OP_115_SET_MUSIC_STREAM_VOLUME, 0x7FFFU, 11U},
        {OP_115_SET_MUSIC_STREAM_VOLUME, 0xFFFFU, 11U},
    }};

    for (const auto& test_case : cases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, test_case.raw_opcode);
        write_u16(fixture.state.window, 2U, test_case.raw_level);
        fixture.state.music_request = 0x11223344U;
        fixture.state.current_first_stream = 0x55667788U;
        fixture.state.previous_opcode = 0x66U;
        bool volume_saw_precommit_state{};
        bool audio_saw_committed_state{};
        fixture.ports.music_volume_callback = [&]() {
            volume_saw_precommit_state =
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0x66U &&
                fixture.ports.direct_audio_service_count == 0U;
        };
        fixture.ports.audio_service_callback = [&]() {
            audio_saw_committed_state =
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_115_SET_MUSIC_STREAM_VOLUME &&
                fixture.ports.music_volume_write_count == 1U;
        };

        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_115_SET_MUSIC_STREAM_VOLUME &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_115_SET_MUSIC_STREAM_VOLUME &&
                fixture.state.music_request == 0x11223344U &&
                fixture.state.current_first_stream == 0x55667788U &&
                fixture.ports.music_volume_write_count == 1U &&
                fixture.ports.last_music_volume_level ==
                    test_case.expected_level &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{12U, 2U} &&
                volume_saw_precommit_state && audio_saw_committed_state,
            "opcode 115 aliases clamp the zero-extended level then yield"
        );
    }

    auto prime_tail = [](Fixture& fixture, const u16 ip) {
        prime_loaded_instruction(fixture, OP_115_SET_MUSIC_STREAM_VOLUME);
        fixture.context.instruction_offset = ip;
        fixture.state.previous_opcode = 0x66U;
        write_u16(fixture.state.window, ip, OP_115_SET_MUSIC_STREAM_VOLUME);
    };

    Fixture truncated;
    prime_tail(truncated, 0x7FFEU);
    const auto truncated_result = truncated.step();

    Fixture exact_tail;
    prime_tail(exact_tail, 0x7FFCU);
    write_u16(exact_tail.state.window, 0x7FFEU, 11U);
    const auto exact_tail_result = exact_tail.step();

    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x66U &&
            truncated.ports.music_volume_write_count == 0U,
        "opcode 115 stops at its operand unsafe point"
    );
    test.expect_true(
        exact_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_115_SET_MUSIC_STREAM_VOLUME &&
            exact_tail.ports.music_volume_write_count == 1U &&
            exact_tail.ports.last_music_volume_level == 11U,
        "opcode 115 commits its exact-tail volume before yielding"
    );
}

void test_batch_set_role_positions_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_116_BATCH_SET_ROLE_POSITIONS,
        static_cast<u16>(OP_116_BATCH_SET_ROLE_POSITIONS | 0x4000U),
        static_cast<u16>(OP_116_BATCH_SET_ROLE_POSITIONS | 0x8000U),
        static_cast<u16>(OP_116_BATCH_SET_ROLE_POSITIONS | 0xC000U),
    };
    for (const u16 raw_opcode : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_opcode);
        write_u16(fixture.state.window, 2U, 0U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);

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
                    OP_116_BATCH_SET_ROLE_POSITIONS,
            "opcode 116 aliases accept an empty batch and continue"
        );
    }

    Fixture batch;
    StoryPathHarness batch_paths{batch, 1U};
    prime_loaded_instruction(batch, OP_116_BATCH_SET_ROLE_POSITIONS);
    write_u16(batch.state.window, 2U, 2U);
    write_u16(batch.state.window, 4U, 0xFFF0U);
    write_u16(batch.state.window, 6U, 21U);
    write_u16(batch.state.window, 8U, 15U);
    write_u16(batch.state.window, 10U, 0xFFFEU);
    write_u16(batch.state.window, 12U, 22U);
    write_u16(batch.state.window, 14U, 16U);
    write_u16(batch.state.window, 16U, kStoryVmTypedStop);
    const auto batch_result = batch.step(0, 0, 1U);
    const auto role_one_slot = std::ranges::find_if(
        batch.active_object_slots, [](const LegacyWorldObjectSlot& slot) {
            return read_u16(slot.bytes, 0U) == 1U;
        }
    );
    test.expect_true(
        batch_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            batch_result.opcode == kStoryVmTypedStop &&
            batch_result.executed_instruction_count == 2U &&
            batch_result.direct_audio_service_count == 0U &&
            batch.context.instruction_offset == 16U &&
            batch.state.previous_opcode == OP_116_BATCH_SET_ROLE_POSITIONS &&
            batch.dialogs.close.flagged_dialog_counter == 0x8000U &&
            role_one_slot != batch.active_object_slots.end() &&
            read_u16(role_one_slot->bytes, 4U) == 352U &&
            read_u16(role_one_slot->bytes, 6U) == 256U,
        "opcode 116 schedules every record and marks the controlled role"
    );

    Fixture controlled_alias;
    StoryPathHarness controlled_paths{controlled_alias, 1U};
    prime_loaded_instruction(controlled_alias, OP_116_BATCH_SET_ROLE_POSITIONS);
    write_u16(controlled_alias.state.window, 2U, 1U);
    write_u16(controlled_alias.state.window, 4U, 0xFFFEU);
    write_u16(controlled_alias.state.window, 6U, 0x1015U);
    write_u16(controlled_alias.state.window, 8U, 0x100FU);
    write_u16(controlled_alias.state.window, 10U, kStoryVmTypedStop);
    const auto controlled_alias_result = controlled_alias.step(0, 0, 1U);
    const auto controlled_slot = std::ranges::find_if(
        controlled_alias.active_object_slots,
        [](const LegacyWorldObjectSlot& slot) {
            return read_u16(slot.bytes, 0U) == 1U;
        }
    );
    test.expect_true(
        controlled_alias_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            controlled_alias.context.instruction_offset == 10U &&
            controlled_alias.dialogs.close.flagged_dialog_counter == 0x8000U &&
            controlled_slot != controlled_alias.active_object_slots.end() &&
            read_u16(controlled_slot->bytes, 4U) == 336U &&
            read_u16(controlled_slot->bytes, 6U) == 240U,
        "opcode 116 keeps FFFE and 16-bit coordinate shift semantics"
    );

    Fixture missing;
    StoryPathHarness missing_paths{missing};
    prime_loaded_instruction(missing, OP_116_BATCH_SET_ROLE_POSITIONS);
    write_u16(missing.state.window, 2U, 1U);
    write_u16(missing.state.window, 4U, 0x7777U);
    write_u16(missing.state.window, 6U, 21U);
    write_u16(missing.state.window, 8U, 15U);
    missing.state.previous_opcode = 0x66U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x66U &&
            missing.dialogs.close.flagged_dialog_counter == 0U,
        "opcode 116 stops where a missing resolver result becomes a role pointer"
    );

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_116_BATCH_SET_ROLE_POSITIONS);
    write_u16(unavailable.state.window, 2U, 1U);
    write_u16(unavailable.state.window, 4U, 0x00F8U);
    write_u16(unavailable.state.window, 6U, 21U);
    write_u16(unavailable.state.window, 8U, 15U);
    unavailable.state.previous_opcode = 0x66U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x66U,
        "opcode 116 reads a full record before requiring the path runtime"
    );

    auto prime_tail = [](Fixture& fixture, const u16 ip) {
        prime_loaded_instruction(fixture, OP_116_BATCH_SET_ROLE_POSITIONS);
        fixture.context.instruction_offset = ip;
        fixture.state.previous_opcode = 0x66U;
        write_u16(fixture.state.window, ip, OP_116_BATCH_SET_ROLE_POSITIONS);
    };

    Fixture count_truncated;
    prime_tail(count_truncated, 0x7FFEU);
    const auto count_truncated_result = count_truncated.step();

    Fixture selector_truncated;
    prime_tail(selector_truncated, 0x7FFCU);
    write_u16(selector_truncated.state.window, 0x7FFEU, 1U);
    const auto selector_truncated_result = selector_truncated.step();

    Fixture coordinates_truncated;
    prime_tail(coordinates_truncated, 0x7FF8U);
    write_u16(coordinates_truncated.state.window, 0x7FFAU, 1U);
    write_u16(coordinates_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(coordinates_truncated.state.window, 0x7FFEU, 21U);
    const auto coordinates_truncated_result = coordinates_truncated.step();

    Fixture empty_exact_tail;
    prime_tail(empty_exact_tail, 0x7FFCU);
    write_u16(empty_exact_tail.state.window, 0x7FFEU, 0U);
    const auto empty_exact_tail_result = empty_exact_tail.step();

    Fixture record_exact_tail;
    StoryPathHarness record_exact_paths{record_exact_tail};
    prime_tail(record_exact_tail, 0x7FF6U);
    write_u16(record_exact_tail.state.window, 0x7FF8U, 1U);
    write_u16(record_exact_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(record_exact_tail.state.window, 0x7FFCU, 21U);
    write_u16(record_exact_tail.state.window, 0x7FFEU, 15U);
    const auto record_exact_tail_result = record_exact_tail.step();

    test.expect_true(
        count_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            count_truncated.context.instruction_offset == 0x7FFEU &&
            count_truncated.state.previous_opcode == 0x66U &&
            selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.context.instruction_offset == 0x7FFCU &&
            selector_truncated.state.previous_opcode == 0x66U &&
            coordinates_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            coordinates_truncated.context.instruction_offset == 0x7FF8U &&
            coordinates_truncated.state.previous_opcode == 0x66U,
        "opcode 116 preserves count, selector and Y-before-X unsafe points"
    );
    test.expect_true(
        empty_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            empty_exact_tail_result.executed_instruction_count == 1U &&
            empty_exact_tail.context.instruction_offset == 0x8000U &&
            empty_exact_tail.state.previous_opcode ==
                OP_116_BATCH_SET_ROLE_POSITIONS &&
            record_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            record_exact_tail_result.executed_instruction_count == 1U &&
            record_exact_tail.context.instruction_offset == 0x8000U &&
            record_exact_tail.state.previous_opcode ==
                OP_116_BATCH_SET_ROLE_POSITIONS,
        "opcode 116 commits empty and one-record exact tails before refetch"
    );
}

void test_remove_dialogs_for_role_guid_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID,
        static_cast<u16>(OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID | 0x4000U),
        static_cast<u16>(OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID | 0x8000U),
        static_cast<u16>(OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID | 0xC000U),
    };
    for (const u16 raw_opcode : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_opcode);
        write_u16(fixture.state.window, 2U, 0xFFF0U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);

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
                    OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
                read_u16(fixture.state.window, 2U) == 0xFFF0U,
            "opcode 118 aliases preserve an empty list and continue"
        );
    }

    const auto append_message =
        [](Fixture& fixture, const u16 role_index, const u16 marker) {
            fixture.dialogs.messages.emplace_back();
            auto& message = fixture.dialogs.messages.back();
            message.record.role_index = role_index;
            message.record.display_counter = marker;
            message.text = {static_cast<u8>(marker)};
            message.caption = {static_cast<u8>(marker + 1U)};
        };

    Fixture matching;
    matching.roles[0].guid = 1U;
    matching.roles[1].guid = 2U;
    matching.roles[2].guid = 1U;
    matching.roles[0].interaction_gate = 7U;
    matching.roles[1].interaction_gate = 8U;
    matching.roles[2].interaction_gate = 9U;
    append_message(matching, 1U, 10U);
    append_message(matching, 0U, 11U);
    append_message(matching, 2U, 12U);
    append_message(matching, 0U, 13U);
    append_message(matching, 1U, 14U);
    matching.dialogs.close.flagged_dialog_counter = 0xCAFE8002U;
    prime_loaded_instruction(matching, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID);
    write_u16(matching.state.window, 2U, 1U);
    write_u16(matching.state.window, 4U, kStoryVmTypedStop);
    const auto matching_result = matching.step();
    auto remaining = matching.dialogs.messages.begin();
    const u16 first_remaining_marker = remaining->record.display_counter;
    ++remaining;
    const u16 second_remaining_marker = remaining->record.display_counter;
    test.expect_true(
        matching_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            matching_result.opcode == kStoryVmTypedStop &&
            matching_result.executed_instruction_count == 2U &&
            matching_result.direct_audio_service_count == 0U &&
            matching.context.instruction_offset == 4U &&
            matching.state.previous_opcode ==
                OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
            matching.dialogs.messages.size() == 2U &&
            first_remaining_marker == 10U && second_remaining_marker == 14U &&
            matching.roles[0].interaction_gate == 0U &&
            matching.roles[1].interaction_gate == 8U &&
            matching.roles[2].interaction_gate == 0U &&
            matching.dialogs.close.flagged_dialog_counter == 0x8000U,
        "opcode 118 removes every matching GUID and clamps the low-15 count"
    );

    Fixture partial_failure;
    partial_failure.roles[0].guid = 1U;
    partial_failure.roles[0].interaction_gate = 7U;
    append_message(partial_failure, 0U, 20U);
    append_message(partial_failure, 0xFFFDU, 21U);
    append_message(partial_failure, 1U, 22U);
    partial_failure.dialogs.close.flagged_dialog_counter = 0x12348002U;
    partial_failure.state.previous_opcode = 0x66U;
    prime_loaded_instruction(
        partial_failure, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID
    );
    write_u16(partial_failure.state.window, 2U, 1U);
    const auto partial_failure_result = partial_failure.step();
    test.expect_true(
        partial_failure_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            partial_failure_result.executed_instruction_count == 1U &&
            partial_failure_result.direct_audio_service_count == 0U &&
            partial_failure.context.instruction_offset == 0U &&
            partial_failure.state.previous_opcode == 0x66U &&
            partial_failure.dialogs.messages.size() == 2U &&
            partial_failure.dialogs.messages.front().record.role_index ==
                0xFFFDU &&
            partial_failure.roles[0].interaction_gate == 0U &&
            partial_failure.dialogs.close.flagged_dialog_counter == 0x8001U,
        "opcode 118 retains earlier deletions before an invalid record index"
    );

    Fixture operand_truncated;
    append_message(operand_truncated, 0xFFFDU, 30U);
    operand_truncated.dialogs.close.flagged_dialog_counter = 0x12348001U;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.previous_opcode = 0x66U;
    prime_loaded_instruction(
        operand_truncated, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID
    );
    operand_truncated.context.instruction_offset = 0x7FFEU;
    write_u16(
        operand_truncated.state.window,
        0x7FFEU,
        OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID
    );
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U &&
            operand_truncated.dialogs.messages.size() == 1U &&
            operand_truncated.dialogs.close.flagged_dialog_counter ==
                0x12348001U,
        "opcode 118 reads the selector before touching the dialog list"
    );

    Fixture exact_tail;
    exact_tail.roles[1].interaction_gate = 7U;
    append_message(exact_tail, 1U, 40U);
    exact_tail.dialogs.close.flagged_dialog_counter = 0xCAFE8001U;
    exact_tail.state.previous_opcode = 0x66U;
    prime_loaded_instruction(exact_tail, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID);
    exact_tail.context.instruction_offset = 0x7FFCU;
    write_u16(
        exact_tail.state.window, 0x7FFCU, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID
    );
    write_u16(exact_tail.state.window, 0x7FFEU, 0xFFF0U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.direct_audio_service_count == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
            exact_tail.dialogs.messages.empty() &&
            exact_tail.roles[1].interaction_gate == 0U &&
            exact_tail.dialogs.close.flagged_dialog_counter == 0x8000U &&
            read_u16(exact_tail.state.window, 0x7FFEU) == 0xFFF0U,
        "opcode 118 commits its exact-tail removals before refetch"
    );
}

void test_wait_dialog_flag_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        u32 completion_mask;
        u32 unrelated_mask;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_119_WAIT_DIALOG_FLAG_BIT0, 0x00000001U, 0x00008000U},
        Variant{OP_139_WAIT_DIALOG_FLAG_BIT15, 0x00008000U, 0x00000001U},
    };
    constexpr std::array<u16, 4U> alias_bits{0U, 0x4000U, 0x8000U, 0xC000U};
    for (const auto& variant : variants) {
        for (const u16 alias : alias_bits) {
            Fixture fixture;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | alias)
            );
            write_u16(fixture.state.window, 2U, 0x00F8U);
            write_u16(fixture.state.window, 4U, kStoryVmTypedStop);

            const auto result = fixture.step();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 0U &&
                    fixture.context.instruction_offset == 4U &&
                    fixture.state.previous_opcode == variant.opcode,
                "dialog wait aliases consume an empty message list"
            );
        }

        Fixture matching;
        matching.dialogs.messages.emplace_back();
        matching.dialogs.messages.back().record.role_index = 1U;
        matching.dialogs.messages.back().record.flags = variant.unrelated_mask;
        prime_loaded_instruction(matching, variant.opcode);
        write_u16(matching.state.window, 2U, 0x00F8U);
        write_u16(matching.state.window, 4U, kStoryVmTypedStop);
        const auto waiting = matching.step();
        matching.dialogs.messages.back().record.flags |=
            variant.completion_mask;
        const auto completed = matching.step();
        test.expect_true(
            waiting.status == LegacyWorldStoryVmStatus::yielded &&
                waiting.executed_instruction_count == 1U &&
                waiting.direct_audio_service_count == 1U &&
                matching.ports.direct_audio_service_count == 1U &&
                completed.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                completed.opcode == kStoryVmTypedStop &&
                completed.executed_instruction_count == 2U &&
                completed.direct_audio_service_count == 0U &&
                matching.context.instruction_offset == 4U &&
                matching.state.previous_opcode == variant.opcode,
            "each dialog wait variant uses only its own completion bit"
        );
    }

    Fixture first_match;
    for (const u32 flags : {0U, 1U}) {
        first_match.dialogs.messages.emplace_back();
        first_match.dialogs.messages.back().record.role_index = 1U;
        first_match.dialogs.messages.back().record.flags = flags;
    }
    prime_loaded_instruction(first_match, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(first_match.state.window, 2U, 0x00F8U);
    write_u16(first_match.state.window, 4U, kStoryVmTypedStop);
    const auto first_match_waiting = first_match.step();
    first_match.dialogs.messages.front().record.flags = 1U;
    first_match.dialogs.messages.back().record.flags = 0U;
    const auto first_match_completed = first_match.step();
    test.expect_true(
        first_match_waiting.status == LegacyWorldStoryVmStatus::yielded &&
            first_match_waiting.direct_audio_service_count == 1U &&
            first_match_completed.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            first_match.context.instruction_offset == 4U,
        "dialog waits inspect only the first matching message"
    );

    Fixture detached;
    detached.dialogs.messages.emplace_back();
    detached.dialogs.messages.back().record.role_index = 0xFFFDU;
    detached.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(detached, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(detached.state.window, 2U, 0xFFFDU);
    const auto detached_result = detached.step();
    test.expect_true(
        detached_result.status == LegacyWorldStoryVmStatus::yielded &&
            detached_result.direct_audio_service_count == 1U &&
            detached.context.instruction_offset == 0U &&
            detached.state.previous_opcode == OP_119_WAIT_DIALOG_FLAG_BIT0,
        "dialog wait selector FFFD matches detached messages without lookup"
    );

    Fixture context_detached;
    context_detached.context.source_guid = 0xFFFDU;
    context_detached.dialogs.messages.emplace_back();
    context_detached.dialogs.messages.back().record.role_index = 0xFFFDU;
    context_detached.dialogs.messages.back().record.flags = 1U;
    prime_loaded_instruction(context_detached, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(context_detached.state.window, 2U, 0xFFF0U);
    write_u16(context_detached.state.window, 4U, kStoryVmTypedStop);
    const auto context_detached_result = context_detached.step();
    test.expect_true(
        context_detached_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            context_detached.context.instruction_offset == 4U &&
            context_detached.state.previous_opcode ==
                OP_119_WAIT_DIALOG_FLAG_BIT0 &&
            read_u16(context_detached.state.window, 2U) == 0xFFF0U,
        "dialog wait applies FFFD handling after FFF0 replacement"
    );

    Fixture current_source;
    current_source.dialogs.messages.emplace_back();
    current_source.dialogs.messages.back().record.role_index = 1U;
    current_source.dialogs.messages.back().record.flags = 0x8000U;
    prime_loaded_instruction(current_source, OP_139_WAIT_DIALOG_FLAG_BIT15);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, kStoryVmTypedStop);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            current_source.context.instruction_offset == 4U &&
            current_source.state.previous_opcode ==
                OP_139_WAIT_DIALOG_FLAG_BIT15 &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "dialog wait FFF0 replacement is local and preserves the script"
    );

    Fixture missing;
    missing.dialogs.messages.emplace_back();
    missing.dialogs.messages.back().record.role_index = 0xFFFDU;
    missing.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(missing, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, kStoryVmTypedStop);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            missing_result.executed_instruction_count == 2U &&
            missing_result.direct_audio_service_count == 0U &&
            missing.context.instruction_offset == 4U &&
            missing.dialogs.messages.size() == 1U,
        "dialog wait lookup failure consumes without scanning the list"
    );

    Fixture controlled;
    controlled.roles.resize(4U);
    controlled.dialogs.messages.emplace_back();
    controlled.dialogs.messages.back().record.role_index = 3U;
    controlled.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(controlled, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    const auto controlled_result = controlled.step(0, 0, 3U);
    test.expect_true(
        controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            controlled_result.direct_audio_service_count == 1U &&
            controlled.context.instruction_offset == 0U,
        "dialog wait uses the controlled index directly for matching"
    );

    Fixture wide_controlled;
    wide_controlled.roles.resize(0x00010004U);
    wide_controlled.dialogs.messages.emplace_back();
    wide_controlled.dialogs.messages.back().record.role_index = 3U;
    wide_controlled.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(wide_controlled, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(wide_controlled.state.window, 2U, 0xFFFEU);
    write_u16(wide_controlled.state.window, 4U, kStoryVmTypedStop);
    const auto wide_controlled_result = wide_controlled.step(0, 0, 0x00010003U);
    test.expect_true(
        wide_controlled_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            wide_controlled_result.direct_audio_service_count == 0U &&
            wide_controlled.context.instruction_offset == 4U,
        "dialog wait compares the full controlled index with a u16 record"
    );

    Fixture operand_truncated;
    operand_truncated.dialogs.messages.emplace_back();
    operand_truncated.dialogs.messages.back().record.role_index = 1U;
    operand_truncated.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(operand_truncated, OP_119_WAIT_DIALOG_FLAG_BIT0);
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.state.previous_opcode = 0x66U;
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_119_WAIT_DIALOG_FLAG_BIT0
    );
    const auto operand_truncated_result = operand_truncated.step();

    Fixture waiting_tail;
    waiting_tail.dialogs.messages.emplace_back();
    waiting_tail.dialogs.messages.back().record.role_index = 1U;
    waiting_tail.dialogs.messages.back().record.flags = 0U;
    prime_loaded_instruction(waiting_tail, OP_119_WAIT_DIALOG_FLAG_BIT0);
    waiting_tail.context.instruction_offset = 0x7FFCU;
    write_u16(waiting_tail.state.window, 0x7FFCU, OP_119_WAIT_DIALOG_FLAG_BIT0);
    write_u16(waiting_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto waiting_tail_result = waiting_tail.step();

    Fixture completed_tail;
    completed_tail.dialogs.messages.emplace_back();
    completed_tail.dialogs.messages.back().record.role_index = 1U;
    completed_tail.dialogs.messages.back().record.flags = 0x8000U;
    prime_loaded_instruction(completed_tail, OP_139_WAIT_DIALOG_FLAG_BIT15);
    completed_tail.context.instruction_offset = 0x7FFCU;
    write_u16(
        completed_tail.state.window, 0x7FFCU, OP_139_WAIT_DIALOG_FLAG_BIT15
    );
    write_u16(completed_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto completed_tail_result = completed_tail.step();

    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x66U &&
            operand_truncated_result.direct_audio_service_count == 0U &&
            waiting_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_tail_result.executed_instruction_count == 1U &&
            waiting_tail_result.direct_audio_service_count == 1U &&
            waiting_tail.context.instruction_offset == 0x7FFCU &&
            waiting_tail.state.previous_opcode ==
                OP_119_WAIT_DIALOG_FLAG_BIT0 &&
            completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail_result.direct_audio_service_count == 0U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_139_WAIT_DIALOG_FLAG_BIT15,
        "dialog waits preserve selector and two exact-tail exit contracts"
    );
}

void test_release_role_path_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_18_RELEASE_ROLE_PATH,
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0x4000U),
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0x8000U),
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        for (auto& object : fixture.active_object_slots) {
            object.bytes.fill(0xFFU);
        }
        fixture.roles[1].flags = 0xA0000000U;
        fixture.roles[1].action.wait_remaining = 7U;
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0x00U, 1U);
        slot.bytes[0x1BU] = 2U;
        openswd3::world_map::LegacyWorldStoryPathRuntime paths{};
        paths.roles = fixture.roles;
        paths.active_object_slots = fixture.active_object_slots;
        fixture.runtime.story_paths = &paths;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_18_RELEASE_ROLE_PATH &&
                fixture.roles[1].flags == 0x20000000U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 18 aliases complete a type>1 slot and continue in-call"
        );
    }

    Fixture already_released;
    already_released.roles[1].flags = 0x20000000U;
    already_released.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(already_released, OP_18_RELEASE_ROLE_PATH);
    write_u16(already_released.state.window, 2U, 0x00F8U);
    write_u16(already_released.state.window, 4U, kStoryVmTypedStop);
    const auto already_released_result = already_released.step();
    test.expect_true(
        already_released_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            already_released_result.executed_instruction_count == 2U &&
            already_released.context.instruction_offset == 4U &&
            already_released.roles[1].flags == 0x20000000U &&
            already_released.roles[1].action.wait_remaining == 0U,
        "opcode 18 advances without a story-path runtime after bit31 is clear"
    );

    Fixture no_slot;
    no_slot.roles[1].flags = 0xA0000000U;
    no_slot.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(no_slot, OP_18_RELEASE_ROLE_PATH);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    write_u16(no_slot.state.window, 4U, kStoryVmTypedStop);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            no_slot_result.executed_instruction_count == 3U &&
            no_slot.context.instruction_offset == 4U &&
            no_slot.state.previous_opcode == OP_18_RELEASE_ROLE_PATH &&
            no_slot.roles[1].flags == 0x20000000U &&
            no_slot.roles[1].action.wait_remaining == 0U,
        "opcode 18 clears bit31 then retries once in-call after no slot"
    );

    Fixture type_one;
    type_one.roles[1].flags = 0x80000000U;
    prime_loaded_instruction(type_one, OP_18_RELEASE_ROLE_PATH);
    write_u16(type_one.state.window, 2U, 0x00F8U);
    write_u16(type_one.state.window, 4U, kStoryVmTypedStop);
    write_u16(type_one.active_object_slots[0].bytes, 0x00U, 1U);
    type_one.active_object_slots[0].bytes[0x1BU] = 1U;
    const auto type_one_result = type_one.step();
    test.expect_true(
        type_one_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            type_one_result.executed_instruction_count == 3U &&
            type_one.context.instruction_offset == 4U &&
            type_one.active_object_slots[0].bytes[0x1BU] == 1U,
        "opcode 18 requires slot type low nibble greater than one"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_18_RELEASE_ROLE_PATH);
    write_u16(missing.state.window, 2U, 0x7777U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 18 stops at the original helper role dereference after a miss"
    );

    Fixture raw_current_token;
    prime_loaded_instruction(raw_current_token, OP_18_RELEASE_ROLE_PATH);
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            raw_current_token.context.instruction_offset == 0U,
        "opcode 18 passes FFF0 raw without source substitution"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_18_RELEASE_ROLE_PATH);
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
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 18 obeys the VM controlled-role entry safety boundary"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x80000000U;
    runtime_unavailable.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(runtime_unavailable, OP_18_RELEASE_ROLE_PATH);
    write_u16(runtime_unavailable.state.window, 2U, 0x00F8U);
    write_u16(runtime_unavailable.active_object_slots[0].bytes, 0x00U, 1U);
    runtime_unavailable.active_object_slots[0].bytes[0x1BU] = 2U;
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.roles[1].flags == 0x80000000U &&
            runtime_unavailable.roles[1].action.wait_remaining == 7U,
        "opcode 18 requires the typed owner only after a matching slot"
    );

    Fixture helper_failure;
    helper_failure.roles[1].flags = 0x80000000U;
    helper_failure.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(helper_failure, OP_18_RELEASE_ROLE_PATH);
    write_u16(helper_failure.state.window, 2U, 0x00F8U);
    auto& failed_slot = helper_failure.active_object_slots[0];
    write_u16(failed_slot.bytes, 0x00U, 1U);
    write_u16(failed_slot.bytes, 0x08U, 2U);
    failed_slot.bytes[0x1BU] = 2U;
    openswd3::world_map::LegacyWorldStoryPathRuntime failed_paths{};
    failed_paths.roles = helper_failure.roles;
    failed_paths.active_object_slots = helper_failure.active_object_slots;
    helper_failure.runtime.story_paths = &failed_paths;
    const auto helper_failure_result = helper_failure.step();
    test.expect_true(
        helper_failure_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            helper_failure.context.instruction_offset == 0U &&
            helper_failure.roles[1].flags == 0x80000000U &&
            helper_failure.roles[1].action.wait_remaining == 7U,
        "opcode 18 checked-stops when chained-path ownership is unavailable"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.roles[1].flags = 0x80000000U;
    truncated.roles[1].action.wait_remaining = 7U;
    write_u16(truncated.state.window, 0x7FFEU, OP_18_RELEASE_ROLE_PATH);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.roles[1].flags == 0x80000000U &&
            truncated.roles[1].action.wait_remaining == 7U,
        "opcode 18 reads the selector before any helper or role side effect"
    );
}

void test_release_all_role_paths_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_19_RELEASE_ROLE_PATHS,
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0x4000U),
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0x8000U),
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        for (auto& object : fixture.active_object_slots) {
            object.bytes.fill(0xFFU);
        }
        fixture.roles[0].flags = 0xA0000000U;
        fixture.roles[0].action.wait_remaining = 5U;
        fixture.roles[1].flags = 0xA0000000U;
        fixture.roles[1].action.wait_remaining = 7U;
        fixture.roles[2].flags = 0xA0000000U;
        fixture.roles[2].action.wait_remaining = 9U;
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0x00U, 2U);
        slot.bytes[0x1BU] = 2U;
        openswd3::world_map::LegacyWorldStoryPathRuntime paths{};
        paths.roles = fixture.roles;
        paths.active_object_slots = fixture.active_object_slots;
        fixture.runtime.story_paths = &paths;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS &&
                fixture.roles[0].flags == 0xA0000000U &&
                fixture.roles[0].action.wait_remaining == 5U &&
                fixture.roles[1].flags == 0x20000000U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[2].flags == 0x20000000U &&
                fixture.roles[2].action.wait_remaining == 0U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 19 aliases release flagged roles except role zero"
        );
    }

    Fixture already_released;
    already_released.roles[1].flags = 0x20000000U;
    already_released.roles[1].action.wait_remaining = 7U;
    already_released.roles[2].action.wait_remaining = 9U;
    prime_loaded_instruction(already_released, OP_19_RELEASE_ROLE_PATHS);
    write_u16(already_released.state.window, 2U, kStoryVmTypedStop);
    const auto already_released_result = already_released.step();
    test.expect_true(
        already_released_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            already_released_result.executed_instruction_count == 2U &&
            already_released.context.instruction_offset == 2U &&
            already_released.roles[1].action.wait_remaining == 7U &&
            already_released.roles[2].action.wait_remaining == 9U,
        "opcode 19 leaves bit31-clear role action waits untouched"
    );

    Fixture type_one;
    type_one.roles[1].flags = 0x80000000U;
    type_one.roles[1].action.wait_remaining = 7U;
    write_u16(type_one.active_object_slots[0].bytes, 0x00U, 1U);
    type_one.active_object_slots[0].bytes[0x1BU] = 1U;
    prime_loaded_instruction(type_one, OP_19_RELEASE_ROLE_PATHS);
    write_u16(type_one.state.window, 2U, kStoryVmTypedStop);
    const auto type_one_result = type_one.step();
    test.expect_true(
        type_one_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            type_one_result.executed_instruction_count == 2U &&
            type_one.roles[1].flags == 0U &&
            type_one.roles[1].action.wait_remaining == 0U &&
            type_one.active_object_slots[0].bytes[0x1BU] == 1U,
        "opcode 19 ignores helper zero return for a type-one slot"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x80000000U;
    runtime_unavailable.roles[1].action.wait_remaining = 7U;
    runtime_unavailable.roles[2].flags = 0x80000000U;
    runtime_unavailable.roles[2].action.wait_remaining = 9U;
    write_u16(runtime_unavailable.active_object_slots[0].bytes, 0x00U, 1U);
    runtime_unavailable.active_object_slots[0].bytes[0x1BU] = 2U;
    prime_loaded_instruction(runtime_unavailable, OP_19_RELEASE_ROLE_PATHS);
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.roles[1].flags == 0x80000000U &&
            runtime_unavailable.roles[1].action.wait_remaining == 7U &&
            runtime_unavailable.roles[2].flags == 0x80000000U &&
            runtime_unavailable.roles[2].action.wait_remaining == 9U,
        "opcode 19 stops in role order when a matching slot lacks its owner"
    );

    Fixture one_role;
    one_role.roles[0].flags = 0x80000000U;
    one_role.roles[0].action.wait_remaining = 5U;
    prime_loaded_instruction(one_role, OP_19_RELEASE_ROLE_PATHS);
    write_u16(one_role.state.window, 2U, kStoryVmTypedStop);
    auto one_role_span =
        std::span<LegacyWorldRoleRecord>{one_role.roles}.first(1U);
    const auto one_role_result =
        openswd3::world_map::step_legacy_world_story_vm(
            one_role.context,
            one_role.state,
            one_role_span,
            0U,
            one_role.active_object_slots,
            one_role.maps_payload,
            one_role.dialogs,
            one_role.dialog_resources,
            one_role.first_name,
            one_role.second_name,
            one_role.runtime,
            one_role.ports
        );
    test.expect_true(
        one_role_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            one_role_result.executed_instruction_count == 2U &&
            one_role.context.instruction_offset == 2U &&
            one_role.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS &&
            one_role.roles[0].flags == 0x80000000U &&
            one_role.roles[0].action.wait_remaining == 5U,
        "opcode 19 count-one path skips role zero and consumes two bytes"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFEU, OP_19_RELEASE_ROLE_PATHS);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.context.instruction_offset == 0x8000U &&
            truncated.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS,
        "opcode 19 consumes at the window tail before the next fetch fails"
    );
}

void test_schedule_role_paths_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> opcode20_aliases{
        OP_20_SCHEDULE_ROLE_PATHS,
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0x4000U),
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0x8000U),
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0xC000U)
    };
    for (const u16 raw_word : opcode20_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        fixture.roles[1].flags = 0x00080000U;
        fixture.roles[1].action.cached_base_variant = 11U;
        fixture.roles[1].action.cached_variant_delta = 22U;
        fixture.roles[1].action.one_shot_base_variant = 33U;
        fixture.roles[1].action.one_shot_variant_delta = 44U;
        fixture.roles[1].action.wait_override = 0U;
        fixture.roles[1].interaction_gate = 0x1234U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 1U);
        write_u16(fixture.state.window, 4U, 0x00F8U);
        write_u16(fixture.state.window, 6U, 21U);
        write_u16(fixture.state.window, 8U, 15U);
        write_u16(fixture.state.window, 10U, kStoryVmTypedStop);

        const auto scheduled = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status == LegacyWorldStoryVmStatus::yielded &&
                scheduled.opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                scheduled.executed_instruction_count == 1U &&
                scheduled.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                read_u16(fixture.state.window, 2U) == 0x4001U &&
                (fixture.roles[1].flags & 0x00080000U) == 0U &&
                fixture.roles[1].action.cached_base_variant ==
                    std::numeric_limits<u32>::max() &&
                fixture.roles[1].action.cached_variant_delta ==
                    std::numeric_limits<u32>::max() &&
                fixture.roles[1].action.one_shot_base_variant == 33U &&
                fixture.roles[1].action.one_shot_variant_delta == 44U &&
                fixture.roles[1].action.wait_override == 0x8001U &&
                fixture.roles[1].interaction_gate == 0x1234U &&
                read_u16(slot, 0x04U) == 336U &&
                read_u16(slot, 0x06U) == 240U &&
                read_u16(slot, 0x10U) == 0xFFFFU &&
                read_u16(slot, 0x12U) == 0xFFFFU &&
                read_u16(slot, 0x14U) == 0xFFFFU,
            "opcode 20 aliases schedule six-byte records and publish phase one"
        );

        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();
        test.expect_true(
            completed.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                completed.opcode == kStoryVmTypedStop &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                read_u16(fixture.state.window, 2U) == 1U,
            "opcode 20 ready phase clears high bits and continues in-call"
        );
    }

    constexpr std::array<u16, 4U> opcode169_aliases{
        OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS,
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0x4000U),
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0x8000U),
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0xC000U)
    };
    for (const u16 raw_word : opcode169_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 1U);
        write_u16(fixture.state.window, 4U, 0x00F8U);
        write_u16(fixture.state.window, 6U, 21U);
        write_u16(fixture.state.window, 8U, 15U);
        write_u16(fixture.state.window, 10U, 0x1234U);
        write_u16(fixture.state.window, 12U, 0xFFFEU);
        write_u16(fixture.state.window, 14U, 7U);
        write_u16(fixture.state.window, 16U, kStoryVmTypedStop);

        const auto scheduled = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status == LegacyWorldStoryVmStatus::yielded &&
                scheduled.opcode == OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                scheduled.executed_instruction_count == 1U &&
                scheduled.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                read_u16(fixture.state.window, 2U) == 0x4001U &&
                read_u16(slot, 0x10U) == 0x1234U &&
                read_u16(slot, 0x12U) == 0xFFFEU && read_u16(slot, 0x14U) == 7U,
            "opcode 169 aliases forward twelve-byte action-bearing records"
        );

        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();
        test.expect_true(
            completed.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                completed.opcode == kStoryVmTypedStop &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 16U &&
                fixture.state.previous_opcode ==
                    OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                read_u16(fixture.state.window, 2U) == 1U,
            "opcode 169 ready phase advances by twelve bytes per record"
        );
    }

    struct Variant {
        u16 opcode;
        std::size_t record_size;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_20_SCHEDULE_ROLE_PATHS, 6U},
        Variant{OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS, 12U},
    };
    for (const auto variant : variants) {
        Fixture empty;
        prime_loaded_instruction(empty, variant.opcode);
        write_u16(empty.state.window, 2U, 0U);
        write_u16(empty.state.window, 4U, kStoryVmTypedStop);
        const auto staged = empty.step();
        const u16 staged_count = read_u16(empty.state.window, 2U);
        const auto completed = empty.step();
        test.expect_true(
            staged.status == LegacyWorldStoryVmStatus::yielded &&
                staged.opcode == variant.opcode &&
                staged.executed_instruction_count == 1U &&
                staged_count == 0x4000U &&
                read_u16(empty.state.window, 2U) == 0U &&
                completed.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                completed.opcode == kStoryVmTypedStop &&
                completed.executed_instruction_count == 2U &&
                empty.context.instruction_offset == 4U &&
                empty.state.previous_opcode == variant.opcode,
            "shared path handler count-zero stages then completes without runtime"
        );
    }

    Fixture selected_fallback;
    selected_fallback.roles[1].world_x = 352U;
    selected_fallback.roles[1].world_y = 256U;
    StoryPathHarness selected_paths{selected_fallback, 1U};
    prime_loaded_instruction(selected_fallback, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(selected_fallback.state.window, 2U, 0x8001U);
    write_u16(selected_fallback.state.window, 4U, 0xFFFEU);
    write_u16(selected_fallback.state.window, 6U, 0xFFFFU);
    write_u16(selected_fallback.state.window, 8U, 0xFFFFU);
    const auto selected_staged = selected_fallback.step(0, 0, 1U);
    const auto& selected_slot = selected_fallback.active_object_slots[0].bytes;
    test.expect_true(
        selected_staged.status == LegacyWorldStoryVmStatus::yielded &&
            read_u16(selected_fallback.state.window, 2U) == 0xC001U &&
            read_u16(selected_slot, 0x00U) == 1U &&
            read_u16(selected_slot, 0x04U) == 352U &&
            read_u16(selected_slot, 0x06U) == 256U &&
            (selected_slot[0x1BU] & 0x80U) == 0U &&
            (selected_fallback.dialogs.close.flagged_dialog_counter &
             0x8000U) != 0U,
        "opcode 20 forwards bit15 and uses selected-role FFFF coordinates"
    );
    selected_fallback.roles[1].flags |= 0x02000000U;
    write_u16(selected_fallback.state.window, 10U, kStoryVmTypedStop);
    const auto selected_completed = selected_fallback.step(0, 0, 1U);
    test.expect_true(
        selected_completed.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            selected_fallback.context.instruction_offset == 10U &&
            read_u16(selected_fallback.state.window, 2U) == 1U,
        "ready phase clears both bit14 and forwarded bit15"
    );

    for (const auto variant : variants) {
        Fixture missing_tail;
        missing_tail.context.instruction_offset = 0x7FFAU;
        missing_tail.context.talk_data_offset = 0x1111U;
        missing_tail.state.loaded_file_number = 1U;
        missing_tail.state.loaded_data_offset = 0x1111U;
        missing_tail.state.window_loaded = true;
        missing_tail.state.previous_opcode = 0x55U;
        write_u16(missing_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(missing_tail.state.window, 0x7FFCU, 1U);
        write_u16(missing_tail.state.window, 0x7FFEU, 0xFFF0U);
        const auto missing_tail_result = missing_tail.step();
        test.expect_true(
            missing_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
                missing_tail_result.executed_instruction_count == 1U &&
                missing_tail.context.instruction_offset == 0x7FFAU &&
                missing_tail.state.previous_opcode == variant.opcode &&
                read_u16(missing_tail.state.window, 0x7FFCU) == 0x4001U,
            "missing selector skips its record tail before phase publication"
        );

        Fixture valid_tail;
        valid_tail.context.instruction_offset = 0x7FFAU;
        valid_tail.context.talk_data_offset = 0x1111U;
        valid_tail.state.loaded_file_number = 1U;
        valid_tail.state.loaded_data_offset = 0x1111U;
        valid_tail.state.window_loaded = true;
        valid_tail.state.previous_opcode = 0x55U;
        valid_tail.roles[1].flags = 0x00080000U;
        valid_tail.roles[1].action.cached_base_variant = 11U;
        valid_tail.roles[1].action.cached_variant_delta = 22U;
        write_u16(valid_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(valid_tail.state.window, 0x7FFCU, 1U);
        write_u16(valid_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto valid_tail_result = valid_tail.step();
        test.expect_true(
            valid_tail_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                valid_tail.context.instruction_offset == 0x7FFAU &&
                valid_tail.state.previous_opcode == 0x55U &&
                (valid_tail.roles[1].flags & 0x00080000U) == 0U &&
                valid_tail.roles[1].action.cached_base_variant ==
                    std::numeric_limits<u32>::max() &&
                valid_tail.roles[1].action.cached_variant_delta ==
                    std::numeric_limits<u32>::max() &&
                valid_tail.roles[1].action.wait_override == 0x8001U,
            "valid selector applies role effects before truncated coordinates"
        );

        Fixture wait_tail;
        wait_tail.context.instruction_offset = 0x7FFAU;
        wait_tail.context.talk_data_offset = 0x1111U;
        wait_tail.state.loaded_file_number = 1U;
        wait_tail.state.loaded_data_offset = 0x1111U;
        wait_tail.state.window_loaded = true;
        wait_tail.roles[1].flags = 0x02000000U;
        write_u16(wait_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(wait_tail.state.window, 0x7FFCU, 0x4001U);
        write_u16(wait_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto wait_tail_result = wait_tail.step();
        const u16 expected_ip =
            static_cast<u16>(0x7FFAU + 4U + variant.record_size);
        test.expect_true(
            wait_tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                wait_tail_result.executed_instruction_count == 1U &&
                wait_tail.context.instruction_offset == expected_ip &&
                wait_tail.state.previous_opcode == variant.opcode &&
                read_u16(wait_tail.state.window, 0x7FFCU) == 1U,
            "ready phase reads only selectors before advancing past the window"
        );
    }

    Fixture action_tail;
    action_tail.context.instruction_offset = 0x7FF4U;
    action_tail.context.talk_data_offset = 0x1111U;
    action_tail.state.loaded_file_number = 1U;
    action_tail.state.loaded_data_offset = 0x1111U;
    action_tail.state.window_loaded = true;
    action_tail.state.previous_opcode = 0x55U;
    action_tail.roles[1].flags = 0x00080000U;
    action_tail.roles[1].action.cached_base_variant = 11U;
    action_tail.roles[1].action.cached_variant_delta = 22U;
    write_u16(
        action_tail.state.window,
        0x7FF4U,
        OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS
    );
    write_u16(action_tail.state.window, 0x7FF6U, 1U);
    write_u16(action_tail.state.window, 0x7FF8U, 0x00F8U);
    write_u16(action_tail.state.window, 0x7FFAU, 21U);
    write_u16(action_tail.state.window, 0x7FFCU, 15U);
    write_u16(action_tail.state.window, 0x7FFEU, 0x1234U);
    const auto action_tail_result = action_tail.step();
    test.expect_true(
        action_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            action_tail.context.instruction_offset == 0x7FF4U &&
            action_tail.state.previous_opcode == 0x55U &&
            (action_tail.roles[1].flags & 0x00080000U) == 0U &&
            action_tail.roles[1].action.cached_base_variant ==
                std::numeric_limits<u32>::max() &&
            action_tail.roles[1].action.cached_variant_delta ==
                std::numeric_limits<u32>::max() &&
            action_tail.roles[1].action.wait_override == 0x8001U &&
            read_u16(action_tail.state.window, 0x7FF6U) == 1U,
        "opcode 169 applies role effects before a truncated action tail"
    );

    Fixture wait_missing;
    prime_loaded_instruction(wait_missing, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(wait_missing.state.window, 2U, 0x4001U);
    write_u16(wait_missing.state.window, 4U, 0x7777U);
    wait_missing.state.previous_opcode = 0x55U;
    const auto wait_missing_result = wait_missing.step();
    test.expect_true(
        wait_missing_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            wait_missing.context.instruction_offset == 0U &&
            wait_missing.state.previous_opcode == 0x55U &&
            read_u16(wait_missing.state.window, 2U) == 0x4001U,
        "wait phase stops at the original role dereference after resolver miss"
    );

    Fixture not_ready;
    StoryPathHarness not_ready_paths{not_ready};
    prime_loaded_instruction(not_ready, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(not_ready.state.window, 2U, 0x4001U);
    write_u16(not_ready.state.window, 4U, 0x00F8U);
    const auto not_ready_result = not_ready.step();
    test.expect_true(
        not_ready_result.status == LegacyWorldStoryVmStatus::yielded &&
            not_ready_result.executed_instruction_count == 1U &&
            not_ready.context.instruction_offset == 0U &&
            not_ready.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
            read_u16(not_ready.state.window, 2U) == 0x4001U,
        "wait phase keeps the instruction staged while a role is not ready"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x00080000U;
    runtime_unavailable.roles[1].action.cached_base_variant = 11U;
    runtime_unavailable.roles[1].action.cached_variant_delta = 22U;
    prime_loaded_instruction(runtime_unavailable, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(runtime_unavailable.state.window, 2U, 1U);
    write_u16(runtime_unavailable.state.window, 4U, 0x00F8U);
    write_u16(runtime_unavailable.state.window, 6U, 21U);
    write_u16(runtime_unavailable.state.window, 8U, 15U);
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.state.previous_opcode == 0U &&
            (runtime_unavailable.roles[1].flags & 0x00080000U) == 0U &&
            runtime_unavailable.roles[1].action.cached_base_variant ==
                std::numeric_limits<u32>::max() &&
            runtime_unavailable.roles[1].action.cached_variant_delta ==
                std::numeric_limits<u32>::max() &&
            runtime_unavailable.roles[1].action.wait_override == 0x8001U &&
            read_u16(runtime_unavailable.state.window, 2U) == 1U,
        "initial phase preserves role effects before missing path runtime"
    );
}

void test_jump_if_global_bit_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        bool branch_when_set;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_21_JUMP_IF_GLOBAL_BIT_SET, true},
        Variant{OP_22_JUMP_IF_GLOBAL_BIT_CLEAR, false},
    };

    for (const auto variant : variants) {
        constexpr std::array<u16, 4U> modifiers{0U, 0x4000U, 0x8000U, 0xC000U};
        for (const u16 modifier : modifiers) {
            Fixture fixture;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | modifier)
            );
            write_u16(fixture.state.window, 2U, 0x0123U);
            write_u32(fixture.state.window, 4U, 0x12345678U);
            if (variant.branch_when_set) {
                openswd3::world_map::set_legacy_world_story_flag(
                    fixture.state, 0x0123U
                );
            }
            write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
            const auto result = fixture.step();
            test.expect_true(
                result.status ==
                        LegacyWorldStoryVmStatus::
                            script_variable_index_out_of_range &&
                    result.opcode == kStoryVmTypedStop &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.context.talk_data_offset == 0x12345678U &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.data_load_count == 1U,
                "opcode 21/22 aliases branch on the XOR-selected flag state"
            );
        }

        Fixture sequential;
        prime_loaded_instruction(sequential, variant.opcode);
        write_u16(sequential.state.window, 2U, 0x0123U);
        write_u32(sequential.state.window, 4U, 0x12345678U);
        if (!variant.branch_when_set) {
            openswd3::world_map::set_legacy_world_story_flag(
                sequential.state, 0x0123U
            );
        }
        write_u16(sequential.state.window, 8U, kStoryVmTypedStop);
        const auto sequential_result = sequential.step();
        test.expect_true(
            sequential_result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                sequential_result.opcode == kStoryVmTypedStop &&
                sequential_result.executed_instruction_count == 2U &&
                sequential_result.direct_audio_service_count == 0U &&
                sequential.context.instruction_offset == 8U &&
                sequential.state.previous_opcode == variant.opcode &&
                sequential.ports.data_load_count == 0U,
            "opcode 21/22 advance eight bytes on the opposite flag state"
        );
    }

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_21_JUMP_IF_GLOBAL_BIT_SET);
    write_u16(load_failure.state.window, 2U, 0x0123U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    openswd3::world_map::set_legacy_world_story_flag(
        load_failure.state, 0x0123U
    );
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_21_JUMP_IF_GLOBAL_BIT_SET &&
            !load_failure.state.window_loaded,
        "opcode 21 preserves same-file loader effects before I/O failure"
    );

    Fixture missing_bit;
    missing_bit.context.instruction_offset = 0x7FFEU;
    missing_bit.context.talk_data_offset = 0x1111U;
    missing_bit.state.loaded_file_number = 1U;
    missing_bit.state.loaded_data_offset = 0x1111U;
    missing_bit.state.window_loaded = true;
    missing_bit.state.previous_opcode = 0x55U;
    write_u16(missing_bit.state.window, 0x7FFEU, OP_21_JUMP_IF_GLOBAL_BIT_SET);
    const auto missing_bit_result = missing_bit.step();
    test.expect_true(
        missing_bit_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_bit.context.instruction_offset == 0x7FFEU &&
            missing_bit.state.previous_opcode == 0x55U,
        "opcode 21 reads the bit selector before any control decision"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FFCU;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FFCU, OP_21_JUMP_IF_GLOBAL_BIT_SET
    );
    write_u16(missing_target.state.window, 0x7FFEU, 0x0123U);
    openswd3::world_map::set_legacy_world_story_flag(
        missing_target.state, 0x0123U
    );
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FFCU &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 21 branch reads the target at its original danger point"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FFCU;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FFCU, OP_21_JUMP_IF_GLOBAL_BIT_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFEU, 0x0123U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8004U &&
            sequential_tail.state.previous_opcode ==
                OP_21_JUMP_IF_GLOBAL_BIT_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 21 no-branch advances before the next fetch fails"
    );
}

void test_jump_if_all_global_bits_set_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET,
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0x4000U),
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0x8000U),
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0456U);
        write_u16(fixture.state.window, 6U, 0xFF00U);
        write_u32(fixture.state.window, 8U, 0x12345678U);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0123U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0456U
        );
        write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
                fixture.ports.data_load_count == 1U,
            "opcode 23 aliases jump when every listed global bit is set"
        );
    }

    Fixture one_clear;
    prime_loaded_instruction(one_clear, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(one_clear.state.window, 2U, 0x0123U);
    write_u16(one_clear.state.window, 4U, 0x0456U);
    write_u16(one_clear.state.window, 6U, 0xFF00U);
    write_u32(one_clear.state.window, 8U, 0x12345678U);
    write_u16(one_clear.state.window, 12U, kStoryVmTypedStop);
    openswd3::world_map::set_legacy_world_story_flag(one_clear.state, 0x0123U);
    const auto one_clear_result = one_clear.step();
    test.expect_true(
        one_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            one_clear_result.opcode == kStoryVmTypedStop &&
            one_clear_result.executed_instruction_count == 2U &&
            one_clear_result.direct_audio_service_count == 0U &&
            one_clear.context.instruction_offset == 12U &&
            one_clear.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            one_clear.ports.data_load_count == 0U,
        "opcode 23 advances over the full list when any bit is clear"
    );

    Fixture empty;
    prime_loaded_instruction(empty, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(empty.state.window, 2U, 0xFF00U);
    write_u32(empty.state.window, 4U, 0x22223333U);
    write_u16(empty.ports.transferred_window, 0U, kStoryVmTypedStop);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            empty_result.opcode == kStoryVmTypedStop &&
            empty_result.direct_audio_service_count == 1U &&
            empty.context.talk_data_offset == 0x22223333U &&
            empty.state.previous_opcode == OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET,
        "opcode 23 treats an empty bit list as unconditionally true"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(load_failure.state.window, 2U, 0xFF00U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            !load_failure.state.window_loaded,
        "opcode 23 preserves loader effects before taken-branch I/O failure"
    );

    Fixture unterminated;
    unterminated.context.instruction_offset = 0x7FFCU;
    unterminated.context.talk_data_offset = 0x1111U;
    unterminated.state.loaded_file_number = 1U;
    unterminated.state.loaded_data_offset = 0x1111U;
    unterminated.state.window_loaded = true;
    unterminated.state.previous_opcode = 0x55U;
    write_u16(
        unterminated.state.window, 0x7FFCU, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(unterminated.state.window, 0x7FFEU, 0x0123U);
    const auto unterminated_result = unterminated.step();
    test.expect_true(
        unterminated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            unterminated.context.instruction_offset == 0x7FFCU &&
            unterminated.state.previous_opcode == 0x55U,
        "opcode 23 scans for FF00 until the original list danger point"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FFCU;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FFCU, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(missing_target.state.window, 0x7FFEU, 0xFF00U);
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FFCU &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 23 taken empty-list branch reads the target after FF00"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FF8U;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FF8U, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFAU, 0x0123U);
    write_u16(sequential_tail.state.window, 0x7FFCU, 0xFF00U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8002U &&
            sequential_tail.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 23 no-branch skips an unavailable target before fetch fails"
    );
}

void test_jump_if_any_global_bit_set_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET,
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0x4000U),
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0x8000U),
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0456U);
        write_u16(fixture.state.window, 6U, 0xFF00U);
        write_u32(fixture.state.window, 8U, 0x12345678U);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0456U
        );
        write_u16(fixture.ports.transferred_window, 0U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
                fixture.ports.data_load_count == 1U,
            "opcode 24 aliases jump when any listed global bit is set"
        );
    }

    Fixture all_clear;
    prime_loaded_instruction(all_clear, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(all_clear.state.window, 2U, 0x0123U);
    write_u16(all_clear.state.window, 4U, 0x0456U);
    write_u16(all_clear.state.window, 6U, 0xFF00U);
    write_u32(all_clear.state.window, 8U, 0x12345678U);
    write_u16(all_clear.state.window, 12U, kStoryVmTypedStop);
    const auto all_clear_result = all_clear.step();
    test.expect_true(
        all_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            all_clear_result.opcode == kStoryVmTypedStop &&
            all_clear_result.executed_instruction_count == 2U &&
            all_clear_result.direct_audio_service_count == 0U &&
            all_clear.context.instruction_offset == 12U &&
            all_clear.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            all_clear.ports.data_load_count == 0U,
        "opcode 24 advances over the full list when every bit is clear"
    );

    Fixture empty;
    prime_loaded_instruction(empty, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(empty.state.window, 2U, 0xFF00U);
    write_u32(empty.state.window, 4U, 0x22223333U);
    write_u16(empty.state.window, 8U, kStoryVmTypedStop);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            empty_result.opcode == kStoryVmTypedStop &&
            empty_result.direct_audio_service_count == 0U &&
            empty.context.instruction_offset == 8U &&
            empty.state.previous_opcode == OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            empty.ports.data_load_count == 0U,
        "opcode 24 treats an empty bit list as false"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(load_failure.state.window, 2U, 0x0123U);
    write_u16(load_failure.state.window, 4U, 0xFF00U);
    write_u32(load_failure.state.window, 6U, 0x44445555U);
    openswd3::world_map::set_legacy_world_story_flag(
        load_failure.state, 0x0123U
    );
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            !load_failure.state.window_loaded,
        "opcode 24 preserves loader effects before taken-branch I/O failure"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FF8U;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FF8U, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET
    );
    write_u16(missing_target.state.window, 0x7FFAU, 0x0123U);
    write_u16(missing_target.state.window, 0x7FFCU, 0xFF00U);
    openswd3::world_map::set_legacy_world_story_flag(
        missing_target.state, 0x0123U
    );
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FF8U &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 24 taken branch reads the target only after FF00"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FF8U;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FF8U, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFAU, 0x0123U);
    write_u16(sequential_tail.state.window, 0x7FFCU, 0xFF00U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8002U &&
            sequential_tail.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 24 no-branch skips an unavailable target before fetch fails"
    );
}

void test_set_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_25_SET_GLOBAL_BIT,
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0x4000U),
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0x8000U),
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0123U
                ) &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_25_SET_GLOBAL_BIT,
            "opcode 25 aliases set the addressed global bit then continue"
        );
    }

    Fixture already_set;
    prime_loaded_instruction(already_set, OP_25_SET_GLOBAL_BIT);
    write_u16(already_set.state.window, 2U, 0x0123U);
    write_u16(already_set.state.window, 4U, kStoryVmTypedStop);
    openswd3::world_map::set_legacy_world_story_flag(
        already_set.state, 0x0123U
    );
    const auto already_set_result = already_set.step();
    test.expect_true(
        already_set_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            openswd3::world_map::query_legacy_world_story_flag(
                already_set.state, 0x0123U
            ) &&
            already_set.state.previous_opcode == OP_25_SET_GLOBAL_BIT,
        "opcode 25 is idempotent when the bit is already set"
    );

    constexpr u16 last_valid_bit = static_cast<u16>(
        openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U - 1U
    );
    Fixture last_bit;
    prime_loaded_instruction(last_bit, OP_25_SET_GLOBAL_BIT);
    write_u16(last_bit.state.window, 2U, last_valid_bit);
    write_u16(last_bit.state.window, 4U, kStoryVmTypedStop);
    const auto last_bit_result = last_bit.step();
    test.expect_true(
        last_bit_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            openswd3::world_map::query_legacy_world_story_flag(
                last_bit.state, last_valid_bit
            ),
        "opcode 25 sets the final bit owned by the typed global flag array"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFEU, OP_25_SET_GLOBAL_BIT);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            !openswd3::world_map::query_legacy_world_story_flag(
                truncated.state, 0U
            ),
        "opcode 25 reads the bit operand before mutating global flags"
    );
}

void test_clear_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_26_CLEAR_GLOBAL_BIT,
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0x4000U),
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0x8000U),
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, kStoryVmTypedStop);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0120U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0123U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0127U
        );
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    LegacyWorldStoryVmStatus::
                        script_variable_index_out_of_range &&
                result.opcode == kStoryVmTypedStop &&
                result.executed_instruction_count == 2U &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0120U
                ) &&
                !openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0123U
                ) &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0127U
                ) &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_26_CLEAR_GLOBAL_BIT,
            "opcode 26 aliases clear only the addressed global bit"
        );
    }

    Fixture already_clear;
    prime_loaded_instruction(already_clear, OP_26_CLEAR_GLOBAL_BIT);
    write_u16(already_clear.state.window, 2U, 0x0123U);
    write_u16(already_clear.state.window, 4U, kStoryVmTypedStop);
    const auto already_clear_result = already_clear.step();
    test.expect_true(
        already_clear_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            !openswd3::world_map::query_legacy_world_story_flag(
                already_clear.state, 0x0123U
            ) &&
            already_clear.state.previous_opcode == OP_26_CLEAR_GLOBAL_BIT,
        "opcode 26 is idempotent when the bit is already clear"
    );

    constexpr u16 last_valid_bit = static_cast<u16>(
        openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U - 1U
    );
    Fixture last_bit;
    prime_loaded_instruction(last_bit, OP_26_CLEAR_GLOBAL_BIT);
    write_u16(last_bit.state.window, 2U, last_valid_bit);
    write_u16(last_bit.state.window, 4U, kStoryVmTypedStop);
    openswd3::world_map::set_legacy_world_story_flag(
        last_bit.state, last_valid_bit
    );
    const auto last_bit_result = last_bit.step();
    test.expect_true(
        last_bit_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            !openswd3::world_map::query_legacy_world_story_flag(
                last_bit.state, last_valid_bit
            ),
        "opcode 26 clears the final bit owned by the typed global flag array"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    openswd3::world_map::set_legacy_world_story_flag(truncated.state, 0U);
    write_u16(truncated.state.window, 0x7FFEU, OP_26_CLEAR_GLOBAL_BIT);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            openswd3::world_map::query_legacy_world_story_flag(
                truncated.state, 0U
            ),
        "opcode 26 reads the bit operand before mutating global flags"
    );
}
