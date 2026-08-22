#include "legacy_world_story_vm_test_cases.hpp"
#include "legacy_world_story_vm_test_support.hpp"

void test_real_set_role_action_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000051C9);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[1].action.action_id = 0xDEADBEEFU;
    prime_loaded_instruction(fixture, OP_45_SET_ROLE_ACTION_ID);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_45_SET_ROLE_ACTION_ID &&
            read_u16(instruction, 2U) == 0x00F8U &&
            read_u16(instruction, 4U) == 0x0223U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 1U &&
            fixture.roles[1].action.action_id == 0x0223U &&
            (fixture.roles[1].flags & 0x1000U) != 0U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 45 changes the requested role action and continues"
    );
}

void test_real_camera_move_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    std::array<u8, 10U> relative_instruction{};
    input.seekg(0x0000A18C);
    input.read(
        reinterpret_cast<char*>(relative_instruction.data()),
        static_cast<std::streamsize>(relative_instruction.size())
    );
    const bool relative_read = static_cast<bool>(input);

    input.clear();
    std::array<u8, 10U> absolute_instruction{};
    input.seekg(0x000046B8);
    input.read(
        reinterpret_cast<char*>(absolute_instruction.data()),
        static_cast<std::streamsize>(absolute_instruction.size())
    );
    const bool absolute_read = static_cast<bool>(input);

    input.clear();
    std::array<u8, 8U> role_instruction{};
    input.seekg(0x000096A3);
    input.read(
        reinterpret_cast<char*>(role_instruction.data()),
        static_cast<std::streamsize>(role_instruction.size())
    );
    const bool role_read = static_cast<bool>(input);

    CameraMoveFixture relative;
    relative.camera.right = 640U;
    relative.camera.bottom = 1120U;
    prime_loaded_instruction(relative, OP_50_START_RELATIVE_CAMERA_MOVE);
    std::ranges::copy(relative_instruction, relative.state.window.begin());
    write_u16(relative.state.window, 10U, OP_1025);
    const auto relative_result = relative.step(0, 640);

    CameraMoveFixture absolute;
    absolute.camera.right = 656U;
    absolute.camera.bottom = 512U;
    prime_loaded_instruction(absolute, OP_70_START_ABSOLUTE_CAMERA_MOVE);
    std::ranges::copy(absolute_instruction, absolute.state.window.begin());
    write_u16(absolute.state.window, 10U, OP_1025);
    const auto absolute_result = absolute.step(16, 32);

    CameraMoveFixture role;
    role.roles[1].guid = 1U;
    role.roles[1].world_x = 800U;
    role.roles[1].world_y = 640U;
    prime_loaded_instruction(role, OP_73_START_CAMERA_MOVE_TO_ROLE);
    std::ranges::copy(role_instruction, role.state.window.begin());
    write_u16(role.state.window, 8U, OP_1025);
    const auto role_result = role.step();

    test.expect_true(
        relative_read &&
            read_u16(relative_instruction, 0U) ==
                OP_50_START_RELATIVE_CAMERA_MOVE &&
            static_cast<i16>(read_u16(relative_instruction, 2U)) == 0 &&
            static_cast<i16>(read_u16(relative_instruction, 4U)) == -32 &&
            read_u16(relative_instruction, 6U) == 8U &&
            read_u16(relative_instruction, 8U) == 4U &&
            relative_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            relative_result.executed_instruction_count == 2U &&
            relative.camera_pan.remaining_x == 0 &&
            relative.camera_pan.remaining_y == -512 &&
            relative.camera_pan.step_x == 0 &&
            relative.camera_pan.step_y == -4 &&
            relative.context.instruction_offset == 10U &&
            relative.state.previous_opcode == OP_50_START_RELATIVE_CAMERA_MOVE,
        "real opcode 50 starts the requested relative camera move"
    );
    test.expect_true(
        absolute_read &&
            read_u16(absolute_instruction, 0U) ==
                OP_70_START_ABSOLUTE_CAMERA_MOVE &&
            read_u16(absolute_instruction, 2U) == 5U &&
            read_u16(absolute_instruction, 4U) == 8U &&
            read_u16(absolute_instruction, 6U) == 4U &&
            read_u16(absolute_instruction, 8U) == 16U &&
            absolute_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            absolute_result.executed_instruction_count == 2U &&
            absolute.camera_pan.remaining_x == 64 &&
            absolute.camera_pan.remaining_y == 96 &&
            absolute.camera_pan.step_x == 4 &&
            absolute.camera_pan.step_y == 16 &&
            absolute.context.instruction_offset == 10U &&
            absolute.state.previous_opcode == OP_70_START_ABSOLUTE_CAMERA_MOVE,
        "real opcode 70 starts the requested absolute camera move"
    );
    test.expect_true(
        role_read &&
            read_u16(role_instruction, 0U) == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            read_u16(role_instruction, 2U) == 1U &&
            read_u16(role_instruction, 4U) == 8U &&
            read_u16(role_instruction, 6U) == 8U &&
            role_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            role_result.executed_instruction_count == 2U &&
            role.camera_pan.remaining_x == 480 &&
            role.camera_pan.remaining_y == 400 && role.camera_pan.step_x == 8 &&
            role.camera_pan.step_y == 8 &&
            role.context.instruction_offset == 8U &&
            role.state.previous_opcode == OP_73_START_CAMERA_MOVE_TO_ROLE,
        "real opcode 73 starts a camera move toward role 1"
    );
}

void test_real_start_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000043B8);
    std::array<u8, 16U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState color{};
    fixture.runtime.frame_color = &color;
    prime_loaded_instruction(fixture, OP_52_START_FRAME_COLOR_TRANSITION);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 16U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_52_START_FRAME_COLOR_TRANSITION &&
            static_cast<i16>(read_u16(instruction, 2U)) == -30 &&
            static_cast<i16>(read_u16(instruction, 4U)) == -30 &&
            static_cast<i16>(read_u16(instruction, 6U)) == -30 &&
            read_u16(instruction, 8U) == 0U &&
            read_u16(instruction, 10U) == 0U &&
            read_u16(instruction, 12U) == 0U &&
            read_u16(instruction, 14U) == 6U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            color.current_red == -30.0F && color.current_green == -30.0F &&
            color.current_blue == -30.0F && color.target_red == 0.0F &&
            color.target_green == 0.0F && color.target_blue == 0.0F &&
            color.countdown == 6 && color.step_red == 5.0F &&
            color.step_green == 5.0F && color.step_blue == 5.0F &&
            fixture.context.instruction_offset == 16U &&
            fixture.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 52 initializes the scripted frame-color transition"
    );
}

void test_real_play_sound_effect_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](
                                 const std::filesystem::path& filename,
                                 const std::streamoff file_offset
                             ) -> std::array<u8, 4U> {
        std::ifstream input{root / filename, std::ios::binary | std::ios::in};
        input.seekg(file_offset);
        std::array<u8, 4U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        return instruction;
    };
    const auto minimum = read_record("TALK2.DAT", 0x00003370);
    const auto maximum = read_record("TALK3.DAT", 0x00015D89);

    Fixture fixture;
    const auto execute = [&fixture](const std::array<u8, 4U>& instruction) {
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;
        return fixture.step();
    };
    const auto minimum_result = execute(minimum);
    const auto maximum_result = execute(maximum);

    test.expect_true(
        read_u16(minimum, 0U) == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(minimum, 2U) == 1U &&
            read_u16(maximum, 0U) == OP_59_PLAY_SOUND_EFFECT &&
            read_u16(maximum, 2U) == 656U &&
            minimum_result.status == LegacyWorldStoryVmStatus::yielded &&
            maximum_result.status == LegacyWorldStoryVmStatus::yielded &&
            fixture.ports.sound_effect_requests == std::vector<u16>{1U, 656U} &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            fixture.ports.direct_audio_service_count == 2U,
        "real opcode 59 records submit the minimum and maximum observed sound ids"
    );
}

void test_real_write_map_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00005B5D);
    std::array<u8, 18U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 0x9999U;
    MapRoleWriteHarness harness{fixture};
    harness.add_source({
        .logical_map_id = 1U,
        .guid = 0x00F8U,
        .action_id = 1U,
        .base_variant = 2U,
        .variant_delta = 3U,
        .tile_x = 4U,
        .tile_y = 5U,
        .talk_script_id = 6U,
        .path_data_id = 7U,
        .path_word_index = 8,
        .flags = 0x0100U,
    });
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 18U, 67U);
    write_u16(fixture.state.window, 20U, 0U);
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();
    const auto& source = harness.database.role_sources.front();

    test.expect_true(
        read_u16(instruction, 0U) == OP_62_WRITE_MAP_ROLE &&
            read_u16(instruction, 2U) == 0x00F8U &&
            read_u16(instruction, 4U) == 81U &&
            read_u16(instruction, 6U) == 0U &&
            read_u16(instruction, 8U) == 26U &&
            read_u16(instruction, 10U) == 26U &&
            read_u16(instruction, 12U) == 623U &&
            read_u16(instruction, 14U) == 0U &&
            read_u16(instruction, 16U) == 4U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 67U && result.executed_instruction_count == 2U &&
            result.role_materialization_count == 0U &&
            source.logical_map_id == 81U && source.guid == 0x00F8U &&
            source.path_data_id == 0U && source.path_word_index == 0 &&
            source.tile_x == 26U && source.tile_y == 26U &&
            source.action_id == 623U && source.base_variant == 0U &&
            source.variant_delta == 4U &&
            fixture.context.instruction_offset == 18U &&
            fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "real opcode 62 record writes GUID 248 onto map 81 with its observed path, coordinates and action tuple"
    );
}

void test_real_set_selection_scroll_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00022D55);
    std::array<u8, 22U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.selection_scroll = {
        .cursor_word_index = 9U,
        .frames_remaining = 10,
        .frame_interval = 11,
        .saved_left = 12U,
        .saved_top = 13U,
    };
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 22U, 67U);
    write_u16(fixture.state.window, 24U, 0U);
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step(100, 200);
    constexpr std::array<u16, 8U> expected{
        1U,
        1U,
        0U,
        0U,
        0xFFFFU,
        1U,
        0U,
        0U,
    };
    bool table_matches = true;
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        table_matches = table_matches &&
            std::bit_cast<u16>(fixture.selection_words[index]) ==
                expected[index];
    }

    test.expect_equal(
        input.gcount(),
        static_cast<std::streamsize>(instruction.size()),
        "real opcode 63 record read length"
    );
    test.expect_equal(
        read_u16(instruction, 0U),
        OP_63_SET_SELECTION_SCROLL,
        "real opcode 63 raw word"
    );
    test.expect_equal(
        read_u16(instruction, 2U), u16{4U}, "real opcode 63 prefix"
    );
    test.expect_equal(
        read_u16(instruction, 20U), u16{0xFF00U}, "real opcode 63 terminator"
    );
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 67U && result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 22U &&
            fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "real opcode 63 record publishes previous and continues to the following wait in the same call"
    );
    test.expect_true(
        table_matches &&
            std::bit_cast<u16>(fixture.selection_words[8]) ==
                openswd3::world_map::kLegacyWorldSelectionSentinel,
        "real opcode 63 record installs its eight observed scroll words and retains a CFCF tail"
    );
    test.expect_true(
        fixture.selection_scroll.cursor_word_index == 9U &&
            fixture.selection_scroll.frame_interval == 4 &&
            fixture.selection_scroll.frames_remaining == 4 &&
            fixture.selection_scroll.saved_left == 100U &&
            fixture.selection_scroll.saved_top == 200U,
        "real opcode 63 record preserves the cursor, writes both timing fields and snapshots the viewport"
    );
}

void test_real_clear_selection_scroll_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00025E07);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.selection_words.fill(std::bit_cast<i16>(u16{0x1234U}));
    fixture.selection_scroll = {
        .cursor_word_index = 1U,
        .frames_remaining = 2,
        .frame_interval = 3,
        .saved_left = 4U,
        .saved_top = 5U,
    };
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, 67U);
    write_u16(fixture.state.window, 4U, 0U);
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step(100, 200);

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_64_CLEAR_SELECTION_SCROLL &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 67U && result.executed_instruction_count == 2U &&
            std::ranges::all_of(
                fixture.selection_words,
                [](const i16 value) {
                    return std::bit_cast<u16>(value) ==
                        openswd3::world_map::kLegacyWorldSelectionSentinel;
                }
            ) &&
            fixture.selection_scroll.cursor_word_index == 1U &&
            fixture.selection_scroll.frames_remaining == 2 &&
            fixture.selection_scroll.frame_interval == 3 &&
            fixture.selection_scroll.saved_left == 4U &&
            fixture.selection_scroll.saved_top == 5U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK,
        "real opcode 64 record clears only the selection table and continues to the following wait"
    );
}

void test_real_transfer_role_to_party_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000FE4F);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 3U;
    fixture.roles[1].flags = 0x00004082U;
    fixture.roles[1].talk_script_id = 0x0033U;
    fixture.roles[1].path_data_id = 0U;
    fixture.live_party_object_slots[1].bytes.fill(0x44U);
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_65_TRANSFER_ROLE_TO_PARTY &&
            read_u16(instruction, 2U) == 3U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.role_transfer_status ==
                openswd3::world_map::LegacyWorldRoleTransferStatus::ready &&
            fixture.role_transfer_state.party_role_count == 2U &&
            fixture.role_transfer_state.party_role_indices[1] == 1U &&
            fixture.live_party_role_count == 2U &&
            std::ranges::all_of(
                fixture.live_party_object_slots[1].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            fixture.roles[1].talk_script_id == 0U &&
            fixture.roles[1].flags == 0x00000082U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_65_TRANSFER_ROLE_TO_PARTY,
        "real opcode 65 record transfers GUID 3 into party bookkeeping and yields"
    );
}

void test_real_update_role_map_state_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00029F5D);
    std::array<u8, 16U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    MapRoleWriteHarness runtime{fixture};
    fixture.roles[1].guid = 9U;
    fixture.roles[1].world_x = 0x20U;
    fixture.roles[1].world_y = 0x20U;
    fixture.roles[1].map_cell_pointer_32 = 2U;
    fixture.roles[1].flags = 0x80U;
    fixture.roles[1].action.field_2c = 1U;
    fixture.roles[1].action.field_30 = 1U;
    runtime.add_source({
        .logical_map_id = 81U,
        .guid = 9U,
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
    fixture.role_transfer_state.party_role_count = 2U;
    fixture.live_party_role_count = 2U;
    fixture.role_transfer_state.party_role_indices = {
        10U,
        1U,
        12U,
        13U,
        14U,
        15U,
        16U,
        17U,
    };
    for (std::size_t index = 0U; index < fixture.live_party_object_slots.size();
         ++index) {
        fixture.live_party_object_slots[index].bytes.fill(
            static_cast<u8>(index)
        );
        write_u16(fixture.live_party_object_slots[index].bytes, 2U, 0x7FFFU);
    }
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();
    const auto& source = runtime.database.role_sources.front();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_66_UPDATE_ROLE_MAP_STATE &&
            read_u16(instruction, 2U) == 9U &&
            read_u16(instruction, 4U) == 0U &&
            read_u16(instruction, 6U) == 9U &&
            read_u16(instruction, 8U) == 9U &&
            read_u16(instruction, 10U) == 0U &&
            read_u16(instruction, 12U) == 7U &&
            read_u16(instruction, 14U) == 0xD100U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.role_map_update.status ==
                openswd3::world_map::LegacyWorldRoleMapUpdateStatus::ready &&
            result.role_map_update.party_role_removed &&
            fixture.roles[1].path_data_id == 0U &&
            fixture.roles[1].path_word_index == 0U &&
            fixture.roles[1].talk_script_id == 9U &&
            fixture.roles[1].action.action_id == 9U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.variant_delta == 7U &&
            fixture.roles[1].flags == 0xD100U && source.action_id == 100U &&
            source.base_variant == 101U && source.variant_delta == 102U &&
            source.talk_script_id == 9U && source.path_data_id == 0U &&
            source.path_word_index == 0 && source.flags == 0x0101U &&
            fixture.role_transfer_state.party_role_count == 1U &&
            fixture.live_party_role_count == 1U &&
            fixture.context.instruction_offset == 16U &&
            fixture.state.previous_opcode == OP_66_UPDATE_ROLE_MAP_STATE,
        "real opcode 66 record applies GUID 9 map state, removes its physical party slot and yields"
    );
}

void test_real_frame_clock_wait_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000044F7);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_59_PLAY_SOUND_EFFECT);
    write_u16(fixture.state.window, 6U, 1U);
    fixture.runtime.current_tick = 12345U;
    fixture.state.previous_opcode = 0x66U;

    const auto initialized = fixture.step();
    fixture.runtime.current_tick = 14345U;
    const auto equal = fixture.step();
    fixture.runtime.current_tick = 14346U;
    const auto completed = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_67_WAIT_FRAME_CLOCK &&
            read_u16(instruction, 2U) == 2000U &&
            initialized.status == LegacyWorldStoryVmStatus::yielded &&
            initialized.opcode == OP_67_WAIT_FRAME_CLOCK &&
            fixture.state.wait_duration == 2000U &&
            fixture.state.wait_started_at == 12345U &&
            equal.status == LegacyWorldStoryVmStatus::yielded &&
            equal.opcode == OP_67_WAIT_FRAME_CLOCK &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_59_PLAY_SOUND_EFFECT &&
            completed.executed_instruction_count == 2U &&
            read_u16(fixture.state.window, 2U) == 2000U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_59_PLAY_SOUND_EFFECT &&
            fixture.ports.sound_effect_requests == std::vector<u16>{1U},
        "real opcode 67 record waits through equality, completes at duration plus one and continues in the same call"
    );
}

void test_real_clear_role_flag_0400_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00009795);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 1U;
    fixture.roles[1].flags = 0xA5A5FFFFU;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_68_CLEAR_ROLE_FLAG_0400 &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_68_CLEAR_ROLE_FLAG_0400 &&
            result.executed_instruction_count == 1U &&
            fixture.roles[1].flags == 0xA5A5FBFFU &&
            fixture.ports.role_patch_requests.empty() &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_68_CLEAR_ROLE_FLAG_0400,
        "real opcode 68 record clears only GUID 1 role flag 0400 and yields"
    );
}

