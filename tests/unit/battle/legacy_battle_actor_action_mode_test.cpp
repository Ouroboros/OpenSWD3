#include "openswd3/battle/legacy_battle_actor_action_mode.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorActionModeAccess;
using openswd3::battle::LegacyBattleActorActionModeRequest;
using openswd3::battle::LegacyBattleActorActionModeStatus;
using openswd3::battle::LegacyBattleActorActionModeView;
using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorActionModeRequest request(const u32 mode) {
    return {
        .actor_token = 0x005029D0U,
        .mode = mode,
        .entry_eax = 0xA5A51234U,
        .entry_edx = 0x33334444U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x00453B15U,
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
subtract_flags(const u32 lhs, const u32 rhs) noexcept {
    const u32 value = lhs - rhs;
    return {
        .carry = lhs < rhs,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((lhs ^ rhs ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((lhs ^ rhs) & (lhs ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u8 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80U) != 0U,
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

void test_battle_actor_action_mode(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const auto owners = openswd3::battle::LegacyBattleActorActionModeOwners{
            .action = &action,
            .startup = &startup,
        };
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_action_mode(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_action_mode(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
                    2U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupBStride
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_action_mode(
                {},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        test.expect_true(
            group_a.action_kind ==
                    &action.group_a_action_execution[1U].action_kind &&
                group_a.display_kind ==
                    &startup.party[1U].item_effect_application.display_kind &&
                group_a.mode_flags ==
                    &startup.party[1U].item_effect_application.mode_flags &&
                group_b.action_kind ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_composition.action_kind &&
                group_b.display_kind ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_composition.display_kind &&
                group_b.mode_flags ==
                    &(*startup.group_b_lifecycle)[2U]
                         .action_composition.mode_flags &&
                missing.action_kind == nullptr &&
                missing.display_kind == nullptr &&
                missing.mode_flags == nullptr,
            "action-mode resolver aliases existing split Group-A owners and unified Group-B lifecycle owner"
        );
    }

    struct BranchCase {
        u32 mode;
        u16 action_kind;
        u16 display_kind;
        u8 mode_flags;
        u32 mode_reads;
        u32 mode_writes;
        u32 actor_accesses;
        LegacyBattleActorCoordinateFlags flags;
    };
    constexpr u8 initial_mode_flags = 0x91U;
    const std::array<BranchCase, 8> cases{{
        {0x12345678U,
         0x5678U,
         0xBEEFU,
         initial_mode_flags,
         0U,
         0U,
         1U,
         subtract_flags(0x12345678U, 15U)},
        {2U, 0U, 2U, initial_mode_flags, 0U, 0U, 2U, subtract_flags(2U, 2U)},
        {13U,
         0U,
         13U,
         initial_mode_flags,
         0U,
         0U,
         2U,
         subtract_flags(13U, 13U)},
        {3U, 0U, 3U, initial_mode_flags, 0U, 0U, 2U, subtract_flags(3U, 3U)},
        {14U,
         0U,
         14U,
         static_cast<u8>(initial_mode_flags | 0x40U),
         1U,
         1U,
         4U,
         logical_flags(static_cast<u8>(initial_mode_flags | 0x40U))},
        {6U,
         0U,
         6U,
         static_cast<u8>(initial_mode_flags | 0x08U),
         1U,
         1U,
         4U,
         logical_flags(static_cast<u8>(initial_mode_flags | 0x08U))},
        {15U,
         0U,
         15U,
         static_cast<u8>(initial_mode_flags | 0x40U),
         1U,
         1U,
         4U,
         logical_flags(static_cast<u8>(initial_mode_flags | 0x40U))},
        {0x00010002U,
         2U,
         0xBEEFU,
         initial_mode_flags,
         0U,
         0U,
         1U,
         subtract_flags(0x00010002U, 15U)},
    }};

    for (const BranchCase& expected : cases) {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = initial_mode_flags;
        const auto entry = request(expected.mode);
        const auto result =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                entry
            );
        test.expect_true(
            result.status == LegacyBattleActorActionModeStatus::completed &&
                result.returned && action_kind == expected.action_kind &&
                display_kind == expected.display_kind &&
                mode_flags == expected.mode_flags && result.return_eax == 1U &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == entry.entry_edx &&
                result.return_esp == entry.entry_esp + 8U &&
                result.return_eip == entry.entry_return_address &&
                result.argument_token == entry.entry_esp + 4U &&
                result.mode_flags_token == entry.actor_token + 0x2A87U &&
                result.display_kind_token == entry.actor_token + 0x2A70U &&
                result.action_kind_token == entry.actor_token + 0x2A6CU &&
                result.argument_reads == 1U &&
                result.mode_flags_reads == expected.mode_reads &&
                result.mode_flags_writes == expected.mode_writes &&
                result.display_kind_writes ==
                    (expected.mode == 2U || expected.mode == 13U ||
                             expected.mode == 3U || expected.mode == 14U ||
                             expected.mode == 6U || expected.mode == 15U
                         ? 1U
                         : 0U) &&
                result.action_kind_writes == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 2U &&
                result.stack_read_tokens[0U] == entry.entry_esp + 4U &&
                result.stack_reads[0U] == expected.mode &&
                result.stack_read_tokens[1U] == entry.entry_esp &&
                result.stack_reads[1U] == entry.entry_return_address &&
                result.actor_access_count == expected.actor_accesses &&
                result.flags_known && flags_equal(result.flags, expected.flags),
            "all default and special action-mode branches preserve writes, ABI, stack reads, and final flags"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto argument_stop_request = request(14U);
        argument_stop_request.access.argument_readable = false;
        argument_stop_request.entry_flags_known = false;
        const auto argument_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                argument_stop_request
            );
        test.expect_true(
            argument_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        argument_read_typed_stop &&
                action_kind == 0xCAFEU && display_kind == 0xBEEFU &&
                mode_flags == 0x11U &&
                argument_stop.return_eax == argument_stop_request.entry_eax &&
                argument_stop.return_ecx == argument_stop_request.actor_token &&
                argument_stop.return_edx == argument_stop_request.entry_edx &&
                argument_stop.return_esp == argument_stop_request.entry_esp &&
                argument_stop.return_eip == 0x00478710U &&
                argument_stop.stack_read_count == 0U &&
                !argument_stop.flags_known && !argument_stop.returned,
            "argument read stop preserves the complete entry machine state and actor bytes"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto default_stop_request = request(0xABCD1234U);
        default_stop_request.access.action_kind_writable = false;
        const auto default_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                default_stop_request
            );
        const auto missing_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                LegacyBattleActorActionModeView{}, request(0xABCD1234U)
            );
        test.expect_true(
            default_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        action_kind_write_typed_stop &&
                missing_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        action_kind_write_typed_stop &&
                action_kind == 0xCAFEU && display_kind == 0xBEEFU &&
                mode_flags == 0x11U &&
                default_stop.return_eax == default_stop_request.mode &&
                default_stop.return_eip == 0x00478732U &&
                default_stop.return_esp == default_stop_request.entry_esp &&
                default_stop.argument_reads == 1U &&
                default_stop.actor_access_count == 0U &&
                default_stop.stack_read_count == 1U &&
                flags_equal(
                    default_stop.flags,
                    subtract_flags(default_stop_request.mode, 15U)
                ) &&
                !default_stop.returned,
            "default action-kind write stop occurs after all six CMP instructions without committing a field"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto mode_read_request = request(14U);
        mode_read_request.access.mode_flags_readable = false;
        const auto mode_read_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                mode_read_request
            );
        auto mode_write_request = request(14U);
        mode_write_request.access.mode_flags_writable = false;
        const auto mode_write_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                mode_write_request
            );
        test.expect_true(
            mode_read_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        mode_flags_read_typed_stop &&
                mode_write_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        mode_flags_write_typed_stop &&
                mode_read_stop.return_eip == 0x0047874FU &&
                mode_write_stop.return_eip == 0x0047874FU &&
                mode_read_stop.actor_access_count == 0U &&
                mode_write_stop.actor_access_count == 1U &&
                mode_write_stop.actor_accesses[0U] ==
                    LegacyBattleActorActionModeAccess::mode_flags_read &&
                mode_write_stop.mode_flags_reads == 1U &&
                mode_write_stop.mode_flags_writes == 0U &&
                mode_flags == 0x11U && action_kind == 0xCAFEU &&
                display_kind == 0xBEEFU &&
                flags_equal(mode_read_stop.flags, subtract_flags(14U, 6U)) &&
                flags_equal(mode_write_stop.flags, subtract_flags(14U, 6U)),
            "mode-byte RMW read and write stops preserve the exact committed prefix and CMP flags"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto display_stop_request = request(6U);
        display_stop_request.access.display_kind_writable = false;
        const auto display_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                display_stop_request
            );
        test.expect_true(
            display_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        display_kind_write_typed_stop &&
                mode_flags == 0x19U && display_kind == 0xBEEFU &&
                action_kind == 0xCAFEU &&
                display_stop.return_eip == 0x00478756U &&
                display_stop.actor_access_count == 2U &&
                display_stop.actor_accesses[0U] ==
                    LegacyBattleActorActionModeAccess::mode_flags_read &&
                display_stop.actor_accesses[1U] ==
                    LegacyBattleActorActionModeAccess::mode_flags_write &&
                display_stop.mode_flags_reads == 1U &&
                display_stop.mode_flags_writes == 1U &&
                display_stop.display_kind_writes == 0U &&
                flags_equal(display_stop.flags, logical_flags(0x19U)),
            "display-kind fault preserves the preceding mode-byte OR write and OR flags"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto action_stop_request = request(15U);
        action_stop_request.access.action_kind_writable = false;
        const auto action_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                action_stop_request
            );
        test.expect_true(
            action_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        action_kind_write_typed_stop &&
                mode_flags == 0x51U && display_kind == 15U &&
                action_kind == 0xCAFEU &&
                action_stop.return_eip == 0x0047875DU &&
                action_stop.actor_access_count == 3U &&
                action_stop.actor_accesses[2U] ==
                    LegacyBattleActorActionModeAccess::display_kind_write &&
                action_stop.display_kind_writes == 1U &&
                action_stop.action_kind_writes == 0U &&
                flags_equal(action_stop.flags, logical_flags(0x51U)),
            "special action-kind clear fault preserves prior mode and display writes"
        );
    }

    {
        constexpr std::array<u32, 41> caller_return_addresses{
            0x00453B15U, 0x00453C93U, 0x00453F8BU, 0x00455053U, 0x004551C8U,
            0x0045550CU, 0x004558F9U, 0x00455951U, 0x00455AE8U, 0x00455C1BU,
            0x00455CBBU, 0x00456080U, 0x0045652BU, 0x00456BA6U, 0x00456C8EU,
            0x00456E47U, 0x00457C68U, 0x00457CA0U, 0x00457E34U, 0x0045AEECU,
            0x004671F9U, 0x0046B550U, 0x0046B5E5U, 0x0046B6B7U, 0x0046B75FU,
            0x0046B87EU, 0x0046B900U, 0x0046DCC9U, 0x004761BAU, 0x004761F4U,
            0x00476246U, 0x004762C3U, 0x004762DAU, 0x004803FAU, 0x0048055AU,
            0x0048058CU, 0x0048066BU, 0x00480691U, 0x004809F9U, 0x00480A16U,
            0x00480A53U,
        };
        u16 action_kind{};
        u16 display_kind{};
        u8 mode_flags{};
        bool identities_match = true;
        for (std::size_t index = 0U; index < caller_return_addresses.size();
             ++index) {
            auto caller_request = request(static_cast<u32>(0x100U + index));
            caller_request.entry_return_address =
                caller_return_addresses[index];
            const auto caller =
                openswd3::battle::set_legacy_battle_actor_action_mode(
                    {
                        .action_kind = &action_kind,
                        .display_kind = &display_kind,
                        .mode_flags = &mode_flags,
                    },
                    caller_request
                );
            identities_match = identities_match &&
                caller.status == LegacyBattleActorActionModeStatus::completed &&
                caller.returned &&
                caller.return_eip == caller_return_addresses[index] &&
                caller.stack_reads[1U] == caller_return_addresses[index];
        }
        test.expect_true(
            identities_match,
            "all forty-one physical callers preserve their distinct return identities"
        );
    }

    {
        u16 action_kind = 0xCAFEU;
        u16 display_kind = 0xBEEFU;
        u8 mode_flags = 0x11U;
        auto return_stop_request = request(13U);
        return_stop_request.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::set_legacy_battle_actor_action_mode(
                {
                    .action_kind = &action_kind,
                    .display_kind = &display_kind,
                    .mode_flags = &mode_flags,
                },
                return_stop_request
            );
        test.expect_true(
            return_stop.status ==
                    LegacyBattleActorActionModeStatus::
                        return_address_read_typed_stop &&
                action_kind == 0U && display_kind == 13U &&
                mode_flags == 0x11U && return_stop.return_eax == 1U &&
                return_stop.return_ecx == return_stop_request.actor_token &&
                return_stop.return_edx == return_stop_request.entry_edx &&
                return_stop.return_esp == return_stop_request.entry_esp &&
                return_stop.return_eip == 0x0047876BU &&
                return_stop.stack_read_count == 1U &&
                return_stop.return_address_reads == 0U &&
                flags_equal(return_stop.flags, subtract_flags(13U, 13U)) &&
                !return_stop.returned,
            "RETN 4 return-address fault preserves completed actor writes and leaves ESP before both pops"
        );
    }
}
