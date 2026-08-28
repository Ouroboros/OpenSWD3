#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_color_fade.hpp"
#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleTextMessageFrameFontToken =
    0x004AB998U;
inline constexpr compat::u32 kLegacyBattleTextMessageFrameSurfaceToken =
    0x004CD76CU;
inline constexpr compat::u32 kLegacyBattleTextMessageFramePanelAction = 0x233BU;

enum class LegacyBattleTextMessageFrameCall : compat::u8 {
    draw_text,
    release_node,
};

struct LegacyBattleTextMessageFrameCallRequest {
    LegacyBattleTextMessageFrameCall call{
        LegacyBattleTextMessageFrameCall::draw_text
    };
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleTextMessageFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleTextMessageFramePort {
public:
    virtual ~LegacyBattleTextMessageFramePort() = default;

    [[nodiscard]] virtual LegacyBattleTextMessageFrameCallReply
    invoke_text_message_frame(
        const LegacyBattleTextMessageFrameCallRequest& request
    ) = 0;
};

struct LegacyBattleTextMessageFrameBindings {
    LegacyBattleTextMessageState& messages;
    compat::u32& head_token;
    const compat::u32& freeze_gate;  // 0x0053C00C
    asset_runtime::LegacyActionRecord& panel_action_record;
    LegacyBattleColorFadeState& color_fade;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    const rendering::LegacyPixelConversionState& pixel_conversion;
};

struct LegacyBattleTextMessageFrameRequest {
    LegacyBattleTextMessageRegisters entry{};
    LegacyBattleTextMessageRegisters rectangle_return{};
    LegacyBattleTextMessageRegisters tiled_frame_return{};
    std::span<const LegacyBattleTextMessageRegisters> color_fade_returns{};
};

enum class LegacyBattleTextMessageFrameStatus : compat::u8 {
    completed,
    chain_typed_stop,
    rectangle_typed_stop,
    tiled_frame_typed_stop,
    color_fade_typed_stop,
};

struct LegacyBattleTextMessageFrameResult {
    LegacyBattleTextMessageFrameStatus status{
        LegacyBattleTextMessageFrameStatus::completed
    };
    LegacyBattleTextMessageRegisters return_registers{};
    compat::u32 first_pass_nodes{};
    compat::u32 second_pass_nodes{};
    compat::u32 text_calls{};
    compat::u32 release_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 color_fade_calls{};
    compat::u32 port_calls{};
    compat::u32 stopped_chain_token{};
    std::vector<compat::u32> retained_tokens;
    std::vector<compat::u32> released_tokens;
};

// Typed closure of legacy 0x00469960. Draws and advances active battle text
// messages, then performs the original expired-node slide-out and in-place
// unlink/release pass over the same shared singly linked list.
[[nodiscard]] LegacyBattleTextMessageFrameResult
advance_legacy_battle_text_message_frame(
    LegacyBattleTextMessageFrameBindings bindings,
    LegacyBattleTextMessageFramePort& port,
    const LegacyBattleTextMessageFrameRequest& request = {}
);

}  // namespace openswd3::battle
