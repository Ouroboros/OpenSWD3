#include "openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorTargetSelectionCountQueryRequest request() {
    return {
        .call_address = 0x0046D429U,
        .return_address = 0x0046D42EU,
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x55667788U,
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

void test_battle_actor_target_selection_count_query(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionCountQueryCallRequests;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryOwners;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryStatus;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryTrace;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryView;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        execute_legacy_battle_actor_target_selection_count_query_call;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::kLegacyBattleActorTargetSelectionCountQueryAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountQueryCallAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountQueryEndAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionCountQueryReturnAddress;
    using openswd3::battle::query_legacy_battle_actor_target_selection_count;
    using openswd3::battle::
        resolve_legacy_battle_actor_target_selection_count_query;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[2U].target_selection_count = 0x1122U;
        (*startup.group_b_lifecycle)[3U]
            .action_execution.target_selection_count = 0x5566U;
        const LegacyBattleActorTargetSelectionCountQueryOwners owners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a =
            resolve_legacy_battle_actor_target_selection_count_query(
                owners,
                kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U * kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            resolve_legacy_battle_actor_target_selection_count_query(
                owners,
                kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U * kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto invalid =
            resolve_legacy_battle_actor_target_selection_count_query(
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
            "target-selection count query aliases only canonical Group-A action-execution and Group-B lifecycle owners"
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
            const auto entry = request();
            const auto result =
                query_legacy_battle_actor_target_selection_count(
                    {.target_selection_count = &value}, entry
                );
            matches = matches &&
                result.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        completed &&
                result.count_value == value && result.field_reads == 1U &&
                result.field_token == entry.actor_token + 0x2A76U &&
                result.return_eax == (entry.entry_eax & 0xFFFF0000U) + value &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.return_address &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == entry.return_address &&
                result.returned && result.flags_known &&
                flags_equal(result.flags, entry.entry_flags);
        }
        test.expect_true(
            matches,
            "target-selection count query replaces only AX and preserves registers, flags, and plain RET"
        );
    }

    {
        const u16 value = 0xBEEFU;
        auto field_request = request();
        field_request.access.count_readable = false;
        const auto field_stop =
            query_legacy_battle_actor_target_selection_count(
                {.target_selection_count = &value}, field_request
            );
        const auto missing_stop =
            query_legacy_battle_actor_target_selection_count(
                LegacyBattleActorTargetSelectionCountQueryView{}, request()
            );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            query_legacy_battle_actor_target_selection_count(
                {.target_selection_count = &value}, return_request
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        count_read_typed_stop &&
                field_stop.return_eax == field_request.entry_eax &&
                field_stop.return_ecx == field_request.actor_token &&
                field_stop.return_edx == field_request.entry_edx &&
                field_stop.return_esp == field_request.entry_esp &&
                field_stop.return_eip == 0x00478AB0U &&
                field_stop.field_reads == 0U &&
                field_stop.return_address_reads == 0U && !field_stop.returned &&
                missing_stop.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        count_read_typed_stop &&
                return_stop.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == 0xA5A5BEEFU &&
                return_stop.return_ecx == return_request.actor_token &&
                return_stop.return_edx == return_request.entry_edx &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x00478AB7U &&
                return_stop.field_reads == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && !return_stop.returned &&
                flags_equal(field_stop.flags, field_request.entry_flags) &&
                flags_equal(return_stop.flags, return_request.entry_flags),
            "target-selection count query stops preserve the exact field-read and RET prefixes"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.group_a_action_execution[0U].target_selection_count = 7U;
        LegacyBattleActorTargetSelectionCountQueryTrace trace;
        LegacyBattleActorTargetSelectionCountQueryCallRequests requests;
        requests.count = 1U;
        requests.calls[0U].entry_esp = 0x90001000U;
        const bool completed =
            execute_legacy_battle_actor_target_selection_count_query_call(
                trace,
                requests,
                {.action = &action, .startup = &startup},
                kLegacyBattleActorTargetSelectionCountQueryCallAddress,
                kLegacyBattleActorTargetSelectionCountQueryReturnAddress,
                kLegacyBattleActorCoordinatesGroupABaseToken,
                0x11223344U,
                0x55667788U,
                {.carry = true}
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x0046D429U &&
                trace.return_addresses[0U] == 0x0046D42EU &&
                trace.actor_tokens[0U] ==
                    kLegacyBattleActorCoordinatesGroupABaseToken &&
                trace.last.return_eax == 0x11220007U &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U,
            "target-selection count query retains the physical call identity and canonical actor"
        );
    }

    test.expect_true(
        kLegacyBattleActorTargetSelectionCountQueryAddress == 0x00478AB0U &&
            kLegacyBattleActorTargetSelectionCountQueryEndAddress ==
                0x00478AB7U &&
            kLegacyBattleActorTargetSelectionCountQueryCallAddress ==
                0x0046D429U &&
            kLegacyBattleActorTargetSelectionCountQueryReturnAddress ==
                0x0046D42EU,
        "target-selection count query retains its leaf and physical caller range"
    );
}
