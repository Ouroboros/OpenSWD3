#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

LegacyBattleActorGroupAElementConstructionResult
construct_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementState& state,
    LegacyBattleActorGroupAElementConstructionPort& port
) {
    LegacyBattleActorGroupAElementConstructionResult result;
    static_cast<void>(port.construct_base(state.object_token));
    ++result.base_constructor_calls;
    state.field_2f26 = 0U;
    state.field_2f18 = 0U;

    const auto allocation = port.allocate(0x38U);
    ++result.allocation_calls;
    state.resource_cleanup.primary_resource_token = allocation.eax;
    result.return_ecx = allocation.ecx;
    result.return_edx = allocation.edx;
    if (state.resource_cleanup.primary_resource_token == 0U) {
        result.status = LegacyBattleActorGroupAElementConstructionStatus::
            description_write_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 0x0EU;
        return result;
    }

    state.description_bytes.fill(0U);
    result.description_bytes_written =
        static_cast<compat::u32>(state.description_bytes.size());
    result.return_eax = state.object_token;
    result.return_ecx = 0U;
    return result;
}

LegacyBattleActorGroupBElementConstructionResult
construct_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state,
    LegacyBattleActorGroupBElementConstructionPort& port
) {
    LegacyBattleActorGroupBElementConstructionResult result;
    static_cast<void>(port.construct_base(state.object_token));
    ++result.base_constructor_calls;

    const auto allocation = port.allocate(0xA4U);
    ++result.allocation_calls;
    state.resource_token = allocation.eax;
    result.return_ecx = allocation.ecx;
    result.return_edx = allocation.edx;
    if (state.resource_token == 0U) {
        result.status = LegacyBattleActorGroupBElementConstructionStatus::
            resource_write_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 0x29U;
        return result;
    }

    state.resource_bytes.fill(0U);
    state.resource_description.clear();
    result.resource_bytes_written =
        static_cast<compat::u32>(state.resource_bytes.size());
    result.return_eax = state.object_token;
    result.return_ecx = 0U;
    return result;
}

LegacyBattleActorGroupBElementDestructionResult
release_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state,
    LegacyBattleActorGroupBElementDestructionPort& port,
    const LegacyBattleActorElementDestructionRequest request
) {
    LegacyBattleActorGroupBElementDestructionResult result;
    try {
        result.resource_cleanup = release_legacy_battle_group_b_resource(
            &state,
            port,
            {
                .actor_token = state.object_token,
                .actor_index =
                    (state.object_token - kLegacyBattleActorGroupBBaseToken) /
                    kLegacyBattleActorGroupBElementSize,
                .entry_eax = request.seh_chain_token,
                .entry_ecx = state.object_token,
                .entry_edx = request.entry_edx,
            }
        );
        ++result.extension_destructor_calls;
    } catch (...) {
        static_cast<void>(port.destroy_base(state));
        throw;
    }
    if (result.resource_cleanup.status !=
        LegacyBattleGroupBResourceCleanupStatus::completed) {
        result.status = LegacyBattleActorGroupBElementDestructionStatus::
            resource_cleanup_typed_stop;
    }

    const auto base = port.destroy_base(state);
    ++result.base_destructor_calls;
    result.return_eax = base.eax;
    result.return_ecx = request.seh_chain_token;
    result.return_edx = base.edx;
    return result;
}

LegacyBattleActorGroupAElementDestructionResult
release_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementState& state,
    LegacyBattleActorGroupAElementDestructionPort& port,
    const LegacyBattleActorElementDestructionRequest request
) {
    LegacyBattleActorGroupAElementDestructionResult result;
    try {
        result.resource_cleanup = release_legacy_battle_group_a_resources(
            &state.resource_cleanup,
            port,
            {
                .actor_token = state.object_token,
                .entry_ecx = state.object_token,
            }
        );
        ++result.resource_cleanup_calls;
    } catch (...) {
        static_cast<void>(port.destroy_base(state));
        throw;
    }
    if (result.resource_cleanup.status !=
        LegacyBattleGroupAResourceCleanupStatus::completed) {
        result.status = LegacyBattleActorGroupAElementDestructionStatus::
            resource_cleanup_typed_stop;
        const auto base = port.destroy_base(state);
        ++result.base_destructor_calls;
        result.return_eax = base.eax;
        result.return_ecx = request.seh_chain_token;
        result.return_edx = base.edx;
        return result;
    }
    if (result.resource_cleanup.primary_resource_released) {
        state.description_bytes.fill(0U);
    }

    const auto base = port.destroy_base(state);
    ++result.base_destructor_calls;
    result.return_eax = base.eax;
    result.return_ecx = request.seh_chain_token;
    result.return_edx = base.edx;
    return result;
}

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
