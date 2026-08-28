#pragma once

#include "openswd3/battle/legacy_battle_growth_caption.hpp"

namespace openswd3::battle {

// Typed closure of legacy 0x00468AD0. The rendering body is shared with
// 0x00468930, while the sample gate and call-site register profile remain
// distinct.
[[nodiscard]] LegacyBattleGrowthCaptionResult
advance_legacy_battle_growth_completion_caption(
    LegacyBattleGrowthCaptionBindings bindings,
    LegacyBattleGrowthCaptionPort& port,
    LegacyBattleGrowthCaptionRequest request = {}
);

}  // namespace openswd3::battle
