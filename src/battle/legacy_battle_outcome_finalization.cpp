#include "openswd3/battle/legacy_battle_outcome_finalization.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] bool advance_item(
    LegacyBattleOutcomeFinalizationResult& result,
    LegacyBattleOutcomeFinalizationPort& port,
    const u32 item_id,
    const LegacyBattleOutcomeFinalizationStage stage,
    const u32 index,
    u32& eax
) {
    result.item_quantity =
        advance_legacy_battle_player_item_quantity(port, item_id, 0U);
    ++result.item_quantity_calls;
    if (stage == LegacyBattleOutcomeFinalizationStage::player_reward) {
        ++result.player_reward_calls;
    } else {
        ++result.group_reward_calls;
    }
    eax = result.item_quantity.return_token;
    if (result.item_quantity.status ==
        LegacyBattlePlayerItemQuantityStatus::completed) {
        return true;
    }

    result.status =
        LegacyBattleOutcomeFinalizationStatus::player_item_quantity_typed_stop;
    result.stopped_stage = stage;
    result.stopped_index = index;
    result.return_value = eax;
    return false;
}

}  // namespace

LegacyBattleOutcomeFinalizationResult finalize_legacy_battle_outcome(
    LegacyBattleOutcomeFinalizationPort& port,
    u32& group_b_count,
    const u32 entry_eax_snapshot
) {
    LegacyBattleOutcomeFinalizationResult result;
    auto& state = port.outcome_finalization_state();
    u32 eax = entry_eax_snapshot;

    for (u32 index = 0U; index < state.player_reward_item_ids.size(); ++index) {
        eax = (eax & 0xFFFF0000U) |
            static_cast<u32>(state.player_reward_item_ids[index]);
        if (static_cast<u16>(eax) != 0U &&
            !advance_item(
                result,
                port,
                eax,
                LegacyBattleOutcomeFinalizationStage::player_reward,
                index,
                eax
            )) {
            return result;
        }
    }

    eax = group_b_count;
    u32 index = 0U;
    if (signed_bits(eax) > 0) {
        do {
            if (!advance_item(
                    result,
                    port,
                    kLegacyBattleOutcomeGroupRewardItem,
                    LegacyBattleOutcomeFinalizationStage::group_reward,
                    index,
                    eax
                )) {
                return result;
            }
            eax = group_b_count;
            ++index;
            ++result.group_iterations;
        } while (signed_bits(index) < signed_bits(eax));
    }

    state.completion_words[1] = 0U;
    group_b_count = 0U;
    state.player_reward_item_ids.fill(0U);
    result.return_value = eax;
    result.cleanup_applied = true;
    return result;
}

}  // namespace openswd3::battle
