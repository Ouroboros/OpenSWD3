#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionUpdateRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

enum class LegacyBattleVerticalPanelStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
    ratio_divide_by_zero,
    ratio_divide_overflow,
    fill_loop_nonterminating,
};

struct LegacyBattleVerticalPanelState {
    asset_runtime::LegacyActionRecord action_record{};
    std::array<bool, 4> action_update_attempted{};
    std::array<asset_runtime::LegacyActionUpdateResult, 4> action_updates{};
    std::array<compat::u32, 4> base_variants{};
    std::array<compat::u32, 4> frame_resource_ids{};
    std::array<compat::u32, 4> frame_indices{};
    std::array<bool, 4> frame_available{};
    std::array<rendering::LegacyFramePiece, 4> frames{};
    bool source_published{};
    rendering::LegacyBlitSource current_source{};

    compat::i32 maximum_count{};
    compat::i32 current_count{};
    compat::i32 ratio_quotient{};
    compat::i32 panel_content_top{};
    compat::i32 fill_start{};
    compat::i32 fill_clip_bottom{};
    compat::i32 panel_content_bottom{};

    rendering::LegacyBlitClipRectangle shared_clip{0, 0, 640, 480};
    compat::i32 screen_width{640};
    compat::i32 screen_height{480};
};

struct LegacyBattleVerticalPanelResult {
    LegacyBattleVerticalPanelStatus status{
        LegacyBattleVerticalPanelStatus::completed
    };
    compat::u32 stopped_phase{};
    compat::u32 action_update_calls{};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    std::array<compat::u32, 4> phase_draw_calls{};
    compat::u32 middle_draw_count{};
    compat::u32 fill_draw_count{};
    compat::i32 repeated_middle_height{};
    compat::i32 scaled_fill_height{};
    compat::i32 ratio_quotient{};
    compat::i32 bottom_draw_y{};
    compat::u32 clip_set_calls{};
    compat::u32 return_value{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_450C50: draw a four-phase vertical battle status panel.
[[nodiscard]] LegacyBattleVerticalPanelResult draw_legacy_battle_vertical_panel(
    LegacyBattleVerticalPanelState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 action_id,
    compat::i32 x,
    compat::i32 y,
    compat::i32 middle_count,
    compat::i32 fill_offset,
    compat::u32 selector,
    const std::array<LegacyBattleActionUpdateRegisterSnapshot, 4>&
        update_registers,
    compat::u32 final_blit_eax_snapshot
);

}  // namespace openswd3::battle
