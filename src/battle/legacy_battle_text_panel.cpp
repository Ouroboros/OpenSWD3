#include "openswd3/battle/legacy_battle_text_panel.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr u32 bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 add(const i32 left, const i32 right) noexcept {
    return signed_bits(bits(left) + bits(right));
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const compat::u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

}  // namespace

LegacyBattleTextPanelResult draw_legacy_battle_text_panel(
    LegacyBattleVictoryRewardState& shared_state,
    LegacyBattleTextPanelPort& port,
    const LegacyBattleTextPanelRequest& request
) {
    LegacyBattleTextPanelResult result{};
    u32 eax = request.entry.eax;
    u32 ecx = request.entry.ecx;
    u32 edx = request.entry.edx;

    auto invoke = [&](const LegacyBattleTextPanelCall call,
                      const std::array<u32, 8U>& arguments = {}) {
        result.call_trace.push_back(call);
        ++result.port_calls;
        const auto reply = port.invoke_text_panel({
            .call = call,
            .arguments = arguments,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (reply.publish_action_field_4a) {
            shared_state.panel_action_record.field_4a = reply.action_field_4a;
        }
    };

    shared_state.panel_action_record.action_id = kLegacyBattleTextPanelAction;
    shared_state.panel_action_record.base_variant = 0U;
    result.action_entry = {eax, ecx, edx};
    invoke(
        LegacyBattleTextPanelCall::update_action,
        {kLegacyBattleTextPanelActionRecordToken}
    );
    ++result.action_calls;

    eax = bits(request.width);
    result.rectangle_entry = {eax, ecx, edx};
    invoke(
        LegacyBattleTextPanelCall::draw_rectangle,
        {bits(request.left),
         bits(request.top),
         bits(request.width),
         bits(request.height),
         0U,
         4U,
         4U,
         0U}
    );
    ++result.rectangle_calls;

    result.frame_left = add(request.left, 4);
    result.frame_top = add(request.top, 4);
    result.frame_right = add(add(request.left, request.width), -4);
    result.frame_bottom = add(add(request.top, request.height), -4);
    edx = replace_low_word(
        bits(request.width), shared_state.panel_action_record.field_4a
    );
    result.frame_resource = edx;
    eax = bits(result.frame_right);
    ecx = bits(result.frame_left);
    result.tiled_frame_entry = {eax, ecx, edx};
    invoke(
        LegacyBattleTextPanelCall::draw_tiled_frame,
        {edx,
         bits(result.frame_left),
         bits(result.frame_top),
         bits(result.frame_right),
         bits(result.frame_bottom),
         0U,
         0x80000008U,
         0U}
    );
    ++result.tiled_frame_calls;

    eax = bits(request.text_x);
    ecx = bits(request.text_y);
    if (request.text_x == 0 && request.text_y == 0) {
        result.used_default_text_position = true;
        result.resolved_text_x = add(request.left, 2);
        result.resolved_text_y = add(request.top, 4);
        eax = request.text_token;
        ecx = kLegacyBattleTextPanelFontToken;
    } else {
        result.resolved_text_x = request.text_x;
        result.resolved_text_y = request.text_y;
        edx = request.text_token;
        eax = kLegacyBattleTextPanelSurfaceToken;
        ecx = kLegacyBattleTextPanelFontToken;
    }
    result.text_entry = {eax, ecx, edx};
    invoke(
        LegacyBattleTextPanelCall::draw_text,
        {kLegacyBattleTextPanelFontToken,
         kLegacyBattleTextPanelSurfaceToken,
         bits(result.resolved_text_x),
         bits(result.resolved_text_y),
         request.text_token,
         0xFFC0U,
         16U,
         0U}
    );
    ++result.text_calls;

    result.return_registers = {eax, ecx, edx};
    return result;
}

}  // namespace openswd3::battle
