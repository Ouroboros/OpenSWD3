#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleGroupAStorage;

struct LegacyBattleGroupAResourceCleanupState {
    compat::u32 primary_resource_token{};
    compat::u32 secondary_resource_token{};
};

enum class LegacyBattleGroupAResourceCleanupStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupAResourceCleanupResult {
    LegacyBattleGroupAResourceCleanupStatus status{
        LegacyBattleGroupAResourceCleanupStatus::completed
    };
    bool secondary_resource_released{};
    bool primary_resource_released{};
};

[[nodiscard]] LegacyBattleGroupAResourceCleanupResult
release_legacy_battle_group_a_resources(
    compat::u32& primary_resource_token,
    compat::u32& secondary_resource_token,
    LegacyBattleGroupAStorage* resources,
    compat::u32 actor_token
);

[[nodiscard]] LegacyBattleGroupAResourceCleanupResult
release_legacy_battle_group_a_resources(
    LegacyBattleGroupAResourceCleanupState* state,
    LegacyBattleGroupAStorage* resources,
    compat::u32 actor_token
);

}  // namespace openswd3::battle
