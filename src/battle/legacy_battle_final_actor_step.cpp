#include "openswd3/battle/legacy_battle_final_actor_step.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kCallValidateActor = 0x00479850U;
constexpr u32 kCallRemoveActor = 0x004750C0U;
constexpr u32 kCallPublishActorCode = 0x0045EFB0U;
constexpr u32 kCallResetActor = 0x0047C660U;
constexpr u32 kCallQueryContinuation = 0x0047F340U;
constexpr u32 kCallConfigureActor = 0x00478330U;
constexpr u32 kCallQueryCoordinates = 0x00475870U;
constexpr u32 kCallQueryDescriptor = 0x00480AD0U;
constexpr u32 kCallQueryAction = 0x0047F910U;
constexpr u32 kCallPublishAction = 0x00477710U;
constexpr u32 kCallQueryGroupBReset = 0x0047CE80U;
constexpr u32 kPublishActionOwnerToken = 0x004B9F00U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

[[nodiscard]] constexpr u8 low_byte(const u32 value) noexcept {
    return static_cast<u8>(value);
}

[[nodiscard]] constexpr u8 third_byte(const u32 value) noexcept {
    return static_cast<u8>(value >> 16U);
}

void replace_low_byte(u32& value, const u8 replacement) noexcept {
    value = (value & 0xFFFFFF00U) | replacement;
}

void replace_second_byte(u32& value, const u8 replacement) noexcept {
    value = (value & 0xFFFF00FFU) | (static_cast<u32>(replacement) << 8U);
}

void replace_high_word(u32& value, const u16 replacement) noexcept {
    value = (value & 0x0000FFFFU) | (static_cast<u32>(replacement) << 16U);
}

[[nodiscard]] LegacyBattleActionCallReply invoke(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 callee,
    const std::array<u32, 8>& arguments = {}
) {
    ++result.port_calls;
    return port.invoke({.callee_token = callee, .arguments = arguments});
}

