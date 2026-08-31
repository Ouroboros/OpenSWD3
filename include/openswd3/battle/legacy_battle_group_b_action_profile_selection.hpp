#pragma once

#include "openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBActionProfileSelectionOutput {
    compat::u32* dword{};
    compat::u16* low_word{};
    compat::u16* high_word{};
};

struct LegacyBattleGroupBActionProfileSelectionRequest {
    compat::u32 selector_argument{};
    compat::u32 output_token{};
    compat::u32 actor_token{};
};

enum class LegacyBattleGroupBActionProfileSelectionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_state_typed_stop,
    profile_load_typed_stop,
    output_state_typed_stop,
};

struct LegacyBattleGroupBActionProfileSelectionResult {
    LegacyBattleGroupBActionProfileSelectionStatus status{
        LegacyBattleGroupBActionProfileSelectionStatus::completed
    };
    compat::u32 profile_load_calls{};
    compat::u32 profile_dwords_cleared{};
    compat::u32 mode_update_calls{};
    compat::u32 output_write_calls{};
    compat::u16 profile_id{};
    compat::u16 derived_word{};
    compat::u32 output_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_476250. The fixed mode-one and mode-two paths are expanded directly;
// only the still-pending action-profile loader remains behind a narrow port.
[[nodiscard]] LegacyBattleGroupBActionProfileSelectionResult
select_legacy_battle_group_b_action_profile(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBActionProfileSelectionOutput& output,
    LegacyBattleGroupBActionProfileModePort& port,
    const LegacyBattleGroupBActionProfileSelectionRequest& request
);

[[nodiscard]] inline LegacyBattleGroupBActionProfileSelectionResult
select_legacy_battle_group_b_action_profile(
    LegacyBattleActorGroupBElementState* const actor,
    compat::u32* const output,
    LegacyBattleGroupBActionProfileModePort& port,
    const LegacyBattleGroupBActionProfileSelectionRequest& request
) {
    return select_legacy_battle_group_b_action_profile(
        actor, {.dword = output}, port, request
    );
}

}  // namespace openswd3::battle
