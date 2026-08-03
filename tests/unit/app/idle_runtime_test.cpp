#include "test.hpp"

#include "openswd3/app/idle_runtime.hpp"

#include <vector>

namespace {

enum class Call {
    step_video,
    maintain_audio,
    yield,
    step_game_frame,
    present_pause,
};

class RecordingPorts final : public openswd3::app::IdleRuntimePorts {
public:
    void step_video() override {
        calls.push_back(Call::step_video);
    }

    void maintain_audio() override {
        calls.push_back(Call::maintain_audio);
    }

    void yield() override {
        calls.push_back(Call::yield);
    }

    void step_game_frame() override {
        calls.push_back(Call::step_game_frame);
    }

    void present_pause() override {
        calls.push_back(Call::present_pause);
    }

    std::vector<Call> calls;
};

void expect_calls(
    openswd3::test::Context& test,
    const openswd3::app::IdleState& state,
    const std::vector<Call>& expected,
    const char* description
) {
    RecordingPorts ports;
    openswd3::app::run_idle_iteration(state, ports);
    test.expect_equal(ports.calls, expected, description);
}

}  // namespace

int main() {
    openswd3::test::Context test;
    expect_calls(
        test,
        {1, 0x20, 0, 1},
        {Call::step_video, Call::maintain_audio},
        "video is followed immediately by one audio maintenance call"
    );
    expect_calls(
        test,
        {1, 0x01, 0, 1},
        {Call::yield},
        "idle suppression performs only one yield"
    );
    expect_calls(
        test,
        {1, 0, 0, 1},
        {Call::step_game_frame},
        "normal idle iteration enters one game frame"
    );
    expect_calls(
        test,
        {0, 0x20, 0, 1},
        {Call::present_pause},
        "paused display does not step video or maintain audio"
    );
    return test.exit_code();
}
