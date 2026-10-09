#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorGroupBElementState;
class LegacyBattleGroupBStorage;

enum class LegacyBattleGroupBResourceCleanupStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupBResourceCleanupResult {
    LegacyBattleGroupBResourceCleanupStatus status{
        LegacyBattleGroupBResourceCleanupStatus::completed
    };
    bool resource_released{};
};

[[nodiscard]] LegacyBattleGroupBResourceCleanupResult
release_legacy_battle_group_b_resource(
    LegacyBattleActorGroupBElementState* state,
    LegacyBattleGroupBStorage* resources,
    compat::u32 actor_token
);

}  // namespace openswd3::battle
