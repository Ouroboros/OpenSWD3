#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

struct LegacyBattleFrameDrawState {
    bool frame_record_published{};
    bool frame_record_available{};
    bool source_published{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

enum class LegacyBattleFrameDrawStatus : compat::u8 {
    completed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleFrameDrawResult {
    LegacyBattleFrameDrawStatus status{LegacyBattleFrameDrawStatus::completed};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_450270: query frame zero and draw it once at the supplied coordinates.
[[nodiscard]] LegacyBattleFrameDrawResult draw_legacy_battle_frame_zero(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y
) noexcept;

}  // namespace openswd3::battle
