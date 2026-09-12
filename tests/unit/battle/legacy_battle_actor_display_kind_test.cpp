#include "openswd3/battle/legacy_battle_actor_display_kind.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorDisplayKindRequest;
using openswd3::battle::LegacyBattleActorDisplayKindStatus;
using openswd3::battle::LegacyBattleActorDisplayKindView;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorDisplayKindRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00453A13U,
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

void test_battle_actor_display_kind(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        startup.party[2U].item_effect_application.display_kind = 0x1122U;
        (*startup.group_b_lifecycle)[3U].action_composition.display_kind =
            0x5566U;
        const auto owners =
            openswd3::battle::LegacyBattleActorDisplayKindOwners{
                .startup = &startup,
            };
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_display_kind(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_display_kind(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    3U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_display_kind(
                {},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_display_kind(
                owners, 0xDEADBEEFU
            );
        test.expect_true(
            group_a.display_kind ==
                    &startup.party[2U].item_effect_application.display_kind &&
                *group_a.display_kind == 0x1122U &&
                group_b.display_kind ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_composition.display_kind &&
                *group_b.display_kind == 0x5566U &&
                missing.display_kind == nullptr &&
                invalid.display_kind == nullptr,
            "display-kind resolver aliases only canonical startup Group-A item-effect and Group-B lifecycle owners"
        );
    }

    {
        const u16 display_kind = 0xCDEFU;
        const auto entry = request();
        const auto result =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                {.display_kind = &display_kind}, entry
            );
        test.expect_true(
            result.status == LegacyBattleActorDisplayKindStatus::completed &&
                result.returned && result.return_eax == 0xA5A5CDEFU &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.field_token == entry.actor_token + 0x2A70U &&
                result.display_kind_reads == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 1U &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known &&
                flags_equal(result.flags, entry.entry_flags),
            "display-kind getter replaces only AX and preserves the plain-RET machine state and flags"
        );
    }

    {
        const auto entry = request();
        const u16 zero = 0U;
        const u16 one = 1U;
        const auto zero_result =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                {.display_kind = &zero}, entry
            );
        const auto one_result =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                {.display_kind = &one}, entry
            );
        test.expect_true(
            zero_result.return_eax == 0xA5A50000U &&
                one_result.return_eax == 0xA5A50001U &&
                flags_equal(zero_result.flags, entry.entry_flags) &&
                flags_equal(one_result.flags, entry.entry_flags),
            "zero and one display kinds replace AX without zero extension, sign extension, or synthetic flags"
        );
    }

    {
        const u16 display_kind = 0xBEEFU;
        auto field_request = request();
        field_request.access.display_kind_readable = false;
        const auto field_stop =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                {.display_kind = &display_kind}, field_request
            );
        const auto missing_stop =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                LegacyBattleActorDisplayKindView{}, request()
            );

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::query_legacy_battle_actor_display_kind(
                {.display_kind = &display_kind}, return_request
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorDisplayKindStatus::
                        display_kind_read_typed_stop &&
                field_stop.return_eax == field_request.entry_eax &&
                field_stop.return_ecx == field_request.actor_token &&
                field_stop.return_edx == field_request.entry_edx &&
                field_stop.return_esp == field_request.entry_esp &&
                field_stop.return_eip == 0x004786C0U &&
                field_stop.display_kind_reads == 0U &&
                field_stop.return_address_reads == 0U && !field_stop.returned &&
                missing_stop.status ==
                    LegacyBattleActorDisplayKindStatus::
                        display_kind_read_typed_stop &&
                return_stop.status ==
                    LegacyBattleActorDisplayKindStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eax == 0xA5A5BEEFU &&
                return_stop.return_ecx == return_request.actor_token &&
                return_stop.return_edx == return_request.entry_edx &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x004786C7U &&
                return_stop.display_kind_reads == 1U &&
                return_stop.return_address_reads == 0U &&
                return_stop.stack_read_count == 0U && !return_stop.returned &&
                flags_equal(field_stop.flags, field_request.entry_flags) &&
                flags_equal(return_stop.flags, return_request.entry_flags),
            "field and RET stops preserve exact read prefix, partial EAX, registers, stack, EIP, and flags"
        );
    }
}
