#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_group_a_storage.hpp"
#include "openswd3/battle/legacy_battle_group_b_storage.hpp"

#include <algorithm>
#include <exception>

namespace openswd3::battle {

LegacyBattleActorGroupAElementConstructionResult
construct_legacy_battle_actor_group_a_element(
    const LegacyBattleActorGroupAElementConstructionView state,
    compat::u32& registered_resource
) {
    LegacyBattleActorGroupAElementConstructionResult result;
    result.base_initialization = initialize_legacy_battle_actor_base(
        state.base_initialization,
        state.action_execution,
        state.resource_definition,
        state.resource_definition_description,
        state.action_text,
        state.action_kind,
        state.object_token != 0U ? state.object_writable_bytes : 0U
    );
    if (result.base_initialization.status !=
        LegacyBattleActorBaseInitializationStatus::completed) {
        result.status = LegacyBattleActorGroupAElementConstructionStatus::
            base_construction_typed_stop;
        return result;
    }

    if (state.object_writable_bytes < 0x2F28U) {
        result.status = LegacyBattleActorGroupAElementConstructionStatus::
            object_write_typed_stop;
        result.stopped_object_offset = 0x2F26U;
        return result;
    }

    state.field_2f26 = 0U;
    state.field_2f18 = 0U;

    const auto allocation = asset_runtime::reserve_legacy_guest_bytes(0x38U);
    if (allocation) {
        registered_resource = *allocation;
    }

    state.primary_resource_token = allocation.value_or(0U);
    if (state.primary_resource_token == 0U) {
        result.status = LegacyBattleActorGroupAElementConstructionStatus::
            description_write_typed_stop;
        return result;
    }

    for (compat::u32 index = 0U; index < 14U; ++index) {
        const auto offset = index * 4U;
        if (state.description_bytes.size() < offset + 4U) {
            result.status = LegacyBattleActorGroupAElementConstructionStatus::
                description_write_typed_stop;
            return result;
        }

        for (compat::u32 byte = 0U; byte < 4U; ++byte) {
            state.description_bytes[offset + byte] = 0U;
        }

        result.description_bytes_written += 4U;
    }

    return result;
}

LegacyBattleActorGroupAElementConstructionResult
construct_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementState& state, compat::u32& registered_resource
) {
    auto& base = state.base_initialization;
    return construct_legacy_battle_actor_group_a_element(
        {
            .object_token = state.object_token,
            .object_writable_bytes = state.object_writable_bytes,
            .base_initialization = base.fields,
            .action_execution = base.action_execution,
            .resource_definition = base.resource_definition,
            .resource_definition_description =
                base.resource_definition_description,
            .action_text = base.action_text,
            .action_kind = base.action_execution.action_kind,
            .field_2f18 = state.field_2f18,
            .field_2f26 = state.field_2f26,
            .primary_resource_token =
                state.resource_cleanup.primary_resource_token,
            .description_bytes = state.description_bytes,
        },
        registered_resource
    );
}

LegacyBattleActorGroupBElementConstructionResult
construct_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state, compat::u32& registered_resource
) {
    LegacyBattleActorGroupBElementConstructionResult result;
    result.base_initialization = initialize_legacy_battle_actor_base(
        state.base_initialization,
        state.action_execution,
        state.action_composition.resource_definition,
        state.action_composition.resource_definition_description,
        state.action_composition.action_text,
        state.action_composition.action_kind,
        state.object_token != 0U ? state.object_writable_bytes : 0U
    );
    if (result.base_initialization.status !=
        LegacyBattleActorBaseInitializationStatus::completed) {
        result.status = LegacyBattleActorGroupBElementConstructionStatus::
            base_construction_typed_stop;
        return result;
    }

    const auto allocation = asset_runtime::reserve_legacy_guest_bytes(0xA4U);
    if (allocation) {
        registered_resource = *allocation;
    }

    state.resource_token = allocation.value_or(0U);
    if (state.resource_token == 0U) {
        result.status = LegacyBattleActorGroupBElementConstructionStatus::
            resource_write_typed_stop;
        return result;
    }

    state.resource_bytes.fill(0U);
    state.resource_description.clear();
    result.resource_bytes_written =
        static_cast<compat::u32>(state.resource_bytes.size());
    return result;
}

LegacyBattleActorGroupBElementDestructionResult
release_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state,
    LegacyBattleGroupBStorage& resources
) {
    LegacyBattleActorGroupBElementDestructionResult result;
    try {
        result.resource_cleanup = release_legacy_battle_group_b_resource(
            &state, &resources, state.object_token
        );
    } catch (...) {
        result.base_release = release_legacy_battle_actor_base(
            state.action_composition.resource_definition,
            state.action_composition.resource_definition_description,
            {
                .object_token = state.object_token,
                .readable_bytes = state.object_readable_bytes,
                .writable_bytes = state.object_writable_bytes,
            }
        );
        if (legacy_battle_actor_base_release_stopped(
                result.base_release.status
            )) {
            result.status = LegacyBattleActorGroupBElementDestructionStatus::
                base_release_typed_stop;
            return result;
        }

        throw;
    }

    if (result.resource_cleanup.status !=
        LegacyBattleGroupBResourceCleanupStatus::completed) {
        result.status = LegacyBattleActorGroupBElementDestructionStatus::
            resource_cleanup_typed_stop;
    }

    result.base_release = release_legacy_battle_actor_base(
        state.action_composition.resource_definition,
        state.action_composition.resource_definition_description,
        {
            .object_token = state.object_token,
            .readable_bytes = state.object_readable_bytes,
            .writable_bytes = state.object_writable_bytes,
        }
    );
    if (legacy_battle_actor_base_release_stopped(result.base_release.status)) {
        result.status = LegacyBattleActorGroupBElementDestructionStatus::
            base_release_typed_stop;
    }

    return result;
}

