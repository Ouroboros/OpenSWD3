#include "openswd3/battle/legacy_battle_transition_control_selection.hpp"

#include "test.hpp"

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;

void set_word(
    openswd3::battle::LegacyBattleStartupResetBlocks& reset,
    const u32 index,
    const u16 value
) {
    u32& packed = reset.block_52022c[index / 2U];
    if ((index & 1U) == 0U) {
        packed = (packed & 0xFFFF0000U) | value;
    } else {
        packed = (packed & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
    }
}

[[nodiscard]] u16 word(
    const openswd3::battle::LegacyBattleStartupResetBlocks& reset,
    const u32 index
) {
    const u32 packed = reset.block_52022c[index / 2U];
    return static_cast<u16>((index & 1U) == 0U ? packed : (packed >> 16U));
}

}  // namespace

void test_battle_transition_control_selection(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleTransitionControlTableEndToken;
    using openswd3::battle::kLegacyBattleTransitionControlTableToken;
    using openswd3::battle::select_legacy_battle_transition_control;

    {
        openswd3::battle::LegacyBattleStartupResetBlocks reset;
        openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
        set_word(reset, 0U, 0x1234U);
        target.transition_control_words = 0xCAFE5678U;
        const auto result = select_legacy_battle_transition_control(
            {.reset = reset, .target_selection = target},
            {.entry_eax = 0x11111111U,
             .entry_ecx = 0x22222222U,
             .entry_edx = 0x33333333U}
        );
        test.expect_true(
            result.active_control_short_circuit && !result.selected &&
                result.rows_scanned == 0U && result.zero_words_scanned == 0U &&
                result.return_eax == 0x11111111U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U &&
                target.transition_control_words == 0xCAFE5678U &&
                word(reset, 0U) == 0x1234U,
            "transition control selection preserves every register and table word when the active high word is nonzero"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupResetBlocks reset;
        openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
        reset.block_52022c[0U] = 0xABCD1234U;
        target.transition_control_words = 0x00005678U;
        const auto result = select_legacy_battle_transition_control(
            {.reset = reset, .target_selection = target}
        );
        test.expect_true(
            result.selected && result.selected_flat_index == 0U &&
                result.selected_row == 0U && result.selected_value == 0x1234U &&
                result.rows_scanned == 1U && result.zero_words_scanned == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x00521234U &&
                result.return_edx == kLegacyBattleTransitionControlTableToken &&
                target.transition_control_words == 0x12340000U &&
                reset.block_52022c[0U] == 0xABCD0000U,
            "transition control selection consumes the first low word, preserves its neighbor and publishes row zero"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupResetBlocks reset;
        openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
        reset.block_52022c[11U] = 0xBEEF0000U;
        target.transition_control_words = 0x00007777U;
        const auto result = select_legacy_battle_transition_control(
            {.reset = reset, .target_selection = target},
            {.entry_eax = 0xFFFFFFFFU,
             .entry_ecx = 0xEEEEEEEEU,
             .entry_edx = 0xDDDDDDDDU}
        );
        test.expect_true(
            result.selected && result.selected_flat_index == 23U &&
                result.selected_row == 2U && result.selected_value == 0xBEEFU &&
                result.rows_scanned == 3U && result.zero_words_scanned == 23U &&
                result.return_eax == 23U && result.return_ecx == 0x0052BEEFU &&
                result.return_edx ==
                    kLegacyBattleTransitionControlTableToken + 40U &&
                target.transition_control_words == 0xBEEF0002U &&
                reset.block_52022c[11U] == 0U,
            "transition control selection uses row-major flat indexing and clears the selected high half-word"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupResetBlocks reset;
        openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
        target.transition_control_words = 0x00007777U;
        const auto result = select_legacy_battle_transition_control(
            {.reset = reset, .target_selection = target}
        );
        test.expect_true(
            !result.selected && !result.active_control_short_circuit &&
                result.rows_scanned == 4U && result.zero_words_scanned == 40U &&
                result.return_eax == 10U &&
                result.return_ecx ==
                    kLegacyBattleTransitionControlTableEndToken &&
                result.return_edx ==
                    kLegacyBattleTransitionControlTableEndToken &&
                target.transition_control_words == 0x00007777U,
            "transition control selection scans all forty zeros and preserves the stale low word when no value exists"
        );
    }
}
