#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleTransitionControlTableToken =
    0x0052022CU;
inline constexpr compat::u32 kLegacyBattleTransitionControlTableEndToken =
    0x0052027CU;
inline constexpr compat::u32 kLegacyBattleTransitionControlRows = 4U;
inline constexpr compat::u32 kLegacyBattleTransitionControlColumns = 10U;

struct LegacyBattleTransitionControlSelectionBindings {
    LegacyBattleStartupResetBlocks& reset;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
};

struct LegacyBattleTransitionControlSelectionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleTransitionControlSelectionResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 rows_scanned{};
    compat::u32 zero_words_scanned{};
    compat::u32 selected_flat_index{0xFFFFFFFFU};
    compat::u16 selected_row{};
    compat::u16 selected_value{};
    bool active_control_short_circuit{};
    bool selected{};
};

// Typed closure of legacy 0x004694E0.
[[nodiscard]] LegacyBattleTransitionControlSelectionResult
select_legacy_battle_transition_control(
    LegacyBattleTransitionControlSelectionBindings bindings,
    const LegacyBattleTransitionControlSelectionRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
