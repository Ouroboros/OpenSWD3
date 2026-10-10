#pragma once

#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

#include <optional>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleActorMetricState;
struct LegacyBattleFinalActorStepState;
struct LegacyBattleScriptWorkspace;

struct LegacyBattleFrameSelectionBindings {
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleScriptWorkspace& script_workspace;
    compat::u16& delay;
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<LegacyBattleIntensityEffectRecord> adjacent_intensity_records;
    std::span<const LegacyBattlePartyStartupRecord> party;
};

enum class LegacyBattleFrameSelectionStatus : compat::u8 {
    completed,
    queue_head_typed_stop,
    dequeue_typed_stop,
};

struct LegacyBattleFrameSelectionResult {
    LegacyBattleFrameSelectionStatus status{
        LegacyBattleFrameSelectionStatus::completed
    };
    std::optional<LegacyBattleAttackOrderDequeueResult> dequeue;
};

[[nodiscard]] LegacyBattleFrameSelectionResult
prepare_legacy_battle_frame_selection(
    LegacyBattleFrameSelectionBindings bindings
);

}  // namespace openswd3::battle
