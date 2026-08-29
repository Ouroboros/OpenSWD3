#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr std::array<compat::u8, 3> kLegacyBattleModeGridMissingText{
    0xB5U, 0x4CU, 0U
};

struct LegacyBattleModeGridFrameState {
    std::array<compat::u8, 20> row_text{};
    compat::u32 primary_count{};
    compat::u32 secondary_count{};
    compat::u32 group_slot_count{};
};

struct LegacyBattleModeGridFrameBindings {
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

struct LegacyBattleModeGridFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_cell{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 row_text_token{};
    compat::u32 primary_count_token{};
    compat::u32 secondary_count_token{};
    compat::u32 title_text_token{kLegacyBattleStaticActionTextTokens[9U]};
    compat::u32 missing_text_token{0x0049F9FCU};
    LegacyBattleGridFrameRegisterSnapshot panel_rectangle_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 2>
        tiled_frame_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot
        selection_rectangle_return_registers{};
};

enum class LegacyBattleModeGridFrameStatus : compat::u8 {
    completed,
    panel_rectangle_typed_stop,
    first_tiled_frame_typed_stop,
    second_tiled_frame_typed_stop,
    group_a_actor_typed_stop,
    selection_rectangle_typed_stop,
};

struct LegacyBattleModeGridCellTrace {
    compat::u32 cell{};
    compat::u32 x{};
    compat::u32 y{};
    compat::u32 page{};
    compat::u32 group_index{};
    bool queried_secondary{};
    bool missing{};
    bool selected{};
    std::array<compat::u8, 20> row_text{};
};

struct LegacyBattleModeGridFrameResult {
    LegacyBattleModeGridFrameStatus status{
        LegacyBattleModeGridFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 font_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 panel_rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 primary_query_calls{};
    compat::u32 secondary_count_query_calls{};
    compat::u32 secondary_row_query_calls{};
    compat::u32 actor_refresh_calls{};
    compat::u32 text_copy_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 selection_rectangle_calls{};
    compat::u32 selected_page{};
    compat::u32 selected_group_index{};
    LegacyBattleActorModeResourceQueryResult primary_query{};
    LegacyBattleActorModeResourceCountResult secondary_count_query{};
    LegacyBattleActorModeResourceQueryResult secondary_row_query{};
    std::array<LegacyBattleActorResourceListCommitResult, 2> actor_refreshes{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    std::array<LegacyBattleModeGridCellTrace, 10> cells{};
    rendering::LegacyRectangleEffectStatus selection_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
};

// Typed closure of legacy 0x00466190.
[[nodiscard]] LegacyBattleModeGridFrameResult
draw_legacy_battle_mode_grid_frame(
    LegacyBattleModeGridFrameState& state,
    LegacyBattleModeGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleModeGridFrameRequest& request
);

}  // namespace openswd3::battle
