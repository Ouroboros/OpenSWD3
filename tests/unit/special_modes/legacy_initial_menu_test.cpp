#include "openswd3/special_modes/legacy_initial_menu.hpp"
#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::special_modes::initialize_legacy_initial_menu;
using openswd3::special_modes::initialize_legacy_standard_mode_selector;
using openswd3::special_modes::initialize_legacy_standard_special_modes;
using openswd3::special_modes::kLegacyInitialMenuCommitCounter;
using openswd3::special_modes::kLegacyInitialMenuEntryCounter;
using openswd3::special_modes::kLegacyInitialMenuExitCounter;
using openswd3::special_modes::kLegacyInitialMenuNameOneCounter;
using openswd3::special_modes::kLegacyInitialMenuNameTwoCounter;
using openswd3::special_modes::LegacyInitialMenuEvent;
using openswd3::special_modes::LegacyInitialMenuInput;
using openswd3::special_modes::LegacyInitialMenuState;
using openswd3::special_modes::run_legacy_initial_menu_frame;
using openswd3::special_modes::run_legacy_standard_special_mode_frame;
using openswd3::special_modes::kLegacySpecialModeAlternateFlag;
using openswd3::special_modes::kLegacySpecialModeInitializeFlag;
using openswd3::special_modes::LegacyLowSpecialModeInitialization;
using openswd3::special_modes::LegacyModeThreeSixRecordInitialization;
using openswd3::special_modes::LegacyStandardModeSelectorPorts;
using openswd3::special_modes::LegacyStandardModeSelectorState;
using openswd3::special_modes::LegacyStandardSpecialModeInitializationPorts;
using openswd3::special_modes::LegacyStandardSpecialModePorts;
using openswd3::special_modes::LegacyStandardSpecialModeState;

struct DrawCall {
    i32 x{};
    i32 y{};
    u32 flags{};
    i32 red_offset{};
};

class FakeActionPorts final : public LegacyActionDrawPorts {
public:
    explicit FakeActionPorts(LegacyBlitEffectState& effects) noexcept
        : effects_(effects) {}

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        ++update_count;
        record.field_4a = static_cast<u16>(record.action_id);
        record.field_4c = static_cast<u16>(record.base_variant);
        record.draw_offset_x = 2U;
        record.draw_offset_y = 3U;
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        piece.width = 16U;
        piece.height = 32U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&,
        const i32 destination_x,
        const i32 destination_y,
        const u32 flags,
        const i32
    ) noexcept override {
        draws.push_back(
            DrawCall{
                .x = destination_x,
                .y = destination_y,
                .flags = flags,
                .red_offset = effects_.red_offset,
            }
        );
        return LegacyBlitExecutionStatus::completed;
    }

    LegacyBlitEffectState& effects_;
    u32 update_count{};
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
};

class FakeStandardModeInitializationPorts final
    : public LegacyStandardSpecialModeInitializationPorts {
public:
    explicit FakeStandardModeInitializationPorts(
        LegacyStandardSpecialModeState& state
    ) noexcept
        : state_(state) {}

    void install_mode_callbacks() override {
        events.push_back(1U);
    }

    i32 story_flag(const u32 flag_index) override {
        events.push_back(2U);
        queried_flag = flag_index;
        const auto& records = state_.initialization_records;
        query_saw_exact_prefix = records[0U].action_id == 0x232AU &&
            records[0U].base_variant == 0U &&
            records[2U].action_id == 0x232AU &&
            records[2U].base_variant == 1U &&
            records[1U].action_id == 0x232AU &&
            records[1U].base_variant == 2U &&
            records[3U].action_id == 0xDEAD0003U;
        return story_flag_value;
    }

    LegacyStandardSpecialModeState& state_;
    std::vector<u32> events;
    i32 story_flag_value{};
    u32 queried_flag{};
    bool query_saw_exact_prefix{};
};

