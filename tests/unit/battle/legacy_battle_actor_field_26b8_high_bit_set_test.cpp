#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_field_26c0.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>
#include <utility>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetAccess;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetCallRequests;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetCallTrace;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetRequest;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetStatus;
using openswd3::battle::LegacyBattleActorField26b8HighBitSetView;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorField26b8HighBitSetRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x004541C3U,
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

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    u8 bits = value;
    bool parity = true;
    while (bits != 0U) {
        parity = !parity;
        bits = static_cast<u8>(bits & static_cast<u8>(bits - 1U));
    }
    return parity;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
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

struct Fields {
    u32 field_26c0{};
    u32 field_26b8{};
    u16 summon_completion_word{};
    u16 special_target_command_cursor{};

    [[nodiscard]] LegacyBattleActorField26b8HighBitSetView view() noexcept {
        return {
            .field_26c0 = &field_26c0,
            .field_26b8 = &field_26b8,
            .summon_completion_word = &summon_completion_word,
            .special_target_command_cursor = &special_target_command_cursor,
        };
    }
};

}  // namespace

void test_battle_actor_field_26b8_high_bit_set(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActorField26c0 owner{0x11223344U};
        auto copy = owner;
        auto moved = std::move(copy);
        moved = 0x55667788U;
        test.expect_true(
            static_cast<u32>(owner) == 0x55667788U &&
                static_cast<u32>(copy) == 0x55667788U &&
                owner.data() == copy.data() && owner.data() == moved.data(),
            "field-26c0 copy and move views retain one shared dword backing"
        );
    }

    {
        const auto action = std::make_unique<
            openswd3::battle::LegacyBattleActionDispatchState>();
        const auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const auto owners =
            openswd3::battle::LegacyBattleActorField26b8HighBitSetOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const u32 group_a_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
            openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
        const u32 group_b_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
            2U * openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
        const auto group_a = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_set(
                owners, group_a_token
            );
        const auto group_b = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_set(
                owners, group_b_token
            );
        const auto invalid = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_set(
                owners, group_a_token + 1U
            );
        auto& group_a_owner = action->group_a_action_execution[1U];
        auto& group_b_owner =
            (*startup->group_b_lifecycle)[2U].action_execution;
        test.expect_true(
            group_a.field_26c0 == group_a_owner.field_26c0.data() &&
                group_a.field_26b8 == &group_a_owner.field_26b8 &&
                group_a.summon_completion_word ==
                    &group_a_owner.summon_completion_word &&
                group_a.special_target_command_cursor ==
                    &group_a_owner.special_target_action_record
                         .command_cursor &&
                startup->party[1U].progress.field_26c0.data() ==
                    group_a_owner.field_26c0.data() &&
                group_b.field_26c0 == group_b_owner.field_26c0.data() &&
                startup->enemies[2U].progress.field_26c0.data() ==
                    group_b_owner.field_26c0.data() &&
                invalid.field_26c0 == nullptr,
            "field-26b8 set resolver reuses both action owners and aliases every progress field-26c0 view"
        );
    }

    {
        Fields fields{
            .field_26c0 = 0x02000080U,
            .field_26b8 = 0x12345678U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U,
        };
        const auto entry = request();
        const auto result =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::completed &&
                result.gate_blocked && result.returned &&
                fields.field_26b8 == 0x12345678U &&
                fields.summon_completion_word == 0x1111U &&
                fields.special_target_command_cursor == 0x2222U &&
                result.return_eax == entry.entry_eax &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.actor_access_count == 1U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        field_26c0_read &&
                result.field_26b8_reads == 0U &&
                result.field_26b8_writes == 0U &&
                flags_equal(result.flags, logical_flags(0x02000000U)),
            "field-26c0 bit25 gate returns after TEST without touching later fields"
        );
    }

    {
        Fields fields{
            .field_26c0 = 0x00000080U,
            .field_26b8 = 0x12345678U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U,
        };
        const auto entry = request();
        const auto result =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::completed &&
                !result.high_bit_was_set && fields.field_26b8 == 0x92345678U &&
                fields.summon_completion_word == 0x1111U &&
                fields.special_target_command_cursor == 0x2222U &&
                result.return_eax == 0x92345678U &&
                result.return_edx == entry.entry_edx &&
                result.actor_access_count == 3U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        field_26c0_read &&
                result.actor_accesses[1U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        field_26b8_read &&
                result.actor_accesses[2U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        field_26b8_write &&
                flags_equal(result.flags, logical_flags(0x92345678U)),
            "clear input high bit skips both word writes but still commits the final dword OR"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x80000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U,
        };
        const auto entry = request();
        const auto result =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::completed &&
                result.high_bit_was_set && fields.field_26b8 == 0x80000005U &&
                fields.summon_completion_word == 0U &&
                fields.special_target_command_cursor == 0U &&
                result.return_eax == 0x80000005U && result.return_edx == 0U &&
                result.actor_access_count == 5U &&
                result.actor_accesses[2U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        summon_completion_word_write &&
                result.actor_accesses[3U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        special_target_command_cursor_write &&
                result.actor_accesses[4U] ==
                    LegacyBattleActorField26b8HighBitSetAccess::
                        field_26b8_write &&
                flags_equal(result.flags, logical_flags(0x80000005U)),
            "set input high bit clears both words in order and returns final OR flags"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x80000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U
        };
        auto gate_stop_request = request();
        gate_stop_request.access.field_26c0_readable = false;
        const auto gate_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), gate_stop_request
            );
        auto field_read_stop_request = request();
        field_read_stop_request.access.field_26b8_readable = false;
        const auto field_read_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), field_read_stop_request
            );
        test.expect_true(
            gate_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        field_26c0_read_typed_stop &&
                gate_stop.return_eip == 0x00478780U &&
                gate_stop.actor_access_count == 0U &&
                flags_equal(gate_stop.flags, gate_stop_request.entry_flags) &&
                field_read_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        field_26b8_read_typed_stop &&
                field_read_stop.return_eip == 0x0047878CU &&
                field_read_stop.actor_access_count == 1U &&
                flags_equal(field_read_stop.flags, logical_flags(0U)),
            "gate and field reads stop at their exact faulting instructions with committed prefix flags"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x80000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U
        };
        auto first_word_stop_request = request();
        first_word_stop_request.access.summon_completion_word_writable = false;
        const auto first_word_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), first_word_stop_request
            );
        test.expect_true(
            first_word_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        summon_completion_word_write_typed_stop &&
                first_word_stop.return_eip == 0x0047879BU &&
                first_word_stop.return_eax == 0x80000005U &&
                first_word_stop.return_edx == 0U &&
                fields.summon_completion_word == 0x1111U &&
                fields.special_target_command_cursor == 0x2222U &&
                fields.field_26b8 == 0x80000005U &&
                flags_equal(first_word_stop.flags, logical_flags(0U)),
            "first word fault preserves the read prefix and XOR register-flags state"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x80000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U
        };
        auto second_word_stop_request = request();
        second_word_stop_request.access.special_target_command_cursor_writable =
            false;
        const auto second_word_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), second_word_stop_request
            );
        test.expect_true(
            second_word_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        special_target_command_cursor_write_typed_stop &&
                second_word_stop.return_eip == 0x004787A2U &&
                fields.summon_completion_word == 0U &&
                fields.special_target_command_cursor == 0x2222U &&
                fields.field_26b8 == 0x80000005U &&
                second_word_stop.actor_access_count == 3U,
            "second word fault retains the first word write and suppresses the final dword write"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x80000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U
        };
        auto field_write_stop_request = request();
        field_write_stop_request.access.field_26b8_writable = false;
        const auto field_write_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), field_write_stop_request
            );
        test.expect_true(
            field_write_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        field_26b8_write_typed_stop &&
                field_write_stop.return_eip == 0x004787AEU &&
                fields.summon_completion_word == 0U &&
                fields.special_target_command_cursor == 0U &&
                fields.field_26b8 == 0x80000005U &&
                field_write_stop.return_eax == 0x80000005U &&
                flags_equal(field_write_stop.flags, logical_flags(0x80000005U)),
            "final dword fault retains both word writes plus OR EAX and flags without committing the dword"
        );
    }

    {
        Fields fields{
            .field_26b8 = 0x00000005U,
            .summon_completion_word = 0x1111U,
            .special_target_command_cursor = 0x2222U
        };
        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::set_legacy_battle_actor_field_26b8_high_bit(
                fields.view(), return_stop_request
            );
        test.expect_true(
            return_stop.status ==
                    LegacyBattleActorField26b8HighBitSetStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eip == 0x004787B4U &&
                return_stop.return_esp == return_stop_request.entry_esp &&
                fields.field_26b8 == 0x80000005U &&
                return_stop.return_address_reads == 0U && !return_stop.returned,
            "plain RET fault keeps all actor commits but does not read the stack or advance ESP"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor{};
        actor.field_26b8 = 5U;
        LegacyBattleActorField26b8HighBitSetCallTrace trace{};
        const LegacyBattleActorField26b8HighBitSetCallRequests requests{};
        for (const u32 caller : openswd3::battle::
                 kLegacyBattleActorField26b8HighBitSetClosedCallerAddresses) {
            const bool completed = openswd3::battle::
                execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                    &actor,
                    trace,
                    requests,
                    0x005029D0U,
                    0x11111111U,
                    0x22222222U,
                    caller + 5U,
                    {},
                    true
                );
            test.expect_true(
                completed, "each closed physical caller composes the typed leaf"
            );
        }
        bool return_addresses_match = trace.calls == 27U;
        for (u32 index = 0U; index < trace.calls; ++index) {
            return_addresses_match = return_addresses_match &&
                trace.return_addresses[index] ==
                    openswd3::battle::
                            kLegacyBattleActorField26b8HighBitSetClosedCallerAddresses
                                [index] +
                        5U;
        }
        test.expect_true(
            return_addresses_match &&
                openswd3::battle::
                        kLegacyBattleActorField26b8HighBitSetDeferredCallerAddresses
                            .size() == 8U &&
                openswd3::battle::
                        kLegacyBattleActorField26b8HighBitSetDeferredCallerAddresses
                            .front() == 0x00478BD6U &&
                openswd3::battle::
                        kLegacyBattleActorField26b8HighBitSetDeferredCallerAddresses
                            .back() == 0x00483F61U,
            "all 35 physical CALL identities remain split into 27 closed and eight deferred boundaries"
        );
    }
}
