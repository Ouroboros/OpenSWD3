#pragma once

#include "openswd3/battle/legacy_battle_outcome_finalization.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleRewardItemSlotBase = 0x004FF2EAU;

enum class LegacyBattleRewardItemSlotStatus : compat::u8 {
    completed,
    slot_typed_stop,
};

struct LegacyBattleRewardItemSlotResult {
    LegacyBattleRewardItemSlotStatus status{
        LegacyBattleRewardItemSlotStatus::completed
    };
    compat::u32 store_address{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool stored{};
};

[[nodiscard]] LegacyBattleRewardItemSlotResult
write_legacy_battle_reward_item_slot(
    LegacyBattleOutcomeFinalizationState& state,
    compat::u32 slot_index,
    compat::u32 value_stack_slot,
    compat::u32 entry_eax,
    compat::u32 entry_edx
) noexcept;

}  // namespace openswd3::battle
