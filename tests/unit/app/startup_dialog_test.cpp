#include "test.hpp"

#include "openswd3/app/startup_dialog.hpp"

#include <array>
#include <vector>

namespace {

using openswd3::app::StartupDialogAction;
using openswd3::app::StartupDialogPointerMessage;
using openswd3::compat::u8;
using openswd3::compat::u32;

[[nodiscard]] constexpr u32 packed_position(const u32 x, const u32 y) {
    return (y << 16U) | x;
}

void expect_visibility(
    openswd3::test::Context& test,
    const openswd3::app::StartupDialogState& state,
    const std::array<u8, 5>& expected,
    const char* description
) {
    test.expect_equal(state.control_visibility, expected, description);
}

void test_layout_and_initialization(openswd3::test::Context& test) {
    test.expect_equal(
        openswd3::app::kStartupDialogControlLayouts[0],
        openswd3::app::StartupDialogControlLayout{0x40EU, 48U, 4U, 39U, 180U},
        "first control layout"
    );
    test.expect_equal(
        openswd3::app::kStartupDialogControlLayouts[4],
        openswd3::app::StartupDialogControlLayout{0x412U, 570U, 4U, 39U, 180U},
        "last control layout"
    );

    openswd3::app::StartupDialogState state{{5U, 4U, 3U, 2U, 1U}};
    openswd3::app::initialize_startup_dialog(state);
    expect_visibility(test, state, {}, "initialization clears all hover states");
}

void test_command_selection(openswd3::test::Context& test) {
    using openswd3::app::select_startup_dialog_command;

    const std::array expected{
        StartupDialogAction::show_auxiliary_then_end_dialog_2,
        StartupDialogAction::open_url,
        StartupDialogAction::open_readme,
        StartupDialogAction::end_dialog_6,
        StartupDialogAction::end_dialog_3,
    };
    for (u32 offset = 0U; offset < expected.size(); ++offset) {
        test.expect_equal(
            select_startup_dialog_command(0xABCD0000U | (0x3FEU + offset)),
            expected[offset],
            "WM_COMMAND uses only the low word for IDs 1022 through 1026"
        );
    }
    test.expect_equal(
        select_startup_dialog_command(0x3FDU),
        StartupDialogAction::none,
        "command below table is ignored"
    );
    test.expect_equal(
        openswd3::app::startup_dialog_close_action(),
        StartupDialogAction::end_dialog_6,
        "WM_CLOSE ends with numeric result six"
    );
}

void test_pointer_state(openswd3::test::Context& test) {
    openswd3::app::StartupDialogState state{};
    auto result = openswd3::app::update_startup_dialog_pointer(
        state,
        true,
        StartupDialogPointerMessage::move,
        packed_position(0x2BU, 7U)
    );
    expect_visibility(test, state, {5U, 0U, 0U, 0U, 0U}, "first hover enters");
    test.expect_true(result.play_hover_sound, "entering first hover plays sound");
    test.expect_equal(result.action, StartupDialogAction::none, "move has no click action");

    result = openswd3::app::update_startup_dialog_pointer(
        state,
        true,
        StartupDialogPointerMessage::move,
        packed_position(0x51U, 0xBBU)
    );
    test.expect_false(result.play_hover_sound, "remaining in same hover is silent");

    result = openswd3::app::update_startup_dialog_pointer(
        state,
        true,
        StartupDialogPointerMessage::left_button_down,
        packed_position(0x2BU, 7U)
    );
    test.expect_equal(
        result.action,
        StartupDialogAction::end_dialog_1,
        "first saved-game click returns one"
    );

    result = openswd3::app::update_startup_dialog_pointer(
        state,
        false,
        StartupDialogPointerMessage::left_button_down,
        packed_position(0x2BU, 7U)
    );
    expect_visibility(test, state, {}, "first hover is hidden without any save");
    test.expect_equal(
        result.action,
        StartupDialogAction::none,
        "first click is disabled without any save"
    );
    test.expect_false(result.play_hover_sound, "disabled first hover is silent");

    struct PointerCase {
        u32 x{};
        StartupDialogAction action{};
        std::array<u8, 5> visibility{};
    };
    const std::array cases{
        PointerCase{0x57U, StartupDialogAction::end_dialog_2, {0U, 5U, 0U, 0U, 0U}},
        PointerCase{0x1E4U, StartupDialogAction::open_url, {0U, 0U, 5U, 0U, 0U}},
        PointerCase{0x20EU, StartupDialogAction::open_readme, {0U, 0U, 0U, 5U, 0U}},
        PointerCase{0x235U, StartupDialogAction::end_dialog_6, {0U, 0U, 0U, 0U, 5U}},
    };
    for (const PointerCase& pointer_case : cases) {
        result = openswd3::app::update_startup_dialog_pointer(
            state,
            true,
            StartupDialogPointerMessage::left_button_down,
            packed_position(pointer_case.x, 7U)
        );
        expect_visibility(test, state, pointer_case.visibility, "pointer range visibility");
        test.expect_equal(result.action, pointer_case.action, "pointer range click action");
        test.expect_true(result.play_hover_sound, "entering a different range plays sound");
    }

    result = openswd3::app::update_startup_dialog_pointer(
        state,
        true,
        StartupDialogPointerMessage::move,
        packed_position(0x25CU, 7U)
    );
    expect_visibility(test, state, {}, "past-last x boundary is excluded");
    test.expect_false(result.play_hover_sound, "leaving all ranges is silent");

    result = openswd3::app::update_startup_dialog_pointer(
        state,
        true,
        StartupDialogPointerMessage::move,
        packed_position(0x57U, 0xBCU)
    );
    expect_visibility(test, state, {}, "y above 0xBB is excluded");
}

enum class PortCall {
    auxiliary,
    url,
    readme,
    end_dialog,
};

struct PortEvent {
    PortCall call{};
    openswd3::compat::i32 result{};