class FakeStandardModeSelectorPorts final
    : public LegacyStandardModeSelectorPorts {
public:
    explicit FakeStandardModeSelectorPorts(
        LegacyStandardModeSelectorState& state
    ) noexcept
        : state_(state) {
        input_words.fill(0xFFFFFFFFU);
    }

    void bind_mode_callbacks(const u16 selector) override {
        events.push_back(1U);
        callback_selector = selector;
        bind_saw_header = state_.selector == 2U &&
            state_.derived_index == 0x2716U && state_.item_count == 5U &&
            state_.resource_ids ==
                std::array<u16, 3>{0xEA60U, 0xEA60U, 0xEA60U} &&
            state_.mode_value == 0xDEADBEEFU;
    }

    void establish_item_state(const u16 item_count) override {
        events.push_back(2U);
        established_item_count = item_count;
    }

    void clear_mode_input_records() override {
        events.push_back(3U);
        clear_saw_preceding_state = state_.mode_value == 0xDEADBEEFU;
        input_words.fill(0U);
    }

    u32 create_shared_input_token(
        const u32 first, const u32 second, const u32 third
    ) override {
        events.push_back(4U);
        token_arguments = {first, second, third};
        create_saw_mode_clear = state_.mode_value == 0U;
        return shared_token;
    }

    void publish_input_token(
        const std::size_t owner_index, const u32 token
    ) override {
        events.push_back(static_cast<u32>(10U + owner_index));
        token_owners.push_back(owner_index);
        published_tokens.push_back(token);
    }

    i16 publish_input_sentinel(
        const std::size_t owner_index, const u16 sentinel
    ) override {
        events.push_back(static_cast<u32>(20U + owner_index));
        sentinel_owners.push_back(owner_index);
        published_sentinels.push_back(sentinel);
        return static_cast<i16>(0x120U + owner_index);
    }

    LegacyStandardModeSelectorState& state_;
    std::array<u32, 0x80U> input_words{};
    std::vector<u32> events;
    std::vector<std::size_t> token_owners;
    std::vector<std::size_t> sentinel_owners;
    std::vector<u32> published_tokens;
    std::vector<u16> published_sentinels;
    std::array<u32, 3U> token_arguments{};
    u32 shared_token{0xCAFEBABEU};
    u16 callback_selector{};
    u16 established_item_count{};
    bool bind_saw_header{};
    bool clear_saw_preceding_state{};
    bool create_saw_mode_clear{};
};

class FakeStandardModePorts final : public LegacyStandardSpecialModePorts {
public:
    void initialize_low_mode(
        const LegacyLowSpecialModeInitialization& initialization
    ) override {
        events.push_back(1U);
        low_initialization = initialization;
        alternate = initialization.install_alternate_callback;
    }

    void reset_mode_records() override {
        events.push_back(2U);
    }

    void initialize_mode_3_or_6_records(
        const LegacyModeThreeSixRecordInitialization& initialization
    ) override {
        events.push_back(8U);
        mode_three_six_initialization = initialization;
    }

    void initialize_mode_selector(
        const u32 selected, const u32 selected_resource
    ) override {
        events.push_back(3U);
        selector = selected;
        resource_id = selected_resource;
    }

    void play_entry_sound(const u16 selected_sound) override {
        events.push_back(4U);
        sound_id = selected_sound;
    }

    void update_mode_objects() override {
        events.push_back(5U);
    }

    void process_mode_input(u32& tagged_mode_value) override {
        events.push_back(6U);
        if (clear_during_input) {
            tagged_mode_value = 0U;
        }
    }

    void draw_mode(u32& tagged_mode_value) override {
        events.push_back(7U);
        if (clear_during_draw) {
            tagged_mode_value = 0U;
        }
    }

    std::vector<u32> events;
    LegacyLowSpecialModeInitialization low_initialization;
    LegacyModeThreeSixRecordInitialization mode_three_six_initialization;
    u32 selector{};
    u32 resource_id{};
    u16 sound_id{};
    bool alternate{};
    bool clear_during_input{};
    bool clear_during_draw{};
};

[[nodiscard]] LegacyInputRecord pressed() noexcept {
    return LegacyInputRecord{
        .rapid_press_multiplicity = 1U,
        .held_sample_count = 1U,
    };
}

void test_initialization_and_entry_counter(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    const auto first = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    );
    test.expect_true(
        state.initialized && state.phase == 0 &&
            state.counter == kLegacyInitialMenuEntryCounter + 1 &&
            state.selected_choice == 0U &&
            state.background_action.action_id == 0x232AU &&
            state.background_action.base_variant == 0x4EU &&
            state.choice_actions[0].action_id == 0x232BU &&
            state.choice_actions[0].base_variant == 0x2CU &&
            state.choice_actions[3].base_variant == 0x2FU &&
            first.event == LegacyInitialMenuEvent::none,
        "normal mode 3 initializes the exact action keys and -10 counter"
    );

    for (i32 frame = 1; frame < -kLegacyInitialMenuEntryCounter; ++frame) {
        static_cast<void>(run_legacy_initial_menu_frame(
            state, LegacyInitialMenuInput{}, ports, effects
        ));
    }
    test.expect_true(
        state.phase == 1 && state.counter == 0 &&
            state.slide_offsets[0] == -30 && state.slide_offsets[1] == -12,
        "the 10th terminal callback enters phase one after slide animation"
    );
}

