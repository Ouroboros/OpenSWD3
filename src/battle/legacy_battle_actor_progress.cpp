#include "openswd3/battle/legacy_battle_actor_progress.hpp"

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

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

[[nodiscard]] constexpr i32
percentage(const i32 value, const u32 percent) noexcept {
    const i32 product = std::bit_cast<i32>(std::bit_cast<u32>(value) * percent);
    return product / 100;
}

[[nodiscard]] constexpr i32 thirty_percent(const i32 value) noexcept {
    return percentage(value, 30U);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

[[nodiscard]] constexpr u16 group_b_base_speed(
    const LegacyBattleActorGroupBElementState& element
) noexcept {
    return static_cast<u16>(element.resource_bytes[0x5AU]) |
        static_cast<u16>(static_cast<u16>(element.resource_bytes[0x5BU]) << 8U);
}

struct X87TruncateResult {
    u32 eax{};
    u32 edx{};
};

[[nodiscard]] X87TruncateResult
truncate_x87_integer(const long double value) noexcept {
    constexpr u32 kIndefiniteHighDword = 0x80000000U;
    if (!std::isfinite(value)) {
        return {.eax = 0U, .edx = kIndefiniteHighDword};
    }
    const long double truncated = std::trunc(value);
    constexpr long double minimum =
        static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    constexpr long double maximum_exclusive = -minimum;
    if (truncated < minimum || truncated >= maximum_exclusive) {
        return {.eax = 0U, .edx = kIndefiniteHighDword};
    }
    const auto converted = static_cast<std::int64_t>(truncated);
    const auto bits = std::bit_cast<std::uint64_t>(converted);
    return {
        .eax = static_cast<u32>(bits),
        .edx = static_cast<u32>(bits >> 32U),
    };
}

}  // namespace

LegacyBattleActorProgressWidthResult query_legacy_battle_actor_progress_width(
    const LegacyBattleActorProgressState* const actor,
    const LegacyBattleTimingState* const timing,
    const LegacyBattleActorProgressWidthRequest& request
) noexcept {
    LegacyBattleActorProgressWidthResult result{
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    result.return_eax = 0U;
    if (actor == nullptr || !actor->progress_read_accessible) {
        result.status = LegacyBattleActorProgressWidthStatus::
            actor_progress_read_typed_stop;
        return result;
    }

    result.progress_value = static_cast<u16>(actor->progress);
    result.return_eax = result.progress_value;
    result.x87_stack_depth = 1U;
    if (timing == nullptr || !timing->action_threshold_read_accessible) {
        result.status = LegacyBattleActorProgressWidthStatus::
            action_threshold_read_typed_stop;
        return result;
    }

    const volatile long double ratio =
        static_cast<long double>(result.progress_value) /
        static_cast<long double>(timing->action_threshold);
    const volatile long double scaled = ratio * 62.0L;
    const X87TruncateResult converted = truncate_x87_integer(scaled);
    result.return_eax = converted.eax;
    result.return_edx = converted.edx;
    result.truncate_calls = 1U;
    result.x87_stack_depth = 0U;
    return result;
}

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
    replace_low_word(state.progress, static_cast<u16>(updated));
    result.base_increment = std::bit_cast<u32>(increment);
    result.positive_adjustment = std::bit_cast<u32>(positive);
    result.negative_adjustment = std::bit_cast<u32>(negative);
    return result;
}

LegacyBattleActorGroupBProgressResult
advance_legacy_battle_actor_group_b_progress(
    LegacyBattleActorProgressState& state,
    const LegacyBattleActorGroupBElementState* const element,
    const i32 argument,
    const i32 completion_threshold,
    const u32 object_token,
    const u32 entry_edx
) noexcept {
    LegacyBattleActorGroupBProgressResult result{
        .return_ecx = object_token,
        .return_edx = entry_edx,
    };
    const u16 status = static_cast<u16>(state.mode_gate);
    if ((status & 0x0040U) != 0U || (status & 0x4000U) != 0U) {
        return result;
    }

    const u16 progress = static_cast<u16>(state.progress);
    if (static_cast<i32>(static_cast<u32>(progress)) >= completion_threshold) {
        const u32 started = state.frame_started;
        state.action_complete = 1U;
        state.transition_value = 0U;
        result.return_eax = 1U;
        result.return_edx = started;
        if (started == 1U) {
            state.frame_started = 0U;
            state.post_action_value = 0U;
        }
        return result;
    }

    const u32 resource_token =
        element == nullptr ? 0U : element->resource_token;
    result.return_eax = (static_cast<u32>(progress) & 0x0000FF00U) |
        static_cast<compat::u8>(state.delay_mode);
    result.return_edx = resource_token;
    if (element == nullptr || resource_token == 0U) {
        result.status =
            LegacyBattleActorGroupBProgressStatus::resource_typed_stop;
        return result;
    }

    i32 increment = static_cast<i32>(group_b_base_speed(*element) >> 2U);
    if ((state.delay_mode & 0x40U) != 0U) {
        increment >>= 1U;
    }
    if (argument == 1) {
        result.return_edx = 0U;
        increment = wrapping_add(increment, increment / 4);
        increment = wrapping_add(increment, 1);
    }

    i32 positive = 0;
    i32 negative = 0;
    if ((state.delay_mode & 0x20000000U) != 0U) {
        positive = thirty_percent(increment);
        result.return_edx = std::bit_cast<u32>(positive);
    }
    if ((state.delay_mode & 0x08000000U) != 0U) {
        negative = thirty_percent(increment);
        result.return_edx = std::bit_cast<u32>(negative);
    }
    if (std::bit_cast<i32>(state.delay_mode) < 0) {
        const i32 additional = percentage(increment, 10U);
        negative = wrapping_add(negative, additional);
        result.return_edx = std::bit_cast<u32>(additional);
    }

    i32 updated = wrapping_subtract(static_cast<i32>(progress), negative);
    state.action_complete = 0U;
    updated = wrapping_add(updated, positive);
    updated = wrapping_add(updated, increment);
    replace_low_word(state.progress, static_cast<u16>(updated));
    result.return_eax = 0U;
    result.base_increment = std::bit_cast<u32>(increment);
    result.positive_adjustment = std::bit_cast<u32>(positive);
    result.negative_adjustment = std::bit_cast<u32>(negative);
    return result;
}

}  // namespace openswd3::battle
