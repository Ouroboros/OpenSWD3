#include "test.hpp"

#include "openswd3/app/host_window_event.hpp"

namespace {

using openswd3::compat::u32;

class RecordingWindowPorts final : public openswd3::app::WindowEventPorts {
public:
    void release_active_video() override {
        ++video_releases;
    }
    u32 free_disk_space_mebibytes() override {
        return 0U;
    }
    void capture_legacy_screenshot() override {
        ++screenshots;
    }

    u32 video_releases{};
    u32 screenshots{};
};

class RecordingDisplayPorts final
    : public openswd3::app::DisplayLifecyclePorts {
public:
    bool display_backend_available() override {
        return true;
    }
    void set_frame_interval(u32) override {}
    void suspend_audio_output() override {}
    void suspend_audio_streams() override {}
    void maintain_audio() override {}
    void suspend_battle_display() override {}
    void release_font(u32) override {}
    void minimize_window() override {
        ++minimizations;
    }
    void show_and_position_window() override {
        ++restorations;
    }
    void restore_surfaces() override {}
    void rebuild_framebuffer_binding() override {}
    void rebuild_font(u32) override {}
    void resume_battle_display() override {}
    void finish_display_recovery() override {}

    u32 minimizations{};
    u32 restorations{};
};

void test_dispatch(openswd3::test::Context& test) {
    using openswd3::app::HostWindowEvent;
    using openswd3::app::HostWindowEventKind;
    using openswd3::app::HostWindowEventResult;
    using openswd3::app::dispatch_host_window_event;

    openswd3::app::WindowEventState window_state{1U, 0U, 1U};
    openswd3::app::DisplayLifecycleState display_state{1U, 0U, 0U};
    RecordingWindowPorts window_ports;
    RecordingDisplayPorts display_ports;

    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::size, 1U},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::continue_running,
        "minimize event continues the host loop"
    );
    test.expect_equal(display_state.display_active, 0U, "minimize deactivates");
    test.expect_equal(
        display_state.transition_suppression,
        1U,
        "minimize enables transition suppression"
    );
    test.expect_equal(display_ports.minimizations, 1U, "minimize reaches port");

    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::activation, 1U},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::continue_running,
        "focus event continues the host loop"
    );
    test.expect_equal(
        display_state.display_active, 1U, "focus gain reactivates"
    );
    test.expect_equal(display_ports.restorations, 1U, "restore reaches port");

    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::system_key_down, 0x10DU},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::continue_running,
        "system-key event continues the host loop"
    );
    test.expect_equal(
        display_state.display_active,
        0U,
        "system Enter preserves low-byte legacy toggle"
    );

    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::key_release, 0x77U},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::continue_running,
        "key-release event continues the host loop"
    );
    test.expect_equal(
        window_state.frame_execution_gate, 0U, "F8 reaches app core"
    );

    window_state.process_flags = openswd3::app::kProcessVideoActive;
    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::left_button_down, 0x1BU},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::continue_running,
        "mouse event continues the host loop"
    );
    test.expect_equal(
        window_ports.video_releases, 1U, "left-button path reaches video"
    );
    test.expect_equal(window_state.process_flags, 0U, "video bit is cleared");

    test.expect_equal(
        dispatch_host_window_event(
            HostWindowEvent{HostWindowEventKind::request_close, 0U},
            window_state,
            window_ports,
            display_state,
            display_ports
        ),
        HostWindowEventResult::request_close,
        "close request exits the host loop"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_dispatch(test);
    return test.exit_code();
}
