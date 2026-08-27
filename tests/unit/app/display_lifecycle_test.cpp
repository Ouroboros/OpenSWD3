#include "test.hpp"

#include "openswd3/app/display_lifecycle.hpp"

#include <vector>

namespace {

using openswd3::compat::u32;

enum class DisplayCall {
    query_display_backend,
    set_frame_interval,
    suspend_audio_output,
    suspend_audio_streams,
    maintain_audio,
    suspend_battle_display,
    release_font,
    minimize_window,
    show_and_position_window,
    restore_surfaces,
    rebuild_framebuffer_binding,
    rebuild_font,
    resume_battle_display,
    finish_display_recovery,
};

struct DisplayEvent {
    DisplayCall call{};
    u32 argument{};
    u32 display_active{};
    u32 transition_suppression{};

    bool operator==(const DisplayEvent&) const = default;
};

class RecordingDisplayPorts final
    : public openswd3::app::DisplayLifecyclePorts {
public:
    explicit RecordingDisplayPorts(openswd3::app::DisplayLifecycleState& state)
        : state_(state) {}

    bool display_backend_available() override {
        record(DisplayCall::query_display_backend);
        return backend_available;
    }
    void set_frame_interval(const u32 milliseconds) override {
        record(DisplayCall::set_frame_interval, milliseconds);
    }
    void suspend_audio_output() override {
        record(DisplayCall::suspend_audio_output);
    }
    void suspend_audio_streams() override {
        record(DisplayCall::suspend_audio_streams);
    }
    void maintain_audio() override {
        record(DisplayCall::maintain_audio);
    }
    void suspend_battle_display() override {
        record(DisplayCall::suspend_battle_display);
    }
    void release_font(const u32 point_size) override {
        record(DisplayCall::release_font, point_size);
    }
    void minimize_window() override {
        record(DisplayCall::minimize_window);
    }
    void show_and_position_window() override {
        record(DisplayCall::show_and_position_window);
    }
    void restore_surfaces() override {
        record(DisplayCall::restore_surfaces);
    }
    void rebuild_framebuffer_binding() override {
        record(DisplayCall::rebuild_framebuffer_binding);
    }
    void rebuild_font(const u32 point_size) override {
        record(DisplayCall::rebuild_font, point_size);
    }
    void resume_battle_display() override {
        record(DisplayCall::resume_battle_display);
    }
    void finish_display_recovery() override {
        record(DisplayCall::finish_display_recovery);
    }

    bool backend_available{true};
    std::vector<DisplayEvent> events;

private:
    void record(const DisplayCall call, const u32 argument = 0U) {
        events.push_back(
            {call,
             argument,
             state_.display_active,
             state_.transition_suppression}
        );
    }

    openswd3::app::DisplayLifecycleState& state_;
};

enum class ShutdownEventKind {
    operation,
    battle_runtime,
    close,
    report,
};

struct ShutdownEvent {
    ShutdownEventKind kind{};
    u32 value{};

    bool operator==(const ShutdownEvent&) const = default;
};

class RecordingShutdownPorts final : public openswd3::app::ShutdownPorts {
public:
    void perform_shutdown_operation(
        const openswd3::app::ShutdownOperation operation
    ) override {
        events.push_back(
            {ShutdownEventKind::operation, static_cast<u32>(operation)}
        );
    }

    void release_battle_runtime() override {
        events.push_back({ShutdownEventKind::battle_runtime, 0U});
    }

    bool perform_shutdown_close(
        const openswd3::app::ShutdownCloseOperation operation
    ) override {
        events.push_back(
            {ShutdownEventKind::close, static_cast<u32>(operation)}
        );
        return !fail_close || operation != failed_close;
    }

    void report_shutdown_close_failure(
        const openswd3::app::ShutdownCloseOperation operation
    ) override {
        events.push_back(
            {ShutdownEventKind::report, static_cast<u32>(operation)}
        );
    }

