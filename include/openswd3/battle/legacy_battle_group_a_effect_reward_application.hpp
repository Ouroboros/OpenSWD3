#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupAEffectRewardApplicationRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAEffectRewardApplicationStatus : compat::u8 {
    completed,
    actor_profile_typed_stop,
    destination_record_typed_stop,
    profile_list_typed_stop,
    profile_node_typed_stop,
    allocation_typed_stop,
    host_allocation_typed_stop,
};

struct LegacyBattleGroupAEffectRewardApplicationResult {
    LegacyBattleGroupAEffectRewardApplicationStatus status{
        LegacyBattleGroupAEffectRewardApplicationStatus::completed
    };
    compat::u32 profiles_visited{};
    compat::u32 nonzero_profiles{};
    compat::u32 kind_matches{};
    compat::u32 destination_gate_matches{};
    compat::u32 address_gate_matches{};
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

// sub_46F6E0.
[[nodiscard]] LegacyBattleGroupAEffectRewardApplicationResult
apply_legacy_battle_group_a_effect_rewards(
    LegacyBattleGroupARewardProfileState* state,
    const std::array<LegacyBattleGroupASummonProfileRecord, 2>* profiles,
    const compat::u16* destination_argument_word,
    compat::u32 actor_token,
    compat::u32 profile_list_token,
    compat::u32 destination_token,
    LegacyBattleEffectCallPort& port,
    const LegacyBattleGroupAEffectRewardApplicationRequest& request = {}
);

}  // namespace openswd3::battle
