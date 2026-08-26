#pragma once

#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

namespace openswd3::battle {

struct LegacyBattleFullFrameDarkeningState {
    compat::i32 channel_delta{};
};

enum class LegacyBattleFullFrameDarkeningStatus : compat::u8 {
    completed,
    red_typed_stop,
    green_typed_stop,
    blue_typed_stop,
};

struct LegacyBattleFullFrameDarkeningResult {
    LegacyBattleFullFrameDarkeningStatus status{
        LegacyBattleFullFrameDarkeningStatus::completed
    };
    rendering::LegacyFrameColorStatus red_status{
        rendering::LegacyFrameColorStatus::completed
    };
    rendering::LegacyFrameColorStatus green_status{
        rendering::LegacyFrameColorStatus::completed
    };
    rendering::LegacyFrameColorStatus blue_status{
        rendering::LegacyFrameColorStatus::completed
    };
    compat::u32 channel_calls{};
    compat::i32 applied_delta{};
    compat::i32 decremented_delta{};
    bool clamped_to_zero{};
    compat::u32 return_value{};
};

// sub_45BD10.
[[nodiscard]] LegacyBattleFullFrameDarkeningResult
update_legacy_battle_full_frame_darkening(
    LegacyBattleFullFrameDarkeningState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitEffectState& shared_effects
) noexcept;

}  // namespace openswd3::battle
