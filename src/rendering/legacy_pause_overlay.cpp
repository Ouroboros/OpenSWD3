#include "openswd3/rendering/legacy_pause_overlay.hpp"

#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <cstddef>

namespace openswd3::rendering {
namespace {

constexpr std::array<compat::u8, 23> kPauseText{
    0xB9U, 0x43U, 0xC0U, 0xB8U, 0xBCU, 0xC8U, 0xB0U, 0xB1U,
    0x20U, 0x20U, 0xABU, 0xF6U, 0x46U, 0x38U, 0xC4U, 0x7EU,
    0xC4U, 0xF2U, 0xB9U, 0x43U, 0xC0U, 0xB8U, 0x00U,
};

}  // namespace

std::span<const compat::u8> legacy_pause_overlay_text() noexcept {
    return kPauseText;
}

LegacyPauseOverlayResult draw_legacy_pause_overlay(
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
) {
    constexpr compat::i32 kByteLength =
        static_cast<compat::i32>(kPauseText.size() - 1U);
    constexpr compat::i32 kHalfByteLength = kByteLength / 2;

    LegacyPauseOverlayResult result;
    result.layout = LegacyPauseOverlayLayout{
        .x = 320 - 11 * kHalfByteLength,
        .y = 229,
        .width = 22 * kHalfByteLength,
        .height = 22,
        .foreground_color = static_cast<compat::u16>(
            legacy_pack_color_pair(
                effects.pixel_conversion,
                25,
                23,
                17
            )
        ),
    };

    result.panel = draw_legacy_effect_panel(
        framebuffer,
        raster,
        panel_action_ports,
        frame_provider,
        LegacyEffectPanelRequest{
            .x = result.layout.x,
            .y = result.layout.y,
            .width = result.layout.width,
            .height = result.layout.height,
        },
        effects,
        jitter
    );

    result.text = draw_legacy_text(
        framebuffer,
        glyph_cache,
        glyph_provider,
        text_state,
        LegacyTextDrawRequest{
            .destination_x = result.layout.x,
            .destination_y = result.layout.y,
            .nul_terminated_text = kPauseText,
            .foreground_color = result.layout.foreground_color,
            .flags = 4U,
        }
    );

    result.presentation = submit_legacy_presentation(
        LegacyPresentationSite::pause_overlay,
        presentation_ports
    );
    return result;
}

}  // namespace openswd3::rendering
