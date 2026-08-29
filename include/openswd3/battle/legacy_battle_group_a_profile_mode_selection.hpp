#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupAProfileModeRandomReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAProfileModeSelectionPort {
public:
    virtual ~LegacyBattleGroupAProfileModeSelectionPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAProfileModeRandomReply random_below(
        compat::u32 bound, compat::u32 eax, compat::u32 ecx, compat::u32 edx
    ) = 0;
};

struct LegacyBattleGroupAProfileModeSelectionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAProfileModeSelectionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupAProfileModeSelectionResult {
    LegacyBattleGroupAProfileModeSelectionStatus status{
        LegacyBattleGroupAProfileModeSelectionStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 profile_mode_writes{};
    compat::u32 counter_clears{};
    compat::u32 identity_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46FF00.
[[nodiscard]] LegacyBattleGroupAProfileModeSelectionResult
select_legacy_battle_group_a_profile_mode(
    LegacyBattleGroupAActionExecutionState* state,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleGroupAEmbeddedProfileApplicationState& embedded_profile,
    LegacyBattleGroupAItemEffectApplicationState& item_effect,
    compat::u32 actor_token,
    compat::u32 skip_primary,
    compat::u32 skip_secondary,
    LegacyBattleGroupAProfileModeSelectionPort& port,
    const LegacyBattleGroupAProfileModeSelectionRequest& request = {}
);

}  // namespace openswd3::battle
