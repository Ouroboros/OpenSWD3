#include "openswd3/battle/legacy_battle_group_b_opponent_mode.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 read_word(
    const std::array<compat::u8, 0xA4>& bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr u32 read_dword(
    const std::array<compat::u8, 0xA4>& bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleGroupBOpponentModeResult
select_legacy_battle_group_b_opponent_mode(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleBoundedRandomPort& random
) {
    LegacyBattleGroupBOpponentModeResult result{};
    result.random_value = random.random_bounded(10U);
    result.normalized_random = result.random_value;
    result.random_calls = 1U;
    result.return_eax = result.random_value;
    result.return_edx = result.random_value;

    const u16 random_low = static_cast<u16>(result.random_value);
    if (random_low <= 4U) {
        result.normalized_random = 0U;
    } else if (random_low >= 5U && random_low <= 9U) {
        result.normalized_random = 1U;
    }

    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBOpponentModeStatus::actor_state_typed_stop;
        result.return_ecx_known = false;
        return result;
    }

    result.return_ecx = actor->resource_token;
    result.return_eax = actor->action_configuration.timing_value;
    if (actor->resource_token == 0U) {
        result.status =
            LegacyBattleGroupBOpponentModeStatus::resource_read_typed_stop;
        return result;
    }

    const auto& resource = actor->resource_bytes;
    i32 threshold = static_cast<i32>(
        std::bit_cast<i16>(read_word(resource, 0x64U))
    );
    if (actor->action_configuration.timing_value != 0U) {
        threshold =
            std::bit_cast<i32>(actor->action_configuration.timing_value);
    }

    const i32 resource_third =
        std::bit_cast<i32>(read_dword(resource, 0x4CU)) / 3;
    result.return_edx = std::bit_cast<u32>(resource_third);

    if (threshold >= resource_third) {
        result.return_eax = 0U;
        return result;
    }

    const u16 mode = static_cast<u16>(result.normalized_random);
    if (mode == 0U) {
        if (read_word(resource, 0x7CU) == 0U ||
            (actor->action_execution.retreat_ready_flags & 0x0100U) != 0U ||
            resource[0x8EU] == 0U) {
            result.return_eax = 0U;
            return result;
        }

        actor->action_execution.opponent_mode = 1U;
        result.return_eax = 1U;
        return result;
    }

    if (mode == 1U && read_word(resource, 0x80U) != 0U &&
        (actor->action_execution.retreat_ready_flags & 0x0100U) == 0U &&
        resource[0x8EU] != 0U) {
        actor->action_execution.opponent_mode = 2U;
        result.return_eax = 1U;
        return result;
    }

    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