void test_real_set_role_flag_0400_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000CA01);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 1U;
    fixture.roles[1].flags = 0xA5A50001U;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_69_SET_ROLE_FLAG_0400 &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_69_SET_ROLE_FLAG_0400 &&
            result.executed_instruction_count == 1U &&
            fixture.roles[1].flags == 0xA5A50401U &&
            fixture.ports.role_patch_requests.empty() &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_69_SET_ROLE_FLAG_0400,
        "real opcode 69 record sets only GUID 1 role flag 0400 and yields"
    );
}

void test_real_set_role_head_sign_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004B9D);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 0x00BFU;
    fixture.roles[1].field_3c = 0x12345678U;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_71_SET_ROLE_HEAD_SIGN &&
            read_u16(instruction, 2U) == 0x00BFU &&
            read_u16(instruction, 4U) == 0U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_71_SET_ROLE_HEAD_SIGN &&
            result.executed_instruction_count == 1U &&
            fixture.roles[1].field_3c ==
                openswd3::world_map::legacy_world_head_sign_action_token(0U) &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_71_SET_ROLE_HEAD_SIGN,
        "real opcode 71 record assigns GUID 191 head-sign slot zero and yields"
    );
}

void test_real_clear_role_head_sign_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004BA7);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 0x00BFU;
    fixture.roles[1].field_3c = 0x12345678U;
    prime_loaded_instruction(fixture, read_u16(instruction, 0U));
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.previous_opcode = OP_71_SET_ROLE_HEAD_SIGN;

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_72_CLEAR_ROLE_HEAD_SIGN &&
            read_u16(instruction, 2U) == 0x00BFU &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_72_CLEAR_ROLE_HEAD_SIGN &&
            result.executed_instruction_count == 1U &&
            fixture.roles[1].field_3c == 0U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_72_CLEAR_ROLE_HEAD_SIGN,
        "real opcode 72 record clears GUID 191 head sign and yields"
    );
}

void test_real_cancel_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00005461);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState color{
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
    fixture.runtime.frame_color = &color;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFEU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFEU);

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_74_CANCEL_FRAME_COLOR_TRANSITION &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_74_CANCEL_FRAME_COLOR_TRANSITION &&
            result.executed_instruction_count == 1U && color.countdown == 0 &&
            color.step_red == 0.0F && color.step_green == 0.0F &&
            color.step_blue == 0.0F && color.current_red == 1.0F &&
            color.target_blue == 6.0F &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode ==
                OP_74_CANCEL_FRAME_COLOR_TRANSITION,
        "real opcode 74 record clears the color transition before exact-tail fetch failure"
    );
}

void test_real_suspend_story_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00007A38);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 0x00B5U;
    fixture.roles[1].world_x = 0x20U;
    fixture.roles[1].world_y = 0x30U;
    StoryPathHarness path_harness{fixture};
    fixture.runtime.story_paths = &path_harness.runtime;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFCU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFCU);

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_75_SUSPEND_STORY_ROLE &&
            read_u16(instruction, 2U) == 0x00B5U &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_75_SUSPEND_STORY_ROLE &&
            result.executed_instruction_count == 1U &&
            (fixture.roles[1].flags & 0x80000000U) != 0U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_75_SUSPEND_STORY_ROLE,
        "real opcode 75 record suspends GUID 181 before exact-tail fetch failure"
    );
}

void test_real_turn_and_suspend_story_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A68);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.roles[1].guid = 0x00BFU;
    fixture.roles[1].world_x = 100U;
    fixture.roles[1].world_y = 100U;
    fixture.roles[1].action.field_2c = 0U;
    fixture.roles[1].action.field_30 = 0U;
    fixture.roles[2].guid = 1U;
    fixture.roles[2].world_x = 200U;
    fixture.roles[2].world_y = 100U;
    fixture.roles[2].action.field_2c = 0U;
    fixture.roles[2].action.field_30 = 0U;
    StoryPathHarness path_harness{fixture};
    fixture.runtime.story_paths = &path_harness.runtime;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFAU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFAU);

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_76_TURN_AND_SUSPEND_STORY_ROLE &&
            read_u16(instruction, 2U) == 0x00BFU &&
            read_u16(instruction, 4U) == 1U &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_76_TURN_AND_SUSPEND_STORY_ROLE &&
            result.executed_instruction_count == 1U &&
            result.action_update_count == 1U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.variant_delta == 3U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            (fixture.roles[1].flags & 0x80000000U) != 0U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_76_TURN_AND_SUSPEND_STORY_ROLE,
        "real opcode 76 record turns GUID 191 toward GUID 1 and suspends it before exact-tail fetch failure"
    );
}

void test_real_role_wait_override_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](
                                 const std::streamoff offset,
                                 const std::size_t size
                             ) -> std::array<u8, 6U> {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(size)
        );
        return record;
    };
    const auto assigned_record = read_record(0x00004E24, 6U);
    const auto cleared_record = read_record(0x00042697, 4U);

    Fixture assigned;
    assigned.roles[1].guid = 0x00FAU;
    assigned.context.talk_data_offset = 0x1111U;
    assigned.context.instruction_offset = 0x7FFAU;
    assigned.state.loaded_file_number = 1U;
    assigned.state.loaded_data_offset = 0x1111U;
    assigned.state.window_loaded = true;
    assigned.state.previous_opcode = 0x66U;
    std::ranges::copy(assigned_record, assigned.state.window.begin() + 0x7FFAU);
    const auto assigned_result = assigned.step();

    Fixture cleared;
    cleared.roles[1].guid = 8U;
    cleared.roles[1].action.wait_override = 0x8123U;
    cleared.roles[1].action.wait_remaining = 9U;
    cleared.context.talk_data_offset = 0x1111U;
    cleared.context.instruction_offset = 0x7FFCU;
    cleared.state.loaded_file_number = 1U;
    cleared.state.loaded_data_offset = 0x1111U;
    cleared.state.window_loaded = true;
    cleared.state.previous_opcode = 0x66U;
    std::ranges::copy_n(
        cleared_record.begin(), 4U, cleared.state.window.begin() + 0x7FFCU
    );
    const auto cleared_result = cleared.step();

    test.expect_true(
        read_u16(assigned_record, 0U) == OP_77_SET_ROLE_WAIT_OVERRIDE &&
            read_u16(assigned_record, 2U) == 0x00FAU &&
            read_u16(assigned_record, 4U) == 3U &&
            assigned_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            assigned_result.action_update_count == 1U &&
            assigned.roles[1].action.wait_override == 0x8003U &&
            assigned.roles[1].action.wait_remaining == 0U &&
            assigned.state.previous_opcode == OP_77_SET_ROLE_WAIT_OVERRIDE &&
            read_u16(cleared_record, 0U) == OP_78_CLEAR_ROLE_WAIT_OVERRIDE &&
            read_u16(cleared_record, 2U) == 8U &&
            cleared_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            cleared_result.action_update_count == 1U &&
            cleared.roles[1].action.wait_override == 0U &&
            cleared.roles[1].action.wait_remaining == 0U &&
            cleared.state.previous_opcode == OP_78_CLEAR_ROLE_WAIT_OVERRIDE,
        "real opcode 77/78 records set and clear role wait overrides before exact-tail fetch failure"
    );
}

void test_real_begin_story_video_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_opening = [&root]() {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(0x000044FF);
        std::array<u8, 15U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto read_demo = [&root]() {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(0x00004634);
        std::array<u8, 12U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto opening = read_opening();
    const auto demo = read_demo();

    const auto execute = [](const std::span<const u8> record) {
        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset =
            static_cast<u16>(0x8000U - record.size());
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(
            record,
            fixture.state.window.begin() + fixture.context.instruction_offset
        );
        const auto result = fixture.step();
        return std::tuple{
            result,
            std::string{
                fixture.ports.last_video_filename.begin(),
                fixture.ports.last_video_filename.end(),
            },
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
            fixture.ports.framebuffer_clear_count,
            fixture.ports.framebuffer_present_count,
            fixture.ports.direct_audio_service_count,
            fixture.ports.video_prepare_count,
            fixture.ports.video_begin_count,
        };
    };
    const auto
        [opening_result,
         opening_filename,
         opening_ip,
         opening_previous,
         opening_clear,
         opening_present,
         opening_audio,
         opening_prepare,
         opening_begin] = execute(opening);
    const auto
        [demo_result,
         demo_filename,
         demo_ip,
         demo_previous,
         demo_clear,
         demo_present,
         demo_audio,
         demo_prepare,
         demo_begin] = execute(demo);

    test.expect_true(
        read_u16(opening, 0U) == OP_85_BEGIN_STORY_VIDEO &&
            opening[opening.size() - 2U] == 0x25U &&
            opening[opening.size() - 1U] == 0x51U &&
            opening_result.status == LegacyWorldStoryVmStatus::yielded &&
            opening_result.direct_audio_service_count == 2U &&
            opening_filename == "OPENING.bik" && opening_ip == 0x8000U &&
            opening_previous == OP_85_BEGIN_STORY_VIDEO &&
            opening_clear == 1U && opening_present == 1U &&
            opening_audio == 2U && opening_prepare == 1U &&
            opening_begin == 1U &&
            read_u16(demo, 0U) == OP_85_BEGIN_STORY_VIDEO &&
            demo_result.status == LegacyWorldStoryVmStatus::yielded &&
            demo_result.direct_audio_service_count == 2U &&
            demo_filename == "Demo.mpg" && demo_ip == 0x8000U &&
            demo_previous == OP_85_BEGIN_STORY_VIDEO && demo_clear == 1U &&
            demo_present == 1U && demo_audio == 2U && demo_prepare == 1U &&
            demo_begin == 1U,
        "real opcode 85 BIK and MPG records preserve raw filenames, side-effect order owners and exact-tail yield"
    );
}

void test_real_wait_for_story_video_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000450E);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture active;
    active.ports.video_progress = 0;
    prime_loaded_instruction(active, OP_193_WAIT_STORY_VIDEO);
    std::ranges::copy(instruction, active.state.window.begin());
    write_u16(active.state.window, 2U, OP_1025);
    const auto active_result = active.step();

    Fixture completed;
    completed.ports.video_progress = -1;
    prime_loaded_instruction(completed, OP_193_WAIT_STORY_VIDEO);
    std::ranges::copy(instruction, completed.state.window.begin());
    write_u16(completed.state.window, 2U, OP_1025);
    const auto completed_result = completed.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_193_WAIT_STORY_VIDEO &&
            active_result.status == LegacyWorldStoryVmStatus::yielded &&
            active_result.executed_instruction_count == 1U &&
            active_result.direct_audio_service_count == 1U &&
            active.context.instruction_offset == 0U &&
            active.state.previous_opcode == OP_193_WAIT_STORY_VIDEO &&
            active.ports.video_progress_query_count == 1U &&
            active.ports.direct_audio_service_count == 1U &&
            completed_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            completed_result.executed_instruction_count == 2U &&
            completed_result.direct_audio_service_count == 0U &&
            completed.context.instruction_offset == 2U &&
            completed.state.previous_opcode == OP_193_WAIT_STORY_VIDEO &&
            completed.ports.video_progress_query_count == 1U &&
            completed.ports.direct_audio_service_count == 0U,
        "real opcode 193 record audio-yields active video and same-calls after inactive completion"
    );
}

void test_real_rewrite_role_head_action_key_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](
                                 const std::filesystem::path& filename,
                                 const std::streamoff offset
                             ) {
        std::ifstream input{root / filename, std::ios::binary | std::ios::in};
        input.seekg(offset);
        std::array<u8, 10U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto first = read_record("TALK1.DAT", 0x0001CC18);
    const auto second = read_record("TALK4.DAT", 0x0002BA21);

    const auto execute = [](const std::array<u8, 10U>& record) {
        Fixture fixture;
        openswd3::world_map::LegacyRoleHeadActionList actions(3U);
        const u16 old_action_id = read_u16(record, 2U);
        const u16 old_variant = read_u16(record, 4U);
        auto variant_miss = actions.begin();
        variant_miss->action.action_id = old_action_id;
        variant_miss->action.base_variant = old_variant + 1U;
        auto exact = std::next(variant_miss);
        exact->action.action_id = old_action_id;
        exact->action.base_variant = old_variant;
        auto duplicate = std::next(exact);
        duplicate->action.action_id = old_action_id;
        duplicate->action.base_variant = old_variant;
        fixture.runtime.role_head_actions = &actions;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FF6U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FF6U);

        const auto result = fixture.step();

        return std::tuple{
            result,
            exact->action.action_id,
            exact->action.base_variant,
            duplicate->action.action_id,
            duplicate->action.base_variant,
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
        };
    };
    const auto
        [first_result,
         first_id,
         first_variant,
         first_duplicate_id,
         first_duplicate_variant,
         first_ip,
         first_previous] = execute(first);
    const auto
        [second_result,
         second_id,
         second_variant,
         second_duplicate_id,
         second_duplicate_variant,
         second_ip,
         second_previous] = execute(second);

    test.expect_true(
        read_u16(first, 0U) == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY &&
            read_u16(first, 2U) == 10002U && read_u16(first, 4U) == 18U &&
            read_u16(first, 6U) == 10002U && read_u16(first, 8U) == 24U &&
            first_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            first_id == 10002U && first_variant == 24U &&
            first_duplicate_id == 10002U && first_duplicate_variant == 18U &&
            first_ip == 0x8000U &&
            first_previous == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY &&
            read_u16(second, 0U) == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY &&
            read_u16(second, 2U) == 10001U && read_u16(second, 4U) == 22U &&
            read_u16(second, 6U) == 10001U && read_u16(second, 8U) == 54U &&
            second_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            second_id == 10001U && second_variant == 54U &&
            second_duplicate_id == 10001U && second_duplicate_variant == 22U &&
            second_ip == 0x8000U &&
            second_previous == OP_86_REWRITE_ROLE_HEAD_ACTION_KEY,
        "real opcode 86 records rewrite only the first exact role-head key and preserve exact-tail continuation"
    );
}

void test_real_reload_random_target_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_three_targets = [&root]() {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(0x00028B22);
        std::array<u8, 18U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto read_six_targets = [&root]() {
        std::ifstream input{
            root / "TALK4.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(0x0002FE09);
        std::array<u8, 30U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto three_targets = read_three_targets();
    const auto six_targets = read_six_targets();

    const auto execute = [](const std::span<const u8> record,
                            const u32 file_number) {
        Fixture fixture;
        fixture.secondary_rng.seed(0x12345678U);
        write_u16(fixture.ports.transferred_window, 0U, 88U);
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset =
            static_cast<u16>(0x8000U - record.size());
        fixture.state.loaded_file_number = file_number;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(
            record,
            fixture.state.window.begin() + fixture.context.instruction_offset
        );

        const auto result = fixture.step();

        return std::tuple{
            result,
            fixture.secondary_rng.index(),
            fixture.ports.last_data_file_number,
            fixture.ports.last_data_offset,
            fixture.ports.last_data_clear_before_read,
            fixture.context.talk_data_offset,
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
        };
    };
    const auto
        [three_result,
         three_rng_index,
         three_file,
         three_target,
         three_clear,
         three_context_target,
         three_ip,
         three_previous] = execute(three_targets, 1U);
    const auto
        [six_result,
         six_rng_index,
         six_file,
         six_target,
         six_clear,
         six_context_target,
         six_ip,
         six_previous] = execute(six_targets, 4U);

    test.expect_true(
        read_u16(three_targets, 0U) == OP_87_RELOAD_RANDOM_TARGET &&
            read_u32(three_targets, 2U) == 0x0002897DU &&
            read_u32(three_targets, 6U) == 0x00028995U &&
            read_u32(three_targets, 10U) == 0x000289DAU &&
            read_u32(three_targets, 14U) == 0xFF00FF00U &&
            three_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            three_result.executed_instruction_count == 2U &&
            three_result.direct_audio_service_count == 1U &&
            three_rng_index == 2U && three_file == 1U &&
            three_target == 0x00028995U && !three_clear &&
            three_context_target == 0x00028995U && three_ip == 0U &&
            three_previous == OP_87_RELOAD_RANDOM_TARGET &&
            read_u16(six_targets, 0U) == OP_87_RELOAD_RANDOM_TARGET &&
            read_u32(six_targets, 26U) == 0xFF00FF00U &&
            six_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            six_result.executed_instruction_count == 2U &&
            six_result.direct_audio_service_count == 1U &&
            six_rng_index == 2U && six_file == 4U &&
            six_target == 0x0002FEB3U && !six_clear &&
            six_context_target == 0x0002FEB3U && six_ip == 0U &&
            six_previous == OP_87_RELOAD_RANDOM_TARGET,
        "real opcode 87 three- and six-target tables preserve RNG selection, exact-tail reload and same-call continuation"
    );
}

void test_real_request_battle_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](
                                 const std::filesystem::path& filename,
                                 const std::streamoff offset
                             ) {
        std::ifstream input{root / filename, std::ios::binary | std::ios::in};
        input.seekg(offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto first = read_record("TALK1.DAT", 0x00005547);
    const auto second = read_record("TALK4.DAT", 0x0001F694);

    const auto execute = [](const std::array<u8, 4U>& record) {
        Fixture fixture;
        std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows(1U);
        openswd3::world_map::LegacyRoleHeadActionList role_heads(1U);
        openswd3::world_map::LegacyMovingActionList moving_actions(1U);
        u32 battle_request = 0x11111111U;
        fixture.runtime.packed_row_effects = &packed_rows;
        fixture.runtime.role_head_actions = &role_heads;
        fixture.runtime.moving_actions = &moving_actions;
        fixture.runtime.battle_request_value = &battle_request;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto result = fixture.step();

        return std::tuple{
            result,
            battle_request,
            packed_rows.size(),
            role_heads.size(),
            moving_actions.size(),
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
        };
    };
    const auto
        [first_result,
         first_request,
         first_rows,
         first_heads,
         first_moving,
         first_ip,
         first_previous] = execute(first);
    const auto
        [second_result,
         second_request,
         second_rows,
         second_heads,
         second_moving,
         second_ip,
         second_previous] = execute(second);

    test.expect_true(
        read_u16(first, 0U) == OP_88_REQUEST_BATTLE &&
            read_u16(first, 2U) == 98U &&
            first_result.status == LegacyWorldStoryVmStatus::yielded &&
            first_request == 0x80000062U && first_rows == 0U &&
            first_heads == 0U && first_moving == 1U && first_ip == 0x8000U &&
            first_previous == OP_88_REQUEST_BATTLE &&
            read_u16(second, 0U) == OP_88_REQUEST_BATTLE &&
            read_u16(second, 2U) == 290U &&
            second_result.status == LegacyWorldStoryVmStatus::yielded &&
            second_request == 0x80000122U && second_rows == 0U &&
            second_heads == 0U && second_moving == 1U && second_ip == 0x8000U &&
            second_previous == OP_88_REQUEST_BATTLE,
        "real opcode 88 records clear exactly two overlay lists, publish signed battle requests and yield at the exact tail"
    );
}

void test_real_set_scene_render_bit1_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000049F4);
    std::array<u8, 2U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFEU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    u8 scene_render_flags = 0xA5U;
    fixture.runtime.scene_render_flags = &scene_render_flags;
    std::ranges::copy(record, fixture.state.window.begin() + 0x7FFEU);

    const auto result = fixture.step();

    test.expect_true(
        record_read && read_u16(record, 0U) == OP_94_SET_SCENE_RENDER_BIT1 &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 1U &&
            scene_render_flags == 0xA7U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_94_SET_SCENE_RENDER_BIT1,
        "real opcode 94 sets scene bit 1 and publishes previous at the exact tail"
    );
}

