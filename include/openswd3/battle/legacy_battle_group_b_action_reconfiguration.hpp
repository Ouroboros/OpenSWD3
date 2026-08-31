#pragma once

#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBActionReconfigurationRequest {
    compat::u32 definition_argument{};
    compat::u32 actor_token{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBActionReconfigurationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_load_typed_stop,
    resource_read_typed_stop,
    profile_load_typed_stop,
    resource_release_typed_stop,
};

struct LegacyBattleGroupBActionReconfigurationResult {
    LegacyBattleGroupBActionReconfigurationStatus status{
        LegacyBattleGroupBActionReconfigurationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_475820.
[[nodiscard]] LegacyBattleGroupBActionReconfigurationResult
reconfigure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleGroupBActionConfigurationPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionReconfigurationRequest& request
);

}  // namespace openswd3::battle
