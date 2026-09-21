#include "openswd3/battle/legacy_battle_actor_gate_decay.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

inline constexpr u32 kStartGateOffset = 0x00002A74U;
inline constexpr u32 kTargetSelectionCountOffset = 0x00002A76U;
inline constexpr u32 kStartGateLatchOffset = 0x00002AE0U;
inline constexpr u32 kStartGateWriteInstruction = 0x00478AEFU;
inline constexpr u32 kTargetSelectionCountReadInstruction = 0x00478AF6U;
inline constexpr u32 kTargetSelectionCountWriteInstruction = 0x00478B03U;
inline constexpr u32 kStartGateLatchWriteInstruction = 0x00478B0AU;
inline constexpr u32 kReturnInstruction = 0x00478B10U;

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr bool resolve_index(
    const u32 token,
    const u32 base,
    const u32 stride,
    const std::size_t count,
    std::size_t& index
) noexcept {
    if (token < base) {
        return false;
    }
    const u32 delta = token - base;
    if (delta % stride != 0U) {
        return false;
    }
    index = delta / stride;
    return index < count;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
compare_word_zero_flags(const u16 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags decrement_flags(
    const u32 previous,
    const u32 decremented,
    const LegacyBattleActorCoordinateFlags& entry
) noexcept {
    return {
        .carry = entry.carry,
        .parity = has_even_parity(decremented),
        .auxiliary_carry = ((previous ^ decremented) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = decremented == 0U,
        .sign = (decremented & 0x80000000U) != 0U,
        .overflow = previous == 0x80000000U,
    };
}

}  // namespace

LegacyBattleActorGateDecayView resolve_legacy_battle_actor_gate_decay(
    const LegacyBattleActorGateDecayOwners& owners, const u32 actor_token
) noexcept {
    if (actor_token == kLegacyBattleActorGateDecayFixedPreGroupBToken &&
        owners.fixed_pre_group_b_packed_counts != nullptr &&
        owners.fixed_pre_group_b_start_gate_latch != nullptr) {
        return {
            .packed_start_gate_and_target_selection_count =
                owners.fixed_pre_group_b_packed_counts,
            .start_gate_latch = owners.fixed_pre_group_b_start_gate_latch,
        };
    }

    std::size_t index{};
    if (owners.action != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            owners.action->group_a_action_execution.size(),
            index
        )) {
        auto& actor = owners.action->group_a_action_execution[index];
        return {
            .start_gate = &actor.start_gate,
            .target_selection_count = &actor.target_selection_count,
            .start_gate_latch = &actor.start_gate_latch,
        };
    }

    if (owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            owners.startup->group_b_lifecycle->size(),
            index
        )) {
        auto& actor =
            (*owners.startup->group_b_lifecycle)[index].action_execution;
        return {
            .start_gate = &actor.start_gate,
            .target_selection_count = &actor.target_selection_count,
            .start_gate_latch = &actor.start_gate_latch,
        };
    }

    return {};
}

