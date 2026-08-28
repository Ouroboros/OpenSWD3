#include "openswd3/battle/legacy_battle_text_message_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 sar_one(const u32 value) noexcept {
    return (value >> 1U) | (value & 0x80000000U);
}

[[nodiscard]] constexpr u32 text_half_width(const u32 length) noexcept {
    const u32 half = sar_one(length);
    u32 value = 8U - half;
    const u32 five = value + value * 4U;
    value = value + five * 2U;
    value <<= 1U;
    return sar_one(value);
}

[[nodiscard]] constexpr u32 centered_text_x(const u32 length) noexcept {
    const u32 half = sar_one(length);
    u32 width = half + half * 4U;
    width <<= 2U;
    width = sar_one(width);
    return 315U - width;
}

[[nodiscard]] constexpr u32
toward_zero_half_after_add_32(const u32 value) noexcept {
    const u32 adjusted = value + 32U;
    const u32 sign = signed_bits(adjusted) < 0 ? 0xFFFFFFFFU : 0U;
    return sar_one(adjusted - sign);
}

[[nodiscard]] LegacyBattleTextMessageAllocation*
find_allocation(LegacyBattleTextMessageState& state, const u32 token) noexcept {
    const auto found = std::ranges::find(
        state.allocations, token, &LegacyBattleTextMessageAllocation::token
    );
    return found == state.allocations.end() ? nullptr : &*found;
}

[[nodiscard]] bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out;
}

[[nodiscard]] bool
fade_completed(const rendering::LegacyBlitExecutionStatus status) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

}  // namespace

