#include "openswd3/battle/legacy_battle_debug_status_panel.hpp"

#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u32;

constexpr u32 kCallUpdateAction = 0x004321E0U;
constexpr u32 kCallDrawRectangle = 0x0043B110U;
constexpr u32 kCallDrawTiledFrame = 0x0042E850U;
constexpr u32 kCallAdvanceStage = 0x00469620U;
constexpr u32 kCallPackColor = 0x004239D0U;
constexpr u32 kCallCopyText = 0x00478A80U;
constexpr u32 kCallFormatText = 0x00489610U;
constexpr u32 kCallDrawText = 0x00436AD0U;
constexpr u32 kCallTruncateWidth = 0x00489654U;
constexpr u32 kCallDrawFade = 0x00450A50U;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 add_bits(const u32 left, const u32 right) noexcept {
    return signed_bits(left + right);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] constexpr bool
fade_completed(const rendering::LegacyBlitExecutionStatus status) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const compat::u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

[[nodiscard]] bool
format_value(std::array<u8, 8>& buffer, const i32 value) noexcept {
    if (value == 0) {
        buffer[0] = 0x2DU;
        buffer[1] = 0x2DU;
        buffer[2] = 0U;
        return true;
    }

    const u32 magnitude =
        value < 0 ? 0U - std::bit_cast<u32>(value) : static_cast<u32>(value);
    std::array<char, 5> digits{};
    const auto conversion =
        std::to_chars(digits.data(), digits.data() + digits.size(), magnitude);
    if (conversion.ec != std::errc{}) {
        return false;
    }
    const u32 digit_count = static_cast<u32>(conversion.ptr - digits.data());
    u32 cursor = 0U;
    if (digit_count < 2U) {
        buffer[cursor++] = 0x20U;
    }
    for (u32 index = 0U; index < digit_count; ++index) {
        if (cursor >= buffer.size()) {
            return false;
        }
        buffer[cursor++] = static_cast<u8>(digits[index]);
    }
    if (cursor + 3U > buffer.size()) {
        return false;
    }
    buffer[cursor++] = 0x30U;
    buffer[cursor++] = 0x25U;
    buffer[cursor] = 0U;
    return true;
}

[[nodiscard]] u32 truncate_x87_width(const float value) noexcept {
    return static_cast<u32>(
        static_cast<std::int64_t>(std::trunc(static_cast<double>(value)))
    );
}

}  // namespace

