#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"

namespace openswd3::battle {

enum class LegacyBattleGroupBOrderStatus : compat::u8 {
    completed,
    output_store_typed_stop,
};

struct LegacyBattleGroupBOrderResult {
    LegacyBattleGroupBOrderStatus status{
        LegacyBattleGroupBOrderStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 scanned_slots{};
    compat::u32 copied_slots{};
    bool reached_requested_count{};
};

[[nodiscard]] LegacyBattleGroupBOrderResult
rebuild_legacy_battle_group_b_order(LegacyBattleActorMetricState& state);

}  // namespace openswd3::battle
