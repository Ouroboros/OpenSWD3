#include "openswd3/battle/legacy_battle_actor_progress.hpp"

#include <bit>
#include <cstdint>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) + std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32 thirty_percent(const i32 value) noexcept {
    const i32 product = std::bit_cast<i32>(std::bit_cast<u32>(value) * 30U);
    return product / 100;
}

}  // namespace

LegacyBattleActorProgressResult advance_legacy_battle_actor_progress(
    LegacyBattleActorProgressState& state,
    const i32 argument,
    const i32 completion_threshold,
    const u32 object_token
) noexcept {
    LegacyBattleActorProgressResult result{
        .return_ecx = object_token,
    };
    const u16 status = static_cast<u16>(state.mode_gate);
    if ((status & 0x0040U) != 0U || (status & 0x4000U) != 0U ||
        state.special_ready == 1U || state.action_complete == 1U) {
        return result;
    }

    const u16 progress = static_cast<u16>(state.progress);
    if (static_cast<i32>(static_cast<u32>(progress)) >= completion_threshold) {
        const u32 started = state.frame_started;
        state.action_complete = 1U;
        state.transition_value = 0U;
        if (started == 1U) {
            const u32 scene = state.scene_identity;
            state.frame_started = 0U;
            if (scene == 0U) {
                state.post_action_value = 0U;
            }
        }
        state.cache_x = 0U;
        state.cache_y = 0U;
        state.update_ready = 1U;
        result.return_eax = 1U;
        return result;
    }

    i32 increment = static_cast<i32>(state.base_speed >> 2U);
    if ((state.delay_mode & 0x40U) != 0U) {
        increment >>= 1U;
    }
    if (argument == 1) {
        increment = wrapping_add(increment, increment / 4);
        increment = wrapping_add(increment, 1);
    }

    i32 positive = 0;
    i32 negative = 0;
    if ((state.delay_mode & 0x20000000U) != 0U) {
        positive = thirty_percent(increment);
    }
    if ((state.delay_mode & 0x08000000U) != 0U) {
        negative = thirty_percent(increment);
    }
    if (std::bit_cast<i32>(state.delay_mode) < 0) {
        const i32 product = std::bit_cast<i32>(
            static_cast<u32>(state.progress_multiplier) *
            std::bit_cast<u32>(increment)
        );
        negative = wrapping_add(negative, product / 100 / 2);
        negative = wrapping_add(negative, 4);
    }

    i32 updated = wrapping_subtract(static_cast<i32>(progress), negative);
    updated = wrapping_add(updated, positive);
    updated = wrapping_add(updated, increment);
    state.action_complete = 0U;
    state.progress = static_cast<u16>(updated);
    result.base_increment = std::bit_cast<u32>(increment);
    result.positive_adjustment = std::bit_cast<u32>(positive);
    result.negative_adjustment = std::bit_cast<u32>(negative);
    return result;
}

}  // namespace openswd3::battle
