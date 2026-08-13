#include "openswd3/rendering/legacy_effect_panel.hpp"

#include <bit>

namespace openswd3::rendering {
namespace {

using compat::i32;
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

[[nodiscard]] constexpr bool
rectangle_effect_completed(const LegacyRectangleEffectStatus status) noexcept {
    return status == LegacyRectangleEffectStatus::completed ||
        status == LegacyRectangleEffectStatus::clipped_out ||
        status == LegacyRectangleEffectStatus::unsupported_mode;
}

}  // namespace

LegacyEffectPanelResult draw_legacy_effect_panel(
    LegacyFramebuffer& framebuffer,
    LegacyRasterGeometryState& raster,
    LegacyEffectPanelActionPorts& action_ports,
    LegacyFramePieceProvider& frame_provider,
    const LegacyEffectPanelRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    LegacyEffectPanelResult result;
    result.rectangle_status = apply_legacy_rectangle_effect(
        framebuffer,
        raster,
        effects.pixel_conversion,
        LegacyRectangleEffectRequest{
            .x = wrapping_subtract(request.x, 8),
            .y = wrapping_subtract(request.y, 8),
            .width = wrapping_add(request.width, 16),
            .height = wrapping_add(request.height, 16),
            .red = request.red,
            .green = request.green,
            .blue = request.blue,
            .mode = request.mode,
        }
    );
    if (!rectangle_effect_completed(result.rectangle_status)) {
        result.status = LegacyEffectPanelStatus::rectangle_effect_failed;
        return result;
    }

    if (!action_ports.update_action_frame(
            0x233BU, 0, result.frame_resource_id
        )) {
        result.status = LegacyEffectPanelStatus::action_update_failed;
        return result;
    }

    result.tiled_frame = draw_legacy_tiled_frame(
        framebuffer,
        raster,
        frame_provider,
        LegacyTiledFrameRequest{
            .resource_id = result.frame_resource_id,
            .left = request.x,
            .top = request.y,
            .right = wrapping_add(request.x, request.width),
            .bottom = wrapping_add(request.y, request.height),
            .opacity_step = 0,
            .flags = 0x80000008U,
        },
        effects,
        jitter
    );
    if (result.tiled_frame.status != LegacyTiledFrameStatus::completed) {
        result.status = LegacyEffectPanelStatus::tiled_frame_failed;
    }
    return result;
}

}  // namespace openswd3::rendering
