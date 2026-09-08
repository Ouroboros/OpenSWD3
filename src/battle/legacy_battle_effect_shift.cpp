#include "openswd3/battle/legacy_battle_effect_shift.hpp"

#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right, const u32 value) noexcept {
    return {
        .carry = value < left,
        .parity = has_even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right, const u32 value) noexcept {
    return {
        .carry = left < right,
        .parity = has_even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = (((left ^ right) & (left ^ value)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr i32
arithmetic_shift_right_one(const i32 value) noexcept {
    const std::int64_t wide = value;
    return static_cast<i32>(wide >= 0 ? wide / 2 : -(((-wide) + 1) / 2));
}

[[nodiscard]] constexpr u32
actor_token(const u32 base, const u32 stride, const i32 index) noexcept {
    return base + static_cast<u32>(index) * stride;
}

[[nodiscard]] bool apply_group(
    LegacyBattleEffectCallPort& port,
    LegacyBattleEffectShiftResult& result,
    const bool group_a,
    u32& argument_value,
    u32& scratch_value,
    u32& final_edx,
    const LegacyBattleActorCoordinateOwners& coordinate_owners,
    const LegacyBattleEffectShiftCurrentCoordinateAccess&
        current_coordinate_access
) {
    auto& shift = port.effect_shift_state();
    auto& metrics = port.actor_metric_state();
    const u32 base = group_a ? kLegacyBattleEffectShiftGroupABaseToken
                             : kLegacyBattleEffectShiftGroupBBaseToken;
    const u32 stride = group_a ? kLegacyBattleEffectShiftGroupAStride
                               : kLegacyBattleEffectShiftGroupBStride;
    const i32 capacity = group_a ? 10 : 8;
    auto count = [&]() {
        return signed_dword(
            group_a ? metrics.group_a_count : metrics.group_b_count
        );
    };

    if (count() <= 0) {
        return true;
    }

    i16 index = 0;
    for (;;) {
        const i32 signed_index = index;
        if (signed_index < 0 || signed_index >= capacity) {
            result.status = group_a
                ? LegacyBattleEffectShiftStatus::group_a_actor_typed_stop
                : LegacyBattleEffectShiftStatus::group_b_actor_typed_stop;
            return false;
        }
        const u32 actor = actor_token(base, stride, signed_index);
        const u32 index_bits = static_cast<u32>(signed_index);
        const u32 flag_left = group_a ? index_bits * 0x3F0U : index_bits * 24U;
        const u32 flag_value = flag_left - index_bits;
        const u32 query_entry_eax =
            group_a ? flag_value * 3U : index_bits + flag_value * 60U;
        u16 output_x = static_cast<u16>(argument_value);
        u16 output_y = static_cast<u16>(scratch_value);
        result.current_coordinate_query =
            query_legacy_battle_actor_current_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    coordinate_owners, actor
                ),
                &output_x,
                &output_y,
                {
                    .actor_token = actor,
                    .output_x_token = current_coordinate_access.output_x_token,
                    .output_y_token = current_coordinate_access.output_y_token,
                    .entry_eax = query_entry_eax,
                    .entry_edx = group_a
                        ? final_edx
                        : current_coordinate_access.output_y_token,
                    .entry_flags =
                        subtract_flags(flag_left, index_bits, flag_value),
                    .first_output_pointer_readable =
                        current_coordinate_access.first_output_pointer_readable,
                    .second_output_pointer_readable =
                        current_coordinate_access
                            .second_output_pointer_readable,
                    .first_output_writable =
                        current_coordinate_access.first_output_writable,
                    .second_output_writable =
                        current_coordinate_access.second_output_writable,
                }
            );
        ++result.current_coordinate_query_calls;
        const auto& current = result.current_coordinate_query;
        if (current.output_writes >= 1U) {
            argument_value =
                (argument_value & 0xFFFF0000U) | static_cast<u32>(output_x);
        }
        if (current.output_writes >= 2U) {
            scratch_value =
                (scratch_value & 0xFFFF0000U) | static_cast<u32>(output_y);
        }
        if (current.status !=
            LegacyBattleActorCurrentCoordinateQueryStatus::completed) {
            result.status = group_a ? LegacyBattleEffectShiftStatus::
                                          group_a_current_coordinate_typed_stop
                                    : LegacyBattleEffectShiftStatus::
                                          group_b_current_coordinate_typed_stop;
            result.return_value = current.return_eax;
            result.final_ecx = current.return_ecx;
            result.final_edx = current.return_edx;
            final_edx = current.return_edx;
            return false;
        }

        const u32 add_left = argument_value;
        const u32 add_right = to_bits(shift.actor_delta);
        argument_value += add_right;
        result.coordinate_publication = publish_legacy_battle_actor_coordinates(
            resolve_legacy_battle_actor_coordinates(coordinate_owners, actor),
            argument_value,
            scratch_value,
            {
                .actor_token = actor,
                .entry_eax = argument_value,
                .entry_ecx = actor,
                .entry_edx = group_a ? scratch_value : current.return_edx,
                .entry_esi = actor,
                .entry_edi = std::bit_cast<u32>(signed_index),
                .entry_flags = add_flags(add_left, add_right, argument_value),
            }
        );
        ++result.coordinate_publication_calls;
        final_edx = result.coordinate_publication.return_edx;
        if (result.coordinate_publication.status !=
            LegacyBattleActorCoordinatePublicationStatus::completed) {
            result.status = group_a
                ? LegacyBattleEffectShiftStatus::
                      group_a_coordinate_publication_typed_stop
                : LegacyBattleEffectShiftStatus::
                      group_b_coordinate_publication_typed_stop;
            result.return_value = result.coordinate_publication.return_eax;
            result.final_ecx = result.coordinate_publication.return_ecx;
            return false;
        }
        if (group_a) {
            ++result.group_a_iterations;
        } else {
            ++result.group_b_iterations;
        }

        index =
            std::bit_cast<i16>(static_cast<u16>(static_cast<u16>(index) + 1U));
        if (static_cast<i32>(index) >= count()) {
            return true;
        }
    }
}

[[nodiscard]] bool apply_all_groups(
    LegacyBattleEffectCallPort& port,
    LegacyBattleEffectShiftResult& result,
    u32& argument_value,
    u32& scratch_value,
    u32& final_edx,
    const LegacyBattleActorCoordinateOwners& coordinate_owners,
    const LegacyBattleEffectShiftCurrentCoordinateAccess&
        current_coordinate_access
) {
    return apply_group(
               port,
               result,
               true,
               argument_value,
               scratch_value,
               final_edx,
               coordinate_owners,
               current_coordinate_access
           ) &&
        apply_group(
               port,
               result,
               false,
               argument_value,
               scratch_value,
               final_edx,
               coordinate_owners,
               current_coordinate_access
        );
}

}  // namespace

LegacyBattleEffectShiftResult advance_legacy_battle_effect_shift(
    LegacyBattleEffectCallPort& port,
    u32 argument_value,
    const u32 completion_mode,
    const u32 entry_ecx,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateOwners& coordinate_owners,
    const LegacyBattleEffectShiftCurrentCoordinateAccess&
        current_coordinate_access
) {
    LegacyBattleEffectShiftResult result{};
    auto& shift = port.effect_shift_state();
    u32 scratch_value = entry_ecx;
    u32 final_edx = entry_edx;

    const i16 phase = signed_word(shift.phase_word);
    shift.invocation_counter = static_cast<u16>(shift.invocation_counter + 1U);
    const u32 direction_snapshot = shift.direction_mode;

    if (phase > 0) {
        const i32 half = arithmetic_shift_right_one(static_cast<i32>(phase));
        final_edx = 0U;
        shift.invocation_counter = 0U;
        shift.accumulated_step =
            static_cast<u16>(shift.accumulated_step + static_cast<u16>(half));
        shift.phase_word = static_cast<u16>(half);
        shift.actor_delta = direction_snapshot == 0U ? -half : half;
        result.phase_halved = true;
    }

    if (shift.actor_delta != 0) {
        if (!apply_all_groups(
                port,
                result,
                argument_value,
                scratch_value,
                final_edx,
                coordinate_owners,
                current_coordinate_access
            )) {
            result.argument_value = argument_value;
            result.scratch_value = scratch_value;
            if (result.status !=
                    LegacyBattleEffectShiftStatus::
                        group_a_current_coordinate_typed_stop &&
                result.status !=
                    LegacyBattleEffectShiftStatus::
                        group_b_current_coordinate_typed_stop &&
                result.status !=
                    LegacyBattleEffectShiftStatus::
                        group_a_coordinate_publication_typed_stop &&
                result.status !=
                    LegacyBattleEffectShiftStatus::
                        group_b_coordinate_publication_typed_stop) {
                result.final_ecx = entry_ecx;
            }
            result.final_edx = final_edx;
            return result;
        }
        result.argument_value = argument_value;
        result.scratch_value = scratch_value;
        result.final_ecx = entry_ecx;
        result.final_edx = final_edx;
        result.return_value = 0U;
        return result;
    }

    const i32 threshold = static_cast<i32>(signed_word(shift.threshold_word));
    final_edx = to_bits(threshold);
    const i32 low_argument = static_cast<i32>(argument_value & 0xFFFFU);
    if (low_argument > threshold) {
        const i16 accumulated = signed_word(shift.accumulated_step);
        final_edx = (final_edx & 0xFFFF0000U) | shift.accumulated_step;
        if (accumulated > 0) {
            const i16 step = accumulated >= 30 ? i16{30} : accumulated;
            shift.accumulated_step = static_cast<u16>(
                shift.accumulated_step - static_cast<u16>(step)
            );
            final_edx = (final_edx & 0xFFFF0000U) | shift.accumulated_step;
            shift.actor_delta = direction_snapshot == 0U
                ? static_cast<i32>(step)
                : -static_cast<i32>(step);
            if (direction_snapshot != 0U) {
                final_edx = to_bits(shift.actor_delta);
            }
            if (!apply_all_groups(
                    port,
                    result,
                    argument_value,
                    scratch_value,
                    final_edx,
                    coordinate_owners,
                    current_coordinate_access
                )) {
                result.argument_value = argument_value;
                result.scratch_value = scratch_value;
                if (result.status !=
                        LegacyBattleEffectShiftStatus::
                            group_a_current_coordinate_typed_stop &&
                    result.status !=
                        LegacyBattleEffectShiftStatus::
                            group_b_current_coordinate_typed_stop &&
                    result.status !=
                        LegacyBattleEffectShiftStatus::
                            group_a_coordinate_publication_typed_stop &&
                    result.status !=
                        LegacyBattleEffectShiftStatus::
                            group_b_coordinate_publication_typed_stop) {
                    result.final_ecx = entry_ecx;
                }
                result.final_edx = final_edx;
                return result;
            }
        }
        shift.completion_latch = 1U;
        result.completion_latch_published = true;
    }

    if (completion_mode == 1U) {
        shift.phase_word = 0x01A4U;
    }
    result.argument_value = argument_value;
    result.scratch_value = scratch_value;
    result.final_ecx = entry_ecx;
    result.final_edx = final_edx;
    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