void test_real_clear_scene_render_bit1_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A22);
    std::array<u8, 2U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFEU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    u8 scene_render_flags = 0xA7U;
    fixture.runtime.scene_render_flags = &scene_render_flags;
    std::ranges::copy(record, fixture.state.window.begin() + 0x7FFEU);

    const auto result = fixture.step();

    test.expect_true(
        record_read && read_u16(record, 0U) == OP_95_CLEAR_SCENE_RENDER_BIT1 &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 1U &&
            scene_render_flags == 0xA5U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_95_CLEAR_SCENE_RENDER_BIT1,
        "real opcode 95 clears scene bit 1 and publishes previous at the exact tail"
    );
}

void test_real_begin_custom_ani_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        std::string_view expected_name;
        u8 expected_flags;
    };
    constexpr std::array<RealCase, 2U> cases{
        RealCase{
            "TALK1.DAT",
            0x000043FA,
            "expv.ani",
            openswd3::asset_runtime::kLegacyAniSkipRevealFlag |
                openswd3::asset_runtime::kLegacyAniEndingEffectFlag,
        },
        RealCase{
            "TALK2.DAT",
            0x0000D39F,
            "memory.ani",
            openswd3::asset_runtime::kLegacyAniEndingEffectFlag,
        },
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 32U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);
        const auto terminator =
            std::ranges::search(record, std::array<u8, 2U>{u8{'%'}, u8{'Q'}});
        const std::size_t record_size =
            static_cast<std::size_t>(terminator.begin() - record.begin()) + 2U;

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = static_cast<u16>(
            openswd3::resource_io::kLegacyTalkWindowSize - record_size
        );
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        u8 scene_flags = 0xA5U;
        fixture.runtime.scene_render_flags = &scene_flags;
        std::ranges::copy(
            std::span<const u8>{record}.first(record_size),
            fixture.state.window.begin() + fixture.context.instruction_offset
        );

        const auto result = fixture.step();
        const std::vector<u8> expected_name{
            real_case.expected_name.begin(), real_case.expected_name.end()
        };
        test.expect_true(
            record_read && read_u16(record, 0U) == OP_96_BEGIN_CUSTOM_ANI &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 3U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_96_BEGIN_CUSTOM_ANI &&
                fixture.ports.last_ani_frame_interval == 70U &&
                fixture.ports.last_ani_filename == expected_name &&
                fixture.ports.last_ani_flags == real_case.expected_flags,
            "real opcode 96 parses ANI prefixes and starts the requested archive at the exact tail"
        );
    }
}

void test_real_wait_custom_ani_complete_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004408);
    std::array<u8, 2U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFEU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.ports.ani_active = true;
    std::ranges::copy(record, fixture.state.window.begin() + 0x7FFE);

    const auto active_result = fixture.step();
    fixture.ports.ani_active = false;
    const auto completed_result = fixture.step();

    test.expect_true(
        record_read && read_u16(record, 0U) == OP_97_WAIT_CUSTOM_ANI_COMPLETE &&
            active_result.status == LegacyWorldStoryVmStatus::yielded &&
            active_result.executed_instruction_count == 1U &&
            completed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_result.executed_instruction_count == 1U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_97_WAIT_CUSTOM_ANI_COMPLETE &&
            fixture.ports.last_ani_frame_interval == 35U,
        "real opcode 97 waits while active then completes at the exact tail"
    );
}

void test_real_consume_four_byte_noop_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 payload;
    };
    constexpr std::array<RealCase, 3U> cases{
        RealCase{"TALK2.DAT", 0x0001708D, 0x0190U},
        RealCase{"TALK3.DAT", 0x0000B039, 0x006CU},
        RealCase{"TALK3.DAT", 0x0000CFD2, 0x0001U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_98_CONSUME_FOUR_BYTE_NOOP &&
                read_u16(record, 2U) == real_case.payload &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_98_CONSUME_FOUR_BYTE_NOOP,
            "real opcode 98 consumes its four-byte record and yields at the exact tail"
        );
    }
}

void test_real_wait_custom_ani_phase_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 threshold;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK2.DAT", 0x0000D3AE, 1U},
        RealCase{"TALK2.DAT", 0x00017089, 350U},
        RealCase{"TALK3.DAT", 0x0000898A, 30U},
        RealCase{"TALK4.DAT", 0x0001484C, 11U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.ports.ani_phase = static_cast<i32>(real_case.threshold);
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto waiting_result = fixture.step();
        fixture.ports.ani_phase = static_cast<i32>(real_case.threshold) + 1;
        const auto completed_result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_99_WAIT_CUSTOM_ANI_PHASE &&
                read_u16(record, 2U) == real_case.threshold &&
                waiting_result.status == LegacyWorldStoryVmStatus::yielded &&
                waiting_result.executed_instruction_count == 1U &&
                completed_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                completed_result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_99_WAIT_CUSTOM_ANI_PHASE &&
                fixture.ports.ani_phase_query_count == 2U,
            "real opcode 99 waits at equality then completes above the phase threshold at the exact tail"
        );
    }
}

void test_real_set_role_talk_script_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 selector;
        u16 talk_script_id;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x00043399, 0x0019U, 0U},
        RealCase{"TALK2.DAT", 0x00013B3F, 0x0068U, 0U},
        RealCase{"TALK3.DAT", 0x00002E44, 0x000EU, 0U},
        RealCase{"TALK4.DAT", 0x0000488B, 0x0252U, 6421U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFAU);

        const auto result = fixture.step();
        const auto patch = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_100_SET_ROLE_TALK_SCRIPT &&
                read_u16(record, 2U) == real_case.selector &&
                read_u16(record, 4U) == real_case.talk_script_id &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_100_SET_ROLE_TALK_SCRIPT &&
                fixture.ports.role_patch_requests.size() == 1U &&
                patch.guid == real_case.selector &&
                patch.talk_script_id == real_case.talk_script_id &&
                patch.flags_or_mask == 0U && patch.flags_and_mask == 0xFFFFU,
            "real opcode 100 submits the Talk-only MAPS patch before exact-tail completion"
        );
    }
}

void test_real_set_role_status_bit26_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 selector;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x0001BF4B, 0x0001U},
        RealCase{"TALK2.DAT", 0x00007474, 0x00BDU},
        RealCase{"TALK3.DAT", 0x00002662, 0x0001U},
        RealCase{"TALK4.DAT", 0x000059CC, 0x0001U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.roles[1].guid = real_case.selector;
        fixture.roles[1].flags = 0xA0A50020U;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_101_SET_ROLE_STATUS_BIT26 &&
                read_u16(record, 2U) == real_case.selector &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.roles[1].flags == 0xA4A50020U &&
                fixture.state.previous_opcode == OP_101_SET_ROLE_STATUS_BIT26 &&
                fixture.ports.role_patch_requests.empty(),
            "real opcode 101 sets live role bit 26 before exact-tail completion"
        );
    }
}

void test_real_set_role_status_from_boolean_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 opcode;
        u16 selector;
        u16 value;
        u32 mask;
    };
    constexpr std::array<RealCase, 6U> cases{
        RealCase{
            "TALK1.DAT",
            0x0001BA75,
            OP_102_SET_ROLE_STATUS_BIT6,
            0x00E6U,
            0U,
            0x00000040U,
        },
        RealCase{
            "TALK2.DAT",
            0x00001723,
            OP_103_SET_ROLE_STATUS_BIT5,
            0xFFF0U,
            1U,
            0x00000020U,
        },
        RealCase{
            "TALK3.DAT",
            0x00002E4A,
            OP_117_SET_ROLE_STATUS_BIT4,
            0x023FU,
            0U,
            0x00000010U,
        },
        RealCase{
            "TALK4.DAT",
            0x00022104,
            OP_136_SET_ROLE_STATUS_BIT12,
            0x0027U,
            1U,
            0x00001000U,
        },
        RealCase{
            "TALK1.DAT",
            0x000317E4,
            OP_140_SET_ROLE_STATUS_BIT11,
            0x0009U,
            1U,
            0x00000800U,
        },
        RealCase{
            "TALK2.DAT",
            0x00011366,
            OP_146_SET_ROLE_STATUS_BIT8,
            0x00E6U,
            1U,
            0x00000100U,
        },
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        const u32 role_index = real_case.selector == 0xFFF0U ? 0U : 1U;
        if (real_case.selector != 0xFFF0U) {
            fixture.roles[role_index].guid = real_case.selector;
        }
        const bool enabled = real_case.value != 0U;
        const u32 initial_flags = 0x80000001U | (enabled ? 0U : real_case.mask);
        const u32 final_flags =
            (initial_flags & ~real_case.mask) | (enabled ? real_case.mask : 0U);
        fixture.roles[role_index].flags = initial_flags;
        fixture.roles[role_index].map_cell_pointer_32 = 0U;
        fixture.roles[role_index].action.field_2c = 1U;
        fixture.roles[role_index].action.field_30 = 1U;
        std::array<u8, 16U> surface{};
        surface.fill(0xFFU);
        fixture.runtime.role_surface = {
            .map_width = 2U,
            .selected_guid = 0xFFFFU,
            .surface_grid = surface,
        };
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFAU);

        const auto result = fixture.step();
        test.expect_true(
            record_read && read_u16(record, 0U) == real_case.opcode &&
                read_u16(record, 2U) == real_case.selector &&
                read_u16(record, 4U) == real_case.value &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.roles[role_index].flags == final_flags &&
                fixture.state.previous_opcode == real_case.opcode &&
                fixture.ports.role_patch_requests.empty(),
            "real shared role-status record updates the resolved live role before exact-tail completion"
        );
    }
}

void test_real_set_text_layout_pair_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        i16 first;
        i16 second;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x000054AD, 0, -96},
        RealCase{"TALK2.DAT", 0x0000CB89, 0, -30},
        RealCase{"TALK3.DAT", 0x00009DDF, 20, -75},
        RealCase{"TALK4.DAT", 0x00002273, 20, -75},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFAU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_104_SET_TEXT_LAYOUT_PAIR &&
                static_cast<i16>(read_u16(record, 2U)) == real_case.first &&
                static_cast<i16>(read_u16(record, 4U)) == real_case.second &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.text_control_flags == 0xEFFFFFFFU &&
                fixture.state.text_layout_first == real_case.first &&
                fixture.state.text_layout_second == real_case.second &&
                fixture.state.previous_opcode == OP_104_SET_TEXT_LAYOUT_PAIR,
            "real opcode 104 stores the signed layout pair before exact-tail completion"
        );
    }
}

void test_real_clear_text_control_bit27_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x0000256C},
        RealCase{"TALK2.DAT", 0x00001703},
        RealCase{"TALK3.DAT", 0x00009DDD},
        RealCase{"TALK4.DAT", 0x0000169B},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFEU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFEU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_105_CLEAR_TEXT_CONTROL_BIT27 &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.text_control_flags == 0xF7FFFFFFU &&
                fixture.state.previous_opcode ==
                    OP_105_CLEAR_TEXT_CONTROL_BIT27,
            "real opcode 105 clears bit 27 before exact-tail completion"
        );
    }
}

void test_real_clear_text_control_bit26_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x0000965C},
        RealCase{"TALK2.DAT", 0x0000F18D},
        RealCase{"TALK3.DAT", 0x00023123},
        RealCase{"TALK4.DAT", 0x0000135C},
    };

    for (const auto& real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.state.text_control_flags = 0xFFFFFFFFU;
        prime_loaded_instruction(fixture, OP_121_CLEAR_TEXT_CONTROL_BIT26);
        fixture.context.instruction_offset = 0x7FFEU;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFEU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_121_CLEAR_TEXT_CONTROL_BIT26 &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.text_control_flags == 0xFBFFFFFFU &&
                fixture.state.previous_opcode ==
                    OP_121_CLEAR_TEXT_CONTROL_BIT26,
            "real opcode 121 clears bit 26 before exact-tail completion"
        );
    }
}

void test_real_clear_speed_mode_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x00041D98},
        RealCase{"TALK2.DAT", 0x000190F9},
        RealCase{"TALK3.DAT", 0x000100A6},
        RealCase{"TALK4.DAT", 0x00022045},
    };

    for (const auto& real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.speed_mode = 1U;
        prime_loaded_instruction(fixture, OP_122_CLEAR_SPEED_MODE);
        fixture.context.instruction_offset = 0x7FFEU;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFEU);

        const auto result = fixture.step();
        test.expect_true(
            record_read && read_u16(record, 0U) == OP_122_CLEAR_SPEED_MODE &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.speed_mode == 0U &&
                fixture.state.previous_opcode == OP_122_CLEAR_SPEED_MODE,
            "real opcode 122 clears speed mode before exact-tail completion"
        );
    }
}

void test_real_update_scene_music_table_entry_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready,
        "real opcode 123 test loads the writable MAPS payload"
    );
    if (initialized.status !=
            openswd3::resource_io::LegacyResourceDatabaseStatus::ready ||
        maps.status != openswd3::resource_io::LegacyMapsPayloadStatus::ready) {
        return;
    }
    const std::vector<u8> baseline{
        databases.maps_payload_bytes().begin(),
        databases.maps_payload_bytes().end(),
    };

    struct RealCase {
        const char* file;
        std::streamoff offset;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x00022C77},
        RealCase{"TALK2.DAT", 0x0001836F},
        RealCase{"TALK3.DAT", 0x0000CD7A},
        RealCase{"TALK4.DAT", 0x000289D1},
    };

    for (const auto& real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 10U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        std::vector<u8> payload = baseline;
        const u32 first_offset = read_u32(payload, 0x08U);
        const u32 second_offset = read_u32(payload, first_offset + 4U);
        std::size_t entry = read_u32(payload, second_offset);
        const u16 key = read_u16(record, 2U);
        while (read_u16(payload, entry) != 0U &&
               read_u16(payload, entry) != key) {
            entry += 8U;
        }
        const u16 preserved_tail = read_u16(payload, entry + 6U);

        Fixture fixture;
        fixture.runtime.mutable_maps_payload = payload;
        prime_loaded_instruction(
            fixture, OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY
        );
        fixture.context.instruction_offset = 0x7FF8U;
        std::ranges::copy_n(
            record.begin(), 8U, fixture.state.window.begin() + 0x7FF8U
        );

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY &&
                read_u16(payload, entry) == key &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 0x8002U &&
                read_u32(payload, entry) == read_u32(record, 2U) &&
                read_u16(payload, entry + 4U) == read_u16(record, 6U) &&
                read_u16(payload, entry + 6U) == preserved_tail &&
                fixture.state.previous_opcode ==
                    OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY,
            "real opcode 123 updates one MAPS music entry without reading +8"
        );
    }
}

void test_real_role_base_variant_reload_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00007B8C);
    std::array<u8, 10U> instruction{};
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
    state.loaded_data_offset = 0x0000798CU;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0xEA46U;
    context.talk_script_id = 100U;
    context.talk_data_offset = 0x0000798CU;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0xEA46U;
    roles[1].action.base_variant = 8U;
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
            read_u16(instruction, 0U) ==
                OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL &&
            read_u16(instruction, 2U) == 0xEA46U &&
            read_u16(instruction, 4U) == 8U &&
            read_u32(instruction, 6U) == 0x000079D7U &&
            result.status == LegacyWorldStoryVmStatus::terminated &&
            result.opcode == 16383U &&
            result.executed_instruction_count == 3U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 1U &&
            context.talk_data_offset == 0xFFFFFFFFU &&
            context.instruction_offset == 0xFFFFU && !state.window_loaded &&
            state.loaded_file_number == 0U && state.loaded_data_offset == 0U &&
            state.previous_opcode == OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL,
        "real opcode 126 reloads TALK1 target 0x79D7 and same-call terminates through 1026 and FFFF"
    );
}

void test_real_adjust_player_item_quantity_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000764F);
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
    state.loaded_data_offset = 0x0000744FU;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00F8U;
    context.talk_script_id = 100U;
    context.talk_data_offset = 0x0000744FU;
    std::array<LegacyWorldRoleRecord, 1U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    std::list<LegacyWorldItemNode> player_inventory;
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{};
    runtime.player_inventory = &player_inventory;
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
            read_u16(instruction, 0U) == OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
            read_u16(instruction, 2U) == 971U &&
            read_u16(instruction, 4U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            context.instruction_offset == 6U &&
            state.previous_opcode == OP_128_ADJUST_PLAYER_ITEM_QUANTITY &&
            player_inventory.size() == 1U &&
            player_inventory.front().item_id == 971U &&
            player_inventory.front().quantity_a == 0U &&
            player_inventory.front().quantity_b == 1U &&
            player_inventory.front().definition_snapshot[0U] == 0xCBU &&
            player_inventory.front().definition_snapshot[1U] == 0x03U &&
            player_inventory.front().definition_snapshot[0x21U] == 0x80U &&
            player_inventory.front().description ==
                std::vector<u8>{static_cast<u8>('I'), 0U},
        "real opcode 128 adds one unit of item 971 from TALK1 offset 0x764F"
    );
}

