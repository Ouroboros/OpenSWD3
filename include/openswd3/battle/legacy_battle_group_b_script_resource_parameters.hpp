#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/compat/types.hpp"

#include <span>

namespace openswd3::battle {

enum class LegacyBattleGroupBScriptResourceParametersStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    script_read_typed_stop,
    resource_write_typed_stop,
};

struct LegacyBattleGroupBScriptResourceParametersRequest {
    std::span<const compat::u8> script_bytes{};
    compat::u32 script_capacity{};
    compat::u32 source_offset{};
    compat::u32 source_token{};
    compat::u32 actor_token{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBScriptResourceParametersResult {
    LegacyBattleGroupBScriptResourceParametersStatus status{
        LegacyBattleGroupBScriptResourceParametersStatus::completed
    };
    compat::u32 stopped_offset{};
    compat::u32 resource_pointer_loads{};
    compat::u32 source_reads{};
    compat::u32 resource_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_476920.
[[nodiscard]] LegacyBattleGroupBScriptResourceParametersResult
write_legacy_battle_group_b_script_resource_parameters(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBScriptResourceParametersRequest& request
);

}  // namespace openswd3::battle
