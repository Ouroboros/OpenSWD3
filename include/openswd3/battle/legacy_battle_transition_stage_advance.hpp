#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

enum class LegacyBattleTransitionStageAdvanceStatus : compat::u8 {
    completed,
    divide_by_zero_typed_stop,
    divide_overflow_typed_stop,
};

struct LegacyBattleTransitionStageAdvanceRequest {
    compat::u32 base_offset{};
    compat::u32 target{};
    compat::u32 divisor{};
};

struct LegacyBattleTransitionStageAdvanceResult {
    LegacyBattleTransitionStageAdvanceStatus status{
        LegacyBattleTransitionStageAdvanceStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::i32 numerator{};
    compat::i32 quotient{};
    compat::i32 remainder{};
    compat::u32 stage_before{};
    compat::u32 stage_after{};
};

// Assembly-exact closure of legacy 0x00469620. Advances the shared transition
// stage by signed((target - stage - base_offset) / divisor) and returns one
// only when the quotient is zero.
[[nodiscard]] LegacyBattleTransitionStageAdvanceResult
advance_legacy_battle_transition_stage(
    compat::u32& transition_stage,
    const LegacyBattleTransitionStageAdvanceRequest& request
) noexcept;

}  // namespace openswd3::battle
