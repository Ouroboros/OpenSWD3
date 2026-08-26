#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

enum class LegacyBattleScaleFillPanelStatus : compat::u8 {
    completed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleScaleFillPanelState {
    bool frame_record_published{};
    bool frame_record_available{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    bool source_published{};
    rendering::LegacyBlitSource current_source{};

    rendering::LegacyBlitClipRectangle shared_clip{0, 0, 640, 480};
    compat::i32 screen_width{640};
    compat::i32 screen_height{480};
};

struct LegacyBattleScaleFillPanelResult {
    LegacyBattleScaleFillPanelStatus status{
        LegacyBattleScaleFillPanelStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 clip_set_calls{};
    compat::i32 segment_height{};
    compat::i32 fill_height{};
    compat::i32 content_y{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_4512B0: draw the six-level battle scale fill panel.
[[nodiscard]] LegacyBattleScaleFillPanelResult
draw_legacy_battle_scale_fill_panel(
    LegacyBattleScaleFillPanelState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::i32 x,
    compat::i32 y,
    compat::i32 level
) noexcept;

}  // namespace openswd3::battle
