#include "test.hpp"

#include "openswd3/app/initialization.hpp"

#include <vector>

namespace {

enum class Call {
    hide_cursor,
    platform_backends,
    input_audio_paths,
    software_drawing,
    report_drawing_failure,
    memory_capacity,
    resource_database,
    render_resources,
    frame_interval,
    report_frame_clock_failure,
    synchronous_destroy,
    gameplay_state,
};

class RecordingPorts final : public openswd3::app::InitializationPorts {
public:
    void hide_cursor() override {
        calls.push_back(Call::hide_cursor);
    }
    bool initialize_platform_backends() override {
        calls.push_back(Call::platform_backends);
        return platform_success;
    }
    void configure_input_and_audio_paths() override {
        calls.push_back(Call::input_audio_paths);
    }
    bool initialize_software_drawing() override {
        calls.push_back(Call::software_drawing);
        return drawing_success;
    }
    void report_software_drawing_failure() override {
        calls.push_back(Call::report_drawing_failure);
    }
    void check_legacy_memory_capacity() override {
        calls.push_back(Call::memory_capacity);
    }
    void initialize_resource_database() override {
        calls.push_back(Call::resource_database);
    }
    void initialize_render_resources() override {
        calls.push_back(Call::render_resources);
    }
    bool initialize_frame_interval_35() override {
        calls.push_back(Call::frame_interval);
        return frame_clock_success;
    }
    void report_frame_clock_failure() override {
        calls.push_back(Call::report_frame_clock_failure);
    }
    void request_synchronous_destroy() override {
        calls.push_back(Call::synchronous_destroy);
    }
    void initialize_story_world_and_asset_state() override {
        calls.push_back(Call::gameplay_state);
    }

    bool platform_success{true};
    bool drawing_success{true};
    bool frame_clock_success{true};
    std::vector<Call> calls;
};

const std::vector<Call> kSuccessfulCalls{
    Call::hide_cursor,
    Call::platform_backends,
    Call::input_audio_paths,
    Call::software_drawing,
    Call::memory_capacity,
    Call::resource_database,
    Call::render_resources,
    Call::frame_interval,
    Call::gameplay_state,
};

void test_success_and_ignored_platform_result(openswd3::test::Context& test) {
    for (const bool platform_success : {true, false}) {
        openswd3::app::InitializationState state{9, 9, 9};
        RecordingPorts ports;
        ports.platform_success = platform_success;
        test.expect_true(
            openswd3::app::run_total_initialization(state, ports),
            "platform return is ignored by the original caller"
        );
        test.expect_equal(
            ports.calls, kSuccessfulCalls, "successful initialization order"
        );
        test.expect_equal(
            state.transition_suppression, 0U, "suppression starts cleared"
        );
        test.expect_equal(
            state.frame_counter, 0U, "frame counter starts cleared"
        );
        test.expect_equal(
            state.special_mode_state,
            0x80000003U,
            "successful initialization requests tagged mode three"
        );
    }
}

void test_drawing_failure(openswd3::test::Context& test) {
    openswd3::app::InitializationState state{};
    RecordingPorts ports;
    ports.drawing_success = false;
    test.expect_false(
        openswd3::app::run_total_initialization(state, ports),
        "software drawing failure stops initialization"
    );
    const std::vector<Call> expected{
        Call::hide_cursor,
        Call::platform_backends,
        Call::input_audio_paths,
        Call::software_drawing,
        Call::report_drawing_failure,
        Call::synchronous_destroy,
    };
    test.expect_equal(
        ports.calls, expected, "drawing failure destroys synchronously"
    );
}

void test_frame_clock_failure(openswd3::test::Context& test) {
    openswd3::app::InitializationState state{};
    RecordingPorts ports;
    ports.frame_clock_success = false;
    test.expect_false(
        openswd3::app::run_total_initialization(state, ports),
        "frame-clock failure stops before gameplay state"
    );
    const std::vector<Call> expected{
        Call::hide_cursor,
        Call::platform_backends,
        Call::input_audio_paths,
        Call::software_drawing,
        Call::memory_capacity,
        Call::resource_database,
        Call::render_resources,
        Call::frame_interval,
        Call::report_frame_clock_failure,
        Call::synchronous_destroy,
    };
    test.expect_equal(ports.calls, expected, "frame-clock failure call order");
    test.expect_equal(
        state.transition_suppression,
        1U,
        "frame-clock failure enables transition suppression before destroy"
    );
}

void test_dialog_wrapper_always_returns_one(openswd3::test::Context& test) {
    for (const bool drawing_success : {true, false}) {
        openswd3::app::InitializationState state{};
        RecordingPorts ports;
        ports.drawing_success = drawing_success;
        test.expect_equal(
            openswd3::app::run_initialization_dialog_wrapper(state, ports),
            1,
            "0x00424910 replaces the total-initialization result with one"
        );
        test.expect_false(
            ports.calls.empty(),
            "dialog wrapper always invokes total initialization first"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_success_and_ignored_platform_result(test);
    test_drawing_failure(test);
    test_frame_clock_failure(test);
    test_dialog_wrapper_always_returns_one(test);
    return test.exit_code();
}
