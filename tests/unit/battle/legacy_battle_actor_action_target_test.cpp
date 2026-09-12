#include "openswd3/battle/legacy_battle_actor_action_target.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorActionTargetRequest;
using openswd3::battle::LegacyBattleActorActionTargetStatus;
using openswd3::battle::LegacyBattleActorActionTargetView;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorActionTargetRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00454A42U,
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

void test_battle_actor_action_target(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        openswd3::battle::LegacyBattleStartupState startup;
        openswd3::battle::LegacyBattleDebugHotkeyState debug_hotkeys;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[2U].action_target = 0x1122U;
        (*startup.group_b_lifecycle)[3U].action_execution.action_target =
            0x5566U;
        debug_hotkeys.special_actor_action_target.action_target = 0x7788U;
        const auto owners =
            openswd3::battle::LegacyBattleActorActionTargetOwners{
                .action = &action,
                .startup = &startup,
                .debug_hotkeys = &debug_hotkeys,
            };
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_action_target(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_action_target(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto special =
            openswd3::battle::resolve_legacy_battle_actor_action_target(
                owners, openswd3::battle::kLegacyBattleDebugSpecialActorToken
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_action_target(
                {},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        test.expect_true(
            group_a.action_target ==
                    &action.group_a_action_execution[2U].action_target &&
                *group_a.action_target == 0x1122U &&
                group_b.action_target ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.action_target &&
                *group_b.action_target == 0x5566U &&
                special.action_target ==
                    &debug_hotkeys.special_actor_action_target.action_target &&
                *special.action_target == 0x7788U &&
                missing.action_target == nullptr,
            "action-target resolver aliases canonical Group-A, Group-B, and debug special actor owners"
        );
    }

    {
        const u16 action_target = 0xCDEFU;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = const_cast<u16*>(&action_target)}, entry
            );
        test.expect_true(
            result.status == LegacyBattleActorActionTargetStatus::completed &&
                result.returned && result.return_eax == 0xA5A5CDEFU &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x29A2U &&
                result.action_target_reads == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                flags_equal(result.flags, entry.entry_flags),
            "action-target getter replaces only AX and preserves plain-RET machine state and flags"
        );
    }

    {
        u16 zero = 0U;
        u16 one = 1U;
        u16 minus_one = 0xFFFFU;
        const auto entry = request();
        const auto zero_result =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = &zero}, entry
            );
        const auto one_result =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = &one}, entry
            );
        const auto minus_one_result =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = &minus_one}, entry
            );
        test.expect_true(
            zero_result.return_eax == 0xA5A50000U &&
                one_result.return_eax == 0xA5A50001U &&
                minus_one_result.return_eax == 0xA5A5FFFFU &&
                flags_equal(zero_result.flags, entry.entry_flags) &&
                flags_equal(one_result.flags, entry.entry_flags) &&
                flags_equal(minus_one_result.flags, entry.entry_flags),
            "zero, one, and minus-one action targets replace AX without caller post-processing"
        );
    }

    {
        u16 action_target = 0xBEEFU;
        auto field_request = request();
        field_request.access.action_target_readable = false;
        const auto field_stop =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = &action_target}, field_request
            );
        const auto missing_stop =
            openswd3::battle::query_legacy_battle_actor_action_target(
                LegacyBattleActorActionTargetView{}, request()
            );
        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::query_legacy_battle_actor_action_target(
                {.action_target = &action_target}, return_request
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorActionTargetStatus::
                        action_target_read_typed_stop &&
                field_stop.return_eax == field_request.entry_eax &&
                field_stop.return_ecx == field_request.actor_token &&
                field_stop.return_edx == field_request.entry_edx &&
                field_stop.return_esp == field_request.entry_esp &&
                field_stop.return_eip == 0x004786E0U &&
                field_stop.action_target_reads == 0U &&
                field_stop.return_address_reads == 0U && !field_stop.returned &&
                missing_stop.status ==
                    LegacyBattleActorActionTargetStatus::
                        action_target_read_typed_stop &&
                return_stop.status ==
                    LegacyBattleActorActionTargetStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == 0xA5A5BEEFU &&
                return_stop.return_ecx == return_request.actor_token &&
                return_stop.return_edx == return_request.entry_edx &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x004786E7U &&
                return_stop.action_target_reads == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && !return_stop.returned &&
                flags_equal(field_stop.flags, field_request.entry_flags) &&
                flags_equal(return_stop.flags, return_request.entry_flags),
            "action-target field and RET stops preserve exact prefixes, registers, stack, EIP, and flags"
        );
    }
}
