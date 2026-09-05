#pragma once

#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

namespace openswd3::battle {

struct LegacyBattleActorProfilePreparationRequest {
    compat::u32 source_value{};
    compat::u32 entry_edx{};
    // Original local/output addresses. Zero denotes an uncaptured address.
    compat::u32 definition_output_token{};
    compat::u32 output_token{};
    // Original local bytes: 0x00476DBA consumes +0xA0 before rep stosd clears
    // the record. Default zero is not an original-runtime capture.
    LegacyBattleMonDefinitionBytes initial_definition_bytes{};
};

enum class LegacyBattleActorProfilePreparationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    output_access_typed_stop,
    profile_load_typed_stop,
    definition_load_typed_stop,
    definition_release_typed_stop,
};

struct LegacyBattleActorProfilePreparationResult {
    LegacyBattleActorProfilePreparationStatus status{
        LegacyBattleActorProfilePreparationStatus::completed
    };
    compat::u32 build_calls{};
    compat::u32 release_calls{};
    compat::u32 profile_load_calls{};
    compat::u32 output_writes{};
    compat::u32 fallback_writes{};
    compat::u32 mode_flag_writes{};
    compat::u32 output_value{};
    compat::u32 profile_argument{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4707B0. The two audited callers borrow aligned startup DWORD slots.
// A null output means that this original write destination is unavailable;
// stop at 0x004707E5, after definition loading and actor-text release.
[[nodiscard]] LegacyBattleActorProfilePreparationResult
prepare_legacy_battle_actor_profile(
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    compat::u32* output,
    compat::u32 actor_token,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleActorProfilePreparationRequest& request
);

}  // namespace openswd3::battle
