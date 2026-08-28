#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleNarrowGridTitleToken =
    kLegacyBattleStaticActionTextTokens[19U];

struct LegacyBattleNarrowGridFrameState {
    compat::u32 display_count{};
    compat::u32 row_value{};
};

struct LegacyBattleNarrowGridFrameBindings {
    compat::u32 queued_actor_code{};
    compat::u8& panel_row_limit;
    compat::u32& selection_input_gate;
    compat::u32& candidate_argument;
    compat::u16 primary_text_color{};
    std::array<compat::u32, 5>& selection_workspace;
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleNarrowGridFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_row{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 row_text_token{0x0053C184U};
    compat::u32 row_value_token{};
    LegacyBattleGridFrameRegisterSnapshot panel_rectangle_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 2>
        tiled_frame_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot
        selection_rectangle_return_registers{};
};

enum class LegacyBattleNarrowGridFrameStatus : compat::u8 {
    completed,
    panel_rectangle_typed_stop,
    first_tiled_frame_typed_stop,
    second_tiled_frame_typed_stop,
    group_a_actor_typed_stop,
    selection_rectangle_typed_stop,
};

struct LegacyBattleNarrowGridRowTrace {
    compat::u32 iterator{};
    compat::u32 row_value{};
    compat::u32 x{};
    compat::u32 y{};
    bool selected{};
    std::array<compat::u8, 20> row_text{};
};

struct LegacyBattleNarrowGridFrameResult {
    LegacyBattleNarrowGridFrameStatus status{
        LegacyBattleNarrowGridFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 panel_rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 font_calls{};
    compat::u32 actor_initialization_calls{};
    compat::u32 actor_refresh_calls{};
    compat::u32 row_query_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 selection_rectangle_calls{};
    compat::u32 final_iterator{};
    compat::u32 displayed_rows{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    rendering::LegacyRectangleEffectStatus selection_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<LegacyBattleNarrowGridRowTrace, 7> rows{};
};

// Typed closure of legacy 0x00466500.
[[nodiscard]] LegacyBattleNarrowGridFrameResult
draw_legacy_battle_narrow_grid_frame(
    LegacyBattleNarrowGridFrameState& state,
    LegacyBattleNarrowGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleNarrowGridFrameRequest& request = {}
);

}  // namespace openswd3::battle
