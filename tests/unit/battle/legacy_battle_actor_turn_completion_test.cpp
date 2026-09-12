#include "openswd3/battle/legacy_battle_actor_turn_completion.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorTurnCompletionRequest;
using openswd3::battle::LegacyBattleActorTurnCompletionStatus;
using openswd3::battle::LegacyBattleActorTurnCompletionView;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorTurnCompletionRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0x11112222U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00456DDBU,
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

void test_battle_actor_turn_completion(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[2U].turn_completion_latch = 0x11223344U;
        (*startup.group_b_lifecycle)[3U]
            .action_execution.turn_completion_latch = 0x55667788U;
        const auto owners =
            openswd3::battle::LegacyBattleActorTurnCompletionOwners{
                .action = &action,
                .startup = &startup,
            };
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_turn_completion(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_turn_completion(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_turn_completion(
                {.action = &action},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_turn_completion(
                owners, 0xDEADBEEFU
            );
        test.expect_true(
            group_a.latch ==
                    &action.group_a_action_execution[2U]
                         .turn_completion_latch &&
                *group_a.latch == 0x11223344U &&
                group_b.latch ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.turn_completion_latch &&
                *group_b.latch == 0x55667788U && missing.latch == nullptr &&
                invalid.latch == nullptr,
            "turn completion resolver aliases only canonical Group-A action and Group-B lifecycle owners"
        );
    }

    {
        const u32 latch = 0xA5A55A5AU;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_turn_completion(
                {.latch = &latch}, entry
            );
        test.expect_true(
            result.status == LegacyBattleActorTurnCompletionStatus::completed &&
                result.returned && result.return_eax == latch &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x2AACU &&
                result.latch_reads == 1U && result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                flags_equal(result.flags, entry.entry_flags),
            "turn completion getter returns the full dword and plain-RET machine state without changing flags"
        );
    }

    {
        const u32 latch = 0U;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_turn_completion(
                {.latch = &latch}, entry
            );
        test.expect_true(
            result.status == LegacyBattleActorTurnCompletionStatus::completed &&
                result.returned && result.return_eax == 0U &&
                result.latch_reads == 1U && result.return_address_reads == 1U &&
                flags_equal(result.flags, entry.entry_flags),
            "zero turn completion is loaded without synthesizing TEST or XOR flags"
        );
    }

    {
        const u32 latch = 0xCAFEBABEU;
        auto field_request = request();
        field_request.access.latch_readable = false;
        const auto field_stop =
            openswd3::battle::query_legacy_battle_actor_turn_completion(
                {.latch = &latch}, field_request
            );
        const auto missing_stop =
            openswd3::battle::query_legacy_battle_actor_turn_completion(
                LegacyBattleActorTurnCompletionView{}, request()
            );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::query_legacy_battle_actor_turn_completion(
                {.latch = &latch}, return_request
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                field_stop.return_eax == field_request.entry_eax &&
                field_stop.return_ecx == field_request.actor_token &&
                field_stop.return_edx == field_request.entry_edx &&
                field_stop.return_esp == field_request.entry_esp &&
                field_stop.return_eip == 0x00478690U &&
                field_stop.latch_reads == 0U &&
                field_stop.return_address_reads == 0U && !field_stop.returned &&
                missing_stop.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                return_stop.status ==
                    LegacyBattleActorTurnCompletionStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == latch &&
                return_stop.return_ecx == return_request.actor_token &&
                return_stop.return_edx == return_request.entry_edx &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x00478696U &&
                return_stop.latch_reads == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && !return_stop.returned &&
                flags_equal(field_stop.flags, field_request.entry_flags) &&
                flags_equal(return_stop.flags, return_request.entry_flags),
            "field and RET typed stops preserve the exact completed read prefix, registers, stack, EIP, and flags"
        );
    }
}
