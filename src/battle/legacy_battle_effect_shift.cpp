#include "openswd3/battle/legacy_battle_effect_shift.hpp"

#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallReadActorValue = 0x00478600U;
constexpr u32 kCallWriteActorValue = 0x004785C0U;

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
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

[[nodiscard]] LegacyBattleEffectCallReply invoke(
    LegacyBattleEffectCallPort& port,
    LegacyBattleEffectShiftResult& result,
    const u32 callee,
    const u32 actor,
    const u32 argument_value,
    const u32 scratch_value,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) {
    LegacyBattleEffectCallRequest request{};
    request.callee_token = callee;
    request.arguments[0] = actor;
    request.arguments[1] = argument_value;
    request.arguments[2] = scratch_value;
    request.eax = eax;
    request.ecx = ecx;
    request.edx = edx;
    ++result.port_calls;
    return port.invoke(request);
}

[[nodiscard]] bool apply_group(
    LegacyBattleEffectCallPort& port,
    LegacyBattleEffectShiftResult& result,
    const bool group_a,
    u32& argument_value,
    u32& scratch_value,
    u32& final_edx
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
        const auto read = invoke(
            port,
            result,
            kCallReadActorValue,
            actor,
            argument_value,
            scratch_value,
            argument_value,
            actor,
            final_edx
        );
        if ((read.output_write_mask & 1U) != 0U) {
            argument_value = read.outputs[0];
        }
        if ((read.output_write_mask & 2U) != 0U) {
            scratch_value = read.outputs[1];
        }

        argument_value += to_bits(shift.actor_delta);
        const auto write = invoke(
            port,
            result,
            kCallWriteActorValue,
            actor,
            argument_value,
            scratch_value,
            argument_value,
            actor,
            scratch_value
        );
        final_edx = write.edx;
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
    u32& final_edx
) {
    return apply_group(
               port, result, true, argument_value, scratch_value, final_edx
           ) &&
        apply_group(
               port, result, false, argument_value, scratch_value, final_edx
        );
}

}  // namespace

LegacyBattleEffectShiftResult advance_legacy_battle_effect_shift(
    LegacyBattleEffectCallPort& port,
    u32 argument_value,
    const u32 completion_mode,
    const u32 entry_ecx,
    const u32 entry_edx
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
                port, result, argument_value, scratch_value, final_edx
            )) {
            result.argument_value = argument_value;
            result.scratch_value = scratch_value;
            result.final_ecx = entry_ecx;
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
                    port, result, argument_value, scratch_value, final_edx
                )) {
                result.argument_value = argument_value;
                result.scratch_value = scratch_value;
                result.final_ecx = entry_ecx;
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
