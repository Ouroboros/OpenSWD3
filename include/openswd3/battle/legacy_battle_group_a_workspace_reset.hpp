#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupAWorkspaceState {
    compat::u32 object_token{};
    // actor + 0x0AF0 .. +0x0C1F
    std::array<compat::u32, 0x4C> early_workspace{};
    // actor + 0x2B24 .. +0x2EBF; the upper 0xBE dwords are cleared first.
    std::array<compat::u32, 0xE7> late_workspace{};
    compat::u32 field_2f0c{};
    compat::u16 untouched_field_2f0e{};
    // actor + 0x2F10 .. +0x2F24
    std::array<compat::u16, 11> tail_words{};
    compat::u16 untouched_field_2f26{};
};

struct LegacyBattleGroupAWorkspaceResetResult {
    compat::u32 explicit_words_zeroed{};
    compat::u32 explicit_dwords_zeroed{};
    compat::u32 upper_workspace_dwords_zeroed{};
    compat::u32 early_workspace_dwords_zeroed{};
    compat::u32 lower_workspace_dwords_zeroed{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E6A0.
[[nodiscard]] LegacyBattleGroupAWorkspaceResetResult
reset_legacy_battle_group_a_workspace(
    LegacyBattleGroupAWorkspaceState& state
) noexcept;

}  // namespace openswd3::battle
