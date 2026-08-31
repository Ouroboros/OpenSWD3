#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

enum class LegacyBattleGroupBActionSixTargetAvailabilityStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
};

struct LegacyBattleGroupBActionSixTargetAvailabilityRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionSixTargetAvailabilityResult {
    LegacyBattleGroupBActionSixTargetAvailabilityStatus status{
        LegacyBattleGroupBActionSixTargetAvailabilityStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 resource_flags{};
    compat::u16 resource_threshold{};
};

[[nodiscard]] LegacyBattleGroupBActionSixTargetAvailabilityResult
query_legacy_battle_group_b_action_six_target_availability(
    const LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBActionSixTargetAvailabilityRequest& request = {}
);

}  // namespace openswd3::battle
