#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"
#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorField26b8HighBitQueryAccess;
using openswd3::battle::LegacyBattleActorField26b8HighBitQueryRequest;
using openswd3::battle::LegacyBattleActorField26b8HighBitQueryStatus;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorField26b8HighBitQueryRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x0045340EU,
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

void test_battle_actor_field_26b8_high_bit_query(
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
        const auto query_owners =
            openswd3::battle::LegacyBattleActorField26b8HighBitQueryOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto clear_owners =
            openswd3::battle::LegacyBattleActorField26b8HighBitClearOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto set_owners =
            openswd3::battle::LegacyBattleActorField26b8HighBitSetOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto group_a = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_query(
                query_owners, group_a_token
            );
        const auto group_b = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_query(
                query_owners, group_b_token
            );
        const auto group_a_clear = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                clear_owners, group_a_token
            );
        const auto group_b_clear = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                clear_owners, group_b_token
            );
        const auto group_a_set = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_set(
                set_owners, group_a_token
            );
        const auto group_b_set = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_set(
                set_owners, group_b_token
            );
        const auto invalid = openswd3::battle::
            resolve_legacy_battle_actor_field_26b8_high_bit_query(
                query_owners, group_a_token + 1U
            );
        test.expect_true(
            group_a.field_26b8 == group_a_clear.field_26b8 &&
                group_a.field_26b8 == group_a_set.field_26b8 &&
                group_b.field_26b8 == group_b_clear.field_26b8 &&
                group_b.field_26b8 == group_b_set.field_26b8 &&
                invalid.field_26b8 == nullptr,
            "field-26b8 query aliases the clear and set canonical owners for both actor groups"
        );
    }

    struct Case {
        u32 value;
        u32 expected_eax;
        bool expected_carry;
    };
    constexpr std::array<Case, 4> cases{{
        {0x00000000U, 0U, false},
        {0x40000000U, 0U, true},
        {0x80000000U, 1U, false},
        {0xC0000000U, 1U, true},
    }};
    for (const auto& expected : cases) {
        u32 field = expected.value;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorField26b8HighBitQueryStatus::completed &&
                result.returned && field == expected.value &&
                result.field_read_value == expected.value &&
                result.return_eax == expected.expected_eax &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x26B8U &&
                result.return_address_token == entry.entry_esp &&
                result.field_reads == 1U && result.actor_access_count == 1U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorField26b8HighBitQueryAccess::field_read &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_read_tokens[0U] == entry.entry_esp &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                result.flags.carry == expected.expected_carry &&
                result.flags.parity == (expected.expected_eax == 0U) &&
                !result.flags.auxiliary_carry &&
                !result.flags.auxiliary_carry_defined &&
                result.flags.zero == (expected.expected_eax == 0U) &&
                !result.flags.sign && !result.flags.overflow &&
                !result.overflow_defined,
            "field-26b8 query returns bit 31 and preserves SHR 31 flags and plain-RET state"
        );
    }

    {
        u32 field = 0xC0000000U;
        auto read_stop_request = request();
        read_stop_request.access.field_readable = false;
        read_stop_request.entry_flags_known = false;
        read_stop_request.entry_overflow_defined = false;
        const auto read_stop =
            openswd3::battle::query_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, read_stop_request
            );
        const auto missing_stop =
            openswd3::battle::query_legacy_battle_actor_field_26b8_high_bit(
                {}, request()
            );
        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorField26b8HighBitQueryStatus::
                        field_read_typed_stop &&
                missing_stop.status ==
                    LegacyBattleActorField26b8HighBitQueryStatus::
                        field_read_typed_stop &&
                read_stop.return_eax == read_stop_request.entry_eax &&
                read_stop.return_ecx == read_stop_request.actor_token &&
                read_stop.return_edx == read_stop_request.entry_edx &&
                read_stop.return_esp == read_stop_request.entry_esp &&
                read_stop.return_eip == 0x004787C0U &&
                read_stop.field_reads == 0U &&
                read_stop.actor_access_count == 0U && !read_stop.flags_known &&
                !read_stop.overflow_defined &&
                flags_equal(read_stop.flags, read_stop_request.entry_flags) &&
                !read_stop.returned,
            "field-26b8 read fault preserves the complete entry machine state"
        );
    }

    {
        u32 field = 0xC0000000U;
        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::query_legacy_battle_actor_field_26b8_high_bit(
                {.field_26b8 = &field}, return_stop_request
            );
        test.expect_true(
            return_stop.status ==
                    LegacyBattleActorField26b8HighBitQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.field_reads == 1U && return_stop.return_eax == 1U &&
                return_stop.return_ecx == return_stop_request.actor_token &&
                return_stop.return_edx == return_stop_request.entry_edx &&
                return_stop.return_esp == return_stop_request.entry_esp &&
                return_stop.return_eip == 0x004787C9U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && return_stop.flags_known &&
                return_stop.flags.carry && !return_stop.flags.parity &&
                !return_stop.flags.auxiliary_carry_defined &&
                !return_stop.flags.zero && !return_stop.flags.sign &&
                !return_stop.flags.overflow && !return_stop.overflow_defined &&
                !return_stop.returned,
            "plain-RET fault preserves the completed field read and SHR result without advancing ESP"
        );
    }

    test.expect_true(
        openswd3::battle::kLegacyBattleActorField26b8HighBitQueryAddress ==
                0x004787C0U &&
            openswd3::battle::
                    kLegacyBattleActorField26b8HighBitQueryCallerParentAddress ==
                0x00453200U &&
            openswd3::battle::
                    kLegacyBattleActorField26b8HighBitQueryCallerAddress ==
                0x00453409U &&
            openswd3::battle::
                    kLegacyBattleActorField26b8HighBitQueryCallerReturnAddress ==
                0x0045340EU,
        "the sole physical caller and return identities remain fixed"
    );
}
