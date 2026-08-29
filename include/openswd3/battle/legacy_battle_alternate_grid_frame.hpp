#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleAlternateGridFrameState {
    std::array<compat::u8, 20> row_text{};
    std::array<compat::u8, 20> numeric_text{};
    compat::u32 displayed_rows{};
    compat::u32 row_value{};
    compat::u32 numeric_text_length{};
};

struct LegacyBattleAlternateGridFrameBindings {
    const compat::u32& queued_actor_code;
    compat::u16& panel_row_limit;
    compat::u32& selection_input_gate;
    compat::u32& target_argument;
    const compat::u16& primary_text_color;
    std::span<LegacyBattlePartyStartupRecord> party{};
    bool scripted_port_test_compat{};
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleAlternateGridFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_row{};
    compat::u32 scroll_offset{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 row_text_token{};
    compat::u32 row_value_token{};
    compat::u32 numeric_text_token{};
    compat::u32 title_text_token{kLegacyBattleStaticActionTextTokens[6U]};
    LegacyBattleGridFrameRegisterSnapshot panel_rectangle_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 2>
        tiled_frame_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 7>
        format_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot
        selection_rectangle_return_registers{};
};

enum class LegacyBattleAlternateGridFrameStatus : compat::u8 {
    completed,
    panel_rectangle_typed_stop,
    first_tiled_frame_typed_stop,
    second_tiled_frame_typed_stop,
    group_a_actor_typed_stop,
    selection_rectangle_typed_stop,
};

struct LegacyBattleAlternateGridRowTrace {
    compat::u32 iterator{};
    compat::u32 value{};
    compat::u32 displayed_row{};
    bool selected{};
    compat::u32 numeric_text_length{};
    std::array<compat::u8, 20> row_text{};
    std::array<compat::u8, 20> numeric_text{};
};

struct LegacyBattleAlternateGridFrameResult {
    LegacyBattleAlternateGridFrameStatus status{
        LegacyBattleAlternateGridFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 font_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 panel_rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 actor_initialization_calls{};
    LegacyBattleActorResourceListCountResult actor_initialization{};
    compat::u32 actor_refresh_calls{};
    LegacyBattleActorResourceListCommitResult actor_refresh{};
    compat::u32 row_query_calls{};
    LegacyBattleActorResourceListQueryResult row_query{};
    compat::u32 scanned_rows{};
    compat::u32 displayed_rows{};
    compat::u32 text_draw_calls{};
    compat::u32 selection_rectangle_calls{};
    compat::u32 final_iterator{};
    compat::u32 selected_iterator{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    std::array<LegacyBattleAlternateGridRowTrace, 7> rows{};
    rendering::LegacyRectangleEffectStatus selection_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
};

// Typed closure of legacy 0x00465E50.
[[nodiscard]] LegacyBattleAlternateGridFrameResult
draw_legacy_battle_alternate_grid_frame(
    LegacyBattleAlternateGridFrameState& state,
    LegacyBattleAlternateGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleAlternateGridFrameRequest& request
);

}  // namespace openswd3::battle
