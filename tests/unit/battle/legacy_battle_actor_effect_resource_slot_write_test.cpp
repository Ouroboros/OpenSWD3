#include "openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorEffectResourceSlotWriteAccess;
using openswd3::battle::LegacyBattleActorEffectResourceSlotWriteRequest;
using openswd3::battle::LegacyBattleActorEffectResourceSlotWriteStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorEffectResourceSlotWriteRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .value = 0xBEEFU,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0xCAFE5678U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00453B8AU,
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

void test_battle_actor_effect_resource_slot_write(
    openswd3::test::Context& test
) {
    {
        const auto action = std::make_unique<
            openswd3::battle::LegacyBattleActionDispatchState>();
        const auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const u32 group_a_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
            openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
        const u32 group_b_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
            2U * openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
        const auto owners =
            openswd3::battle::LegacyBattleActorEffectResourceSlotWriteOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto group_a = openswd3::battle::
            resolve_legacy_battle_actor_effect_resource_slot_write(
                owners, group_a_token
            );
        const auto group_b = openswd3::battle::
            resolve_legacy_battle_actor_effect_resource_slot_write(
                owners, group_b_token
            );
        const auto invalid = openswd3::battle::
            resolve_legacy_battle_actor_effect_resource_slot_write(
                owners, group_a_token + 1U
            );
        group_a.slots->fill(0x1111U);
        *group_a.cursor = 33U;
        *group_a.execution_complete = 0U;
        openswd3::battle::
            synchronize_legacy_battle_actor_effect_resource_cursor_update(
                owners, group_a_token, 1U
            );
        const u16 incremented_cursor = *group_a.cursor;
        openswd3::battle::
            synchronize_legacy_battle_actor_effect_resource_cursor_update(
                owners, group_a_token, 1U
            );
        const u16 clamped_cursor = *group_a.cursor;
        openswd3::battle::
            synchronize_legacy_battle_actor_effect_resource_cursor_update(
                owners, group_a_token, 0U
            );
        const u16 decremented_cursor = *group_a.cursor;
        *group_a.cursor = 0U;
        openswd3::battle::
            synchronize_legacy_battle_actor_effect_resource_cursor_update(
                owners, group_a_token, 2U
            );
        const u16 wrapped_cursor = *group_a.cursor;
        const u32 wrapped_execution_complete = *group_a.execution_complete;
        openswd3::battle::
            synchronize_legacy_battle_actor_effect_resource_cursor_update(
                owners, group_a_token, 0U
            );
        openswd3::battle::reset_legacy_battle_actor_effect_resource_slots(
            owners, group_a_token
        );
        test.expect_true(
            group_a.slots ==
                    &action->group_a_action_execution[1U]
                         .effect_resource_slots &&
                group_a.cursor ==
                    &action->group_a_action_execution[1U]
                         .effect_resource_cursor &&
                group_b.slots ==
                    &(*startup->group_b_lifecycle)[2U]
                         .action_execution.effect_resource_slots &&
                group_b.cursor ==
                    &(*startup->group_b_lifecycle)[2U]
                         .action_execution.effect_resource_cursor &&
                invalid.slots == nullptr && invalid.cursor == nullptr &&
                incremented_cursor == 34U && clamped_cursor == 34U &&
                decremented_cursor == 33U && wrapped_cursor == 0xFFFFU &&
                wrapped_execution_complete == 2U && *group_a.cursor == 0U &&
                *group_a.execution_complete == 0U &&
                std::all_of(
                    group_a.slots->begin(),
                    group_a.slots->end(),
                    [](const u16 value) { return value == 0U; }
                ),
            "effect-resource slots share the canonical Group-A/Group-B owners and synchronize reset/cursor updates"
        );
    }

    {
        std::array<u16, 35> slots{};
        u16 cursor = 7U;
        u32 execution_complete = 0U;
        const auto entry = request();
        const auto result =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {
                    .slots = &slots,
                    .cursor = &cursor,
                    .execution_complete = &execution_complete,
                },
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::completed &&
                result.returned && slots[7U] == 0xBEEFU &&
                result.return_eax == 7U &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == 0xCAFEBEEFU &&
                result.return_esp == entry.entry_esp + 8U &&
                result.return_eip == entry.entry_return_address &&
                result.argument_token == entry.entry_esp + 4U &&
                result.cursor_token == entry.actor_token + 0x2A7CU &&
                result.target_token == entry.actor_token + 0x29C4U + 14U &&
                result.return_address_token == entry.entry_esp &&
                result.argument_reads == 1U && result.cursor_reads == 1U &&
                result.target_writes == 1U &&
                result.return_address_reads == 1U &&
                result.actor_access_count == 2U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorEffectResourceSlotWriteAccess::
                        cursor_read &&
                result.actor_accesses[1U] ==
                    LegacyBattleActorEffectResourceSlotWriteAccess::
                        target_write &&
                result.stack_read_count == 2U &&
                result.stack_read_tokens[0U] == entry.entry_esp + 4U &&
                result.stack_reads[0U] == 0xBEEFU &&
                result.stack_read_tokens[1U] == entry.entry_esp &&
                result.stack_reads[1U] == entry.entry_return_address &&
                result.flags_known && result.overflow_defined &&
                !result.flags.carry && result.flags.parity &&
                !result.flags.auxiliary_carry &&
                !result.flags.auxiliary_carry_defined && result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "sub_4787D0 preserves partial registers, XOR flags, indexed write order, and RETN 4 state"
        );
    }

    {
        std::array<u16, 35> slots{};
        u16 cursor = 3U;
        auto argument_request = request();
        argument_request.access.argument_readable = false;
        argument_request.entry_flags_known = false;
        argument_request.entry_overflow_defined = false;
        const auto result =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {.slots = &slots, .cursor = &cursor}, argument_request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::
                        argument_read_typed_stop &&
                result.return_eax == argument_request.entry_eax &&
                result.return_edx == argument_request.entry_edx &&
                result.return_esp == argument_request.entry_esp &&
                result.return_eip == 0x004787D0U &&
                result.stack_read_count == 0U &&
                result.actor_access_count == 0U && !result.flags_known &&
                !result.overflow_defined &&
                flags_equal(result.flags, argument_request.entry_flags) &&
                !result.returned,
            "argument-read fault preserves the complete entry machine state"
        );
    }

    {
        std::array<u16, 35> slots{};
        u16 cursor = 3U;
        auto cursor_request = request();
        cursor_request.access.cursor_readable = false;
        const auto result =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {.slots = &slots, .cursor = &cursor}, cursor_request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::
                        cursor_read_typed_stop &&
                result.return_eax == 0U && result.return_edx == 0xCAFEBEEFU &&
                result.return_eip == 0x004787D7U &&
                result.stack_read_count == 1U && result.argument_reads == 1U &&
                result.cursor_reads == 0U && result.target_writes == 0U &&
                result.flags_known && result.overflow_defined &&
                result.flags.zero && result.flags.parity && !result.returned,
            "cursor-read fault preserves committed DX and XOR state"
        );
    }

    {
        std::array<u16, 35> slots{};
        slots.fill(0x1111U);
        u16 cursor = 34U;
        auto write_request = request();
        write_request.access.target_writable = false;
        const auto explicit_stop =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {.slots = &slots, .cursor = &cursor}, write_request
            );
        cursor = 35U;
        const auto out_of_owner_stop =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {.slots = &slots, .cursor = &cursor}, request()
            );
        test.expect_true(
            explicit_stop.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::
                        target_write_typed_stop &&
                out_of_owner_stop.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::
                        target_write_typed_stop &&
                explicit_stop.return_eax == 34U &&
                explicit_stop.return_edx == 0xCAFEBEEFU &&
                explicit_stop.return_eip == 0x004787DEU &&
                explicit_stop.cursor_reads == 1U &&
                explicit_stop.target_writes == 0U &&
                explicit_stop.actor_access_count == 1U &&
                out_of_owner_stop.return_eax == 35U &&
                out_of_owner_stop.target_token ==
                    request().actor_token + 0x29C4U + 70U &&
                std::all_of(
                    slots.begin(),
                    slots.end(),
                    [](const u16 value) { return value == 0x1111U; }
                ),
            "indexed-write fault preserves the cursor read and does not invent out-of-owner storage"
        );
    }

    {
        std::array<u16, 35> slots{};
        u16 cursor = 2U;
        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto result =
            openswd3::battle::write_legacy_battle_actor_effect_resource_slot(
                {.slots = &slots, .cursor = &cursor}, return_request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorEffectResourceSlotWriteStatus::
                        return_address_read_typed_stop &&
                slots[2U] == 0xBEEFU && result.return_eax == 2U &&
                result.return_edx == 0xCAFEBEEFU &&
                result.return_esp == return_request.entry_esp &&
                result.return_eip == 0x004787E6U &&
                result.stack_read_count == 1U &&
                result.return_address_reads == 0U && !result.returned,
            "return-address fault preserves the committed indexed write without advancing ESP"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor{};
        actor.effect_resource_cursor = 4U;
        openswd3::battle::LegacyBattleActorEffectResourceSlotWriteCallTrace
            trace{};
        openswd3::battle::LegacyBattleActorEffectResourceSlotWriteCallRequests
            requests{};
        requests.calls[0U].entry_esp = 0x90000000U;
        const bool completed = openswd3::battle::
            execute_legacy_battle_actor_effect_resource_slot_write_call(
                &actor,
                trace,
                requests,
                0x005029D0U,
                0x246FU,
                0x11112222U,
                0x33334444U,
                0x00453B85U,
                0x00453B8AU,
                {},
                true,
                true
            );
        const bool addresses_match = std::equal(
            openswd3::battle::
                kLegacyBattleActorEffectResourceSlotWriteCallerAddresses
                    .begin(),
            openswd3::battle::
                kLegacyBattleActorEffectResourceSlotWriteCallerAddresses.end(),
            openswd3::battle::
                kLegacyBattleActorEffectResourceSlotWriteReturnAddresses
                    .begin(),
            [](const u32 call, const u32 returned) {
                return returned == call + 5U;
            }
        );
        test.expect_true(
            completed && actor.effect_resource_slots[4U] == 0x246FU &&
                trace.calls == 1U && trace.call_addresses[0U] == 0x00453B85U &&
                trace.return_addresses[0U] == 0x00453B8AU &&
                trace.last.return_eax == 4U &&
                trace.last.return_edx == 0x3333246FU &&
                trace.last.return_esp == 0x90000008U && addresses_match,
            "typed call helper preserves physical CALL/return identities for all forty callers"
        );
    }
}
