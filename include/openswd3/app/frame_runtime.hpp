#pragma once

#include "openswd3/app/battle_transition.hpp"
#include "openswd3/app/legacy_new_game_transition.hpp"

namespace openswd3::app {

struct FrameCoordinatorState {
    compat::u32 frame_execution_gate{};
    compat::u32 process_flags{};
    compat::u32 transition_suppression{};
    bool battle_entry_blocked{};
    BattleTransitionState battle;
};

class FrameRuntimePorts : public BattleTransitionPorts,
                          public LegacyNewGameTransitionPorts {
public:
    virtual void step_high_priority(FrameCoordinatorState& state) = 0;
    virtual void update_background_music(FrameCoordinatorState& state) = 0;
    virtual void step_world_interaction(FrameCoordinatorState& state) = 0;
    virtual void step_world_player(FrameCoordinatorState& state) = 0;
    virtual void step_story(FrameCoordinatorState& state) = 0;
    virtual void finish_world_frame(FrameCoordinatorState& state) = 0;
    virtual void prepare_special_mode_objects(FrameCoordinatorState& state) = 0;
    [[nodiscard]] virtual StandardSpecialModeEvent step_standard_special_mode(
        FrameCoordinatorState& state
    ) = 0;
    virtual void step_shop_mode(FrameCoordinatorState& state) = 0;
    virtual void request_synchronous_close() = 0;
};

enum class FrameRunOutcome {
    common_tail_completed,
    battle_early_return,
};

[[nodiscard]] FrameRunOutcome run_accepted_frame(
    FrameCoordinatorState& state,
    FrameRuntimePorts& ports
);

}  // namespace openswd3::app
