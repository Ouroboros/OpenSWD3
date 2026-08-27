#pragma once

#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_input_record_priming.hpp"

namespace openswd3::battle {

struct LegacyBattleTargetSelectionRefreshBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    compat::u16& startup_supplemental_count_word;
    compat::u32& startup_mirror_mode;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleInputDispatchState& input_dispatch;
    std::span<input_time_rng::LegacyInputRecord> input_records;
    LegacyBattleTargetSelectionRuntimeState& runtime;
    compat::u32& target_ready_gate;
    compat::u32& message_state;
};

struct LegacyBattleTargetSelectionRefreshRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetSelectionRefreshStatus : compat::u8 {
    completed,
    action_remap_typed_stop,
    target_actor_index_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    actor_runtime_record_typed_stop,
    action_workspace_typed_stop,
    target_marker_typed_stop,
    actor_result_word_typed_stop,
    input_record_typed_stop,
};

struct LegacyBattleTargetSelectionRefreshResult {
    LegacyBattleTargetSelectionRefreshStatus status{
        LegacyBattleTargetSelectionRefreshStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
    compat::u32 group_a_calls{};
    compat::u32 group_b_calls{};
    compat::u32 actor_runtime_reads{};
    compat::u32 actor_runtime_writes{};
    compat::u32 workspace_reads{};
    compat::u32 workspace_writes{};
    compat::u32 input_record_prime_calls{};
    compat::u32 input_record_writes{};
};

// Typed closure of legacy 0x00462740.
[[nodiscard]] LegacyBattleTargetSelectionRefreshResult
refresh_legacy_battle_target_selection(
    LegacyBattleTargetSelectionRefreshBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleTargetSelectionRefreshRequest& request
);

}  // namespace openswd3::battle
