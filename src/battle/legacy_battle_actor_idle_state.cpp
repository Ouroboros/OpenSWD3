#include "openswd3/battle/legacy_battle_actor_idle_state.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kLatchReadInstruction = 0x004786A0U;
inline constexpr u32 kReturnInstruction = 0x004786A6U;
inline constexpr u32 kLatchOffset = 0x00002AB4U;

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

LegacyBattleActorIdleStateView resolve_legacy_battle_actor_idle_state(
    const LegacyBattleActorIdleStateOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            10U,
            index
        ) &&
        owners.action != nullptr) {
        return {
            .latch = &owners.action->group_a_action_execution[index]
                          .idle_state_latch,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            8U,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        return {
            .latch = &(*owners.startup->group_b_lifecycle)[index]
                          .action_execution.idle_state_latch,
        };
    }
    return {};
}

LegacyBattleActorIdleStateResult query_legacy_battle_actor_idle_state(
    const LegacyBattleActorIdleStateView actor,
    const LegacyBattleActorIdleStateRequest& request
) noexcept {
    LegacyBattleActorIdleStateResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kLatchReadInstruction,
        .field_token = request.actor_token + kLatchOffset,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.latch == nullptr || !request.access.latch_readable) {
        result.status = LegacyBattleActorIdleStateStatus::latch_read_typed_stop;
        return result;
    }
    result.return_eax = *actor.latch;
    ++result.latch_reads;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorIdleStateStatus::return_address_read_typed_stop;
        return result;
    }
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    ++result.return_address_reads;
    result.return_esp += 4U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

}  // namespace openswd3::battle
