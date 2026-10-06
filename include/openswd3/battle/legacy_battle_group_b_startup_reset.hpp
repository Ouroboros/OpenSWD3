#pragma once

#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

namespace openswd3::battle {

struct LegacyBattleActorGroupBElementState;
struct LegacyBattleActorProgressState;
struct LegacyBattleRewardScaleActorState;
struct LegacyBattleTargetPhaseState;

// All state arguments are borrowed from the existing actor and action owners.
// Every successful guest store is applied before the next access or callback.
[[nodiscard]] LegacyBattleActorStartupResetResult
reset_legacy_battle_group_b_for_startup(
    LegacyBattleActorGroupBElementState& actor,
    LegacyBattleActorProgressState& progress,
    LegacyBattleRewardScaleActorState& reward,
    LegacyBattleTargetPhaseState& particle,
    LegacyBattleActorStartupResetHeapPort& heap,
    compat::u32 entry_edx
);

}  // namespace openswd3::battle
