#pragma once

#include <array>
#include <vector>

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleVictoryItemListTitleToken =
    0x004A7AF4U;
inline constexpr compat::u32 kLegacyBattleVictoryItemListFormatToken =
    0x004A7AE8U;
inline constexpr compat::u32 kLegacyBattleVictoryItemListFramebufferToken =
    0x004CD76CU;

struct LegacyBattleVictoryItemListPanelBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

enum class LegacyBattleVictoryItemListPanelCall : compat::u8 {
    set_font_size,
    draw_title,
    query_panel,
    format_item_row,
    draw_item_row,
};

struct LegacyBattleVictoryItemListPanelCallRequest {
    LegacyBattleVictoryItemListPanelCall call{
        LegacyBattleVictoryItemListPanelCall::set_font_size
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8U> arguments{};
    compat::u32 item_name_token{};
    compat::u16 item_quantity{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64U> text{};
    compat::u32 text_length{};
};

struct LegacyBattleVictoryItemListPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_item_count{};
    compat::u16 item_count{};
    bool publish_formatted_text{};
    std::array<compat::u8, 64U> formatted_text{};
    compat::u32 formatted_text_length{};
};

class LegacyBattleVictoryItemListPanelPort {
public:
    virtual ~LegacyBattleVictoryItemListPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleVictoryItemListPanelCallReply
    invoke_victory_item_list_panel(
        const LegacyBattleVictoryItemListPanelCallRequest& request
    ) {
        LegacyBattleVictoryItemListPanelCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call ==
            LegacyBattleVictoryItemListPanelCall::format_item_row) {
            reply.eax = 0U;
            reply.publish_formatted_text = true;
        }
        return reply;
    }
};

struct LegacyBattleVictoryItemListPanelRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u8 local_text_seed{};
    compat::u32 local_text_token{};
    LegacyBattleVictoryRewardRegisters action_return{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters list_frame_return{};
};

enum class LegacyBattleVictoryItemListPanelStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    list_frame_typed_stop,
    item_row_typed_stop,
    format_buffer_typed_stop,
};

struct LegacyBattleVictoryItemListPanelResult {
    LegacyBattleVictoryItemListPanelStatus status{
        LegacyBattleVictoryItemListPanelStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 font_size_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 title_draw_calls{};
    compat::u32 query_calls{};
    compat::u32 format_calls{};
    compat::u32 item_draw_calls{};
    compat::u32 item_rows_drawn{};
    compat::u32 initial_item_count{};
    compat::u32 panel_bottom{};
    compat::i32 rectangle_height{};
    compat::i32 list_frame_bottom{};
    compat::u32 stopped_item_index{};
    compat::u32 stopped_text_index{};
    compat::u32 first_frame_resource{};
    compat::u32 second_frame_resource{};
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
    std::vector<LegacyBattleVictoryItemListPanelCall> call_trace;
};

// Typed closure of legacy 0x00469080.
[[nodiscard]] LegacyBattleVictoryItemListPanelResult
draw_legacy_battle_victory_item_list_panel(
    LegacyBattleVictoryItemListPanelBindings bindings,
    LegacyBattleVictoryItemListPanelPort& port,
    const LegacyBattleVictoryItemListPanelRequest& request = {}
);

}  // namespace openswd3::battle
