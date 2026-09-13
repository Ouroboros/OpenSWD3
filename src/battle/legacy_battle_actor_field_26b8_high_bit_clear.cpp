#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

inline constexpr u32 kFieldOffset = 0x000026B8U;
inline constexpr u32 kHighBitClearMask = 0x7FFFFFFFU;
inline constexpr u32 kReturnInstruction = 0x0047877AU;

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

void record_access(
    LegacyBattleActorField26b8HighBitClearResult& result,
    const LegacyBattleActorField26b8HighBitClearAccess access
) noexcept {
    result.actor_accesses[result.actor_access_count] = access;
    ++result.actor_access_count;
}

}  // namespace

LegacyBattleActorField26b8HighBitClearView
resolve_legacy_battle_actor_field_26b8_high_bit_clear(
    const LegacyBattleActorField26b8HighBitClearOwners& owners,
    const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            kLegacyBattleActorGroupAElementCount,
            index
        ) &&
        owners.action != nullptr) {
        return {
            .field_26b8 =
                &owners.action->group_a_action_execution[index].field_26b8,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        return {
            .field_26b8 = &(*owners.startup->group_b_lifecycle)[index]
                               .action_execution.field_26b8,
        };
    }

    return {};
}

LegacyBattleActorField26b8HighBitClearResult
clear_legacy_battle_actor_field_26b8_high_bit(
    const LegacyBattleActorField26b8HighBitClearView actor,
    const LegacyBattleActorField26b8HighBitClearRequest& request
) noexcept {
    LegacyBattleActorField26b8HighBitClearResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .field_token = request.actor_token + kFieldOffset,
        .return_address_token = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.field_26b8 == nullptr || !request.access.field_readable) {
        result.status =
            LegacyBattleActorField26b8HighBitClearStatus::field_read_typed_stop;
        return result;
    }
    result.field_read_value = *actor.field_26b8;
    ++result.field_reads;
    record_access(
        result, LegacyBattleActorField26b8HighBitClearAccess::field_read
    );

    const u32 updated = result.field_read_value & kHighBitClearMask;
    if (!request.access.field_writable) {
        result.status = LegacyBattleActorField26b8HighBitClearStatus::
            field_write_typed_stop;
        return result;
    }
    *actor.field_26b8 = updated;
    result.field_write_value = updated;
    ++result.field_writes;
    record_access(
        result, LegacyBattleActorField26b8HighBitClearAccess::field_write
    );
    result.flags = logical_flags(updated);
    result.flags_known = true;
    result.return_eip = kReturnInstruction;

    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorField26b8HighBitClearStatus::
            return_address_read_typed_stop;
        return result;
    }
    result.stack_read_tokens[result.stack_read_count] = request.entry_esp;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    ++result.return_address_reads;
    result.return_esp += 4U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

}  // namespace openswd3::battle