void test_strict_hitbox_and_new_game_submit(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x80,
            .mouse_y = 0x107,
            .mouse_button_mask = 1U,
        },
        ports,
        effects
    ));
    test.expect_true(
        state.phase == 1 && state.selected_choice == 0U,
        "y equal to 0x107 is excluded by the second strict hit box"
    );

    const auto submitted = run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x80,
            .mouse_y = 0x108,
            .mouse_button_mask = 1U,
        },
        ports,
        effects
    );
    test.expect_true(
        submitted.event == LegacyInitialMenuEvent::none && state.phase == 2 &&
            state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter &&
            state.name_input.has_value() && state.name_input->x() == 0x12C &&
            state.name_input->y() == 0xE6 && state.first_name[0] == 0xC1U &&
            state.first_name[3] == 0x53U && state.second_name[0] == 0xA9U &&
            state.second_name[3] == 0x69U &&
            ports.loads[ports.loads.size() - 2U] ==
                std::pair<u16, u16>{0x2449U, 0U},
        "choice one creates the 8-byte name field and draws its 0x2449 panel"
    );
}

void test_keyboard_name_gate_and_commit(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord down = pressed();
    LegacyInputRecord primary = pressed();

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.down = &down}, ports, effects
    ));
    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_true(
        state.phase == 2 && state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter,
        "direction and primary callbacks select new game without bypassing " "phase two"
    );

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameTwoCounter,
        "first confirmation advances to the second 0x20-byte input object"
    );
    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuExitCounter,
        "second confirmation returns before the exit fade increment"
    );

    LegacyInitialMenuEvent event = LegacyInitialMenuEvent::none;
    for (i32 counter = kLegacyInitialMenuExitCounter;
         counter < kLegacyInitialMenuCommitCounter;
         ++counter) {
        event = run_legacy_initial_menu_frame(
                    state, LegacyInitialMenuInput{}, ports, effects
        )
                    .event;
    }
    test.expect_true(
        state.counter == kLegacyInitialMenuCommitCounter &&
            event == LegacyInitialMenuEvent::commit_new_game_004492ba,
        "only counter 105 emits the assembly new-game commit event"
    );
}

void test_name_cancel_returns_to_selection(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameTwoCounter;
    state.selected_choice = 1U;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord cancel = pressed();

    const auto result = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.cancel = &cancel}, ports, effects
    );
    test.expect_true(
        result.event == LegacyInitialMenuEvent::none && state.phase == 1 &&
            state.counter == kLegacyInitialMenuNameTwoCounter,
        "cancel destroys the active name object and restores phase one"
    );
}

void test_name_mouse_accept_uses_recovered_axes(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameOneCounter;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord mouse_left = pressed();

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x108,
            .mouse_y = 0x170,
            .mouse_left = &mouse_left,
        },
        ports,
        effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameOneCounter,
        "the old transposed name-button coordinates are rejected"
    );

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x170,
            .mouse_y = 0x108,
            .mouse_left = &mouse_left,
        },
        ports,
        effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameTwoCounter,
        "the name button accepts x 0x162..0x198 and y 0x101..0x112"
    );
}

void test_text_object_result_and_edited_name(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameOneCounter;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    ));
    auto view = state.name_input->borrow_edit_view();
    view.bytes[0] = 0x41U;
    view.bytes[1] = 0U;
    *view.result = 1;

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    ));
    test.expect_true(
        state.counter == kLegacyInitialMenuNameTwoCounter &&
            state.first_name[0] == 0x41U && state.first_name[1] == 0U &&
            state.name_input.has_value(),
        "text result one commits the edit and creates the second input object"
    );
}

