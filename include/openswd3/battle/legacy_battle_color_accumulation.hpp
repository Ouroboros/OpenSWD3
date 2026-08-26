#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

namespace openswd3::battle {

inline constexpr compat::i32 kLegacyBattleColorAccumulationPixelCount = 0x3C000;

class LegacyBattleColorAccumulationStatePort {
public:
    [[nodiscard]] virtual rendering::LegacyFrameColorTransitionState&
    battle_color_accumulation_state() noexcept {
        return battle_color_accumulation_state_;
    }

    [[nodiscard]] virtual const rendering::LegacyFrameColorTransitionState&
    battle_color_accumulation_state() const noexcept {
        return battle_color_accumulation_state_;
    }

protected:
    LegacyBattleColorAccumulationStatePort() = default;
    ~LegacyBattleColorAccumulationStatePort() = default;

private:
    rendering::LegacyFrameColorTransitionState
        battle_color_accumulation_state_{};
};

[[nodiscard]] rendering::LegacyFrameColorTransitionResult
update_legacy_battle_color_accumulation(
    rendering::LegacyFrameColorTransitionState& state,
    bool decrement_countdown,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& format
) noexcept;

}  // namespace openswd3::battle
