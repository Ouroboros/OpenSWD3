#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

struct LegacyBattleActionFrameDrawState {
    asset_runtime::LegacyActionRecord action_record{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    bool action_update_attempted{};
    bool frame_record_published{};
    bool frame_record_available{};
    bool source_published{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
    std::array<compat::u16, 2> outline_color_slot{};
};

enum class LegacyBattleActionFrameDrawStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    outline_state_out_of_range,
    outline_typed_stop,
    primary_blit_typed_stop,
    overlay_blit_typed_stop,
};

struct LegacyBattleActionFrameDrawResult {
    LegacyBattleActionFrameDrawStatus status{
        LegacyBattleActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 outline_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    rendering::LegacyOutlineBlitResult outline{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_4502B0.
[[nodiscard]] LegacyBattleActionFrameDrawResult draw_legacy_battle_action_frame(
    LegacyBattleActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    std::span<const compat::u8> outline_state_by_variant,
    compat::u32 variant,
    compat::i32 x,
    compat::i32 y,
    compat::u32 overlay_selector
);

}  // namespace openswd3::battle
