#include "openswd3/battle/legacy_battle_actor_action_target_clear.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorActionTargetClearRequest;
using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorActionTargetClearRequest request() {
    return {
        .call_address = 0x00457EBCU,
        .return_address = 0x00457EC1U,
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

void test_battle_actor_action_target_clear(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorActionTargetClearCallRequests;
    using openswd3::battle::LegacyBattleActorActionTargetClearStatus;
    using openswd3::battle::LegacyBattleActorActionTargetClearTrace;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::clear_legacy_battle_actor_action_target;
    using openswd3::battle::
        execute_legacy_battle_actor_action_target_clear_call;
    using openswd3::battle::kLegacyBattleActorActionTargetClearAddress;
    using openswd3::battle::kLegacyBattleActorActionTargetClearCallAddresses;
    using openswd3::battle::kLegacyBattleActorActionTargetClearReturnAddresses;
    using openswd3::battle::
        kLegacyBattleActorActionTargetClearReturnInstruction;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::resolve_legacy_battle_actor_action_target;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        const auto group_a = resolve_legacy_battle_actor_action_target(
            {.action = &action, .startup = &startup},
            kLegacyBattleActorCoordinatesGroupABaseToken +
                2U * kLegacyBattleActorCoordinatesGroupAStride
        );
        const auto group_b = resolve_legacy_battle_actor_action_target(
            {.action = &action, .startup = &startup},
            kLegacyBattleActorCoordinatesGroupBBaseToken +
                3U * kLegacyBattleActorCoordinatesGroupBStride
        );
        const auto invalid = resolve_legacy_battle_actor_action_target(
            {.action = &action, .startup = &startup}, 0xDEADBEEFU
        );
        test.expect_true(
            group_a.action_target ==
                    &action.group_a_action_execution[2U].action_target &&
                group_b.action_target ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.action_target &&
                invalid.action_target == nullptr,
            "action-target clear reuses the canonical Group-A and Group-B resolver"
        );
    }

    for (const u16 initial :
         std::array<u16, 4>{0x0000U, 0x7FFFU, 0x8000U, 0xFFFFU}) {
        u16 action_target = initial;
        const auto result = clear_legacy_battle_actor_action_target(
            {.action_target = &action_target}, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionTargetClearStatus::completed &&
                action_target == 0xFFFFU && result.action_target_writes == 1U &&
                result.action_target_field_token == 0x00527EAAU &&
                result.return_eax == request().entry_eax &&
                result.return_ecx == request().actor_token &&
                result.return_edx == request().entry_edx &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x00457EC1U &&
                result.return_address_reads == 1U &&
                result.stack_reads[0U] == 0x00457EC1U && result.returned &&
                result.flags_known &&
                flags_equal(result.flags, request().entry_flags),
            "action-target clear unconditionally writes minus one and preserves registers and flags through RET"
        );
    }

    {
        u16 action_target = 0x1234U;
        auto write_stop_request = request();
        write_stop_request.access.action_target_writable = false;
        const auto write_stop = clear_legacy_battle_actor_action_target(
            {.action_target = &action_target}, write_stop_request
        );
        const u16 action_target_after_write_stop = action_target;

        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop = clear_legacy_battle_actor_action_target(
            {.action_target = &action_target}, return_stop_request
        );

        test.expect_true(
            write_stop.status ==
                    LegacyBattleActorActionTargetClearStatus::
                        action_target_write_typed_stop &&
                action_target_after_write_stop == 0x1234U &&
                action_target == 0xFFFFU &&
                write_stop.action_target_writes == 0U &&
                write_stop.return_eip ==
                    kLegacyBattleActorActionTargetClearAddress &&
                write_stop.return_esp == request().entry_esp &&
                !write_stop.returned &&
                return_stop.status ==
                    LegacyBattleActorActionTargetClearStatus::
                        return_address_read_typed_stop &&
                return_stop.action_target_writes == 1U &&
                return_stop.return_eip ==
                    kLegacyBattleActorActionTargetClearReturnInstruction &&
                return_stop.return_esp == request().entry_esp &&
                !return_stop.returned,
            "action-target clear typed stops preserve write and RET partial commits"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleActorActionTargetClearTrace trace;
        LegacyBattleActorActionTargetClearCallRequests requests;
        requests.count = 3U;
        requests.calls[2U].entry_esp = 0x90001000U;
        action.group_a_action_execution[0U].action_target = 7U;
        const bool completed =
            execute_legacy_battle_actor_action_target_clear_call(
                trace,
                requests,
                {.action = &action},
                0x004570F5U,
                0x004570FAU,
                kLegacyBattleActorCoordinatesGroupABaseToken,
                0x11223344U,
                0x55667788U,
                {.zero = true},
                true,
                2U
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x004570F5U &&
                trace.return_addresses[0U] == 0x004570FAU &&
                trace.actor_tokens[0U] ==
                    kLegacyBattleActorCoordinatesGroupABaseToken &&
                trace.last.return_eax == 0x11223344U &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U &&
                action.group_a_action_execution[0U].action_target == 0xFFFFU,
            "action-target clear retains physical identity and consumes the explicit request offset"
        );
    }

    test.expect_true(
        kLegacyBattleActorActionTargetClearAddress == 0x00478B20U &&
            kLegacyBattleActorActionTargetClearReturnInstruction ==
                0x00478B29U &&
            kLegacyBattleActorActionTargetClearCallAddresses ==
                std::array<u32, 5>{
                    0x00456F94U,
                    0x004570F5U,
                    0x00457EBCU,
                    0x0045AEC2U,
                    0x0045AF58U,
                } &&
            kLegacyBattleActorActionTargetClearReturnAddresses ==
                std::array<u32, 5>{
                    0x00456F99U,
                    0x004570FAU,
                    0x00457EC1U,
                    0x0045AEC7U,
                    0x0045AF5DU,
                },
        "action-target clear retains all five physical callers"
    );
}