void test_real_request_shop_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00007AEE);
    std::array<u8, 16U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    std::ranges::copy(record, fixture.state.window.begin());
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x000078EEU;
    fixture.state.window_loaded = true;
    fixture.context.talk_data_offset = 0x000078EEU;

    const auto result = fixture.step();
    test.expect_true(
        record_read && read_u16(record, 0U) == OP_133_REQUEST_SHOP &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_133_REQUEST_SHOP &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 16U &&
            fixture.state.previous_opcode == OP_133_REQUEST_SHOP &&
            fixture.state.shop_item_ids ==
                std::vector<u16>{501U, 502U, 503U, 521U, 851U, 855U} &&
            fixture.special_mode_state == 0x80000002U,
        "real opcode 133 requests shop mode 2 with six item ids from TALK1 offset 0x7AEE"
    );
}

void test_real_adjust_party_member_resources_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream restore_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    restore_input.seekg(0x000233AF);
    std::array<u8, 42U> restore_records{};
    restore_input.read(
        reinterpret_cast<char*>(restore_records.data()),
        static_cast<std::streamsize>(restore_records.size())
    );
    const bool restore_records_read = static_cast<bool>(restore_input);

    Fixture restore;
    std::ranges::copy(restore_records, restore.state.window.begin());
    write_u16(restore.state.window, 40U, OP_1025);
    restore.state.loaded_file_number = 1U;
    restore.state.loaded_data_offset = 0x000231AFU;
    restore.state.window_loaded = true;
    restore.context.talk_data_offset = 0x000231AFU;
    for (std::size_t index = 0U;
         index < restore.state.party_member_resources.size();
         ++index) {
        auto& resources = restore.state.party_member_resources[index];
        resources.current_first = 1U;
        resources.current_second = 2U;
        resources.current_third = 3U;
        resources.limit_first = static_cast<u16>(101U + index);
        resources.limit_second = static_cast<u16>(201U + index);
        resources.limit_third = static_cast<u16>(301U + index);
        resources.transient_value = 99U;
    }

    const auto restore_result = restore.step();
    bool restored_all = true;
    for (std::size_t index = 0U;
         index < restore.state.party_member_resources.size();
         ++index) {
        const auto& resources = restore.state.party_member_resources[index];
        restored_all = restored_all &&
            resources.current_first == 101U + index &&
            resources.current_second == 201U + index &&
            resources.current_third == 301U + index &&
            resources.transient_value == 0U;
    }

    std::ifstream damage_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    damage_input.seekg(0x000299AF);
    std::array<u8, 14U> damage_record{};
    damage_input.read(
        reinterpret_cast<char*>(damage_record.data()),
        static_cast<std::streamsize>(damage_record.size())
    );
    const bool damage_record_read = static_cast<bool>(damage_input);

    Fixture damage;
    std::ranges::copy(damage_record, damage.state.window.begin());
    damage.state.loaded_file_number = 1U;
    damage.state.loaded_data_offset = 0x000297AFU;
    damage.state.window_loaded = true;
    damage.context.talk_data_offset = 0x000297AFU;
    auto& damage_resources = damage.state.party_member_resources[0U];
    damage_resources.current_first = 50U;
    damage_resources.current_second = 10U;
    damage_resources.current_third = 10U;
    damage_resources.limit_first = 100U;
    damage_resources.limit_second = 20U;
    damage_resources.limit_third = 20U;
    damage_resources.transient_value = 99U;

    const auto damage_result = damage.step();
    test.expect_true(
        restore_records_read &&
            read_u16(restore_records, 0U) ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            read_u16(restore_records, 10U) ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            read_u16(restore_records, 20U) ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            read_u16(restore_records, 30U) ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            read_u16(restore_records, 40U) == OP_171_SET_MODE17_TEXT &&
            restore_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            restore_result.opcode == OP_1025 &&
            restore_result.executed_instruction_count == 5U &&
            restore.context.instruction_offset == 40U &&
            restore.state.previous_opcode ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            restored_all && damage_record_read &&
            read_u16(damage_record, 0U) ==
                OP_134_ADJUST_PARTY_MEMBER_RESOURCES &&
            read_u16(damage_record, 2U) == 1U &&
            read_u16(damage_record, 4U) == 0xFF9CU &&
            read_u16(damage_record, 10U) == 0xFFFFU &&
            damage_result.status == LegacyWorldStoryVmStatus::yielded &&
            damage_result.opcode == OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            damage_result.executed_instruction_count == 2U &&
            damage_result.direct_audio_service_count == 1U &&
            damage.context.instruction_offset == 14U &&
            damage.state.previous_opcode ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            damage_resources.current_first == 0U &&
            damage_resources.current_second == 10U &&
            damage_resources.current_third == 10U &&
            damage_resources.transient_value == 0U &&
            damage.special_mode_state == 0x80000004U &&
            damage.ports.input_menu_reset_count == 1U &&
            read_u16(damage.state.window, 10U) ==
                OP_144_REQUEST_SPECIAL_MODE_4_OR_5,
        "real opcode 134 restores all four party-member resources before the independently tested opcode 171 successor and same-calls its rewritten opcode 144 mode-four audio-yield path"
    );
}

void test_real_item_presence_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 opcode;
        u16 item_id;
        u32 target;
    };

    constexpr std::array<RealCase, 4U> cases{
        RealCase{
            "TALK1.DAT",
            0x0004D692,
            OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM,
            799U,
            0x0004D868U,
        },
        RealCase{
            "TALK1.DAT",
            0x00029C8F,
            OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM,
            402U,
            0x00029AC9U,
        },
        RealCase{
            "TALK1.DAT",
            0x000238AB,
            OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM,
            1004U,
            0x00023785U,
        },
        RealCase{
            "TALK3.DAT",
            0x0002452E,
            OP_168_RELOAD_IF_NO_ROLE_ITEM_ROOT_HAS_ITEM,
            578U,
            0x0002435EU,
        },
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 8U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        if (real_case.opcode == OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM) {
            fixture.player_inventory.emplace_back();
            fixture.player_inventory.front().item_id = real_case.item_id;
        } else if (
            real_case.opcode == OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM
        ) {
            fixture.item_lists.role_item_lists[0]->sentinel.item_id =
                real_case.item_id;
        }

        std::ranges::copy(record, fixture.state.window.begin());
        fixture.state.loaded_file_number =
            real_case.file == std::string_view{"TALK3.DAT"} ? 3U : 1U;
        fixture.state.loaded_data_offset =
            static_cast<u32>(real_case.offset) - 0x200U;
        fixture.state.window_loaded = true;
        fixture.context.talk_data_offset =
            static_cast<u32>(real_case.offset) - 0x200U;
        write_u16(fixture.ports.transferred_window, 0U, 1026U);
        write_u16(fixture.ports.transferred_window, 2U, 16383U);

        const auto result = fixture.step();
        test.expect_true(
            record_read && read_u16(record, 0U) == real_case.opcode &&
                read_u16(record, 2U) == real_case.item_id &&
                read_u32(record, 4U) == real_case.target &&
                result.status == LegacyWorldStoryVmStatus::terminated &&
                result.executed_instruction_count == 3U &&
                result.direct_audio_service_count == 1U &&
                fixture.ports.last_data_offset == real_case.target &&
                fixture.state.previous_opcode == real_case.opcode,
            "real shared item-presence reload record takes its opcode-specific predicate and same-calls the target"
        );
    }
}

void test_real_wait_primary_picture_action_byte_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 threshold;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x0001C0C9, 9U},
        RealCase{"TALK2.DAT", 0x000101E7, 3U},
        RealCase{"TALK3.DAT", 0x00016EF6, 25U},
        RealCase{"TALK4.DAT", 0x0000258A, 110U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        openswd3::world_map::LegacyPictureActionLists picture_actions;
        picture_actions.primary.emplace_back();
        picture_actions.primary.front().action.packed_ap_state =
            static_cast<u16>((real_case.threshold + 1U) << 8U);
        fixture.runtime.picture_actions = &picture_actions;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) ==
                    OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE &&
                read_u16(record, 2U) == real_case.threshold &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode ==
                    OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE,
            "real opcode 106 completes above its threshold before exact-tail fetch failure"
        );
    }
}

void test_real_wait_role_action_index_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        u16 selector;
        u16 threshold;
    };
    constexpr std::array<RealCase, 4U> cases{
        RealCase{"TALK1.DAT", 0x00005445, 0x0001U, 5U},
        RealCase{"TALK2.DAT", 0x000074CC, 0x0001U, 5U},
        RealCase{"TALK3.DAT", 0x00002B20, 0x000BU, 9U},
        RealCase{"TALK4.DAT", 0x00001987, 0x0011U, 8U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.roles[1].guid = real_case.selector;
        fixture.roles[1].action.packed_ap_state =
            static_cast<u16>((real_case.threshold << 8U) | real_case.threshold);
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFAU);

        const auto result = fixture.step();
        test.expect_true(
            record_read &&
                read_u16(record, 0U) == OP_107_WAIT_ROLE_ACTION_INDEX &&
                read_u16(record, 2U) == real_case.selector &&
                read_u16(record, 4U) == real_case.threshold &&
                result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                result.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_107_WAIT_ROLE_ACTION_INDEX,
            "real opcode 107 reaches its action index before exact-tail completion"
        );
    }
}

void test_real_step_role_list_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        const char* file;
        std::streamoff offset;
        std::size_t length;
        u16 count;
    };
    constexpr std::array<RealCase, 2U> cases{
        RealCase{"TALK1.DAT", 0x0001BF5B, 6U, 1U},
        RealCase{"TALK2.DAT", 0x0000F92D, 40U, 18U},
    };

    for (const auto real_case : cases) {
        std::ifstream input{
            root / real_case.file, std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::vector<u8> record(real_case.length);
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset =
            static_cast<u16>(fixture.state.window.size() - real_case.length);
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        std::ranges::copy(
            record,
            fixture.state.window.begin() + fixture.context.instruction_offset
        );

        const auto result = fixture.step();
        test.expect_true(
            record_read && read_u16(record, 0U) == OP_109_STEP_ROLES &&
                read_u16(record, 2U) == real_case.count &&
                real_case.length == 4U + 2U * real_case.count &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0x8000U &&
                fixture.state.previous_opcode == OP_109_STEP_ROLES,
            "real opcode 109 consumes counted role lists and yields at the exact tail"
        );
    }
}

void test_real_secondary_role_bit30_reload_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000F981);
    std::array<u8, 10U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);
    input.seekg(0x0000F92D);
    std::array<u8, 4U> target_record{};
    input.read(
        reinterpret_cast<char*>(target_record.data()),
        static_cast<std::streamsize>(target_record.size())
    );
    const bool target_read = static_cast<bool>(input);
    input.close();

    Fixture sequential;
    prime_loaded_instruction(
        sequential, OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30
    );
    std::ranges::copy(record, sequential.state.window.begin());
    const auto sequential_result = sequential.step();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(record, state.window.begin());
    state.loaded_file_number = 2U;
    state.loaded_data_offset = 0x0000F781U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x1234U;
    context.talk_script_id = 2200U;
    context.talk_data_offset = 0x0000F781U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x1234U;
    roles[1].flags = 0x40000000U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto reload_result = openswd3::world_map::step_legacy_world_story_vm(
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
        record_read && target_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(record, 0U) == OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30 &&
            read_u32(record, 2U) == 0x0000F72DU &&
            read_u16(record, 6U) == OP_67_WAIT_FRAME_CLOCK &&
            read_u16(record, 8U) == 0x012CU &&
            read_u16(target_record, 0U) == OP_109_STEP_ROLES &&
            read_u16(target_record, 2U) == 18U &&
            sequential_result.status == LegacyWorldStoryVmStatus::yielded &&
            sequential_result.opcode == OP_67_WAIT_FRAME_CLOCK &&
            sequential_result.executed_instruction_count == 2U &&
            sequential_result.direct_audio_service_count == 1U &&
            sequential.context.instruction_offset == 6U &&
            sequential.state.previous_opcode == OP_67_WAIT_FRAME_CLOCK &&
            reload_result.status == LegacyWorldStoryVmStatus::yielded &&
            reload_result.opcode == OP_109_STEP_ROLES &&
            reload_result.executed_instruction_count == 2U &&
            reload_result.direct_audio_service_count == 2U &&
            context.talk_data_offset == 0x0000F72DU &&
            context.instruction_offset == 40U &&
            state.loaded_file_number == 2U &&
            state.loaded_data_offset == 0x0000F72DU &&
            state.previous_opcode == OP_109_STEP_ROLES,
        "real opcode 111 either continues to 67 or reloads the count-18 opcode 109"
    );
}

void test_real_wait_overlay_action_lists_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000307D8);
    std::array<u8, 4U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(
        fixture, OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS
    );
    std::ranges::copy(record, fixture.state.window.begin());
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows;
    openswd3::world_map::LegacyRoleHeadActionList role_heads(1U);
    openswd3::world_map::LegacyMovingActionList moving_actions(1U);
    fixture.runtime.packed_row_effects = &packed_rows;
    fixture.runtime.role_head_actions = &role_heads;
    fixture.runtime.moving_actions = &moving_actions;
    fixture.state.previous_opcode = 0x66U;

    const auto waiting_result = fixture.step();
    const u16 waiting_ip = fixture.context.instruction_offset;
    const u32 waiting_previous = fixture.state.previous_opcode;
    role_heads.clear();
    const auto completed_result = fixture.step();

    test.expect_true(
        record_read &&
            read_u16(record, 0U) ==
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            read_u16(record, 2U) == OP_26_CLEAR_GLOBAL_BIT &&
            waiting_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_result.executed_instruction_count == 1U &&
            waiting_result.direct_audio_service_count == 1U &&
            waiting_ip == 0U &&
            waiting_previous == OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            completed_result.status == LegacyWorldStoryVmStatus::yielded &&
            completed_result.executed_instruction_count == 1U &&
            completed_result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode ==
                OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS &&
            fixture.ports.direct_audio_service_count == 2U &&
            moving_actions.size() == 1U,
        "real opcode 112 waits for both overlay lists and ignores moving actions"
    );
}

void test_real_stage_scene_music_stream_request_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000044EF);
    std::array<u8, 8U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST);
    std::ranges::copy(record, fixture.state.window.begin());
    write_u16(fixture.state.window, 8U, OP_1025);
    fixture.state.current_first_stream = 2U;
    fixture.state.current_stream_fade_divisor = 9U;
    fixture.state.current_second_stream = 7U;
    fixture.state.music_control_flags = 0x01020304U;
    fixture.state.previous_opcode = 0x66U;

    const auto result = fixture.step();
    test.expect_true(
        record_read &&
            read_u16(record, 0U) == OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST &&
            read_u16(record, 2U) == 25U && read_u16(record, 4U) == 25U &&
            read_u16(record, 6U) == 0x2000U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 0U &&
            fixture.state.music_request == 0x80000001U &&
            fixture.state.music_first_stream == 25U &&
            fixture.state.music_second_stream == 25U &&
            fixture.state.current_first_stream == 2U &&
            fixture.state.current_stream_fade_divisor == 7U &&
            fixture.state.current_second_stream == 7U &&
            fixture.state.music_control_flags == 0x01820300U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode ==
                OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST &&
            fixture.ports.music_transition_apply_count == 1U,
        "real opcode 114 stages both stream ids and continues after transition"
    );
}

void test_real_stop_scene_music_stream_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        const char* filename;
        std::streamoff offset;
    };
    constexpr std::array locations{
        RecordLocation{"TALK1.DAT", 0x00005925},
        RecordLocation{"TALK2.DAT", 0x0000DEA4},
        RecordLocation{"TALK3.DAT", 0x00002E28},
        RecordLocation{"TALK4.DAT", 0x00002F40},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / location.filename, std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_137_STOP_SCENE_MUSIC_STREAM);
        std::ranges::copy(record, fixture.state.window.begin());
        fixture.state.world_music_first_stream = 31U;
        fixture.state.world_music_second_stream = 32U;
        fixture.state.music_request = 0x80000001U;
        fixture.state.music_first_stream = 41U;
        fixture.state.music_second_stream = 42U;
        fixture.state.music_control_flags = 0x008300A5U;
        fixture.state.current_first_stream = 2U;
        fixture.state.current_stream_fade_divisor = 9U;
        fixture.state.current_second_stream = 7U;
        write_u16(fixture.state.window, 2U, OP_1025);

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_137_STOP_SCENE_MUSIC_STREAM &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 0U &&
            fixture.state.world_music_request == 0x80000001U &&
            fixture.state.world_music_first_stream == 31U &&
            fixture.state.world_music_second_stream == 32U &&
            fixture.state.music_request == 0U &&
            fixture.state.music_first_stream == 0U &&
            fixture.state.music_second_stream == 0U &&
            fixture.state.music_control_flags == 0x00000000U &&
            fixture.state.current_first_stream == 2U &&
            fixture.state.current_stream_fade_divisor == 7U &&
            fixture.state.current_second_stream == 7U &&
            fixture.state.previous_opcode == OP_137_STOP_SCENE_MUSIC_STREAM &&
            fixture.ports.music_transition_apply_count == 1U;
    }

    test.expect_true(
        all_records_match,
        "real opcode 137 records in all four TALK files restore the world-music slot group and continue"
    );
}

void test_real_role_distance_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        const char* filename;
        std::streamoff offset;
        u32 file_number;
    };
    constexpr std::array locations{
        RecordLocation{"TALK1.DAT", 0x00005B99, 1U},
        RecordLocation{"TALK2.DAT", 0x0001950A, 2U},
        RecordLocation{"TALK3.DAT", 0x0002397F, 3U},
        RecordLocation{"TALK4.DAT", 0x00004F08, 4U},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / location.filename, std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 14U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS);
        std::ranges::copy(record, fixture.state.window.begin());
        fixture.state.loaded_file_number = location.file_number;
        fixture.roles[1].guid = 1U;
        fixture.roles[1].world_x = 0U;
        fixture.roles[1].world_y = 0U;
        write_u16(fixture.ports.transferred_window, 0U, OP_1025);
        const u32 target = read_u32(record, 10U);

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            read_u16(record, 6U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.talk_data_offset == target &&
            fixture.context.instruction_offset == 0U &&
            fixture.state.previous_opcode ==
                OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS &&
            fixture.ports.data_load_count == 1U &&
            fixture.ports.last_data_file_number == location.file_number &&
            fixture.ports.last_data_offset == target &&
            fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U};
    }

    test.expect_true(
        all_records_match,
        "real opcode 138 records from all TALK files take the scaled-distance reload and same-call the target"
    );
}

