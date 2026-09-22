#include "openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorStartGateLatchQueryRequest;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorStartGateLatchQueryRequest request() {
    return {
        .call_address = 0x00457842U,
        .return_address = 0x00457847U,
        .actor_token = 0x00525508U,
        .entry_eax = 0x92345678U,
        .entry_edx = 0x89ABCDEFU,
        .entry_esp = 0x80001000U,
        .entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = true,
            .overflow = true,
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

void test_battle_actor_start_gate_latch_query(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorStartGateLatchQueryCallRequests;
    using openswd3::battle::LegacyBattleActorStartGateLatchQueryStatus;
    using openswd3::battle::LegacyBattleActorStartGateLatchQueryTrace;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        execute_legacy_battle_actor_start_gate_latch_query_call;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::kLegacyBattleActorStartGateLatchQueryAddress;
    using openswd3::battle::kLegacyBattleActorStartGateLatchQueryCallAddresses;
    using openswd3::battle::
        kLegacyBattleActorStartGateLatchQueryReturnAddresses;
    using openswd3::battle::
        kLegacyBattleActorStartGateLatchQueryReturnInstruction;
    using openswd3::battle::query_legacy_battle_actor_start_gate_latch;
    using openswd3::battle::resolve_legacy_battle_actor_start_gate_increment;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        const auto group_a = resolve_legacy_battle_actor_start_gate_increment(
            {.action = &action, .startup = &startup},
            kLegacyBattleActorCoordinatesGroupABaseToken +
                2U * kLegacyBattleActorCoordinatesGroupAStride
        );
        const auto group_b = resolve_legacy_battle_actor_start_gate_increment(
            {.action = &action, .startup = &startup},
            kLegacyBattleActorCoordinatesGroupBBaseToken +
                3U * kLegacyBattleActorCoordinatesGroupBStride
        );
        const auto invalid = resolve_legacy_battle_actor_start_gate_increment(
            {.action = &action, .startup = &startup}, 0xDEADBEEFU
        );
        const auto unaligned = resolve_legacy_battle_actor_start_gate_increment(
            {.action = &action, .startup = &startup},
            kLegacyBattleActorCoordinatesGroupABaseToken + 1U
        );
        const auto out_of_range =
            resolve_legacy_battle_actor_start_gate_increment(
                {.action = &action, .startup = &startup},
                kLegacyBattleActorCoordinatesGroupBBaseToken +
                    kLegacyBattleActorGroupBElementCount *
                        kLegacyBattleActorCoordinatesGroupBStride
            );
        test.expect_true(
            group_a.start_gate_latch ==
                    &action.group_a_action_execution[2U].start_gate_latch &&
                group_b.start_gate_latch ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate_latch &&
                invalid.start_gate_latch == nullptr &&
                unaligned.start_gate_latch == nullptr &&
                out_of_range.start_gate_latch == nullptr,
            "start-gate latch getter resolves only aligned in-range canonical actors"
        );
    }

    for (const u32 value : std::array<u32, 6>{
             0U, 1U, 2U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU
         }) {
        u32 start_gate_latch = value;
        const auto result = query_legacy_battle_actor_start_gate_latch(
            {.start_gate_latch = &start_gate_latch}, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::completed &&
                result.return_eax == value &&
                result.start_gate_latch_reads == 1U &&
                result.start_gate_latch_field_token == 0x00527FE8U &&
                result.return_ecx == request().actor_token &&
                result.return_edx == request().entry_edx &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x00457847U &&
                result.return_address_reads == 1U &&
                result.stack_reads[0U] == 0x00457847U && result.returned &&
                result.flags_known &&
                flags_equal(result.flags, request().entry_flags),
            "start-gate latch getter returns the full dword and preserves ECX EDX and flags through RET"
        );
    }

    {
        u32 start_gate_latch = 0xA5A5A5A5U;
        auto read_stop_request = request();
        read_stop_request.access.start_gate_latch_readable = false;
        const auto read_stop = query_legacy_battle_actor_start_gate_latch(
            {.start_gate_latch = &start_gate_latch}, read_stop_request
        );

        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop = query_legacy_battle_actor_start_gate_latch(
            {.start_gate_latch = &start_gate_latch}, return_stop_request
        );

        auto unknown_flags_request = request();
        unknown_flags_request.entry_flags_known = false;
        const auto unknown_flags = query_legacy_battle_actor_start_gate_latch(
            {.start_gate_latch = &start_gate_latch}, unknown_flags_request
        );

        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::
                        start_gate_latch_read_typed_stop &&
                read_stop.return_eax == request().entry_eax &&
                read_stop.start_gate_latch_reads == 0U &&
                read_stop.return_eip ==
                    kLegacyBattleActorStartGateLatchQueryAddress &&
                read_stop.return_esp == request().entry_esp &&
                !read_stop.returned &&
                return_stop.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == 0xA5A5A5A5U &&
                return_stop.start_gate_latch_reads == 1U &&
                return_stop.return_eip ==
                    kLegacyBattleActorStartGateLatchQueryReturnInstruction &&
                return_stop.return_esp == request().entry_esp &&
                !return_stop.returned && !unknown_flags.flags_known,
            "start-gate latch getter typed stops preserve field-read and RET partial commits"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        action.group_a_action_execution[0U].start_gate_latch = 0x13579BDFU;
        LegacyBattleActorStartGateLatchQueryTrace trace;
        LegacyBattleActorStartGateLatchQueryCallRequests requests;
        requests.count = 3U;
        requests.calls[2U].entry_esp = 0x90001000U;
        const u32 actor_token = kLegacyBattleActorCoordinatesGroupABaseToken;
        const bool completed =
            execute_legacy_battle_actor_start_gate_latch_query_call(
                trace,
                requests,
                {.action = &action},
                0x00457842U,
                0x00457847U,
                actor_token,
                0x11223344U,
                0x55667788U,
                {.zero = true},
                true,
                2U
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x00457842U &&
                trace.return_addresses[0U] == 0x00457847U &&
                trace.actor_tokens[0U] == actor_token &&
                trace.last.return_eax == 0x13579BDFU &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U,
            "start-gate latch getter retains physical identity and consumes the explicit request offset"
        );
    }

    test.expect_true(
        kLegacyBattleActorStartGateLatchQueryAddress == 0x00478B50U &&
            kLegacyBattleActorStartGateLatchQueryReturnInstruction ==
                0x00478B56U &&
            kLegacyBattleActorStartGateLatchQueryCallAddresses ==
                std::array<u32, 1>{0x00457842U} &&
            kLegacyBattleActorStartGateLatchQueryReturnAddresses ==
                std::array<u32, 1>{0x00457847U},
        "start-gate latch getter retains its only physical caller"
    );
}
