#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

LegacyBattleActorGroupAConstructionResult construct_legacy_battle_actor_group_a(
    LegacyBattleActorVectorConstructionPort& construction_port
) {
    LegacyBattleActorGroupAConstructionResult result{
        .request = {
            .base_token = kLegacyBattleActorGroupABaseToken,
            .element_size = kLegacyBattleActorGroupAElementSize,
            .element_count = kLegacyBattleActorGroupAElementCount,
            .constructor_token = kLegacyBattleActorGroupAConstructorToken,
            .destructor_token = kLegacyBattleActorGroupADestructorToken,
        },
    };
    result.return_value = construction_port.construct_vector(result.request);
    result.vector_constructor_calls = 1U;
    return result;
}

LegacyBattleActorGroupADestructionResult release_legacy_battle_actor_group_a(
    LegacyBattleActorVectorDestructionPort& destruction_port
) {
    LegacyBattleActorGroupADestructionResult result{
        .request = {
            .base_token = kLegacyBattleActorGroupABaseToken,
            .element_size = kLegacyBattleActorGroupAElementSize,
            .element_count = kLegacyBattleActorGroupAElementCount,
            .destructor_token = kLegacyBattleActorGroupADestructorToken,
        },
    };
    result.return_value = destruction_port.destroy_vector(result.request);
    result.vector_destructor_calls = 1U;
    return result;
}

LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleActorVectorConstructionPort& construction_port,
    LegacyBattleActorGroupAExitRegistrationPort& exit_registration_port
) {
    LegacyBattleActorGroupAStaticInitializationResult result;
    const auto construction =
        construct_legacy_battle_actor_group_a(construction_port);
    result.construct_calls = 1U;
    result.construction_return_value = construction.return_value;
    result.return_value = exit_registration_port.register_exit_cleanup(
        kLegacyBattleActorGroupAExitCleanupToken
    );
    result.exit_registration_calls = 1U;
    return result;
}

}  // namespace openswd3::battle
