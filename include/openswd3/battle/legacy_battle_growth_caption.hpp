#pragma once

#include <array>
#include <vector>

#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGrowthCaptionNameFormatToken =
    0x004A7AB4U;
inline constexpr compat::u32 kLegacyBattleGrowthCaptionDetailFormatToken =
    0x004A7AACU;
inline constexpr compat::u32 kLegacyBattleGrowthCaptionNameBaseToken =
    0x0049E148U;

struct LegacyBattleGrowthCaptionBindings {
    LegacyBattleLevelAdvancementState& advancement;
    LegacyBattleVictoryRewardBindings victory;
};

enum class LegacyBattleGrowthCaptionCall : compat::u8 {
    format_name,
    query_panel,
    draw_text,
    format_detail,
};

struct LegacyBattleGrowthCaptionCallRequest {
    LegacyBattleGrowthCaptionCall call{
        LegacyBattleGrowthCaptionCall::format_name
    };
    std::array<compat::u32, 6U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleGrowthCaptionCallReply {
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

class LegacyBattleGrowthCaptionPort {
public:
    virtual ~LegacyBattleGrowthCaptionPort() = default;

    [[nodiscard]] virtual LegacyBattleGrowthCaptionCallReply
    invoke_growth_caption(const LegacyBattleGrowthCaptionCallRequest& request) {
        LegacyBattleGrowthCaptionCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call == LegacyBattleGrowthCaptionCall::format_name ||
            request.call == LegacyBattleGrowthCaptionCall::format_detail) {
            reply.eax = request.text_length;
            reply.publish_formatted_text = true;
            reply.formatted_text = request.text;
            reply.formatted_text_length = request.text_length;
        }
        return reply;
    }
};

struct LegacyBattleGrowthCaptionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters frame_return{};
    compat::u32 local_text_token{};
};

enum class LegacyBattleGrowthCaptionStatus : compat::u8 {
    completed,
    actor_index_typed_stop,
    name_format_buffer_typed_stop,
    rectangle_typed_stop,
    frame_typed_stop,
    caption_source_typed_stop,
    detail_format_buffer_typed_stop,
};

struct LegacyBattleGrowthCaptionResult {
    LegacyBattleGrowthCaptionStatus status{
        LegacyBattleGrowthCaptionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 format_calls{};
    compat::u32 length_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 name_length{};
    compat::u32 detail_length{};
    compat::i32 rectangle_width{};
    compat::i32 detail_x{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    rendering::LegacyTiledFrameResult tiled_frame{};
    std::vector<LegacyBattleGrowthCaptionCall> call_trace;
};

// Typed closure of legacy 0x00468930.
[[nodiscard]] LegacyBattleGrowthCaptionResult
advance_legacy_battle_growth_caption(
    LegacyBattleGrowthCaptionBindings bindings,
    LegacyBattleGrowthCaptionPort& port,
    const LegacyBattleGrowthCaptionRequest& request = {}
);

}  // namespace openswd3::battle
