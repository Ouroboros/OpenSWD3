#include "openswd3/battle/legacy_battle_timing.hpp"

#include <bit>

namespace openswd3::battle {

compat::i32 publish_legacy_battle_action_threshold(
    LegacyBattleTimingState& state, const compat::i32 speed_setting
) noexcept {
    compat::u32 value = 20U - std::bit_cast<compat::u32>(speed_setting);
    value = value + value * 4U;
    value = value + value * 4U;
    value <<= 2U;
    state.action_threshold = std::bit_cast<compat::i32>(value);
    return state.action_threshold;
}

}  // namespace openswd3::battle