void test_real_configure_music_stream_transition_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        const char* filename;
        std::streamoff offset;
        u16 expected_mode;
        u16 expected_pending_divisor;
    };
    constexpr std::array locations{
        RecordLocation{"TALK1.DAT", 0x000044E9, 2U, 25U},
        RecordLocation{"TALK2.DAT", 0x0000D38D, 1U, 25U},
        RecordLocation{"TALK3.DAT", 0x0000264E, 2U, 40U},
        RecordLocation{"TALK4.DAT", 0x00002DC8, 1U, 45U},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / location.filename, std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        prime_loaded_instruction(
            fixture, OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION
        );
        std::ranges::copy(record, fixture.state.window.begin());
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.state.current_stream_fade_divisor = 0x12345678U;

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION &&
            read_u16(record, 2U) == location.expected_mode &&
            read_u16(record, 4U) == location.expected_pending_divisor &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 0U &&
            fixture.state.current_first_stream == location.expected_mode &&
            fixture.state.current_stream_fade_divisor == 0x12345678U &&
            fixture.state.current_second_stream ==
                location.expected_pending_divisor &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode ==
                OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION &&
            fixture.ports.music_transition_apply_count == 0U;
    }

    test.expect_true(
        all_records_match,
        "real opcode 141 records from all TALK files configure transition state and same-call the successor"
    );
}

void test_real_initialize_primary_countdown_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        std::streamoff offset;
        u16 expected_transition_value;
    };
    constexpr std::array locations{
        RecordLocation{0x000233A7, 728U},
        RecordLocation{0x00058A88, 1111U},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 8U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        openswd3::rendering::LegacyCountdownState countdown{
            .primary_value_004c97e8 = 1U,
            .primary_value_004c97ec = 2U,
        };
        fixture.runtime.countdown = &countdown;
        prime_loaded_instruction(fixture, OP_142_INITIALIZE_PRIMARY_COUNTDOWN);
        std::ranges::copy(record, fixture.state.window.begin());
        write_u16(fixture.state.window, 8U, OP_1025);

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_142_INITIALIZE_PRIMARY_COUNTDOWN &&
            read_u16(record, 2U) == 5U && read_u16(record, 4U) == 0U &&
            read_u16(record, 6U) == location.expected_transition_value &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_142_INITIALIZE_PRIMARY_COUNTDOWN &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            countdown.primary_ticks == 9000U &&
            countdown.primary_transition_value ==
                location.expected_transition_value &&
            countdown.primary_value_004c97e8 == 0U &&
            countdown.primary_value_004c97ec == 0U &&
            openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 0x10U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 0x12U
            ) &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode ==
                OP_142_INITIALIZE_PRIMARY_COUNTDOWN;
    }

    test.expect_true(
        all_records_match,
        "real opcode 142 records initialize the primary countdown then service audio and yield"
    );
}

void test_real_disable_primary_countdown_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        const char* filename;
        std::streamoff offset;
    };
    constexpr std::array locations{
        RecordLocation{"TALK1.DAT", 0x0002D30D},
        RecordLocation{"TALK1.DAT", 0x00038E4F},
        RecordLocation{"TALK3.DAT", 0x0002D19E},
        RecordLocation{"TALK3.DAT", 0x0002D1B2},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / location.filename, std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        openswd3::rendering::LegacyCountdownState countdown{
            .primary_ticks = 1234U,
            .secondary_ticks = 5678U,
        };
        fixture.runtime.countdown = &countdown;
        prime_loaded_instruction(fixture, OP_143_DISABLE_PRIMARY_COUNTDOWN);
        std::ranges::copy(record, fixture.state.window.begin());
        write_u16(fixture.state.window, 2U, OP_1025);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x10U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x12U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 0x4CU);

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_143_DISABLE_PRIMARY_COUNTDOWN &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_143_DISABLE_PRIMARY_COUNTDOWN &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            countdown.primary_ticks == 0xFFFFFFFFU &&
            countdown.secondary_ticks == 5678U &&
            !openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 0x10U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 0x12U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 0x4CU
            ) &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_143_DISABLE_PRIMARY_COUNTDOWN;
    }

    test.expect_true(
        all_records_match,
        "real opcode 143 records disable the primary countdown then service audio and yield"
    );
}

void test_real_request_special_mode_four_or_five_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00025F51);
    std::array<u8, 4U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_144_REQUEST_SPECIAL_MODE_4_OR_5);
    std::ranges::copy(record, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    fixture.special_mode_state = 0x11111111U;
    fixture.high_priority_state = 0x22222222U;
    fixture.high_priority_submode = 0x33333333U;
    fixture.high_priority_auxiliary = 0x44444444U;
    fixture.special_input_mode = 0x55555555U;

    const auto result = fixture.step();
    test.expect_true(
        static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            record[2U] == 0U && record[3U] == 0U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_144_REQUEST_SPECIAL_MODE_4_OR_5 &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            fixture.special_mode_state == 0x80000004U &&
            fixture.high_priority_state == 0U &&
            fixture.high_priority_submode == 0U &&
            fixture.high_priority_auxiliary == 0U &&
            fixture.special_input_mode == 0U &&
            fixture.ports.input_menu_reset_count == 1U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_144_REQUEST_SPECIAL_MODE_4_OR_5,
        "real opcode 144 record requests special mode four, resets menu state, services audio, and yields"
    );
}

void test_real_set_story_flag_70_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RecordLocation {
        const char* filename;
        std::streamoff offset;
    };
    constexpr std::array locations{
        RecordLocation{"TALK1.DAT", 0x000226C6},
        RecordLocation{"TALK2.DAT", 0x0002E6EC},
        RecordLocation{"TALK3.DAT", 0x0000B057},
        RecordLocation{"TALK4.DAT", 0x00003080},
    };

    bool all_records_match = true;
    for (const auto& location : locations) {
        std::ifstream input{
            root / location.filename, std::ios::binary | std::ios::in
        };
        input.seekg(location.offset);
        std::array<u8, 2U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_147_SET_STORY_FLAG_70);
        std::ranges::copy(record, fixture.state.window.begin());
        write_u16(fixture.state.window, 2U, OP_1025);
        openswd3::world_map::clear_legacy_world_story_flag(fixture.state, 70U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 69U);
        openswd3::world_map::set_legacy_world_story_flag(fixture.state, 71U);

        const auto result = fixture.step();
        all_records_match = all_records_match && static_cast<bool>(input) &&
            read_u16(record, 0U) == OP_147_SET_STORY_FLAG_70 &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_147_SET_STORY_FLAG_70 &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 69U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 70U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                                fixture.state, 71U
            ) &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_147_SET_STORY_FLAG_70;
    }

    test.expect_true(
        all_records_match,
        "real opcode 147 records set story flag 70 then service audio and yield"
    );
}

void test_real_batch_set_role_positions_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0001BF4F);
    std::array<u8, 10U> record{};
    input.read(
        reinterpret_cast<char*>(record.data()),
        static_cast<std::streamsize>(record.size())
    );
    const bool record_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[1].guid = 1U;
    StoryPathHarness paths{fixture};
    prime_loaded_instruction(fixture, OP_116_BATCH_SET_ROLE_POSITIONS);
    std::ranges::copy(record, fixture.state.window.begin());
    write_u16(fixture.state.window, record.size(), OP_1025);
    const auto result = fixture.step();
    const auto slot = std::ranges::find_if(
        fixture.active_object_slots,
        [](const LegacyWorldObjectSlot& candidate) {
            return read_u16(candidate.bytes, 0U) == 1U;
        }
    );

    test.expect_true(
        record_read &&
            read_u16(record, 0U) == OP_116_BATCH_SET_ROLE_POSITIONS &&
            read_u16(record, 2U) == 1U && read_u16(record, 4U) == 1U &&
            read_u16(record, 6U) == 42U && read_u16(record, 8U) == 33U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 0U &&
            fixture.context.instruction_offset == 10U &&
            fixture.state.previous_opcode == OP_116_BATCH_SET_ROLE_POSITIONS &&
            slot != fixture.active_object_slots.end() &&
            read_u16(slot->bytes, 4U) == 672U &&
            read_u16(slot->bytes, 6U) == 528U,
        "real opcode 116 schedules its counted role-position record"
    );
}

void test_real_remove_dialogs_for_role_guid_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000462C);
    std::array<u8, 8U> records{};
    input.read(
        reinterpret_cast<char*>(records.data()),
        static_cast<std::streamsize>(records.size())
    );
    const bool records_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[0].guid = 0U;
    fixture.roles[1].guid = 1U;
    fixture.roles[0].interaction_gate = 3U;
    fixture.roles[1].interaction_gate = 4U;
    fixture.dialogs.messages.emplace_back();
    fixture.dialogs.messages.back().record.role_index = 1U;
    fixture.dialogs.messages.back().text = {'A'};
    fixture.dialogs.messages.back().caption = {'B'};
    fixture.dialogs.messages.emplace_back();
    fixture.dialogs.messages.back().record.role_index = 0U;
    fixture.dialogs.messages.back().text = {'C'};
    fixture.dialogs.messages.back().caption = {'D'};
    fixture.dialogs.close.flagged_dialog_counter = 0x8002U;
    prime_loaded_instruction(fixture, OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID);
    std::ranges::copy(records, fixture.state.window.begin());
    write_u16(fixture.state.window, records.size(), OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        records_read &&
            read_u16(records, 0U) == OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
            read_u16(records, 2U) == 0U &&
            read_u16(records, 4U) == OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
            read_u16(records, 6U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 3U &&
            result.direct_audio_service_count == 0U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode ==
                OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID &&
            fixture.dialogs.messages.empty() &&
            fixture.roles[0].interaction_gate == 0U &&
            fixture.roles[1].interaction_gate == 0U &&
            fixture.dialogs.close.flagged_dialog_counter == 0x8000U,
        "real opcode 118 records remove role zero and role one dialogs"
    );
}

void test_real_wait_dialog_flag_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealCase {
        std::streamoff offset;
        u16 opcode;
        u16 selector;
        u16 role_index;
        u32 completion_mask;
    };
    constexpr std::array<RealCase, 2U> cases{
        RealCase{
            0x0000968B,
            OP_119_WAIT_DIALOG_FLAG_BIT0,
            10000U,
            1U,
            0x00000001U,
        },
        RealCase{
            0x000045E4,
            OP_139_WAIT_DIALOG_FLAG_BIT15,
            0U,
            0U,
            0x00008000U,
        },
    };

    for (const auto& real_case : cases) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(real_case.offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        const bool record_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.roles[real_case.role_index].guid = real_case.selector;
        fixture.dialogs.messages.emplace_back();
        fixture.dialogs.messages.back().record.role_index =
            real_case.role_index;
        fixture.dialogs.messages.back().record.flags = 0U;
        prime_loaded_instruction(fixture, real_case.opcode);
        std::ranges::copy(record, fixture.state.window.begin());
        write_u16(fixture.state.window, record.size(), OP_1025);
        const auto waiting = fixture.step();
        fixture.dialogs.messages.back().record.flags |=
            real_case.completion_mask;
        const auto completed = fixture.step();

        test.expect_true(
            record_read && read_u16(record, 0U) == real_case.opcode &&
                read_u16(record, 2U) == real_case.selector &&
                waiting.status == LegacyWorldStoryVmStatus::yielded &&
                waiting.executed_instruction_count == 1U &&
                waiting.direct_audio_service_count == 1U &&
                completed.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                completed.opcode == OP_1025 &&
                completed.executed_instruction_count == 2U &&
                completed.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == real_case.opcode,
            "real dialog wait records cross their variant completion bit"
        );
    }
}

void test_real_update_role_action_fields_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](const std::streamoff offset) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(offset);
        std::array<u8, 10U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return std::pair{record, static_cast<bool>(input)};
    };
    const auto [found_record, found_read] = read_record(0x0000465A);
    const auto [missing_record, missing_read] = read_record(0x00004380);

    Fixture found;
    found.roles[1].guid = 0x007BU;
    prime_loaded_instruction(found, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    std::ranges::copy(found_record, found.state.window.begin());
    write_u16(found.state.window, found_record.size(), OP_1025);
    const auto found_result = found.step();

    Fixture missing;
    prime_loaded_instruction(missing, OP_120_UPDATE_ROLE_ACTION_FIELDS);
    std::ranges::copy(missing_record, missing.state.window.begin());
    write_u16(missing.state.window, missing_record.size(), OP_1025);
    const auto missing_result = missing.step();
    const auto missing_patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();

    test.expect_true(
        found_read &&
            read_u16(found_record, 0U) == OP_120_UPDATE_ROLE_ACTION_FIELDS &&
            read_u16(found_record, 2U) == 0x007BU &&
            read_u16(found_record, 4U) == 0x0231U &&
            read_u16(found_record, 6U) == 0x0008U &&
            read_u16(found_record, 8U) == 0U &&
            found_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            found_result.opcode == OP_1025 &&
            found_result.executed_instruction_count == 2U &&
            found.roles[1].action.action_id == 0x0231U &&
            found.roles[1].action.base_variant == 8U &&
            found.roles[1].action.variant_delta == 0U &&
            (found.roles[1].flags & 0x00001000U) != 0U &&
            found.state.previous_opcode == OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "real opcode 120 record updates a live role action"
    );
    test.expect_true(
        missing_read &&
            read_u16(missing_record, 0U) == OP_120_UPDATE_ROLE_ACTION_FIELDS &&
            read_u16(missing_record, 2U) == 1U &&
            read_u16(missing_record, 4U) == 0x0062U &&
            read_u16(missing_record, 6U) == 0xFFFFU &&
            read_u16(missing_record, 8U) == 0xFFFFU &&
            missing_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            missing_result.opcode == OP_1025 &&
            missing.ports.role_patch_requests.size() == 1U &&
            missing_patch.guid == 1U && missing_patch.action_id == 0x0062U &&
            missing_patch.base_variant == 0xFFFFU &&
            missing_patch.variant_delta == 0xFFFFU &&
            missing_patch.flags_or_mask == 0x1000U &&
            missing_patch.flags_and_mask == 0xFFFFU &&
            missing.state.previous_opcode == OP_120_UPDATE_ROLE_ACTION_FIELDS,
        "real opcode 120 record patches a missing MAPS role source"
    );
}

void test_real_load_name_record_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](const std::streamoff offset) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(offset);
        std::array<u8, 4U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return std::pair{record, static_cast<bool>(input)};
    };
    const auto [explicit_record, explicit_read] = read_record(0x000364EB);
    const auto [dynamic_record, dynamic_read] = read_record(0x000364C9);

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    constexpr std::array<u8, 5U> default_first{0xC1U, 0xC9U, 0xAFU, 0x53U, 0U};
    constexpr std::array<u8, 5U> default_second{0xA9U, 0x67U, 0xA5U, 0x69U, 0U};
    const auto execute = [&](const std::array<u8, 4U>& record,
                             const bool dynamic) {
        Fixture fixture;
        std::ranges::copy(default_first, fixture.first_name.begin());
        std::ranges::copy(default_second, fixture.second_name.begin());
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFCU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        if (dynamic) {
            fixture.state.script_variables[11U] = 782U;
        }
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFCU);

        const auto result = openswd3::world_map::step_legacy_world_story_vm(
            fixture.context,
            fixture.state,
            fixture.roles,
            0U,
            fixture.active_object_slots,
            databases.maps_payload_bytes(),
            fixture.dialogs,
            fixture.dialog_resources,
            fixture.first_name,
            fixture.second_name,
            fixture.runtime,
            fixture.ports
        );

        return std::tuple{
            result,
            fixture.state.speaker_name,
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
        };
    };
    const auto
        [explicit_result, explicit_name, explicit_ip, explicit_previous] =
            execute(explicit_record, false);
    const auto [dynamic_result, dynamic_name, dynamic_ip, dynamic_previous] =
        execute(dynamic_record, true);

    test.expect_true(
        explicit_read && dynamic_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            read_u16(explicit_record, 0U) == OP_91_LOAD_NAME_RECORD &&
            read_u16(explicit_record, 2U) == 782U &&
            explicit_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            explicit_result.executed_instruction_count == 1U &&
            explicit_ip == 0x8000U &&
            explicit_previous == OP_91_LOAD_NAME_RECORD &&
            read_u16(dynamic_record, 0U) == OP_162_LOAD_DYNAMIC_NAME_RECORD &&
            read_u16(dynamic_record, 2U) == 11U &&
            dynamic_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            dynamic_result.executed_instruction_count == 1U &&
            dynamic_ip == 0x8000U &&
            dynamic_previous == OP_162_LOAD_DYNAMIC_NAME_RECORD &&
            explicit_name == dynamic_name &&
            std::ranges::find(explicit_name, u8{}) != explicit_name.end(),
        "real opcodes 91 and 162 resolve record 782 through explicit and variable-11 paths to the same exact-tail name"
    );
}

void test_real_control_packed_row_effect_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](const std::streamoff offset) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(offset);
        std::array<u8, 6U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto operation_zero = read_record(0x00006147);
    const auto operation_one = read_record(0x00009697);
    const auto stale_operation = read_record(0x00053AD3);

    const auto execute = [](const std::array<u8, 6U>& record) {
        Fixture fixture;
        std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
            {.mode = static_cast<u16>(0x8000U | read_u16(record, 2U))},
        };
        fixture.runtime.packed_row_effects = &effects;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FFAU);
        const auto result = fixture.step();
        return std::tuple{
            result,
            effects.front().mode,
            fixture.context.instruction_offset,
            fixture.state.previous_opcode,
        };
    };
    const auto [zero_result, zero_mode, zero_ip, zero_previous] =
        execute(operation_zero);
    const auto [one_result, one_mode, one_ip, one_previous] =
        execute(operation_one);
    const auto [stale_result, stale_mode, stale_ip, stale_previous] =
        execute(stale_operation);

    test.expect_true(
        read_u16(operation_zero, 0U) == OP_84_CONTROL_PACKED_ROW_EFFECT &&
            read_u16(operation_zero, 2U) == 5U &&
            read_u16(operation_zero, 4U) == 0U &&
            zero_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            zero_mode == 0x2005U && zero_ip == 0x8000U &&
            zero_previous == OP_84_CONTROL_PACKED_ROW_EFFECT &&
            read_u16(operation_one, 0U) == OP_84_CONTROL_PACKED_ROW_EFFECT &&
            read_u16(operation_one, 2U) == 1U &&
            read_u16(operation_one, 4U) == 1U &&
            one_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            one_mode == 0x1001U && one_ip == 0x8000U &&
            one_previous == OP_84_CONTROL_PACKED_ROW_EFFECT &&
            read_u16(stale_operation, 0U) == OP_84_CONTROL_PACKED_ROW_EFFECT &&
            read_u16(stale_operation, 2U) == 2U &&
            read_u16(stale_operation, 4U) == 3U &&
            stale_result.status ==
                LegacyWorldStoryVmStatus::
                    unsupported_packed_row_effect_operation &&
            stale_mode == 0x8002U && stale_ip == 0x7FFAU &&
            stale_previous == 0x66U,
        "real opcode 84 records preserve operations 0/1 and typed-stop the stale-var_44 operation 3"
    );
}

