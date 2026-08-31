#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

enum class LegacyBattleGroupBOpponentWaveParametersStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    first_output_write_typed_stop,
    second_output_write_typed_stop,
};

struct LegacyBattleGroupBOpponentWaveParametersRequest {
    compat::u32 actor_token{};
    compat::u32 first_output_token{};
    compat::u32 second_output_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBOpponentWaveParametersResult {
    LegacyBattleGroupBOpponentWaveParametersStatus status{
        LegacyBattleGroupBOpponentWaveParametersStatus::completed
    };
    compat::u16 special_action{};
    compat::u8 spawn_count{};
    compat::u32 first_output_writes{};
    compat::u32 second_output_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_476900.
[[nodiscard]] LegacyBattleGroupBOpponentWaveParametersResult
read_legacy_battle_group_b_opponent_wave_parameters(
    const LegacyBattleActorGroupBElementState* actor,
    compat::u16* special_action_output,
    compat::u16* spawn_count_output,
    const LegacyBattleGroupBOpponentWaveParametersRequest& request = {}
);

}  // namespace openswd3::battle
