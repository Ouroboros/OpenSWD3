#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleEffectCallPort;

inline constexpr compat::u32 kLegacyBattleEffectShiftGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleEffectShiftGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleEffectShiftGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleEffectShiftGroupBStride = 0x2B28U;

struct LegacyBattleEffectShiftState {
    compat::u16 phase_word{};
    compat::u16 invocation_counter{};
    compat::u32 direction_mode{};
    compat::u16 accumulated_step{};
    compat::u32 packed_reward{};
    compat::i32 actor_delta{};
    compat::u16 threshold_word{};
    compat::u32 completion_latch{};
};

class LegacyBattleEffectShiftStatePort {
public:
    [[nodiscard]] virtual LegacyBattleEffectShiftState&
    effect_shift_state() noexcept {
        return effect_shift_state_;
    }

    [[nodiscard]] virtual const LegacyBattleEffectShiftState&
    effect_shift_state() const noexcept {
        return effect_shift_state_;
    }

protected:
    LegacyBattleEffectShiftStatePort() = default;
    ~LegacyBattleEffectShiftStatePort() = default;

private:
    LegacyBattleEffectShiftState effect_shift_state_{};
};

enum class LegacyBattleEffectShiftStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
};

struct LegacyBattleEffectShiftResult {
    LegacyBattleEffectShiftStatus status{
        LegacyBattleEffectShiftStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
    compat::u32 group_a_iterations{};
    compat::u32 group_b_iterations{};
    compat::u32 argument_value{};
    compat::u32 scratch_value{};
    bool phase_halved{};
    bool completion_latch_published{};
};

// sub_45BD90.
[[nodiscard]] LegacyBattleEffectShiftResult advance_legacy_battle_effect_shift(
    LegacyBattleEffectCallPort& port,
    compat::u32 argument_value,
    compat::u32 completion_mode,
    compat::u32 entry_ecx,
    compat::u32 entry_edx
);

}  // namespace openswd3::battle