void test_standard_mode_global_initialization(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState state{.transient_flags = 0xFFFFFFFFU};
    for (std::size_t index = 0U; index < state.initialization_records.size();
         ++index) {
        auto& record = state.initialization_records[index];
        record.action_id = 0xDEAD0000U + static_cast<u32>(index);
        record.base_variant = 0xBEEF0000U + static_cast<u32>(index);
        record.field_1c = 0U;
        record.one_shot_base_variant = 0U;
        record.one_shot_variant_delta = 0U;
        record.wait_override = 0xFFFFU;
        record.wait_default = 0xFFFFU;
        record.wait_remaining = 0xFFFFU;
        record.command_cursor = 0xFFFFU;
        record.external_mode = 0xFFFFFFFFU;
    }

    FakeStandardModeInitializationPorts ports{state};
    ports.story_flag_value = 1;
    const auto result = initialize_legacy_standard_special_modes(state, ports);
    const std::array<u32, 18U> expected_action_ids{
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0xDEAD000AU,
        0x232BU,
        0x232BU,
        0x232BU,
        0x232BU,
        0xDEAD000FU,
        0x232AU,
        0x233BU,
    };
    const std::array<u32, 18U> expected_base_variants{
        0U,
        3U,
        1U,
        4U,
        5U,
        6U,
        0x18U,
        0x19U,
        0x1AU,
        0x1BU,
        0xBEEF000AU,
        0x2CU,
        0x2DU,
        0x2EU,
        0x2FU,
        0xBEEF000FU,
        3U,
        0U,
    };
    bool records_match = true;
    for (std::size_t index = 0U; index < state.initialization_records.size();
         ++index) {
        const auto& record = state.initialization_records[index];
        records_match = records_match &&
            record.action_id == expected_action_ids[index] &&
            record.base_variant == expected_base_variants[index] &&
            record.field_1c == 0xFFFFFFFFU &&
            record.one_shot_base_variant == 0xFFFFFFFFU &&
            record.one_shot_variant_delta == 0xFFFFFFFFU &&
            record.wait_override == 0U && record.wait_default == 0U &&
            record.wait_remaining == 0U && record.command_cursor == 0U &&
            record.external_mode == 0U;
    }
    test.expect_true(
        records_match && state.transient_flags == 0U &&
            ports.events == std::vector<u32>{1U, 2U} &&
            ports.queried_flag == 0x49U && ports.query_saw_exact_prefix &&
            result.action_record_initialization_count == 18U &&
            result.callback_installation_count == 1U &&
            result.story_flag_query_count == 1U &&
            result.return_value == 0x232BU,
        "0x439DE0 installs callbacks, initializes eighteen records in " "address order and applies story flag 0x49 after the three-record " "prefix"
    );

    LegacyStandardSpecialModeState non_one_state;
    FakeStandardModeInitializationPorts non_one_ports{non_one_state};
    non_one_ports.story_flag_value = 2;
    static_cast<void>(
        initialize_legacy_standard_special_modes(non_one_state, non_one_ports)
    );
    test.expect_equal(
        non_one_state.initialization_records[1U].base_variant,
        2U,
        "only a story-flag result equal to one selects variant three"
    );
}

void test_standard_mode_selector_initialization(openswd3::test::Context& test) {
    LegacyStandardModeSelectorState state{.mode_value = 0xDEADBEEFU};
    FakeStandardModeSelectorPorts ports{state};
    const auto result = initialize_legacy_standard_mode_selector(
        state, 0x0000EA60, 0x00010002U, ports
    );
    test.expect_true(
        state.selector == 2U && state.derived_index == 0x2716U &&
            state.item_count == 5U &&
            state.resource_ids ==
                std::array<u16, 3U>{0xEA60U, 0xEA60U, 0xEA60U} &&
            state.mode_value == 0U && ports.callback_selector == 2U &&
            ports.established_item_count == 5U && ports.bind_saw_header &&
            ports.clear_saw_preceding_state && ports.create_saw_mode_clear &&
            ports.input_words == std::array<u32, 0x80U>{} &&
            ports.token_arguments == std::array<u32, 3U>{6U, 4U, 3U} &&
            ports.token_owners == std::vector<std::size_t>{0U, 1U, 2U} &&
            ports.published_tokens ==
                std::vector<u32>{0xCAFEBABEU, 0xCAFEBABEU, 0xCAFEBABEU} &&
            ports.sentinel_owners == std::vector<std::size_t>{0U, 1U, 2U} &&
            ports.published_sentinels ==
                std::vector<u16>{0xFFFEU, 0xFFFEU, 0xFFFEU} &&
            ports.events ==
                std::vector<u32>{
                    1U, 2U, 3U, 4U, 10U, 11U, 12U, 20U, 21U, 22U
                } &&
            result.callback_bind_count == 1U && result.item_state_count == 1U &&
            result.input_clear_count == 1U &&
            result.token_publish_count == 3U &&
            result.sentinel_publish_count == 3U && result.return_value == 0x122,
        "0x43A2A0 preserves the selector header, 0x200-byte input clear " "and three-owner token then sentinel order"
    );

    LegacyStandardModeSelectorState signed_state;
    FakeStandardModeSelectorPorts signed_ports{signed_state};
    static_cast<void>(initialize_legacy_standard_mode_selector(
        signed_state, 0x17, 0xFFFF0003U, signed_ports
    ));
    test.expect_true(
        signed_state.selector == 3U && signed_state.derived_index == 10U &&
            signed_state.resource_ids ==
                std::array<u16, 3U>{0x17U, 0x17U, 0x17U},
        "the x86 signed division truncates negative resource deltas toward zero"
    );
}

