#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

namespace openswd3::battle {

class LegacyBattleGroupAFinalProcessingPort
    : public LegacyBattleGroupAItemEffectApplicationPort,
      public LegacyBattleGroupAProfileModeSelectionPort {
public:
    ~LegacyBattleGroupAFinalProcessingPort() override = default;
};

struct LegacyBattleGroupAFinalProcessingRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAFinalProcessingStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    actor_record_typed_stop,
    item_effect_typed_stop,
    profile_mode_typed_stop,
    profile_load_typed_stop,
};

struct LegacyBattleGroupAFinalProcessingResult {
    LegacyBattleGroupAFinalProcessingStatus status{
        LegacyBattleGroupAFinalProcessingStatus::completed
    };
    LegacyBattleGroupAItemEffectApplicationResult item_effect{};
    LegacyBattleGroupAProfileModeSelectionResult profile_mode{};
    compat::u32 item_effect_calls{};
    compat::u32 profile_mode_calls{};
    compat::u32 profile_load_calls{};
    compat::u32 pre_effect_dwords_zeroed{};
    compat::u32 completion_latch_writes{};
    compat::u32 action_kind_writes{};
    compat::u32 derived_word_writes{};
    compat::u32 profile_buffer_dwords_zeroed{};
    compat::u32 profile_buffer_flag_writes{};
    compat::u32 display_kind_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

enum class LegacyBattleActorModeFourFinalizationStatus : compat::u8 {
    completed,
    final_state_typed_stop,
    item_state_typed_stop,
    workspace_typed_stop,
};

struct LegacyBattleActorModeFourFinalizationResult {
    LegacyBattleActorModeFourFinalizationStatus status{
        LegacyBattleActorModeFourFinalizationStatus::completed
    };
    compat::u32 mode_flag_writes{};
    compat::u32 completion_latch_writes{};
    compat::u32 action_kind_writes{};
    compat::u32 workspace_dwords_zeroed{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4708C0.
[[nodiscard]] LegacyBattleActorModeFourFinalizationResult
finalize_legacy_battle_actor_mode_four(
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    compat::u32 actor_token,
    compat::u32 entry_eax
) noexcept;

// sub_46FFF0.
[[nodiscard]] LegacyBattleGroupAFinalProcessingResult
process_legacy_battle_group_a_final(
    LegacyBattleGroupAFinalProcessingState* state,
    LegacyBattleGroupAActionExecutionState* action,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAAttributeAggregationState& aggregation,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    compat::u32 actor_token,
    compat::u32 skip_primary,
    compat::u32 skip_secondary,
    LegacyBattleGroupAFinalProcessingPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupAFinalProcessingRequest& request = {}
);

}  // namespace openswd3::battle
