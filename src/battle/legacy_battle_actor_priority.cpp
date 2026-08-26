#include "openswd3/battle/legacy_battle_actor_priority.hpp"

#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u32;

constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupAStride = 0x2F34U;
constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x2B28U;
constexpr u32 kActorOrderEndToken = 0x005214F4U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 sign_extend_word(const u32 value) noexcept {
    return std::bit_cast<u32>(
        static_cast<i32>(static_cast<i16>(value & 0xFFFFU))
    );
}

[[nodiscard]] bool read_metric(
    const LegacyBattleActorMetricState& state,
    const u32 index,
    i32& value,
    LegacyBattleActorPriorityResult& result
) noexcept {
    if (index >= state.values.size()) {
        result.status = LegacyBattleActorPriorityStatus::metric_typed_stop;
        result.return_value = index;
        return false;
    }
    value = state.values[index];
    return true;
}

[[nodiscard]] bool read_mask(
    const LegacyBattleActorMetricState& state,
    const u32 index,
    u32& value,
    LegacyBattleActorPriorityResult& result
) noexcept {
    if (index >= state.selected_mask.size()) {
        result.status = LegacyBattleActorPriorityStatus::mask_typed_stop;
        result.return_value = index;
        return false;
    }
    value = state.selected_mask[index];
    return true;
}

[[nodiscard]] bool write_mask(
    LegacyBattleActorMetricState& state,
    const u32 index,
    const u32 value,
    LegacyBattleActorPriorityResult& result
) noexcept {
    if (index >= state.selected_mask.size()) {
        result.status = LegacyBattleActorPriorityStatus::mask_typed_stop;
        result.return_value = index;
        return false;
    }
    state.selected_mask[index] = value;
    return true;
}

[[nodiscard]] bool read_order(
    const LegacyBattleActorMetricState& state,
    const u32 index,
    u32& value,
    LegacyBattleActorPriorityResult& result
) noexcept {
    if (index >= state.actor_order.size()) {
        result.status = LegacyBattleActorPriorityStatus::order_typed_stop;
        result.return_value = index;
        return false;
    }
    value = state.actor_order[index];
    return true;
}

[[nodiscard]] bool write_order(
    LegacyBattleActorMetricState& state,
    const u32 index,
    const u32 value,
    LegacyBattleActorPriorityResult& result
) noexcept {
    if (index >= state.actor_order.size()) {
        result.status = LegacyBattleActorPriorityStatus::order_typed_stop;
        result.return_value = index;
        return false;
    }
    state.actor_order[index] = value;
    ++result.order_writes;
    return true;
}

void publish_registers(
    LegacyBattleActorMetricState& state,
    LegacyBattleActorPriorityResult& result,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.return_value = eax;
    result.final_ecx = ecx;
    result.final_edx = edx;
    state.entry_eax = eax;
    state.entry_ecx = ecx;
    state.entry_edx = edx;
}

}  // namespace

