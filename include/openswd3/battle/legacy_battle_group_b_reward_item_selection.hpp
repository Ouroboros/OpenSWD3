#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBRewardItemSelectionRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBRewardItemSelectionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
};

struct LegacyBattleGroupBRewardItemSelectionResult {
    LegacyBattleGroupBRewardItemSelectionStatus status{
        LegacyBattleGroupBRewardItemSelectionStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 random_value{};
    compat::u16 item_id{};
    compat::u16 threshold{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x004762F0.
[[nodiscard]] LegacyBattleGroupBRewardItemSelectionResult
select_legacy_battle_group_b_reward_item(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleGroupBRewardItemSelectionRequest& request = {}
);

}  // namespace openswd3::battle
