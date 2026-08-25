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
    width_nonpositive,
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
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_4504E0: query and draw the selected frame at the supplied coordinates.
[[nodiscard]] LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::u32 frame_index,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_450490: query the selected frame and draw an explicit width by its height.
[[nodiscard]] LegacyBattleFrameDrawResult
draw_legacy_battle_resource_frame_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::u32 frame_index,
    compat::i32 x,
    compat::i32 y,
    compat::i32 explicit_width
) noexcept;

}  // namespace openswd3::battle
