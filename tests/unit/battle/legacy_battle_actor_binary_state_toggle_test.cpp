#include "openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorBinaryStateToggleAccessKind;
using openswd3::battle::LegacyBattleActorBinaryStateToggleRequest;
using openswd3::battle::LegacyBattleActorBinaryStateToggleStatus;
using openswd3::battle::LegacyBattleActorBinaryStateToggleView;
using openswd3::compat::u32;

struct ActorBacking {
    u32 value{};

    [[nodiscard]] LegacyBattleActorBinaryStateToggleView view() noexcept {
        return {.value = &value};
    }
};

[[nodiscard]] LegacyBattleActorBinaryStateToggleRequest request() noexcept {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0x11223344U,
        .entry_edx = 0x55667788U,
        .entry_esp = 0x0012FF00U,
        .entry_return_address = 0x0046B053U,
        .entry_flags =
            {
                .carry = true,
                .parity = false,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = true,
                .zero = false,
                .sign = true,
                .overflow = true,
            },
        .entry_flags_known = true,
    };
}

}  // namespace

void test_battle_actor_binary_state_toggle(openswd3::test::Context& test) {
    {
        const auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const auto owners =
            openswd3::battle::LegacyBattleActorBinaryStateToggleOwners{
                .startup = startup.get(),
            };
        const u32 group_a_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
            2U * openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
        const u32 group_b_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
            5U * openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_binary_state_toggle(
                owners, group_a_token
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_binary_state_toggle(
                owners, group_b_token
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_binary_state_toggle(
                owners,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
                    10U *
                        openswd3::battle::
                            kLegacyBattleActorCoordinatesGroupAStride
            );
        test.expect_true(
            group_a.value == &startup->party[2U].progress.script_binary_state &&
                group_b.value ==
                    &(*startup->group_b_lifecycle)[5U]
                         .action_configuration.script_binary_state &&
                invalid.value == nullptr,
            "binary-state toggle resolves only canonical Group-A and Group-B actor fields"
        );
    }

    {
        struct Vector {
            u32 old_value;
            u32 new_value;
            bool zero;
            bool sign;
            bool parity;
        };
        constexpr std::array<Vector, 6U> vectors{
            Vector{0U, 1U, true, false, true},
            Vector{1U, 0U, false, false, false},
            Vector{2U, 0U, false, false, false},
            Vector{0x00010000U, 0U, false, false, true},
            Vector{0x80000000U, 0U, false, true, true},
            Vector{0xFFFFFFFFU, 0U, false, true, true},
        };
        bool exact = true;
        for (const auto& vector : vectors) {
            ActorBacking actor{.value = vector.old_value};
            const auto entry = request();
            const auto result =
                openswd3::battle::toggle_legacy_battle_actor_binary_state(
                    actor.view(), entry
                );
            exact = exact && actor.value == vector.new_value &&
                result.status ==
                    LegacyBattleActorBinaryStateToggleStatus::completed &&
                result.old_value == vector.old_value &&
                result.new_value == vector.new_value &&
                result.return_eax == vector.new_value &&
                result.return_ecx == entry.actor_token &&
                result.return_edx == vector.old_value &&
                result.return_esp == entry.entry_esp + 8U &&
                result.return_eip == entry.entry_return_address &&
                result.actor_access_count == 2U &&
                result.actor_accesses[0U].kind ==
                    LegacyBattleActorBinaryStateToggleAccessKind::value_read &&
                result.actor_accesses[0U].value == vector.old_value &&
                result.actor_accesses[1U].kind ==
                    LegacyBattleActorBinaryStateToggleAccessKind::value_write &&
                result.actor_accesses[1U].value == vector.new_value &&
                result.stack_read_count == 1U &&
                result.stack_read_tokens[0U] == entry.entry_esp &&
                result.stack_reads[0U] == entry.entry_return_address &&
                result.flags_known && result.flags.zero == vector.zero &&
                result.flags.sign == vector.sign &&
                result.flags.parity == vector.parity && !result.flags.carry &&
                !result.flags.overflow &&
                !result.flags.auxiliary_carry_defined && result.returned;
        }
        test.expect_true(
            exact,
            "zero becomes one while every nonzero dword becomes zero with TEST flags and RETN 4 state"
        );
    }

    {
        ActorBacking actor{.value = 0x80000000U};
        auto read_entry = request();
        read_entry.access.value_readable = false;
        const auto read_stop =
            openswd3::battle::toggle_legacy_battle_actor_binary_state(
                actor.view(), read_entry
            );

        auto write_entry = request();
        write_entry.access.value_writable = false;
        const auto write_stop =
            openswd3::battle::toggle_legacy_battle_actor_binary_state(
                actor.view(), write_entry
            );

        actor.value = 0U;
        auto return_entry = request();
        return_entry.access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::toggle_legacy_battle_actor_binary_state(
                actor.view(), return_entry
            );

        test.expect_true(
            read_stop.status ==
                    LegacyBattleActorBinaryStateToggleStatus::
                        value_read_typed_stop &&
                read_stop.return_eip == 0x00478830U &&
                read_stop.return_eax == read_entry.entry_eax &&
                read_stop.return_edx == read_entry.entry_edx &&
                read_stop.actor_access_count == 0U &&
                read_stop.flags.carry == read_entry.entry_flags.carry &&
                write_stop.status ==
                    LegacyBattleActorBinaryStateToggleStatus::
                        value_write_typed_stop &&
                write_stop.return_eip == 0x0047883DU &&
                write_stop.return_eax == 0U &&
                write_stop.return_edx == 0x80000000U &&
                write_stop.actor_access_count == 1U &&
                return_stop.status ==
                    LegacyBattleActorBinaryStateToggleStatus::
                        return_address_read_typed_stop &&
                return_stop.return_eip == 0x00478843U && actor.value == 1U &&
                return_stop.actor_access_count == 2U &&
                return_stop.return_esp == return_entry.entry_esp &&
                return_stop.stack_read_count == 0U && !return_stop.returned,
            "all three memory boundaries preserve exact register, flag, and partial-write prefixes"
        );
    }

    {
        const auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        const auto owners =
            openswd3::battle::LegacyBattleActorBinaryStateToggleOwners{
                .startup = startup.get(),
            };
        openswd3::battle::LegacyBattleActorBinaryStateToggleCallTrace trace{};
        const openswd3::battle::LegacyBattleActorBinaryStateToggleCallRequests
            requests{};
        const u32 token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken +
            3U * openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
        const bool returned = openswd3::battle::
            execute_legacy_battle_actor_binary_state_toggle_call(
                owners,
                trace,
                requests,
                token,
                1U,
                0x10203040U,
                0x50607080U,
                0x0046B072U,
                0x0046B077U
            );
        test.expect_true(
            returned && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x0046B072U &&
                trace.return_addresses[0U] == 0x0046B077U &&
                trace.arguments[0U] == 1U && trace.last.return_ecx == token,
            "physical CALL identity retains its ignored argument and return address"
        );
    }
}
