#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"

#include <algorithm>
#include <cstddef>

namespace openswd3::battle {
namespace {

constexpr std::size_t kUpperWorkspaceBegin = 0x29U;

}  // namespace

LegacyBattleGroupAWorkspaceResetResult reset_legacy_battle_group_a_workspace(
    LegacyBattleGroupAWorkspaceState& state
) noexcept {
    state.tail_words[7U] = 0U;
    state.tail_words[8U] = 0U;
    state.tail_words[9U] = 0U;
    state.field_2f0c = 0U;
    state.tail_words[0U] = 0U;
    state.tail_words[1U] = 0U;
    state.tail_words[6U] = 0U;
    state.tail_words[5U] = 0U;
    state.tail_words[10U] = 0U;
    state.tail_words[2U] = 0U;
    state.tail_words[4U] = 0U;
    state.tail_words[3U] = 0U;

    std::fill(
        state.late_workspace.begin() + kUpperWorkspaceBegin,
        state.late_workspace.end(),
        0U
    );
    state.early_workspace.fill(0U);
    std::fill(
        state.late_workspace.begin(),
        state.late_workspace.begin() + kUpperWorkspaceBegin,
        0U
    );

    return {
        .explicit_words_zeroed = 11U,
        .explicit_dwords_zeroed = 1U,
        .upper_workspace_dwords_zeroed = 0xBEU,
        .early_workspace_dwords_zeroed = 0x4CU,
        .lower_workspace_dwords_zeroed = 0x29U,
        .return_edx = state.object_token,
    };
}

}  // namespace openswd3::battle
