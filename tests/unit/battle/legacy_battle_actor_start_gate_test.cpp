#include "openswd3/battle/legacy_battle_actor_start_gate.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorStartGateRequest;
using openswd3::battle::LegacyBattleActorStartGateStatus;
using openswd3::battle::LegacyBattleActorStartGateView;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorStartGateRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x004566B2U,
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
    const openswd3::battle::LegacyBattleActorCoordinateFlags& left,
    const openswd3::battle::LegacyBattleActorCoordinateFlags& right
) noexcept {
    return left.carry == right.carry && left.parity == right.parity &&
        left.auxiliary_carry == right.auxiliary_carry &&
        left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
        left.zero == right.zero && left.sign == right.sign &&
        left.overflow == right.overflow;
}

}  // namespace

void test_battle_actor_start_gate(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[2U].start_gate = 0x1122U;
        (*startup.group_b_lifecycle)[3U].action_execution.start_gate = 0x5566U;
        const auto owners = openswd3::battle::LegacyBattleActorStartGateOwners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_start_gate(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_start_gate(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_start_gate(
                {},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_start_gate(
                owners, 0xDEADBEEFU
            );
        test.expect_true(
            group_a.start_gate ==
                    &action.group_a_action_execution[2U].start_gate &&
                *group_a.start_gate == 0x1122U &&
                group_b.start_gate ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate &&
                *group_b.start_gate == 0x5566U &&
                missing.start_gate == nullptr && invalid.start_gate == nullptr,
            "start-gate resolver aliases only canonical Group-A action-execution and Group-B lifecycle owners"
        );
    }

    {
        const u16 start_gate = 0xCDEFU;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                {.start_gate = &start_gate}, entry
            );
        test.expect_true(
            result.status == LegacyBattleActorStartGateStatus::completed &&
                result.returned && result.return_eax == 0xA5A5CDEFU &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x2A74U &&
                result.start_gate_reads == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                flags_equal(result.flags, entry.entry_flags),
            "start-gate getter replaces only AX and preserves the plain-RET machine state and flags"
        );
    }

    {
        const auto entry = request();
        const u16 zero = 0U;
        const u16 one = 1U;
        const auto zero_result =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                {.start_gate = &zero}, entry
            );
        const auto one_result =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                {.start_gate = &one}, entry
            );
        test.expect_true(
            zero_result.return_eax == 0xA5A50000U &&
                one_result.return_eax == 0xA5A50001U &&
                flags_equal(zero_result.flags, entry.entry_flags) &&
                flags_equal(one_result.flags, entry.entry_flags),
            "zero and one start gates replace AX without zero extension, sign extension, or synthetic flags"
        );
    }

    {
        const u16 start_gate = 0xBEEFU;
        auto field_request = request();
        field_request.access.start_gate_readable = false;
        const auto field_stop =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                {.start_gate = &start_gate}, field_request
            );
        const auto missing_stop =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                LegacyBattleActorStartGateView{}, request()
            );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::query_legacy_battle_actor_start_gate(
                {.start_gate = &start_gate}, return_request
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorStartGateStatus::
                        start_gate_read_typed_stop &&
                field_stop.return_eax == field_request.entry_eax &&
                field_stop.return_ecx == field_request.actor_token &&
                field_stop.return_edx == field_request.entry_edx &&
                field_stop.return_esp == field_request.entry_esp &&
                field_stop.return_eip == 0x004786D0U &&
                field_stop.start_gate_reads == 0U &&
                field_stop.return_address_reads == 0U && !field_stop.returned &&
                missing_stop.status ==
                    LegacyBattleActorStartGateStatus::
                        start_gate_read_typed_stop &&
                return_stop.status ==
                    LegacyBattleActorStartGateStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == 0xA5A5BEEFU &&
                return_stop.return_ecx == return_request.actor_token &&
                return_stop.return_edx == return_request.entry_edx &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x004786D7U &&
                return_stop.start_gate_reads == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && !return_stop.returned &&
                flags_equal(field_stop.flags, field_request.entry_flags) &&
                flags_equal(return_stop.flags, return_request.entry_flags),
            "field and RET stops preserve exact read prefix, partial EAX, registers, stack, EIP, and flags"
        );
    }
}
