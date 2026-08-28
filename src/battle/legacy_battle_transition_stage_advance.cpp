#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"

#include <bit>
#include <limits>

namespace openswd3::battle {
namespace {

[[nodiscard]] constexpr compat::i32 signed_bits(const compat::u32 value) {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::u32 bits(const compat::i32 value) {
    return std::bit_cast<compat::u32>(value);
}

}  // namespace

LegacyBattleTransitionStageAdvanceResult advance_legacy_battle_transition_stage(
    compat::u32& transition_stage,
    const LegacyBattleTransitionStageAdvanceRequest& request
) noexcept {
    LegacyBattleTransitionStageAdvanceResult result{};
    result.stage_before = transition_stage;
    const compat::u32 numerator_bits =
        request.target - transition_stage - request.base_offset;
    const compat::i32 numerator = signed_bits(numerator_bits);
    const compat::i32 divisor = signed_bits(request.divisor);
    result.numerator = numerator;
    result.return_eax = numerator_bits;
    result.return_ecx = transition_stage;
    result.return_edx = numerator < 0 ? 0xFFFFFFFFU : 0U;
    result.stage_after = transition_stage;

    if (divisor == 0) {
        result.status =
            LegacyBattleTransitionStageAdvanceStatus::divide_by_zero_typed_stop;
        return result;
    }
    if (numerator == std::numeric_limits<compat::i32>::min() && divisor == -1) {
        result.status = LegacyBattleTransitionStageAdvanceStatus::
            divide_overflow_typed_stop;
        return result;
    }

    result.quotient = numerator / divisor;
    result.remainder = numerator % divisor;
    transition_stage += bits(result.quotient);
    result.stage_after = transition_stage;
    const compat::u32 completed = result.quotient == 0 ? 1U : 0U;
    result.return_eax = completed;
    result.return_ecx = completed;
    result.return_edx = bits(result.remainder);
    return result;
}

}  // namespace openswd3::battle
