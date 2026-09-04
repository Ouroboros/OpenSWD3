#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <memory>

namespace openswd3::battle {

enum class LegacyBattleGroupBActionConfigurationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    source_record_typed_stop,
    resource_load_typed_stop,
    resource_read_typed_stop,
    profile_load_typed_stop,
    resource_release_typed_stop,
};

struct LegacyBattleGroupBActionConfigurationResult {
    LegacyBattleGroupBActionConfigurationStatus status{
        LegacyBattleGroupBActionConfigurationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 copied_dwords{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_475720.
[[nodiscard]] LegacyBattleGroupBActionConfigurationResult
configure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBActionRecord* source,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 definition_argument,
    compat::u32 actor_token,
    compat::u32 source_token
);

}  // namespace openswd3::battle
