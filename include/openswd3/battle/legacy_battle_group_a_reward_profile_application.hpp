#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <list>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupARewardProfileListToken =
    0x004B8A00U;
inline constexpr compat::u32 kLegacyBattleGroupARewardProfileNodeSize = 0x14U;

struct LegacyBattleGroupARewardProfileNode {
    compat::u32 legacy_token{};
    compat::u32 legacy_next_token{};
    compat::u16 item_id{};
    compat::u16 quantity{};
    compat::u16 percentage{};
    compat::u16 blocking_flag{};
};

struct LegacyBattleGroupARewardProfileState {
    LegacyBattleGroupARewardProfileNode head{
        .legacy_token = kLegacyBattleGroupARewardProfileListToken,
    };
    std::list<LegacyBattleGroupARewardProfileNode> nodes;
};

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
    compat::u32 port_calls{};
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
