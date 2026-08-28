#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleListFrameCall : compat::u8 {
    configure_font_style,
};

struct LegacyBattleListFrameCallRequest {
    LegacyBattleListFrameCall call{
        LegacyBattleListFrameCall::configure_font_style
    };
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleListFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleListFramePort
    : public virtual LegacyBattleOffsetActionFrameDrawStatePort {
public:
    virtual ~LegacyBattleListFramePort() = default;

    [[nodiscard]] virtual LegacyBattleListFrameCallReply
    invoke_list_frame(const LegacyBattleListFrameCallRequest& request) = 0;
};

struct LegacyBattleListFrameBindings {
    LegacyBattleInputDispatchState& input;
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleListFrameRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleListFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    std::array<compat::u32, 4> action_update_edx_snapshots{};
    std::array<LegacyBattleListFrameRegisterSnapshot, 4>
        action_frame_return_registers{};
    LegacyBattleListFrameRegisterSnapshot rectangle_return_registers{};
    LegacyBattleListFrameRegisterSnapshot tiled_frame_return_registers{};
};

enum class LegacyBattleListFrameStatus : compat::u8 {
    completed,
    action_frame_typed_stop,
    rectangle_typed_stop,
    tiled_frame_typed_stop,
};

struct LegacyBattleListFrameResult {
    LegacyBattleListFrameStatus status{LegacyBattleListFrameStatus::completed};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 action_frame_calls{};
    compat::u32 font_style_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    std::array<LegacyBattleOffsetActionFrameDrawResult, 4> action_frames{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    rendering::LegacyTiledFrameResult tiled_frame{};
};

// Typed closure of legacy 0x00465480.
[[nodiscard]] LegacyBattleListFrameResult draw_legacy_battle_list_frame(
    LegacyBattleListFrameBindings bindings,
    LegacyBattleListFramePort& port,
    const LegacyBattleListFrameRequest& request
);

}  // namespace openswd3::battle
