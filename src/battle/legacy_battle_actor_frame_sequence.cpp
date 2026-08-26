#include "openswd3/battle/legacy_battle_actor_frame_sequence.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

void synchronize_counts_to_frame(
    const LegacyBattleActorMetricState& metric_state,
    LegacyBattleGroupBFrameState& frame_state
) noexcept {
    frame_state.shared.action.group_b_count =
        signed_dword(metric_state.group_b_count);
    frame_state.shared.action.group_a_count =
        signed_dword(metric_state.group_a_count);
}

void synchronize_counts_from_frame(
    LegacyBattleActorMetricState& metric_state,
    const LegacyBattleGroupBFrameState& frame_state
) noexcept {
    metric_state.group_b_count =
        std::bit_cast<u32>(frame_state.shared.action.group_b_count);
    metric_state.group_a_count =
        std::bit_cast<u32>(frame_state.shared.action.group_a_count);
}

}  // namespace

LegacyBattleActorFrameSequenceResult advance_legacy_battle_actor_frame_sequence(
    LegacyBattleActorMetricState& metric_state,
    LegacyBattleActorFrameAdvanceContext* const frame_context,
    const compat::u32 caller_edx
) {
    LegacyBattleActorFrameSequenceResult result;
    const u32 initial_group_a = metric_state.group_a_count;
    const u32 initial_count = metric_state.group_b_count + initial_group_a;
    result.initial_count = initial_count;
    result.return_value = initial_count;
    metric_state.entry_eax = initial_count;
    metric_state.entry_ecx = initial_group_a;
    metric_state.entry_edx = caller_edx;
    if (signed_dword(initial_count) <= 0) {
        return result;
    }

    u32 eax = initial_count;
    for (u32 iteration = 0U; iteration < initial_count; ++iteration) {
        if (iteration >= metric_state.actor_order.size()) {
            result.status =
                LegacyBattleActorFrameSequenceStatus::actor_order_typed_stop;
            result.return_value = eax;
            return result;
        }
        eax = metric_state.actor_order[iteration];
        ++result.scanned_slots;

        const bool group_b_actor = signed_dword(eax) < 8;
        u32 actor_index = eax;
        bool invoke_frame = false;
        if (group_b_actor) {
            invoke_frame = signed_dword(actor_index) <
                signed_dword(metric_state.group_b_count);
        } else {
            actor_index -= 8U;
            eax = actor_index;
            invoke_frame = signed_dword(actor_index) <
                signed_dword(metric_state.group_a_count);
        }
        if (!invoke_frame) {
            continue;
        }
        if (frame_context == nullptr) {
            result.status =
                LegacyBattleActorFrameSequenceStatus::frame_context_typed_stop;
            result.return_value = eax;
            return result;
        }
        if (&frame_context->port.actor_metric_state() != &metric_state) {
            result.status =
                LegacyBattleActorFrameSequenceStatus::shared_state_typed_stop;
            result.return_value = eax;
            return result;
        }

        synchronize_counts_to_frame(metric_state, frame_context->state);
        LegacyBattleActionDispatchResult frame_result;
        if (group_b_actor) {
            frame_result = advance_legacy_battle_group_b_frame(
                frame_context->state,
                frame_context->port,
                frame_context->dispatch,
                actor_index
            );
            ++result.group_b_calls;
        } else {
            frame_result = advance_legacy_battle_group_a_frame(
                frame_context->state.shared,
                frame_context->port,
                frame_context->dispatch,
                actor_index
            );
            ++result.group_a_calls;
        }
        result.frame_results[result.frame_result_count] = frame_result;
        ++result.frame_result_count;
        synchronize_counts_from_frame(metric_state, frame_context->state);
        eax = frame_result.return_value;
        if (frame_result.status !=
            LegacyBattleActionDispatchStatus::completed) {
            result.status = group_b_actor
                ? LegacyBattleActorFrameSequenceStatus::group_b_typed_stop
                : LegacyBattleActorFrameSequenceStatus::group_a_typed_stop;
            result.return_value = eax;
            return result;
        }
    }

    result.return_value = eax;
    metric_state.entry_eax = result.return_value;
    return result;
}

}  // namespace openswd3::battle
