#include "openswd3/battle/legacy_battle_actor_record_selection.hpp"
#include "test.hpp"

#include <array>

void test_battle_actor_record_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateFlags;
    using openswd3::battle::LegacyBattleActorRecordSelectionRequest;
    using openswd3::battle::LegacyBattleActorRecordSelectionStatus;
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::select_legacy_battle_actor_record;
    using openswd3::compat::u32;

    {
        LegacyBattleGroupAConfigurationState actor{
            .actor_record_token = 0x80000002U,
            .source_record_token = 0x80000001U,
        };
        const auto result = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 0U,
                .actor_token = 0x005029D0U,
                .entry_eax = 0xDEADBEEFU,
                .entry_edx = 0x13579BDFU,
                .entry_esp = 0x70001000U,
                .entry_return_address = 0x0048110EU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRecordSelectionStatus::completed &&
                result.return_eax == 0x80000001U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x13579BDFU &&
                result.return_esp == 0x70001008U &&
                result.return_eip == 0x0048110EU &&
                result.argument_reads == 1U && result.actor_field_reads == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 2U && result.stack_reads[0U] == 0U &&
                result.stack_reads[1U] == 0x0048110EU &&
                result.tests_executed == 2U && result.selected_source_record &&
                !result.selected_actor_record && !result.xor_zero_executed &&
                result.returned && result.flags_known && !result.flags.carry &&
                !result.flags.parity && !result.flags.zero &&
                result.flags.sign && !result.flags.overflow &&
                !result.flags.auxiliary_carry_defined,
            "zero argument selects only actor +4 and returns through 0x0047867F"
        );
    }

    {
        LegacyBattleGroupAConfigurationState actor{
            .actor_record_token = 0x00000002U,
            .source_record_token = 0x80000001U,
        };
        const auto result = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 0x80000000U,
                .actor_token = 0x00505904U,
                .entry_eax = 0xCAFEBABEU,
                .entry_edx = 0x2468ACE0U,
                .entry_esp = 0x70002000U,
                .entry_return_address = 0x00481AE2U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRecordSelectionStatus::completed &&
                result.return_eax == 2U && result.return_ecx == 0x00505904U &&
                result.return_edx == 0x2468ACE0U &&
                result.return_esp == 0x70002008U &&
                result.return_eip == 0x00481AE2U &&
                result.argument_reads == 1U && result.actor_field_reads == 1U &&
                result.return_address_reads == 1U &&
                result.stack_read_count == 2U &&
                result.stack_reads[0U] == 0x80000000U &&
                result.stack_reads[1U] == 0x00481AE2U &&
                result.tests_executed == 2U && result.selected_actor_record &&
                !result.selected_source_record && !result.xor_zero_executed &&
                result.returned && !result.flags.carry &&
                !result.flags.parity && !result.flags.zero &&
                !result.flags.sign && !result.flags.overflow &&
                !result.flags.auxiliary_carry_defined,
            "nonzero argument selects only actor +0 and preserves ECX and EDX"
        );
    }

    {
        LegacyBattleGroupAConfigurationState actor;
        const auto source = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 0U,
                .actor_token = 0x005029D0U,
                .entry_edx = 0x12345678U,
                .entry_esp = 0x70003000U,
                .entry_return_address = 0x11112222U,
            }
        );
        const auto primary = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 1U,
                .actor_token = 0x005029D0U,
                .entry_edx = 0x87654321U,
                .entry_esp = 0x70004000U,
                .entry_return_address = 0x33334444U,
            }
        );
        test.expect_true(
            source.status ==
                    LegacyBattleActorRecordSelectionStatus::completed &&
                primary.status ==
                    LegacyBattleActorRecordSelectionStatus::completed &&
                source.return_eax == 0U && primary.return_eax == 0U &&
                source.return_edx == 0x12345678U &&
                primary.return_edx == 0x87654321U &&
                source.return_esp == 0x70003008U &&
                primary.return_esp == 0x70004008U &&
                source.selected_source_record &&
                primary.selected_actor_record && source.xor_zero_executed &&
                primary.xor_zero_executed && source.tests_executed == 2U &&
                primary.tests_executed == 2U && source.flags.parity &&
                primary.flags.parity && source.flags.zero &&
                primary.flags.zero && !source.flags.sign &&
                !primary.flags.sign && !source.flags.carry &&
                !primary.flags.carry && !source.flags.overflow &&
                !primary.flags.overflow &&
                !source.flags.auxiliary_carry_defined &&
                !primary.flags.auxiliary_carry_defined,
            "both zero tokens execute XOR EAX and the shared final RET 4"
        );
    }

    {
        LegacyBattleGroupAConfigurationState actor{
            .actor_record_token = 0x004AB790U,
            .source_record_token = 0x004AB790U,
        };
        const auto source = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 0U,
                .actor_token = 0x005029D0U,
                .entry_return_address = 0x11111111U,
            }
        );
        const auto primary = select_legacy_battle_actor_record(
            actor,
            {
                .argument = 1U,
                .actor_token = 0x005029D0U,
                .entry_return_address = 0x22222222U,
            }
        );
        test.expect_true(
            source.return_eax == primary.return_eax &&
                source.selected_source_record &&
                !source.selected_actor_record &&
                primary.selected_actor_record &&
                !primary.selected_source_record,
            "equal owner tokens do not change which physical field was selected"
        );
    }

    {
        using Status = LegacyBattleActorRecordSelectionStatus;
        struct FaultCase {
            Status status;
            u32 argument;
            u32 expected_eax;
            u32 expected_eip;
            u32 expected_argument_reads;
            u32 expected_actor_reads;
            u32 expected_tests;
            bool expected_xor;
        };
        constexpr std::array cases{
            FaultCase{
                Status::argument_read_typed_stop,
                1U,
                0xAABBCCDDU,
                0x00478670U,
                0U,
                0U,
                0U,
                false,
            },
            FaultCase{
                Status::source_record_read_typed_stop,
                0U,
                0U,
                0x00478678U,
                1U,
                0U,
                1U,
                false,
            },
            FaultCase{
                Status::actor_record_read_typed_stop,
                1U,
                1U,
                0x00478682U,
                1U,
                0U,
                1U,
                false,
            },
            FaultCase{
                Status::source_return_address_read_typed_stop,
                0U,
                0x80000001U,
                0x0047867FU,
                1U,
                1U,
                2U,
                false,
            },
            FaultCase{
                Status::final_return_address_read_typed_stop,
                1U,
                0x00000002U,
                0x0047868AU,
                1U,
                1U,
                2U,
                false,
            },
            FaultCase{
                Status::final_return_address_read_typed_stop,
                0U,
                0U,
                0x0047868AU,
                1U,
                1U,
                2U,
                true,
            },
        };
        for (const auto& fault : cases) {
            LegacyBattleGroupAConfigurationState actor{
                .actor_record_token = 2U,
                .source_record_token = fault.expected_xor ? 0U : 0x80000001U,
            };
            LegacyBattleActorRecordSelectionRequest request{
                .argument = fault.argument,
                .actor_token = 0x005029D0U,
                .entry_eax = 0xAABBCCDDU,
                .entry_edx = 0x55667788U,
                .entry_esp = 0x70005000U,
                .entry_return_address = 0x11223344U,
                .entry_flags = LegacyBattleActorCoordinateFlags{
                    .carry = true,
                    .parity = false,
                    .auxiliary_carry = true,
                    .auxiliary_carry_defined = true,
                    .zero = false,
                    .sign = true,
                    .overflow = true,
                },
            };
            switch (fault.status) {
            case Status::argument_read_typed_stop:
                request.access.argument_readable = false;
                break;

            case Status::source_record_read_typed_stop:
                request.access.source_record_readable = false;
                request.access.actor_record_readable = false;
                break;

            case Status::actor_record_read_typed_stop:
                request.access.source_record_readable = false;
                request.access.actor_record_readable = false;
                break;

            case Status::source_return_address_read_typed_stop:
                request.access.source_return_address_readable = false;
                request.access.actor_record_readable = false;
                break;

            case Status::final_return_address_read_typed_stop:
                request.access.final_return_address_readable = false;
                if (fault.argument == 0U) {
                    request.access.actor_record_readable = false;
                } else {
                    request.access.source_record_readable = false;
                }
                break;

            case Status::completed:
                break;
            }
            const auto result =
                select_legacy_battle_actor_record(actor, request);
            const bool entry_flags_preserved =
                fault.status == Status::argument_read_typed_stop;
            test.expect_true(
                result.status == fault.status &&
                    result.return_eax == fault.expected_eax &&
                    result.return_ecx == 0x005029D0U &&
                    result.return_edx == 0x55667788U &&
                    result.return_esp == 0x70005000U &&
                    result.return_eip == fault.expected_eip &&
                    result.argument_reads == fault.expected_argument_reads &&
                    result.actor_field_reads == fault.expected_actor_reads &&
                    result.return_address_reads == 0U &&
                    result.stack_read_count == fault.expected_argument_reads &&
                    result.tests_executed == fault.expected_tests &&
                    result.xor_zero_executed == fault.expected_xor &&
                    !result.returned &&
                    result.flags.carry == entry_flags_preserved &&
                    result.flags.overflow == entry_flags_preserved &&
                    result.flags.auxiliary_carry_defined ==
                        entry_flags_preserved,
                "every field and RET fault preserves the exact reached prefix"
            );
        }
    }
}