void test_real_upsert_packed_row_effect_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000060C2);
    std::array<u8, 16U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    std::list<openswd3::rendering::LegacyPackedRowEffect> effects{
        {.mode = 0x0805U},
        {.mode = 0x8006U},
    };
    fixture.runtime.packed_row_effects = &effects;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FF0U;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FF0U);

    const auto result = fixture.step();
    const auto& effect = effects.front();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_83_UPSERT_PACKED_ROW_EFFECT &&
            read_u16(instruction, 2U) == 5U &&
            read_u16(instruction, 4U) == 9U &&
            read_u16(instruction, 6U) == 1U &&
            read_u16(instruction, 8U) == 130U &&
            read_u16(instruction, 10U) == 360U &&
            read_u16(instruction, 12U) == 382U &&
            read_u16(instruction, 14U) == 111U &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_83_UPSERT_PACKED_ROW_EFFECT &&
            result.executed_instruction_count == 1U && effects.size() == 2U &&
            effect.base_x == 130 && effect.base_y == 360 &&
            effect.limit == 382 && effect.row_count == 110 &&
            effect.mode == 0x4005U && effect.row_offsets.size() == 110U &&
            effect.row_lengths.size() == 110U &&
            effect.row_offsets.front() == 380 &&
            effect.row_lengths.front() == 2 &&
            std::next(effects.begin())->mode == 0x8006U &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_83_UPSERT_PACKED_ROW_EFFECT,
        "real opcode 83 record replaces ID 5 and builds the 110-row mode-1 effect before exact-tail failure"
    );
}

void test_real_dismiss_role_head_action_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000614D);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    openswd3::world_map::LegacyRoleHeadActionList actions(1U);
    actions.front().action.action_id = 0x2711U;
    actions.front().action.base_variant = 0U;
    actions.front().current_x = 100;
    actions.front().horizontal_motion = 0;
    fixture.runtime.role_head_actions = &actions;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFAU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFAU);

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_82_DISMISS_ROLE_HEAD_ACTION &&
            read_u16(instruction, 2U) == 0x2711U &&
            read_u16(instruction, 4U) == 0U &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_82_DISMISS_ROLE_HEAD_ACTION &&
            result.executed_instruction_count == 1U &&
            actions.front().horizontal_motion == -1 &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_82_DISMISS_ROLE_HEAD_ACTION,
        "real opcode 82 record dismisses the matching left-side head action before exact-tail fetch failure"
    );
}

void test_real_enqueue_role_head_action_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record = [&root](
                                 const std::string_view file_name,
                                 const std::streamoff offset
                             ) -> std::array<u8, 10U> {
        std::ifstream input{root / file_name, std::ios::binary | std::ios::in};
        input.seekg(offset);
        std::array<u8, 10U> record{};
        input.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size())
        );
        return record;
    };
    const auto ordinary_record = read_record("TALK1.DAT", 0x000060D2);
    const auto special_record = read_record("TALK2.DAT", 0x0000EBAC);

    const auto execute = [](const std::array<u8, 10U>& record) {
        Fixture fixture;
        openswd3::world_map::LegacyRoleHeadActionList actions;
        fixture.runtime.role_head_actions = &actions;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = 0x7FF6U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        std::ranges::copy(record, fixture.state.window.begin() + 0x7FF6U);
        const auto result = fixture.step();
        return std::tuple{result, actions.front()};
    };
    const auto [ordinary_result, ordinary] = execute(ordinary_record);
    const auto [special_result, special] = execute(special_record);

    test.expect_true(
        read_u16(ordinary_record, 0U) == OP_81_ENQUEUE_ROLE_HEAD_ACTION &&
            read_u16(ordinary_record, 2U) == 0x2711U &&
            read_u16(ordinary_record, 4U) == 0U &&
            read_u16(ordinary_record, 6U) == 0x0230U &&
            read_u16(ordinary_record, 8U) == 0x01CCU &&
            ordinary_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            ordinary.action.action_id == 0x2711U &&
            ordinary.action.base_variant == 0U && ordinary.target_x == 560 &&
            ordinary.y == 460 && ordinary.current_x == 760 &&
            ordinary.horizontal_motion == 0 &&
            read_u16(special_record, 0U) == OP_81_ENQUEUE_ROLE_HEAD_ACTION &&
            read_u16(special_record, 2U) == 0x2711U &&
            read_u16(special_record, 4U) == 9U &&
            read_u16(special_record, 6U) == 50U &&
            read_u16(special_record, 8U) == 0x8078U &&
            special_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            special.target_x == 50 && special.y == 120 &&
            special.current_x == 50 &&
            special.horizontal_motion == std::bit_cast<i16>(u16{0x8000U}),
        "real opcode 81 records preserve ordinary right-start and rare bit15 special-start behavior"
    );
}

void test_real_clear_text_control_bit29_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004520);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );

    Fixture fixture;
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0x7FFEU;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    fixture.state.previous_opcode = 0x66U;
    fixture.state.text_control_flags = 0xFFFFFFFFU;
    std::ranges::copy(instruction, fixture.state.window.begin() + 0x7FFEU);

    const auto result = fixture.step();

    test.expect_true(
        input.gcount() == static_cast<std::streamsize>(instruction.size()) &&
            read_u16(instruction, 0U) == OP_80_CLEAR_TEXT_CONTROL_BIT29 &&
            result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            result.opcode == OP_80_CLEAR_TEXT_CONTROL_BIT29 &&
            result.executed_instruction_count == 1U &&
            fixture.state.text_control_flags == 0xDFFFFFFFU &&
            fixture.context.instruction_offset == 0x8000U &&
            fixture.state.previous_opcode == OP_80_CLEAR_TEXT_CONTROL_BIT29,
        "real opcode 80 record clears text-control bit 29 before exact-tail fetch failure"
    );
}

void test_real_shared_scene_render_control_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record =
        [&root](const std::streamoff file_offset) -> std::array<u8, 2U> {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(file_offset);
        std::array<u8, 2U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        return instruction;
    };
    const auto suspend = read_record(0x000044E3);
    const auto resume = read_record(0x000046B6);

    Fixture fixture;
    u8 scene_render_flags{0xA5U};
    std::vector<u8> flags_during_clear;
    fixture.runtime.scene_render_flags = &scene_render_flags;
    fixture.ports.framebuffer_clear_callback = [&]() noexcept {
        flags_during_clear.push_back(scene_render_flags);
    };
    const auto execute = [&fixture](const std::array<u8, 2U>& instruction) {
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;
        return fixture.step();
    };
    const auto suspend_result = execute(suspend);
    const u8 flags_after_suspend = scene_render_flags;
    const auto resume_result = execute(resume);

    test.expect_true(
        read_u16(suspend, 0U) ==
                OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING &&
            read_u16(resume, 0U) == OP_60_RESUME_WORLD_SCENE_RENDERING &&
            suspend_result.status == LegacyWorldStoryVmStatus::yielded &&
            suspend_result.opcode ==
                OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING &&
            suspend_result.executed_instruction_count == 1U &&
            resume_result.status == LegacyWorldStoryVmStatus::yielded &&
            resume_result.opcode == OP_60_RESUME_WORLD_SCENE_RENDERING &&
            resume_result.executed_instruction_count == 1U &&
            flags_during_clear == std::vector<u8>{0xA4U} &&
            flags_after_suspend == 0xA5U && scene_render_flags == 0xA4U &&
            fixture.ports.framebuffer_clear_count == 1U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_60_RESUME_WORLD_SCENE_RENDERING,
        "real opcodes 61 and 60 clear and suspend the world scene, then resume it"
    );
}

void test_real_shared_picture_action_enqueue_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record =
        [&root](const std::streamoff file_offset) -> std::array<u8, 10U> {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(file_offset);
        std::array<u8, 10U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        return instruction;
    };
    const auto primary = read_record(0x0000549F);
    const auto secondary_first = read_record(0x0000468A);
    const auto secondary_second = read_record(0x00004698);

    Fixture fixture;
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    fixture.runtime.picture_actions = &picture_actions;
    const auto execute = [&fixture](const std::array<u8, 10U>& instruction) {
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;
        return fixture.step();
    };

    const auto primary_result = execute(primary);
    const auto first_secondary_result = execute(secondary_first);
    const auto second_secondary_result = execute(secondary_second);
    const auto secondary_tail = picture_actions.secondary.size() >= 2U
        ? std::next(picture_actions.secondary.begin())
        : picture_actions.secondary.end();

    test.expect_true(
        read_u16(primary, 0U) == OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION &&
            read_u16(secondary_first, 0U) ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            read_u16(secondary_second, 0U) ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            primary_result.status == LegacyWorldStoryVmStatus::yielded &&
            first_secondary_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            second_secondary_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            picture_actions.primary.size() == 1U &&
            picture_actions.secondary.size() == 2U &&
            picture_actions.primary.front().screen_x == 82U &&
            picture_actions.primary.front().screen_y == 344U &&
            picture_actions.primary.front().action.action_id == 9006U &&
            picture_actions.primary.front().action.base_variant == 2U &&
            picture_actions.secondary.front().screen_x == 480U &&
            picture_actions.secondary.front().screen_y == 400U &&
            picture_actions.secondary.front().action.action_id == 9050U &&
            picture_actions.secondary.front().action.base_variant == 1U &&
            secondary_tail->screen_x == 360U &&
            secondary_tail->screen_y == 400U &&
            secondary_tail->action.action_id == 9050U &&
            secondary_tail->action.base_variant == 0U &&
            fixture.context.instruction_offset == 10U &&
            fixture.state.previous_opcode ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            fixture.ports.direct_audio_service_count == 3U,
        "real opcodes 58 and 153 prepend primary and secondary picture actions"
    );
}

void test_real_shared_role_spatial_group_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealRecord {
        std::streamoff file_offset;
        u16 opcode;
        u16 selector;
        u32 old_group;
        u32 target_group;
    };
    constexpr std::array<RealRecord, 4U> records{
        RealRecord{0x000121DA, OP_55_SET_ROLE_SPATIAL_GROUP_1, 322U, 0U, 1U},
        RealRecord{0x00012B12, OP_56_SET_ROLE_SPATIAL_GROUP_0, 322U, 2U, 0U},
        RealRecord{0x00005084, OP_57_SET_ROLE_SPATIAL_GROUP_2, 701U, 1U, 2U},
        RealRecord{0x0000526B, OP_57_SET_ROLE_SPATIAL_GROUP_2, 702U, 1U, 2U},
    };
    constexpr std::size_t role_row =
        openswd3::world_map::kLegacySpatialRowPadding + 2U;

    for (const RealRecord record : records) {
        std::ifstream input{
            root / "TALK4.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(record.file_offset);
        std::array<u8, 4U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        openswd3::world_map::LegacyRoleSpatialIndex spatial;
        spatial.map_height = 4U;
        for (auto& group : spatial.row_heads) {
            group.assign(44U, 0U);
        }
        fixture.roles[1].guid = record.selector;
        fixture.roles[1].world_y = 32U;
        fixture.roles[1].flags = record.old_group;
        const bool inserted = openswd3::world_map::insert_legacy_role_spatially(
            spatial, fixture.roles, 1U, record.old_group
        );
        fixture.runtime.spatial_index = &spatial;
        prime_loaded_instruction(fixture, record.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            instruction_read && inserted &&
                read_u16(instruction, 0U) == record.opcode &&
                read_u16(instruction, 2U) == record.selector &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == record.opcode &&
                result.executed_instruction_count == 1U &&
                (fixture.roles[1].flags & 3U) == record.target_group &&
                spatial.row_heads[record.old_group][role_row] == 0U &&
                spatial.row_heads[record.target_group][role_row] == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == record.opcode &&
                fixture.ports.direct_audio_service_count == 1U,
            "real opcodes 55-57 move the selected role between spatial groups"
        );
    }
}

void test_real_repeat_role_action_refresh_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00005A6B);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[2].guid = 1U;
    auto& action = fixture.roles[2].action;
    action.command_cursor = 0x1111U;
    action.wait_remaining = 0x2222U;
    action.field_58 = 0x3333U;
    prime_loaded_instruction(fixture, OP_54_REPEAT_ROLE_ACTION_REFRESH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            read_u16(instruction, 2U) == 1U &&
            static_cast<i16>(read_u16(instruction, 4U)) == 1 &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 2U &&
            result.action_update_failure_count == 0U &&
            action.command_cursor == 0U && action.wait_remaining == 0U &&
            action.field_58 == 0U && fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 54 performs the initial and requested repeated refresh"
    );
}

void test_real_wait_for_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000043B8);
    std::array<u8, 18U> instructions{};
    input.read(
        reinterpret_cast<char*>(instructions.data()),
        static_cast<std::streamsize>(instructions.size())
    );
    const bool instructions_read = static_cast<bool>(input);

    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState color{};
    fixture.runtime.frame_color = &color;
    prime_loaded_instruction(fixture, OP_52_START_FRAME_COLOR_TRANSITION);
    std::ranges::copy(instructions, fixture.state.window.begin());
    write_u16(fixture.state.window, 18U, OP_1025);

    const auto waiting_result = fixture.step();
    const bool waiting_state =
        waiting_result.status == LegacyWorldStoryVmStatus::yielded &&
        waiting_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
        waiting_result.executed_instruction_count == 2U &&
        color.countdown == 6 && color.step_red == 5.0F &&
        color.step_green == 5.0F && color.step_blue == 5.0F &&
        fixture.context.instruction_offset == 16U &&
        fixture.state.previous_opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION;

    color.countdown = 0;
    const auto completed_result = fixture.step();

    test.expect_true(
        instructions_read &&
            read_u16(instructions, 0U) == OP_52_START_FRAME_COLOR_TRANSITION &&
            read_u16(instructions, 16U) == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            waiting_state &&
            completed_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            completed_result.opcode == OP_1025 &&
            completed_result.executed_instruction_count == 2U &&
            color.countdown == 0 && fixture.context.instruction_offset == 18U &&
            fixture.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 53 waits after transition start and continues after completion"
    );
}

void test_real_wait_for_camera_move_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000046C2);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    CameraMoveFixture fixture;
    fixture.state.previous_opcode = 0x55U;
    prime_loaded_instruction(fixture, OP_51_WAIT_CAMERA_MOVE_COMPLETE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, OP_1025);
    fixture.camera_pan.remaining_x = 64;
    fixture.camera_pan.step_x = 4;

    const auto waiting = fixture.step();
    const u16 waiting_offset = fixture.context.instruction_offset;
    const u32 waiting_previous = fixture.state.previous_opcode;
    const i32 waiting_remaining_x = fixture.camera_pan.remaining_x;
    const i32 waiting_step_x = fixture.camera_pan.step_x;
    fixture.camera_pan.remaining_x = 0;
    fixture.camera_pan.step_x = 0;
    const auto completed = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting.executed_instruction_count == 1U && waiting_offset == 0U &&
            waiting_previous == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_remaining_x == 64 && waiting_step_x == 4 &&
            completed.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            completed.opcode == OP_1025 &&
            completed.executed_instruction_count == 2U &&
            waiting.direct_audio_service_count == 1U &&
            completed.direct_audio_service_count == 0U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 51 audio-yields for camera motion then continues without a second audio service"
    );
}

void test_real_wait_for_camera_top_while_moving_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK4.DAT", std::ios::binary | std::ios::in};
    bool records_match = true;
    for (u32 index = 0U; index < 13U; ++index) {
        const u32 file_offset = 0x0002FC5FU + index * 14U;
        input.seekg(static_cast<std::streamoff>(file_offset));
        std::array<u8, 4U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const i32 expected_top = static_cast<i16>(read_u16(instruction, 2U));
        records_match = records_match && static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            expected_top == static_cast<i32>(800U + index * 320U);

        CameraMoveFixture matching;
        matching.camera_pan.remaining_x = 1;
        prime_loaded_instruction(matching, OP_191_WAIT_CAMERA_TOP_WHILE_MOVING);
        std::ranges::copy(instruction, matching.state.window.begin());
        write_u16(matching.state.window, 4U, OP_1025);
        const auto matching_result = matching.step(0, expected_top);

        CameraMoveFixture mismatching;
        mismatching.camera_pan.remaining_y = -1;
        prime_loaded_instruction(
            mismatching, OP_191_WAIT_CAMERA_TOP_WHILE_MOVING
        );
        std::ranges::copy(instruction, mismatching.state.window.begin());
        write_u16(mismatching.state.window, 4U, OP_1025);
        const auto mismatching_result = mismatching.step(0, expected_top + 1);

        records_match = records_match &&
            matching_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            matching_result.executed_instruction_count == 2U &&
            matching.context.instruction_offset == 4U &&
            matching.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            matching.ports.direct_audio_service_count == 0U &&
            mismatching_result.status == LegacyWorldStoryVmStatus::yielded &&
            mismatching_result.executed_instruction_count == 1U &&
            mismatching_result.direct_audio_service_count == 1U &&
            mismatching.context.instruction_offset == 0U &&
            mismatching.state.previous_opcode ==
                OP_191_WAIT_CAMERA_TOP_WHILE_MOVING &&
            mismatching.ports.direct_audio_service_count == 1U;
    }

    test.expect_true(
        records_match,
        "all thirteen real opcode 191 records preserve their signed viewport-top equality and mismatch paths"
    );
}

void test_real_set_role_flag_8000_and_clear_one_shots_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000049F0);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(
        fixture, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.flags_or_mask == 0x8000U &&
            request.flags_and_mask == 0xFFFFU,
        "real opcode 39 record patches missing role flag then continues"
    );
}

void test_real_clear_role_from_scene_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004656);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_38_CLEAR_ROLE_FROM_SCENE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_38_CLEAR_ROLE_FROM_SCENE &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.flags_or_mask == 0U &&
            request.flags_and_mask == 0x7FFFU,
        "real opcode 38 record patches missing role flags then continues"
    );
}

