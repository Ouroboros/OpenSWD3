#include "openswd3/battle/legacy_battle_actor_start_gate_increment.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorStartGateIncrementRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorStartGateIncrementRequest request() {
    return {
        .call_address = 0x0045786BU,
        .return_address = 0x00457870U,
        .actor_token = 0x005029D0U,
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

void test_battle_actor_start_gate_increment(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorStartGateIncrementCallRequests;
    using openswd3::battle::LegacyBattleActorStartGateIncrementOwners;
    using openswd3::battle::LegacyBattleActorStartGateIncrementStatus;
    using openswd3::battle::LegacyBattleActorStartGateIncrementTrace;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        execute_legacy_battle_actor_start_gate_increment_call;
    using openswd3::battle::increment_legacy_battle_actor_start_gate;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::kLegacyBattleActorStartGateIncrementAddress;
    using openswd3::battle::kLegacyBattleActorStartGateIncrementCallAddresses;
    using openswd3::battle::kLegacyBattleActorStartGateIncrementEndAddress;
    using openswd3::battle::kLegacyBattleActorStartGateIncrementReturnAddresses;
    using openswd3::battle::resolve_legacy_battle_actor_start_gate_increment;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<
            std::array<LegacyBattleActorGroupBElementState, 8>>();
        action.group_a_action_execution[2U].start_gate = 0x1122U;
        action.group_a_action_execution[2U].start_gate_latch = 0x33445566U;
        (*startup.group_b_lifecycle)[3U].action_execution.start_gate = 0x7788U;
        (*startup.group_b_lifecycle)[3U].action_execution.start_gate_latch =
            0x99AABBCCU;
        const LegacyBattleActorStartGateIncrementOwners owners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a = resolve_legacy_battle_actor_start_gate_increment(
            owners,
            kLegacyBattleActorCoordinatesGroupABaseToken +
                2U * kLegacyBattleActorCoordinatesGroupAStride
        );
        const auto group_b = resolve_legacy_battle_actor_start_gate_increment(
            owners,
            kLegacyBattleActorCoordinatesGroupBBaseToken +
                3U * kLegacyBattleActorCoordinatesGroupBStride
        );
        const auto invalid = resolve_legacy_battle_actor_start_gate_increment(
            owners, 0xDEADBEEFU
        );
        test.expect_true(
            group_a.start_gate ==
                    &action.group_a_action_execution[2U].start_gate &&
                group_a.start_gate_latch ==
                    &action.group_a_action_execution[2U].start_gate_latch &&
                group_b.start_gate ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate &&
                group_b.start_gate_latch ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate_latch &&
                invalid.start_gate == nullptr &&
                invalid.start_gate_latch == nullptr,
            "start-gate increment resolver aliases only canonical Group-A and Group-B action-execution owners"
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
            u16 start_gate = previous_values[index];
            u32 start_gate_latch = 0xFFFFFFFFU;
            const auto result = increment_legacy_battle_actor_start_gate(
                {
                    .start_gate = &start_gate,
                    .start_gate_latch = &start_gate_latch,
                },
                request()
            );
            matches = matches &&
                result.status ==
                    LegacyBattleActorStartGateIncrementStatus::completed &&
                start_gate == expected_values[index] &&
                start_gate_latch == 1U &&
                result.previous_start_gate == previous_values[index] &&
                result.incremented_start_gate == expected_values[index] &&
                result.start_gate_field_token == 0x00505444U &&
                result.start_gate_latch_field_token == 0x005054B0U &&
                result.start_gate_reads == 1U &&
                result.start_gate_writes == 1U &&
                result.start_gate_latch_writes == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == 0x00457870U &&
                result.return_eax == 0x12345678U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x89ABCDEFU &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x00457870U && result.returned &&
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
            "start-gate increment preserves registers, CF, latch publication, plain RET, and u16 wraparound"
        );
    }

    {
        u16 start_gate = 0x1234U;
        u32 start_gate_latch = 0xCAFEBABEU;
        auto read_request = request();
        read_request.access.start_gate_readable = false;
        const auto read_stop = increment_legacy_battle_actor_start_gate(
            {
                .start_gate = &start_gate,
                .start_gate_latch = &start_gate_latch,
            },
            read_request
        );

        auto write_request = request();
        write_request.access.start_gate_writable = false;
        const auto write_stop = increment_legacy_battle_actor_start_gate(
            {
                .start_gate = &start_gate,
                .start_gate_latch = &start_gate_latch,
            },
            write_request
        );

        auto latch_request = request();
        latch_request.access.start_gate_latch_writable = false;
        const auto latch_stop = increment_legacy_battle_actor_start_gate(
            {
                .start_gate = &start_gate,
                .start_gate_latch = &start_gate_latch,
            },
            latch_request
        );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop = increment_legacy_battle_actor_start_gate(
            {
                .start_gate = &start_gate,
                .start_gate_latch = &start_gate_latch,
            },
            return_request
        );

        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorStartGateIncrementStatus::
                        start_gate_read_typed_stop &&
                read_stop.start_gate_reads == 0U &&
                read_stop.start_gate_writes == 0U &&
                flags_equal(read_stop.flags, read_request.entry_flags) &&
                write_stop.status ==
                    LegacyBattleActorStartGateIncrementStatus::
                        start_gate_write_typed_stop &&
                write_stop.previous_start_gate == 0x1234U &&
                write_stop.start_gate_reads == 1U &&
                write_stop.start_gate_writes == 0U &&
                flags_equal(write_stop.flags, write_request.entry_flags) &&
                latch_stop.status ==
                    LegacyBattleActorStartGateIncrementStatus::
                        start_gate_latch_write_typed_stop &&
                latch_stop.previous_start_gate == 0x1234U &&
                latch_stop.incremented_start_gate == 0x1235U &&
                latch_stop.start_gate_reads == 1U &&
                latch_stop.start_gate_writes == 1U &&
                latch_stop.start_gate_latch_writes == 0U &&
                latch_stop.return_eip == 0x00478AC7U && start_gate == 0x1236U &&
                start_gate_latch == 1U &&
                return_stop.status ==
                    LegacyBattleActorStartGateIncrementStatus::
                        return_address_read_typed_stop &&
                return_stop.previous_start_gate == 0x1235U &&
                return_stop.incremented_start_gate == 0x1236U &&
                return_stop.start_gate_latch_writes == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x00478AD1U && !return_stop.returned,
            "start-gate increment stops preserve all four physical access prefixes"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        LegacyBattleActorStartGateIncrementTrace trace;
        LegacyBattleActorStartGateIncrementCallRequests requests;
        requests.count = 1U;
        requests.calls[0U].entry_esp = 0x90001000U;
        const bool completed =
            execute_legacy_battle_actor_start_gate_increment_call(
                trace,
                requests,
                {.action = &action, .startup = &startup},
                0x00456FE1U,
                0x00456FE6U,
                kLegacyBattleActorCoordinatesGroupBBaseToken,
                0x11223344U,
                0x55667788U,
                {.carry = true}
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x00456FE1U &&
                trace.return_addresses[0U] == 0x00456FE6U &&
                trace.actor_tokens[0U] ==
                    kLegacyBattleActorCoordinatesGroupBBaseToken &&
                trace.last.return_eax == 0x11223344U &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U &&
                (*startup.group_b_lifecycle)[0U].action_execution.start_gate ==
                    1U &&
                (*startup.group_b_lifecycle)[0U]
                        .action_execution.start_gate_latch == 1U,
            "start-gate increment call retains physical identity and updates the canonical actor"
        );
    }

    test.expect_true(
        kLegacyBattleActorStartGateIncrementAddress == 0x00478AC0U &&
            kLegacyBattleActorStartGateIncrementEndAddress == 0x00478AD1U &&
            kLegacyBattleActorStartGateIncrementCallAddresses ==
                std::array<u32, 5>{
                    0x00456FE1U,
                    0x0045707AU,
                    0x0045786BU,
                    0x00457E1CU,
                    0x0046DD4AU,
                } &&
            kLegacyBattleActorStartGateIncrementReturnAddresses ==
                std::array<u32, 5>{
                    0x00456FE6U,
                    0x0045707FU,
                    0x00457870U,
                    0x00457E21U,
                    0x0046DD4FU,
                },
        "start-gate increment retains all five physical callers"
    );
}
