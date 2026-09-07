#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

namespace openswd3::battle {

struct LegacyBattleMenuSelectionRetreatBindings {
    LegacyBattleStartupState& startup;
    LegacyBattleStartupResetBlocks& startup_reset;
    compat::u16& startup_supplemental_count_word;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleMenuSelectionRetreatRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleActorFrameSnapshotRequest actor_frame_snapshot{};
};

enum class LegacyBattleMenuSelectionRetreatStatus : compat::u8 {
    completed,
    permission_typed_stop,
    startup_mode_typed_stop,
    group_b_order_typed_stop,
    group_b_actor_typed_stop,
    actor_order_typed_stop,
    group_a_actor_typed_stop,
    actor_frame_snapshot_typed_stop,
    target_marker_typed_stop,
    equipment_selection_typed_stop,
    equipment_scroll_typed_stop,
};

struct LegacyBattleMenuSelectionRetreatResult {
    LegacyBattleMenuSelectionRetreatStatus status{
        LegacyBattleMenuSelectionRetreatStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
    compat::u32 actor_iterations{};
    compat::u32 actor_frame_snapshot_queries{};
    compat::u32 actor_frame_snapshot_actor_token{};
    compat::u32 actor_frame_snapshot_entry_eax{};
    compat::u32 actor_frame_snapshot_entry_ecx{};
    compat::u32 actor_frame_snapshot_entry_edx{};
    LegacyBattleActorFrameSnapshotResult actor_frame_snapshot{};
};

// Typed closure of legacy 0x00460C40.
[[nodiscard]] LegacyBattleMenuSelectionRetreatResult
retreat_legacy_battle_menu_selection(
    LegacyBattleMenuSelectionRetreatBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuSelectionRetreatRequest& request
);

}  // namespace openswd3::battle
