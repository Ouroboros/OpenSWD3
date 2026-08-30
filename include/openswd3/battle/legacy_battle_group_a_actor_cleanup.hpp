#pragma once

#include "openswd3/battle/legacy_battle_actor_list_query.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_effect.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupAActorCleanupBindings {
    LegacyBattleGroupAActionExecutionState* actor{};
    LegacyBattleGroupAWorkspaceState* workspace{};
    LegacyBattleGroupAFinalProcessingState* final_processing{};
    LegacyBattleGroupAItemEffectApplicationState* item_effect{};
    LegacyBattleGroupAAttributeEffectState* attribute_effect{};
    LegacyBattleActorListQueryState* actor_list{};
};

enum class LegacyBattleGroupAActorCleanupStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    profile_state_typed_stop,
    actor_list_state_typed_stop,
    workspace_state_typed_stop,
    item_effect_state_typed_stop,
    attribute_effect_state_typed_stop,
    resource_node_typed_stop,
};

struct LegacyBattleGroupAActorCleanupResult {
    LegacyBattleGroupAActorCleanupStatus status{
        LegacyBattleGroupAActorCleanupStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 profile_dwords_zeroed{};
    compat::u32 pre_effect_dwords_zeroed{};
    compat::u32 explicit_words_zeroed{};
    compat::u32 resource_quantity_decrements{};
    compat::u32 resource_gate_clears{};
};

// Typed closure of legacy 0x004750C0.
[[nodiscard]] LegacyBattleGroupAActorCleanupResult
cleanup_legacy_battle_group_a_actor(
    LegacyBattleGroupAActorCleanupBindings bindings,
    compat::u32 actor_token
) noexcept;

}  // namespace openswd3::battle
