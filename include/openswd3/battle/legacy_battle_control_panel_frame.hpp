#pragma once

#include "openswd3/battle/legacy_battle_border_panel.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_item_option.hpp"
#include "openswd3/battle/legacy_battle_selection_hint_frame.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleControlPanelResourceId = 0x234EU;
inline constexpr compat::u32 kLegacyBattleControlPanelTextObjectToken =
    0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleControlPanelFontToken = 0x004CD76CU;
inline constexpr compat::u32 kLegacyBattleControlPanelControlTextToken =
    0x004A76B0U;
inline constexpr compat::u32 kLegacyBattleControlPanelAttackTextToken =
    0x004A6BD8U;
inline constexpr compat::u32 kLegacyBattleControlPanelReleaseTextToken =
    0x004A79E8U;
inline constexpr compat::u32 kLegacyBattleControlPanelSharedTextToken =
    0x0053C16CU;
inline constexpr std::array<compat::u8, 4>
    kLegacyBattleControlPanelSpecialPrefix{0xAFU, 0x53U, 0xAEU, 0xEDU};

struct LegacyBattleControlPanelFrameState {
    LegacyBattleBorderPanelState border_panel{};
};

enum class LegacyBattleControlPanelFrameCall : compat::u8 {
    configure_font_reset,
    configure_font_style,
    draw_text,
    reserved_query_primary_option_slot,
    query_special_option,
};

struct LegacyBattleControlPanelFrameCallRequest {
    LegacyBattleControlPanelFrameCall call{
        LegacyBattleControlPanelFrameCall::configure_font_reset
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 text_token{};
    std::array<compat::u8, 24> text_bytes{};
    compat::u32 text_length{};
};

struct LegacyBattleControlPanelFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_text{};
    std::array<compat::u8, 24> text{};
    bool publish_primary_value{};
    compat::u32 primary_value{};
};

class LegacyBattleControlPanelFramePort
    : public virtual LegacyBattleGroupBActionItemOptionPort {
public:
    ~LegacyBattleControlPanelFramePort() override = default;

    [[nodiscard]] virtual LegacyBattleControlPanelFrameCallReply
    invoke_control_panel_frame(
        const LegacyBattleControlPanelFrameCallRequest& request
    ) = 0;

    [[nodiscard]] LegacyBattleGroupBActionItemDefinitionLoadReply
    load_action_item_definition(
        const LegacyBattleGroupBActionItemDefinitionLoadRequest& request
    ) override {
        static_cast<void>(request);
        return {};
    }

    [[nodiscard]] LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const LegacyBattleGroupBActionItemNameCopyRequest& request
    ) override {
        static_cast<void>(request);
        return {};
    }
};

struct LegacyBattleControlPanelFrameBindings {
    LegacyBattleControlPanelFrameState& state;
    LegacyBattleColorFadeState& shared_color_fade;
    compat::u32 alternate_selection_limit{};
    compat::u16 selected_group_b_index{};
    std::span<LegacyBattleActorGroupBElementState> group_b_actors;
    compat::u32& transition_value_a;
    compat::u32& transition_value_b;
    std::span<compat::u32, 6> selection_text_workspace;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleControlPanelRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleControlPanelFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_index{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 local_primary_value_token{};
    std::array<LegacyBattleControlPanelRegisterSnapshot, 2>
        border_return_registers{};
};

enum class LegacyBattleControlPanelFrameStatus : compat::u8 {
    completed,
    title_border_typed_stop,
    body_border_typed_stop,
    group_b_actor_typed_stop,
    primary_option_typed_stop,
};

struct LegacyBattleControlPanelRowTrace {
    compat::u32 selected_index{};
    compat::u32 source_index{};
    compat::u32 text_token{};
    compat::u32 x{};
    compat::u32 y{};
    bool primary{};
    bool selected{};
};

struct LegacyBattleControlPanelFrameResult {
    LegacyBattleControlPanelFrameStatus status{
        LegacyBattleControlPanelFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 border_calls{};
    std::array<LegacyBattleBorderPanelResult, 2> borders{};
    compat::u32 font_reset_calls{};
    compat::u32 font_style_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 primary_query_calls{};
    std::array<LegacyBattleGroupBActionItemOptionResult, 3> primary_options{};
    compat::u32 special_query_calls{};
    compat::u32 primary_rows{};
    compat::u32 special_rows{};
    compat::u32 visible_option_rows{};
    compat::u32 release_selected_index{};
    std::array<LegacyBattleControlPanelRowTrace, 7> rows{};
    compat::u32 row_trace_count{};
    std::array<compat::u8, 24> final_text{};
};

[[nodiscard]] LegacyBattleControlPanelFrameResult
draw_legacy_battle_control_panel_frame(
    LegacyBattleControlPanelFrameBindings bindings,
    LegacyBattleControlPanelFramePort& port,
    const LegacyBattleControlPanelFrameRequest& request
);

}  // namespace openswd3::battle
