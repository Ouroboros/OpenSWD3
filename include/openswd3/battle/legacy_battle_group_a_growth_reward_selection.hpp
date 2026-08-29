#pragma once

#include "openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleGroupAGrowthRewardSelectionStatus : compat::u8 {
    completed,
    actor_profile_typed_stop,
    profile_list_typed_stop,
    profile_node_typed_stop,
};

struct LegacyBattleGroupAGrowthRewardSelectionResult {
    LegacyBattleGroupAGrowthRewardSelectionStatus status{
        LegacyBattleGroupAGrowthRewardSelectionStatus::completed
    };
    compat::u32 profiles_visited{};
    compat::u32 nonzero_profiles{};
    compat::u32 traversed_nodes{};
    compat::u32 matching_nodes{};
    compat::u32 blocked_nodes{};
    compat::u32 insufficient_nodes{};
    compat::u32 quantity_writes{};
    compat::u32 blocking_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46F850.
[[nodiscard]] LegacyBattleGroupAGrowthRewardSelectionResult
select_legacy_battle_group_a_growth_reward(
    LegacyBattleGroupARewardProfileState* state,
    const std::array<LegacyBattleGroupASummonProfileRecord, 2>* profiles,
    compat::u32 actor_token,
    compat::u32 profile_list_token
) noexcept;

}  // namespace openswd3::battle
