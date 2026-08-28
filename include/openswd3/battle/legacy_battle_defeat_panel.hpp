#pragma once

#include <array>
#include <vector>

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleDefeatTitleToken = 0x004A7B08U;
inline constexpr compat::u32 kLegacyBattleDefeatDetailToken = 0x004A7AFCU;
inline constexpr compat::u32 kLegacyBattleDefeatFramebufferToken = 0x004CD76CU;

struct LegacyBattleDefeatPanelBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

enum class LegacyBattleDefeatPanelCall : compat::u8 {
    draw_title,
    reserved_transition_stage_advance_slot,
    set_font_size,
    draw_detail,
};

struct LegacyBattleDefeatPanelCallRequest {
    LegacyBattleDefeatPanelCall call{LegacyBattleDefeatPanelCall::draw_title};
    compat::u32 object_token{};
    std::array<compat::u32, 8U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 16U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleDefeatPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_stage{};
    compat::u32 stage{};
};

class LegacyBattleDefeatPanelPort {
public:
    virtual ~LegacyBattleDefeatPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleDefeatPanelCallReply
    invoke_defeat_panel(const LegacyBattleDefeatPanelCallRequest& request) {
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }
};

struct LegacyBattleDefeatPanelRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters action_return{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters detail_frame_return{};
};

enum class LegacyBattleDefeatPanelStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    detail_frame_typed_stop,
    transition_stage_typed_stop,
};

struct LegacyBattleDefeatPanelResult {
    LegacyBattleDefeatPanelStatus status{
        LegacyBattleDefeatPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 title_draw_calls{};
    compat::u32 query_calls{};
    compat::u32 font_size_calls{};
    compat::u32 detail_draw_calls{};
    compat::u32 transition_stage_calls{};
    LegacyBattleTransitionStageAdvanceResult transition_stage{};
    compat::i32 rectangle_height{};
    compat::i32 detail_frame_bottom{};
    compat::u32 first_frame_resource{};
    compat::u32 second_frame_resource{};
    LegacyBattleVictoryRewardRegisters action_entry{};
    LegacyBattleVictoryRewardRegisters rectangle_entry{};
    LegacyBattleVictoryRewardRegisters first_frame_entry{};
    LegacyBattleVictoryRewardRegisters second_frame_entry{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2U> tiled_frames{};
    std::vector<LegacyBattleDefeatPanelCall> call_trace;
};

// Typed closure of legacy 0x00469220.
[[nodiscard]] LegacyBattleDefeatPanelResult draw_legacy_battle_defeat_panel(
    LegacyBattleDefeatPanelBindings bindings,
    LegacyBattleDefeatPanelPort& port,
    const LegacyBattleDefeatPanelRequest& request = {}
);

}  // namespace openswd3::battle
