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

inline constexpr compat::u32 kLegacyBattleTalismanFailureDetailToken =
    0x004A7B14U;
inline constexpr compat::u32 kLegacyBattleTalismanFailureTitleToken =
    0x004A7B24U;
inline constexpr compat::u32 kLegacyBattleTalismanSuccessFormatToken =
    0x004A7B30U;
inline constexpr compat::u32 kLegacyBattleTalismanSuccessTitleToken =
    0x004A7B3CU;
inline constexpr compat::u32 kLegacyBattleTalismanFramebufferToken =
    0x004CD76CU;

struct LegacyBattleTalismanResultPanelBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

enum class LegacyBattleTalismanResultPanelCall : compat::u8 {
    reserved_transition_stage_advance_slot,
    draw_success_title,
    format_success_detail,
    draw_success_detail,
    draw_failure_title,
    draw_failure_detail,
};

struct LegacyBattleTalismanResultPanelCallRequest {
    LegacyBattleTalismanResultPanelCall call{
        LegacyBattleTalismanResultPanelCall::
            reserved_transition_stage_advance_slot
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8U> arguments{};
    compat::u32 item_name_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleTalismanResultPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_stage{};
    compat::u32 stage{};
    bool publish_result_mode{};
    compat::u8 result_mode{};
    bool publish_formatted_text{};
    std::array<compat::u8, 64U> formatted_text{};
    compat::u32 formatted_text_length{};
};

class LegacyBattleTalismanResultPanelPort {
public:
    virtual ~LegacyBattleTalismanResultPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleTalismanResultPanelCallReply
    invoke_talisman_result_panel(
        const LegacyBattleTalismanResultPanelCallRequest& request
    ) {
        LegacyBattleTalismanResultPanelCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call ==
            LegacyBattleTalismanResultPanelCall::format_success_detail) {
            reply.eax = 0U;
            reply.publish_formatted_text = true;
        }
        return reply;
    }
};

struct LegacyBattleTalismanResultPanelRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u8 local_text_seed{};
    compat::u32 local_text_token{};
    LegacyBattleVictoryRewardRegisters action_return{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters detail_frame_return{};
};

enum class LegacyBattleTalismanResultPanelStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    detail_frame_typed_stop,
    format_buffer_typed_stop,
    transition_stage_typed_stop,
};

struct LegacyBattleTalismanResultPanelResult {
    LegacyBattleTalismanResultPanelStatus status{
        LegacyBattleTalismanResultPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 query_calls{};
    compat::u32 title_draw_calls{};
    compat::u32 format_calls{};
    compat::u32 detail_draw_calls{};
    compat::u32 transition_stage_calls{};
    LegacyBattleTransitionStageAdvanceResult transition_stage{};
    compat::i32 rectangle_height{};
    compat::i32 detail_frame_bottom{};
    compat::u32 first_frame_resource{};
    compat::u32 second_frame_resource{};
    compat::u32 stopped_text_index{};
    LegacyBattleVictoryRewardRegisters action_entry{};
    LegacyBattleVictoryRewardRegisters rectangle_entry{};
    LegacyBattleVictoryRewardRegisters first_frame_entry{};
    LegacyBattleVictoryRewardRegisters second_frame_entry{};
    std::array<compat::u8, 64U> local_text{};
    compat::u32 local_text_length{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2U> tiled_frames{};
    std::vector<LegacyBattleTalismanResultPanelCall> call_trace;
};

// Typed closure of legacy 0x00469340.
[[nodiscard]] LegacyBattleTalismanResultPanelResult
draw_legacy_battle_talisman_result_panel(
    LegacyBattleTalismanResultPanelBindings bindings,
    LegacyBattleTalismanResultPanelPort& port,
    const LegacyBattleTalismanResultPanelRequest& request = {}
);

}  // namespace openswd3::battle
