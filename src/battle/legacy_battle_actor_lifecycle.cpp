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

LegacyBattleActorGroupBConstructionResult construct_legacy_battle_actor_group_b(
    LegacyBattleActorVectorConstructionPort& construction_port
) {
    LegacyBattleActorGroupBConstructionResult result{
        .request = {
            .base_token = kLegacyBattleActorGroupBBaseToken,
            .element_size = kLegacyBattleActorGroupBElementSize,
            .element_count = kLegacyBattleActorGroupBElementCount,
            .constructor_token = kLegacyBattleActorGroupBConstructorToken,
            .destructor_token = kLegacyBattleActorGroupBDestructorToken,
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

LegacyBattleActorGroupBDestructionResult release_legacy_battle_actor_group_b(
    LegacyBattleActorVectorDestructionPort& destruction_port
) {
    LegacyBattleActorGroupBDestructionResult result{
        .request = {
            .base_token = kLegacyBattleActorGroupBBaseToken,
            .element_size = kLegacyBattleActorGroupBElementSize,
            .element_count = kLegacyBattleActorGroupBElementCount,
            .destructor_token = kLegacyBattleActorGroupBDestructorToken,
        },
    };
    result.return_value = destruction_port.destroy_vector(result.request);
    result.vector_destructor_calls = 1U;
    return result;
}

LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleActorVectorConstructionPort& construction_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
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

LegacyBattleActorGroupBStaticInitializationResult
initialize_legacy_battle_actor_group_b_static_lifecycle(
    LegacyBattleActorVectorConstructionPort& construction_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
) {
    LegacyBattleActorGroupBStaticInitializationResult result;
    const auto construction =
        construct_legacy_battle_actor_group_b(construction_port);
    result.construction_return_value = construction.return_value;
    result.construct_calls = 1U;
    result.return_value = exit_registration_port.register_exit_cleanup(
        kLegacyBattleActorGroupBExitCleanupToken
    );
    result.exit_registration_calls = 1U;
    return result;
}

LegacyBattleActorSingletonOperationResult
construct_legacy_battle_actor_singleton(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port
) {
    LegacyBattleActorSingletonOperationResult result{
        .object_token = kLegacyBattleActorSingletonToken,
    };
    result.return_value =
        object_lifecycle_port.construct_object(result.object_token);
    result.object_operation_calls = 1U;
    return result;
}

LegacyBattleActorSingletonOperationResult release_legacy_battle_actor_singleton(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port
) {
    LegacyBattleActorSingletonOperationResult result{
        .object_token = kLegacyBattleActorSingletonToken,
    };
    result.return_value =
        object_lifecycle_port.destroy_object(result.object_token);
    result.object_operation_calls = 1U;
    return result;
}

LegacyBattleActorSingletonStaticInitializationResult
initialize_legacy_battle_actor_singleton_static_lifecycle(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
) {
    LegacyBattleActorSingletonStaticInitializationResult result;
    const auto construction =
        construct_legacy_battle_actor_singleton(object_lifecycle_port);
    result.construction_return_value = construction.return_value;
    result.construct_calls = 1U;
    result.return_value = exit_registration_port.register_exit_cleanup(
        kLegacyBattleActorSingletonExitCleanupToken
    );
    result.exit_registration_calls = 1U;
    return result;
}

}  // namespace openswd3::battle
