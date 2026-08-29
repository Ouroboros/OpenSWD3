#include "openswd3/battle/legacy_battle_actor_list_index_commit.hpp"

#include "test.hpp"

void test_battle_actor_list_index_commit(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorListIndexCommitStatus;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::commit_legacy_battle_actor_list_index;

    {
        const auto result = commit_legacy_battle_actor_list_index(
            nullptr, 0U, {.entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListIndexCommitStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xABCDEF01U && result.writes == 0U,
            "actor list index commit stops at the first next-index read with entry registers intact"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.current_list_index = 0xAAAAAAAAU;
        state.next_list_index = 0x89ABCDEFU;
        const auto result = commit_legacy_battle_actor_list_index(
            &state,
            0x005029D0U,
            {.entry_eax = 0x11111111U, .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListIndexCommitStatus::completed &&
                state.current_list_index == 0x89ABCDEFU &&
                state.next_list_index == 0x89ABCDEFU && result.writes == 1U &&
                result.return_eax == 0x89ABCDEFU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x22222222U,
            "actor list index commit copies the full next dword and returns it in eax"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.current_list_index = 7U;
        state.next_list_index = 7U;
        const auto result =
            commit_legacy_battle_actor_list_index(&state, 0x005029D0U);
        test.expect_true(
            result.return_eax == 7U && state.current_list_index == 7U &&
                result.writes == 1U,
            "equal list indices still execute the original write"
        );
    }
}
