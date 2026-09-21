#include "openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorTargetSelectionCountIncrementRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorTargetSelectionCountIncrementRequest request() {
    return {
        .call_address = 0x00456CDDU,
        .return_address = 0x00456CE2U,
        .actor_token = 0x00525508U,
        .entry_eax = 0x12345678U,
        .entry_edx = 0x89ABCDEFU,
        .entry_esp = 0x80001000U,
        .entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = false,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = false,
            .overflow = false,
        },
    };
}

[[nodiscard]] bool flags_equal(
    const LegacyBattleActorCoordinateFlags& left,
    const LegacyBattleActorCoordinateFlags& right
) noexcept {
    return left.carry == right.carry && left.parity == right.parity &&
        left.auxiliary_carry == right.auxiliary_carry &&
        left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
        left.zero == right.zero && left.sign == right.sign &&
        left.overflow == right.overflow;
}

}  // namespace

void test_battle_actor_target_selection_count_increment(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionCountIncrementCallRequests;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionCountIncrementOwners;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionCountIncrementStatus;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountIncrementTrace;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountIncrementView;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        execute_legacy_battle_actor_target_selection_count_increment_call;
    using openswd3::battle::
        increment_legacy_battle_actor_target_selection_count;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountIncrementAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountIncrementCallAddresses;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountIncrementEndAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountIncrementReturnAddresses;
    using openswd3::battle::
        resolve_legacy_battle_actor_target_selection_count_increment;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<
            std::array<LegacyBattleActorGroupBElementState, 8>>();
        action.group_a_action_execution[2U].target_selection_count = 0x1122U;
        (*startup.group_b_lifecycle)[3U]
            .action_execution.target_selection_count = 0x5566U;
        const LegacyBattleActorTargetSelectionCountIncrementOwners owners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a =
            resolve_legacy_battle_actor_target_selection_count_increment(
                owners,
                kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U * kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            resolve_legacy_battle_actor_target_selection_count_increment(
                owners,
                kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U * kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto invalid =
            resolve_legacy_battle_actor_target_selection_count_increment(
                owners, 0xDEADBEEFU
            );
        test.expect_true(
            group_a.target_selection_count ==
                    &action.group_a_action_execution[2U]
                         .target_selection_count &&
                *group_a.target_selection_count == 0x1122U &&
                group_b.target_selection_count ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.target_selection_count &&
                *group_b.target_selection_count == 0x5566U &&
                invalid.target_selection_count == nullptr,
            "target-selection count resolver aliases only canonical Group-A action-execution and Group-B lifecycle owners"
        );
    }

    {
        constexpr std::array<u16, 5> previous_values{
            0U,
            1U,
            0x7FFFU,
            0x8000U,
            0xFFFFU,
        };
        constexpr std::array<u16, 5> expected_values{
            1U,
            2U,
            0x8000U,
            0x8001U,
            0U,
        };
        bool matches = true;
        for (std::size_t index = 0U; index < previous_values.size(); ++index) {
            u16 count = previous_values[index];
            const auto result =
                increment_legacy_battle_actor_target_selection_count(
                    {.target_selection_count = &count}, request()
                );
            matches = matches &&
                result.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        completed &&
                count == expected_values[index] &&
                result.previous_value == previous_values[index] &&
                result.incremented_value == expected_values[index] &&
                result.field_token == 0x00527F7EU && result.field_reads == 1U &&
                result.field_writes == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == 0x00456CE2U &&
                result.return_eax == 0x12345678U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x89ABCDEFU &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x00456CE2U && result.returned &&
                result.flags_known && result.flags.carry;
            if (previous_values[index] == 0x7FFFU) {
                matches = matches && result.flags.overflow &&
                    result.flags.sign && !result.flags.zero &&
                    result.flags.auxiliary_carry && result.flags.parity;
            }
            if (previous_values[index] == 0xFFFFU) {
                matches = matches && !result.flags.overflow &&
                    !result.flags.sign && result.flags.zero &&
                    result.flags.auxiliary_carry && result.flags.parity;
            }
        }
        test.expect_true(
            matches,
            "target-selection count increment preserves registers, CF, plain RET, and u16 wraparound"
        );
    }

    {
        u16 count = 0x1234U;
        auto read_request = request();
        read_request.access.count_readable = false;
        const auto read_stop =
            increment_legacy_battle_actor_target_selection_count(
                {.target_selection_count = &count}, read_request
            );

        auto write_request = request();
        write_request.access.count_writable = false;
        const auto write_stop =
            increment_legacy_battle_actor_target_selection_count(
                {.target_selection_count = &count}, write_request
            );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            increment_legacy_battle_actor_target_selection_count(
                {.target_selection_count = &count}, return_request
            );
        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        count_read_typed_stop &&
                read_stop.field_reads == 0U && read_stop.field_writes == 0U &&
                count == 0x1235U &&
                flags_equal(read_stop.flags, read_request.entry_flags) &&
                write_stop.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        count_write_typed_stop &&
                write_stop.previous_value == 0x1234U &&
                write_stop.field_reads == 1U && write_stop.field_writes == 0U &&
                flags_equal(write_stop.flags, write_request.entry_flags) &&
                return_stop.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        return_address_read_typed_stop &&
                return_stop.previous_value == 0x1234U &&
                return_stop.incremented_value == 0x1235U &&
                return_stop.field_reads == 1U &&
                return_stop.field_writes == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x00478AA7U && !return_stop.returned,
            "target-selection count stops preserve the exact read, write, and RET prefixes"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        LegacyBattleActorTargetSelectionCountIncrementTrace trace;
        LegacyBattleActorTargetSelectionCountIncrementCallRequests requests;
        requests.count = 1U;
        requests.calls[0U].entry_esp = 0x90001000U;
        const bool completed =
            execute_legacy_battle_actor_target_selection_count_increment_call(
                trace,
                requests,
                {.action = &action, .startup = &startup},
                0x00456A1FU,
                0x00456A24U,
                kLegacyBattleActorCoordinatesGroupBBaseToken,
                0x11223344U,
                0x55667788U,
                {.carry = true}
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x00456A1FU &&
                trace.return_addresses[0U] == 0x00456A24U &&
                trace.actor_tokens[0U] ==
                    kLegacyBattleActorCoordinatesGroupBBaseToken &&
                trace.last.return_eax == 0x11223344U &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U &&
                (*startup.group_b_lifecycle)[0U]
                        .action_execution.target_selection_count == 1U,
            "target-selection count call retains physical identity and updates the canonical actor"
        );
    }

    test.expect_true(
        kLegacyBattleActorTargetSelectionCountIncrementAddress == 0x00478AA0U &&
            kLegacyBattleActorTargetSelectionCountIncrementEndAddress ==
                0x00478AA7U &&
            kLegacyBattleActorTargetSelectionCountIncrementCallAddresses ==
                std::array<u32, 3>{
                    0x00456A1FU,
                    0x00456CDDU,
                    0x00456D71U,
                } &&
            kLegacyBattleActorTargetSelectionCountIncrementReturnAddresses ==
                std::array<u32, 3>{
                    0x00456A24U,
                    0x00456CE2U,
                    0x00456D76U,
                },
        "target-selection count increment retains all three physical callers"
    );
}
