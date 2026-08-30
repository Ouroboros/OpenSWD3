#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"

namespace openswd3::battle {

enum class LegacyBattleGroupBOpponentModeStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
};

struct LegacyBattleGroupBOpponentModeResult {
    LegacyBattleGroupBOpponentModeStatus status{
        LegacyBattleGroupBOpponentModeStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 random_value{};
    compat::u32 normalized_random{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool return_ecx_known{true};
};

// Typed closure of legacy 0x00476080. One call consumes exactly one bounded
// secondary-RNG value and may publish opponent mode 1 or 2.
[[nodiscard]] LegacyBattleGroupBOpponentModeResult
select_legacy_battle_group_b_opponent_mode(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleBoundedRandomPort& random
);

}  // namespace openswd3::battle
