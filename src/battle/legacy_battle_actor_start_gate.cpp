#include "openswd3/battle/legacy_battle_actor_start_gate.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kStartGateReadInstruction = 0x004786D0U;
inline constexpr u32 kReturnInstruction = 0x004786D7U;
inline constexpr u32 kStartGateOffset = 0x00002A74U;

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

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const compat::u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

}  // namespace

LegacyBattleActorStartGateView resolve_legacy_battle_actor_start_gate(
    const LegacyBattleActorStartGateOwners& owners, const u32 actor_token
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
            .start_gate =
                &owners.action->group_a_action_execution[index].start_gate,
        };
    }

    if (owners.startup != nullptr &&
        resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup->group_b_lifecycle != nullptr) {
        return {
            .start_gate = &(*owners.startup->group_b_lifecycle)[index]
                               .action_execution.start_gate,
        };
    }
    return {};
}

LegacyBattleActorStartGateResult query_legacy_battle_actor_start_gate(
    const LegacyBattleActorStartGateView actor,
    const LegacyBattleActorStartGateRequest& request
) noexcept {
    LegacyBattleActorStartGateResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kStartGateReadInstruction,
        .field_token = request.actor_token + kStartGateOffset,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.start_gate == nullptr || !request.access.start_gate_readable) {
        result.status =
            LegacyBattleActorStartGateStatus::start_gate_read_typed_stop;
        return result;
    }
    result.return_eax = replace_low_word(result.return_eax, *actor.start_gate);
    ++result.start_gate_reads;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorStartGateStatus::return_address_read_typed_stop;
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
