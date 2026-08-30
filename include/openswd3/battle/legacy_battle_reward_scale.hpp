#pragma once

#include "openswd3/battle/legacy_battle_group_effect_frame.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleRewardScaleActorState {
    compat::u8 status_bits{};  // actor + 0x26C8
    compat::u16 percent{};     // actor + 0x26DC
};

struct LegacyBattleRewardScaleRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleRewardScaleStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    value_typed_stop,
};

struct LegacyBattleRewardScaleResult {
    LegacyBattleRewardScaleStatus status{
        LegacyBattleRewardScaleStatus::completed
    };
    compat::u32 percent_refresh_calls{};
    compat::u32 action_configure_calls{};
    compat::u32 port_calls{};
    compat::u32 scaled_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_472C70.
[[nodiscard]] LegacyBattleRewardScaleResult scale_legacy_battle_reward(
    LegacyBattleRewardScaleActorState* actor,
    compat::u32* value,
    LegacyBattleEffectCallPort& port,
    const LegacyBattleRewardScaleRequest& request
);

}  // namespace openswd3::battle
