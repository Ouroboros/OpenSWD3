#include "test.hpp"

#include "openswd3/app/window_events.hpp"

#include <vector>

namespace {

using openswd3::app::DisplayTransition;
using openswd3::compat::u32;

void test_display_transition_selectors(openswd3::test::Context& test) {
    using openswd3::app::select_activation_display_transition;
    using openswd3::app::select_size_display_transition;
    using openswd3::app::select_system_enter_display_transition;

    test.expect_equal(
        select_size_display_transition(0U, 0U),
        DisplayTransition::none,
        "WM_SIZE is ignored before runtime initialization"
    );
    test.expect_equal(
        select_size_display_transition(1U, 0U),
        DisplayTransition::reactivate,
        "SIZE_RESTORED reactivates"
    );
    test.expect_equal(
        select_size_display_transition(1U, 2U),
        DisplayTransition::reactivate,
        "SIZE_MAXIMIZED reactivates"
    );
    for (const u32 value : {1U, 3U, 4U}) {
        test.expect_equal(
            select_size_display_transition(1U, value),
            DisplayTransition::deactivate,
            "legacy inactive size codes deactivate"
        );
    }
    test.expect_equal(
        select_size_display_transition(1U, 5U),
        DisplayTransition::none,
        "unknown size code is not normalized"
    );

    test.expect_equal(
        select_activation_display_transition(0U, 1U),
        DisplayTransition::none,
        "WM_ACTIVATEAPP is ignored before runtime initialization"
    );
    test.expect_equal(
        select_activation_display_transition(1U, 0x12340000U),
        DisplayTransition::deactivate,
        "WM_ACTIVATEAPP tests only the low word"
    );
    test.expect_equal(
        select_activation_display_transition(1U, 0x12340001U),
        DisplayTransition::reactivate,
        "nonzero low word reactivates"
    );

    test.expect_equal(
        select_system_enter_display_transition(0x10DU, 1U),
        DisplayTransition::deactivate,
        "system Enter compares only the low byte"
    );
    test.expect_equal(
        select_system_enter_display_transition(0x0DU, 2U),
        DisplayTransition::reactivate,
        "display value other than exact one reactivates"
    );
    test.expect_equal(
        select_system_enter_display_transition(0x0CU, 1U),
        DisplayTransition::none,
        "non-Enter system key is ignored"
    );
}

void test_system_command_filter(openswd3::test::Context& test) {
    for (const u32 command : {0xF130U, 0xF140U, 0xF170U, 0xF090U}) {
        test.expect_true(
            openswd3::app::should_intercept_system_command(command),
            "exact legacy system command is intercepted"
        );
    }
    test.expect_false(
        openswd3::app::should_intercept_system_command(0xF131U),
        "system command is not masked before comparison"
    );
    test.expect_false(
        openswd3::app::should_intercept_system_command(0xF100U),
        "unlisted system command reaches host default handling"
    );
}

struct TextInputCall {
    u32 message{};
    u32 first_parameter{};
    u32 second_parameter{};

    bool operator==(const TextInputCall&) const = default;
};

class RecordingTextInputPorts final
    : public openswd3::app::TextInputMessagePorts {
public:
    u32 filter_text_input_message(
        const u32 message, const u32 first_parameter, const u32 second_parameter
    ) override {
        calls.push_back({message, first_parameter, second_parameter});
        return result;
    }

    u32 result{};
    std::vector<TextInputCall> calls;
};

void test_text_input_message_prefix(openswd3::test::Context& test) {
    using openswd3::app::WindowMessagePrefixResult;
    using openswd3::app::run_window_message_prefix;

    RecordingTextInputPorts ports;
    test.expect_equal(
        run_window_message_prefix(0U, 0x100U, 0x41U, 0x12345678U, ports),
        WindowMessagePrefixResult::continue_dispatch,
        "inactive text input bypasses the downstream filter"
    );
    test.expect_true(
        ports.calls.empty(),
        "inactive text input does not call the downstream filter"
    );

    ports.result = 0U;
    test.expect_equal(
        run_window_message_prefix(1U, 0x102U, 0x42U, 0x87654321U, ports),
        WindowMessagePrefixResult::consume_and_return_one,
        "zero downstream result consumes the window message"
    );
    const std::vector<TextInputCall> expected_zero{
        {0x102U, 0x42U, 0x87654321U},
    };
    test.expect_equal(
        ports.calls,
        expected_zero,
        "active prefix forwards all three message values unchanged"
    );

    ports.calls.clear();
    ports.result = 7U;
    test.expect_equal(
        run_window_message_prefix(2U, 0x101U, 0x43U, 0xFFFFFFFFU, ports),
        WindowMessagePrefixResult::continue_dispatch,
        "any nonzero downstream result continues normal dispatch"
    );
}

enum class WindowPortCall {
    release_video,
    query_disk_space,
    capture_screenshot,
};

struct WindowPortEvent {
    WindowPortCall call{};
    u32 process_flags{};