LegacyBattleActorGroupAElementDestructionResult
release_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementDestructionView state,
    LegacyBattleGroupAStorage& resources
) {
    LegacyBattleActorGroupAElementDestructionResult result;
    try {
        result.resource_cleanup = release_legacy_battle_group_a_resources(
            state.primary_resource_token,
            state.secondary_resource_token,
            &resources,
            state.object_token
        );
    } catch (...) {
        result.base_release = release_legacy_battle_actor_base(
            state.resource_definition,
            state.resource_definition_description,
            {
                .object_token = state.object_token,
                .readable_bytes = state.object_readable_bytes,
                .writable_bytes = state.object_writable_bytes,
            }
        );
        if (legacy_battle_actor_base_release_stopped(
                result.base_release.status
            )) {
            result.status = LegacyBattleActorGroupAElementDestructionStatus::
                base_release_typed_stop;
            return result;
        }

        throw;
    }

    if (result.resource_cleanup.status !=
        LegacyBattleGroupAResourceCleanupStatus::completed) {
        result.status = LegacyBattleActorGroupAElementDestructionStatus::
            resource_cleanup_typed_stop;
    }

    if (result.resource_cleanup.primary_resource_released) {
        std::ranges::fill(state.description_bytes, 0U);
    }

    result.base_release = release_legacy_battle_actor_base(
        state.resource_definition,
        state.resource_definition_description,
        {
            .object_token = state.object_token,
            .readable_bytes = state.object_readable_bytes,
            .writable_bytes = state.object_writable_bytes,
        }
    );
    if (legacy_battle_actor_base_release_stopped(result.base_release.status)) {
        result.status = LegacyBattleActorGroupAElementDestructionStatus::
            base_release_typed_stop;
    }

    return result;
}

LegacyBattleActorGroupBDestructionResult
release_legacy_battle_actor_group_b(LegacyBattleGroupBStorage& actors) {
    LegacyBattleActorGroupBDestructionResult result;
    auto remaining = kLegacyBattleActorGroupBElementCount;
    const auto release_remaining = [&] {
        while (remaining != 0U) {
            --remaining;
            result.element = release_legacy_battle_actor_group_b_element(
                (*actors.actors())[remaining], actors
            );
            if (result.element.status !=
                LegacyBattleActorGroupBElementDestructionStatus::completed) {
                result.stopped_actor_index = remaining;
                return;
            }
        }
    };

    try {
        release_remaining();
    } catch (...) {
        try {
            release_remaining();
        } catch (...) {
            std::terminate();
        }

        if (result.stopped_actor_index) {
            return result;
        }

        throw;
    }

    return result;
}

LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleGroupAStorage& actors, LegacyBattleExitCleanups& cleanups
) {
    LegacyBattleActorGroupAStaticInitializationResult result;
    result.constructed = actors.construct();
    if (!result.constructed) {
        return result;
    }

    result.cleanup_registered = cleanups.add([&actors] {
        return !actors.release().stopped_actor_index;
    });

    return result;
}

LegacyBattleActorGroupBStaticInitializationResult
initialize_legacy_battle_actor_group_b_static_lifecycle(
    LegacyBattleGroupBStorage& actors, LegacyBattleExitCleanups& cleanups
) {
    LegacyBattleActorGroupBStaticInitializationResult result;
    result.constructed = actors.construct();
    if (!result.constructed) {
        return result;
    }

    result.cleanup_registered = cleanups.add([&actors] {
        return !release_legacy_battle_actor_group_b(actors).stopped_actor_index;
    });

    return result;
}

LegacyBattleActorBaseInitializationResult
construct_legacy_battle_actor_singleton(
    LegacyBattleActorSingletonState& state
) {
    return initialize_legacy_battle_actor_base(
        state.base_initialization, state.object_writable_bytes
    );
}

LegacyBattleActorBaseReleaseResult
release_legacy_battle_actor_singleton(LegacyBattleActorSingletonState& state) {
    return release_legacy_battle_actor_base(
        state.base_initialization,
        {
            .object_token = kLegacyBattleActorSingletonToken,
            .readable_bytes = state.object_readable_bytes,
            .writable_bytes = state.object_writable_bytes,
        }
    );
}

LegacyBattleActorSingletonStaticInitializationResult
initialize_legacy_battle_actor_singleton_static_lifecycle(
    LegacyBattleActorSingletonState& state, LegacyBattleExitCleanups& cleanups
) {
    LegacyBattleActorSingletonStaticInitializationResult result;
    result.construction = construct_legacy_battle_actor_singleton(state);
    if (result.construction.status !=
        LegacyBattleActorBaseInitializationStatus::completed) {
        result.status = LegacyBattleActorSingletonStaticInitializationStatus::
            construction_typed_stop;
        return result;
    }

    result.cleanup_registered = cleanups.add([&state] {
        return !legacy_battle_actor_base_release_stopped(
            release_legacy_battle_actor_singleton(state).status
        );
    });

    return result;
}

}  // namespace openswd3::battle
