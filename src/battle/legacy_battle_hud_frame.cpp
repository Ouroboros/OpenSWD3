#include "openswd3/battle/legacy_battle_hud_frame.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::i8;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kCallFontReset = 0x00435670U;
constexpr u32 kCallFontStyle = 0x00435660U;
constexpr u32 kCallResolveActor = 0x00480AD0U;
constexpr u32 kCallQueryPrimaryValues = 0x00484500U;
constexpr u32 kCallDrawExplicitFrame = 0x00450490U;
constexpr u32 kCallQueryStatusValue = 0x00478340U;
constexpr u32 kCallQueryColor = 0x004239D0U;
constexpr u32 kCallDrawColorFade = 0x00450A50U;
constexpr u32 kCallDrawPanel = 0x0043B110U;
constexpr u32 kCallQueryExcluded = 0x0047D930U;
constexpr u32 kCallQueryBlocked = 0x0047CE80U;
constexpr u32 kCallDrawLayeredWidth = 0x00450630U;
constexpr u32 kCallDrawText = 0x00436AD0U;
constexpr u32 kCallDrawActionFrame = 0x004502B0U;
constexpr u32 kCallPublishStatus = 0x0047E7F0U;
constexpr u32 kCallDrawLayeredTwo = 0x004505B0U;
constexpr u32 kCallDrawLayeredResource = 0x00450530U;
constexpr u32 kCallDrawIndexedFrame = 0x00450400U;
constexpr u32 kCallDrawSelectedResource = 0x004504E0U;
constexpr u32 kCallDrawDecimal = 0x004506B0U;
constexpr u32 kCallQuerySecondaryValues = 0x004838A0U;
constexpr u32 kCallQueryTertiaryValues = 0x00483870U;

constexpr u32 kFontToken = 0x004C9A28U;
constexpr u32 kSurfaceToken = 0x004CD76CU;
constexpr u32 kNameBaseToken = 0x0049E148U;
constexpr u32 kFooterDataToken = 0x004A7814U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return signed_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return signed_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_complement(const i32 value) noexcept {
    return signed_bits(~to_bits(value));
}

[[nodiscard]] constexpr u32 actor_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] u32 truncate_x87_low(const long double value) noexcept {
    if (!std::isfinite(value) ||
        value < static_cast<long double>(
                    std::numeric_limits<std::int64_t>::min()
                ) ||
        value > static_cast<long double>(
                    std::numeric_limits<std::int64_t>::max()
                )) {
        return 0U;
    }
    return static_cast<u32>(
        static_cast<std::uint64_t>(static_cast<std::int64_t>(std::trunc(value)))
    );
}

[[nodiscard]] long double
ratio_extended(const i32 current, const i32 maximum) noexcept {
    return static_cast<long double>(current) /
        static_cast<long double>(maximum) * 56.0L;
}

[[nodiscard]] float ratio_float(const i32 current, const i32 maximum) noexcept {
    return static_cast<float>(ratio_extended(current, maximum));
}

[[nodiscard]] i32 legacy_delta_step(const i32 delta) noexcept {
    i32 adjusted = delta;
    if ((to_bits(adjusted) & 0x08000000U) != 0U) {
        adjusted = wrapping_complement(adjusted);
    }
    return wrapping_add(adjusted / 10, 1);
}

void advance_delta(
    i32& delta, const i32 step, i32& current_snapshot, i32& sampled
) noexcept {
    if (delta < 0) {
        delta = wrapping_add(delta, step);
        if (delta > 0) {
            delta = 0;
        }
        return;
    }
    if (delta > 0) {
        delta = wrapping_subtract(delta, step);
        if (delta < 0) {
            delta = 0;
        }
        if (sampled <= 0) {
            sampled = 0;
            delta = 0;
            current_snapshot = 0;
        }
    }
}

}  // namespace

LegacyBattleHudFrameState::LegacyBattleHudFrameState() noexcept {
    actor_value_tokens.fill(1U);
}

