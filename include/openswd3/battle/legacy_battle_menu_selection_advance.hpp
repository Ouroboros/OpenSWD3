#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

namespace openswd3::battle {

struct LegacyBattleMenuSelectionAdvanceBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    compat::u16& startup_supplemental_count_word;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleMenuSelectionAdvanceRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMenuSelectionAdvanceStatus : compat::u8 {
    completed,
    permission_typed_stop,
    startup_mode_typed_stop,
    group_b_order_typed_stop,
    group_b_actor_typed_stop,
    actor_order_typed_stop,
    group_a_actor_typed_stop,
    target_marker_typed_stop,
    equipment_selection_typed_stop,
    equipment_scroll_typed_stop,
};

struct LegacyBattleMenuSelectionAdvanceResult {
    LegacyBattleMenuSelectionAdvanceStatus status{
        LegacyBattleMenuSelectionAdvanceStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
    compat::u32 actor_iterations{};
};

// Typed closure of legacy 0x00461240.
[[nodiscard]] LegacyBattleMenuSelectionAdvanceResult
advance_legacy_battle_menu_selection(
    LegacyBattleMenuSelectionAdvanceBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuSelectionAdvanceRequest& request
);

}  // namespace openswd3::battle