void test_real_shared_dialog_handler_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        const char* filename;
        std::streamoff file_offset;
        std::size_t size;
        u16 opcode;
    };
    constexpr std::array<Sample, 8U> samples{
        Sample{"TALK4.DAT", 0x000304C5, 36U, 1U},
        Sample{"TALK1.DAT", 0x00007533, 23U, 2U},
        Sample{"TALK1.DAT", 0x00004295, 77U, 3U},
        Sample{"TALK1.DAT", 0x00004422, 185U, 4U},
        Sample{"TALK1.DAT", 0x00020CA4, 51U, 5U},
        Sample{"TALK1.DAT", 0x000046CC, 34U, 6U},
        Sample{"TALK1.DAT", 0x00002634, 56U, 89U},
        Sample{"TALK1.DAT", 0x000027A2, 32U, 90U},
    };
    constexpr std::array<u8, 2U> percent_t{'%', 'T'};

    for (const auto& sample : samples) {
        std::ifstream input{
            root / sample.filename, std::ios::binary | std::ios::in
        };
        std::vector<u8> instruction(sample.size);
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        if (!input) {
            test.expect_true(false, "real shared-dialog sample is readable");
            continue;
        }

        Fixture fixture;
        prime_loaded_instruction(fixture, sample.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.roles[0].guid = 0xEEEEU;
        const u16 selector = read_u16(instruction, 2U);
        fixture.roles[1].guid =
            selector == 0xFFF0U ? fixture.context.source_guid : selector;
        const auto result = fixture.step();
        const u32 mode =
            sample.opcode <= 2U ? 0U : (sample.opcode <= 6U ? 1U : 2U);
        const std::size_t text_offset =
            mode == 0U ? 6U : (mode == 1U ? 14U : 10U);
        const auto& message = fixture.dialogs.messages.front();
        const bool odd_variant = (sample.opcode & 1U) != 0U;
        const auto percent_t_match =
            std::ranges::search(message.text, percent_t);
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == sample.opcode &&
                result.executed_instruction_count == 1U &&
                result.dialog_enqueue_count == 1U &&
                result.dialog_text_prepare_count == 1U &&
                result.dialog_text_prepare_success_count == 0U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.instruction_offset == sample.size &&
                message.text.size() == sample.size - text_offset &&
                fixture.roles[1].interaction_gate == (odd_variant ? 1U : 2U) &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 3U, 4U, 2U} &&
                (sample.opcode != 1U ||
                 percent_t_match.begin() != message.text.end()),
            "one real physical record closes each shared dialog variant"
        );
    }
}

void test_real_suppress_next_dialog_flag18_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        std::size_t size;
        std::size_t dialog_offset;
        u32 executed_instruction_count;
    };
    constexpr std::array<Sample, 2U> samples{
        Sample{0x00005975, 40U, 4U, 3U},
        Sample{0x0000762E, 33U, 2U, 2U},
    };

    for (const auto sample : samples) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        std::vector<u8> instructions(sample.size);
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instructions.data()),
            static_cast<std::streamsize>(instructions.size())
        );
        if (!input) {
            test.expect_true(false, "real opcode 160 dialog chain is readable");
            continue;
        }

        Fixture fixture;
        prime_loaded_instruction(fixture, OP_160_SUPPRESS_NEXT_DIALOG_FLAG18);
        std::ranges::copy(instructions, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();
        test.expect_true(
            read_u16(instructions, 0U) == OP_160_SUPPRESS_NEXT_DIALOG_FLAG18 &&
                read_u16(instructions, sample.dialog_offset) == 6U &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 6U &&
                result.executed_instruction_count ==
                    sample.executed_instruction_count &&
                result.dialog_enqueue_count == 1U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.instruction_offset == sample.size &&
                fixture.dialogs.messages.size() == 1U &&
                fixture.dialogs.messages.front().record.flags == 0x40U &&
                fixture.state.next_dialog_flag18_suppression == 0U &&
                fixture.state.previous_opcode == 6U,
            "real opcode 160 chains suppress dialog flag bit 18 once and clear the one-shot value"
        );
    }
}

void test_real_story_248_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = 248U;
    context.talk_script_id = 248U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    roles[0].world_x = 16U;
    roles[0].world_y = 16U;
    roles[1].guid = 248U;
    roles[1].flags = 0U;
    roles[1].world_x = 320U;
    roles[1].world_y = 240U;
    roles[1].action.variant_delta = 4U;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U, 0x2331U};
    constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU, 0x233BU};
    for (std::size_t index = 0U; index < kFrames.size(); ++index) {
        dialog_resources.frame_actions[index].action_id = kFrames[index];
        dialog_resources.caption_actions[index].action_id = kCaptions[index];
    }
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    openswd3::world_map::LegacyWorldCameraRect camera{};
    const openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .camera = &camera,
    };

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        databases.maps_payload_bytes(),
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        runtime,
        ports
    );
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 89U && result.executed_instruction_count == 3U &&
            result.dialog_enqueue_count == 1U &&
            dialogs.messages.size() == 1U &&
            !dialogs.messages.front().caption.empty() &&
            dialogs.messages.front().record.width == 154U &&
            dialogs.messages.front().record.height == 88U &&
            roles[1].interaction_gate == 1U,
        "real story 248 executes 0x402, 91 and 89 into its first dialog"
    );
}

