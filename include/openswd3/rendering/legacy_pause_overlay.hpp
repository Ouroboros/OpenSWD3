#pragma once

#include "openswd3/rendering/legacy_effect_panel.hpp"
#include "openswd3/rendering/legacy_presentation.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <span>

namespace openswd3::rendering {

struct LegacyPauseOverlayLayout {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::u16 foreground_color{};
};

struct LegacyPauseOverlayResult {
    LegacyPauseOverlayLayout layout{};
    LegacyEffectPanelResult panel{};
    LegacyTextDrawResult text{};
    LegacyPresentationDispatchResult presentation{};
};

[[nodiscard]] std::span<const compat::u8>
legacy_pause_overlay_text() noexcept;

// sub_411FA0. The caller supplies the already-initialized 20x20 renderer
// state used by the original static object at 0x004AB998.
[[nodiscard]] LegacyPauseOverlayResult draw_legacy_pause_overlay(
    LegacyFramebuffer& framebuffer,
    LegacyRasterGeometryState& raster,
    LegacyEffectPanelActionPorts& panel_action_ports,
    LegacyFramePieceProvider& frame_provider,
    LegacyGlyphCache& glyph_cache,
    LegacyGlyphProvider& glyph_provider,
    const LegacyTextRendererState& text_state,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter,
    LegacyPresentationPorts& presentation_ports
);

}  // namespace openswd3::rendering
