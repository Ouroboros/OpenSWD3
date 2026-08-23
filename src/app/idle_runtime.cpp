#include "openswd3/app/idle_runtime.hpp"

namespace openswd3::app {

void run_idle_iteration(const IdleState& state, IdleRuntimePorts& ports) {
    switch (select_idle_action(state)) {
    case IdleAction::step_video_then_audio:
        ports.step_video();
        ports.maintain_audio();
        break;

    case IdleAction::yield:
        ports.yield();
        break;

    case IdleAction::step_game_frame:
        ports.step_game_frame();
        break;

    case IdleAction::present_pause:
        ports.present_pause();
        break;
    }

    ports.refresh_display();
}

}  // namespace openswd3::app
