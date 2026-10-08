#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleFrameEffectControlState {
    compat::u32 primary_suppression{};
    compat::u32 secondary_suppression{};
    compat::i16 red_factor{};
    compat::i16 green_factor{};
    compat::i16 blue_factor{};
};

class LegacyBattleFrameEffectControlStatePort {
public:
    [[nodiscard]] virtual LegacyBattleFrameEffectControlState&
    frame_effect_control_state() noexcept {
        return frame_effect_control_state_;
    }

    [[nodiscard]] virtual const LegacyBattleFrameEffectControlState&
    frame_effect_control_state() const noexcept {
        return frame_effect_control_state_;
    }

protected:
    LegacyBattleFrameEffectControlStatePort() = default;
    ~LegacyBattleFrameEffectControlStatePort() = default;

private:
    LegacyBattleFrameEffectControlState frame_effect_control_state_{};
};

}  // namespace openswd3::battle
