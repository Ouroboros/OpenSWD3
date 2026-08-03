#include "openswd3/app/frame_runtime.hpp"

#include "openswd3/app/frame_dispatch.hpp"

namespace openswd3::app {

namespace {

void run_common_tail(FrameCoordinatorState& state, FrameRuntimePorts& ports) {
    ports.maintain_audio();
    if (should_request_close(state.process_flags)) {
        ports.request_synchronous_close();
    }
}

}  // namespace

FrameRunOutcome run_accepted_frame(
    FrameCoordinatorState& state,
    FrameRuntimePorts& ports
) {
    if (state.battle.high_priority_state != 0U) {
        ports.step_high_priority(state);
        run_common_tail(state, ports);
        return FrameRunOutcome::common_tail_completed;
    }

    static_cast<void>(
        consume_battle_request(
            state.battle,
            state.battle_entry_blocked,
            ports
        )
    );
    if (state.battle.battle_active != 0U) {
        static_cast<void>(run_battle_frame(state.battle, ports));
        return FrameRunOutcome::battle_early_return;
    }

    ports.update_background_music(state);
    if (state.battle.special_mode_state == 0U) {
        ports.step_world_interaction(state);
        ports.step_world_player(state);
        if (should_step_story(
                {
                    state.frame_execution_gate,
                    state.transition_suppression,
                    state.battle.special_mode_state,
                    state.battle.battle_active,
                    state.battle.high_priority_state,
                }
            )) {
            ports.step_story(state);
            if ((state.process_flags & 0x05U) == 0U) {
                ports.finish_world_frame(state);
            }
        }
    } else {
        ports.maintain_audio();
        ports.prepare_special_mode_objects(state);
        switch (select_special_mode_handler(state.battle.special_mode_state)) {
            case SpecialModeHandler::none:
                break;
            case SpecialModeHandler::standard_modes_1_3_4_5_6:
                ports.step_standard_special_mode(state);
                break;
            case SpecialModeHandler::shop_mode_2:
                ports.step_shop_mode(state);
                break;
        }
    }

    run_common_tail(state, ports);
    return FrameRunOutcome::common_tail_completed;
}

}  // namespace openswd3::app
