#include "openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 read_word(
    const std::array<compat::u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

}  // namespace

LegacyBattleGroupBRewardItemSelectionResult
select_legacy_battle_group_b_reward_item(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleGroupBRewardItemSelectionRequest& request
) {
    LegacyBattleGroupBRewardItemSelectionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBRewardItemSelectionStatus::actor_state_typed_stop;
        return result;
    }

    result.return_eax = actor->resource_token;
    if (actor->resource_token == 0U) {
        result.status = LegacyBattleGroupBRewardItemSelectionStatus::
            resource_read_typed_stop;
        return result;
    }

    result.item_id = read_word(actor->resource_bytes, 0x82U);
    if (result.item_id == 0U) {
        result.return_eax &= 0xFFFF0000U;
        return result;
    }

    result.random_value = random.random_bounded(20U);
    result.random_calls = 1U;
    result.return_eax = result.random_value;
    result.return_ecx = actor->resource_token;
    result.return_edx = result.random_value;
    if (result.return_ecx == 0U) {
        result.status = LegacyBattleGroupBRewardItemSelectionStatus::
            resource_read_typed_stop;
        return result;
    }

    result.threshold = read_word(actor->resource_bytes, 0x84U);
    if (static_cast<u16>(result.random_value) >= result.threshold) {
        result.return_eax &= 0xFFFF0000U;
        return result;
    }

    result.item_id = read_word(actor->resource_bytes, 0x82U);
    result.return_eax = (result.return_eax & 0xFFFF0000U) | result.item_id;
    return result;
}

}  // namespace openswd3::battle
