#include "openswd3/battle/legacy_battle_group_b_order.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

constexpr u32 kGroupBOrderBaseToken = 0x00520DF8U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

}  // namespace

LegacyBattleGroupBOrderResult
rebuild_legacy_battle_group_b_order(LegacyBattleActorMetricState& state) {
    LegacyBattleGroupBOrderResult result;
    const u32 requested_count = state.group_b_count;
    u32 copied = 0U;
    u32 eax = 0U;

    for (const u32 actor_index : state.actor_order) {
        eax = actor_index;
        ++result.scanned_slots;
        if (signed_dword(actor_index) >= 8) {
            continue;
        }
        if (copied >= state.group_b_order.size()) {
            result.status =
                LegacyBattleGroupBOrderStatus::output_store_typed_stop;
            result.return_value = eax;
            result.final_ecx = copied;
            result.final_edx = kGroupBOrderBaseToken + copied * 4U;
            return result;
        }
        state.group_b_order[copied] = actor_index;
        ++copied;
        ++result.copied_slots;
        if (copied == requested_count) {
            result.reached_requested_count = true;
            break;
        }
    }

    result.return_value = eax;
    result.final_ecx = copied;
    result.final_edx = kGroupBOrderBaseToken + copied * 4U;
    state.entry_eax = result.return_value;
    state.entry_ecx = result.final_ecx;
    state.entry_edx = result.final_edx;
    return result;
}

}  // namespace openswd3::battle
