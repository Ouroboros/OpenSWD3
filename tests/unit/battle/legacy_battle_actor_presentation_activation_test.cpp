#include "openswd3/battle/legacy_battle_actor_presentation_activation.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorPresentationActivationAccess;
using openswd3::battle::LegacyBattleActorPresentationActivationRequest;
using openswd3::battle::LegacyBattleActorPresentationActivationStatus;
using openswd3::battle::LegacyBattleActorPresentationActivationView;
using openswd3::compat::u8;
using openswd3::compat::u32;

struct ActorBacking {
    u32 special_ready{0xAAAAAAAAU};
    u8 marker{};
    u32 presentation_enabled{};
    u32 source_runtime_value{1U};
    u32 live_record_token{0x70000000U};
    u32 live_record_value_04{0xCAFE1234U};

    [[nodiscard]] LegacyBattleActorPresentationActivationView view() noexcept {
        return {
            .special_ready = &special_ready,
            .marker = &marker,
            .presentation_enabled = &presentation_enabled,
            .source_runtime_value = &source_runtime_value,
            .live_record_token = &live_record_token,
            .live_record_value_04 = &live_record_value_04,
        };
    }
};

[[nodiscard]] LegacyBattleActorPresentationActivationRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .value = 0x12345678U,
        .entry_eax = 0xA5A5A5A5U,
        .entry_edx = 0xB6B6B6B6U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x004540E9U,
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

