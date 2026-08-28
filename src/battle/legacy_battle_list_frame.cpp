#include "openswd3/battle/legacy_battle_list_frame.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

inline constexpr u32 kActionId = 0x2394U;
inline constexpr u32 kPanelActionId = 0x233BU;
inline constexpr u32 kFontToken = 0x004C9A28U;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

}  // namespace

LegacyBattleListFrameResult draw_legacy_battle_list_frame(
    const LegacyBattleListFrameBindings bindings,
    LegacyBattleListFramePort& port,
    const LegacyBattleListFrameRequest& request
) {
    LegacyBattleListFrameResult result;
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const u32 row_y = request.origin_y + 0x20U;
    u32 row_x = request.origin_x + 0x54U;
    auto& action_state = port.battle_offset_action_frame_draw_state();
    for (u32 index = 0U; index < 3U; ++index) {
        const u32 variant = 2U - index;
        result.action_frames[index] = draw_legacy_battle_offset_action_frame(
            action_state,
            bindings.framebuffer,
            bindings.clip,
            bindings.shared_request,
            bindings.shared_effects,
            bindings.jitter,
            bindings.action_updater,
            bindings.frame_provider,
            kActionId,
            variant,
            wrapping_i32(row_x),
            wrapping_i32(row_y),
            0U,
            request.action_update_edx_snapshots[index]
        );
        ++result.action_frame_calls;
        eax = request.action_frame_return_registers[index].eax;
        ecx = request.action_frame_return_registers[index].ecx;
        edx = request.action_frame_return_registers[index].edx;
        if (result.action_frames[index].status !=
            LegacyBattleOffsetActionFrameDrawStatus::completed) {
            result.status =
                LegacyBattleListFrameStatus::action_frame_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        row_x -= 0x2AU;
    }

    const u32 category = bindings.input.action_category_index;
    const u32 selected_x = request.origin_x + category * 0x2AU;
    result.action_frames[3U] = draw_legacy_battle_offset_action_frame(
        action_state,
        bindings.framebuffer,
        bindings.clip,
        bindings.shared_request,
        bindings.shared_effects,
        bindings.jitter,
        bindings.action_updater,
        bindings.frame_provider,
        kActionId,
        category + 4U,
        wrapping_i32(selected_x),
        wrapping_i32(row_y),
        0U,
        request.action_update_edx_snapshots[3U]
    );
    ++result.action_frame_calls;
    eax = request.action_frame_return_registers[3U].eax;
    ecx = request.action_frame_return_registers[3U].ecx;
    edx = request.action_frame_return_registers[3U].edx;
    if (result.action_frames[3U].status !=
        LegacyBattleOffsetActionFrameDrawStatus::completed) {
        result.status = LegacyBattleListFrameStatus::action_frame_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    auto font_reply = port.invoke_list_frame({
        .call = LegacyBattleListFrameCall::configure_font_style,
        .arguments = {0xF000U},
        .eax = eax,
        .ecx = kFontToken,
        .edx = edx,
    });
    ++result.port_calls;
    ++result.font_style_calls;
    eax = font_reply.eax;
    ecx = font_reply.ecx;
    edx = font_reply.edx;
    font_reply = port.invoke_list_frame({
        .call = LegacyBattleListFrameCall::configure_font_style,
        .arguments = {0xFFFEU},
        .eax = eax,
        .ecx = kFontToken,
        .edx = edx,
    });
    ++result.port_calls;
    ++result.font_style_calls;
    eax = font_reply.eax;
    ecx = font_reply.ecx;
    edx = font_reply.edx;

    static_cast<void>(
        clear_legacy_battle_action_record(bindings.panel_action_record)
    );
    bindings.input.selection_animation_frame_a = 10U;
    bindings.panel_action_record.action_id = kPanelActionId;
    bindings.panel_action_record.base_variant = 0U;
    result.panel_action_update =
        bindings.action_updater.update(bindings.panel_action_record);
    ++result.panel_action_update_calls;

    result.rectangle_status = rendering::apply_legacy_rectangle_effect(
        bindings.framebuffer,
        bindings.raster,
        bindings.shared_effects.pixel_conversion,
        {
            .x = wrapping_i32(request.origin_x),
            .y = wrapping_i32(request.origin_y + 0x24U),
            .width = 0xBE,
            .height = 0x98,
            .red = 0,
            .green = 4,
            .blue = 4,
            .mode = 2U,
        }
    );
    ++result.rectangle_calls;
    eax = request.rectangle_return_registers.eax;
    ecx = request.rectangle_return_registers.ecx;
    edx = request.rectangle_return_registers.edx;
    if (!rectangle_completed(result.rectangle_status)) {
        result.status = LegacyBattleListFrameStatus::rectangle_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    ecx = (ecx & 0xFFFF0000U) |
        static_cast<u32>(bindings.panel_action_record.field_4a);
    result.tiled_frame = rendering::draw_legacy_tiled_frame(
        bindings.framebuffer,
        bindings.raster,
        bindings.frame_provider,
        {
            .resource_id = ecx,
            .left = wrapping_i32(request.origin_x + 6U),
            .top = wrapping_i32(request.origin_y + 0x28U),
            .right = wrapping_i32(request.origin_x + 0xBAU),
            .bottom = wrapping_i32(request.origin_y + 0xB8U),
            .opacity_step = 0,
            .flags = 0x80000008U,
        },
        bindings.shared_effects,
        bindings.jitter
    );
    ++result.tiled_frame_calls;
    eax = request.tiled_frame_return_registers.eax;
    ecx = request.tiled_frame_return_registers.ecx;
    edx = request.tiled_frame_return_registers.edx;
    if (result.tiled_frame.status !=
        rendering::LegacyTiledFrameStatus::completed) {
        result.status = LegacyBattleListFrameStatus::tiled_frame_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    eax = bindings.input.selection_animation_frame_b + 2U;
    bindings.input.selection_animation_frame_b = eax;
    if (signed_bits(eax) > 7) {
        bindings.input.selection_animation_frame_b = 7U;
    }
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
