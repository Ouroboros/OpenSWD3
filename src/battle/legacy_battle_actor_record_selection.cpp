#include "openswd3/battle/legacy_battle_actor_record_selection.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kArgumentReadInstruction = 0x00478670U;
inline constexpr u32 kArgumentTestInstruction = 0x00478674U;
inline constexpr u32 kSourceRecordReadInstruction = 0x00478678U;
inline constexpr u32 kSourceRecordTestInstruction = 0x0047867BU;
inline constexpr u32 kSourceReturnInstruction = 0x0047867FU;
inline constexpr u32 kActorRecordReadInstruction = 0x00478682U;
inline constexpr u32 kActorRecordTestInstruction = 0x00478684U;
inline constexpr u32 kXorZeroInstruction = 0x00478688U;
inline constexpr u32 kFinalReturnInstruction = 0x0047868AU;

[[nodiscard]] constexpr bool even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logic_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

}  // namespace

LegacyBattleActorRecordSelectionResult select_legacy_battle_actor_record(
    const LegacyBattleGroupAConfigurationState& actor,
    const LegacyBattleActorRecordSelectionRequest& request
) noexcept {
    LegacyBattleActorRecordSelectionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kArgumentReadInstruction,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    const auto stop = [&](const LegacyBattleActorRecordSelectionStatus status) {
        result.status = status;
        return result;
    };
    const auto test = [&](const u32 instruction) {
        result.return_eip = instruction;
        result.flags = logic_flags(result.return_eax);
        result.flags_known = true;
        ++result.tests_executed;
    };
    const auto return_from =
        [&](const u32 instruction,
            const bool readable,
            const LegacyBattleActorRecordSelectionStatus status) {
            result.return_eip = instruction;
            if (!readable) {
                result.status = status;
                return false;
            }
            result.stack_reads[result.stack_read_count] =
                request.entry_return_address;
            ++result.stack_read_count;
            ++result.return_address_reads;
            result.return_esp += 8U;
            result.return_eip = request.entry_return_address;
            result.returned = true;
            return true;
        };

    if (!request.access.argument_readable) {
        return stop(
            LegacyBattleActorRecordSelectionStatus::argument_read_typed_stop
        );
    }
    result.return_eax = request.argument;
    result.stack_reads[result.stack_read_count] = request.argument;
    ++result.stack_read_count;
    ++result.argument_reads;
    test(kArgumentTestInstruction);

    if (result.return_eax == 0U) {
        result.return_eip = kSourceRecordReadInstruction;
        if (!request.access.source_record_readable) {
            return stop(
                LegacyBattleActorRecordSelectionStatus::
                    source_record_read_typed_stop
            );
        }
        result.return_eax = actor.source_record_token;
        result.selected_source_record = true;
        ++result.actor_field_reads;
        test(kSourceRecordTestInstruction);
        if (result.return_eax != 0U) {
            static_cast<void>(return_from(
                kSourceReturnInstruction,
                request.access.source_return_address_readable,
                LegacyBattleActorRecordSelectionStatus::
                    source_return_address_read_typed_stop
            ));
            return result;
        }
    } else {
        result.return_eip = kActorRecordReadInstruction;
        if (!request.access.actor_record_readable) {
            return stop(
                LegacyBattleActorRecordSelectionStatus::
                    actor_record_read_typed_stop
            );
        }
        result.return_eax = actor.actor_record_token;
        result.selected_actor_record = true;
        ++result.actor_field_reads;
        test(kActorRecordTestInstruction);
        if (result.return_eax != 0U) {
            static_cast<void>(return_from(
                kFinalReturnInstruction,
                request.access.final_return_address_readable,
                LegacyBattleActorRecordSelectionStatus::
                    final_return_address_read_typed_stop
            ));
            return result;
        }
    }

    result.return_eip = kXorZeroInstruction;
    result.return_eax = 0U;
    result.flags = logic_flags(0U);
    result.flags_known = true;
    result.xor_zero_executed = true;
    static_cast<void>(return_from(
        kFinalReturnInstruction,
        request.access.final_return_address_readable,
        LegacyBattleActorRecordSelectionStatus::
            final_return_address_read_typed_stop
    ));
    return result;
}

}  // namespace openswd3::battle
