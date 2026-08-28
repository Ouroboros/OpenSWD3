#include "openswd3/battle/legacy_battle_growth_completion_caption.hpp"

namespace openswd3::battle {

LegacyBattleGrowthCaptionResult advance_legacy_battle_growth_completion_caption(
    LegacyBattleGrowthCaptionBindings bindings,
    LegacyBattleGrowthCaptionPort& port,
    LegacyBattleGrowthCaptionRequest request
) {
    request.variant = LegacyBattleGrowthCaptionVariant::completion;
    return advance_legacy_battle_growth_caption(bindings, port, request);
}

}  // namespace openswd3::battle