void test_battle_actor_presentation_activation(openswd3::test::Context& test) {
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
            openswd3::battle::LegacyBattleActorPresentationActivationOwners{
                .action = action.get(),
                .startup = startup.get(),
            };
        const auto group_a = openswd3::battle::
            resolve_legacy_battle_actor_presentation_activation(
                owners, group_a_token
            );
        const auto group_b = openswd3::battle::
            resolve_legacy_battle_actor_presentation_activation(
                owners, group_b_token
            );
        const auto invalid = openswd3::battle::
            resolve_legacy_battle_actor_presentation_activation(
                owners, group_a_token + 1U
            );
        test.expect_true(
            group_a.special_ready ==
                    &startup->party[1U].progress.special_ready &&
                group_a.marker ==
                    &startup->party[1U].base_initialization.field_2a94 &&
                group_a.presentation_enabled ==
                    &startup->party[1U].progress.presentation_enabled &&
                group_a.source_runtime_value ==
                    &startup->party[1U].configuration.source_runtime_value &&
                group_a.live_record_token ==
                    &startup->party[1U].configuration.actor_record_token &&
                group_a.live_record_value_04 ==
                    &startup->party[1U].configuration.actor_record[2U] &&
                group_b.special_ready ==
                    &(*startup->group_b_lifecycle)[2U]
                         .action_configuration.special_ready &&
                group_b.marker ==
                    &(*startup->group_b_lifecycle)[2U]
                         .base_initialization.field_2a94 &&
                group_b.presentation_enabled ==
                    &(*startup->group_b_lifecycle)[2U]
                         .action_configuration.presentation_enabled &&
                group_b.live_record_token ==
                    &(*startup->group_b_lifecycle)[2U].live_record_token &&
                group_b.live_record_value_04 ==
                    &(*startup->group_b_lifecycle)[2U].live_record_value_04 &&
                invalid.special_ready == nullptr,
            "presentation activation resolves the canonical Group-A and Group-B actor fields"
        );
    }

    {
        ActorBacking actor{};
        const auto entry = request();
        const auto result =
            openswd3::battle::activate_legacy_battle_actor_presentation(
                actor.view(), entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorPresentationActivationStatus::completed &&
                result.returned && actor.special_ready == entry.value &&
                actor.marker == 6U && actor.presentation_enabled == 1U &&
                actor.live_record_value_04 == 0xCAFE0000U &&
                result.return_eax == 1U &&
                result.return_ecx == actor.live_record_token &&
                result.return_edx == 0x12345600U &&
                result.return_esp == entry.entry_esp + 8U &&
                result.return_eip == entry.entry_return_address &&
                result.argument_reads == 1U &&
                result.special_ready_writes == 1U &&
                result.marker_reads == 1U && result.marker_writes == 1U &&
                result.presentation_enabled_writes == 1U &&
                result.source_runtime_value_reads == 1U &&
                result.live_record_token_reads == 1U &&
                result.live_record_value_writes == 1U &&
                result.return_address_reads == 1U &&
                result.actor_access_count == 7U &&
                result.actor_accesses[0U] ==
                    LegacyBattleActorPresentationActivationAccess::
                        special_ready_write &&
                result.actor_accesses[6U] ==
                    LegacyBattleActorPresentationActivationAccess::
                        live_record_value_write &&
                result.stack_read_count == 2U &&
                result.stack_read_tokens[0U] == entry.entry_esp + 4U &&
                result.stack_reads[0U] == entry.value &&
                result.stack_read_tokens[1U] == entry.entry_esp &&
                result.stack_reads[1U] == entry.entry_return_address &&
                result.flags_known && !result.flags.zero &&
                !result.flags.carry && !result.flags.overflow,
            "sub_4787F0 preserves write order, original marker in DL, TEST flags, and RETN 4 state"
        );
    }

    {
        constexpr std::array<u8, 4> markers{0U, 1U, 0x80U, 0xFFU};
        constexpr std::array<u32, 4> values{0U, 1U, 0x12345678U, 0xFFFFFFFFU};
        bool exact = true;
        for (std::size_t index = 0U; index < markers.size(); ++index) {
            ActorBacking actor{};
            actor.marker = markers[index];
            actor.source_runtime_value = 2U;
            auto entry = request();
            entry.value = values[index];
            const auto result =
                openswd3::battle::activate_legacy_battle_actor_presentation(
                    actor.view(), entry
                );
            exact = exact && actor.special_ready == values[index] &&
                actor.marker == (markers[index] == 0U ? 6U : markers[index]) &&
                result.return_edx ==
                    ((values[index] & 0xFFFFFF00U) | markers[index]) &&
                result.return_ecx == entry.actor_token &&
                result.source_runtime_value == 2U &&
                result.live_record_token_reads == 0U &&
                result.live_record_value_writes == 0U && result.flags_known &&
                !result.flags.zero;
        }
        test.expect_true(
            exact,
            "full dword arguments and original marker bytes preserve width and branch behavior"
        );
    }

    {
        constexpr std::array<u32, 4> gates{0U, 1U, 2U, 0xFFFFFFFFU};
        bool exact = true;
        for (const u32 gate : gates) {
            ActorBacking actor{};
            actor.source_runtime_value = gate;
            const auto result =
                openswd3::battle::activate_legacy_battle_actor_presentation(
                    actor.view(), request()
                );
            const bool entered_live_record = gate == 1U;
            exact = exact && result.source_runtime_value == gate &&
                result.live_record_token_reads ==
                    static_cast<u32>(entered_live_record) &&
                result.live_record_value_writes ==
                    static_cast<u32>(entered_live_record) &&
                actor.live_record_value_04 ==
                    (entered_live_record ? 0xCAFE0000U : 0xCAFE1234U);
        }
        test.expect_true(
            exact,
            "only the full dword source value one enters the live-record path"
        );
    }

    {
        ActorBacking actor{};
        actor.marker = 0x80U;
        actor.live_record_token = 0U;
        const auto result =
            openswd3::battle::activate_legacy_battle_actor_presentation(
                actor.view(), request()
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorPresentationActivationStatus::completed &&
                result.return_ecx == 0U && result.flags.zero &&
                actor.marker == 0x80U && result.marker_writes == 0U &&
                result.live_record_token_reads == 1U &&
                result.live_record_value_writes == 0U &&
                actor.live_record_value_04 == 0xCAFE1234U,
            "zero live-record tokens stop before the record word write and retain TEST flags"
        );
    }

    {
        ActorBacking actor{};
        const auto baseline = request();
        constexpr std::array<
            LegacyBattleActorPresentationActivationStatus,
            9>
            statuses{
                LegacyBattleActorPresentationActivationStatus::
                    argument_read_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    special_ready_write_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    marker_read_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    presentation_enabled_write_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    marker_write_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    source_runtime_value_read_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    live_record_token_read_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    live_record_value_write_typed_stop,
                LegacyBattleActorPresentationActivationStatus::
                    return_address_read_typed_stop,
            };
        constexpr std::array<u32, 9> instruction_pointers{
            0x004787F0U,
            0x004787F9U,
            0x004787FFU,
            0x00478807U,
            0x0047880FU,
            0x00478816U,
            0x0047881EU,
            0x00478825U,
            0x0047882BU,
        };
        constexpr std::array<u32, 9> committed_actor_accesses{
            0U, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U
        };
        bool exact = true;
        for (std::size_t stop = 0U; stop < statuses.size(); ++stop) {
            actor = {};
            auto entry = baseline;
            switch (stop) {
            case 0U:
                entry.access.argument_readable = false;
                break;

            case 1U:
                entry.access.special_ready_writable = false;
                break;

            case 2U:
                entry.access.marker_readable = false;
                break;

            case 3U:
                entry.access.presentation_enabled_writable = false;
                break;

            case 4U:
                entry.access.marker_writable = false;
                break;

            case 5U:
                entry.access.source_runtime_value_readable = false;
                break;

            case 6U:
                entry.access.live_record_token_readable = false;
                break;

            case 7U:
                entry.access.live_record_value_writable = false;
                break;

            case 8U:
                entry.access.return_address_readable = false;
                break;
            }
            const auto result =
                openswd3::battle::activate_legacy_battle_actor_presentation(
                    actor.view(), entry
                );
            exact = exact && result.status == statuses[stop] &&
                result.return_eip == instruction_pointers[stop] &&
                result.actor_access_count == committed_actor_accesses[stop] &&
                result.stack_read_count == (stop == 0U ? 0U : 1U) &&
                !result.returned && result.return_esp == entry.entry_esp;
            if (stop == 0U) {
                exact = exact &&
                    result.status ==
                        LegacyBattleActorPresentationActivationStatus::
                            argument_read_typed_stop &&
                    result.return_eax == entry.entry_eax &&
                    result.return_edx == entry.entry_edx &&
                    flags_equal(result.flags, entry.entry_flags);
            }
            if (stop == 3U) {
                exact = exact && actor.special_ready == entry.value &&
                    result.return_edx == 0x12345600U && result.flags.zero &&
                    actor.presentation_enabled == 0U;
            }
            if (stop == 7U) {
                exact = exact && actor.marker == 6U &&
                    actor.presentation_enabled == 1U &&
                    result.return_ecx == actor.live_record_token &&
                    !result.flags.zero &&
                    actor.live_record_value_04 == 0xCAFE1234U;
            }
            if (stop == 8U) {
                exact = exact && actor.live_record_value_04 == 0xCAFE0000U &&
                    result.return_eip == 0x0047882BU &&
                    result.return_address_reads == 0U;
            }
        }
        test.expect_true(
            exact,
            "all nine physical memory accesses preserve their exact partial-commit prefixes"
        );
    }

    {
        const bool addresses_match = std::equal(
            openswd3::battle::
                kLegacyBattleActorPresentationActivationCallerAddresses.begin(),
            openswd3::battle::
                kLegacyBattleActorPresentationActivationCallerAddresses.end(),
            openswd3::battle::
                kLegacyBattleActorPresentationActivationReturnAddresses.begin(),
            [](const u32 call, const u32 returned) {
                return returned == call + 5U;
            }
        );
        test.expect_true(
            addresses_match,
            "all seven physical presentation-activation CALL identities retain five-byte return addresses"
        );
    }
}
