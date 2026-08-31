#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleGroupBScriptSpecialActionItemParametersStatus : compat::
    u8 {
        completed,
        actor_state_typed_stop,
        resource_write_typed_stop,
    };

struct LegacyBattleGroupBScriptSpecialActionItemParametersRequest {
    std::array<compat::u16, 4> parameters{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBScriptSpecialActionItemParametersResult {
    LegacyBattleGroupBScriptSpecialActionItemParametersStatus status{
        LegacyBattleGroupBScriptSpecialActionItemParametersStatus::completed
    };
    compat::u32 stopped_offset{};
    compat::u32 stopped_parameter_index{};
    compat::u32 parameter_reads{};
    compat::u32 resource_pointer_loads{};
    compat::u32 resource_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_476A10.
[[nodiscard]] LegacyBattleGroupBScriptSpecialActionItemParametersResult
write_legacy_battle_group_b_script_special_action_item_parameters(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBScriptSpecialActionItemParametersRequest& request
);

}  // namespace openswd3::battle