LegacyBattleTextMessageFrameResult advance_legacy_battle_text_message_frame(
    LegacyBattleTextMessageFrameBindings bindings,
    LegacyBattleTextMessageFramePort& port,
    const LegacyBattleTextMessageFrameRequest& request
) {
    LegacyBattleTextMessageFrameResult result{};
    u32 eax = request.entry.eax;
    u32 ecx = request.entry.ecx;
    u32 edx = request.entry.edx;
    const auto finish = [&]() {
        result.return_registers = {.eax = eax, .ecx = ecx, .edx = edx};
        return result;
    };
    const auto stop_on_unknown = [&](const u32 token) {
        result.status = LegacyBattleTextMessageFrameStatus::chain_typed_stop;
        result.stopped_chain_token = token;
        eax = token;
        return finish();
    };
    const auto draw_text = [&](const u32 x, const u32 y, const u32 text_token) {
        const auto reply = port.invoke_text_message_frame({
            .call = LegacyBattleTextMessageFrameCall::draw_text,
            .arguments =
                {
                    kLegacyBattleTextMessageFrameFontToken,
                    kLegacyBattleTextMessageFrameSurfaceToken,
                    x,
                    y,
                    text_token,
                    0xFFFFU,
                    0x10U,
                    0U,
                },
            .eax = eax,
            .ecx = kLegacyBattleTextMessageFrameFontToken,
            .edx = edx,
        });
        ++result.text_calls;
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };
    std::size_t color_fade_return_index = 0U;
    const auto draw_fade = [&](const u32 x, const u32 y) {
        const auto fade = fade_legacy_battle_rectangle(
            bindings.color_fade,
            bindings.framebuffer,
            bindings.clip,
            bindings.shared_request,
            bindings.shared_effects,
            bindings.jitter,
            wrapping_i32(x),
            wrapping_i32(y),
            176,
            32,
            0xFFU
        );
        ++result.color_fade_calls;
        if (color_fade_return_index < request.color_fade_returns.size()) {
            const auto registers =
                request.color_fade_returns[color_fade_return_index];
            eax = registers.eax;
            ecx = registers.ecx;
            edx = registers.edx;
        }
        ++color_fade_return_index;
        if (!fade_completed(fade.status)) {
            result.status =
                LegacyBattleTextMessageFrameStatus::color_fade_typed_stop;
            return false;
        }
        return true;
    };

    u32 token = bindings.head_token;
    while (token != 0U) {
        LegacyBattleTextMessageAllocation* node =
            find_allocation(bindings.messages, token);
        if (node == nullptr) {
            return stop_on_unknown(token);
        }
        ++result.first_pass_nodes;
        auto& record = node->record;
        u32 y = record.value_08 + 6U;
        u32 x = record.value_04 + 16U;
        const u32 entry_flags = record.flags;

        if ((entry_flags & 0x80000000U) != 0U) {
            bindings.panel_action_record.action_id =
                kLegacyBattleTextMessageFramePanelAction;
            bindings.panel_action_record.base_variant = 0U;
            const auto action =
                bindings.action_updater.update(bindings.panel_action_record);
            ++result.panel_action_update_calls;
            eax = action.return_value;

            const auto rectangle = rendering::apply_legacy_rectangle_effect(
                bindings.framebuffer,
                bindings.raster,
                bindings.pixel_conversion,
                {
                    .x = 10,
                    .y = 10,
                    .width = 620,
                    .height = 32,
                    .red = -8,
                    .green = -8,
                    .blue = -8,
                    .mode = 1U,
                }
            );
            ++result.rectangle_calls;
            eax = request.rectangle_return.eax;
            ecx = request.rectangle_return.ecx;
            edx = request.rectangle_return.edx;
            if (!rectangle_completed(rectangle)) {
                result.status =
                    LegacyBattleTextMessageFrameStatus::rectangle_typed_stop;
                return finish();
            }
            eax = (eax & 0xFFFF0000U) |
                static_cast<u32>(bindings.panel_action_record.field_4a);
            const auto frame = rendering::draw_legacy_tiled_frame(
                bindings.framebuffer,
                bindings.raster,
                bindings.frame_provider,
                {
                    .resource_id = eax,
                    .left = 14,
                    .top = 14,
                    .right = 626,
                    .bottom = 38,
                    .opacity_step = 0,
                    .flags = 0x80000008U,
                },
                bindings.shared_effects,
                bindings.jitter
            );
            ++result.tiled_frame_calls;
            eax = request.tiled_frame_return.eax;
            ecx = request.tiled_frame_return.ecx;
            edx = request.tiled_frame_return.edx;
            if (frame.status != rendering::LegacyTiledFrameStatus::completed) {
                result.status =
                    LegacyBattleTextMessageFrameStatus::tiled_frame_typed_stop;
                return finish();
            }
        }
        if ((record.flags & 0x40000000U) != 0U) {
            const auto fade = fade_legacy_battle_rectangle(
                bindings.color_fade,
                bindings.framebuffer,
                bindings.clip,
                bindings.shared_request,
                bindings.shared_effects,
                bindings.jitter,
                10,
                10,
                620,
                32,
                0xFFU
            );
            ++result.color_fade_calls;
            if (color_fade_return_index < request.color_fade_returns.size()) {
                const auto registers =
                    request.color_fade_returns[color_fade_return_index];
                eax = registers.eax;
                ecx = registers.ecx;
                edx = registers.edx;
            }
            ++color_fade_return_index;
            if (!fade_completed(fade.status)) {
                result.status =
                    LegacyBattleTextMessageFrameStatus::color_fade_typed_stop;
                return finish();
            }
        }
        const u32 position_flags = record.flags;
        if ((position_flags & 0x01U) != 0U) {
            x = 18U;
        }
        if ((position_flags & 0x02U) != 0U) {
            x = centered_text_x(record.text_length);
        }

        if (signed_word(record.kind) > 0) {
            if ((position_flags & 0x10U) != 0U) {
                const u32 width = text_half_width(record.text_length);
                const u32 target = width + 227U;
                y = 16U;
                if (record.value_10 == 0U) {
                    record.value_10 = 640U;
                }
                if (bindings.freeze_gate == 0U) {
                    record.value_10 = sar_one(record.value_10 + target);
                }
                x = record.value_10;
                if (!draw_fade(x - width, 10U)) {
                    return finish();
                }
            }
            if ((record.flags & 0x20U) != 0U) {
                const u32 width = text_half_width(record.text_length);
                y = 16U;
                if (bindings.freeze_gate == 0U) {
                    record.value_10 = sar_one(record.value_10 + width + 227U);
                }
                x = record.value_10;
                if (!draw_fade(x - width, 10U)) {
                    return finish();
                }
            }
            if ((record.flags & 0x40U) != 0U) {
                const u32 width = text_half_width(record.text_length);
                record.value_14 =
                    toward_zero_half_after_add_32(record.value_14);
                record.value_10 = width + 227U;
                x = record.value_10;
                y = record.value_14;
                if (!draw_fade(x - width, y - 6U)) {
                    return finish();
                }
            }
            draw_text(x, y, record.text_token);
            record.kind = static_cast<u16>(record.kind - 1U);
        }
        token = record.next_token;
    }

    u32 previous_token = kLegacyBattleTextMessageHeadToken;
    token = bindings.head_token;
    while (token != 0U) {
        LegacyBattleTextMessageAllocation* node =
            find_allocation(bindings.messages, token);
        if (node == nullptr) {
            return stop_on_unknown(token);
        }
        ++result.second_pass_nodes;
        auto& record = node->record;
        const u32 next_token = record.next_token;
        bool retain = signed_word(record.kind) > 0;

        if (!retain && (record.flags & 0x20U) != 0U) {
            const u32 width = text_half_width(record.text_length);
            if (bindings.freeze_gate == 0U) {
                record.value_10 += 80U;
            }
            if (!draw_fade(record.value_10 - width, 10U)) {
                return finish();
            }
            draw_text(record.value_10, 16U, record.text_token);
            retain = signed_bits(record.value_10) < 640;
        }
        if (!retain && (record.flags & 0x10U) != 0U) {
            const u32 width = text_half_width(record.text_length);
            if (bindings.freeze_gate == 0U) {
                record.value_10 -= 80U;
            }
            if (!draw_fade(record.value_10 - width, 10U)) {
                return finish();
            }
            draw_text(record.value_10, 16U, record.text_token);
            retain = signed_bits(record.value_10) > 0;
        }
        if (!retain && (record.flags & 0x40U) != 0U) {
            const u32 width = text_half_width(record.text_length);
            if (bindings.freeze_gate == 0U) {
                record.value_14 -= record.value_08;
            }
            record.value_08 <<= 1U;
            if (!draw_fade(record.value_10 - width, record.value_14 - 6U)) {
                return finish();
            }
            draw_text(record.value_10, record.value_14, record.text_token);
            retain = signed_bits(record.value_14) > -32;
        }

        if (retain) {
            result.retained_tokens.push_back(token);
            previous_token = token;
            token = next_token;
            continue;
        }

        if (previous_token == kLegacyBattleTextMessageHeadToken) {
            bindings.head_token = next_token;
        } else {
            LegacyBattleTextMessageAllocation* previous =
                find_allocation(bindings.messages, previous_token);
            if (previous == nullptr) {
                return stop_on_unknown(previous_token);
            }
            previous->record.next_token = next_token;
        }
        const auto reply = port.invoke_text_message_frame({
            .call = LegacyBattleTextMessageFrameCall::release_node,
            .arguments = {token},
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result.release_calls;
        ++result.port_calls;
        result.released_tokens.push_back(token);
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        std::erase_if(bindings.messages.allocations, [token](const auto& item) {
            return item.token == token;
        });
        token = next_token;
    }

    return finish();
}

}  // namespace openswd3::battle
