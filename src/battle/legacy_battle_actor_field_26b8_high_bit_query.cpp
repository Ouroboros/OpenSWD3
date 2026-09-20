#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kFieldOffset = 0x000026B8U;
inline constexpr u32 kReturnInstruction = 0x004787C9U;
inline constexpr u32 kCarrySourceMask = 0x40000000U;

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
shift_right_31_flags(const u32 source, const u32 result) noexcept {
    return {
        .carry = (source & kCarrySourceMask) != 0U,
        .parity = result == 0U,
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = result == 0U,
        .sign = false,
        .overflow = false,
    };
}

}  // namespace

LegacyBattleActorField26b8HighBitQueryView
resolve_legacy_battle_actor_field_26b8_high_bit_query(
    const LegacyBattleActorField26b8HighBitQueryOwners& owners,
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

LegacyBattleActorField26b8HighBitQueryResult
query_legacy_battle_actor_field_26b8_high_bit(
    const LegacyBattleActorField26b8HighBitQueryView actor,
    const LegacyBattleActorField26b8HighBitQueryRequest& request
) noexcept {
    LegacyBattleActorField26b8HighBitQueryResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .field_token = request.actor_token + kFieldOffset,
        .return_address_token = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .overflow_defined = request.entry_overflow_defined,
        .flags = request.entry_flags,
    };

    if (actor.field_26b8 == nullptr || !request.access.field_readable) {
        result.status =
            LegacyBattleActorField26b8HighBitQueryStatus::field_read_typed_stop;
        return result;
    }
    result.field_read_value = *actor.field_26b8;
    result.return_eax = result.field_read_value;
    ++result.field_reads;
    result.actor_accesses[result.actor_access_count] =
        LegacyBattleActorField26b8HighBitQueryAccess::field_read;
    ++result.actor_access_count;

    result.return_eax >>= 31U;
    result.flags =
        shift_right_31_flags(result.field_read_value, result.return_eax);
    result.flags_known = true;
    result.overflow_defined = false;
    result.return_eip = kReturnInstruction;

    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorField26b8HighBitQueryStatus::
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
