#include "openswd3/battle/legacy_battle_actor_list_index_commit.hpp"

namespace openswd3::battle {

LegacyBattleActorListIndexCommitResult commit_legacy_battle_actor_list_index(
    LegacyBattleGroupAActionExecutionState* state,
    const compat::u32 actor_token,
    const LegacyBattleActorListIndexCommitRequest& request
) noexcept {
    LegacyBattleActorListIndexCommitResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || state == nullptr) {
        result.status =
            LegacyBattleActorListIndexCommitStatus::actor_state_typed_stop;
        return result;
    }

    result.return_eax = state->next_list_index;
    state->current_list_index = result.return_eax;
    result.writes = 1U;
    return result;
}

}  // namespace openswd3::battle
