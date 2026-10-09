#pragma once

#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp"

namespace openswd3::battle {

enum class LegacyBattleActorMessagePercentRefreshStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    fixed_record_typed_stop,
};

struct LegacyBattleActorMessagePercentRefreshResult {
    LegacyBattleActorMessagePercentRefreshStatus status{
        LegacyBattleActorMessagePercentRefreshStatus::completed
    };
    compat::u16 message_percent{};
};

[[nodiscard]] LegacyBattleActorMessagePercentRefreshResult
refresh_legacy_battle_actor_message_percent(
    LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleGroupAAttributeAggregationState& attributes,
    LegacyBattleFixedObjectState& fixed_objects
) noexcept;

}  // namespace openswd3::battle
