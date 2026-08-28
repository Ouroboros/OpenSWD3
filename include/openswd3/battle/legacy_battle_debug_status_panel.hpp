#pragma once

#include "openswd3/battle/legacy_battle_color_fade.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_selection_frame.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_text_panel.hpp"
#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleDebugStatusPanelAction = 0x233BU;
inline constexpr compat::u32 kLegacyBattleDebugStatusPanelFontToken =
    0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleDebugStatusPanelSurfaceToken =
    0x004CD76CU;
inline constexpr compat::u32 kLegacyBattleDebugStatusPanelLabelTableToken =
    0x004A79A8U;
inline constexpr std::array<compat::u32, 9>
    kLegacyBattleDebugStatusPanelLabelTokens{
        0x004A718CU,
        0x004A7174U,
        0x004A7184U,
        0x004A7170U,
        0x004A716CU,
        0x004A7180U,
        0x004A7188U,
        0x004A7168U,
        0x004A7164U,
    };

struct LegacyBattleDebugStatusPanelRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleDebugStatusPanelBindings {
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    LegacyBattleVictoryRewardState& victory_rewards;
    LegacyBattleSelectionFrameState& selection_frame;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    const rendering::LegacyPixelConversionState& pixel_conversion;
};

struct LegacyBattleDebugStatusPanelRequest {
    LegacyBattleDebugStatusPanelRegisters entry{};
    compat::u32 local_text_token{};
    compat::u8 local_text_seed{};
    LegacyBattleDebugStatusPanelRegisters rectangle_return{};
    LegacyBattleDebugStatusPanelRegisters panel_frame_return{};
    LegacyBattleDebugStatusPanelRegisters color_fade_return{};
};

enum class LegacyBattleDebugStatusPanelStatus : compat::u8 {
    completed,
    profile_typed_stop,
    panel_rectangle_typed_stop,
    panel_frame_typed_stop,
    title_frame_typed_stop,
    text_format_typed_stop,
    color_fade_typed_stop,
    transition_stage_typed_stop,
};

struct LegacyBattleDebugStatusPanelRow {
    compat::u32 label_token{};
    compat::i8 value{};
    compat::u32 packed_color{};
    compat::u32 fade_width{};
    compat::i32 text_y{};
    compat::i32 fade_y{};
    std::array<compat::u8, 8> formatted_text{};
};

struct LegacyBattleDebugStatusPanelResult {
    LegacyBattleDebugStatusPanelStatus status{
        LegacyBattleDebugStatusPanelStatus::completed
    };
    LegacyBattleDebugStatusPanelRegisters return_registers{};
    bool debug_gate_open{};
    bool stage_gate_open{};
    std::array<compat::u8, 8> local_text{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    compat::u32 panel_action_update_calls{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    compat::u32 panel_rectangle_calls{};
    compat::i32 panel_rectangle_height{};
    std::array<rendering::LegacyTiledFrameResult, 2> panel_frames{};
    std::array<compat::u32, 2> panel_frame_resources{};
    std::array<compat::i32, 2> panel_frame_bottoms{};
    compat::u32 panel_frame_calls{};
    LegacyBattleTransitionStageAdvanceResult transition_stage_advance{};
    compat::u32 transition_stage_advance_calls{};
    compat::u32 text_calls{};
    compat::u32 color_fade_calls{};
    compat::u32 port_calls{};
    std::array<LegacyBattleDebugStatusPanelRow, 9> rows{};
    std::vector<compat::u32> call_trace;
};

// Typed closure of legacy 0x00469650. Draws the debug-only nine-row battle
// status panel while preserving signed byte branches, x87 truncation widths,
// the shared transition-stage gate, and the original call order.
[[nodiscard]] LegacyBattleDebugStatusPanelResult
draw_legacy_battle_debug_status_panel(
    LegacyBattleDebugStatusPanelBindings bindings,
    LegacyBattleTextPanelPort& port,
    const LegacyBattleDebugStatusPanelRequest& request
);

}  // namespace openswd3::battle
