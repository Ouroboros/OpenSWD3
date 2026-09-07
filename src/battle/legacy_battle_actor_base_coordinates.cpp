#include "openswd3/battle/legacy_battle_actor_base_coordinates.hpp"

namespace openswd3::battle {
namespace {

using compat::i32;
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
subtract_word_flags(const u16 left, const u16 right) noexcept {
    const u16 difference = static_cast<u16>(left - right);
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry =
            ((left ^ right ^ difference) & static_cast<u16>(0x10U)) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x8000U) != 0U,
        .overflow = ((left ^ right) & (left ^ difference) &
                     static_cast<u16>(0x8000U)) != 0U,
    };
}

template <typename Value>
[[nodiscard]] bool
readable(const Value* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

[[nodiscard]] constexpr u16 low_word(const i32 value) noexcept {
    return static_cast<u16>(static_cast<u32>(value));
}

}  // namespace

LegacyBattleActorBaseCoordinateQueryResult
query_legacy_battle_actor_base_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    u16* const output_x,
    u16* const output_y,
    const LegacyBattleActorBaseCoordinateQueryRequest& request
) noexcept {
    LegacyBattleActorBaseCoordinateQueryResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .flags = request.entry_flags,
    };

    if (!readable(actor.position_x, actor.position_x_read_accessible)) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            position_x_read_typed_stop;
        return result;
    }
    const u16 position_x = *actor.position_x;
    replace_low_word(result.return_eax, position_x);
    ++result.actor_reads;

    if (!request.output_x_pointer_readable) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            output_x_pointer_read_typed_stop;
        return result;
    }
    result.return_edx = request.output_x_token;
    ++result.output_pointer_reads;

    if (!readable(
            actor.source_y_offset, actor.source_y_offset_read_accessible
        )) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            x_adjustment_read_typed_stop;
        return result;
    }
    const u16 x_adjustment = *actor.source_y_offset;
    ++result.actor_reads;
    result.output_x = static_cast<u16>(position_x - x_adjustment);
    replace_low_word(result.return_eax, result.output_x);
    result.flags = subtract_word_flags(position_x, x_adjustment);

    if (output_x == nullptr || !request.output_x_writable) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            output_x_write_typed_stop;
        return result;
    }
    *output_x = result.output_x;
    ++result.output_writes;

    if (!readable(actor.position_y, actor.position_y_read_accessible)) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            position_y_read_typed_stop;
        return result;
    }
    const u16 position_y = *actor.position_y;
    replace_low_word(result.return_eax, position_y);
    ++result.actor_reads;

    if (!readable(
            actor.target_phase_y_adjustment,
            actor.target_phase_y_adjustment_read_accessible
        )) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            y_adjustment_read_typed_stop;
        return result;
    }
    const u16 y_adjustment = low_word(*actor.target_phase_y_adjustment);
    ++result.actor_reads;
    result.output_y = static_cast<u16>(position_y - y_adjustment);
    replace_low_word(result.return_eax, result.output_y);
    result.flags = subtract_word_flags(position_y, y_adjustment);

    if (!request.output_y_pointer_readable) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            output_y_pointer_read_typed_stop;
        return result;
    }
    result.return_ecx = request.output_y_token;
    ++result.output_pointer_reads;

    if (output_y == nullptr || !request.output_y_writable) {
        result.status = LegacyBattleActorBaseCoordinateQueryStatus::
            output_y_write_typed_stop;
        return result;
    }
    *output_y = result.output_y;
    ++result.output_writes;
    return result;
}

}  // namespace openswd3::battle