    bool operator==(const PortEvent&) const = default;
};

class RecordingPorts final : public openswd3::app::StartupDialogPorts {
public:
    void show_auxiliary_dialog() override {
        events.push_back({PortCall::auxiliary, 0});
    }
    void open_url() override { events.push_back({PortCall::url, 0}); }
    void open_readme() override { events.push_back({PortCall::readme, 0}); }
    void end_dialog(const openswd3::compat::i32 result) override {
        events.push_back({PortCall::end_dialog, result});
    }

    std::vector<PortEvent> events;
};

void test_action_execution(openswd3::test::Context& test) {
    RecordingPorts ports;
    openswd3::app::execute_startup_dialog_action(
        StartupDialogAction::show_auxiliary_then_end_dialog_2,
        ports
    );
    const std::vector<PortEvent> auxiliary_expected{
        {PortCall::auxiliary, 0},
        {PortCall::end_dialog, 2},
    };
    test.expect_equal(
        ports.events,
        auxiliary_expected,
        "auxiliary dialog closes parent with result two afterward"
    );

    ports.events.clear();
    openswd3::app::execute_startup_dialog_action(
        StartupDialogAction::open_url,
        ports
    );
    openswd3::app::execute_startup_dialog_action(
        StartupDialogAction::open_readme,
        ports
    );
    openswd3::app::execute_startup_dialog_action(
        StartupDialogAction::end_dialog_3,
        ports
    );
    const std::vector<PortEvent> remaining_expected{
        {PortCall::url, 0},
        {PortCall::readme, 0},
        {PortCall::end_dialog, 3},
    };
    test.expect_equal(ports.events, remaining_expected, "remaining action order");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_layout_and_initialization(test);
    test_command_selection(test);
    test_pointer_state(test);
    test_action_execution(test);
    return test.exit_code();
}
