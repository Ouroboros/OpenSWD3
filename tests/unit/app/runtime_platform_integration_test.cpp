#include "test.hpp"

#include "openswd3/app/command_line.hpp"
#include "openswd3/app/host_window_event.hpp"
#include "openswd3/app/idle_runtime.hpp"
#include "openswd3/app/process_startup.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace {

enum class Call {
    check_instance,
    read_time,
    seed_crt,
    seed_secondary,
    game_frame,
    shutdown,
    uninitialize_com,
    post_quit,
};

class IntegrationPorts final
    : public openswd3::app::ExistingInstancePorts,
      public openswd3::app::CommandLinePorts,
      public openswd3::app::RngSeedPorts,
      public openswd3::app::IdleRuntimePorts,
      public openswd3::app::WindowEventPorts,
      public openswd3::app::DisplayLifecyclePorts,
      public openswd3::app::ShutdownPorts,
      public openswd3::app::ProcessExitPorts {
public:
    bool matching_instance_exists() override {
        calls.push_back(Call::check_instance);
        return false;
    }

    void initialize_default_key_bindings() override {}
    void run_legacy_command(openswd3::compat::u8, std::string_view) override {}

    openswd3::compat::u32 read_time_seconds() override {
        calls.push_back(Call::read_time);
        return next_time++;
    }
    void seed_crt_rng(openswd3::compat::u32) override {
        calls.push_back(Call::seed_crt);
    }
    void seed_secondary_rng(openswd3::compat::u32) override {
        calls.push_back(Call::seed_secondary);
    }

    void step_video() override {}
    void maintain_audio() override {}
    void yield() override {}
    void step_game_frame() override { calls.push_back(Call::game_frame); }
    void present_pause() override {}

    void release_active_video() override {}
    openswd3::compat::u32 free_disk_space_mebibytes() override { return 0U; }
    void capture_legacy_screenshot() override {}

    bool display_backend_available() override { return true; }
    void set_frame_interval(openswd3::compat::u32) override {}
    void suspend_audio_output() override {}
    void suspend_audio_streams() override {}
    void suspend_battle_display() override {}
    void release_font(openswd3::compat::u32) override {}
    void minimize_window() override {}
    void show_and_position_window() override {}
    void restore_surfaces() override {}
    void rebuild_framebuffer_binding() override {}
    void rebuild_font(openswd3::compat::u32) override {}
    void resume_battle_display() override {}
    void finish_display_recovery() override {}

    void perform_shutdown_operation(openswd3::app::ShutdownOperation) override {
        record_shutdown();
    }
    bool perform_shutdown_close(
        openswd3::app::ShutdownCloseOperation
    ) override {
        record_shutdown();
        return true;
    }
    void report_shutdown_close_failure(
        openswd3::app::ShutdownCloseOperation
    ) override {}

    void uninitialize_com() override { calls.push_back(Call::uninitialize_com); }
    void post_quit_message_zero() override { calls.push_back(Call::post_quit); }

    openswd3::compat::u32 next_time{11U};
    openswd3::compat::u32 shutdown_calls{};
    std::vector<Call> calls;

private:
    void record_shutdown() {
        if (shutdown_calls == 0U) {
            calls.push_back(Call::shutdown);
        }
        ++shutdown_calls;
    }
};

void test_normal_lifecycle(openswd3::test::Context& test) {
    IntegrationPorts ports;
    test.expect_equal(
        openswd3::app::run_process_startup_gates(
            std::optional<std::string_view>{std::string_view{}},
            ports,
            ports
        ),
        openswd3::app::ProcessStartupGateResult::continue_normal_startup,
        "normal process gates continue"
    );

    openswd3::app::seed_two_rng_streams(ports);
    openswd3::app::run_idle_iteration({1U, 0U, 0U, 1U}, ports);

    openswd3::app::WindowEventState window_state{1U, 0U, 1U};
    openswd3::app::DisplayLifecycleState display_state{1U, 0U, 0U};
    test.expect_equal(
        openswd3::app::dispatch_host_window_event(
            {
                openswd3::app::HostWindowEventKind::request_close,
                0U,
            },
            window_state,
            ports,
            display_state,
            ports
        ),
        openswd3::app::HostWindowEventResult::request_close,
        "host close reaches the destroy boundary"
    );
    openswd3::app::handle_window_destroy(window_state, ports, ports);

    const std::vector<Call> expected{
        Call::check_instance,
        Call::read_time,
        Call::seed_crt,
        Call::read_time,
        Call::seed_secondary,
        Call::game_frame,
        Call::shutdown,
        Call::uninitialize_com,
        Call::post_quit,
    };
    test.expect_equal(ports.calls, expected, "B1 lifecycle integration order");
    test.expect_equal(
        ports.shutdown_calls,
        50U,
        "destroy executes all 42 operations and eight closes"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_normal_lifecycle(test);
    return test.exit_code();
}
