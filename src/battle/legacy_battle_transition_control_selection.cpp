#include "openswd3/battle/legacy_battle_transition_control_selection.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] constexpr u32 pack_words(const u16 low, const u16 high) noexcept {
    return static_cast<u32>(low) | (static_cast<u32>(high) << 16U);
}

[[nodiscard]] u16 table_word(
    const LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    const u32 packed = reset.block_52022c[index / 2U];
    return (index & 1U) == 0U ? low_word(packed) : high_word(packed);
}

void clear_table_word(
    LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    u32& packed = reset.block_52022c[index / 2U];
    if ((index & 1U) == 0U) {
        packed &= 0xFFFF0000U;
    } else {
        packed &= 0x0000FFFFU;
    }
}

}  // namespace

LegacyBattleTransitionControlSelectionResult
select_legacy_battle_transition_control(
    const LegacyBattleTransitionControlSelectionBindings bindings,
    const LegacyBattleTransitionControlSelectionRequest& request
) noexcept {
    LegacyBattleTransitionControlSelectionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };

    const u32 entry_control =
        bindings.target_selection.transition_control_words;
    if (high_word(entry_control) != 0U) {
        result.active_control_short_circuit = true;
        return result;
    }

    u32 row = 0U;
    u32 row_offset = 0U;
    u32 row_token = kLegacyBattleTransitionControlTableToken;
    while (row_token < kLegacyBattleTransitionControlTableEndToken) {
        u32 column = 0U;
        u32 word_token = row_token;
        while (column < kLegacyBattleTransitionControlColumns &&
               table_word(bindings.reset, row_offset + column) == 0U) {
            ++result.zero_words_scanned;
            ++column;
            word_token += 2U;
        }

        result.return_eax = column;
        result.return_ecx = word_token;
        result.return_edx = row_token;
        if (column < kLegacyBattleTransitionControlColumns) {
            const u32 flat_index = row_offset + column;
            result.return_eax = flat_index;
            bindings.target_selection.transition_control_words =
                pack_words(static_cast<u16>(row), high_word(entry_control));
            const u16 selected = table_word(bindings.reset, flat_index);
            clear_table_word(bindings.reset, flat_index);
            result.return_ecx = replace_low_word(word_token, selected);
            bindings.target_selection.transition_control_words = pack_words(
                low_word(bindings.target_selection.transition_control_words),
                selected
            );
            result.selected_flat_index = flat_index;
            result.selected_row = static_cast<u16>(row);
            result.selected_value = selected;
            result.selected = selected != 0U;
            if (selected != 0U) {
                result.rows_scanned = row + 1U;
                return result;
            }
        }

        row_token += kLegacyBattleTransitionControlColumns * 2U;
        ++row;
        row_offset += kLegacyBattleTransitionControlColumns;
    }

    result.rows_scanned = row;
    result.return_edx = row_token;
    return result;
}

}  // namespace openswd3::battle
