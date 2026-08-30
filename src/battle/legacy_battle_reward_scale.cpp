#include "openswd3/battle/legacy_battle_reward_scale.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

constexpr compat::u32 kCallRefreshPercent = 0x00482F10U;
constexpr compat::u32 kCallConfigureAction = 0x004830A0U;

}  // namespace

LegacyBattleRewardScaleResult scale_legacy_battle_reward(
    LegacyBattleRewardScaleActorState* actor,
    compat::u32* value,
    LegacyBattleEffectCallPort& port,
    const LegacyBattleRewardScaleRequest& request
) {
    LegacyBattleRewardScaleResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status = LegacyBattleRewardScaleStatus::actor_state_typed_stop;
        return result;
    }
    if ((actor->status_bits & 0x10U) == 0U) {
        result.return_eax = 0U;
        return result;
    }

    ++result.percent_refresh_calls;
    ++result.port_calls;
    auto reply = port.invoke({
        .callee_token = kCallRefreshPercent,
        .arguments = {0x37U},
        .eax = result.return_eax,
        .ecx = request.actor_token,
        .edx = result.return_edx,
    });
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;

    ++result.action_configure_calls;
    ++result.port_calls;
    reply = port.invoke({
        .callee_token = kCallConfigureAction,
        .arguments = {0x004B8A00U, 0x37U, 0x0CU},
        .eax = result.return_eax,
        .ecx = request.actor_token,
        .edx = result.return_edx,
    });
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;

    actor->percent = static_cast<compat::u16>(actor->percent >> 1U);
    if (value == nullptr) {
        result.status = LegacyBattleRewardScaleStatus::value_typed_stop;
        return result;
    }

    const compat::u32 product =
        *value * static_cast<compat::u32>(actor->percent);
    const compat::i32 signed_product = std::bit_cast<compat::i32>(product);
    const compat::i32 scaled = signed_product / 100 + 1;
    *value = std::bit_cast<compat::u32>(scaled);
    result.scaled_value = *value;
    result.return_eax = 1U;
    return result;
}

}  // namespace openswd3::battle