    bool operator==(const WindowPortEvent&) const = default;
};

class RecordingWindowPorts final : public openswd3::app::WindowEventPorts {
public:
    explicit RecordingWindowPorts(openswd3::app::WindowEventState& state)
        : state_(state) {}

    void release_active_video() override {
        record(WindowPortCall::release_video);
    }

    u32 free_disk_space_mebibytes() override {
        record(WindowPortCall::query_disk_space);
        return free_space;
    }

    void capture_legacy_screenshot() override {
        record(WindowPortCall::capture_screenshot);
    }

    u32 free_space{};
    std::vector<WindowPortEvent> events;

private:
    void record(const WindowPortCall call) {
        events.push_back({call, state_.process_flags});
    }

    openswd3::app::WindowEventState& state_;
};

void test_key_release_video_and_pause(openswd3::test::Context& test) {
    using openswd3::app::handle_key_release;
    using openswd3::app::kProcessVideoActive;

    openswd3::app::WindowEventState state{0U, 0xA0U, 7U};
    RecordingWindowPorts ports(state);
    handle_key_release(state, 0x1BU, ports);
    const std::vector<WindowPortEvent> expected_release{
        {WindowPortCall::release_video, 0xA0U},
    };
    test.expect_equal(
        ports.events,
        expected_release,
        "Escape releases video before clearing its process bit"
    );
    test.expect_equal(
        state.process_flags, 0x80U, "Escape clears only the video-active bit"
    );
    test.expect_equal(
        state.frame_execution_gate, 7U, "Escape does not change the frame gate"
    );

    state = {0U, 0U, 0U};
    ports.events.clear();
    handle_key_release(state, 0x77U, ports);
    test.expect_equal(state.frame_execution_gate, 1U, "F8 turns zero gate on");
    handle_key_release(state, 0x77U, ports);
    test.expect_equal(
        state.frame_execution_gate, 0U, "F8 turns nonzero gate off"
    );

    state = {0U, kProcessVideoActive, 9U};
    handle_key_release(state, 0x77U, ports);
    test.expect_equal(
        state.frame_execution_gate,
        1U,
        "F8 forces exact gate value one while video is active"
    );
    test.expect_true(ports.events.empty(), "F8 does not release active video");
}

void test_screenshot_gates(openswd3::test::Context& test) {
    using openswd3::app::handle_key_release;
    using openswd3::app::kProcessIdleSuppression;

    openswd3::app::WindowEventState state{0U, 0U, 1U};
    RecordingWindowPorts ports(state);
    ports.free_space = 65U;
    handle_key_release(state, 0x50U, ports);
    test.expect_true(
        ports.events.empty(),
        "P does not query disk before runtime initialization"
    );

    state.runtime_initialized = 1U;
    state.process_flags = kProcessIdleSuppression;
    handle_key_release(state, 0x50U, ports);
    test.expect_true(
        ports.events.empty(),
        "idle-suppression bit blocks screenshot before disk query"
    );

    state.process_flags = 0U;
    ports.free_space = 64U;
    handle_key_release(state, 0x50U, ports);
    const std::vector<WindowPortEvent> threshold_events{
        {WindowPortCall::query_disk_space, 0U},
    };
    test.expect_equal(
        ports.events, threshold_events, "exactly 64 MiB does not capture"
    );

    ports.events.clear();
    ports.free_space = 65U;
    handle_key_release(state, 0x50U, ports);
    const std::vector<WindowPortEvent> capture_events{
        {WindowPortCall::query_disk_space, 0U},
        {WindowPortCall::capture_screenshot, 0U},
    };
    test.expect_equal(
        ports.events,
        capture_events,
        "more than 64 MiB captures after the query"
    );
}

void test_left_button_legacy_comparison(openswd3::test::Context& test) {
    using openswd3::app::handle_left_button_down;
    using openswd3::app::kProcessVideoActive;

    openswd3::app::WindowEventState state{1U, kProcessVideoActive, 1U};
    RecordingWindowPorts ports(state);
    handle_left_button_down(state, 1U, ports);
    test.expect_true(
        ports.events.empty(), "ordinary left-button state does not stop video"
    );
    test.expect_equal(
        state.process_flags,
        kProcessVideoActive,
        "ordinary left-button state preserves video bit"
    );

    handle_left_button_down(state, 0x1BU, ports);
    const std::vector<WindowPortEvent> expected{
        {WindowPortCall::release_video, kProcessVideoActive},
    };
    test.expect_equal(
        ports.events,
        expected,
        "exact legacy button-state value 0x1B stops video"
    );
    test.expect_equal(state.process_flags, 0U, "video stop clears video bit");
}

enum class ExitCall {
    shutdown,
    uninitialize_com,
    post_quit_zero,
};

class RecordingShutdownPorts final : public openswd3::app::ShutdownPorts {
public:
    explicit RecordingShutdownPorts(std::vector<ExitCall>& calls)
        : calls_(calls) {}

