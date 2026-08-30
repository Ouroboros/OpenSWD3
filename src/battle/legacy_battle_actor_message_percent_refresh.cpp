#include "openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp"

namespace openswd3::battle {

LegacyBattleActorMessagePercentRefreshResult
refresh_legacy_battle_actor_message_percent(
    LegacyBattleGroupAActionExecutionState* const actor,
    LegacyBattleActorMessagePercentRefreshPort& port,
    const LegacyBattleActorMessagePercentRefreshRequest& request
) {
    LegacyBattleActorMessagePercentRefreshResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };

    const auto reply = port.invoke_actor_message_percent_refresh({
        .callee_token = kLegacyBattleActorMessagePercentRefreshCalleeToken,
        .actor_token = request.actor_token,
        .refresh_argument = 30U,
        .eax = result.return_eax,
        .ecx = request.actor_token,
        .edx = result.return_edx,
    });
    ++result.percent_refresh_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;

    if (actor == nullptr || request.actor_token == 0U) {
        result.status = LegacyBattleActorMessagePercentRefreshStatus::
            actor_state_typed_stop;
        return result;
    }

    if (reply.publish_message_percent) {
        actor->message_percent = reply.message_percent;
    }
    result.return_eax =
        (result.return_eax & 0xFFFF0000U) | actor->message_percent;
    return result;
}

}  // namespace openswd3::battle
