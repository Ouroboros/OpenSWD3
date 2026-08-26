#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"

namespace openswd3::battle {

class LegacyBattleFrameCoordinatorPort;

enum class LegacyBattleActorPriorityStatus : compat::u8 {
    completed,
    metric_typed_stop,
    mask_typed_stop,
    order_typed_stop,
    nested_order_typed_stop,
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
    bool order_ready_published{};
};

[[nodiscard]] LegacyBattleActorPriorityResult
update_legacy_battle_actor_priority(
    LegacyBattleFrameCoordinatorPort& port,
    compat::u32 caller_eax = 0U,
    compat::u32 caller_ecx = 0U,
    compat::u32 caller_edx = 0U
);

}  // namespace openswd3::battle