    void perform_shutdown_operation(openswd3::app::ShutdownOperation) override {
        record_once();
    }

    bool
    perform_shutdown_close(openswd3::app::ShutdownCloseOperation) override {
        record_once();
        return true;
    }

    void report_shutdown_close_failure(
        openswd3::app::ShutdownCloseOperation
    ) override {
        record_once();
    }

private:
    void record_once() {
        if (calls_.empty()) {
            calls_.push_back(ExitCall::shutdown);
        }
    }

    std::vector<ExitCall>& calls_;
};

class RecordingExitPorts final : public openswd3::app::ProcessExitPorts {
public:
    explicit RecordingExitPorts(std::vector<ExitCall>& calls) : calls_(calls) {}

    void uninitialize_com() override {
        calls_.push_back(ExitCall::uninitialize_com);
    }
    void post_quit_message_zero() override {
        calls_.push_back(ExitCall::post_quit_zero);
    }

private:
    std::vector<ExitCall>& calls_;
};

void test_destroy_gate_and_order(openswd3::test::Context& test) {
    std::vector<ExitCall> calls;
    RecordingShutdownPorts shutdown_ports(calls);
    RecordingExitPorts exit_ports(calls);

    openswd3::app::WindowEventState state{};
    openswd3::app::handle_window_destroy(state, shutdown_ports, exit_ports);
    const std::vector<ExitCall> no_shutdown{
        ExitCall::uninitialize_com,
        ExitCall::post_quit_zero,
    };
    test.expect_equal(
        calls,
        no_shutdown,
        "destroy without init or close bit skips total shutdown"
    );

    calls.clear();
    state.runtime_initialized = 1U;
    openswd3::app::handle_window_destroy(state, shutdown_ports, exit_ports);
    const std::vector<ExitCall> expected{
        ExitCall::shutdown,
        ExitCall::uninitialize_com,
        ExitCall::post_quit_zero,
    };
    test.expect_equal(calls, expected, "initialized destroy order");

    calls.clear();
    state.runtime_initialized = 0U;
    state.process_flags = openswd3::app::kProcessCloseRequested;
    openswd3::app::handle_window_destroy(state, shutdown_ports, exit_ports);
    test.expect_equal(
        calls, expected, "close-request bit also enables shutdown"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_display_transition_selectors(test);
    test_system_command_filter(test);
    test_text_input_message_prefix(test);
    test_key_release_video_and_pause(test);
    test_screenshot_gates(test);
    test_left_button_legacy_comparison(test);
    test_destroy_gate_and_order(test);
    return test.exit_code();
}
