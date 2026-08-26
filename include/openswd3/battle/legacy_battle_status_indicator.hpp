#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

class LegacyBattleBoundedRandomPort {
public:
    virtual ~LegacyBattleBoundedRandomPort() = default;
    [[nodiscard]] virtual compat::u32 random_bounded(compat::u32 bound) = 0;
};

class LegacyBattleIndicatorSoundPort {
public:
    virtual ~LegacyBattleIndicatorSoundPort() = default;
    virtual void
    play_indicator_sound(compat::u16 sound_id, compat::u16 level) = 0;
};

enum class LegacyBattleStatusIndicatorStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleStatusIndicatorState {
    compat::u32 tick_counter{};
    compat::u16 side_state{};
    compat::u16 completed_hold_count{};
    compat::u16 intensity{};
    compat::u16 intensity_countdown{};

    asset_runtime::LegacyActionRecord action_record{};
    bool action_update_attempted{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    compat::u32 frame_resource_id{};
    bool source_published{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

struct LegacyBattleStatusIndicatorResult {
    LegacyBattleStatusIndicatorStatus status{
        LegacyBattleStatusIndicatorStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 action_update_calls{};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 sound_calls{};
    compat::u32 request_flags{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    bool tick_multiple_of_25{};
    bool state_toggled{};
    bool action_record_cleared{};
    compat::u32 return_value{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_450F90: update and draw the pulsing battle status indicator.
[[nodiscard]] LegacyBattleStatusIndicatorResult
advance_legacy_battle_status_indicator(
    LegacyBattleStatusIndicatorState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleIndicatorSoundPort& sound,
    compat::u32 action_update_eax_snapshot
);

}  // namespace openswd3::battle
