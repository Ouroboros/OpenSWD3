#include "openswd3/battle/legacy_battle_actor_action_mode.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

inline constexpr u32 kArgumentReadInstruction = 0x00478710U;
inline constexpr u32 kDefaultActionKindWriteInstruction = 0x00478732U;
inline constexpr u32 kDefaultReturnInstruction = 0x0047873EU;
inline constexpr u32 kModeSixOrInstruction = 0x00478746U;
inline constexpr u32 kModeFortyOrInstruction = 0x0047874FU;
inline constexpr u32 kDisplayKindWriteInstruction = 0x00478756U;
inline constexpr u32 kActionKindClearInstruction = 0x0047875DU;
inline constexpr u32 kSpecialReturnInstruction = 0x0047876BU;
inline constexpr u32 kActionKindOffset = 0x00002A6CU;
inline constexpr u32 kDisplayKindOffset = 0x00002A70U;
inline constexpr u32 kModeFlagsOffset = 0x00002A87U;

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
subtract_flags(const u32 lhs, const u32 rhs) noexcept {
    const u32 value = lhs - rhs;
    return {
        .carry = lhs < rhs,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((lhs ^ rhs ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((lhs ^ rhs) & (lhs ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u8 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80U) != 0U,
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
    LegacyBattleActorActionModeResult& result,
    const LegacyBattleActorActionModeAccess access
) noexcept {
    result.actor_accesses[result.actor_access_count] = access;
    ++result.actor_access_count;
}

}  // namespace

LegacyBattleActorActionModeView resolve_legacy_battle_actor_action_mode(
    const LegacyBattleActorActionModeOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            kLegacyBattleActorGroupAElementCount,
            index
        ) &&
        owners.action != nullptr && owners.startup != nullptr) {
        return {
            .action_kind =
                &owners.action->group_a_action_execution[index].action_kind,
            .display_kind = &owners.startup->party[index]
                                 .item_effect_application.display_kind,
            .mode_flags = &owners.startup->party[index]
                               .item_effect_application.mode_flags,
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
        auto& action =
            (*owners.startup->group_b_lifecycle)[index].action_composition;
        return {
            .action_kind = &action.action_kind,
            .display_kind = &action.display_kind,
            .mode_flags = &action.mode_flags,
        };
    }

    return {};
}

LegacyBattleActorActionModeResult set_legacy_battle_actor_action_mode(
    const LegacyBattleActorActionModeView actor,
    const LegacyBattleActorActionModeRequest& request
) noexcept {
    LegacyBattleActorActionModeResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .return_eip = kArgumentReadInstruction,
        .argument_token = request.entry_esp + 4U,
        .mode_flags_token = request.actor_token + kModeFlagsOffset,
        .display_kind_token = request.actor_token + kDisplayKindOffset,
        .action_kind_token = request.actor_token + kActionKindOffset,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };

    if (!request.access.argument_readable) {
        result.status =
            LegacyBattleActorActionModeStatus::argument_read_typed_stop;
        return result;
    }
    result.stack_read_tokens[result.stack_read_count] = result.argument_token;
    result.stack_reads[result.stack_read_count] = request.mode;
    ++result.stack_read_count;
    ++result.argument_reads;
    result.return_eax = request.mode;

    constexpr std::array<u32, 6> special_modes{2U, 13U, 3U, 14U, 6U, 15U};
    bool special = false;
    for (const u32 candidate : special_modes) {
        result.flags = subtract_flags(result.return_eax, candidate);
        result.flags_known = true;
        if (result.return_eax == candidate) {
            special = true;
            break;
        }
    }

    if (!special) {
        result.return_eip = kDefaultActionKindWriteInstruction;
        if (actor.action_kind == nullptr ||
            !request.access.action_kind_writable) {
            result.status =
                LegacyBattleActorActionModeStatus::action_kind_write_typed_stop;
            return result;
        }
        *actor.action_kind = static_cast<u16>(request.mode);
        record_access(
            result, LegacyBattleActorActionModeAccess::action_kind_write
        );
        ++result.action_kind_writes;

        result.return_eax = 1U;
        result.return_eip = kDefaultReturnInstruction;
    } else {
        u8 mode_mask{};
        if (request.mode == 6U) {
            mode_mask = 0x08U;
            result.return_eip = kModeSixOrInstruction;
        } else if (request.mode == 14U || request.mode == 15U) {
            result.flags = subtract_flags(result.return_eax, 6U);
            result.flags_known = true;
            mode_mask = 0x40U;
            result.return_eip = kModeFortyOrInstruction;
        }

        if (mode_mask != 0U) {
            if (actor.mode_flags == nullptr ||
                !request.access.mode_flags_readable) {
                result.status = LegacyBattleActorActionModeStatus::
                    mode_flags_read_typed_stop;
                return result;
            }
            const u8 current_mode_flags = *actor.mode_flags;
            record_access(
                result, LegacyBattleActorActionModeAccess::mode_flags_read
            );
            ++result.mode_flags_reads;

            if (!request.access.mode_flags_writable) {
                result.status = LegacyBattleActorActionModeStatus::
                    mode_flags_write_typed_stop;
                return result;
            }
            const u8 updated_mode_flags =
                static_cast<u8>(current_mode_flags | mode_mask);
            *actor.mode_flags = updated_mode_flags;
            record_access(
                result, LegacyBattleActorActionModeAccess::mode_flags_write
            );
            ++result.mode_flags_writes;
            result.flags = logical_flags(updated_mode_flags);
            result.flags_known = true;
        }

        result.return_eip = kDisplayKindWriteInstruction;
        if (actor.display_kind == nullptr ||
            !request.access.display_kind_writable) {
            result.status = LegacyBattleActorActionModeStatus::
                display_kind_write_typed_stop;
            return result;
        }
        *actor.display_kind = static_cast<u16>(request.mode);
        record_access(
            result, LegacyBattleActorActionModeAccess::display_kind_write
        );
        ++result.display_kind_writes;

        result.return_eip = kActionKindClearInstruction;
        if (actor.action_kind == nullptr ||
            !request.access.action_kind_writable) {
            result.status =
                LegacyBattleActorActionModeStatus::action_kind_write_typed_stop;
            return result;
        }
        *actor.action_kind = 0U;
        record_access(
            result, LegacyBattleActorActionModeAccess::action_kind_write
        );
        ++result.action_kind_writes;

        result.return_eax = 1U;
        result.return_eip = kSpecialReturnInstruction;
    }

    if (!request.access.return_address_readable) {
        result.status =
            LegacyBattleActorActionModeStatus::return_address_read_typed_stop;
        return result;
    }
    result.stack_read_tokens[result.stack_read_count] = request.entry_esp;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    ++result.return_address_reads;
    result.return_esp += 8U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

}  // namespace openswd3::battle
