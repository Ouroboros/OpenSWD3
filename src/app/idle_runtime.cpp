#include "openswd3/app/idle_runtime.hpp"

namespace openswd3::app {

void run_idle_iteration(const IdleState& state, IdleRuntimePorts& ports) {
    switch (select_idle_action(state)) {
    case IdleAction::step_video_then_audio:
        ports.step_video();
        ports.maintain_audio();
        return;
    case IdleAction::yield:
        ports.yield();
        return;
    case IdleAction::step_game_frame:
        ports.step_game_frame();
        return;
    case IdleAction::present_pause:
        ports.present_pause();
        return;
    }
}

}  // namespace openswd3::app
