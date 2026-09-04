#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleTimingState {
    compat::i32 action_threshold{900};
    bool action_threshold_read_accessible{true};
};

// sub_44FFC0. Returns the same value published to action_threshold.
[[nodiscard]] compat::i32 publish_legacy_battle_action_threshold(
    LegacyBattleTimingState& state, compat::i32 speed_setting
) noexcept;

}  // namespace openswd3::battle
