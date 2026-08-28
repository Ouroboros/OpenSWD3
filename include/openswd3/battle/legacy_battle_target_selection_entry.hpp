#pragma once

#include "openswd3/battle/legacy_battle_target_selection_refresh.hpp"

namespace openswd3::battle {

struct LegacyBattleTargetSelectionEntryBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleTextMessageState& text_messages;
    const LegacyBattleActionModeSourceState& action_mode_source;
    const std::array<compat::u8, 4>& startup_party_presence;
    const compat::u32& startup_mode_flags;
    compat::u16& startup_supplemental_count_word;
    compat::u32& startup_mirror_mode;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleInputDispatchState& input_dispatch;
    std::span<input_time_rng::LegacyInputRecord> input_records;
    LegacyBattleTargetSelectionRuntimeState& target_selection_runtime;
    story_scene::LegacyDialogRuntimeState& dialogs;
    compat::u32& one_shot_interaction_state;
    compat::u32& target_ready_gate;
    compat::u32& outcome_darkening_gate;
    compat::u32& message_state;
};

struct LegacyBattleTargetSelectionEntryRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetSelectionEntryStatus : compat::u8 {
    completed,
    active_group_a_actor_typed_stop,
    selected_group_b_actor_typed_stop,
    action_mode_refresh_typed_stop,
    target_selection_refresh_typed_stop,
};

struct LegacyBattleTargetSelectionEntryResult {
    LegacyBattleTargetSelectionEntryStatus status{
        LegacyBattleTargetSelectionEntryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
    compat::u32 primary_scan_calls{};
    compat::u32 secondary_scan_calls{};
    compat::u32 action_mode_refresh_calls{};
    compat::u32 target_selection_refresh_calls{};
};

// Typed closure of legacy 0x004620D0.
[[nodiscard]] LegacyBattleTargetSelectionEntryResult
enter_legacy_battle_target_selection(
    LegacyBattleTargetSelectionEntryBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleTargetSelectionEntryRequest& request
);

}  // namespace openswd3::battle
