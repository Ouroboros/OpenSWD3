#include "openswd3/app/legacy_new_game_transition.hpp"

namespace openswd3::app {

LegacyNewGameTransitionResult run_legacy_new_game_transition(
    BattleTransitionState& state,
    LegacyNewGameTransitionPorts& ports
) {
    LegacyNewGameTransitionResult result;

    ports.clear_accumulated_play_time();
    result.sampled_epoch_seconds = ports.sample_epoch_seconds();
    ports.set_play_time_origin(result.sampled_epoch_seconds);

    ports.set_initial_menu_phase(5U);
    ports.clear_game_framebuffer();
    result.initial_world_ready =
        ports.initialize_new_game_state_and_world();

    // sub_4490C0 does not inspect the return from sub_40F160.  Preserve the
    // remaining state writes and calls even when a modern checked loader has
    // reported failure.
    state.special_mode_state = 0U;
    state.high_priority_state = 0U;
    ports.set_high_priority_submode(1U);
    ports.set_high_priority_auxiliary(1U);
    ports.reset_input_menu_and_save_previews();
    ports.apply_new_game_name_overrides();
    ports.load_fame_table();

    return result;
}

}  // namespace openswd3::app
