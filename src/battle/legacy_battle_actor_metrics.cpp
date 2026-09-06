#include "openswd3/battle/legacy_battle_actor_metrics.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kGroupBBaseToken = kLegacyBattleActorCoordinatesGroupBBaseToken;
constexpr u32 kGroupBStride = kLegacyBattleActorCoordinatesGroupBStride;
constexpr u32 kGroupABaseToken = kLegacyBattleActorCoordinatesGroupABaseToken;
constexpr u32 kGroupAStride = kLegacyBattleActorCoordinatesGroupAStride;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
};

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
test_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
compare_flags(const u32 lhs, const u32 rhs) noexcept {
    const u32 difference = lhs - rhs;
    return {
        .carry = lhs < rhs,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((lhs ^ rhs ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x80000000U) != 0U,
        .overflow = (((lhs ^ rhs) & (lhs ^ difference)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr u32 sign_extended_word(const u16 value) noexcept {
    return std::bit_cast<u32>(static_cast<i32>(std::bit_cast<i16>(value)));
}

template <typename Call>
[[nodiscard]] LegacyBattleActorMetricResult
rebuild_impl(LegacyBattleActorMetricState& state, Call&& call) {
    LegacyBattleActorMetricResult result;
    std::ranges::fill(state.values, 0);
    std::ranges::fill(state.actor_order, 0U);
    state.local_word = static_cast<u16>(state.entry_ecx);
    state.local_byte = static_cast<u16>(state.entry_ecx >> 16U);

    Registers registers{
        .eax = state.group_b_count,
        .ecx = 0U,
        .edx = state.entry_edx,
        .flags = test_flags(state.group_b_count),
    };
    u32 index = 0U;
    if (state.group_b_count != 0U) {
        while (true) {
            registers.eax = state.local_word_token;
            registers.ecx = kGroupBBaseToken + index * kGroupBStride;
            const auto coordinate_query = call(
                registers.ecx,
                state.local_byte_token,
                state.local_word_token,
                registers
            );
            registers = {
                .eax = coordinate_query.return_eax,
                .ecx = coordinate_query.return_ecx,
                .edx = coordinate_query.return_edx,
                .flags = coordinate_query.flags,
            };
            result.coordinate_query = coordinate_query;
            ++result.coordinate_query_calls;
            if (coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status =
                    LegacyBattleActorMetricStatus::actor_coordinate_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                result.final_flags = registers.flags;
                state.entry_eax = result.return_value;
                state.entry_ecx = result.final_ecx;
                state.entry_edx = result.final_edx;
                state.entry_flags = result.final_flags;
                return result;
            }
            registers.edx = sign_extended_word(state.local_word);
            registers.eax = state.group_b_count;
            if (index >= state.values.size()) {
                result.status =
                    LegacyBattleActorMetricStatus::value_store_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                result.final_flags = registers.flags;
                state.entry_eax = result.return_value;
                state.entry_ecx = result.final_ecx;
                state.entry_edx = result.final_edx;
                state.entry_flags = result.final_flags;
                return result;
            }
            state.values[index] = std::bit_cast<i32>(registers.edx);
            ++index;
            ++result.group_b_iterations;
            registers.flags = compare_flags(index, state.group_b_count);
            if (index >= state.group_b_count) {
                break;
            }
        }
    }

    index = 8U;
    registers.eax = state.group_a_count + index;
    registers.flags = compare_flags(registers.eax, index);
    if (registers.eax > index) {
        while (true) {
            registers.edx = state.local_byte_token;
            registers.ecx = kGroupABaseToken + (index - 8U) * kGroupAStride;
            const auto coordinate_query = call(
                registers.ecx,
                state.local_byte_token,
                state.local_word_token,
                registers
            );
            registers = {
                .eax = coordinate_query.return_eax,
                .ecx = coordinate_query.return_ecx,
                .edx = coordinate_query.return_edx,
                .flags = coordinate_query.flags,
            };
            result.coordinate_query = coordinate_query;
            ++result.coordinate_query_calls;
            if (coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status =
                    LegacyBattleActorMetricStatus::actor_coordinate_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                result.final_flags = registers.flags;
                state.entry_eax = result.return_value;
                state.entry_ecx = result.final_ecx;
                state.entry_edx = result.final_edx;
                state.entry_flags = result.final_flags;
                return result;
            }
            registers.eax = sign_extended_word(state.local_word);
            registers.ecx = state.group_a_count;
            if (index >= state.values.size()) {
                result.status =
                    LegacyBattleActorMetricStatus::value_store_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                result.final_flags = registers.flags;
                state.entry_eax = result.return_value;
                state.entry_ecx = result.final_ecx;
                state.entry_edx = result.final_edx;
                state.entry_flags = result.final_flags;
                return result;
            }
            state.values[index] = std::bit_cast<i32>(registers.eax);
            ++index;
            ++result.group_a_iterations;
            registers.ecx = state.group_a_count + 8U;
            registers.flags = compare_flags(index, registers.ecx);
            if (index >= registers.ecx) {
                break;
            }
        }
    }

    result.return_value = registers.eax;
    result.final_ecx =
        (static_cast<u32>(state.local_byte) << 16U) | state.local_word;
    result.final_edx = registers.edx;
    result.final_flags = registers.flags;
    state.entry_eax = result.return_value;
    state.entry_ecx = result.final_ecx;
    state.entry_edx = result.final_edx;
    state.entry_flags = result.final_flags;
    return result;
}

[[nodiscard]] LegacyBattleActorCoordinateQueryResult query_coordinates(
    LegacyBattleActorMetricState& state,
    const LegacyBattleActorCoordinateOwners& owners,
    const u32 actor_token,
    const u32 byte_token,
    const u32 word_token,
    const Registers registers
) noexcept {
    return query_legacy_battle_actor_coordinates(
        resolve_legacy_battle_actor_coordinates(owners, actor_token),
        &state.local_byte,
        &state.local_word,
        {
            .actor_token = actor_token,
            .output_x_token = byte_token,
            .output_y_token = word_token,
            .entry_eax = registers.eax,
            .entry_edx = registers.edx,
            .entry_flags = registers.flags,
        }
    );
}

}  // namespace

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActionDispatchPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count,
    const LegacyBattleActorCoordinateOwners& owners
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&state, &owners](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_coordinates(
            state, owners, actor_token, byte_token, word_token, registers
        );
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleStartupPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count,
    const LegacyBattleActorCoordinateOwners& owners
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&state, &owners](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_coordinates(
            state, owners, actor_token, byte_token, word_token, registers
        );
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleFrameCoordinatorPort& port,
    const LegacyBattleActorCoordinateOwners& owners
) {
    auto& state = port.actor_metric_state();
    auto call = [&state, &owners](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_coordinates(
            state, owners, actor_token, byte_token, word_token, registers
        );
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorOrderResult rebuild_legacy_battle_actor_order(
    LegacyBattleActorMetricState& state,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count,
    const compat::u32 caller_edx
) {
    LegacyBattleActorOrderResult result;
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    u32 remaining = group_a_count + group_b_count;
    u32 final_edx = caller_edx;
    u32 output_index = 0U;

    while (remaining != 0U) {
        u32 candidate = 0U;
        i32 candidate_value = 0;
        while (true) {
            if (candidate >= state.values.size()) {
                result.status =
                    LegacyBattleActorOrderStatus::metric_read_typed_stop;
                result.return_value = candidate;
                result.final_edx = final_edx;
                return result;
            }
            candidate_value = state.values[candidate];
            ++result.metric_reads;
            if (candidate_value != 0) {
                if (candidate >= state.selected_mask.size()) {
                    result.status =
                        LegacyBattleActorOrderStatus::mask_access_typed_stop;
                    result.return_value = candidate;
                    result.final_edx = final_edx;
                    return result;
                }
                ++result.mask_reads;
                if (state.selected_mask[candidate] != 1U) {
                    break;
                }
            }
            ++candidate;
        }

        if (candidate >= state.values.size()) {
            result.status =
                LegacyBattleActorOrderStatus::metric_read_typed_stop;
            result.return_value = candidate;
            result.final_edx = final_edx;
            return result;
        }
        candidate_value = state.values[candidate];
        ++result.metric_reads;
        u32 selected = candidate;

        u32 index = candidate + 1U;
        final_edx = group_b_count;
        while (index < group_b_count) {
            if (index >= state.selected_mask.size()) {
                result.status =
                    LegacyBattleActorOrderStatus::mask_access_typed_stop;
                result.return_value = index;
                result.final_edx = final_edx;
                return result;
            }
            ++result.mask_reads;
            if (state.selected_mask[index] != 1U) {
                if (index >= state.values.size()) {
                    result.status =
                        LegacyBattleActorOrderStatus::metric_read_typed_stop;
                    result.return_value = index;
                    result.final_edx = final_edx;
                    return result;
                }
                const i32 value = state.values[index];
                ++result.metric_reads;
                if (value < candidate_value) {
                    candidate_value = value;
                    selected = index;
                }
            }
            ++index;
        }

        final_edx = group_a_count;
        const u32 group_a_bound = group_a_count + 8U;
        index = 8U;
        if (group_a_bound > index) {
            while (index < group_a_bound) {
                if (index >= state.selected_mask.size()) {
                    result.status =
                        LegacyBattleActorOrderStatus::mask_access_typed_stop;
                    result.return_value = index;
                    result.final_edx = final_edx;
                    return result;
                }
                ++result.mask_reads;
                if (state.selected_mask[index] != 1U) {
                    if (index >= state.values.size()) {
                        result.status = LegacyBattleActorOrderStatus::
                            metric_read_typed_stop;
                        result.return_value = index;
                        result.final_edx = final_edx;
                        return result;
                    }
                    const i32 value = state.values[index];
                    ++result.metric_reads;
                    if (value < candidate_value) {
                        candidate_value = value;
                        selected = index;
                    }
                }
                ++index;
            }
        }

        if (selected >= state.selected_mask.size()) {
            result.status =
                LegacyBattleActorOrderStatus::mask_access_typed_stop;
            result.return_value = selected;
            result.final_edx = final_edx;
            return result;
        }
        state.selected_mask[selected] = 1U;
        ++result.mask_writes;
        if (output_index >= state.actor_order.size()) {
            result.status =
                LegacyBattleActorOrderStatus::order_store_typed_stop;
            result.return_value = output_index;
            result.final_edx = final_edx;
            return result;
        }
        state.actor_order[output_index] = selected;
        ++output_index;
        ++result.selections;
        --remaining;
    }

    std::ranges::fill(state.selected_mask, 0U);
    result.return_value = 0U;
    result.final_ecx = 0U;
    result.final_edx = final_edx;
    state.entry_eax = result.return_value;
    state.entry_ecx = result.final_ecx;
    state.entry_edx = result.final_edx;
    return result;
}

}  // namespace openswd3::battle
