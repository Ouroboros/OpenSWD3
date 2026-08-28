#pragma once

#include "openswd3/battle/legacy_battle_color_fade.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

enum class LegacyBattleBorderSourceKind : compat::u8 {
    none,
    frame_piece,
    color_argument,
};

struct LegacyBattleBorderPanelState {
    bool frame_record_published{};
    bool frame_record_available{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    LegacyBattleBorderSourceKind source_kind{
        LegacyBattleBorderSourceKind::none
    };
};

enum class LegacyBattleBorderPanelStatus : compat::u8 {
    completed,
    frame_unavailable,
    color_fade_typed_stop,
    frame_blit_typed_stop,
};

struct LegacyBattleBorderPanelResult {
    LegacyBattleBorderPanelStatus status{
        LegacyBattleBorderPanelStatus::completed
    };
    compat::u32 frame_index{};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 color_fade_calls{};
    compat::i32 final_x{};
    compat::i32 final_y{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_44FFE0.
[[nodiscard]] LegacyBattleBorderPanelResult draw_legacy_battle_border_panel(
    LegacyBattleBorderPanelState& state,
    LegacyBattleColorFadeState& color_fade,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 left,
    compat::i32 top,
    compat::i32 horizontal_repeat_count,
    compat::i32 vertical_repeat_count,
    compat::u32 color_argument
) noexcept;

}  // namespace openswd3::battle
