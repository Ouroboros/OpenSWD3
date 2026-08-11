#include "openswd3/special_modes/legacy_initial_menu.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::special_modes::initialize_legacy_initial_menu;
using openswd3::special_modes::kLegacyInitialMenuCommitCounter;
using openswd3::special_modes::kLegacyInitialMenuEntryCounter;
using openswd3::special_modes::kLegacyInitialMenuExitCounter;
using openswd3::special_modes::kLegacyInitialMenuNameOneCounter;
using openswd3::special_modes::kLegacyInitialMenuNameTwoCounter;
using openswd3::special_modes::LegacyInitialMenuEvent;
using openswd3::special_modes::LegacyInitialMenuInput;
using openswd3::special_modes::LegacyInitialMenuState;
using openswd3::special_modes::run_legacy_initial_menu_frame;

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

    [[nodiscard]] LegacyActionUpdateStatus update_action_record(
        LegacyActionRecord& record
    ) override {
        ++update_count;
        record.field_4a = static_cast<u16>(record.action_id);
        record.field_4c = static_cast<u16>(record.base_variant);
        record.draw_offset_x = 2U;
        record.draw_offset_y = 3U;
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id,
        const u16 frame_index,
        LegacyFramePiece& piece
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
        draws.push_back(DrawCall{
            .x = destination_x,
            .y = destination_y,
            .flags = flags,
            .red_offset = effects_.red_offset,
        });
        return LegacyBlitExecutionStatus::completed;
    }

    LegacyBlitEffectState& effects_;
    u32 update_count{};
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
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
        state,
        LegacyInitialMenuInput{},
        ports,
        effects
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
            state,
            LegacyInitialMenuInput{},
            ports,
            effects
        ));
    }
    test.expect_true(
        state.phase == 1 && state.counter == 0 &&
            state.slide_offsets[0] == -30 &&
            state.slide_offsets[1] == -12,
        "the 10th terminal callback enters phase one after slide animation"
    );
}

void test_strict_hitbox_and_new_game_submit(
    openswd3::test::Context& test
) {
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
        submitted.event == LegacyInitialMenuEvent::none &&
            state.phase == 2 && state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter &&
            state.name_input.has_value() && state.name_input->x() == 0x12C &&
            state.name_input->y() == 0xE6 &&
            state.first_name[0] == 0xC1U &&
            state.first_name[3] == 0x53U &&
            state.second_name[0] == 0xA9U && state.second_name[3] == 0x69U &&
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
        state,
        LegacyInitialMenuInput{.down = &down},
        ports,
        effects
    ));
    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{.primary = &primary},
        ports,
        effects
    ));
    test.expect_true(
        state.phase == 2 && state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter,
        "direction and primary callbacks select new game without bypassing "
        "phase two"
    );

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{.primary = &primary},
        ports,
        effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameTwoCounter,
        "first confirmation advances to the second 0x20-byte input object"
    );
    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{.primary = &primary},
        ports,
        effects
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
                    state,
                    LegacyInitialMenuInput{},
                    ports,
                    effects
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
        state,
        LegacyInitialMenuInput{.cancel = &cancel},
        ports,
        effects
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
        state.counter, kLegacyInitialMenuNameOneCounter,
        "the old transposed name-button coordinates are rejected");

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
        state.counter, kLegacyInitialMenuNameTwoCounter,
        "the name button accepts x 0x162..0x198 and y 0x101..0x112");
}

void test_text_object_result_and_edited_name(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameOneCounter;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects));
    auto view = state.name_input->borrow_edit_view();
    view.bytes[0] = 0x41U;
    view.bytes[1] = 0U;
    *view.result = 1;

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects));
    test.expect_true(
        state.counter == kLegacyInitialMenuNameTwoCounter &&
            state.first_name[0] == 0x41U && state.first_name[1] == 0U &&
            state.name_input.has_value(),
        "text result one commits the edit and creates the second input object");
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
        state,
        LegacyInitialMenuInput{},
        ports,
        effects
    );
    test.expect_true(
        result.action_update_count == 5U &&
            result.frame_request_count == 5U && result.draw_count == 41U &&
            ports.loads.front() == std::pair<u16, u16>{0x232AU, 0x4EU} &&
            ports.loads[2] == std::pair<u16, u16>{0x232BU, 0x2DU} &&
            ports.draws.front().flags == 0U &&
            ports.draws[1].x == 0x7B && ports.draws[1].y == 0xCF &&
            ports.draws[2].x == 0x7B &&
            ports.draws[2].y == 0x104 &&
            ports.draws[2].flags == 4U,
        "the four 0x232B choices use common x and the recovered vertical "
        "anchors"
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
    test_real_draw_contract(test);
    return test.exit_code();
}