LegacyBattleHudFrameResult advance_legacy_battle_hud_frame(
    LegacyBattleHudFrameState& state, LegacyBattleHudCallPort& port
) {
    LegacyBattleHudFrameResult result{};
    auto invoke = [&](const u32 callee,
                      const std::initializer_list<u32> arguments = {}) {
        LegacyBattleHudCallRequest request{};
        request.callee_token = callee;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        ++result.port_calls;
        return port.invoke_hud(request);
    };
    auto validate_actor = [&](const u32 index) {
        if (index < state.actor_active.size()) {
            return true;
        }
        result.status = LegacyBattleHudFrameStatus::actor_index_typed_stop;
        return false;
    };
    auto map_actor = [&](const u32 index, u32& mapped) {
        if (index >= state.display_order.size()) {
            result.status =
                LegacyBattleHudFrameStatus::display_order_typed_stop;
            return false;
        }
        mapped = state.display_order[index];
        if (mapped >= state.status_x.size()) {
            result.status =
                LegacyBattleHudFrameStatus::display_table_typed_stop;
            return false;
        }
        return true;
    };

    static_cast<void>(invoke(kCallFontReset, {kFontToken, 0U}));
    result.return_value = invoke(kCallFontStyle, {kFontToken, 0xFFFEU}).eax;

    u32 top_index = 0U;
    i32 top_y = 10;
    while (signed_bits(top_index) < state.active_actor_count) {
        if (!validate_actor(top_index)) {
            return result;
        }
        const i32 top_x = state.side_mode == 1U ? 10 : 450;
        if (state.actor_active[top_index] == 1U) {
            const u32 token = actor_token(top_index);
            const u32 resolved = invoke(kCallResolveActor, {token}).eax;
            result.text_panels.push_back(draw_legacy_battle_text_panel(
                port.battle_victory_reward_state(),
                port,
                {
                    .left = top_x,
                    .top = top_y,
                    .width = 170,
                    .height = 20,
                    .text_x = wrapping_add(top_x, 5),
                    .text_y = wrapping_add(top_y, 2),
                    .text_token = resolved,
                    .entry = {
                        .eax = resolved,
                        .ecx = to_bits(wrapping_add(top_y, 2)),
                        .edx = to_bits(wrapping_add(top_x, 5)),
                    },
                }
            ));
            ++result.text_panel_calls;
            result.port_calls += result.text_panels.back().port_calls;
            const auto primary = invoke(kCallQueryPrimaryValues, {token});
            ++result.x87_conversions;
            const u32 width = truncate_x87_low(ratio_extended(
                signed_bits(primary.outputs[0]), signed_bits(primary.outputs[1])
            ));
            static_cast<void>(invoke(
                kCallDrawExplicitFrame,
                {0x2350U,
                 1U,
                 to_bits(wrapping_add(top_x, 100)),
                 to_bits(wrapping_add(top_y, 8)),
                 width}
            ));
            const u32 status_value =
                static_cast<u16>(invoke(kCallQueryStatusValue, {token}).eax);
            if (status_value != 0U) {
                const u32 color = invoke(kCallQueryColor, {0U, 0U, 24U}).eax;
                static_cast<void>(invoke(
                    kCallDrawColorFade,
                    {to_bits(wrapping_add(top_x, 100)),
                     to_bits(wrapping_add(top_y, 12)),
                     status_value,
                     3U,
                     color}
                ));
            }
            if (top_index ==
                    to_bits(wrapping_subtract(state.active_actor_count, 1)) &&
                state.top_pulse < 0) {
                u8 pulse =
                    static_cast<u8>(std::bit_cast<u8>(state.top_pulse) + 3U);
                if ((pulse & 0x7FU) > 28U) {
                    pulse = 0U;
                }
                state.top_pulse = std::bit_cast<i8>(pulse);
                static_cast<void>(invoke(
                    kCallDrawPanel,
                    {to_bits(top_x),
                     to_bits(wrapping_subtract(top_y, 4)),
                     170U,
                     28U,
                     static_cast<u32>(pulse & 0x7FU),
                     0U,
                     0U,
                     5U}
                ));
            }
            top_y = wrapping_add(top_y, 28);
            ++result.top_actor_rows;
        }
        ++top_index;
        result.return_value = top_index;
    }

    u32 actor_index = 0U;
    while (signed_bits(actor_index) < state.active_actor_count) {
        if (!validate_actor(actor_index)) {
            return result;
        }
        if (state.actor_skip_primary[actor_index] == 1U ||
            state.actor_skip_secondary[actor_index] == 1U ||
            invoke(kCallQueryExcluded, {actor_token(actor_index)}).eax == 1U) {
            ++actor_index;
            result.return_value = to_bits(state.active_actor_count);
            continue;
        }

        u32 mapped = 0U;
        if (!map_actor(actor_index, mapped)) {
            return result;
        }
        const bool selected =
            to_bits(wrapping_subtract(state.selected_actor_code, 8)) ==
            actor_index;
        if (selected) {
            const u32 frame = state.selected_pulse & 0x7FU;
            static_cast<void>(invoke(
                kCallDrawPanel,
                {to_bits(wrapping_subtract(state.value_x[mapped], 4)),
                 394U,
                 124U,
                 76U,
                 frame,
                 frame,
                 frame,
                 1U}
            ));
            state.selected_pulse_counter = state.selected_pulse_counter + 1U;
            if (state.selected_pulse_counter == 3U) {
                state.selected_pulse_counter = 0U;
                u8 pulse = state.selected_pulse;
                if (std::bit_cast<i8>(pulse) < 0) {
                    pulse = static_cast<u8>(pulse - 1U);
                    if ((pulse & 0x7FU) == 0U) {
                        pulse = 0U;
                    }
                }
                if (std::bit_cast<i8>(pulse) >= 0) {
                    pulse = static_cast<u8>(pulse + 1U);
                    if (pulse > 8U) {
                        pulse = static_cast<u8>(pulse | 0x80U);
                    }
                }
                state.selected_pulse = pulse;
            }
        }

        const u32 token = actor_token(actor_index);
        const i32 status_x = state.status_x[mapped];
        if (invoke(kCallQueryBlocked, {token}).eax == 0U) {
            const u32 status_value =
                static_cast<u16>(invoke(kCallQueryStatusValue, {token}).eax);
            if (state.actor_status_mode[actor_index] == 0U) {
                static_cast<void>(invoke(
                    kCallDrawLayeredWidth,
                    {0x234FU, to_bits(status_x), 394U, status_value}
                ));
            }
        }
        const u32 name_token = kNameBaseToken + mapped * 16U;
        if (invoke(kCallQueryBlocked, {token}).eax == 1U) {
            static_cast<void>(invoke(
                kCallDrawText,
                {kFontToken,
                 kSurfaceToken,
                 to_bits(status_x),
                 394U,
                 name_token,
                 0xF000U,
                 16U}
            ));
        }
        if (state.actor_status_mode[actor_index] == 1U) {
            static_cast<void>(invoke(
                kCallDrawText,
                {kFontToken,
                 kSurfaceToken,
                 to_bits(status_x),
                 394U,
                 name_token,
                 0xFFFFU,
                 16U}
            ));
            if (selected) {
                const u32 color =
                    state.status_blink_counter[actor_index] % 4U != 0U
                    ? 0xF3F0U
                    : 0xFFFFU;
                static_cast<void>(invoke(
                    kCallDrawText,
                    {kFontToken,
                     kSurfaceToken,
                     to_bits(status_x),
                     394U,
                     name_token,
                     color,
                     16U}
                ));
                ++state.status_blink_counter[actor_index];
            }
        }
        const bool blocked = invoke(kCallQueryBlocked, {token}).eax == 1U;
        static_cast<void>(invoke(
            kCallDrawActionFrame,
            {mapped, to_bits(state.value_x[mapped]), 460U, blocked ? 1U : 0U}
        ));
        static_cast<void>(
            invoke(kCallPublishStatus, {to_bits(state.bar_x[mapped]), 462U})
        );

        i32& actor_display = state.actor_value_display[actor_index];
        i32& actor_target = state.actor_value_target[actor_index];
        if (actor_display == actor_target) {
            if (state.actor_value_tokens[actor_index] == 0U) {
                result.status =
                    LegacyBattleHudFrameStatus::actor_value_typed_stop;
                return result;
            }
            actor_display = actor_target;
            const i32 previous_target = actor_target;
            actor_target = state.actor_value[actor_index];
            static_cast<void>(invoke(
                kCallDrawLayeredResource,
                {0x2352U,
                 to_bits(state.value_x[mapped]),
                 466U,
                 to_bits(previous_target)}
            ));
        } else {
            static_cast<void>(invoke(
                kCallDrawLayeredTwo,
                {0x2352U,
                 to_bits(state.value_x[mapped]),
                 466U,
                 to_bits(actor_display)}
            ));
            if (actor_display < actor_target) {
                actor_display = wrapping_add(
                    actor_display,
                    wrapping_subtract(actor_target, actor_display) <= 10 ? 1 : 3
                );
            } else {
                actor_display = wrapping_subtract(
                    actor_display,
                    wrapping_subtract(actor_display, actor_target) <= 10 ? 2 : 5
                );
            }
        }
        if (state.actor_value_tokens[actor_index] == 0U) {
            result.status = LegacyBattleHudFrameStatus::actor_value_typed_stop;
            return result;
        }
        if (state.actor_value[actor_index] == 56 &&
            invoke(kCallQueryBlocked, {token}).eax == 0U) {
            static_cast<void>(invoke(
                kCallDrawIndexedFrame,
                {to_bits(state.value_x[mapped]), 466U, actor_index}
            ));
        }

        const auto primary = invoke(kCallQueryPrimaryValues, {token});
        i32 primary_sample = signed_bits(primary.outputs[0]);
        const i32 primary_maximum = signed_bits(primary.outputs[1]);
        const float primary_ratio =
            ratio_float(primary_sample, primary_maximum);
        if (state.primary_value_snapshot[actor_index] != primary_sample) {
            const i32 delta = wrapping_subtract(
                state.primary_value_snapshot[actor_index], primary_sample
            );
            state.primary_delta[actor_index] = delta;
            state.primary_value_snapshot[actor_index] = primary_sample;
            state.primary_step[actor_index] = legacy_delta_step(delta);
        }
        advance_delta(
            state.primary_delta[actor_index],
            state.primary_step[actor_index],
            state.primary_value_snapshot[actor_index],
            primary_sample
        );
        static_cast<void>(invoke(
            kCallDrawSelectedResource,
            {0x2350U,
             3U,
             to_bits(wrapping_subtract(state.bar_x[mapped], 8)),
             419U}
        ));
        i32& primary_display = state.primary_display[actor_index];
        i32& primary_target = state.primary_display_target[actor_index];
        if (primary_display == primary_target) {
            primary_display = primary_target;
            const i32 previous_target = primary_target;
            ++result.x87_conversions;
            primary_target = signed_bits(truncate_x87_low(primary_ratio));
            static_cast<void>(invoke(
                kCallDrawLayeredResource,
                {0x2350U,
                 to_bits(state.bar_x[mapped]),
                 422U,
                 to_bits(previous_target)}
            ));
        } else {
            static_cast<void>(invoke(
                kCallDrawLayeredTwo,
                {0x2350U,
                 to_bits(state.bar_x[mapped]),
                 422U,
                 to_bits(primary_display)}
            ));
            if (primary_display < primary_target) {
                primary_display = wrapping_add(
                    primary_display,
                    wrapping_subtract(primary_target, primary_display) <= 10 ? 1
                                                                             : 3
                );
            } else {
                primary_display = wrapping_subtract(
                    primary_display,
                    wrapping_subtract(primary_display, primary_target) <= 10 ? 1
                                                                             : 3
                );
            }
        }
        u32 primary_resource = 0x2356U;
        if (primary_sample > 10) {
            primary_resource =
                primary_sample > primary_maximum / 3 ? 0x2354U : 0x2355U;
        }
        static_cast<void>(invoke(
            kCallDrawDecimal,
            {primary_resource,
             to_bits(
                 wrapping_add(state.primary_delta[actor_index], primary_sample)
             ),
             to_bits(wrapping_add(state.bar_x[mapped], 38)),
             412U}
        ));

        const auto secondary = invoke(kCallQuerySecondaryValues, {token});
        i32 secondary_sample =
            static_cast<i32>(signed_word(secondary.outputs[0]));
        const i32 secondary_maximum =
            static_cast<i32>(signed_word(secondary.outputs[1]));
        const float secondary_ratio =
            ratio_float(secondary_sample, secondary_maximum);
        if (state.secondary_value_snapshot[actor_index] != secondary_sample) {
            const i32 delta = wrapping_subtract(
                state.secondary_value_snapshot[actor_index], secondary_sample
            );
            state.secondary_delta[actor_index] = delta;
            state.secondary_step[actor_index] = legacy_delta_step(delta);
            state.secondary_value_snapshot[actor_index] = secondary_sample;
        }
        advance_delta(
            state.secondary_delta[actor_index],
            state.secondary_step[actor_index],
            state.secondary_value_snapshot[actor_index],
            secondary_sample
        );
        static_cast<void>(invoke(
            kCallDrawSelectedResource,
            {0x235FU,
             3U,
             to_bits(wrapping_subtract(state.bar_x[mapped], 8)),
             436U}
        ));
        i32& secondary_display = state.secondary_display[actor_index];
        i32& secondary_target = state.secondary_display_target[actor_index];
        if (secondary_display == secondary_target) {
            secondary_display = secondary_target;
            const i32 previous_target = secondary_target;
            ++result.x87_conversions;
            secondary_target = signed_bits(truncate_x87_low(secondary_ratio));
            static_cast<void>(invoke(
                kCallDrawLayeredResource,
                {0x235FU,
                 to_bits(state.bar_x[mapped]),
                 439U,
                 to_bits(previous_target)}
            ));
        } else {
            static_cast<void>(invoke(
                kCallDrawLayeredTwo,
                {0x235FU,
                 to_bits(state.bar_x[mapped]),
                 439U,
                 to_bits(secondary_display)}
            ));
            if (secondary_display < secondary_target) {
                secondary_display = wrapping_add(
                    secondary_display,
                    wrapping_subtract(secondary_target, secondary_display) <= 10
                        ? 1
                        : 3
                );
            } else {
                secondary_display = wrapping_subtract(
                    secondary_display,
                    wrapping_subtract(secondary_display, secondary_target) <= 10
                        ? 1
                        : 3
                );
            }
        }
        static_cast<void>(invoke(
            kCallDrawDecimal,
            {0x2354U,
             to_bits(wrapping_add(
                 state.secondary_delta[actor_index], secondary_sample
             )),
             to_bits(wrapping_add(state.bar_x[mapped], 38)),
             429U}
        ));

        const auto tertiary = invoke(kCallQueryTertiaryValues, {token});
        i32 tertiary_sample =
            static_cast<i32>(signed_word(tertiary.outputs[0]));
        const i32 tertiary_maximum =
            static_cast<i32>(signed_word(tertiary.outputs[1]));
        const float tertiary_ratio =
            ratio_float(tertiary_sample, tertiary_maximum);
        if (state.tertiary_value_snapshot[actor_index] != tertiary_sample) {
            const i32 delta = wrapping_subtract(
                state.tertiary_value_snapshot[actor_index], tertiary_sample
            );
            state.tertiary_delta[actor_index] = delta;
            state.tertiary_value_snapshot[actor_index] = tertiary_sample;
            state.tertiary_step[actor_index] = legacy_delta_step(delta);
        }
        advance_delta(
            state.tertiary_delta[actor_index],
            state.tertiary_step[actor_index],
            state.tertiary_value_snapshot[actor_index],
            tertiary_sample
        );
        static_cast<void>(invoke(
            kCallDrawSelectedResource,
            {0x241FU,
             3U,
             to_bits(wrapping_subtract(state.bar_x[mapped], 8)),
             453U}
        ));
        i32& tertiary_display = state.tertiary_display[actor_index];
        i32& tertiary_target = state.tertiary_display_target[actor_index];
        if (tertiary_display == tertiary_target) {
            tertiary_display = tertiary_target;
            const i32 previous_target = tertiary_target;
            ++result.x87_conversions;
            tertiary_target = signed_bits(truncate_x87_low(tertiary_ratio));
            static_cast<void>(invoke(
                kCallDrawLayeredResource,
                {0x241FU,
                 to_bits(state.bar_x[mapped]),
                 456U,
                 to_bits(previous_target)}
            ));
        } else {
            static_cast<void>(invoke(
                kCallDrawLayeredTwo,
                {0x241FU,
                 to_bits(state.bar_x[mapped]),
                 456U,
                 to_bits(tertiary_display)}
            ));
            if (tertiary_display < tertiary_target) {
                tertiary_display = wrapping_add(
                    tertiary_display,
                    wrapping_subtract(tertiary_target, tertiary_display) <= 10
                        ? 1
                        : 3
                );
            } else {
                tertiary_display = wrapping_subtract(
                    tertiary_display,
                    wrapping_subtract(tertiary_display, tertiary_target) <= 10
                        ? 1
                        : 3
                );
            }
        }
        static_cast<void>(invoke(
            kCallDrawDecimal,
            {0x2354U,
             to_bits(wrapping_add(
                 state.tertiary_delta[actor_index], tertiary_sample
             )),
             to_bits(wrapping_add(state.bar_x[mapped], 38)),
             446U}
        ));

        ++result.actor_rows;
        ++actor_index;
        result.return_value = to_bits(state.active_actor_count);
    }

    if (state.footer_mode == 1U) {
        const i32 numerator = wrapping_subtract(68, state.footer_position);
        const i32 delta = numerator / 3;
        state.footer_position = wrapping_add(state.footer_position, delta);
        state.footer_delta = delta;
        result.text_panels.push_back(draw_legacy_battle_text_panel(
            port.battle_victory_reward_state(),
            port,
            {
                .left = wrapping_subtract(state.footer_position, 58),
                .top = 354,
                .width = 70,
                .height = 24,
                .text_x = 0,
                .text_y = 0,
                .text_token = kFooterDataToken,
                .entry = {
                    .eax = numerator < 0 ? 1U : 0U,
                    .ecx = to_bits(numerator),
                    .edx = to_bits(delta),
                },
            }
        ));
        ++result.text_panel_calls;
        result.port_calls += result.text_panels.back().port_calls;
        result.return_value = result.text_panels.back().return_registers.eax;
    }
    return result;
}

}  // namespace openswd3::battle
