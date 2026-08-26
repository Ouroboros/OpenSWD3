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

    [[nodiscard]] virtual compat::u32&
    battle_color_initialization_gate() noexcept {
        return battle_color_initialization_gate_;
    }

    [[nodiscard]] virtual const compat::u32&
    battle_color_initialization_gate() const noexcept {
        return battle_color_initialization_gate_;
    }

protected:
    LegacyBattleColorAccumulationStatePort() = default;
    ~LegacyBattleColorAccumulationStatePort() = default;

private:
    rendering::LegacyFrameColorTransitionState
        battle_color_accumulation_state_{};
    compat::u32 battle_color_initialization_gate_{};
};

struct LegacyBattleColorInitializationRequest {
    compat::i32 current_red{};
    compat::i32 current_green{};
    compat::i32 current_blue{};
    compat::i32 target_red{};
    compat::i32 target_green{};
    compat::i32 target_blue{};
    compat::i32 countdown{};
};

struct LegacyBattleColorInitializationResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

[[nodiscard]] LegacyBattleColorInitializationResult
initialize_legacy_battle_color_accumulation(
    rendering::LegacyFrameColorTransitionState& state,
    const LegacyBattleColorInitializationRequest& request
) noexcept;

[[nodiscard]] rendering::LegacyFrameColorTransitionResult
update_legacy_battle_color_accumulation(
    rendering::LegacyFrameColorTransitionState& state,
    bool decrement_countdown,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& format
) noexcept;

}  // namespace openswd3::battle
