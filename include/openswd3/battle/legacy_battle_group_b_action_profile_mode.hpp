#pragma once

#include "openswd3/battle/legacy_battle_actor_action_mode.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupBActionProfileModeRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    std::array<LegacyBattleActorActionModeRequest, 2> action_mode_requests{};
};

enum class LegacyBattleGroupBActionProfileModeStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_state_typed_stop,
    profile_load_typed_stop,
    action_mode_typed_stop,
};

struct LegacyBattleGroupBActionProfileModeResult {
    LegacyBattleGroupBActionProfileModeStatus status{
        LegacyBattleGroupBActionProfileModeStatus::completed
    };
    compat::u32 profile_load_calls{};
    compat::u32 profile_dwords_cleared{};
    compat::u32 mode_update_calls{};
    LegacyBattleActorActionModeResult actor_action_mode{};
    std::array<LegacyBattleActorActionModeResult, 2> actor_action_modes{};
    compat::u16 profile_id{};
    compat::u16 resource_word{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4761D0. The fixed mode-one and mode-two paths and the closed MON.DAT
// profile loader are expanded directly.
[[nodiscard]] LegacyBattleGroupBActionProfileModeResult
compose_legacy_battle_group_b_action_profile_mode(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionProfileModeRequest& request
);

}  // namespace openswd3::battle
