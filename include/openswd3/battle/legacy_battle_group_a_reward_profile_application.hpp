#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupARewardProfileApplicationRequest {
    compat::u32 quantity{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupARewardProfileApplicationStatus : compat::u8 {
    completed,
    actor_profile_typed_stop,
    profile_list_typed_stop,
    profile_node_typed_stop,
    allocation_typed_stop,
    host_allocation_typed_stop,
};

struct LegacyBattleGroupARewardProfileApplicationResult {
    LegacyBattleGroupARewardProfileApplicationStatus status{
        LegacyBattleGroupARewardProfileApplicationStatus::completed
    };
    compat::u32 profiles_visited{};
    compat::u32 nonzero_profiles{};
    compat::u32 traversed_nodes{};
    compat::u32 matched_profiles{};
    compat::u32 blocked_profiles{};
    compat::u32 quantity_writes{};
    compat::u32 percentage_writes{};
    compat::u32 allocation_calls{};
    compat::u32 created_nodes{};
    compat::u32 head_item_id_increments{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46F5B0.
[[nodiscard]] LegacyBattleGroupARewardProfileApplicationResult
apply_legacy_battle_group_a_reward_profiles(
    LegacyBattleGroupARewardProfileState* state,
    const std::array<LegacyBattleGroupASummonProfileRecord, 2>* profiles,
    compat::u32 actor_token,
    compat::u32 profile_list_token,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleGroupARewardProfileApplicationRequest& request
);

}  // namespace openswd3::battle