LegacyBattleDebugStatusPanelResult draw_legacy_battle_debug_status_panel(
    LegacyBattleDebugStatusPanelBindings bindings,
    LegacyBattleTextPanelPort& port,
    const LegacyBattleDebugStatusPanelRequest& request
) {
    LegacyBattleDebugStatusPanelResult result{};
    u32 eax = request.entry.eax;
    u32 ecx = request.entry.ecx;
    u32 edx = request.entry.edx;
    const auto finish = [&]() {
        result.return_registers = {eax, ecx, edx};
        return result;
    };
    const auto invoke_text = [&](const std::array<u32, 8>& arguments,
                                 const u32 entry_eax,
                                 const u32 entry_ecx,
                                 const u32 entry_edx) {
        result.call_trace.push_back(kCallDrawText);
        const auto reply = port.invoke_text_panel({
            .call = LegacyBattleTextPanelCall::draw_text,
            .arguments = arguments,
            .eax = entry_eax,
            .ecx = entry_ecx,
            .edx = entry_edx,
        });
        ++result.text_calls;
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    result.local_text = {request.local_text_seed, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    if ((bindings.debug_hotkeys.battle_mode_flags_53bc24 & 0x20U) == 0U) {
        return finish();
    }
    result.debug_gate_open = true;

    auto& action = bindings.victory_rewards.panel_action_record;
    action.action_id = kLegacyBattleDebugStatusPanelAction;
    action.base_variant = 0U;
    result.call_trace.push_back(kCallUpdateAction);
    result.panel_action_update = bindings.action_updater.update(action);
    ++result.panel_action_update_calls;

    result.panel_rectangle_height =
        add_bits(bindings.target_selection.transition_stage, 40U);
    result.call_trace.push_back(kCallDrawRectangle);
    result.panel_rectangle_status = rendering::apply_legacy_rectangle_effect(
        bindings.framebuffer,
        bindings.raster,
        bindings.pixel_conversion,
        {
            .x = 236,
            .y = 86,
            .width = 184,
            .height = result.panel_rectangle_height,
            .red = 4,
            .green = 4,
            .blue = 0,
            .mode = 0U,
        }
    );
    ++result.panel_rectangle_calls;
    eax = request.rectangle_return.eax;
    ecx = request.rectangle_return.ecx;
    edx = request.rectangle_return.edx;
    if (!rectangle_completed(result.panel_rectangle_status)) {
        result.status =
            LegacyBattleDebugStatusPanelStatus::panel_rectangle_typed_stop;
        return finish();
    }

    result.panel_frame_resources[0] = replace_low_word(eax, action.field_4a);
    result.panel_frame_bottoms[0] = 106;
    result.call_trace.push_back(kCallDrawTiledFrame);
    result.panel_frames[0] = rendering::draw_legacy_tiled_frame(
        bindings.framebuffer,
        bindings.raster,
        bindings.frame_provider,
        {
            .resource_id = result.panel_frame_resources[0],
            .left = 240,
            .top = 90,
            .right = 416,
            .bottom = result.panel_frame_bottoms[0],
            .opacity_step = 0,
            .flags = 0x80000008U,
        },
        bindings.shared_effects,
        bindings.jitter
    );
    ++result.panel_frame_calls;
    eax = request.panel_frame_return.eax;
    ecx = request.panel_frame_return.ecx;
    edx = request.panel_frame_return.edx;
    if (result.panel_frames[0].status !=
        rendering::LegacyTiledFrameStatus::completed) {
        result.status =
            LegacyBattleDebugStatusPanelStatus::panel_frame_typed_stop;
        return finish();
    }

    if (bindings.target_selection.debug_status_profile_token == 0U) {
        result.status = LegacyBattleDebugStatusPanelStatus::profile_typed_stop;
        return finish();
    }
    invoke_text(
        {
            kLegacyBattleDebugStatusPanelFontToken,
            kLegacyBattleDebugStatusPanelSurfaceToken,
            304U,
            90U,
            bindings.target_selection.debug_status_profile_token,
            0xFFC0U,
            16U,
            0U,
        },
        eax,
        kLegacyBattleDebugStatusPanelFontToken,
        kLegacyBattleDebugStatusPanelSurfaceToken
    );

    result.panel_frame_resources[1] = replace_low_word(ecx, action.field_4a);
    result.panel_frame_bottoms[1] =
        add_bits(bindings.target_selection.transition_stage, 122U);
    result.call_trace.push_back(kCallDrawTiledFrame);
    result.panel_frames[1] = rendering::draw_legacy_tiled_frame(
        bindings.framebuffer,
        bindings.raster,
        bindings.frame_provider,
        {
            .resource_id = result.panel_frame_resources[1],
            .left = 240,
            .top = 122,
            .right = 416,
            .bottom = result.panel_frame_bottoms[1],
            .opacity_step = 0,
            .flags = 0x80000008U,
        },
        bindings.shared_effects,
        bindings.jitter
    );
    ++result.panel_frame_calls;
    if (result.panel_frames[1].status !=
        rendering::LegacyTiledFrameStatus::completed) {
        result.status =
            LegacyBattleDebugStatusPanelStatus::title_frame_typed_stop;
        return finish();
    }

    result.call_trace.push_back(kCallAdvanceStage);
    result.transition_stage_advance = advance_legacy_battle_transition_stage(
        bindings.target_selection.transition_stage,
        {.base_offset = 122U, .target = 302U, .divisor = 3U}
    );
    ++result.transition_stage_advance_calls;
    eax = result.transition_stage_advance.return_eax;
    ecx = result.transition_stage_advance.return_ecx;
    edx = result.transition_stage_advance.return_edx;
    if (result.transition_stage_advance.status !=
        LegacyBattleTransitionStageAdvanceStatus::completed) {
        result.status =
            LegacyBattleDebugStatusPanelStatus::transition_stage_typed_stop;
        return finish();
    }
    if (eax != 1U) {
        return finish();
    }
    result.stage_gate_open = true;

    for (u32 index = 0U; index < result.rows.size(); ++index) {
        auto& row = result.rows[index];
        row.label_token = kLegacyBattleDebugStatusPanelLabelTokens[index];
        row.value = bindings.target_selection.debug_status_values[index];
        row.text_y = 122 + static_cast<i32>(index * 20U);
        row.fade_y = 130 + static_cast<i32>(index * 20U);
        const i32 value = static_cast<i32>(row.value);
        float width = 2.0F;

        const auto pack_color =
            [&](const i32 red, const i32 green, const i32 blue) {
                result.call_trace.push_back(kCallPackColor);
                row.packed_color = rendering::legacy_pack_color_pair(
                    bindings.pixel_conversion, red, green, blue
                );
            };
        if (value == 0) {
            pack_color(16, 16, 16);
            result.call_trace.push_back(kCallCopyText);
            if (!format_value(result.local_text, value)) {
                result.status =
                    LegacyBattleDebugStatusPanelStatus::text_format_typed_stop;
                return finish();
            }
        } else if (value > 0) {
            pack_color(28, 2, 2);
            result.call_trace.push_back(kCallFormatText);
            if (!format_value(result.local_text, value)) {
                result.status =
                    LegacyBattleDebugStatusPanelStatus::text_format_typed_stop;
                return finish();
            }
            width = static_cast<float>(value) * 0.1F * 120.0F;
        } else {
            pack_color(2, 13, 28);
            result.call_trace.push_back(kCallFormatText);
            if (!format_value(result.local_text, value)) {
                result.status =
                    LegacyBattleDebugStatusPanelStatus::text_format_typed_stop;
                return finish();
            }
            width = static_cast<float>(value) * -0.1F * 120.0F;
            if (value < -10) {
                pack_color(2, 28, 13);
                result.call_trace.push_back(kCallFormatText);
                if (!format_value(result.local_text, value)) {
                    result.status = LegacyBattleDebugStatusPanelStatus::
                        text_format_typed_stop;
                    return finish();
                }
                width = static_cast<float>(value + 10) * -0.1F * 120.0F;
            }
        }
        row.formatted_text = result.local_text;

        result.call_trace.push_back(kCallPackColor);
        const u32 text_color = rendering::legacy_pack_color_pair(
            bindings.pixel_conversion, 31, 31, 31
        );
        invoke_text(
            {
                kLegacyBattleDebugStatusPanelFontToken,
                kLegacyBattleDebugStatusPanelSurfaceToken,
                252U,
                bits(row.text_y),
                row.label_token,
                text_color,
                16U,
                0U,
            },
            row.label_token,
            kLegacyBattleDebugStatusPanelFontToken,
            kLegacyBattleDebugStatusPanelSurfaceToken
        );

        result.call_trace.push_back(kCallTruncateWidth);
        row.fade_width = truncate_x87_width(width);
        eax = row.fade_width;
        edx = 0U;

        result.call_trace.push_back(kCallDrawFade);
        const auto fade = fade_legacy_battle_rectangle(
            bindings.selection_frame.selection_hint_frame.color_fade,
            bindings.framebuffer,
            bindings.clip,
            bindings.shared_request,
            bindings.shared_effects,
            bindings.jitter,
            284,
            row.fade_y,
            signed_bits(row.fade_width),
            3,
            row.packed_color
        );
        ++result.color_fade_calls;
        eax = request.color_fade_return.eax;
        ecx = request.color_fade_return.ecx;
        edx = request.color_fade_return.edx;
        if (!fade_completed(fade.status)) {
            result.status =
                LegacyBattleDebugStatusPanelStatus::color_fade_typed_stop;
            return finish();
        }

        edx = kLegacyBattleDebugStatusPanelLabelTableToken + (index + 1U) * 4U;
        eax = 8U - index;
    }
    return finish();
}

}  // namespace openswd3::battle
