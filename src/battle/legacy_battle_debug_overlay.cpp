#include "openswd3/battle/legacy_battle_debug_overlay.hpp"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kFontToken = 0x004C9A28U;
constexpr u32 kSurfaceToken = 0x004CD76CU;
constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupAStride = 0x00002F34U;
constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x00002B28U;
constexpr u32 kTextForeground = 0x0000FFFFU;
constexpr u32 kTextHeight = 0x00000010U;
constexpr u16 kMarkerPixel = 0xEEEEU;

constexpr char kGroupBFormat[] =
    "\xA5" "\xCD" "\xA9" "\x52" ":%-3d " "\xC2" "\xEA" "\xA9" "\x77" ":%-1d " "\xA9" "\x52" "\xA5" "\x4F" ":%-1d lv:%-2d";
constexpr char kGroupAFormat[] =
    "\xC2" "\xEA" "\xA9" "\x77" ":%d " "\xA9" "\x52" "\xA5" "\x4F" ":%-3d";
constexpr char kAttackOrderText[] =
    "\xA7" "\xF0" "\xC0" "\xBB" "\xB6" "\xB6" "\xA7" "\xC7" ":";
constexpr char kWaitingOrderText[] =
    "\xA7" "\xDA" "\xA4" "\xE8" "\xB5" "\xA5" "\xAB" "\xDD" "\xB6" "\xB6" "\xA7" "\xC7" ":";
constexpr char kSelectionOrderText[] =
    "\xB1" "\xC6" "\xA6" "\x43" "\xB6" "\xB6" "\xA7" "\xC7" ":";
constexpr char kCurrentSelectionFormat[] =
    "\xA5" "\xD8" "\xAB" "\x65" "\xA8" "\xA4" "\xA6" "\xE2" ":%d SEL:%d";
constexpr char kBattleSummaryFormat[] =
    "\xB5" "\x4C" "\xBC" "\xC4" ":%d UM:%d " "\xBE" "\xD4" "\xB0" "\xAB" ":%d";

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 signed_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr u32
wrapping_mul(const u32 left, const u32 right) noexcept {
    return static_cast<u32>(
        static_cast<std::uint64_t>(left) * static_cast<std::uint64_t>(right)
    );
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kGroupABaseToken + index * kGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kGroupBBaseToken + index * kGroupBStride;
}

template <typename... Args>
void format_text(
    LegacyBattleDebugOverlayState& state,
    LegacyBattleDebugOverlayResult& result,
    const char* const format,
    Args... args
) {
    static_cast<void>(std::snprintf(
        state.text_buffer.data(), state.text_buffer.size(), format, args...
    ));
    ++result.formatted_texts;
}

class Runner {
public:
    Runner(
        LegacyBattleDebugOverlayBindings bindings,
        LegacyBattleDebugOverlayPort& port,
        LegacyBattleDebugOverlayResult& result
    ) noexcept
        : bindings_(bindings), port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleDebugOverlayCallReply call(
        const LegacyBattleDebugOverlayCall call,
        const u32 object_token = 0U,
        const std::array<u32, 4>& arguments = {},
        const u32 argument_count = 0U
    ) {
        ++result_.port_calls;
        return port_.invoke_debug_overlay({
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .argument_count = argument_count,
        });
    }

    void draw(const u32 x, const u32 y, const std::string_view text) {
        ++result_.port_calls;
        ++result_.text_draws;
        static_cast<void>(port_.draw_debug_overlay_text({
            .font_token = kFontToken,
            .surface_token = kSurfaceToken,
            .x = x,
            .y = y,
            .text = std::string{text},
            .foreground = kTextForeground,
            .height = kTextHeight,
        }));
    }

    void draw_buffer(const u32 x, const u32 y) {
        draw(x, y, bindings_.overlay.text_buffer.data());
    }

    [[nodiscard]] std::optional<u16> read_actor_word(const u32 token) {
        return port_.read_debug_actor_level_word_54(token);
    }

private:
    LegacyBattleDebugOverlayBindings bindings_;
    LegacyBattleDebugOverlayPort& port_;
    LegacyBattleDebugOverlayResult& result_;
};

[[nodiscard]] bool write_marker_pixel(
    LegacyBattleDebugOverlayBindings bindings,
    LegacyBattleDebugOverlayResult& result,
    const u32 index
) {
    auto pixels = bindings.framebuffer.physical_pixels();
    if (static_cast<std::size_t>(index) >= pixels.size()) {
        result.status = LegacyBattleDebugOverlayStatus::framebuffer_typed_stop;
        return false;
    }
    pixels[index] = kMarkerPixel;
    ++result.marker_pixels;
    return true;
}

}  // namespace

