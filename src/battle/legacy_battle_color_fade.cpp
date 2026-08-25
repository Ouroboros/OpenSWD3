#include "openswd3/battle/legacy_battle_color_fade.hpp"

#include <span>

namespace openswd3::battle {

rendering::LegacyBlitResult fade_legacy_battle_rectangle(
    LegacyBattleColorFadeState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    const rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const compat::i32 width,
    const compat::i32 height,
    const compat::u32 color_argument
) noexcept {
    state.source_argument_slot = {
        static_cast<compat::u8>(color_argument & 0xFFU),
        static_cast<compat::u8>((color_argument >> 8U) & 0xFFU),
        static_cast<compat::u8>((color_argument >> 16U) & 0xFFU),
        static_cast<compat::u8>(color_argument >> 24U),
    };

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = destination_x;
    request.destination_y = destination_y;
    request.source_width = width;
    request.source_height = height;
    request.flags = 8U;
    request.opacity_step = 0;

    return rendering::blit_legacy_copy_paths(
        framebuffer,
        clip,
        rendering::LegacyBlitSource{
            .bytes = std::span<const compat::u8>{state.source_argument_slot},
            .layout = rendering::LegacyBlitSourceLayout::direct_16,
        },
        request,
        effects,
        jitter
    );
}

}  // namespace openswd3::battle
