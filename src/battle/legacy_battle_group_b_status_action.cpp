#include "openswd3/battle/legacy_battle_group_b_status_action.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 read_word(
    const std::array<compat::u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

void consume_decision_random(
    LegacyBattleGroupBStatusActionResult& result,
    LegacyBattleBoundedRandomPort& random
) {
    result.decision_random_value = random.random_bounded(10U);
    ++result.random_calls;
    result.return_eax = result.decision_random_value;
    result.return_ecx = 0U;
    result.return_edx = result.decision_random_value;
    result.return_ecx_known = false;
}

void publish_comparison(
    LegacyBattleGroupBStatusActionResult& result, const u16 threshold
) noexcept {
    result.decision_threshold = threshold;
    result.return_eax =
        static_cast<u16>(result.decision_random_value) < threshold ? 1U : 0U;
}

}  // namespace

LegacyBattleGroupBStatusActionResult query_legacy_battle_group_b_status_action(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleGroupBStatusActionRequest request
) {
    LegacyBattleGroupBStatusActionResult result{
        .argument = static_cast<compat::u8>(request.entry_edx),
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBStatusActionStatus::actor_state_typed_stop;
        return result;
    }

    if ((actor->action_execution.retreat_ready_flags & 0x0800U) != 0U) {
        result.return_eax = 0U;
        return result;
    }

    result.initial_random_value = random.random_bounded(12U);
    result.random_calls = 1U;
    result.return_eax = result.initial_random_value;
    result.return_ecx = 0U;
    result.return_edx = result.initial_random_value;
    result.return_ecx_known = false;

    const u32 resource_token = actor->resource_token;
    result.return_eax = resource_token;
    if (resource_token == 0U) {
        result.status =
            LegacyBattleGroupBStatusActionStatus::resource_read_typed_stop;
        return result;
    }

    result.initial_resource_chance = actor->resource_bytes[0x91U];
    result.return_ecx = result.initial_resource_chance;
    if (result.initial_resource_chance == 0U) {
        result.return_eax = 0U;
        return result;
    }

    result.resource_base = read_word(actor->resource_bytes, 0x54U);
    result.return_edx = result.resource_base;
    result.signed_delta = signed_bits(
        static_cast<u32>(result.argument) -
        static_cast<u32>(result.resource_base)
    );

    if (result.signed_delta > 10) {
        consume_decision_random(result, random);
        publish_comparison(result, 8U);
        return result;
    }

    if (result.signed_delta > 5) {
        consume_decision_random(result, random);
        publish_comparison(result, 5U);
        return result;
    }

    if (result.signed_delta < 5) {
        result.return_eax = 0U;
        return result;
    }

    consume_decision_random(result, random);
    const u32 reread_resource_token = actor->resource_token;
    result.return_ecx = reread_resource_token;
    result.return_ecx_known = true;
    if (reread_resource_token == 0U) {
        result.status =
            LegacyBattleGroupBStatusActionStatus::resource_reread_typed_stop;
        return result;
    }

    const u16 threshold = actor->resource_bytes[0x91U];
    result.return_edx =
        (result.decision_random_value & 0xFFFF0000U) | threshold;
    publish_comparison(result, threshold);
    return result;
}

}  // namespace openswd3::battle
