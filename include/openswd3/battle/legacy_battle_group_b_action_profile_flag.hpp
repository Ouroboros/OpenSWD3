#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBActionProfileFlagRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBActionProfileFlagStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupBActionProfileFlagResult {
    LegacyBattleGroupBActionProfileFlagStatus status{
        LegacyBattleGroupBActionProfileFlagStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476140. The primary profile flag wins; otherwise
// the fallback profile flag is returned as zero or one.
[[nodiscard]] LegacyBattleGroupBActionProfileFlagResult
query_legacy_battle_group_b_action_profile_flag(
    const LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBActionProfileFlagRequest& request
) noexcept;

}  // namespace openswd3::battle
