#include "openswd3/battle/legacy_battle_actor_turn_completion.hpp"

namespace openswd3::battle {

LegacyBattleActorTurnCompletionResult query_legacy_battle_actor_turn_completion(
    const compat::u32* turn_completion,
    const LegacyBattleActorTurnCompletionAccess access
) noexcept {
    if (turn_completion == nullptr || !access.latch_readable) {
        return {
            .status =
                LegacyBattleActorTurnCompletionStatus::latch_read_typed_stop
        };
    }

    const compat::u32 value = *turn_completion;
    if (!access.return_address_readable) {
        return {
            .status = LegacyBattleActorTurnCompletionStatus::
                return_address_read_typed_stop,
            .value = value,
        };
    }

    return {.value = value};
}
}
