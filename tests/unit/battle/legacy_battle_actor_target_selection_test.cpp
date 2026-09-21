#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_target_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorTargetSelectionAccessKind;
    using openswd3::battle::LegacyBattleActorTargetSelectionOwners;
    using openswd3::battle::LegacyBattleActorTargetSelectionRequest;
    using openswd3::battle::LegacyBattleActorTargetSelectionRequestList;
    using openswd3::battle::LegacyBattleActorTargetSelectionStatus;
    using openswd3::battle::LegacyBattleActorTargetSelectionTrace;
    using openswd3::battle::LegacyBattleActorTargetSelectionView;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::apply_legacy_battle_actor_target_selection;
    using openswd3::battle::execute_legacy_battle_actor_target_selection_call;
    using openswd3::battle::kLegacyBattleActorTargetSelectionAddress;
    using openswd3::battle::kLegacyBattleActorTargetSelectionCallAddresses;
    using openswd3::battle::kLegacyBattleActorTargetSelectionEndAddress;
    using openswd3::battle::kLegacyBattleActorTargetSelectionReturnAddresses;
    using openswd3::battle::resolve_legacy_battle_actor_target_selection;
    using openswd3::compat::u16;
    using openswd3::compat::u32;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.group_a_action_execution[2].idle_state_latch = 7U;
        action.group_a_action_execution[2].action_target = 0x1111U;
        startup.party[2].progress.progress = 0xCAFE2222U;
        const auto view = resolve_legacy_battle_actor_target_selection(
            {.action = &action, .startup = &startup},
            0x005029D0U +
                2U * openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride
        );
        *view.idle_state_latch = 9U;
        *view.action_target = 0x3333U;
        *view.progress = 0xA5A54444U;
        test.expect_true(
            action.group_a_action_execution[2].idle_state_latch == 9U &&
                action.group_a_action_execution[2].action_target == 0x3333U &&
                startup.party[2].progress.progress == 0xA5A54444U,
            "actor target selection resolves the shared group A owners"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        auto& execution = (*startup.group_b_lifecycle)[3].action_execution;
        execution.idle_state_latch = 7U;
        execution.action_target = 0x1111U;
        startup.enemies[3].progress.progress = 0xCAFE2222U;
        const auto view = resolve_legacy_battle_actor_target_selection(
            {.action = &action, .startup = &startup},
            0x00525508U +
                3U * openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride
        );
        *view.idle_state_latch = 9U;
        *view.action_target = 0x3333U;
        *view.progress = 0xA5A54444U;
        test.expect_true(
            execution.idle_state_latch == 9U &&
                execution.action_target == 0x3333U &&
                startup.enemies[3].progress.progress == 0xA5A54444U,
            "actor target selection resolves the shared group B owners"
        );
    }

    {
        constexpr std::array<u16, 5> values{
            0U,
            1U,
            0x7FFFU,
            0x8000U,
            0xFFFFU,
        };
        bool matches = true;
        for (const u16 value : values) {
            u32 idle_state = 7U;
            u16 action_target = 0x2222U;
            u32 progress = 0xCAFE5678U;
            const LegacyBattleActorTargetSelectionRequest request{
                .call_address = 0x00454B8DU,
                .return_address = 0x00454B92U,
                .actor_token = 0x005029D0U,
                .argument_value = value,
                .entry_eax = 0xABCD1234U,
                .entry_edx = 0xA5A55A5AU,
                .entry_esp = 0x0012FFF0U,
                .entry_flags = {.carry = true, .zero = true},
                .entry_flags_known = true,
            };
            const auto result = apply_legacy_battle_actor_target_selection(
                {
                    .idle_state_latch = &idle_state,
                    .action_target = &action_target,
                    .progress = &progress,
                },
                request
            );
            matches = matches &&
                result.status ==
                    LegacyBattleActorTargetSelectionStatus::completed &&
                idle_state == 1U && action_target == value &&
                progress == 0xCAFE0000U &&
                result.return_eax == (0xABCD0000U | value) &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xA5A55A5AU &&
                result.return_esp == 0x0012FFF8U &&
                result.return_eip == 0x00454B92U && result.returned &&
                result.flags_known && result.flags.carry && result.flags.zero &&
                result.access_count == 5U &&
                result.accesses[0].kind ==
                    LegacyBattleActorTargetSelectionAccessKind::argument_read &&
                result.accesses[0].memory_address == 0x0012FFF4U &&
                result.accesses[1].memory_address == 0x00505484U &&
                result.accesses[2].memory_address == 0x00505372U &&
                result.accesses[3].memory_address == 0x005053E2U &&
                result.accesses[4].memory_address == 0x0012FFF0U;
        }
        test.expect_true(
            matches,
            "actor target selection preserves width boundaries write order registers flags and callee cleanup"
        );
    }

    {
        bool matches = true;
        for (std::size_t stop_ordinal = 0U; stop_ordinal < 5U; ++stop_ordinal) {
            u32 idle_state = 7U;
            u16 action_target = 0x2222U;
            u32 progress = 0xCAFE5678U;
            LegacyBattleActorTargetSelectionRequest request{
                .return_address = 0x00454B92U,
                .actor_token = 0x005029D0U,
                .argument_value = 0x3456U,
                .entry_eax = 0xABCD1234U,
                .entry_edx = 0xA5A55A5AU,
                .entry_esp = 0xFFFFFFFCU,
            };
            if (stop_ordinal == 0U) {
                request.argument_read_accessible = false;
            } else if (stop_ordinal == 1U) {
                request.idle_state_write_accessible = false;
            } else if (stop_ordinal == 2U) {
                request.action_target_write_accessible = false;
            } else if (stop_ordinal == 3U) {
                request.progress_write_accessible = false;
            } else {
                request.return_address_read_accessible = false;
            }

            const auto result = apply_legacy_battle_actor_target_selection(
                {
                    .idle_state_latch = &idle_state,
                    .action_target = &action_target,
                    .progress = &progress,
                },
                request
            );
            constexpr std::array statuses{
                LegacyBattleActorTargetSelectionStatus::
                    argument_read_typed_stop,
                LegacyBattleActorTargetSelectionStatus::
                    idle_state_write_typed_stop,
                LegacyBattleActorTargetSelectionStatus::
                    action_target_write_typed_stop,
                LegacyBattleActorTargetSelectionStatus::
                    progress_write_typed_stop,
                LegacyBattleActorTargetSelectionStatus::
                    return_address_read_typed_stop,
            };
            constexpr std::array<u32, 5> stop_addresses{
                0x00478A70U,
                0x00478A75U,
                0x00478A7FU,
                0x00478A86U,
                0x00478A8FU,
            };
            matches = matches && result.status == statuses[stop_ordinal] &&
                result.stop_instruction == stop_addresses[stop_ordinal] &&
                result.return_eip == stop_addresses[stop_ordinal] &&
                !result.returned && result.access_count == stop_ordinal &&
                idle_state == (stop_ordinal >= 2U ? 1U : 7U) &&
                action_target == (stop_ordinal >= 3U ? 0x3456U : 0x2222U) &&
                progress == (stop_ordinal >= 4U ? 0xCAFE0000U : 0xCAFE5678U) &&
                result.return_eax ==
                    (stop_ordinal == 0U ? 0xABCD1234U : 0xABCD3456U) &&
                result.return_esp == 0xFFFFFFFCU;
        }
        test.expect_true(
            matches,
            "actor target selection exposes every typed stop with exact prefix commit"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        LegacyBattleActorTargetSelectionTrace trace;
        LegacyBattleActorTargetSelectionRequestList requests;
        requests.count = 1U;
        requests.calls[0].entry_eax = 0xABCD1234U;
        requests.calls[0].entry_edx = 0xA5A55A5AU;
        requests.calls[0].entry_esp = 0x0012FFF0U;
        const bool completed =
            execute_legacy_battle_actor_target_selection_call(
                trace,
                requests,
                {.action = &action, .startup = &startup},
                0x00454B8DU,
                0x00454B92U,
                0x005029D0U,
                0x4567U
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0] == 0x00454B8DU &&
                trace.return_addresses[0] == 0x00454B92U &&
                trace.actor_tokens[0] == 0x005029D0U &&
                trace.argument_values[0] == 0x4567U &&
                trace.last.return_eax == 0x00004567U &&
                action.group_a_action_execution[0].idle_state_latch == 1U &&
                action.group_a_action_execution[0].action_target == 0x4567U &&
                startup.party[0].progress.progress == 0U,
            "actor target selection call preserves physical identity and updates canonical owners"
        );
    }

    test.expect_true(
        kLegacyBattleActorTargetSelectionAddress == 0x00478A70U &&
            kLegacyBattleActorTargetSelectionEndAddress == 0x00478A91U &&
            kLegacyBattleActorTargetSelectionCallAddresses.size() == 20U &&
            kLegacyBattleActorTargetSelectionReturnAddresses.size() == 20U &&
            kLegacyBattleActorTargetSelectionCallAddresses.front() ==
                0x00454B8DU &&
            kLegacyBattleActorTargetSelectionCallAddresses.back() ==
                0x0046DC9CU &&
            kLegacyBattleActorTargetSelectionReturnAddresses.front() ==
                0x00454B92U &&
            kLegacyBattleActorTargetSelectionReturnAddresses.back() ==
                0x0046DCA1U,
        "actor target selection retains the complete physical caller set"
    );
}
