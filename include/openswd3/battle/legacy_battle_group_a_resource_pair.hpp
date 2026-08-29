#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupAResourcePairState {
    compat::u32 primary_token{};    // actor + 0x2EC8
    compat::u32 secondary_token{};  // actor + 0x2ECC
};

enum class LegacyBattleGroupAResourcePairStatus : compat::u8 {
    completed,
    actor_typed_stop,
};

struct LegacyBattleGroupAResourcePairResult {
    LegacyBattleGroupAResourcePairStatus status{
        LegacyBattleGroupAResourcePairStatus::completed
    };
    compat::u32 writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E850.
[[nodiscard]] LegacyBattleGroupAResourcePairResult
publish_legacy_battle_group_a_resource_pair(
    LegacyBattleGroupAResourcePairState& state,
    compat::u32 object_token,
    compat::u32 resource_token,
    compat::u32 entry_edx = 0U
) noexcept;

}  // namespace openswd3::battle