void test_standard_mode_entry_and_common_order(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState low_state{
        .frame_counter = 999U,
        .transient_flags = 0xFFFFFFFFU,
        .entry_zero_a = 7U,
        .entry_zero_b = 8U,
        .entry_gate = 9U,
    };
    FakeStandardModePorts low_ports;
    u32 low_mode =
        kLegacySpecialModeInitializeFlag | kLegacySpecialModeAlternateFlag | 1U;
    const auto low_result =
        run_legacy_standard_special_mode_frame(low_state, low_mode, low_ports);
    test.expect_true(
        low_mode == 1U && low_state.frame_counter == 0x41U &&
            low_state.transient_flags == 0U && low_state.entry_zero_a == 0U &&
            low_state.entry_zero_b == 0U && low_state.entry_gate == 1U &&
            low_ports.alternate && low_ports.sound_id == 0x00BBU &&
            low_ports.low_initialization.primary_action_id == 0x232AU &&
            low_ports.low_initialization.primary_base_variant == 0x34U &&
            low_ports.low_initialization.secondary_base_variants ==
                std::array<u32, 2>{0x1AU, 0x1BU} &&
            low_ports.low_initialization.choice_base_variants ==
                std::array<u32, 4>{8U, 9U, 10U, 11U} &&
            low_ports.low_initialization.selection_word == 1U &&
            low_ports.low_initialization.setup_resource_id == 0x24U &&
            low_ports.low_initialization.setup_selector == 2U &&
            low_ports.events == std::vector<u32>{1U, 4U, 5U, 6U, 7U} &&
            low_result.effective_mode == 1U &&
            low_result.initialization_count == 1U &&
            low_result.update_count == 1U && low_result.input_count == 1U &&
            low_result.draw_count == 1U,
        "0x439FD0 consumes both entry bits after alternate low-mode setup " "and preserves update-input-draw order"
    );

    LegacyStandardSpecialModeState normal_low_state;
    FakeStandardModePorts normal_low_ports;
    u32 normal_low_mode = kLegacySpecialModeInitializeFlag | 2U;
    static_cast<void>(run_legacy_standard_special_mode_frame(
        normal_low_state, normal_low_mode, normal_low_ports
    ));
    test.expect_true(
        normal_low_mode == 2U && !normal_low_ports.alternate &&
            normal_low_ports.low_initialization.selection_word == 0U &&
            normal_low_ports.low_initialization.setup_resource_id == 0x1EU &&
            normal_low_ports.low_initialization.setup_selector == 1U &&
            normal_low_ports.events == std::vector<u32>{1U, 4U, 5U, 6U, 7U},
        "the bit-30-clear low-mode branch uses the normal setup variant"
    );

    for (const auto [mode, selector] : std::array<std::pair<u32, u32>, 4>{
             std::pair<u32, u32>{3U, 0U},
             std::pair<u32, u32>{4U, 1U},
             std::pair<u32, u32>{5U, 2U},
             std::pair<u32, u32>{6U, 3U},
         }) {
        LegacyStandardSpecialModeState state;
        FakeStandardModePorts ports;
        u32 tagged_mode = kLegacySpecialModeInitializeFlag | 0x20000000U | mode;
        const auto result =
            run_legacy_standard_special_mode_frame(state, tagged_mode, ports);
        const bool initializes_shared_records = mode == 3U || mode == 6U;
        const auto expected_events = initializes_shared_records
            ? std::vector<u32>{2U, 8U, 3U, 5U, 6U, 7U}
            : std::vector<u32>{2U, 3U, 5U, 6U, 7U};
        test.expect_true(
            tagged_mode == (0x20000000U | mode) &&
                state.frame_counter == 0x41U && ports.selector == selector &&
                ports.resource_id == 0x0000EA60U &&
                (!initializes_shared_records ||
                 (ports.mode_three_six_initialization.primary_base_variant ==
                      0x4EU &&
                  ports.mode_three_six_initialization.choice_action_ids ==
                      std::array<u32, 4>{0x232BU, 0x232BU, 0x232BU, 0x232BU} &&
                  ports.mode_three_six_initialization.choice_base_variants ==
                      std::array<u32, 4>{0x2CU, 0x2DU, 0x2EU, 0x2FU})) &&
                ports.events == expected_events &&
                result.initialization_count == 1U,
            "modes 3 through 6 keep bit 29 and map to selectors zero " "through three"
        );
    }
}

