#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorField26b8HighBitClearAccess;
using openswd3::battle::LegacyBattleActorField26b8HighBitClearRequest;
using openswd3::battle::LegacyBattleActorField26b8HighBitClearStatus;
using openswd3::battle::LegacyBattleActorField26b8HighBitClearView;
using openswd3::compat::u8;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorField26b8HighBitClearRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00478CCDU,
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

}  // namespace

void test_battle_actor_field_26b8_high_bit_clear(
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
        const auto owners =
            openswd3::battle::LegacyBattleActorField26b8HighBitClearOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto group_a = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto misaligned = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    1U
            );
        const auto invalid =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                misaligned, request()
            );
        test.expect_true(
            group_a.field_26b8 ==
                    &action->group_a_action_execution[1U].field_26b8 &&
                group_b.field_26b8 ==
                    &(*startup->group_b_lifecycle)[2U]
                         .action_execution.field_26b8 &&
                misaligned.field_26b8 == nullptr &&
                invalid.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::
                        field_read_typed_stop,
            "field-26b8 resolver aliases both canonical owners and stops an invalid token at the first field read"
        );
    }

    struct Case {
        u32 initial;
        u32 expected;
    };
    constexpr std::array<Case, 3> cases{{
        {0xFFFFFFFFU, 0x7FFFFFFFU},
        {0x80000000U, 0U},
        {0x12345678U, 0x12345678U},
    }};
    for (const auto& expected : cases) {
        u32 field = expected.initial;
        const auto entry = request();
        const auto result =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::completed &&
                result.returned && field == expected.expected &&
                result.field_read_value == expected.initial &&
                result.field_write_value == expected.expected &&
                result.return_eax == entry.entry_eax &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x26B8U &&
                result.return_address_token == entry.entry_esp &&
                result.field_reads == 1U && result.field_writes == 1U &&
                result.actor_access_count == 2U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorField26b8HighBitClearAccess::field_read &&
                result.actor_accesses[1U] ==
                    LegacyBattleActorField26b8HighBitClearAccess::field_write &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_read_tokens[0U] == entry.entry_esp &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                flags_equal(result.flags, logical_flags(expected.expected)),
            "field-26b8 clear always performs the dword RMW and preserves registers and plain-RET state"
        );
    }

    {
        u32 field = 0xFEDCBA98U;
        auto read_stop_request = request();
        read_stop_request.access.field_readable = false;
        read_stop_request.entry_flags_known = false;
        const auto read_stop =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, read_stop_request
            );
        const auto missing_stop =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                LegacyBattleActorField26b8HighBitClearView{}, request()
            );
        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::
                        field_read_typed_stop &&
                missing_stop.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::
                        field_read_typed_stop &&
                field == 0xFEDCBA98U && read_stop.field_reads == 0U &&
                read_stop.field_writes == 0U &&
                read_stop.actor_access_count == 0U &&
                read_stop.return_esp == read_stop_request.entry_esp &&
                read_stop.return_eip == 0x00478770U && !read_stop.flags_known &&
                !read_stop.returned,
            "field-26b8 read fault preserves the entry machine state with zero commit"
        );
    }

    {
        u32 field = 0x80000005U;
        auto write_stop_request = request();
        write_stop_request.access.field_writable = false;
        const auto write_stop =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, write_stop_request
            );
        test.expect_true(
            write_stop.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::
                        field_write_typed_stop &&
                field == 0x80000005U && write_stop.field_reads == 1U &&
                write_stop.field_writes == 0U &&
                write_stop.field_read_value == 0x80000005U &&
                write_stop.actor_access_count == 1U &&
                write_stop.actor_accesses[0U] ==
                    LegacyBattleActorField26b8HighBitClearAccess::field_read &&
                write_stop.return_esp == write_stop_request.entry_esp &&
                write_stop.return_eip == 0x00478770U &&
                flags_equal(write_stop.flags, write_stop_request.entry_flags) &&
                !write_stop.returned,
            "field-26b8 write fault keeps the read prefix but commits neither value nor AND flags"
        );
    }

    {
        u32 field = 0x80000000U;
        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, return_stop_request
            );
        test.expect_true(
            return_stop.status ==
                    LegacyBattleActorField26b8HighBitClearStatus::
                        return_address_read_typed_stop &&
                field == 0U && return_stop.field_reads == 1U &&
                return_stop.field_writes == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U &&
                return_stop.return_eax == return_stop_request.entry_eax &&
                return_stop.return_ecx == return_stop_request.actor_token &&
                return_stop.return_edx == return_stop_request.entry_edx &&
                return_stop.return_esp == return_stop_request.entry_esp &&
                return_stop.return_eip == 0x0047877AU &&
                flags_equal(return_stop.flags, logical_flags(0U)) &&
                !return_stop.returned,
            "plain-RET fault preserves the completed field write and AND flags without advancing ESP"
        );
    }

    test.expect_true(
        openswd3::battle::
                    kLegacyBattleActorField26b8HighBitClearDeferredParentAddress ==
                0x00478B60U &&
            openswd3::battle::
                    kLegacyBattleActorField26b8HighBitClearCallerAddress ==
                0x00478CC8U &&
            openswd3::battle::
                    kLegacyBattleActorField26b8HighBitClearCallerReturnAddress ==
                0x00478CCDU,
        "the sole physical caller and deferred Workpack-315 parent identities remain fixed"
    );
}
