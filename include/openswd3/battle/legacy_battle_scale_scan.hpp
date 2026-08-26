#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleScaleScanStatus : compat::u8 {
    completed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleScaleScanState {
    std::array<compat::u16, 3> thresholds{};
    compat::u32 scan_counter{};
    compat::u16 selection_marker{};
    compat::u32 target_selection{};

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

struct LegacyBattleScaleScanResult {
    LegacyBattleScaleScanStatus status{LegacyBattleScaleScanStatus::completed};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 clip_set_calls{};
    compat::u32 threshold_iterations{};
    compat::u32 selection_hits{};
    compat::i32 final_scan_x{};
    compat::u32 return_value{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_451100: draw the three-threshold battle scale scan animation.
[[nodiscard]] LegacyBattleScaleScanResult draw_legacy_battle_scale_scan(
    LegacyBattleScaleScanState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::i32 x,
    compat::i32 y
) noexcept;

}  // namespace openswd3::battle
