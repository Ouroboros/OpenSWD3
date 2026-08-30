#include "openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp"

#include <algorithm>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 actor_pre_effect_token(
    const u32 actor_token
) noexcept {
    return actor_token + 0x2630U;
}

void clear_profile_overlap(
    LegacyBattleGroupAActionExecutionState& actor,
    LegacyBattleGroupAFinalProcessingState& final_processing
) noexcept {
    final_processing.profile_buffer.fill(0U);
    actor.special_particle_coordinate_suppression = 0U;
    actor.special_effect_direct_mode = 0U;
    actor.copied_runtime_word = 0U;
    actor.source_y = 0U;
    final_processing.actor_flags = 0U;
}

}  // namespace

LegacyBattleGroupAActorCleanupResult cleanup_legacy_battle_group_a_actor(
    const LegacyBattleGroupAActorCleanupBindings bindings,
    const u32 actor_token
) noexcept {
    LegacyBattleGroupAActorCleanupResult result{
        .return_ecx = 10U,
        .return_edx = actor_token,
    };
    if (bindings.actor == nullptr || actor_token == 0U) {
        result.status =
            LegacyBattleGroupAActorCleanupStatus::actor_state_typed_stop;
        return result;
    }

    auto& actor = *bindings.actor;
    actor.action_override_flags = 0U;
    result.explicit_words_zeroed = 1U;

    if (bindings.final_processing == nullptr) {
        result.status =
            LegacyBattleGroupAActorCleanupStatus::profile_state_typed_stop;
        return result;
    }
    auto& final_processing = *bindings.final_processing;
    clear_profile_overlap(actor, final_processing);
    result.profile_dwords_zeroed = 10U;

    result.return_ecx = actor_pre_effect_token(actor_token);
    final_processing.pre_effect_words.fill(0U);
    result.pre_effect_dwords_zeroed = 4U;

    if (bindings.actor_list == nullptr) {
        result.status =
            LegacyBattleGroupAActorCleanupStatus::actor_list_state_typed_stop;
        return result;
    }
    auto& actor_list = *bindings.actor_list;
    result.return_eax = actor_list.selected_resource_token;

    if (bindings.workspace == nullptr) {
        result.status =
            LegacyBattleGroupAActorCleanupStatus::workspace_state_typed_stop;
        return result;
    }
    auto& workspace = *bindings.workspace;
    workspace.tail_words[0U] = 0U;
    ++result.explicit_words_zeroed;

    if (bindings.item_effect == nullptr) {
        result.status = LegacyBattleGroupAActorCleanupStatus::
            item_effect_state_typed_stop;
        return result;
    }
    auto& item_effect = *bindings.item_effect;
    workspace.tail_words[1U] = 0U;
    item_effect.cached_profile_item_id = 0U;
    ++result.explicit_words_zeroed;

    actor.special_profile_variant = 0U;
    final_processing.applied_mode_value = 0U;
    ++result.explicit_words_zeroed;

    workspace.tail_words[3U] = 0U;
    final_processing.applied_output_value = 0U;
    ++result.explicit_words_zeroed;

    workspace.tail_words[4U] = 0U;
    final_processing.replacement_action_kind = 0U;
    ++result.explicit_words_zeroed;

    if (bindings.attribute_effect == nullptr) {
        result.status = LegacyBattleGroupAActorCleanupStatus::
            attribute_effect_state_typed_stop;
        return result;
    }
    auto& attribute_effect = *bindings.attribute_effect;
    item_effect.derived_words[1U] = 0U;
    attribute_effect.temporary_values[0U] = 0U;
    ++result.explicit_words_zeroed;
    item_effect.derived_words[2U] = 0U;
    attribute_effect.temporary_values[1U] = 0U;
    ++result.explicit_words_zeroed;
    item_effect.derived_words[3U] = 0U;
    attribute_effect.temporary_values[2U] = 0U;
    ++result.explicit_words_zeroed;
    item_effect.derived_words[0U] = 0U;
    ++result.explicit_words_zeroed;

    workspace.tail_words[2U] = 0U;
    actor.summon_action_id = 0U;
    ++result.explicit_words_zeroed;

    if (result.return_eax == 0U) {
        return result;
    }

    result.return_eax = actor_list.resource_head_token;
    const auto resource = std::ranges::find(
        actor_list.resources,
        actor_list.resource_head_token,
        &LegacyBattleActorListResourceNode::token
    );
    if (actor_list.resource_head_token == 0U ||
        resource == actor_list.resources.end()) {
        result.status =
            LegacyBattleGroupAActorCleanupStatus::resource_node_typed_stop;
        return result;
    }

    u16 quantity = resource->primary_quantity;
    if (quantity > 0U) {
        quantity = static_cast<u16>(quantity - 1U);
        resource->primary_quantity = quantity;
        ++result.resource_quantity_decrements;
    }
    result.return_ecx =
        (result.return_ecx & 0xFFFF0000U) | static_cast<u32>(quantity);
    actor_list.selected_resource_token = 0U;
    result.resource_gate_clears = 1U;
    return result;
}

}  // namespace openswd3::battle
