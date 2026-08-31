#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleGroupBActionSeventeenFrameCall : compat::u8 {
    play_sample,
    set_sample_pan,
    query_coordinates,
    publish_coordinates,
};

struct LegacyBattleGroupBActionSeventeenFrameCallRequest {
    LegacyBattleGroupBActionSeventeenFrameCall call{
        LegacyBattleGroupBActionSeventeenFrameCall::play_sample
    };
    std::array<compat::u32, 2> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupBActionSeventeenFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 2> outputs{};
};

class LegacyBattleGroupBActionSeventeenFramePort {
public:
    virtual ~LegacyBattleGroupBActionSeventeenFramePort() = default;

    [[nodiscard]] virtual LegacyBattleGroupBActionSeventeenFrameCallReply
    invoke(
        const LegacyBattleGroupBActionSeventeenFrameCallRequest& request
    ) = 0;
};

enum class LegacyBattleGroupBActionSeventeenFrameStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
    blit_typed_stop,
};

struct LegacyBattleGroupBActionSeventeenFrameRequest {
    compat::u32 actor_token{};
    compat::u32 sample_handle_token{0x004AB784U};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionSeventeenFrameResult {
    LegacyBattleGroupBActionSeventeenFrameStatus status{
        LegacyBattleGroupBActionSeventeenFrameStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 coordinate_publish_calls{};
    compat::u32 render_calls{};
    compat::u32 cleared_action_record_dwords{};
    compat::u16 frame_width{};
    compat::u16 frame_height{};
    compat::u32 coordinate_x{};
    compat::u32 coordinate_y{};
    compat::u32 adjusted_coordinate_x{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool return_ecx_known{true};
    bool return_edx_known{true};
};

// Typed closure of legacy 0x004763D0. The actor owns the 0x98-byte action
// record and countdown. The shared object owns the published frame-source
// token. Coordinate calls remain narrow pending battle ports.
[[nodiscard]] LegacyBattleGroupBActionSeventeenFrameResult
advance_legacy_battle_group_b_action_seventeen_frame(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleGroupBActionSeventeenFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    const LegacyBattleGroupBActionSeventeenFrameRequest& request
);

}  // namespace openswd3::battle