    bool fail_close{};
    openswd3::app::ShutdownCloseOperation failed_close{
        openswd3::app::ShutdownCloseOperation::role_handle
    };
    std::vector<ShutdownEvent> events;
};

ShutdownEvent operation_event(const openswd3::app::ShutdownOperation value) {
    return {ShutdownEventKind::operation, static_cast<u32>(value)};
}

ShutdownEvent battle_runtime_event() {
    return {ShutdownEventKind::battle_runtime, 0U};
}

ShutdownEvent close_event(const openswd3::app::ShutdownCloseOperation value) {
    return {ShutdownEventKind::close, static_cast<u32>(value)};
}

void test_deactivate(openswd3::test::Context& test) {
    openswd3::app::DisplayLifecycleState state{1, 0, 1};
    RecordingDisplayPorts ports(state);
    openswd3::app::deactivate_display(state, ports);

    const std::vector<DisplayEvent> expected{
        {DisplayCall::query_display_backend, 0, 1, 0},
        {DisplayCall::set_frame_interval, 0, 1, 0},
        {DisplayCall::suspend_audio_output, 0, 1, 0},
        {DisplayCall::suspend_audio_streams, 0, 1, 0},
        {DisplayCall::maintain_audio, 0, 1, 0},
        {DisplayCall::suspend_battle_display, 0, 1, 0},
        {DisplayCall::release_font, 20, 1, 0},
        {DisplayCall::release_font, 16, 1, 0},
        {DisplayCall::release_font, 12, 1, 0},
        {DisplayCall::minimize_window, 0, 0, 1},
    };
    test.expect_equal(
        ports.events, expected, "display deactivation call order"
    );
    test.expect_equal(
        state.display_active, 0U, "deactivation clears display active"
    );
    test.expect_equal(
        state.transition_suppression,
        1U,
        "deactivation enables transition suppression"
    );
}

void test_reactivate(openswd3::test::Context& test) {
    openswd3::app::DisplayLifecycleState state{0, 1, 1};
    RecordingDisplayPorts ports(state);
    openswd3::app::reactivate_display(state, ports);

    const std::vector<DisplayEvent> expected{
        {DisplayCall::query_display_backend, 0, 0, 1},
        {DisplayCall::show_and_position_window, 0, 0, 1},
        {DisplayCall::restore_surfaces, 0, 1, 1},
        {DisplayCall::rebuild_framebuffer_binding, 0, 1, 1},
        {DisplayCall::rebuild_font, 20, 1, 1},
        {DisplayCall::rebuild_font, 16, 1, 1},
        {DisplayCall::rebuild_font, 12, 1, 1},
        {DisplayCall::resume_battle_display, 0, 1, 0},
        {DisplayCall::finish_display_recovery, 0, 1, 0},
        {DisplayCall::set_frame_interval, 35, 1, 0},
    };
    test.expect_equal(
        ports.events, expected, "display reactivation call order"
    );
    test.expect_equal(
        state.display_active, 1U, "reactivation sets display active"
    );
    test.expect_equal(
        state.transition_suppression,
        0U,
        "reactivation clears transition suppression before battle resume"
    );
}

void test_missing_display_backend(openswd3::test::Context& test) {
    openswd3::app::DisplayLifecycleState state{7U, 8U, 9U};
    RecordingDisplayPorts ports(state);
    ports.backend_available = false;

    openswd3::app::deactivate_display(state, ports);
    openswd3::app::reactivate_display(state, ports);

    const std::vector<DisplayEvent> expected{
        {DisplayCall::query_display_backend, 0U, 7U, 8U},
        {DisplayCall::query_display_backend, 0U, 7U, 8U},
    };
    test.expect_equal(
        ports.events,
        expected,
        "missing display backend returns before every lifecycle side effect"
    );
    test.expect_equal(
        state.display_active, 7U, "missing backend preserves display"
    );
    test.expect_equal(
        state.transition_suppression,
        8U,
        "missing backend preserves transition suppression"
    );
    test.expect_equal(
        state.battle_active, 9U, "missing backend preserves battle"
    );
}

void test_shutdown(openswd3::test::Context& test) {
    using enum openswd3::app::ShutdownCloseOperation;
    using enum openswd3::app::ShutdownOperation;

    RecordingShutdownPorts ports;
    test.expect_equal(
        openswd3::app::run_total_shutdown(ports),
        1,
        "total shutdown returns one"
    );
    const std::vector<ShutdownEvent> expected{
        operation_event(release_font_20),
        operation_event(release_font_16),
        operation_event(release_font_12),
        operation_event(release_00406e00),
        battle_runtime_event(),
        operation_event(release_00478110),
        operation_event(drain_list_004a9a2c),
        operation_event(release_0040f5e0),
        operation_event(release_0040f500),
        operation_event(release_0040f540),
        operation_event(release_0040f570),
        operation_event(release_0040dbc0),
        operation_event(release_0040f5a0),
        operation_event(release_0040f630),
        operation_event(release_0040f670),
        operation_event(drain_list_004ab2f4),
        operation_event(drain_list_004b89f4),
        operation_event(release_004020c0),
        operation_event(release_0040f3b0),
        operation_event(free_004a9a04),
        operation_event(free_004a9a08),
        operation_event(free_004a9a0c),
        operation_event(release_0040f410),
        operation_event(release_00433010),
        operation_event(release_00431960),
        operation_event(suspend_audio_output_00485710),
        operation_event(suspend_audio_streams_00485740),
        operation_event(free_004cae78),
        operation_event(free_004cd764),
        operation_event(release_common_source_surface),
        operation_event(release_display_surfaces_00437a50),
        operation_event(release_display_backend_00437ad0),
        operation_event(release_input_backend_004374e0),
        close_event(role_handle),
        close_event(path_view),
        close_event(path_mapping),
        close_event(path_handle),
        close_event(talk_handle),
        close_event(pixel_view),
        close_event(pixel_mapping),
        close_event(pixel_handle),
        operation_event(free_004b7404),
        operation_event(free_004b794c),
        operation_event(free_004b7948),
        operation_event(free_004c9a10),
        operation_event(free_004b8860),
        operation_event(free_004accd0),
        operation_event(drain_list_004acac0),
        operation_event(release_final_drawing_state_004258e0),
        operation_event(show_cursor),
    };
    test.expect_equal(ports.events, expected, "total shutdown assembly order");

    RecordingShutdownPorts failure_ports;
    failure_ports.fail_close = true;
    failure_ports.failed_close = path_mapping;
    static_cast<void>(openswd3::app::run_total_shutdown(failure_ports));
    test.expect_equal(
        failure_ports.events.size(),
        expected.size() + 1U,
        "one close failure adds one report and does not abort"
    );
    test.expect_equal(
        failure_ports.events.at(35U),
        close_event(path_mapping),
        "failing close occurs at its assembly position"
    );
    test.expect_equal(
        failure_ports.events.at(36U),
        ShutdownEvent{
            ShutdownEventKind::report, static_cast<u32>(path_mapping)
        },
        "failure report immediately follows its close"
    );
    test.expect_equal(
        failure_ports.events.at(37U),
        close_event(path_handle),
        "shutdown continues with the next close"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_deactivate(test);
    test_reactivate(test);
    test_missing_display_backend(test);
    test_shutdown(test);
    return test.exit_code();
}