LegacyBattleDebugOverlayResult draw_legacy_battle_debug_overlay(
    LegacyBattleDebugOverlayBindings bindings,
    LegacyBattleDebugOverlayPort& port,
    const LegacyBattleDebugOverlayRequest& request
) {
    LegacyBattleDebugOverlayResult result;
    Runner runner(bindings, port, result);
    auto& state = bindings.overlay;

    if (bindings.hotkeys.toggle_5244e0 == 1U) {
        static_cast<void>(runner.call(
            LegacyBattleDebugOverlayCall::font_style,
            kFontToken,
            {0x0000FFFEU},
            1U
        ));
        static_cast<void>(runner.call(
            LegacyBattleDebugOverlayCall::font_reset, kFontToken, {0U}, 1U
        ));

        u32 vitality = request.vitality_stack_snapshot;
        u32 index = 0U;
        u32 y = 10U;
        while (index < bindings.metrics.group_b_count) {
            const u32 actor = group_b_token(index);
            const auto resolved = runner.call(
                LegacyBattleDebugOverlayCall::resolve_group_b_actor, actor
            );
            state.resolved_actor_token = resolved.eax;

            const auto vitality_reply = runner.call(
                LegacyBattleDebugOverlayCall::query_group_b_vitality, actor
            );
            if ((vitality_reply.output_mask & 1U) != 0U) {
                vitality = vitality_reply.output_0;
            }

            const std::optional<u16> actor_level =
                runner.read_actor_word(state.resolved_actor_token);
            if (!actor_level.has_value()) {
                result.status = LegacyBattleDebugOverlayStatus::
                    resolved_actor_word_typed_stop;
                return result;
            }
            const u32 command =
                runner
                    .call(
                        LegacyBattleDebugOverlayCall::query_actor_command,
                        actor,
                        {*actor_level},
                        1U
                    )
                    .eax &
                0xFFFFU;
            const u32 lock =
                runner
                    .call(
                        LegacyBattleDebugOverlayCall::query_actor_lock,
                        actor,
                        {command},
                        1U
                    )
                    .eax &
                0xFFFFU;
            format_text(
                state,
                result,
                kGroupBFormat,
                signed_bits(vitality),
                static_cast<int>(lock),
                static_cast<int>(command),
                static_cast<int>(*actor_level)
            );
            runner.draw_buffer(10U, y);
            ++result.group_b_rows;
            ++index;
            y += 20U;
        }

        index = 0U;
        y = 10U;
        while (index < bindings.metrics.group_a_count) {
            const u32 actor = group_a_token(index);
            const u32 command =
                runner
                    .call(
                        LegacyBattleDebugOverlayCall::query_actor_command, actor
                    )
                    .eax &
                0xFFFFU;
            const u32 lock =
                runner
                    .call(
                        LegacyBattleDebugOverlayCall::query_actor_lock,
                        actor,
                        {command},
                        1U
                    )
                    .eax &
                0xFFFFU;
            format_text(
                state,
                result,
                kGroupAFormat,
                static_cast<int>(lock),
                static_cast<int>(command)
            );
            runner.draw_buffer(520U, y);
            ++result.group_a_rows;
            ++index;
            y += 20U;
        }

        const u32 attack_y = bindings.metrics.group_b_count * 20U + 10U;
        runner.draw(10U, attack_y, kAttackOrderText);

        index = 0U;
        u32 x = 100U;
        while (index < bindings.metrics.group_b_count +
                   bindings.metrics.group_a_count) {
            if (index >= bindings.startup.reset.records_524788.size()) {
                result.status =
                    LegacyBattleDebugOverlayStatus::startup_record_typed_stop;
                return result;
            }
            format_text(
                state,
                result,
                "%d",
                signed_bits(
                    bindings.startup.reset.records_524788[index].value_00
                )
            );
            runner.draw_buffer(x, bindings.metrics.group_b_count * 20U + 10U);
            ++result.startup_order_rows;
            ++index;
            x += 20U;
        }

        runner.draw(
            10U, bindings.metrics.group_b_count * 20U + 30U, kWaitingOrderText
        );
        index = 0U;
        x = 120U;
        while (index < bindings.metrics.group_a_count) {
            if (index >= bindings.final_actor.actor_order.size()) {
                result.status =
                    LegacyBattleDebugOverlayStatus::actor_order_typed_stop;
                return result;
            }
            format_text(
                state,
                result,
                "%d",
                signed_bits(bindings.final_actor.actor_order[index])
            );
            runner.draw_buffer(x, bindings.metrics.group_b_count * 20U + 30U);
            ++result.actor_order_rows;
            ++index;
            x += 20U;
        }

        runner.draw(
            10U, bindings.metrics.group_b_count * 20U + 50U, kSelectionOrderText
        );
        index = 0U;
        x = 100U;
        while (index < bindings.metrics.group_b_count +
                   bindings.metrics.group_a_count) {
            if (index >= state.selection_order.size()) {
                result.status =
                    LegacyBattleDebugOverlayStatus::selection_order_typed_stop;
                return result;
            }
            format_text(
                state, result, "%d", signed_bits(state.selection_order[index])
            );
            runner.draw_buffer(x, bindings.metrics.group_b_count * 20U + 50U);
            ++result.selection_order_rows;
            ++index;
            x += 20U;
        }

        format_text(
            state,
            result,
            kCurrentSelectionFormat,
            signed_bits(bindings.metrics.priority_actor_index),
            signed_bits(bindings.final_actor.active_actor_code)
        );
        runner.draw_buffer(240U, 70U);

        index = 0U;
        while (index < bindings.metrics.group_b_count) {
            const u32 actor = group_b_token(index);
            const auto position = runner.call(
                LegacyBattleDebugOverlayCall::query_marker_position, actor
            );
            if ((position.output_mask & 1U) != 0U) {
                state.marker_x = static_cast<i16>(position.output_0);
            }
            if ((position.output_mask & 2U) != 0U) {
                state.marker_row = static_cast<i16>(position.output_1);
            }
            const auto& geometry = bindings.framebuffer.geometry();
            const u32 raster_right_bits = signed_bits(geometry.clip_left) +
                signed_bits(geometry.clip_width);
            const i32 raster_right = std::bit_cast<i32>(raster_right_bits);
            const u32 row_offset = wrapping_mul(
                signed_bits(static_cast<i32>(state.marker_row)),
                raster_right_bits
            );
            const u32 width =
                runner
                    .call(
                        LegacyBattleDebugOverlayCall::query_marker_width, actor
                    )
                    .eax &
                0xFFFFU;
            u32 column = 0U;
            while (column < width) {
                const u32 top = row_offset +
                    signed_bits(static_cast<i32>(state.marker_x)) + column;
                if (!write_marker_pixel(bindings, result, top)) {
                    return result;
                }
                const u32 bottom = top + signed_bits(raster_right);
                if (!write_marker_pixel(bindings, result, bottom)) {
                    return result;
                }
                ++column;
            }
            ++result.marker_actors;
            ++index;
        }

        format_text(
            state,
            result,
            kBattleSummaryFormat,
            signed_bits(bindings.hotkeys.toggle_53af68),
            static_cast<int>(state.battle_selector),
            signed_bits(state.battle_mode)
        );
        runner.draw_buffer(240U, 30U);
        format_text(
            state,
            result,
            "fMenu:%d mMove%d",
            signed_bits(bindings.message_state),
            signed_bits(bindings.final_actor.pre_frame_gate_b)
        );
        runner.draw_buffer(240U, 50U);
        format_text(
            state,
            result,
            "MsD:%d dRole1:%d CanS:%d",
            static_cast<int>(static_cast<compat::u8>(state.message_status)),
            static_cast<int>(bindings.effects.group_a_feedback_actor),
            static_cast<int>(static_cast<u16>(state.selection_status))
        );
        runner.draw_buffer(240U, 90U);
        format_text(
            state,
            result,
            "MS:%d Stop:%d mStop%d",
            signed_bits(bindings.final_actor.published_actor_code),
            signed_bits(bindings.final_actor.frame_gate_a),
            signed_bits(bindings.final_actor.frame_gate_b)
        );
        runner.draw_buffer(240U, 110U);
        format_text(
            state, result, "A.Lockc:%d ", signed_bits(state.lock_count)
        );
        runner.draw_buffer(420U, 30U);
        format_text(
            state,
            result,
            "TswMem:%dK",
            signed_bits(state.tsw_cache_bytes) / 1000
        );
        runner.draw_buffer(420U, 50U);
        format_text(
            state,
            result,
            "wLl:%d iMn:%d",
            static_cast<int>(state.world_level),
            static_cast<int>(state.initial_mode)
        );
        runner.draw_buffer(420U, 70U);
        format_text(state, result, "bf:%d", signed_bits(state.battle_frame));
        runner.draw_buffer(450U, 90U);

        if (state.frame_divisor == 0) {
            result.status = LegacyBattleDebugOverlayStatus::frame_divisor_zero;
            return result;
        }
        format_text(state, result, "FRAME:%d", 1000 / state.frame_divisor);
        runner.draw_buffer(240U, 10U);
    }

    static_cast<void>(runner.call(
        LegacyBattleDebugOverlayCall::font_reset, kFontToken, {0U}, 1U
    ));
    const auto tail = runner.call(
        LegacyBattleDebugOverlayCall::font_style, kFontToken, {0x0000FFFEU}, 1U
    );
    result.return_value = tail.eax;
    result.return_ecx = tail.ecx;
    result.return_edx = tail.edx;
    return result;
}

}  // namespace openswd3::battle