[[nodiscard]] bool rebuild_actor_metrics(
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    const auto metrics = rebuild_legacy_battle_actor_metrics(
        port, to_bits(action.group_b_count), to_bits(action.group_a_count)
    );
    result.port_calls += metrics.port_calls;
    action.group_b_count =
        signed_dword(port.actor_metric_state().group_b_count);
    action.group_a_count =
        signed_dword(port.actor_metric_state().group_a_count);
    if (metrics.status != LegacyBattleActorMetricStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_metric_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] bool rebuild_actor_order(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    auto& metric_state = port.actor_metric_state();
    const auto order = rebuild_legacy_battle_actor_order(
        metric_state,
        metric_state.group_b_count,
        metric_state.group_a_count,
        metric_state.entry_edx
    );
    if (order.status != LegacyBattleActorOrderStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_order_typed_stop;
        return false;
    }
    const auto group_b_order =
        rebuild_legacy_battle_group_b_order(metric_state);
    if (group_b_order.status != LegacyBattleGroupBOrderStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::group_b_order_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] bool zero_workspace_record(
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchResult& result,
    const u32 dword_index
) noexcept {
    for (u32 offset = 0U; offset < 8U; ++offset) {
        const u32 index = dword_index + offset;
        if (index >= action.opponent_workspace.size()) {
            result.status = LegacyBattleActionDispatchStatus::
                final_actor_workspace_typed_stop;
            return false;
        }
        action.opponent_workspace[index] = 0U;
    }
    return true;
}

[[nodiscard]] LegacyBattleActionDispatchResult advance_group_a(
    LegacyBattleFinalActorStepState& state,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    const u32 actor_index
) {
    LegacyBattleActionDispatchResult result;
    const u32 actor_token = kLegacyBattleActionGroupABaseToken +
        actor_index * kLegacyBattleActionGroupAStride;
    if (invoke(port, result, kCallValidateActor, {actor_token}).eax != 1U) {
        return result;
    }
    if (actor_index >= state.group_a_completion_flags.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
        return result;
    }

    if (state.group_a_completion_flags[actor_index] == 1U) {
        const u8 completed = static_cast<u8>(
            static_cast<u8>(action.packed_actor_counter >> 8U) + 1U
        );
        replace_second_byte(action.packed_actor_counter, completed);
        const u16 threshold = high_word(action.phase_counter);
        if (static_cast<u16>(completed) >= threshold) {
            for (u32 record = 0U; record < threshold; ++record) {
                const u32 index =
                    to_bits(action.group_a_count) * 8U - record * 8U;
                if (!zero_workspace_record(action, result, index)) {
                    return result;
                }
            }
            replace_high_word(action.phase_counter, 0U);
            action.group_a_count = signed_dword(
                to_bits(action.group_a_count) - static_cast<u32>(threshold)
            );
        }
        if (!rebuild_actor_metrics(action, port, result) ||
            !rebuild_actor_order(port, result)) {
            return result;
        }
    } else {
        state.removed_group_a_count =
            static_cast<u8>(state.removed_group_a_count + 1U);
    }

    static_cast<void>(invoke(port, result, kCallRemoveActor, {actor_token}));
    const u32 actor_code = actor_index + 8U;
    static_cast<void>(
        invoke(port, result, kCallPublishActorCode, {actor_code})
    );
    state.group_a_slot_values[actor_index] = 0U;

    if (state.queued_actor_code == actor_code) {
        for (i32 index = 0; index < action.group_a_count; ++index) {
            static_cast<void>(invoke(
                port,
                result,
                kCallResetActor,
                {kLegacyBattleActionGroupABaseToken +
                     to_bits(index) * kLegacyBattleActionGroupAStride,
                 0U}
            ));
            ++result.group_a_iterations;
        }
        for (i32 index = 0; index < action.group_b_count; ++index) {
            static_cast<void>(invoke(
                port,
                result,
                kCallResetActor,
                {kLegacyBattleActionGroupBBaseToken +
                     to_bits(index) * kLegacyBattleActionGroupBStride,
                 0U}
            ));
            ++result.group_b_iterations;
        }
        state.queued_actor_code = 0U;
        state.action_execution_active = 0U;
        state.published_actor_code = 1U;
        action.message_state = 1U;
        state.frame_gate_a = 0U;
        state.frame_gate_b = 0U;
        state.selection_gate = 0U;
    }

    if (state.active_actor_code == actor_code) {
        state.active_actor_code = 0xFFFFFFFFU;
        state.selection_gate = 0U;
    }

    if (action.group_a_count > 0) {
        for (i32 index = 0; index < action.group_a_count; ++index) {
            const u32 uindex = to_bits(index);
            if (uindex >= state.actor_order.size()) {
                result.status = LegacyBattleActionDispatchStatus::
                    final_actor_workspace_typed_stop;
                return result;
            }
            if (state.actor_order[uindex] != actor_code) {
                continue;
            }
            for (u32 shift = uindex; shift < 9U; ++shift) {
                state.actor_order[shift] = state.actor_order[shift + 1U];
            }
            state.actor_order[9] = 0U;
            break;
        }
    }

    const u32 remaining = to_bits(action.group_a_count) -
        static_cast<u32>(state.excluded_group_a_count) -
        static_cast<u32>(high_word(action.phase_counter));
    if (static_cast<u32>(state.removed_group_a_count) >= remaining) {
        action.opponent_workspace.fill(0U);
        state.active_actor_code = 0xFFFFFFFFU;
        state.frame_gate_a = 1U;
        state.frame_gate_b = 1U;
        action.message_state = 0x67U;
        result.return_value = 1U;
        return result;
    }

    if (invoke(port, result, kCallQueryContinuation, {actor_token}).eax == 1U) {
        state.published_actor_code = state.active_actor_code + 1U;
        state.secondary_actor_code = actor_code;
        static_cast<void>(
            invoke(port, result, kCallConfigureActor, {actor_token, 1U})
        );
        state.action_execution_active = 1U;
        action.opponent_workspace[2U + actor_index] = 1U;
        state.auxiliary_gate = 1U;

        const u32 record_dword = state.secondary_actor_code * 5U - 40U;
        for (u32 offset = 0U; offset < 5U; ++offset) {
            const u32 index = record_dword + offset;
            if (index >= state.actor_runtime_records.size() * 5U) {
                result.status = LegacyBattleActionDispatchStatus::
                    final_actor_record_typed_stop;
                return result;
            }
            state.actor_runtime_records[index / 5U][index % 5U] = 0U;
        }
    }
    result.return_value = 1U;
    return result;
}

[[nodiscard]] LegacyBattleActionDispatchResult advance_group_b(
    LegacyBattleFinalActorStepState& state,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    const u32 actor_index
) {
    LegacyBattleActionDispatchResult result;
    if (actor_index == 0xFFFFFFFFU) {
        return result;
    }
    const u32 actor_token = kLegacyBattleActionGroupBBaseToken +
        actor_index * kLegacyBattleActionGroupBStride;
    if (invoke(port, result, kCallValidateActor, {actor_token}).eax != 1U) {
        return result;
    }

    const auto coordinates =
        invoke(port, result, kCallQueryCoordinates, {actor_token});
    state.coordinate_x =
        static_cast<u16>(state.coordinate_x + low_word(coordinates.outputs[0]));
    state.coordinate_y =
        static_cast<u16>(state.coordinate_y + low_word(coordinates.outputs[1]));

    const auto descriptor =
        invoke(port, result, kCallQueryDescriptor, {actor_token});
    state.actor_descriptor_token = descriptor.eax;
    if (descriptor.eax == 0U) {
        result.status =
            LegacyBattleActionDispatchStatus::final_actor_descriptor_typed_stop;
        return result;
    }
    state.action_delay = static_cast<u16>(
        state.action_delay +
        ((descriptor.object_flags & 0x20U) != 0U ? 0x14U : 3U)
    );

    const auto action_reply =
        invoke(port, result, kCallQueryAction, {actor_token, 1U});
    static_cast<void>(invoke(
        port,
        result,
        kCallPublishAction,
        {kPublishActionOwnerToken, action_reply.eax}
    ));
    static_cast<void>(
        invoke(port, result, kCallPublishActorCode, {actor_index})
    );

    const i32 signed_index = signed_dword(actor_index);
    if (signed_index >= 0 && signed_index <= action.group_b_count) {
        replace_low_byte(
            action.packed_actor_counter,
            static_cast<u8>(action.packed_actor_counter + 1U)
        );
    }
    const u32 processed =
        static_cast<u32>(low_byte(action.packed_actor_counter)) -
        static_cast<u32>(third_byte(action.packed_actor_counter));
    if (signed_dword(processed) >= action.group_b_count) {
        action.message_state = 0x63U;
        state.terminal_latch = 0U;
        state.frame_gate_a = 1U;
        state.frame_gate_b = 1U;
    }

    if (state.group_b_reset_word != 0U &&
        invoke(
            port,
            result,
            kCallQueryGroupBReset,
            {kLegacyBattleActionGroupBBaseToken}
        )
                .eax == 0U &&
        to_bits(action.group_b_count) -
                static_cast<u32>(low_byte(action.packed_actor_counter)) ==
            1U) {
        action.group_b_count = 1;
        replace_low_byte(action.packed_actor_counter, 0U);
        state.group_b_reset_word = 0U;
        if (!rebuild_actor_metrics(action, port, result) ||
            !rebuild_actor_order(port, result)) {
            return result;
        }
    }

    result.return_value = 1U;
    return result;
}

}  // namespace

LegacyBattleActionDispatchResult advance_legacy_battle_final_actor_step(
    LegacyBattleFinalActorStepState& state,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    const compat::u32 actor_index,
    const compat::u32 actor_group
) {
    return actor_group == 1U
        ? advance_group_a(state, action, port, actor_index)
        : advance_group_b(state, action, port, actor_index);
}

}  // namespace openswd3::battle
