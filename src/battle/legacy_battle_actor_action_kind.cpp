#include "openswd3/battle/legacy_battle_actor_action_kind.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kActionKindReadInstruction = 0x004786B0U;
inline constexpr u32 kReturnInstruction = 0x004786B7U;
inline constexpr u32 kActionKindOffset = 0x00002A6CU;

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

LegacyBattleActorActionKindView resolve_legacy_battle_actor_action_kind(
    const LegacyBattleActorActionKindOwners& owners, const u32 actor_token
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
            .action_kind =
                &owners.action->group_a_action_execution[index].action_kind,
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
            .action_kind = &(*owners.startup->group_b_lifecycle)[index]
                                .action_composition.action_kind,
        };
    }
    return {};
}

LegacyBattleActorActionKindResult query_legacy_battle_actor_action_kind(
    const LegacyBattleActorActionKindView actor,
    const LegacyBattleActorActionKindRequest& request
) noexcept {
    LegacyBattleActorActionKindResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kActionKindReadInstruction,
        .field_token = request.actor_token + kActionKindOffset,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.action_kind == nullptr || !request.access.action_kind_readable) {
        result.status =
            LegacyBattleActorActionKindStatus::action_kind_read_typed_stop;
        return result;
    }
    result.return_eax = replace_low_word(result.return_eax, *actor.action_kind);
    ++result.action_kind_reads;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorActionKindStatus::return_address_read_typed_stop;
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
