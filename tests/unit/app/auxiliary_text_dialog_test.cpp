#include "test.hpp"

#include "openswd3/app/auxiliary_text_dialog.hpp"

#include <string>
#include <vector>

namespace {

using openswd3::app::AuxiliaryTextDialogAction;

void test_constants(openswd3::test::Context& test) {
    test.expect_equal(
        openswd3::app::kAuxiliaryTextEditLimitBytes,
        8U,
        "EM_SETLIMITTEXT receives eight"
    );
    test.expect_equal(
        openswd3::app::kAuxiliaryTextCopyBufferBytes,
        8U,
        "GetDlgItemText buffer size is eight including NUL"
    );
    test.expect_equal(
        openswd3::app::kAuxiliaryTextInitMessage,
        0x46AU,
        "second initialization message is preserved numerically"
    );
}

void test_submit_and_cancel(openswd3::test::Context& test) {
    openswd3::app::AuxiliaryTextDialogState state{"previous"};
    test.expect_equal(
        openswd3::app::handle_auxiliary_text_command(state, 1U, ""),
        AuxiliaryTextDialogAction::show_empty_warning,
        "empty submit warns without ending"
    );
    test.expect_equal(
        state.committed_bytes,
        std::string{"previous"},
        "empty submit leaves previous committed bytes untouched"
    );

    test.expect_equal(
        openswd3::app::handle_auxiliary_text_command(
            state,
            0xBEEF0001U,
            "12345678"
        ),
        AuxiliaryTextDialogAction::end_dialog_1,
        "submit command uses its low word"
    );
    test.expect_equal(
        state.committed_bytes,
        std::string{"1234567"},
        "eight visible bytes are copied as only seven plus NUL"
    );

    test.expect_equal(
        openswd3::app::handle_auxiliary_text_command(
            state,
            openswd3::app::kAuxiliaryTextCancelControlId,
            "ignored"
        ),
        AuxiliaryTextDialogAction::end_dialog_0,
        "cancel ends with numeric zero"
    );
    test.expect_equal(
        state.committed_bytes,
        std::string{"1234567"},
        "cancel leaves committed bytes untouched"
    );
    test.expect_equal(
        openswd3::app::auxiliary_text_close_action(),
        AuxiliaryTextDialogAction::end_dialog_0,
        "WM_CLOSE ends with numeric zero"
    );
}

void test_byte_truncation(openswd3::test::Context& test) {
    std::string visible{"abcdef"};
    visible.push_back(static_cast<char>(0xC3));
    visible.push_back(static_cast<char>(0xA9));

    openswd3::app::AuxiliaryTextDialogState state;
    test.expect_equal(
        openswd3::app::handle_auxiliary_text_command(state, 1U, visible),
        AuxiliaryTextDialogAction::end_dialog_1,
        "nonempty split UTF-8 input still submits"
    );
    std::string expected{"abcdef"};
    expected.push_back(static_cast<char>(0xC3));
    test.expect_equal(
        state.committed_bytes,
        expected,
        "legacy copy truncates bytes even inside a UTF-8 sequence"
    );

    const std::string embedded_nul{"abc\0def", 7U};
    test.expect_equal(
        openswd3::app::handle_auxiliary_text_command(state, 1U, embedded_nul),
        AuxiliaryTextDialogAction::end_dialog_1,
        "bytes before embedded NUL still submit"
    );
    test.expect_equal(
        state.committed_bytes,
        std::string{"abc"},
        "legacy control text terminates at the first NUL"
    );
}

enum class PortCall {
    warning,
    end_dialog,
};

struct PortEvent {
    PortCall call{};
    openswd3::compat::i32 result{};

    bool operator==(const PortEvent&) const = default;
};

class RecordingPorts final : public openswd3::app::AuxiliaryTextDialogPorts {
public:
    void show_empty_warning() override {
        events.push_back({PortCall::warning, 0});
    }
    void end_dialog(const openswd3::compat::i32 result) override {
        events.push_back({PortCall::end_dialog, result});
    }

    std::vector<PortEvent> events;
};

void test_action_execution(openswd3::test::Context& test) {
    RecordingPorts ports;
    openswd3::app::execute_auxiliary_text_dialog_action(
        AuxiliaryTextDialogAction::show_empty_warning,
        ports
    );
    openswd3::app::execute_auxiliary_text_dialog_action(
        AuxiliaryTextDialogAction::end_dialog_1,
        ports
    );
    openswd3::app::execute_auxiliary_text_dialog_action(
        AuxiliaryTextDialogAction::end_dialog_0,
        ports
    );
    const std::vector<PortEvent> expected{
        {PortCall::warning, 0},
        {PortCall::end_dialog, 1},
        {PortCall::end_dialog, 0},
    };
    test.expect_equal(ports.events, expected, "auxiliary action execution order");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_constants(test);
    test_submit_and_cancel(test);
    test_byte_truncation(test);
    test_action_execution(test);
    return test.exit_code();
}
