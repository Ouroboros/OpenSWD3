#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGuardPanelTitleToken = 0x004A79D0U;
inline constexpr compat::u32 kLegacyBattleMissingGuardTextToken = 0x0049F9FCU;

struct LegacyBattleGuardPanelFrameBindings {
    compat::u32 group_b_row_selection{};
    compat::u32 group_a_count{};
    compat::u32 target_effect_value{};
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleGuardPanelFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleGridFrameRegisterSnapshot panel_rectangle_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 2>
        tiled_frame_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot
        selection_rectangle_return_registers{};
};

enum class LegacyBattleGuardPanelFrameStatus : compat::u8 {
    completed,
    panel_rectangle_typed_stop,
    first_tiled_frame_typed_stop,
    second_tiled_frame_typed_stop,
    selection_rectangle_typed_stop,
    group_a_actor_typed_stop,
};

struct LegacyBattleGuardPanelRowTrace {
    compat::u32 actor_index{};
    compat::u32 actor_token{};
    compat::u32 label_token{};
    compat::u32 x{};
    compat::u32 y{};
};

struct LegacyBattleGuardPanelFrameResult {
    LegacyBattleGuardPanelFrameStatus status{
        LegacyBattleGuardPanelFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 panel_rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 selection_rectangle_calls{};
    compat::u32 actor_label_query_calls{};
    compat::u32 font_width_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 displayed_rows{};
    bool missing_row_drawn{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    rendering::LegacyRectangleEffectStatus selection_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<LegacyBattleGuardPanelRowTrace, 10> rows{};
};

// Typed closure of legacy 0x004667B0.
[[nodiscard]] LegacyBattleGuardPanelFrameResult
draw_legacy_battle_guard_panel_frame(
    LegacyBattleGuardPanelFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleGuardPanelFrameRequest& request = {}
);

}  // namespace openswd3::battle
