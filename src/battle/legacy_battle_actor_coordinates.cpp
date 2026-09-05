#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
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

LegacyBattleActorCoordinatesView resolve_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinateBindings& bindings, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            bindings.group_a.size(),
            index
        )) {
        return bindings.group_a[index];
    }
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            bindings.group_b.size(),
            index
        )) {
        return bindings.group_b[index];
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
    };
    if (actor.coordinate_mode_gate == nullptr ||
        (actor.coordinate_mode_gate_read_accessible != nullptr &&
         !*actor.coordinate_mode_gate_read_accessible)) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::actor_gate_read_typed_stop;
        return result;
    }

    ++result.gate_reads;
    result.zero_flag = *actor.coordinate_mode_gate == 0U;
    result.alternate_coordinates = !result.zero_flag;
    if (result.alternate_coordinates) {
        result.return_edx = request.output_x_token;
        if (actor.alternate_position_x == nullptr ||
            (actor.alternate_position_x_read_accessible != nullptr &&
             !*actor.alternate_position_x_read_accessible)) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                alternate_x_read_typed_stop;
            return result;
        }
        result.output_x = *actor.alternate_position_x;
        replace_low_word(result.return_eax, result.output_x);
        ++result.coordinate_reads;
        if (output_x == nullptr) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                first_output_write_typed_stop;
            return result;
        }
        *output_x = result.output_x;
        ++result.output_writes;

        if (actor.alternate_position_y == nullptr ||
            (actor.alternate_position_y_read_accessible != nullptr &&
             !*actor.alternate_position_y_read_accessible)) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                alternate_y_read_typed_stop;
            return result;
        }
        result.output_y = *actor.alternate_position_y;
        replace_low_word(result.return_eax, result.output_y);
        ++result.coordinate_reads;
        result.return_ecx = request.output_y_token;
        if (output_y == nullptr) {
            result.status = LegacyBattleActorCoordinateQueryStatus::
                second_output_write_typed_stop;
            return result;
        }
        *output_y = result.output_y;
        ++result.output_writes;
        return result;
    }

    result.return_eax = request.output_x_token;
    if (actor.position_x == nullptr ||
        (actor.position_x_read_accessible != nullptr &&
         !*actor.position_x_read_accessible)) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::primary_x_read_typed_stop;
        return result;
    }
    result.output_x = *actor.position_x;
    replace_low_word(result.return_edx, result.output_x);
    ++result.coordinate_reads;
    if (output_x == nullptr) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            first_output_write_typed_stop;
        return result;
    }
    *output_x = result.output_x;
    ++result.output_writes;

    result.return_edx = request.output_y_token;
    if (actor.position_y == nullptr ||
        (actor.position_y_read_accessible != nullptr &&
         !*actor.position_y_read_accessible)) {
        result.status =
            LegacyBattleActorCoordinateQueryStatus::primary_y_read_typed_stop;
        return result;
    }
    result.output_y = *actor.position_y;
    replace_low_word(result.return_ecx, result.output_y);
    ++result.coordinate_reads;
    if (output_y == nullptr) {
        result.status = LegacyBattleActorCoordinateQueryStatus::
            second_output_write_typed_stop;
        return result;
    }
    *output_y = result.output_y;
    ++result.output_writes;
    return result;
}

}  // namespace openswd3::battle
