#pragma once

#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::rendering {

class LegacyEffectPanelActionPorts {
public:
    virtual ~LegacyEffectPanelActionPorts() = default;

    [[nodiscard]] virtual bool update_action_frame(
        compat::u32 action_id,
        compat::i32 action_index,
        compat::u16& frame_resource_id
    ) noexcept = 0;
};

struct LegacyEffectPanelRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 red{};
    compat::i32 green{};
    compat::i32 blue{};
    compat::u32 mode{};
};

enum class LegacyEffectPanelStatus : compat::u8 {
    completed,
    rectangle_effect_failed,
    action_update_failed,
    tiled_frame_failed,
};

struct LegacyEffectPanelResult {
    LegacyEffectPanelStatus status{LegacyEffectPanelStatus::completed};
    LegacyRectangleEffectStatus rectangle_status{
        LegacyRectangleEffectStatus::completed
    };
    compat::u16 frame_resource_id{};
    LegacyTiledFrameResult tiled_frame{};
};

[[nodiscard]] LegacyEffectPanelResult draw_legacy_effect_panel(
    LegacyFramebuffer& framebuffer,
    LegacyRasterGeometryState& raster,
    LegacyEffectPanelActionPorts& action_ports,
    LegacyFramePieceProvider& frame_provider,
    const LegacyEffectPanelRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

}  // namespace openswd3::rendering