void test_real_new_game_story_patches_unloaded_role(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    auto payload = databases.mutable_maps_payload_bytes();
    const auto decoded =
        openswd3::world_map::decode_legacy_maps_world_database(payload);
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            decoded.status ==
                openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready,
        "real new-game patch test decodes the current MAPS database"
    );
    if (initialized.status !=
            openswd3::resource_io::LegacyResourceDatabaseStatus::ready ||
        maps.status != openswd3::resource_io::LegacyMapsPayloadStatus::ready ||
        decoded.status !=
            openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready) {
        return;
    }

    StoryTestTree tree;
    openswd3::asset_runtime::LegacyActRuntime act_runtime{root};
    openswd3::asset_runtime::LegacyActActionStreamProvider action_provider{
        act_runtime
    };
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        action_provider
    };
    openswd3::world_map::LegacyWorldActionUpdaterInitializer action_initializer{
        action_updater
    };
    auto loaded = openswd3::world_map::load_legacy_world_runtime_session(
        payload,
        openswd3::world_map::LegacyWorldRuntimeSessionRequest{
            .archive_path = root / "huge.lmf",
            .cache_directory = tree.root() / "cache" / "maps",
            .load = decoded.database.initial_load,
            .cache_limit_megabytes = 60U,
            .pixel_conversion = rgb565_conversion(),
        },
        action_initializer
    );
    test.expect_equal(
        loaded.status,
        openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready,
        "real new-game patch test creates the exact initial world session"
    );
    if (loaded.status !=
        openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready) {
        return;
    }

    auto& world = loaded.session;
    auto& map = world.render.map_load.session;
    auto& roles = map.business.state.roles;
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = decoded.database.initial_load.selected_guid;
    context.talk_script_id = 100U;
    openswd3::world_map::LegacyWorldMapRolePathState map_role_paths{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    openswd3::world_map::LegacyWorldCameraPanState camera_pan{};
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_row_effects;
    openswd3::world_map::LegacyRoleHeadActionList role_head_actions;
    u32 battle_request_value{};
    u32 indexed_target_selector{};
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{};
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldPathNodePool path_node_pool;
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = roles,
        .active_object_slots = map_role_paths.active_object_slots,
        .spatial_index = &map.business.state.spatial_index,
        .role_surface =
            {
                .map_width = map.header.width,
                .selected_guid = roles[world.selected_role_index].guid,
                .surface_grid = map.surface_grid.surface_grid,
            },
        .node_pool = &path_node_pool,
        .movement = &movement,
        .camera = &world.camera,
        .selected_arrival_bytes = map_role_paths.guid_one_arrival_bytes,
        .selected_role_index = world.selected_role_index,
        .map_height = map.header.height,
        .scene_render_flags = &scene_render_flags,
    };
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .spatial_index = &map.business.state.spatial_index,
        .role_surface =
            {
                .map_width = map.header.width,
                .selected_guid = roles[world.selected_role_index].guid,
                .surface_grid = map.surface_grid.surface_grid,
            },
        .camera = &world.camera,
        .camera_pan = &camera_pan,
        .movement = &movement,
        .picture_actions = &picture_actions,
        .packed_row_effects = &packed_row_effects,
        .role_head_actions = &role_head_actions,
        .battle_request_value = &battle_request_value,
        .frame_color = &frame_color,
        .story_paths = &story_paths,
        .indexed_target_selector = &indexed_target_selector,
        .scene_render_flags = &scene_render_flags,
        .map_height = map.header.height,
    };
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto step = [&] {
        return openswd3::world_map::step_legacy_world_story_vm(
            context,
            state,
            roles,
            world.selected_role_index,
            map_role_paths.active_object_slots,
            databases.maps_payload_bytes(),
            dialogs,
            dialog_resources,
            first_name,
            second_name,
            runtime,
            ports
        );
    };

    const auto first_clear = step();
    const auto opening_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto opening_video = step();
    auto boundary = opening_video;
    std::size_t boundary_count{};
    for (; boundary_count < 64U && ports.role_patch_requests.size() < 3U;
         ++boundary_count) {
        if (boundary.opcode == 67U) {
            runtime.current_tick = state.wait_started_at +
                static_cast<u32>(state.wait_duration) + 1U;
        } else if (boundary.opcode == 51U) {
            while (openswd3::world_map::advance_legacy_world_camera_pan(
                world.camera, camera_pan
            )) {
            }
        } else if (
            boundary.opcode == 89U ||
            boundary.status != LegacyWorldStoryVmStatus::yielded
        ) {
            break;
        }
        boundary = step();
    }
    const auto missing_patch = ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests.front();
    const auto second_missing_patch = ports.role_patch_requests.size() < 2U
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests[1U];
    const auto position_patch = ports.role_patch_requests.size() < 3U
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests[2U];
    const auto runtime_role = std::ranges::find(
        roles,
        missing_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto second_runtime_role = std::ranges::find(
        roles,
        second_missing_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto position_runtime_role = std::ranges::find(
        roles,
        position_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto source_role = std::ranges::find(
        decoded.database.role_sources,
        missing_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );
    const auto second_source_role = std::ranges::find(
        decoded.database.role_sources,
        second_missing_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );
    const auto position_source_role = std::ranges::find(
        decoded.database.role_sources,
        position_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );

    test.expect_true(
        first_clear.status == LegacyWorldStoryVmStatus::yielded &&
            first_clear.opcode == 61U &&
            opening_wait.status == LegacyWorldStoryVmStatus::yielded &&
            opening_wait.opcode == 67U &&
            opening_video.status == LegacyWorldStoryVmStatus::yielded &&
            opening_video.opcode == 85U,
        "real initial roles execute the opening story through its video boundary"
    );
    test.expect_equal(
        roles.size(), std::size_t{33U}, "real initial world role count"
    );
    test.expect_equal(
        ports.role_patch_requests.size(),
        std::size_t{3U},
        "real opening TALK100 MAPS patch count"
    );
    test.expect_true(
        boundary_count < 64U,
        "real opening TALK100 reaches its MAPS patch boundary"
    );
    test.expect_true(
        boundary.status == LegacyWorldStoryVmStatus::yielded &&
            boundary.opcode == 61U,
        "real opening TALK100 continues past opcode 40"
    );
    test.expect_true(
        runtime_role == roles.end(),
        "first patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        second_runtime_role == roles.end(),
        "second patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        position_runtime_role == roles.end(),
        "position-patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        source_role != decoded.database.role_sources.end(),
        "first patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        second_source_role != decoded.database.role_sources.end(),
        "second patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        position_source_role != decoded.database.role_sources.end(),
        "position-patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        missing_patch.guid == 123U && missing_patch.action_id == 561U &&
            missing_patch.base_variant == 8U &&
            missing_patch.variant_delta == 0U &&
            second_missing_patch.guid == 240U &&
            second_missing_patch.action_id == 561U &&
            second_missing_patch.base_variant == 0U &&
            second_missing_patch.variant_delta == 1U,
        "real TALK100 preserves both unloaded-role action patch operands"
    );
    test.expect_equal(
        missing_patch.flags_or_mask,
        u16{0x1000U},
        "real opening TALK100 role patch OR mask"
    );
    test.expect_equal(
        missing_patch.flags_and_mask,
        u16{0xFFFFU},
        "real opening TALK100 role patch AND mask"
    );
    test.expect_equal(
        missing_patch.logical_map_id,
        u16{0xFFFFU},
        "real opening TALK100 role patch preserves map id"
    );
    test.expect_true(
        second_missing_patch.flags_or_mask == 0x1000U &&
            second_missing_patch.flags_and_mask == 0xFFFFU &&
            second_missing_patch.logical_map_id == 0xFFFFU,
        "second real TALK100 role patch preserves masks and map id"
    );
    test.expect_true(
        position_patch.guid == 195U && position_patch.tile_x == 16U &&
            position_patch.tile_y == 36U &&
            position_patch.flags_or_mask == 0U &&
            position_patch.flags_and_mask == 0xFFFFU &&
            position_patch.logical_map_id == 0xFFFFU,
        "real TALK100 preserves the missing GUID 195 opcode-40 patch"
    );
}

void test_real_new_game_story_reaches_first_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    const auto maps_world =
        openswd3::world_map::decode_legacy_maps_world_database(
            databases.maps_payload_bytes()
        );
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = 1U;
    context.talk_script_id = 100U;

    std::vector<LegacyWorldRoleRecord> roles(11U);
    const auto initialize_role = [&](const std::size_t index,
                                     const u16 guid,
                                     const u32 tile_x,
                                     const u32 tile_y) {
        auto& role = roles[index];
        role.guid = guid;
        role.flags = 0U;
        const auto source = std::ranges::find(
            maps_world.database.role_sources,
            guid,
            &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
        );
        if (source != maps_world.database.role_sources.end()) {
            role.flags |= source->flags;
        }
        role.world_x = tile_x << 4U;
        role.world_y = tile_y << 4U;
        role.map_cell_pointer_32 = tile_y * 80U + tile_x;
        role.action.field_2c = 1U;
        role.action.field_30 = 1U;
    };
    initialize_role(1U, 1U, 2U, 2U);
    initialize_role(2U, 1U, 3U, 3U);
    initialize_role(3U, 123U, 4U, 4U);
    initialize_role(4U, 240U, 5U, 5U);
    initialize_role(5U, 195U, 6U, 6U);
    initialize_role(6U, 248U, 7U, 7U);
    initialize_role(7U, 249U, 8U, 8U);
    initialize_role(8U, 191U, 9U, 9U);
    initialize_role(9U, 250U, 10U, 10U);
    initialize_role(10U, 251U, 11U, 11U);

    openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
    spatial_index.map_height = 80U;
    for (auto& row_heads : spatial_index.row_heads) {
        row_heads.assign(120U, openswd3::world_map::kLegacySpatialNoRole);
    }
    const bool inserted_role_one =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 1U, roles[1U].flags & 3U
        );
    const bool inserted_role_195 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 5U, roles[5U].flags & 3U
        );
    const bool inserted_role_248 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 6U, roles[6U].flags & 3U
        );
    const bool inserted_role_249 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 7U, roles[7U].flags & 3U
        );
    const bool inserted_role_250 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 9U, roles[9U].flags & 3U
        );
    const bool inserted_role_251 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 10U, roles[10U].flags & 3U
        );

    std::vector<u8> surface_grid(80U * 80U * sizeof(u32), 0U);
    openswd3::world_map::LegacyWorldMapRolePathState map_role_paths{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U, 0x2331U};
    constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU, 0x233BU};
    for (std::size_t index = 0U; index < kFrames.size(); ++index) {
        dialog_resources.frame_actions[index].action_id = kFrames[index];
        dialog_resources.caption_actions[index].action_id = kCaptions[index];
    }

    openswd3::world_map::LegacyWorldCameraRect camera{};
    camera.right = 640U;
    camera.bottom = 480U;
    openswd3::world_map::LegacyWorldCameraPanState camera_pan{};
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_row_effects;
    openswd3::world_map::LegacyRoleHeadActionList role_head_actions;
    u32 battle_request_value{};
    u32 indexed_target_selector{};
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{};
    openswd3::rendering::LegacyFramebuffer frame_color_framebuffer;
    openswd3::rendering::LegacyPixelConversionState frame_color_format;
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldPathNodePool path_node_pool;
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = roles,
        .active_object_slots = map_role_paths.active_object_slots,
        .spatial_index = &spatial_index,
        .role_surface =
            {
                .map_width = 80U,
                .selected_guid = 1U,
                .surface_grid = surface_grid,
            },
        .node_pool = &path_node_pool,
        .movement = &movement,
        .camera = &camera,
        .selected_arrival_bytes = map_role_paths.guid_one_arrival_bytes,
        .selected_role_index = 1U,
        .map_height = 80U,
        .scene_render_flags = &scene_render_flags,
    };
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .spatial_index = &spatial_index,
        .role_surface =
            {
                .map_width = 80U,
                .selected_guid = 1U,
                .surface_grid = surface_grid,
            },
        .camera = &camera,
        .camera_pan = &camera_pan,
        .movement = &movement,
        .picture_actions = &picture_actions,
        .packed_row_effects = &packed_row_effects,
        .role_head_actions = &role_head_actions,
        .battle_request_value = &battle_request_value,
        .frame_color = &frame_color,
        .story_paths = &story_paths,
        .indexed_target_selector = &indexed_target_selector,
        .scene_render_flags = &scene_render_flags,
        .map_height = 80U,
    };
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto step = [&] {
        return openswd3::world_map::step_legacy_world_story_vm(
            context,
            state,
            roles,
            1U,
            map_role_paths.active_object_slots,
            databases.maps_payload_bytes(),
            dialogs,
            dialog_resources,
            first_name,
            second_name,
            runtime,
            ports
        );
    };
    StoryFrameActionPorts frame_actions;
    StoryPathCompletionPorts path_completion{story_paths};
    const auto advance_path_frame = [&] {
        return openswd3::world_map::advance_legacy_world_map_role_paths(
            roles,
            spatial_index,
            runtime.role_surface,
            1U,
            scene_render_flags,
            movement,
            camera,
            map_role_paths,
            frame_actions,
            path_completion
        );
    };

    const auto first_clear = step();
    const auto opening_wait = step();
    runtime.current_tick = 2001U;
    const auto opening_video = step();
    const auto branch_clear = step();
    const auto first_scene_wait = step();
    runtime.current_tick += 4501U;
    const auto first_picture = step();
    const auto second_scene_wait = step();
    runtime.current_tick += 7501U;
    const auto second_picture = step();
    const auto third_scene_wait = step();
    runtime.current_tick += 7501U;
    const auto transition_clear = step();
    const auto camera_wait = step();
    while (
        openswd3::world_map::advance_legacy_world_camera_pan(camera, camera_pan)
    ) {
    }
    const auto title = step();
    const auto title_record = dialogs.messages.back().record;
    roles[2].interaction_gate = 0U;
    const auto first_dialog = step();
    const u16 first_dialog_gate = roles[6].interaction_gate;
    const bool first_dialog_has_caption =
        !dialogs.messages.back().caption.empty();
    auto last_dialog = first_dialog;
    for (std::size_t dialog_index = 1U; dialog_index < 10U; ++dialog_index) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        test.expect_equal(
            released_dialog.opcode,
            u16{14U},
            "story 100 dialog release boundary"
        );
        last_dialog = step();
    }
    for (auto& role : roles) {
        role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_last_dialog = step();
    const auto first_path_scene = step();
    const auto first_path_schedule = step();
    auto first_path_wait = first_path_schedule;
    bool first_path_frames_completed = true;
    std::size_t first_path_frame_count{};
    for (; first_path_frame_count < 512U; ++first_path_frame_count) {
        first_path_wait = step();
        if (first_path_wait.opcode == 67U) {
            break;
        }
        const auto advanced = advance_path_frame();
        if (first_path_wait.opcode != 20U ||
            advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
            first_path_frames_completed = false;
            break;
        }
    }
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto second_path_schedule = step();
    auto second_path_wait = second_path_schedule;
    bool second_path_frames_completed = true;
    std::size_t second_path_frame_count{};
    for (; second_path_frame_count < 512U; ++second_path_frame_count) {
        second_path_wait = step();
        if (second_path_wait.opcode == 95U) {
            break;
        }
        const auto advanced = advance_path_frame();
        if (second_path_wait.opcode != 20U ||
            advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
            second_path_frames_completed = false;
            break;
        }
    }
    const auto hidden_scene_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto first_facing_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto next_dialog = step();
    const u32 path_final_facing = roles[1].action.variant_delta;
    auto later_dialog = next_dialog;
    bool later_dialog_chain_completed = true;
    for (std::size_t dialog_index = 0U; dialog_index < 5U; ++dialog_index) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS ||
            released_dialog.status != LegacyWorldStoryVmStatus::yielded) {
            later_dialog_chain_completed = false;
            break;
        }
        later_dialog = step();
        if (later_dialog.opcode != 89U ||
            later_dialog.status != LegacyWorldStoryVmStatus::yielded) {
            later_dialog_chain_completed = false;
            break;
        }
    }
    for (auto& role : roles) {
        role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_later_dialog = step();
    const auto head_sign_assignment = step();
    const u32 head_sign_token = roles[8].field_3c;
    const auto head_sign_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto head_sign_clear = step();
    const auto post_head_sign_dialog = step();
    auto next_unsupported = post_head_sign_dialog;
    std::size_t post_head_sign_dialog_count{};
    bool post_head_sign_dialog_releases_completed = true;
    while (post_head_sign_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            post_head_sign_dialog_releases_completed = false;
            break;
        }
        next_unsupported = step();
        ++post_head_sign_dialog_count;
    }
    std::size_t third_path_frame_count{};
    bool third_path_frames_completed = true;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 20U) {
        for (; third_path_frame_count < 512U; ++third_path_frame_count) {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                third_path_frames_completed = false;
                break;
            }
            next_unsupported = step();
            if (next_unsupported.opcode != 20U) {
                break;
            }
        }
    }
    bool third_path_wait_completed = false;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 67U) {
        runtime.current_tick =
            state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
        next_unsupported = step();
        third_path_wait_completed = true;
    }
    std::size_t third_path_dialog_count{};
    bool third_path_dialog_releases_completed = true;
    while (third_path_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            third_path_dialog_releases_completed = false;
            break;
        }
        next_unsupported = step();
        ++third_path_dialog_count;
    }
    std::size_t final_dialog_count{};
    for (std::size_t head_sign_boundary = 0U; head_sign_boundary < 32U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
         (next_unsupported.opcode == OP_71_SET_ROLE_HEAD_SIGN ||
          next_unsupported.opcode == OP_72_CLEAR_ROLE_HEAD_SIGN ||
          next_unsupported.opcode == OP_67_WAIT_FRAME_CLOCK);
         ++head_sign_boundary) {
        if (next_unsupported.opcode == OP_67_WAIT_FRAME_CLOCK) {
            runtime.current_tick = state.wait_started_at +
                static_cast<u32>(state.wait_duration) + 1U;
        }
        next_unsupported = step();
    }
    while (final_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            break;
        }
        next_unsupported = step();
        for (std::size_t head_sign_boundary = 0U; head_sign_boundary < 16U &&
             next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
             (next_unsupported.opcode == OP_71_SET_ROLE_HEAD_SIGN ||
              next_unsupported.opcode == OP_72_CLEAR_ROLE_HEAD_SIGN ||
              next_unsupported.opcode == OP_67_WAIT_FRAME_CLOCK);
             ++head_sign_boundary) {
            if (next_unsupported.opcode == OP_67_WAIT_FRAME_CLOCK) {
                runtime.current_tick = state.wait_started_at +
                    static_cast<u32>(state.wait_duration) + 1U;
            }
            next_unsupported = step();
        }
        ++final_dialog_count;
    }
    std::size_t fourth_path_frame_count{};
    bool fourth_path_frames_completed = true;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 20U) {
        for (; fourth_path_frame_count < 512U; ++fourth_path_frame_count) {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                fourth_path_frames_completed = false;
                break;
            }
            next_unsupported = step();
            if (next_unsupported.opcode != 20U) {
                break;
            }
        }
    }
    std::size_t post_opcode_45_wait_count{};
    std::size_t post_opcode_45_dialog_count{};
    std::size_t post_opcode_45_path_frame_count{};
    std::size_t post_opcode_45_color_wait_count{};
    bool post_opcode_45_progression_completed = true;
    bool battle_request_submitted = false;
    for (std::size_t boundary_count = 0U; boundary_count < 2048U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded;
         ++boundary_count) {
        switch (next_unsupported.opcode) {
        case 20U: {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_path_frame_count;
            next_unsupported = step();
            break;
        }
        case 51U:
            while (openswd3::world_map::advance_legacy_world_camera_pan(
                camera, camera_pan
            )) {
            }
            next_unsupported = step();
            break;
        case 53U:
            if (const auto advanced =
                    openswd3::rendering::update_legacy_frame_color_transition(
                        frame_color,
                        true,
                        frame_color_framebuffer,
                        frame_color_format
                    );
                advanced.status !=
                openswd3::rendering::LegacyFrameColorTransitionStatus::
                    completed) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_color_wait_count;
            next_unsupported = step();
            break;
        case 67U:
            runtime.current_tick = state.wait_started_at +
                static_cast<u32>(state.wait_duration) + 1U;
            ++post_opcode_45_wait_count;
            next_unsupported = step();
            break;
        case OP_71_SET_ROLE_HEAD_SIGN:
        case OP_72_CLEAR_ROLE_HEAD_SIGN:
            next_unsupported = step();
            break;
        case 89U:
            for (auto& role : roles) {
                role.interaction_gate = 0U;
            }
            context.field_26 = 0U;
            if (const auto released_dialog = step();
                released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
                released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_dialog_count;
            next_unsupported = step();
            break;
        case 88U:
            battle_request_submitted = true;
            break;
        default:
            next_unsupported = step();
            break;
        }
        if (!post_opcode_45_progression_completed || battle_request_submitted) {
            break;
        }
    }
    const std::string video_filename{
        ports.last_video_filename.begin(), ports.last_video_filename.end()
    };
    test.expect_equal(
        first_clear.opcode, u16{61U}, "story 100 first clear boundary"
    );
    test.expect_equal(
        opening_wait.opcode, u16{67U}, "story 100 opening wait boundary"
    );
    test.expect_equal(
        opening_video.opcode, u16{85U}, "story 100 video boundary"
    );
    test.expect_equal(
        branch_clear.opcode, u16{61U}, "story 100 branch clear boundary"
    );
    test.expect_equal(
        first_scene_wait.opcode, u16{67U}, "story 100 first scene wait boundary"
    );
    test.expect_equal(
        first_picture.opcode, u16{153U}, "story 100 first picture boundary"
    );
    test.expect_equal(
        second_scene_wait.opcode,
        u16{67U},
        "story 100 second scene wait boundary"
    );
    test.expect_equal(
        second_picture.opcode, u16{153U}, "story 100 second picture boundary"
    );
    test.expect_equal(
        third_scene_wait.opcode, u16{67U}, "story 100 third scene wait boundary"
    );
    test.expect_equal(
        transition_clear.opcode, u16{60U}, "story 100 transition clear boundary"
    );
    test.expect_equal(
        camera_wait.opcode, u16{51U}, "story 100 camera wait boundary"
    );
    test.expect_equal(title.opcode, u16{6U}, "story 100 title boundary");
    test.expect_equal(
        first_dialog.opcode, u16{89U}, "story 100 first spoken dialog boundary"
    );
    test.expect_equal(
        last_dialog.opcode, u16{89U}, "story 100 tenth spoken dialog boundary"
    );
    test.expect_equal(
        released_last_dialog.opcode,
        u16{14U},
        "story 100 last dialog release boundary"
    );
    test.expect_equal(
        first_path_scene.opcode, u16{94U}, "story 100 first path scene boundary"
    );
    test.expect_equal(
        first_path_schedule.opcode,
        u16{20U},
        "story 100 first path schedule boundary"
    );
    test.expect_equal(
        first_path_wait.opcode,
        u16{67U},
        "story 100 first path completion boundary"
    );
    test.expect_equal(
        second_path_schedule.opcode,
        u16{20U},
        "story 100 second path schedule boundary"
    );
    test.expect_equal(
        second_path_wait.opcode,
        u16{95U},
        "story 100 second path completion boundary"
    );
    test.expect_equal(
        hidden_scene_wait.opcode,
        u16{67U},
        "story 100 hidden-scene wait boundary"
    );
    test.expect_equal(
        first_facing_wait.opcode,
        u16{67U},
        "story 100 first facing wait boundary"
    );
    test.expect_equal(
        next_dialog.opcode, u16{89U}, "story 100 next dialog boundary"
    );
    test.expect_equal(
        later_dialog.opcode,
        u16{89U},
        "story 100 fifth post-path dialog boundary"
    );
    test.expect_equal(
        released_later_dialog.opcode,
        u16{14U},
        "story 100 fifth post-path dialog release boundary"
    );
    test.expect_equal(
        head_sign_assignment.opcode,
        OP_71_SET_ROLE_HEAD_SIGN,
        "story 100 head-sign assignment boundary"
    );
    test.expect_equal(
        head_sign_wait.opcode, u16{67U}, "story 100 head-sign wait boundary"
    );
    test.expect_equal(
        head_sign_clear.opcode,
        OP_72_CLEAR_ROLE_HEAD_SIGN,
        "story 100 head-sign clear boundary"
    );
    test.expect_equal(
        post_head_sign_dialog.opcode,
        u16{89U},
        "story 100 post-head-sign dialog boundary"
    );
    test.expect_equal(
        post_head_sign_dialog_count,
        std::size_t{11U},
        "story 100 post-head-sign dialog count"
    );
    test.expect_equal(
        third_path_frame_count,
        std::size_t{46U},
        "story 100 third path frame count"
    );
    test.expect_equal(
        third_path_dialog_count,
        std::size_t{5U},
        "story 100 third path dialog count"
    );
    test.expect_equal(
        final_dialog_count, std::size_t{7U}, "story 100 final dialog count"
    );
    test.expect_equal(
        fourth_path_frame_count,
        std::size_t{40U},
        "story 100 fourth path frame count"
    );
    test.expect_equal(
        post_opcode_45_wait_count,
        std::size_t{8U},
        "story 100 post-opcode-45 wait count"
    );
    test.expect_equal(
        post_opcode_45_dialog_count,
        std::size_t{8U},
        "story 100 post-opcode-45 dialog count"
    );
    test.expect_equal(
        post_opcode_45_path_frame_count,
        std::size_t{65U},
        "story 100 post-opcode-45 path frame count"
    );
    test.expect_equal(
        post_opcode_45_color_wait_count,
        std::size_t{1U},
        "story 100 post-opcode-45 color wait count"
    );
    test.expect_equal(
        next_unsupported.opcode, u16{88U}, "story 100 battle request boundary"
    );
    test.expect_equal(
        next_unsupported.status,
        LegacyWorldStoryVmStatus::yielded,
        "story 100 yields after consuming opcode 88"
    );
    test.expect_equal(
        state.loaded_data_offset,
        u32{17476U},
        "story 100 branched window base through both paths"
    );
    test.expect_equal(
        context.instruction_offset,
        u16{3847U},
        "story 100 instruction boundary after opcode 88"
    );
    test.expect_equal(
        battle_request_value,
        u32{0x80000062U},
        "story 100 submits battle id 98 with the request tag"
    );
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            maps_world.status ==
                openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready &&
            inserted_role_one && inserted_role_195 && inserted_role_248 &&
            inserted_role_249 && inserted_role_250 && inserted_role_251,
        "real story 100 fixture uses the real databases and valid spatial roles"
    );
    test.expect_true(
        first_clear.opcode == 61U && opening_wait.opcode == 67U &&
            opening_video.opcode == 85U && branch_clear.opcode == 61U &&
            first_scene_wait.opcode == 67U && first_picture.opcode == 153U &&
            second_scene_wait.opcode == 67U && second_picture.opcode == 153U &&
            third_scene_wait.opcode == 67U && transition_clear.opcode == 60U &&
            camera_wait.opcode == 51U && title.opcode == 6U &&
            title.status == LegacyWorldStoryVmStatus::yielded &&
            title_record.flags == 0x00040468U &&
            title_record.lifetime_limit == 20U && title_record.left == 20U &&
            title_record.top == 20U && title_record.width == 132U &&
            title_record.height == 22U && first_dialog.opcode == 89U &&
            first_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            first_dialog_has_caption && first_dialog_gate == 1U &&
            first_path_frames_completed && second_path_frames_completed &&
            first_path_frame_count < 512U && second_path_frame_count < 512U &&
            next_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            later_dialog_chain_completed &&
            head_sign_assignment.status == LegacyWorldStoryVmStatus::yielded &&
            head_sign_wait.status == LegacyWorldStoryVmStatus::yielded &&
            head_sign_clear.status == LegacyWorldStoryVmStatus::yielded &&
            head_sign_token ==
                openswd3::world_map::legacy_world_head_sign_action_token(0U) &&
            post_head_sign_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            roles[8].field_3c == 0U &&
            post_head_sign_dialog_releases_completed &&
            third_path_frames_completed && third_path_wait_completed &&
            third_path_dialog_releases_completed &&
            fourth_path_frames_completed &&
            post_opcode_45_progression_completed && battle_request_submitted &&
            dialogs.messages.size() == 48U &&
            (roles[9].flags & 0x00008000U) != 0U &&
            (roles[10].flags & 0x00008000U) != 0U &&
            roles[9].action.base_variant == 33U &&
            roles[10].action.base_variant == 0U &&
            roles[9].action.wait_override == 0x8002U &&
            roles[10].action.wait_override == 0x8000U &&
            roles[9].action.wait_remaining == 0U &&
            roles[10].action.wait_remaining == 0U && roles[10].field_3c == 0U,
        "real story 100 crosses opcode 45 and all subsequent restored waits, " "dialogs, paths, color transition control, role-path release, primary " "picture enqueue and explicit text layout through the opcode 88 battle " "request"
    );
    test.expect_true(
        ports.sound_effect_requests == std::vector<u16>{0x73U, 0x3CU},
        "real story 100 submits both scripted opcode 59 sound ids"
    );
    test.expect_equal(
        roles[6].world_x, u32{37U * 16U}, "real story 100 relocates role 248 x"
    );
    test.expect_equal(
        roles[6].world_y, u32{33U * 16U}, "real story 100 relocates role 248 y"
    );
    test.expect_equal(
        roles[7].world_x, u32{39U * 16U}, "real story 100 relocates role 249 x"
    );
    test.expect_equal(
        roles[7].world_y, u32{33U * 16U}, "real story 100 relocates role 249 y"
    );
    test.expect_equal(
        roles[1].world_x,
        u32{13U * 16U},
        "real story 100 completes first clear GUID 1 path x"
    );
    test.expect_equal(
        roles[1].world_y,
        u32{28U * 16U},
        "real story 100 completes first clear GUID 1 path y"
    );
    test.expect_equal(
        roles[1].action.base_variant,
        u32{68U},
        "real story 100 applies the later opcode 10 base variant"
    );
    test.expect_equal(
        path_final_facing,
        u32{7U},
        "real story 100 applies the final opcode 11 facing"
    );
    test.expect_equal(
        roles[5].world_x, u32{16U * 16U}, "real story 100 relocates role 195 x"
    );
    test.expect_equal(
        roles[5].world_y, u32{36U * 16U}, "real story 100 relocates role 195 y"
    );
    test.expect_true(
        picture_actions.primary.size() == 1U &&
            picture_actions.secondary.size() == 2U &&
            frame_color.current_red == 10.0F &&
            frame_color.current_green == 10.0F &&
            frame_color.current_blue == 10.0F &&
            frame_color.target_red == 10.0F &&
            frame_color.target_green == 10.0F &&
            frame_color.target_blue == 10.0F && frame_color.step_red == 0.0F &&
            frame_color.step_green == 0.0F && frame_color.step_blue == 0.0F &&
            frame_color.countdown == 0 && scene_render_flags == 0U &&
            (roles[1].flags & 0x00001000U) != 0U,
        "real story 100 creates two pictures then completes and cancels the " "later color transition"
    );
    test.expect_true(
        ports.framebuffer_clear_count == 3U &&
            ports.framebuffer_present_count == 1U &&
            ports.video_begin_count == 1U &&
            ports.video_progress_query_count == 1U &&
            video_filename == "OPENING.bik",
        "real story 100 preserves its framebuffer and video side effects"
    );
}