LegacyBattleActorPriorityResult update_legacy_battle_actor_priority(
    LegacyBattleFrameCoordinatorPort& port,
    const compat::u32 caller_eax,
    const compat::u32 caller_ecx,
    const compat::u32 caller_edx
) {
    LegacyBattleActorPriorityResult result;
    auto& state = port.actor_metric_state();

    u32 eax = (caller_eax & 0xFFFFFF00U) | state.priority_update_gate;
    if (state.priority_update_gate == 1U) {
        publish_registers(state, result, eax, caller_ecx, caller_edx);
        return result;
    }

    eax = state.group_a_mode;
    if (eax == 1U) {
        publish_registers(state, result, eax, caller_ecx, caller_edx);
        return result;
    }
    if (state.group_b_mode == 1U) {
        publish_registers(state, result, eax, caller_ecx, caller_edx);
        return result;
    }

    u32 current_actor = state.priority_actor_index;
    eax = current_actor;
    if (current_actor == 0xFFFFFFFFU) {
        publish_registers(state, result, eax, caller_ecx, caller_edx);
        return result;
    }

    u32 pair_actor = 0U;
    if (signed_dword(current_actor) < 8) {
        const u32 stale_eax = current_actor * 345U;
        const u32 actor_token =
            kGroupBBaseToken + current_actor * kGroupBStride;
        const auto reply = port.invoke({
            .call = LegacyBattleFrameCoordinatorCall::query_actor_pair,
            .arguments = {actor_token, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
            .eax = stale_eax,
            .ecx = actor_token,
            .edx = caller_edx,
        });
        ++result.pair_query_calls;
        pair_actor = sign_extend_word(reply.eax);
        eax = state.group_b_mode;
        if (eax == 0U) {
            pair_actor += 8U;
        }
    } else {
        const u32 relative = current_actor - 8U;
        const u32 stale_eax = relative * 3021U;
        const u32 actor_token = kGroupABaseToken + relative * kGroupAStride;
        const auto reply = port.invoke({
            .call = LegacyBattleFrameCoordinatorCall::query_actor_pair,
            .arguments = {actor_token, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
            .eax = stale_eax,
            .ecx = actor_token,
            .edx = caller_edx,
        });
        ++result.pair_query_calls;
        pair_actor = sign_extend_word(reply.eax);
        eax = state.group_a_mode;
        if (eax == 1U) {
            pair_actor += 8U;
        }
    }

    const u32 group_start = signed_dword(current_actor) < 8 ? 0U : 8U;
    const u32 group_end = signed_dword(current_actor) < 8 ? 8U : 18U;
    i32 current_metric = 0;
    if (!read_metric(state, current_actor, current_metric, result)) {
        result.final_edx = caller_edx;
        return result;
    }

    u32 selected_count = 0U;
    u32 insertion_end = 1U;
    u32 insertion_index = 0U;
    for (u32 candidate = group_start; candidate < group_end; ++candidate) {
        i32 candidate_metric = 0;
        if (!read_metric(state, candidate, candidate_metric, result)) {
            return result;
        }
        if (candidate_metric == 0 || candidate == current_actor ||
            candidate == pair_actor || candidate_metric >= current_metric) {
            continue;
        }

        insertion_index = selected_count;
        if (insertion_end > 1U) {
            u32 position = 0U;
            while (position < selected_count) {
                u32 existing_actor = 0U;
                if (!read_order(state, position, existing_actor, result)) {
                    return result;
                }
                i32 existing_metric = 0;
                if (!read_metric(
                        state, existing_actor, existing_metric, result
                    )) {
                    return result;
                }
                if (candidate_metric < existing_metric) {
                    insertion_index = position;
                    const u32 shift_count = selected_count - position + 1U;
                    u32 cursor = insertion_end;
                    for (u32 shift = 0U; shift < shift_count; ++shift) {
                        u32 moved_actor = 0U;
                        if (!read_order(
                                state, cursor - 1U, moved_actor, result
                            ) ||
                            !write_order(state, cursor, moved_actor, result)) {
                            return result;
                        }
                        --cursor;
                    }
                    break;
                }
                ++position;
            }
        }

        if (!write_order(state, insertion_index, candidate, result) ||
            !write_mask(state, candidate, 1U, result)) {
            return result;
        }
        ++selected_count;
        ++result.selections;
        ++result.priority_prefix_selections;
        ++insertion_end;
        insertion_index = selected_count;
    }

    u32 group_b_bound = state.group_b_count;
    const u32 total_count = state.group_a_count + group_b_bound;
    if (selected_count != total_count) {
        u32 output_index = selected_count;
        while (selected_count != total_count) {
            if (current_actor != 0xFFFFFFFFU &&
                !read_metric(state, current_actor, current_metric, result)) {
                return result;
            }

            u32 candidate = 0U;
            i32 candidate_metric = 0;
            while (true) {
                if (!read_metric(state, candidate, candidate_metric, result)) {
                    return result;
                }
                bool eligible = candidate_metric != 0;
                if (eligible && current_actor != 0xFFFFFFFFU &&
                    signed_dword(candidate) >= signed_dword(group_start) &&
                    signed_dword(candidate) < signed_dword(group_end) &&
                    candidate_metric >= current_metric) {
                    eligible = false;
                }
                if (eligible) {
                    u32 mask = 0U;
                    if (!read_mask(state, candidate, mask, result)) {
                        return result;
                    }
                    if (mask != 1U) {
                        break;
                    }
                }
                ++candidate;
            }

            if (!read_metric(state, candidate, candidate_metric, result)) {
                return result;
            }
            u32 selected = candidate;

            for (u32 index = 0U; index < group_b_bound; ++index) {
                u32 mask = 0U;
                if (!read_mask(state, index, mask, result)) {
                    return result;
                }
                if (mask == 1U) {
                    continue;
                }
                i32 value = 0;
                if (!read_metric(state, index, value, result)) {
                    return result;
                }
                if (value < candidate_metric && index != current_actor &&
                    (signed_dword(current_actor) >= 8 ||
                     current_actor == 0xFFFFFFFFU)) {
                    candidate_metric = value;
                    selected = index;
                }
            }

            const u32 group_a_bound = state.group_a_count + 8U;
            if (group_a_bound > 8U) {
                for (u32 index = 8U; index < group_a_bound; ++index) {
                    u32 mask = 0U;
                    if (!read_mask(state, index, mask, result)) {
                        return result;
                    }
                    if (mask == 1U) {
                        continue;
                    }
                    i32 value = 0;
                    if (!read_metric(state, index, value, result)) {
                        return result;
                    }
                    if (value < candidate_metric && index != current_actor &&
                        (signed_dword(current_actor) < 8 ||
                         current_actor == 0xFFFFFFFFU)) {
                        candidate_metric = value;
                        selected = index;
                    }
                }
            }
            group_b_bound = state.group_b_count;

            if (selected == pair_actor) {
                if (!write_order(state, output_index, pair_actor, result) ||
                    !write_mask(state, pair_actor, 1U, result)) {
                    return result;
                }
                ++output_index;
                if (!write_mask(state, current_actor, 1U, result) ||
                    !write_order(state, output_index, current_actor, result)) {
                    return result;
                }
                ++output_index;
                selected_count += 2U;
                result.selections += 2U;
                result.paired_selections += 2U;
                current_actor = 0xFFFFFFFFU;
                pair_actor = 0xFFFFFFFFU;
            } else {
                if (!write_order(state, output_index, selected, result) ||
                    !write_mask(state, selected, 1U, result)) {
                    return result;
                }
                ++output_index;
                ++selected_count;
                ++result.selections;
            }
        }
    }

    std::ranges::fill(state.selected_mask, 0U);
    state.priority_order_ready = 1U;
    result.order_ready_published = true;

    for (u32 index = 0U; index < state.actor_order.size(); ++index) {
        if (state.actor_order[index] != 18U) {
            continue;
        }
        ++result.nested_order_calls;
        const auto nested = rebuild_legacy_battle_actor_order(
            state, state.group_b_count, state.group_a_count, group_b_bound
        );
        if (nested.status != LegacyBattleActorOrderStatus::completed) {
            result.status =
                LegacyBattleActorPriorityStatus::nested_order_typed_stop;
            result.return_value = nested.return_value;
            result.final_ecx = nested.final_ecx;
            result.final_edx = nested.final_edx;
            return result;
        }
        publish_registers(
            state,
            result,
            nested.return_value,
            nested.final_ecx,
            nested.final_edx
        );
        return result;
    }

    publish_registers(state, result, kActorOrderEndToken, 0U, group_b_bound);
    return result;
}

}  // namespace openswd3::battle
