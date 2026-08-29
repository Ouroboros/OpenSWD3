#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorListIndexCommitRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorListIndexCommitStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleActorListIndexCommitResult {
    LegacyBattleActorListIndexCommitStatus status{
        LegacyBattleActorListIndexCommitStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 writes{};
};

// sub_46FFE0.
[[nodiscard]] LegacyBattleActorListIndexCommitResult
commit_legacy_battle_actor_list_index(
    LegacyBattleGroupAActionExecutionState* state,
    compat::u32 actor_token,
    const LegacyBattleActorListIndexCommitRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