void test_standard_mode_exit_paths(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState input_exit_state{
        .frame_counter = 0xFFFFFFFFU,
        .transient_flags = 3U,
    };
    FakeStandardModePorts input_exit_ports;
    input_exit_ports.clear_during_input = true;
    u32 input_exit_mode = 3U;
    const auto input_exit = run_legacy_standard_special_mode_frame(
        input_exit_state, input_exit_mode, input_exit_ports
    );
    test.expect_true(
        input_exit_state.frame_counter == 0U &&
            input_exit_state.transient_flags == 1U && input_exit_mode == 0U &&
            input_exit_ports.events == std::vector<u32>{5U, 6U} &&
            input_exit.draw_count == 0U,
        "an input-side mode clear skips drawing, clears transient bit one, " "and keeps u32 frame wrap"
    );

    LegacyStandardSpecialModeState draw_exit_state{.transient_flags = 3U};
    FakeStandardModePorts draw_exit_ports;
    draw_exit_ports.clear_during_draw = true;
    u32 draw_exit_mode = 4U;
    const auto draw_exit = run_legacy_standard_special_mode_frame(
        draw_exit_state, draw_exit_mode, draw_exit_ports
    );
    test.expect_true(
        draw_exit_mode == 0U && draw_exit_state.transient_flags == 1U &&
            draw_exit_ports.events == std::vector<u32>{5U, 6U, 7U} &&
            draw_exit.draw_count == 1U,
        "a draw-side mode clear still records the draw and then clears " "transient bit one"
    );

    LegacyStandardSpecialModeState unsupported_state;
    FakeStandardModePorts unsupported_ports;
    u32 unsupported_mode = kLegacySpecialModeInitializeFlag | 0x20000000U | 7U;
    const auto unsupported = run_legacy_standard_special_mode_frame(
        unsupported_state, unsupported_mode, unsupported_ports
    );
    test.expect_true(
        unsupported_mode == 0x20000007U &&
            unsupported_ports.events == std::vector<u32>{5U, 6U, 7U} &&
            unsupported.initialization_count == 0U,
        "an out-of-switch entry still consumes the high initializer and " "runs the common frame tail"
    );
}

void test_real_draw_contract(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    state.selected_choice = 1U;
    state.slide_offsets = {-12, -32, -12, -12};
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    const auto result = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    );
    test.expect_true(
        result.action_update_count == 5U && result.frame_request_count == 5U &&
            result.draw_count == 41U &&
            ports.loads.front() == std::pair<u16, u16>{0x232AU, 0x4EU} &&
            ports.loads[2] == std::pair<u16, u16>{0x232BU, 0x2DU} &&
            ports.draws.front().flags == 0U && ports.draws[1].x == 0x7B &&
            ports.draws[1].y == 0xCF && ports.draws[2].x == 0x7B &&
            ports.draws[2].y == 0x104 && ports.draws[2].flags == 4U,
        "the four 0x232B choices use common x and the recovered vertical " "anchors"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initialization_and_entry_counter(test);
    test_strict_hitbox_and_new_game_submit(test);
    test_keyboard_name_gate_and_commit(test);
    test_name_cancel_returns_to_selection(test);
    test_name_mouse_accept_uses_recovered_axes(test);
    test_text_object_result_and_edited_name(test);
    test_standard_mode_global_initialization(test);
    test_standard_mode_selector_initialization(test);
    test_standard_mode_entry_and_common_order(test);
    test_standard_mode_exit_paths(test);
    test_real_draw_contract(test);
    return test.exit_code();
}