LegacyBattleActorGateDecayResult decay_legacy_battle_actor_gates(
    const LegacyBattleActorGateDecayView actor,
    const LegacyBattleActorGateDecayRequest& request
) noexcept {
    LegacyBattleActorGateDecayResult result{
        .call_address = request.call_address,
        .return_address = request.return_address,
        .actor_token = request.actor_token,
        .start_gate_field_token = request.actor_token + kStartGateOffset,
        .target_selection_count_field_token =
            request.actor_token + kTargetSelectionCountOffset,
        .start_gate_latch_field_token =
            request.actor_token + kStartGateLatchOffset,
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if ((actor.start_gate == nullptr &&
         actor.packed_start_gate_and_target_selection_count == nullptr) ||
        !request.access.start_gate_readable) {
        result.status =
            LegacyBattleActorGateDecayStatus::start_gate_read_typed_stop;
        return result;
    }

    result.previous_start_gate = actor.start_gate != nullptr
        ? *actor.start_gate
        : static_cast<u16>(*actor.packed_start_gate_and_target_selection_count);
    ++result.start_gate_reads;
    result.return_eax =
        (result.return_eax & 0xFFFF0000U) | result.previous_start_gate;
    result.return_edx = 0U;
    result.flags = compare_word_zero_flags(result.previous_start_gate);
    result.flags_known = true;
    result.decayed_start_gate = result.previous_start_gate;
    if (result.previous_start_gate != 0U) {
        const u32 previous_eax = result.return_eax;
        --result.return_eax;
        result.decayed_start_gate = static_cast<u16>(result.return_eax);
        result.flags =
            decrement_flags(previous_eax, result.return_eax, result.flags);
        result.return_eip = kStartGateWriteInstruction;
        if (!request.access.start_gate_writable) {
            result.status =
                LegacyBattleActorGateDecayStatus::start_gate_write_typed_stop;
            return result;
        }

        if (actor.start_gate != nullptr) {
            *actor.start_gate = result.decayed_start_gate;
        } else {
            *actor.packed_start_gate_and_target_selection_count =
                (*actor.packed_start_gate_and_target_selection_count &
                 0xFFFF0000U) |
                result.decayed_start_gate;
        }
        ++result.start_gate_writes;
    }

    result.return_eip = kTargetSelectionCountReadInstruction;
    if ((actor.target_selection_count == nullptr &&
         actor.packed_start_gate_and_target_selection_count == nullptr) ||
        !request.access.target_selection_count_readable) {
        result.status = LegacyBattleActorGateDecayStatus::
            target_selection_count_read_typed_stop;
        return result;
    }

    result.previous_target_selection_count =
        actor.target_selection_count != nullptr
        ? *actor.target_selection_count
        : static_cast<u16>(
              *actor.packed_start_gate_and_target_selection_count >> 16U
          );
    ++result.target_selection_count_reads;
    result.return_eax = (result.return_eax & 0xFFFF0000U) |
        result.previous_target_selection_count;
    result.flags =
        compare_word_zero_flags(result.previous_target_selection_count);
    result.decayed_target_selection_count =
        result.previous_target_selection_count;
    if (result.previous_target_selection_count != 0U) {
        const u32 previous_eax = result.return_eax;
        --result.return_eax;
        result.decayed_target_selection_count =
            static_cast<u16>(result.return_eax);
        result.flags =
            decrement_flags(previous_eax, result.return_eax, result.flags);
        result.return_eip = kTargetSelectionCountWriteInstruction;
        if (!request.access.target_selection_count_writable) {
            result.status = LegacyBattleActorGateDecayStatus::
                target_selection_count_write_typed_stop;
            return result;
        }

        if (actor.target_selection_count != nullptr) {
            *actor.target_selection_count =
                result.decayed_target_selection_count;
        } else {
            *actor.packed_start_gate_and_target_selection_count =
                (*actor.packed_start_gate_and_target_selection_count &
                 0x0000FFFFU) |
                (static_cast<u32>(result.decayed_target_selection_count)
                 << 16U);
        }
        ++result.target_selection_count_writes;
    }

    result.return_eip = kStartGateLatchWriteInstruction;
    if (actor.start_gate_latch == nullptr ||
        !request.access.start_gate_latch_writable) {
        result.status =
            LegacyBattleActorGateDecayStatus::start_gate_latch_write_typed_stop;
        return result;
    }

    *actor.start_gate_latch = result.return_edx;
    ++result.start_gate_latch_writes;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorGateDecayStatus::return_address_read_typed_stop;
        return result;
    }

    result.stack_reads[result.stack_read_count++] = request.return_address;
    ++result.return_address_reads;
    result.return_esp += 4U;
    result.return_eip = request.return_address;
    result.returned = true;
    return result;
}

bool execute_legacy_battle_actor_gate_decay_call(
    LegacyBattleActorGateDecayTrace& trace,
    const LegacyBattleActorGateDecayCallRequests& requests,
    const LegacyBattleActorGateDecayOwners& owners,
    const u32 call_address,
    const u32 return_address,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const std::size_t request_offset
) noexcept {
    const std::size_t call_index = trace.calls;
    const std::size_t request_index = request_offset + call_index;
    auto request = request_index < requests.count
        ? requests.calls[request_index]
        : LegacyBattleActorGateDecayRequest{};
    request.call_address = call_address;
    request.return_address = return_address;
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;

    trace.call_addresses[call_index] = call_address;
    trace.return_addresses[call_index] = return_address;
    trace.actor_tokens[call_index] = actor_token;
    ++trace.calls;
    trace.last = decay_legacy_battle_actor_gates(
        resolve_legacy_battle_actor_gate_decay(owners, actor_token), request
    );
    return trace.last.status == LegacyBattleActorGateDecayStatus::completed;
}

}  // namespace openswd3::battle
