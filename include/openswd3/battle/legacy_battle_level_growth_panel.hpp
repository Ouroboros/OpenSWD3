#pragma once

#include <array>
#include <vector>

#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleLevelGrowthNameBaseToken =
    0x0049E148U;
inline constexpr compat::u32 kLegacyBattleLevelGrowthArrowToken = 0x004A7A50U;
inline constexpr compat::u32 kLegacyBattleLevelGrowthIntegerFormatToken =
    0x004A7A4CU;
inline constexpr compat::u32 kLegacyBattleLevelGrowthSample = 0x125U;
inline constexpr std::array<compat::u32, 7U>
    kLegacyBattleLevelGrowthLabelFormatTokens{
        0x004A7AA0U,
        0x004A7A94U,
        0x004A7A88U,
        0x004A7A7CU,
        0x004A7A70U,
        0x004A7A64U,
        0x004A7A58U,
    };

struct LegacyBattleLevelGrowthPanelBindings {
    LegacyBattleLevelAdvancementState& advancement;
    LegacyBattleVictoryRewardBindings victory;
};

enum class LegacyBattleLevelGrowthPanelCall : compat::u8 {
    reserved_transition_stage_advance_slot,
    format_integer,
    draw_text,
};

struct LegacyBattleLevelGrowthPanelCallRequest {
    LegacyBattleLevelGrowthPanelCall call{
        LegacyBattleLevelGrowthPanelCall::reserved_transition_stage_advance_slot
    };
    std::array<compat::u32, 6U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleLevelGrowthPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_transition_actor_index{};
    compat::u8 transition_actor_index{};
    bool publish_transition_stage{};
    compat::u32 transition_stage{};
    bool publish_formatted_text{};
    std::array<compat::u8, 64U> formatted_text{};
    compat::u32 formatted_text_length{};
};

struct LegacyBattleLevelGrowthPanelRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleLevelGrowthPanelPort {
public:
    virtual ~LegacyBattleLevelGrowthPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleLevelGrowthPanelCallReply
    invoke_level_growth_panel(
        const LegacyBattleLevelGrowthPanelCallRequest& request
    ) {
        LegacyBattleLevelGrowthPanelCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call == LegacyBattleLevelGrowthPanelCall::format_integer) {
            reply.eax = request.text_length;
            reply.publish_formatted_text = true;
            reply.formatted_text = request.text;
            reply.formatted_text_length = request.text_length;
        }
        return reply;
    }

    [[nodiscard]] virtual LegacyBattleLevelGrowthPanelRegisters
    play_level_growth_sample(
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx,
        compat::u32,
        compat::i32
    ) {
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }
};

struct LegacyBattleLevelGrowthPanelRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters summary_frame_return{};
    compat::u32 local_text_token{};
};

enum class LegacyBattleLevelGrowthPanelStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    summary_frame_typed_stop,
    transition_stage_typed_stop,
    actor_index_typed_stop,
    party_member_resource_typed_stop,
    format_buffer_typed_stop,
};

struct LegacyBattleLevelGrowthPanelResult {
    LegacyBattleLevelGrowthPanelStatus status{
        LegacyBattleLevelGrowthPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 format_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 sample_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 displayed_growth_values{};
    compat::u32 decremented_growth_values{};
    compat::u32 transition_stage_calls{};
    LegacyBattleTransitionStageAdvanceResult transition_stage{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2U> tiled_frames{};
    std::vector<LegacyBattleLevelGrowthPanelCall> call_trace;
};

// Typed closure of legacy 0x00467F00.
[[nodiscard]] LegacyBattleLevelGrowthPanelResult
advance_legacy_battle_level_growth_panel(
    LegacyBattleLevelGrowthPanelBindings bindings,
    LegacyBattleLevelGrowthPanelPort& port,
    const LegacyBattleLevelGrowthPanelRequest& request = {}
);

}  // namespace openswd3::battle
