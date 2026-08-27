#include "openswd3/battle/legacy_battle_reward_item_slot.hpp"

namespace openswd3::battle {

LegacyBattleRewardItemSlotResult write_legacy_battle_reward_item_slot(
    LegacyBattleOutcomeFinalizationState& state,
    const compat::u32 slot_index,
    const compat::u32 value_stack_slot,
    const compat::u32 entry_eax,
    const compat::u32 entry_edx
) noexcept {
    LegacyBattleRewardItemSlotResult result;
    const auto item_id = static_cast<compat::u16>(value_stack_slot);
    result.store_address = kLegacyBattleRewardItemSlotBase + slot_index * 2U;
    result.return_eax =
        (entry_eax & 0xFFFF0000U) | static_cast<compat::u32>(item_id);
    result.return_ecx = slot_index;
    result.return_edx = entry_edx;

    switch (slot_index) {
    case 0U:
        state.reward_item_slot_prefix = item_id;
        break;
    case 1U:
        state.player_reward_item_ids[0] = item_id;
        break;
    case 2U:
        state.player_reward_item_ids[1] = item_id;
        break;
    default:
        result.status = LegacyBattleRewardItemSlotStatus::slot_typed_stop;
        return result;
    }

    result.stored = true;
    return result;
}

}  // namespace openswd3::battle
