#pragma once

#include "openswd3/battle/legacy_battle_group_b_frame.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActorFrameAdvanceContext {
    LegacyBattleGroupBFrameState& state;
    LegacyBattleActionDispatchPort& port;
    LegacyBattleActionDispatchContext& dispatch;
};

enum class LegacyBattleActorFrameSequenceStatus : compat::u8 {
    completed,
    actor_order_typed_stop,
    frame_context_typed_stop,
    shared_state_typed_stop,
    group_a_typed_stop,
    group_b_typed_stop,
};

struct LegacyBattleActorFrameSequenceResult {
    LegacyBattleActorFrameSequenceStatus status{
        LegacyBattleActorFrameSequenceStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 initial_count{};
    compat::u32 scanned_slots{};
    compat::u32 group_a_calls{};
    compat::u32 group_b_calls{};
    std::array<LegacyBattleActionDispatchResult, 18> frame_results{};
    compat::u32 frame_result_count{};
};

[[nodiscard]] LegacyBattleActorFrameSequenceResult
advance_legacy_battle_actor_frame_sequence(
    LegacyBattleActorMetricState& metric_state,
    LegacyBattleActorFrameAdvanceContext* frame_context,
    compat::u32 caller_edx = 0U
);

}  // namespace openswd3::battle
