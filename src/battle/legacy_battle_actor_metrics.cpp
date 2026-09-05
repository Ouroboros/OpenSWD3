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

constexpr u32 kGroupBBaseToken = kLegacyBattleActorCoordinatesGroupBBaseToken;
constexpr u32 kGroupBStride = kLegacyBattleActorCoordinatesGroupBStride;
constexpr u32 kGroupABaseToken = kLegacyBattleActorCoordinatesGroupABaseToken;
constexpr u32 kGroupAStride = kLegacyBattleActorCoordinatesGroupAStride;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

struct InvocationReply {
    Registers registers{};
    LegacyBattleActorCoordinateQueryResult coordinate_query{};
    bool coordinate_typed_stop{};
};

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
    };
    u32 index = 0U;
    if (state.group_b_count != 0U) {
        while (true) {
            registers.eax = state.local_word_token;
            registers.ecx = kGroupBBaseToken + index * kGroupBStride;
            const auto reply = call(
                registers.ecx,
                state.local_byte_token,
                state.local_word_token,
                registers
            );
            registers = reply.registers;
            result.coordinate_query = reply.coordinate_query;
            ++result.coordinate_query_calls;
            if (reply.coordinate_typed_stop) {
                result.status =
                    LegacyBattleActorMetricStatus::actor_coordinate_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
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
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
                return result;
            }

            state.values[index] = std::bit_cast<i32>(registers.edx);
            ++index;
            ++result.group_b_iterations;
            if (index >= state.group_b_count) {
                break;
            }
        }
    }

    index = 8U;
    registers.eax = state.group_a_count + index;
    if (registers.eax > index) {
        while (true) {
            registers.edx = state.local_byte_token;
            registers.ecx = kGroupABaseToken + (index - 8U) * kGroupAStride;
            const auto reply = call(
                registers.ecx,
                state.local_byte_token,
                state.local_word_token,
                registers
            );
            registers = reply.registers;
            result.coordinate_query = reply.coordinate_query;
            ++result.coordinate_query_calls;
            if (reply.coordinate_typed_stop) {
                result.status =
                    LegacyBattleActorMetricStatus::actor_coordinate_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
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
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
                return result;
            }

            state.values[index] = std::bit_cast<i32>(registers.eax);
            ++index;
            ++result.group_a_iterations;
            registers.ecx = state.group_a_count + 8U;
            if (index >= registers.ecx) {
                break;
            }
        }
    }

    result.return_value = registers.eax;
    // 0x0045B18A pops the dword overwritten by both WORD outputs.
    result.final_ecx =
        (static_cast<u32>(state.local_byte) << 16U) | state.local_word;
    result.final_edx = registers.edx;
    state.entry_eax = result.return_value;
    state.entry_edx = result.final_edx;
    return result;
}

[[nodiscard]] InvocationReply query_actor_metric(
    LegacyBattleActorMetricState& state,
    const LegacyBattleActorCoordinateBindings& bindings,
    const u32 actor_token,
    const u32 byte_token,
    const u32 word_token,
    const Registers registers
) noexcept {
    u16 first_output = state.local_byte;
    u16 second_output = state.local_word;
    const auto query = query_legacy_battle_actor_coordinates(
        resolve_legacy_battle_actor_coordinates(bindings, actor_token),
        &first_output,
        &second_output,
        {
            .actor_token = actor_token,
            .output_x_token = byte_token,
            .output_y_token = word_token,
            .entry_eax = registers.eax,
            .entry_edx = registers.edx,
        }
    );
    if (query.output_writes >= 1U) {
        state.local_byte = first_output;
    }
    if (query.output_writes >= 2U) {
        state.local_word = second_output;
    }
    return {
        .registers =
            {
                .eax = query.return_eax,
                .ecx = query.return_ecx,
                .edx = query.return_edx,
            },
        .coordinate_query = query,
        .coordinate_typed_stop =
            query.status != LegacyBattleActorCoordinateQueryStatus::completed,
    };
}

}  // namespace

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActionDispatchPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&state, &bindings = port.actor_coordinate_bindings()](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_actor_metric(
            state, bindings, actor_token, byte_token, word_token, registers
        );
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleStartupPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&state, &bindings = port.actor_coordinate_bindings()](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_actor_metric(
            state, bindings, actor_token, byte_token, word_token, registers
        );
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult
rebuild_legacy_battle_actor_metrics(LegacyBattleFrameCoordinatorPort& port) {
    auto& state = port.actor_metric_state();
    auto call = [&state, &bindings = port.actor_coordinate_bindings()](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        return query_actor_metric(
            state, bindings, actor_token, byte_token, word_token, registers
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
