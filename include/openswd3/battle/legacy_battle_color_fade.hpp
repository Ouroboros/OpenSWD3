#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleColorFadeState {
    std::array<compat::u8, 4> source_argument_slot{};
};

// sub_450A50. shared_request supplies the entry snapshots of blitter state not
// carried by this wrapper's stack arguments. The rectangle, mode, and trailing
// opacity argument are overwritten exactly as in the original call.
[[nodiscard]] rendering::LegacyBlitResult fade_legacy_battle_rectangle(
    LegacyBattleColorFadeState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter,
    compat::i32 destination_x,
    compat::i32 destination_y,
    compat::i32 width,
    compat::i32 height,
    compat::u32 color_argument
) noexcept;

}  // namespace openswd3::battle
