#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleActorGroupAStaticLifecyclePort& lifecycle_port
) {
    LegacyBattleActorGroupAStaticInitializationResult result;
    lifecycle_port.construct_group();
    result.construct_calls = 1U;
    result.return_value = lifecycle_port.register_exit_cleanup();
    result.exit_registration_calls = 1U;
    return result;
}

}  // namespace openswd3::battle
