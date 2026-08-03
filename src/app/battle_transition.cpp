#include "openswd3/app/battle_transition.hpp"

namespace openswd3::app {

bool consume_battle_request(
    BattleTransitionState& state,
    const bool battle_entry_blocked,
    BattleTransitionPorts& ports
) {
    if (state.battle_active != 0U || battle_entry_blocked ||
        (state.battle_request_value & kBattleRequestTag) == 0U) {
        return false;
    }

    state.battle_request_value &= kBattleRequestValueMask;
    if (state.battle_request_value == 0U) {
        return false;
    }

    ports.release_display_and_world_for_battle_entry();
    ports.close_world_map_view();
    ports.initialize_battle(
        static_cast<compat::u16>(state.battle_request_value & 0xFFFFU)
    );
    state.battle_active = 1U;
    ports.clear_party_battle_entry_bits();
    return true;
}

compat::i32 run_battle_frame(
    BattleTransitionState& state,
    BattleTransitionPorts& ports
) {
    const compat::i32 result = ports.step_battle();
    ports.maintain_audio();

    switch (result) {
        case 0:
            state.battle_request_value = 0U;
            state.battle_active = 0U;
            ports.rebuild_display_after_result_zero();
            ports.set_result_zero_world_state();
            ports.reopen_world_map_after_result_zero();
            ports.resume_audio_after_result_zero();
            return result;
        case 2:
            ports.prepare_result_two_internal_state();
            state.battle_request_value = 0U;
            state.battle_active = 0U;
            state.special_mode_state = kBattleResultTwoSpecialMode;
            state.high_priority_state = 0U;
            ports.clear_result_two_auxiliary_state();
            ports.finish_result_two_mode_transition();
            return result;
        case 3:
            state.battle_request_value = 0U;
            state.battle_active = 0U;
            ports.clear_result_three_internal_state();
            ports.remap_world_after_result_three();
            return result;
        default:
            return result;
    }
}

}  // namespace openswd3::app
