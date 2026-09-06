#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

[[nodiscard]] constexpr bool has_even_parity(u16 value) noexcept {
    value = static_cast<u16>(value & 0x00FFU);
    value ^= static_cast<u16>(value >> 4U);
    value ^= static_cast<u16>(value >> 2U);
    value ^= static_cast<u16>(value >> 1U);
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

[[nodiscard]] bool
readable(const u16* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

}  // namespace

LegacyBattleActorCoordinatesView resolve_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinateOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            10U,
            index
        )) {
        if (owners.startup != nullptr) {
            return view_legacy_battle_actor_coordinates(
                owners.startup->party[index]
            );
        }
        if (owners.action != nullptr) {
            return view_legacy_battle_actor_coordinates(
                owners.action->group_a_action_execution[index]
            );
        }
        return {};
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
        return view_legacy_battle_actor_coordinates(
            (*owners.startup->group_b_lifecycle)[index].action_execution
        );
    }
    return {};
}

LegacyBattleActorCoordinateQueryResult query_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    u16* const output_x,
    u16* const output_y,
    const LegacyBattleActorCoordinateQueryRequest& request
) noexcept {
    LegacyBattleActorCoordinateQueryResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .flags = request.entry_flags,
    };
    if (!readable(
            actor.coordinate_mode_gate,
            actor.coordinate_mode_gate_read_accessible
        )) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::actor_gate_read_typed_stop;
        return result;
    }

    result.selector = *actor.coordinate_mode_gate;
    ++result.gate_reads;
    result.flags = {
        .carry = false,
        .parity = has_even_parity(result.selector),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = true,
        .zero = result.selector == 0U,
        .sign = (result.selector & 0x8000U) != 0U,
        .overflow = false,
    };
    result.alternate_coordinates = !result.flags.zero;

    if (result.alternate_coordinates) {
        if (!request.first_output_pointer_readable) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                first_output_pointer_read_typed_stop;
            return result;
        }
        result.return_edx = request.output_x_token;
        if (!readable(
                actor.alternate_position_x,
                actor.alternate_position_x_read_accessible
            )) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                alternate_x_read_typed_stop;
            return result;
        }
        result.output_x = *actor.alternate_position_x;
        replace_low_word(result.return_eax, result.output_x);
        ++result.coordinate_reads;
        if (output_x == nullptr || !request.first_output_writable) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                first_output_write_typed_stop;
            return result;
        }
        *output_x = result.output_x;
        ++result.output_writes;

        if (!readable(
                actor.alternate_position_y,
                actor.alternate_position_y_read_accessible
            )) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                alternate_y_read_typed_stop;
            return result;
        }
        result.output_y = *actor.alternate_position_y;
        replace_low_word(result.return_eax, result.output_y);
        ++result.coordinate_reads;
        if (!request.second_output_pointer_readable) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                second_output_pointer_read_typed_stop;
            return result;
        }
        result.return_ecx = request.output_y_token;
        if (output_y == nullptr || !request.second_output_writable) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                second_output_write_typed_stop;
            return result;
        }
        *output_y = result.output_y;
        ++result.output_writes;
        return result;
    }

    if (!request.first_output_pointer_readable) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            first_output_pointer_read_typed_stop;
        return result;
    }
    result.return_eax = request.output_x_token;
    if (!readable(actor.position_x, actor.position_x_read_accessible)) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::primary_x_read_typed_stop;
        return result;
    }
    result.output_x = *actor.position_x;
    replace_low_word(result.return_edx, result.output_x);
    ++result.coordinate_reads;
    if (output_x == nullptr || !request.first_output_writable) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            first_output_write_typed_stop;
        return result;
    }
    *output_x = result.output_x;
    ++result.output_writes;

    if (!request.second_output_pointer_readable) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            second_output_pointer_read_typed_stop;
        return result;
    }
    result.return_edx = request.output_y_token;
    if (!readable(actor.position_y, actor.position_y_read_accessible)) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::primary_y_read_typed_stop;
        return result;
    }
    result.output_y = *actor.position_y;
    replace_low_word(result.return_ecx, result.output_y);
    ++result.coordinate_reads;
    if (output_y == nullptr || !request.second_output_writable) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            second_output_write_typed_stop;
        return result;
    }
    *output_y = result.output_y;
    ++result.output_writes;
    return result;
}

}  // namespace openswd3::battle
