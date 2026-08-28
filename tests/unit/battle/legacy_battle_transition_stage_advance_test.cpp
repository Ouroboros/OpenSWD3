#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"
#include "test.hpp"

#include <bit>

void test_battle_transition_stage_advance(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleTransitionStageAdvanceStatus;
    using openswd3::battle::advance_legacy_battle_transition_stage;
    using openswd3::compat::i32;
    using openswd3::compat::u32;

    {
        u32 stage = 12U;
        const auto result = advance_legacy_battle_transition_stage(
            stage, {.base_offset = 212U, .target = 252U, .divisor = 3U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTransitionStageAdvanceStatus::completed &&
                result.numerator == 28 && result.quotient == 9 &&
                result.remainder == 1 && result.stage_before == 12U &&
                result.stage_after == 21U && stage == 21U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 1U,
            "positive quotient advances the shared stage and returns zero"
        );
    }

    {
        u32 stage = 80U;
        const auto result = advance_legacy_battle_transition_stage(
            stage, {.base_offset = 212U, .target = 244U, .divisor = 3U}
        );
        test.expect_true(
            result.numerator == -48 && result.quotient == -16 &&
                result.remainder == 0 && stage == 64U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0U,
            "negative numerator uses signed truncation and wrapped stage addition"
        );
    }

    {
        u32 stage = 31U;
        const auto result = advance_legacy_battle_transition_stage(
            stage, {.base_offset = 212U, .target = 244U, .divisor = 3U}
        );
        test.expect_true(
            result.numerator == 1 && result.quotient == 0 &&
                result.remainder == 1 && stage == 31U &&
                result.return_eax == 1U && result.return_ecx == 1U &&
                result.return_edx == 1U,
            "zero quotient preserves stage and returns one in EAX and ECX"
        );
    }

    {
        u32 stage = 0xFFFFFFF0U;
        const auto result = advance_legacy_battle_transition_stage(
            stage,
            {.base_offset = 0xFFFFFFF0U,
             .target = 0x80000000U,
             .divisor = 0xFFFFFFFFU}
        );
        test.expect_true(
            result.numerator == std::bit_cast<i32>(0x80000020U) &&
                result.quotient == std::bit_cast<i32>(0x7FFFFFE0U) &&
                stage == 0x7FFFFFD0U,
            "both subtractions and stage addition retain low thirty-two-bit wrapping"
        );
    }

    {
        u32 stage = 0x12345678U;
        const auto result = advance_legacy_battle_transition_stage(
            stage, {.base_offset = 1U, .target = 0U, .divisor = 0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTransitionStageAdvanceStatus::
                        divide_by_zero_typed_stop &&
                stage == 0x12345678U && result.stage_after == 0x12345678U &&
                result.return_eax == 0xEDCBA987U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0xFFFFFFFFU,
            "divide by zero stops after CDQ and before the shared stage store"
        );
    }

    {
        u32 stage = 0U;
        const auto result = advance_legacy_battle_transition_stage(
            stage,
            {.base_offset = 0U, .target = 0x80000000U, .divisor = 0xFFFFFFFFU}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTransitionStageAdvanceStatus::
                        divide_overflow_typed_stop &&
                stage == 0U && result.return_eax == 0x80000000U &&
                result.return_ecx == 0U && result.return_edx == 0xFFFFFFFFU,
            "INT_MIN divided by minus one stops at IDIV without publishing stage"
        );
    }
}
