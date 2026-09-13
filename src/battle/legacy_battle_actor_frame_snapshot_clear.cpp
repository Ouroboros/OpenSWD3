#include "openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <cstring>

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kPushEdiInstruction = 0x004786F2U;
inline constexpr u32 kRepStosdInstruction = 0x00478700U;
inline constexpr u32 kPopEdiInstruction = 0x00478702U;
inline constexpr u32 kReturnInstruction = 0x00478703U;
inline constexpr u32 kFrameSnapshotOffset = 0x000002A0U;

[[nodiscard]] constexpr bool even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
xor_flags(const u32 value) noexcept {
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

}  // namespace

LegacyBattleActorFrameSnapshotClearView
resolve_legacy_battle_actor_frame_snapshot_clear(
    const LegacyBattleActorFrameSnapshotClearOwners& owners,
    const u32 actor_token
) noexcept {
    std::size_t index{};
    if (owners.action != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            owners.action->group_a_action_execution.size(),
            index
        )) {
        return {
            .frame_source_action_record =
                &owners.action->group_a_action_execution[index]
                     .frame_source_action_record,
        };
    }
    return {};
}

LegacyBattleActorFrameSnapshotClearResult
clear_legacy_battle_actor_frame_snapshot(
    const LegacyBattleActorFrameSnapshotClearView actor,
    const LegacyBattleActorFrameSnapshotClearRequest& request
) noexcept {
    LegacyBattleActorFrameSnapshotClearResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.actor_token,
        .return_edi = request.entry_edi,
        .return_esp = request.entry_esp,
        .return_eip = kPushEdiInstruction,
        .region_start_token = request.actor_token + kFrameSnapshotOffset,
        .current_destination_token = request.actor_token + kFrameSnapshotOffset,
        .flags_known = request.entry_flags_known,
        .direction_flag = request.direction_flag,
        .flags = request.entry_flags,
    };

    const auto stop =
        [&](const LegacyBattleActorFrameSnapshotClearStatus status) {
            result.status = status;
            return result;
        };

    if (!request.stack_access.push_edi_writable) {
        return stop(
            LegacyBattleActorFrameSnapshotClearStatus::push_edi_write_typed_stop
        );
    }
    result.return_esp -= 4U;
    result.stack_writes[result.stack_write_count] = request.entry_edi;
    ++result.stack_write_count;

    result.return_ecx =
        static_cast<u32>(kLegacyBattleActorFrameSnapshotClearDwords);
    result.return_eax = 0U;
    result.flags = xor_flags(result.return_eax);
    result.flags_known = true;
    result.return_edi = result.region_start_token;
    result.return_eip = kRepStosdInstruction;

    auto* forward_bytes =
        reinterpret_cast<std::byte*>(actor.frame_source_action_record);
    constexpr u32 zero{};
    for (std::size_t index = 0U;
         index < kLegacyBattleActorFrameSnapshotClearDwords;
         ++index) {
        result.fault_dword_index = index;
        result.current_destination_token = result.return_edi;
        std::byte* destination{};
        if (request.direction_flag) {
            if (actor.reverse_destination_bytes != nullptr) {
                const std::size_t physical_index =
                    kLegacyBattleActorFrameSnapshotClearDwords - 1U - index;
                destination = actor.reverse_destination_bytes +
                    physical_index * sizeof(u32);
            }
        } else if (forward_bytes != nullptr) {
            destination = forward_bytes + index * sizeof(u32);
        }
        if (destination == nullptr ||
            !request.destination_dword_writable[index]) {
            return stop(
                LegacyBattleActorFrameSnapshotClearStatus::
                    destination_dword_write_typed_stop
            );
        }
        std::memcpy(destination, &zero, sizeof(zero));
        ++result.destination_writes;
        ++result.cleared_dwords;
        result.return_edi = request.direction_flag ? result.return_edi - 4U
                                                   : result.return_edi + 4U;
        --result.return_ecx;
    }
    result.fault_dword_index = kLegacyBattleActorFrameSnapshotClearDwords;
    result.current_destination_token = result.return_edi;

    result.return_eip = kPopEdiInstruction;
    if (!request.stack_access.pop_edi_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotClearStatus::pop_edi_read_typed_stop
        );
    }
    result.return_edi = request.entry_edi;
    result.return_esp += 4U;
    result.stack_reads[result.stack_read_count] = request.entry_edi;
    ++result.stack_read_count;

    result.return_eip = kReturnInstruction;
    if (!request.stack_access.return_address_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotClearStatus::
                return_address_read_typed_stop
        );
    }
    result.return_esp += 4U;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

}  // namespace openswd3::battle
