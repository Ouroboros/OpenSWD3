#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"

namespace openswd3::battle {

enum class LegacyBattleGroupBStatusActionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
    resource_reread_typed_stop,
};

struct LegacyBattleGroupBStatusActionRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBStatusActionResult {
    LegacyBattleGroupBStatusActionStatus status{
        LegacyBattleGroupBStatusActionStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 initial_random_value{};
    compat::u32 decision_random_value{};
    compat::u8 argument{};
    compat::u8 initial_resource_chance{};
    compat::u16 resource_base{};
    compat::i32 signed_delta{};
    compat::u16 decision_threshold{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool return_ecx_known{true};
};

// Typed closure of legacy 0x00476330. The entry actor gate may skip all
// random work; otherwise the first bounded draw is consumed before the first
// resource read and a second draw is used only by one of three decision bands.
[[nodiscard]] LegacyBattleGroupBStatusActionResult
query_legacy_battle_group_b_status_action(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleGroupBStatusActionRequest request = {}
);

}  // namespace openswd3::battle
