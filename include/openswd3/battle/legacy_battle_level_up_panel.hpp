#pragma once

#include <array>
#include <span>
#include <vector>

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleLevelUpTitleToken = 0x004A7A44U;
inline constexpr compat::u32 kLegacyBattleLevelUpFormatToken = 0x004A7A38U;
inline constexpr compat::u32 kLegacyBattleLevelUpNameBaseToken = 0x0049E148U;
inline constexpr std::array<compat::u8, 4> kLegacyBattleLevelUpTitle{
    0xA4U,
    0xC9U,
    0xAFU,
    0xC5U,
};

struct LegacyBattleLevelUpPanelBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleStartupState& startup;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    std::span<world_map::LegacyWorldStoryPartyMemberResources>
        party_member_resources;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleLevelUpPanelRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters summary_frame_return{};
    compat::u32 local_text_token{};
};

enum class LegacyBattleLevelUpPanelStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    summary_frame_typed_stop,
    actor_index_typed_stop,
    party_member_resource_typed_stop,
    format_buffer_typed_stop,
};

struct LegacyBattleLevelUpPanelResult {
    LegacyBattleLevelUpPanelStatus status{
        LegacyBattleLevelUpPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 formatted_text_length{};
    std::array<compat::u8, 64> formatted_text{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    std::vector<LegacyBattleVictoryRewardCall> call_trace;
};

// Typed closure of legacy 0x00467AC0.
[[nodiscard]] LegacyBattleLevelUpPanelResult draw_legacy_battle_level_up_panel(
    LegacyBattleLevelUpPanelBindings bindings,
    LegacyBattleVictoryRewardPort& port,
    const LegacyBattleLevelUpPanelRequest& request = {}
);

}  // namespace openswd3::battle
