#include "openswd3/battle/legacy_battle_actor_display_kind.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr u32 kDisplayKindReadInstruction = 0x004786C0U;
inline constexpr u32 kReturnInstruction = 0x004786C7U;
inline constexpr u32 kDisplayKindOffset = 0x00002A70U;

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

LegacyBattleActorDisplayKindView resolve_legacy_battle_actor_display_kind(
    const LegacyBattleActorDisplayKindOwners& owners, const u32 actor_token
) noexcept {
    if (owners.startup == nullptr) {
        return {};
    }

    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            owners.startup->party.size(),
            index
        )) {
        return {
            .display_kind = &owners.startup->party[index]
                                 .item_effect_application.display_kind,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup->group_b_lifecycle != nullptr) {
        return {
            .display_kind = &(*owners.startup->group_b_lifecycle)[index]
                                 .action_composition.display_kind,
        };
    }
    return {};
}

LegacyBattleActorDisplayKindResult query_legacy_battle_actor_display_kind(
    const LegacyBattleActorDisplayKindView actor,
    const LegacyBattleActorDisplayKindRequest& request
) noexcept {
    LegacyBattleActorDisplayKindResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kDisplayKindReadInstruction,
        .field_token = request.actor_token + kDisplayKindOffset,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (actor.display_kind == nullptr ||
        !request.access.display_kind_readable) {
        result.status =
            LegacyBattleActorDisplayKindStatus::display_kind_read_typed_stop;
        return result;
    }
    result.return_eax =
        replace_low_word(result.return_eax, *actor.display_kind);
    ++result.display_kind_reads;

    result.return_eip = kReturnInstruction;
    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorDisplayKindStatus::return_address_read_typed_stop;
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
