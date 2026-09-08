#include "openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp"

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

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_word_flags(const u16 left, const u16 right) noexcept {
    const u32 sum = static_cast<u32>(left) + static_cast<u32>(right);
    const u16 result = static_cast<u16>(sum);
    return {
        .carry = sum > 0xFFFFU,
        .parity = has_even_parity(result),
        .auxiliary_carry =
            ((left ^ right ^ result) & static_cast<u16>(0x10U)) != 0U,
        .auxiliary_carry_defined = true,
        .zero = result == 0U,
        .sign = (result & 0x8000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ result)) &
                     static_cast<u16>(0x8000U)) != 0U,
    };
}

[[nodiscard]] bool accessible(
    const u16* const value,
    const bool* const read_accessible,
    const bool* const write_accessible
) noexcept {
    return value != nullptr &&
        (read_accessible == nullptr || *read_accessible) &&
        (write_accessible == nullptr || *write_accessible);
}

}  // namespace

LegacyBattleActorCoordinateAdjustmentResult
adjust_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    const LegacyBattleActorCoordinateAdjustmentRequest& request
) noexcept {
    LegacyBattleActorCoordinateAdjustmentResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .flags = request.entry_flags,
    };

    if (!request.x_argument_readable) {
        result.status = LegacyBattleActorCoordinateAdjustmentStatus::
            x_argument_read_typed_stop;
        return result;
    }
    const u16 x_delta = static_cast<u16>(request.x_delta);
    replace_low_word(result.return_eax, x_delta);
    ++result.argument_reads;

    if (!request.y_argument_readable) {
        result.status = LegacyBattleActorCoordinateAdjustmentStatus::
            y_argument_read_typed_stop;
        return result;
    }
    const u16 y_delta = static_cast<u16>(request.y_delta);
    replace_low_word(result.return_edx, y_delta);
    ++result.argument_reads;

    if (!accessible(
            actor.position_x,
            actor.position_x_read_accessible,
            actor.position_x_write_accessible
        )) {
        result.status = LegacyBattleActorCoordinateAdjustmentStatus::
            position_x_add_typed_stop;
        return result;
    }
    const u16 position_x = *actor.position_x;
    *actor.position_x = static_cast<u16>(position_x + x_delta);
    result.flags = add_word_flags(position_x, x_delta);
    ++result.coordinate_adds;

    if (!accessible(
            actor.position_y,
            actor.position_y_read_accessible,
            actor.position_y_write_accessible
        )) {
        result.status = LegacyBattleActorCoordinateAdjustmentStatus::
            position_y_add_typed_stop;
        return result;
    }
    const u16 position_y = *actor.position_y;
    *actor.position_y = static_cast<u16>(position_y + y_delta);
    result.flags = add_word_flags(position_y, y_delta);
    ++result.coordinate_adds;
    return result;
}

}  // namespace openswd3::battle
