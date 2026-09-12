#pragma once

#include "openswd3/battle/legacy_battle_actor_action_target.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"

#include <array>

namespace openswd3::battle {

class LegacyBattleFrameCoordinatorPort;

enum class LegacyBattleActorPriorityStatus : compat::u8 {
    completed,
    metric_typed_stop,
    mask_typed_stop,
    order_typed_stop,
    nested_order_typed_stop,
    actor_action_target_typed_stop,
};

struct LegacyBattleActorPriorityResult {
    LegacyBattleActorPriorityStatus status{
        LegacyBattleActorPriorityStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 pair_query_calls{};
    compat::u32 order_writes{};
    compat::u32 selections{};
    compat::u32 priority_prefix_selections{};
    compat::u32 paired_selections{};
    compat::u32 nested_order_calls{};
    LegacyBattleActorActionTargetResult actor_action_target{};
    compat::u32 actor_action_target_calls{};
    bool order_ready_published{};
};

struct LegacyBattleActorPriorityRequest {
    compat::u32 caller_eax{};
    compat::u32 caller_ecx{};
    compat::u32 caller_edx{};
    std::array<LegacyBattleActorActionTargetRequest, 2>
        action_target_requests{};
};

[[nodiscard]] LegacyBattleActorPriorityResult
update_legacy_battle_actor_priority(
    LegacyBattleFrameCoordinatorPort& port,
    LegacyBattleActorActionTargetOwners action_target_owners,
    const LegacyBattleActorPriorityRequest& request = {}
);

}  // namespace openswd3::battle
