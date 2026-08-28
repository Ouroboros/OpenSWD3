#pragma once

#include <array>
#include <vector>

#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGrowthItemCompletionFormatToken =
    0x004A7AD4U;
inline constexpr compat::u32 kLegacyBattleGrowthItemCompletionFontToken =
    0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleGrowthItemCompletionFramebufferToken =
    0x004CD76CU;
inline constexpr compat::u32 kLegacyBattleGrowthItemCompletionCaptionToken =
    0x0053C154U;

struct LegacyBattleGrowthItemCompletionPanelBindings {
    LegacyBattleLevelAdvancementState& level_advancement;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    LegacyBattleVictoryRewardState& victory_rewards;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    rendering::LegacyFramePieceProvider& frame_provider;
};

enum class LegacyBattleGrowthItemCompletionPanelCall : compat::u8 {
    format_text,
    measure_text,
    query_panel,
    set_font_size,
    draw_text,
};

struct LegacyBattleGrowthItemCompletionPanelCallRequest {
    LegacyBattleGrowthItemCompletionPanelCall call{
        LegacyBattleGrowthItemCompletionPanelCall::format_text
    };
    std::array<compat::u32, 8U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleGrowthItemCompletionPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_transition_stage{};
    compat::u32 transition_stage{};
    bool publish_formatted_text{};
    std::array<compat::u8, 64U> formatted_text{};
    compat::u32 formatted_text_length{};
};

class LegacyBattleGrowthItemCompletionPanelPort {
public:
    virtual ~LegacyBattleGrowthItemCompletionPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleGrowthItemCompletionPanelCallReply
    invoke_growth_item_completion_panel(
        const LegacyBattleGrowthItemCompletionPanelCallRequest& request
    ) {
        LegacyBattleGrowthItemCompletionPanelCallReply reply{};
        reply.eax = request.eax;
        reply.ecx = request.ecx;
        reply.edx = request.edx;
        if (request.call ==
            LegacyBattleGrowthItemCompletionPanelCall::format_text) {
            reply.eax = request.text_length;
            reply.publish_formatted_text = true;
            reply.formatted_text = request.text;
            reply.formatted_text_length = request.text_length;
        } else if (
            request.call ==
            LegacyBattleGrowthItemCompletionPanelCall::measure_text
        ) {
            reply.eax = request.text_length;
        }
        return reply;
    }
};

struct LegacyBattleGrowthItemCompletionPanelRequest {
    compat::u8 initial_text_byte{0xFFU};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters frame_return{};
    compat::u32 local_text_token{};
};

enum class LegacyBattleGrowthItemCompletionPanelStatus : compat::u8 {
    completed,
    caption_source_typed_stop,
    format_buffer_typed_stop,
    rectangle_typed_stop,
    frame_typed_stop,
};

struct LegacyBattleGrowthItemCompletionPanelResult {
    LegacyBattleGrowthItemCompletionPanelStatus status{
        LegacyBattleGrowthItemCompletionPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 format_calls{};
    compat::u32 length_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 query_calls{};
    compat::u32 font_size_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 first_measured_length{};
    compat::u32 second_measured_length{};
    compat::i32 half_text_length{};
    compat::u32 panel_base_width{};
    compat::i32 rectangle_width{};
    compat::i32 rectangle_height{};
    compat::u32 frame_resource_id{};
    compat::i32 frame_right{};
    compat::i32 frame_bottom{};
    std::array<compat::u8, 64U> formatted_text{};
    compat::u32 formatted_text_length{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    rendering::LegacyTiledFrameResult tiled_frame{};
    std::vector<LegacyBattleGrowthItemCompletionPanelCall> call_trace;
};

// Typed closure of legacy 0x00468ED0.
[[nodiscard]] LegacyBattleGrowthItemCompletionPanelResult
advance_legacy_battle_growth_item_completion_panel(
    LegacyBattleGrowthItemCompletionPanelBindings bindings,
    LegacyBattleGrowthItemCompletionPanelPort& port,
    const LegacyBattleGrowthItemCompletionPanelRequest& request = {}
);

}  // namespace openswd3::battle
