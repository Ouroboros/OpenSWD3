#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <ranges>

void test_battle_group_a_workspace_reset(openswd3::test::Context& test) {
    openswd3::battle::LegacyBattleGroupAWorkspaceState state{
        .object_token = 0x005029D0U,
        .field_2f0c = 0xA5A5A5A5U,
        .untouched_field_2f0e = 0x1357U,
        .untouched_field_2f26 = 0x2468U,
    };
    state.early_workspace.fill(0x11111111U);
    state.late_workspace.fill(0x22222222U);
    state.tail_words.fill(0x3333U);

    const auto result =
        openswd3::battle::reset_legacy_battle_group_a_workspace(state);

    test.expect_true(
        std::ranges::all_of(
            state.early_workspace, [](const auto value) { return value == 0U; }
        ) &&
            std::ranges::all_of(
                state.late_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                state.tail_words, [](const auto value) { return value == 0U; }
            ) &&
            state.field_2f0c == 0U && state.untouched_field_2f0e == 0x1357U &&
            state.untouched_field_2f26 == 0x2468U &&
            result.explicit_words_zeroed == 11U &&
            result.explicit_dwords_zeroed == 1U &&
            result.upper_workspace_dwords_zeroed == 0xBEU &&
            result.early_workspace_dwords_zeroed == 0x4CU &&
            result.lower_workspace_dwords_zeroed == 0x29U &&
            result.return_eax == 0U && result.return_ecx == 0U &&
            result.return_edx == 0x005029D0U,
        "group-A workspace reset clears exact ranges and preserves skipped adjacent words"
    );

    openswd3::battle::LegacyBattleStartupState startup;
    auto& owned = startup.party[2U].workspace;
    owned.object_token = 0x00508838U;
    owned.early_workspace.fill(0xAAAAAAAAU);
    owned.late_workspace.fill(0xBBBBBBBBU);
    owned.tail_words.fill(0xCCCCU);
    const auto owned_result =
        openswd3::battle::reset_legacy_battle_group_a_workspace(owned);
    test.expect_true(
        owned_result.return_edx == owned.object_token &&
            std::ranges::all_of(
                startup.party[2U].workspace.early_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                startup.party[2U].workspace.late_workspace,
                [](const auto value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                startup.party[2U].workspace.tail_words,
                [](const auto value) { return value == 0U; }
            ),
        "workspace reset mutates the startup party actor's unique physical view"
    );
}
