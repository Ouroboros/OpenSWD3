#pragma once

#include <array>
#include <vector>

#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleTextPanelAction = 0x233BU;
inline constexpr compat::u32 kLegacyBattleTextPanelActionRecordToken =
    0x004FC5B0U;
inline constexpr compat::u32 kLegacyBattleTextPanelFontToken = 0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleTextPanelSurfaceToken = 0x004CD76CU;

struct LegacyBattleTextPanelRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

enum class LegacyBattleTextPanelCall : compat::u8 {
    update_action,
    draw_rectangle,
    draw_tiled_frame,
    draw_text,
};

struct LegacyBattleTextPanelCallRequest {
    LegacyBattleTextPanelCall call{LegacyBattleTextPanelCall::update_action};
    std::array<compat::u32, 8U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleTextPanelCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_action_field_4a{};
    compat::u16 action_field_4a{};
};

class LegacyBattleTextPanelPort {
public:
    virtual ~LegacyBattleTextPanelPort() = default;

    [[nodiscard]] virtual LegacyBattleTextPanelCallReply
    invoke_text_panel(const LegacyBattleTextPanelCallRequest& request) {
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }
};

struct LegacyBattleTextPanelRequest {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 text_x{};
    compat::i32 text_y{};
    compat::u32 text_token{};
    LegacyBattleTextPanelRegisters entry{};
};

struct LegacyBattleTextPanelResult {
    LegacyBattleTextPanelRegisters return_registers{};
    LegacyBattleTextPanelRegisters action_entry{};
    LegacyBattleTextPanelRegisters rectangle_entry{};
    LegacyBattleTextPanelRegisters tiled_frame_entry{};
    LegacyBattleTextPanelRegisters text_entry{};
    compat::u32 port_calls{};
    compat::u32 action_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 text_calls{};
    compat::u32 frame_resource{};
    compat::i32 frame_left{};
    compat::i32 frame_top{};
    compat::i32 frame_right{};
    compat::i32 frame_bottom{};
    compat::i32 resolved_text_x{};
    compat::i32 resolved_text_y{};
    bool used_default_text_position{};
    std::vector<LegacyBattleTextPanelCall> call_trace;
};

// Typed closure of legacy 0x00469550. Draws one battle text panel while
// preserving u32 geometry wrapping, low-word resource replacement, the
// zero-pair relative text-position branch, and call-site register chains.
[[nodiscard]] LegacyBattleTextPanelResult draw_legacy_battle_text_panel(
    LegacyBattleVictoryRewardState& shared_state,
    LegacyBattleTextPanelPort& port,
    const LegacyBattleTextPanelRequest& request
);

}  // namespace openswd3::battle
