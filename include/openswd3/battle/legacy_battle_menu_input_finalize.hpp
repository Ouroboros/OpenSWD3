#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

namespace openswd3::battle {

struct LegacyBattleMenuInputFinalizeBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleMenuInputFinalizeRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMenuInputFinalizeStatus : compat::u8 {
    completed,
    selected_group_b_actor_typed_stop,
    active_group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    group_a_marker_typed_stop,
    group_a_selection_cache_typed_stop,
    attack_order_cache_typed_stop,
};

struct LegacyBattleMenuInputFinalizeResult {
    LegacyBattleMenuInputFinalizeStatus status{
        LegacyBattleMenuInputFinalizeStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 active_group_a_reset_calls{};
    compat::u32 actor_reset_calls{};
};

// Typed closure of legacy 0x00461C10.
[[nodiscard]] LegacyBattleMenuInputFinalizeResult
finalize_legacy_battle_menu_input(
    LegacyBattleMenuInputFinalizeBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuInputFinalizeRequest& request
);

}  // namespace openswd3::battle
