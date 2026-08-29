#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupAValuePairState {
    compat::u32 primary_value{};    // actor + 0x2EC0
    compat::u32 secondary_value{};  // actor + 0x2EC4
};

enum class LegacyBattleGroupAValuePairStatus : compat::u8 {
    completed,
    actor_typed_stop,
};

struct LegacyBattleGroupAValuePairResult {
    LegacyBattleGroupAValuePairStatus status{
        LegacyBattleGroupAValuePairStatus::completed
    };
    compat::u32 writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E870.
[[nodiscard]] LegacyBattleGroupAValuePairResult
publish_legacy_battle_group_a_value_pair(
    LegacyBattleGroupAValuePairState& state,
    compat::u32 object_token,
    compat::u32 value,
    compat::u32 entry_edx = 0U
) noexcept;

}  // namespace openswd3::battle
