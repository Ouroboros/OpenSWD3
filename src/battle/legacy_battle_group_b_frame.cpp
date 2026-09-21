#include "openswd3/battle/legacy_battle_group_b_frame.hpp"

#include "openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp"
#include "openswd3/battle/legacy_battle_group_b_opponent_mode.hpp"
#include "openswd3/battle/legacy_battle_group_b_status_action.hpp"
#include "openswd3/battle/legacy_battle_opponent_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <bit>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryTerminal = 0x0047CE80U;
constexpr u32 kCallUpdateOpponent = 0x0047DAD0U;
constexpr u32 kCallQueryQueueCompletion = 0x0047F920U;
constexpr u32 kCallQueryActorBlocked = 0x0047D930U;
constexpr u32 kCallQueryActorExcluded = 0x00478B50U;
constexpr u32 kCallClearControl = 0x0047C660U;
constexpr u32 kCallPrepareSelection = 0x00478B30U;
constexpr u32 kCallQuerySelectionMode = 0x00483820U;
constexpr u32 kCallRandomBounded = 0x00439070U;
constexpr u32 kCallPublishStatusMode = 0x0047D860U;
constexpr u32 kCallQuerySpecialAction = 0x0047D880U;
constexpr u32 kCallQueryPhaseMode = 0x0047D8D0U;
constexpr u32 kCallQueryStatusSequence = 0x00480220U;
constexpr u32 kCallPublishActionStart = 0x0047C690U;
constexpr u32 kCallSelectionClear = 0x00478B20U;
constexpr u32 kCallSelectionComplete = 0x00478B40U;
constexpr u32 kCallPublishBattleBit = 0x00483FF0U;
constexpr u32 kCallQueryCompletionEffect = 0x0047F360U;
constexpr u32 kCallPublishCompletionResource = 0x0047D640U;
constexpr u32 kCallSetCompletionMode = 0x0047CEC0U;
constexpr u32 kCallPrepareCompletionSurface = 0x0047F150U;
constexpr u32 kCallPublishEffectMode = 0x00478B60U;

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u32 value) noexcept {
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
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 difference = left - right;
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x80000000U) != 0U,
        .overflow =
            (((left ^ right) & (left ^ difference)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_word_flags(const u16 left, const u16 right) noexcept {
    const u16 difference = static_cast<u16>(left - right);
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x8000U) != 0U,
        .overflow = (((left ^ right) & (left ^ difference)) & 0x8000U) != 0U,
    };
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
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

[[nodiscard]] constexpr u8 high_byte(const u16 value) noexcept {
    return static_cast<u8>(value >> 8U);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

void replace_low_byte(u32& destination, const u8 value) noexcept {
    destination = (destination & 0xFFFFFF00U) | value;
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] bool validate_group_a(
    LegacyBattleActionDispatchResult& result, const u32 index
) noexcept {
    if (index < 10U) {
        return true;
    }
    result.status = LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
    return false;
}

[[nodiscard]] bool validate_group_b(
    LegacyBattleActionDispatchResult& result, const u32 index
) noexcept {
    if (index < 8U) {
        return true;
    }
    result.status = LegacyBattleActionDispatchStatus::group_b_index_typed_stop;
    return false;
}

[[nodiscard]] LegacyBattleActionCallReply invoke(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 callee,
    const std::initializer_list<u32> arguments = {}
) {
    LegacyBattleActionCallRequest request{};
    request.callee_token = callee;
    std::copy(arguments.begin(), arguments.end(), request.arguments.begin());
    ++result.port_calls;
    return port.invoke(request);
}

[[nodiscard]] bool reset_actor_runtime(
    LegacyBattleGroupBFrameState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 call_address,
    const u32 return_address,
    const u32 entry_eax = 0U,
    const u32 entry_edx = 0U
) {
    if (execute_legacy_battle_actor_runtime_reset_call(
            {.action = &state.shared.action, .startup = context.startup},
            context.bounded_random,
            result.actor_runtime_reset,
            context.actor_runtime_reset_requests,
            actor_token,
            entry_eax,
            entry_edx,
            call_address,
            return_address
        )) {
        return true;
    }

    result.status =
        LegacyBattleActionDispatchStatus::actor_runtime_reset_typed_stop;
    result.return_value = result.actor_runtime_reset.last.return_eax;
    return false;
}

[[nodiscard]] bool increment_actor_start_gate(
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 call_address,
    const u32 return_address,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags
) {
    if (execute_legacy_battle_actor_start_gate_increment_call(
            result.actor_start_gate_increment,
            context.actor_start_gate_increment_requests,
            {.action = &action, .startup = context.startup},
            call_address,
            return_address,
            actor_token,
            entry_eax,
            entry_edx,
            entry_flags,
            true
        )) {
        return true;
    }

    result.status =
        LegacyBattleActionDispatchStatus::actor_start_gate_increment_typed_stop;
    result.return_value = result.actor_start_gate_increment.last.return_eax;
    return false;
}

[[nodiscard]] bool decay_actor_gates(
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 call_address,
    const u32 return_address,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known = true
) {
    if (execute_legacy_battle_actor_gate_decay_call(
            result.actor_gate_decay,
            context.actor_gate_decay_requests,
            {.action = &action, .startup = context.startup},
            call_address,
            return_address,
            actor_token,
            entry_eax,
            entry_edx,
            entry_flags,
            entry_flags_known,
            context.actor_gate_decay_request_offset
        )) {
        return true;
    }

    result.status =
        LegacyBattleActionDispatchStatus::actor_gate_decay_typed_stop;
    result.return_value = result.actor_gate_decay.last.return_eax;
    return false;
}

[[nodiscard]] bool increment_selected_group_a_start_gate(
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_index,
    const u32 actor_token
) {
    const u32 times_sixty_three = (actor_index << 6U) - actor_index;
    const u32 times_one_thousand_eight = times_sixty_three << 4U;
    const u32 times_one_thousand_seven = times_one_thousand_eight - actor_index;
    const u32 times_three_thousand_twenty_one =
        times_one_thousand_seven + times_one_thousand_seven * 2U;
    return increment_actor_start_gate(
        action,
        context,
        result,
        actor_token,
        0x00457E1CU,
        0x00457E21U,
        times_one_thousand_seven,
        times_three_thousand_twenty_one,
        subtract_flags(times_one_thousand_eight, actor_index)
    );
}

[[nodiscard]] bool query_turn_completion(
    LegacyBattleActionDispatchResult& result,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActorTurnCompletionOwners& owners,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const u32 return_address,
    u32& value
) {
    auto request = context.actor_turn_completion_request;
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = true;
    result.actor_turn_completion = query_legacy_battle_actor_turn_completion(
        resolve_legacy_battle_actor_turn_completion(owners, actor_token),
        request
    );
    ++result.actor_turn_completion_calls;
    if (result.actor_turn_completion.status !=
        LegacyBattleActorTurnCompletionStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_turn_completion_typed_stop;
        result.return_value = result.actor_turn_completion.return_eax;
        return false;
    }
    value = result.actor_turn_completion.return_eax;
    return true;
}

[[nodiscard]] bool query_idle_state(
    LegacyBattleActionDispatchResult& result,
    const LegacyBattleActorIdleStateOwners& owners,
    const LegacyBattleActorIdleStateRequest& request,
    u32& value
) {
    result.actor_idle_state = query_legacy_battle_actor_idle_state(
        resolve_legacy_battle_actor_idle_state(owners, request.actor_token),
        request
    );
    ++result.actor_idle_state_calls;
    if (result.actor_idle_state.status !=
        LegacyBattleActorIdleStateStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_idle_state_typed_stop;
        result.return_value = result.actor_idle_state.return_eax;
        return false;
    }
    value = result.actor_idle_state.return_eax;
    return true;
}

[[nodiscard]] bool query_start_gate(
    LegacyBattleActionDispatchResult& result,
    const LegacyBattleActorStartGateOwners& owners,
    const LegacyBattleActorStartGateRequest& request,
    u32& value
) {
    result.actor_start_gate = query_legacy_battle_actor_start_gate(
        resolve_legacy_battle_actor_start_gate(owners, request.actor_token),
        request
    );
    ++result.actor_start_gate_calls;
    if (result.actor_start_gate.status !=
        LegacyBattleActorStartGateStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_start_gate_typed_stop;
        result.return_value = result.actor_start_gate.return_eax;
        return false;
    }
    value = result.actor_start_gate.return_eax;
    return true;
}

[[nodiscard]] bool query_action_target(
    LegacyBattleActionDispatchResult& result,
    const LegacyBattleActorActionTargetOwners& owners,
    const LegacyBattleActorActionTargetRequest& request,
    u32& value
) {
    result.actor_action_target = query_legacy_battle_actor_action_target(
        resolve_legacy_battle_actor_action_target(owners, request.actor_token),
        request
    );
    result.actor_action_targets[result.actor_action_target_calls] =
        result.actor_action_target;
    ++result.actor_action_target_calls;
    if (result.actor_action_target.status !=
        LegacyBattleActorActionTargetStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_action_target_typed_stop;
        result.return_value = result.actor_action_target.return_eax;
        return false;
    }
    value = result.actor_action_target.return_eax;
    return true;
}

[[nodiscard]] bool publish_text_message(
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const std::array<u32, 5>& arguments
) {
    if (context.startup_reset == nullptr || context.text_messages == nullptr) {
        result.status =
            LegacyBattleActionDispatchStatus::text_message_typed_stop;
        return false;
    }
    result.text_messages.push_back(enqueue_legacy_battle_text_message(
        *context.text_messages,
        context.startup_reset->block_5214f8[0U],
        port,
        {
            .value_04 = arguments[0U],
            .value_08 = arguments[1U],
            .kind = static_cast<u16>(arguments[2U]),
            .text_token = arguments[3U],
            .flags = arguments[4U],
        }
    ));
    ++result.text_message_calls;
    const auto& message = result.text_messages.back();
    result.port_calls += message.allocation_calls + message.measure_calls;
    if (message.status != LegacyBattleTextMessageStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::text_message_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] bool publish_player_item_quantity(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 item_id,
    const u32 quantity_selector
) {
    result.player_item = advance_legacy_battle_player_item_quantity(
        port, item_id, quantity_selector
    );
    ++result.player_item_calls;
    result.port_calls += result.player_item.port_calls;
    if (result.player_item.status !=
        LegacyBattlePlayerItemQuantityStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::player_item_typed_stop;
        return false;
    }
    return true;
}

class SingleEffectPortAdapter final : public LegacyBattleEffectCallPort {
public:
    explicit SingleEffectPortAdapter(LegacyBattleActionDispatchPort& port)
        : port_(port) {}

    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        LegacyBattleActionCallRequest action_request{};
        action_request.callee_token = request.callee_token;
        std::copy_n(
            request.arguments.begin(),
            action_request.arguments.size(),
            action_request.arguments.begin()
        );
        const auto reply = port_.invoke(action_request);
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .outputs = reply.outputs,
            .output_write_mask = reply.output_write_mask,
        };
    }

    [[nodiscard]] LegacyBattleActorMetricState&
    actor_metric_state() noexcept override {
        return port_.actor_metric_state();
    }

    [[nodiscard]] const LegacyBattleActorMetricState&
    actor_metric_state() const noexcept override {
        return port_.actor_metric_state();
    }

    [[nodiscard]] LegacyBattleEffectShiftState&
    effect_shift_state() noexcept override {
        return port_.effect_shift_state();
    }

    [[nodiscard]] const LegacyBattleEffectShiftState&
    effect_shift_state() const noexcept override {
        return port_.effect_shift_state();
    }

private:
    LegacyBattleActionDispatchPort& port_;
};

void merge_nested(
    LegacyBattleActionDispatchResult& result,
    const LegacyBattleActionDispatchResult& nested
) noexcept {
    result.port_calls += nested.port_calls;
    result.framebuffer_clear_calls += nested.framebuffer_clear_calls;
    result.group_a_iterations += nested.group_a_iterations;
    result.group_b_iterations += nested.group_b_iterations;
    result.terminal_resets += nested.terminal_resets;
    result.status_indicator_calls += nested.status_indicator_calls;
    result.scale_scan_calls += nested.scale_scan_calls;
    result.action_record_clear_calls += nested.action_record_clear_calls;
    for (u32 index = 0U; index < nested.actor_action_target_calls; ++index) {
        result.actor_action_targets[result.actor_action_target_calls + index] =
            nested.actor_action_targets[index];
    }
    result.actor_action_target_calls += nested.actor_action_target_calls;
    if (nested.actor_action_target_calls != 0U) {
        result.actor_action_target = nested.actor_action_target;
    }
    for (std::size_t index = 0U; index < nested.actor_gate_decay.calls;
         ++index) {
        const std::size_t destination = result.actor_gate_decay.calls + index;
        if (destination < result.actor_gate_decay.call_addresses.size()) {
            result.actor_gate_decay.call_addresses[destination] =
                nested.actor_gate_decay.call_addresses[index];
            result.actor_gate_decay.return_addresses[destination] =
                nested.actor_gate_decay.return_addresses[index];
            result.actor_gate_decay.actor_tokens[destination] =
                nested.actor_gate_decay.actor_tokens[index];
        }
    }
    result.actor_gate_decay.calls += nested.actor_gate_decay.calls;
    if (nested.actor_gate_decay.calls != 0U) {
        result.actor_gate_decay.last = nested.actor_gate_decay.last;
    }
    result.group_a_actor_cleanup_calls += nested.group_a_actor_cleanup_calls;
    if (nested.group_a_actor_cleanup_calls != 0U) {
        result.group_a_actor_cleanup = nested.group_a_actor_cleanup;
    }
    result.attack_order_calls += nested.attack_order_calls;
    if (nested.attack_order_calls != 0U) {
        result.attack_order = nested.attack_order;
    }
    result.attack_order_insert_calls += nested.attack_order_insert_calls;
    if (nested.attack_order_insert_calls != 0U) {
        result.attack_order_insert = nested.attack_order_insert;
    }
    result.attack_order_remove_calls += nested.attack_order_remove_calls;
    if (nested.attack_order_remove_calls != 0U) {
        result.attack_order_remove = nested.attack_order_remove;
    }
    result.status_indicator = nested.status_indicator;
    result.scale_scan = nested.scale_scan;
    result.action_code = nested.action_code;
    result.return_value = nested.return_value;
    result.status = nested.status;
}

[[nodiscard]] bool read_completion_value(
    LegacyBattleGroupBFrameState& state,
    LegacyBattleActionDispatchResult& result,
    const u32 group_a_count,
    u16& value
) noexcept {
    const u32 delta =
        group_a_count - high_word(state.shared.defeated_actor_packed);
    const u32 index = delta * 16U;
    if (index >= state.completion_value_table.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::target_table_typed_stop;
        return false;
    }
    value = state.completion_value_table[index];
    return true;
}

[[nodiscard]] bool fill_completion_surface(
    LegacyBattleGroupBFrameState& state,
    LegacyBattleActionDispatchResult& result
) noexcept {
    const u32 pixels = to_bits(state.completion_rect_right) *
        to_bits(state.completion_rect_bottom);
    const u32 byte_count = pixels * 2U;
    const u32 word_count = byte_count >> 1U;
    if (word_count == 0U) {
        return true;
    }
    if (state.completion_surface_token == 0U) {
        result.status =
            LegacyBattleActionDispatchStatus::framebuffer_typed_stop;
        return false;
    }
    const std::size_t owned = state.completion_surface.size();
    const std::size_t written = std::min<std::size_t>(word_count, owned);
    std::fill_n(state.completion_surface.begin(), written, 0xFFFFU);
    if (static_cast<u32>(owned) < word_count) {
        result.status =
            LegacyBattleActionDispatchStatus::framebuffer_typed_stop;
        return false;
    }
    return true;
}

}  // namespace

LegacyBattleGroupBFrameState::LegacyBattleGroupBFrameState() noexcept {
    pending_effect_ids.fill(0xFFFFFFFFU);
    final_actor_targets.fill(0xFFFFFFFFU);
}

LegacyBattleActionDispatchResult advance_legacy_battle_group_b_frame(
    LegacyBattleGroupBFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const u32 group_b_index
) {
    LegacyBattleActionDispatchResult result{};
    auto& shared = state.shared;
    auto& action = shared.action;

    if (!validate_group_b(result, group_b_index)) {
        return result;
    }
    const u32 source_token = group_b_token(group_b_index);
    const auto select_actor_target = [&](const u16 target_index,
                                         const u32 entry_eax,
                                         const u32 entry_edx,
                                         const LegacyBattleActorCoordinateFlags&
                                             entry_flags,
                                         const u32 call_address,
                                         const u32 return_address) {
        if (execute_legacy_battle_actor_target_selection_call(
                result.actor_target_selection,
                context.actor_target_selection_requests,
                {.action = &action, .startup = context.startup},
                call_address,
                return_address,
                source_token,
                target_index,
                entry_eax,
                entry_edx,
                entry_flags,
                true,
                context.actor_target_selection_request_offset
            )) {
            return true;
        }

        result.status =
            LegacyBattleActionDispatchStatus::actor_target_selection_typed_stop;
        result.return_value = result.actor_target_selection.last.return_eax;
        return false;
    };
    const LegacyBattleActorIdleStateOwners idle_state_owners{
        .action = &action,
        .startup = context.startup,
    };
    const LegacyBattleActorStartGateOwners start_gate_owners{
        .action = &action,
        .startup = context.startup,
    };
    u32 stale_ebx = group_b_index * kLegacyBattleActionGroupBStride;

    if (state.frame_enabled == 1U) {
        if (invoke(port, result, kCallQueryTerminal, {source_token}).eax ==
                0U &&
            action.action_pending_aux == 0U &&
            port.outcome_resolution_state().resolution_latch == 0U) {
            const auto update_reply =
                invoke(port, result, kCallUpdateOpponent, {source_token});
            if (state.post_update_gate[group_b_index] == 0U) {
                if (context.startup == nullptr) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_progress_typed_stop;
                    return result;
                }
                auto& enemy = context.startup->enemies[group_b_index];
                const auto* const lifecycle =
                    context.startup->group_b_lifecycle == nullptr
                    ? nullptr
                    : &(*context.startup->group_b_lifecycle)[group_b_index];
                if (lifecycle != nullptr) {
                    enemy.progress.field_26c0.alias(
                        lifecycle->action_execution.field_26c0
                    );
                }
                const auto progress =
                    advance_legacy_battle_actor_group_b_progress(
                        enemy.progress,
                        lifecycle,
                        std::bit_cast<i32>(state.update_gate_argument),
                        state.shared.actor_progress_threshold,
                        source_token,
                        update_reply.edx
                    );
                if (progress.status !=
                    LegacyBattleActorGroupBProgressStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_progress_typed_stop;
                    return result;
                }
                if (progress.return_eax == 1U &&
                    port.battle_message_state() != 0x67U) {
                    result
                        .attack_order = append_legacy_battle_attack_order_entry(
                        context.attack_order_records, 2U, group_b_index, 0U, 0U
                    );
                    ++result.attack_order_calls;
                    if (result.attack_order.status !=
                        LegacyBattleAttackOrderEntryStatus::completed) {
                        result.status = LegacyBattleActionDispatchStatus::
                            attack_order_typed_stop;
                        return result;
                    }
                }
            }
        }

        const bool turn_state_nonzero = shared.turn_resolution_bits != 0U;
        if (shared.action_aux_gate == 0U && !turn_state_nonzero &&
            action.active_effect_target == group_b_index) {
            if (invoke(port, result, kCallQueryQueueCompletion, {source_token})
                    .eax == 1U) {
                shared.selection_mode = 0U;
                action.active_effect_gate = 0U;
                shared.action_block_gate = 0U;
                action.action_pending_aux = 0U;
                action.active_effect_target = 0xFFFFFFFFU;
                shared.final_actor_step.queued_actor_code = group_b_index + 1U;
                if (!reset_actor_runtime(
                        state,
                        context,
                        result,
                        source_token,
                        0x00457791U,
                        0x00457796U
                    )) {
                    return result;
                }
                result.return_value =
                    result.actor_runtime_reset.last.return_eax;
                return result;
            }
            const auto phase_terminal =
                invoke(port, result, kCallQueryTerminal, {source_token});
            if (phase_terminal.eax == 1U) {
                shared.selection_mode = 0U;
                action.active_effect_gate = 0U;
                shared.action_block_gate = 0U;
                action.action_pending_aux = 0U;
                action.active_effect_target = 0xFFFFFFFFU;
                if (!reset_actor_runtime(
                        state,
                        context,
                        result,
                        source_token,
                        0x004577CCU,
                        0x004577D1U
                    )) {
                    return result;
                }
                result.return_value =
                    result.actor_runtime_reset.last.return_eax;
                return result;
            }

            if (state.phase_mode == 1U) {
                if (shared.action_aux_gate == 0U &&
                    shared.action_block_gate == 0U) {
                    if (shared.action_side != 0U) {
                        shared.target_ready_gate = 1U;
                        if (!select_actor_target(
                                0U,
                                shared.action_side,
                                phase_terminal.edx,
                                logical_flags(shared.action_side),
                                0x00457925U,
                                0x0045792AU
                            )) {
                            return result;
                        }
                        state.phase_progress = to_bits(action.group_b_count) -
                            low_byte(action.opponent_processed_counter);
                    } else {
                        u32 scanned = 0U;
                        if (action.group_a_count > 0) {
                            stale_ebx = 1U;
                            for (i32 index = 0; index < action.group_a_count;
                                 ++index) {
                                const u32 uindex = to_bits(index);
                                if (!validate_group_a(result, uindex)) {
                                    return result;
                                }
                                const u32 target = group_a_token(uindex);
                                bool target_available = false;
                                if (invoke(
                                        port,
                                        result,
                                        kCallQueryTerminal,
                                        {target}
                                    )
                                            .eax != 1U &&
                                    shared.actor_ai_primary[uindex] != 1U &&
                                    invoke(
                                        port,
                                        result,
                                        kCallQueryActorBlocked,
                                        {target}
                                    )
                                            .eax != 1U) {
                                    const auto excluded = invoke(
                                        port,
                                        result,
                                        kCallQueryActorExcluded,
                                        {target}
                                    );
                                    if (excluded.eax != 1U) {
                                        u32 target_turn_completion{};
                                        if (!query_turn_completion(
                                                result,
                                                context,
                                                {.action = &action,
                                                 .startup = context.startup},
                                                target,
                                                excluded.eax,
                                                excluded.edx,
                                                subtract_flags(
                                                    excluded.eax, stale_ebx
                                                ),
                                                0x00457852U,
                                                target_turn_completion
                                            )) {
                                            return result;
                                        }
                                        if (target_turn_completion == 0U) {
                                            auto request =
                                                context
                                                    .actor_idle_state_request;
                                            request.actor_token = source_token;
                                            request.entry_eax =
                                                target_turn_completion;
                                            request.entry_edx =
                                                result.actor_turn_completion
                                                    .return_edx;
                                            request.entry_return_address =
                                                0x0045785DU;
                                            request.entry_flags = logical_flags(
                                                target_turn_completion
                                            );
                                            request.entry_flags_known = true;
                                            u32 idle_state{};
                                            if (!query_idle_state(
                                                    result,
                                                    idle_state_owners,
                                                    request,
                                                    idle_state
                                                )) {
                                                return result;
                                            }
                                            target_available = idle_state == 0U;
                                        }
                                    }
                                }
                                if (target_available) {
                                    const auto cleared = invoke(
                                        port,
                                        result,
                                        kCallClearControl,
                                        {target, 0U}
                                    );
                                    if (!increment_actor_start_gate(
                                            action,
                                            context,
                                            result,
                                            target,
                                            0x0045786BU,
                                            0x00457870U,
                                            cleared.eax,
                                            cleared.edx,
                                            cleared.flags
                                        )) {
                                        return result;
                                    }
                                    ++state.phase_progress;
                                }
                                ++scanned;
                                ++result.group_a_iterations;
                            }
                        }
                        const u32 threshold = scanned -
                            high_word(shared.defeated_actor_packed) -
                            shared.excluded_actor_count -
                            low_byte(action.packed_actor_counter);
                        if (std::bit_cast<i32>(state.phase_progress) >=
                            std::bit_cast<i32>(threshold)) {
                            u32 selected = 0U;
                            while (selected < scanned) {
                                if (!validate_group_a(result, selected)) {
                                    return result;
                                }
                                if (invoke(
                                        port,
                                        result,
                                        kCallQueryTerminal,
                                        {group_a_token(selected)}
                                    )
                                        .eax != 1U) {
                                    shared.target_ready_gate = 1U;
                                    const auto prepared_selection = invoke(
                                        port,
                                        result,
                                        kCallPrepareSelection,
                                        {source_token}
                                    );
                                    if (!select_actor_target(
                                            static_cast<u16>(selected),
                                            prepared_selection.eax,
                                            prepared_selection.edx,
                                            prepared_selection.flags,
                                            0x00457903U,
                                            0x00457908U
                                        )) {
                                        return result;
                                    }
                                    break;
                                }
                                ++selected;
                            }
                            action.action_pending_aux = 1U;
                        }
                    }
                }
                goto action_decision_done;
            } else if (state.selection_initialized == 0U) {
                const bool selection_mode =
                    invoke(
                        port, result, kCallQuerySelectionMode, {source_token}
                    )
                        .eax != 0U;
                if (selection_mode) {
                    const u32 remaining = to_bits(action.group_b_count) -
                        low_byte(action.opponent_processed_counter);
                    if (remaining == 1U) {
                        while (true) {
                            u32 selected = state.random_target_index;
                            if (action.group_a_count != 0) {
                                selected = invoke(
                                               port,
                                               result,
                                               kCallRandomBounded,
                                               {to_bits(action.group_a_count)}
                                )
                                               .eax;
                                state.random_target_index = selected;
                            }
                            if (!validate_group_a(result, selected)) {
                                return result;
                            }
                            if (shared.actor_ai_secondary[selected] != 1U &&
                                shared.actor_ai_primary[selected] != 1U &&
                                invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_a_token(selected)}
                                )
                                        .eax != 1U) {
                                break;
                            }
                        }
                    } else {
                        while (true) {
                            u32 selected = state.random_target_index;
                            if (action.group_b_count != 0) {
                                selected = invoke(
                                               port,
                                               result,
                                               kCallRandomBounded,
                                               {to_bits(action.group_b_count)}
                                )
                                               .eax;
                                state.random_target_index = selected;
                            }
                            if (!validate_group_b(result, selected)) {
                                return result;
                            }
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_b_token(selected)}
                                )
                                        .eax != 1U &&
                                selected != group_b_index) {
                                break;
                            }
                        }
                        shared.action_side = 1U;
                    }
                } else {
                    while (true) {
                        u32 selected = state.random_target_index;
                        if (action.group_a_count != 0) {
                            selected = invoke(
                                           port,
                                           result,
                                           kCallRandomBounded,
                                           {to_bits(action.group_a_count)}
                            )
                                           .eax;
                            state.random_target_index = selected;
                        }
                        if (!validate_group_a(result, selected)) {
                            return result;
                        }
                        if (shared.actor_ai_secondary[selected] != 1U &&
                            shared.actor_ai_primary[selected] != 1U &&
                            invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {group_a_token(selected)}
                            )
                                    .eax != 1U) {
                            break;
                        }
                    }
                    LegacyBattleActorGroupBElementState* opponent = nullptr;
                    if (context.startup != nullptr &&
                        context.startup->group_b_lifecycle != nullptr) {
                        opponent = &(
                            *context.startup->group_b_lifecycle
                        )[group_b_index];
                    }

                    const auto opponent_mode =
                        select_legacy_battle_group_b_opponent_mode(
                            opponent, context.bounded_random
                        );
                    ++result.group_b_opponent_mode_calls;
                    if (opponent_mode.status !=
                        LegacyBattleGroupBOpponentModeStatus::completed) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_opponent_mode_typed_stop;
                        result.return_value = opponent_mode.return_eax;
                        return result;
                    }
                    if (opponent_mode.return_eax == 1U) {
                        shared.action_side = 1U;
                        state.random_target_index = group_b_index;
                    }
                }
                state.selection_initialized = 1U;
            }

            auto selection_idle_request = context.actor_idle_state_request;
            selection_idle_request.actor_token = source_token;
            selection_idle_request.entry_return_address = 0x00457AC2U;
            u32 selection_idle_state{};
            if (!query_idle_state(
                    result,
                    idle_state_owners,
                    selection_idle_request,
                    selection_idle_state
                )) {
                return result;
            }
            if (selection_idle_state == 0U) {
                const u16 status = state.status_words[group_b_index];
                state.phase_mode = 0U;
                if (high_byte(status) != 0U) {
                    state.random_target_index = low_byte(status);
                }
                const u32 profile_offset = state.action_profile_index * 14U;
                if (profile_offset >= state.action_profile_bytes.size()) {
                    result.status = LegacyBattleActionDispatchStatus::
                        target_table_typed_stop;
                    return result;
                }
                const u32 profile_argument =
                    (state.stale_action_profile_edx & 0xFFFFFF00U) |
                    state.action_profile_bytes[profile_offset];
                LegacyBattleActorGroupBElementState* status_actor = nullptr;
                if (context.startup != nullptr &&
                    context.startup->group_b_lifecycle != nullptr) {
                    status_actor =
                        &(*context.startup->group_b_lifecycle)[group_b_index];
                }
                result.group_b_status_action =
                    query_legacy_battle_group_b_status_action(
                        status_actor,
                        context.bounded_random,
                        {
                            .actor_token = source_token,
                            .entry_eax = profile_offset,
                            .entry_edx = profile_argument,
                        }
                    );
                ++result.group_b_status_action_calls;
                if (result.group_b_status_action.status !=
                    LegacyBattleGroupBStatusActionStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_status_action_typed_stop;
                    result.return_value =
                        result.group_b_status_action.return_eax;
                    return result;
                }
                if (result.group_b_status_action.return_eax != 0U) {
                    if (!select_actor_target(
                            0U,
                            result.group_b_status_action.return_eax,
                            result.group_b_status_action.return_edx,
                            logical_flags(
                                result.group_b_status_action.return_eax
                            ),
                            0x00457E26U,
                            0x00457E2BU
                        )) {
                        return result;
                    }
                    const auto& published = result.actor_target_selection.last;
                    if (!apply_legacy_battle_actor_action_mode_call(
                            action,
                            context,
                            result,
                            source_token,
                            0x11U,
                            published.return_eax,
                            published.return_edx,
                            0x00457E34U,
                            published.flags
                        )) {
                        return result;
                    }
                    static_cast<void>(invoke(
                        port, result, kCallPublishStatusMode, {source_token, 2U}
                    ));
                } else {
                    const bool status_branch = std::bit_cast<i16>(status) < 0 ||
                        (status & 0x6000U) != 0U;
                    if (status_branch) {
                        u32 action_mode_eax =
                            result.group_b_status_action.return_eax;
                        u32 action_mode_edx =
                            result.group_b_status_action.return_edx;
                        const u32 stale_special_selection_pending =
                            state.special_selection_pending;
                        if (state.special_selection_pending == 1U) {
                            shared.action_side = 1U;
                            state.special_selection_pending = 0U;
                        }
                        if (std::bit_cast<i16>(status) < 0) {
                            LegacyBattleActorGroupBElementState* actor =
                                nullptr;
                            if (context.startup != nullptr &&
                                context.startup->group_b_lifecycle != nullptr) {
                                actor = &(
                                    *context.startup->group_b_lifecycle
                                )[group_b_index];
                            }
                            result.group_b_action_profile_mode =
                                compose_legacy_battle_group_b_action_profile_mode(
                                    actor,
                                    port,
                                    {
                                        .actor_token = source_token,
                                        .entry_eax = 0x8000U,
                                        .entry_ecx = source_token,
                                        .entry_edx =
                                            stale_special_selection_pending,
                                        .action_mode_requests = {
                                            context.actor_action_mode_requests
                                                [result
                                                     .actor_action_mode_calls],
                                            context.actor_action_mode_requests
                                                [result
                                                     .actor_action_mode_calls],
                                        },
                                    }
                                );
                            ++result.group_b_action_profile_mode_calls;
                            if (result.group_b_action_profile_mode
                                    .mode_update_calls != 0U) {
                                result.actor_action_mode =
                                    result.group_b_action_profile_mode
                                        .actor_action_mode;
                                result.actor_action_modes
                                    [result.actor_action_mode_calls] =
                                    result.actor_action_mode;
                                ++result.actor_action_mode_calls;
                            }
                            if (result.group_b_action_profile_mode.status !=
                                LegacyBattleGroupBActionProfileModeStatus::
                                    completed) {
                                result.status =
                                    LegacyBattleActionDispatchStatus::
                                        group_b_action_profile_mode_typed_stop;
                                result.return_value =
                                    result.group_b_action_profile_mode
                                        .return_eax;
                                return result;
                            }
                            state.status_action_value =
                                result.group_b_action_profile_mode.return_eax;
                            action_mode_eax =
                                result.group_b_action_profile_mode.return_eax;
                            action_mode_edx =
                                result.group_b_action_profile_mode.return_edx;
                            action.current_actor_index =
                                static_cast<u16>(group_b_index);
                        }
                        if ((status & 0x4000U) != 0U) {
                            if (!apply_legacy_battle_actor_action_mode_call(
                                    action,
                                    context,
                                    result,
                                    source_token,
                                    2U,
                                    action_mode_eax,
                                    action_mode_edx,
                                    0x00457C68U,
                                    logical_flags(
                                        static_cast<u32>(status >> 8U) & 0x40U
                                    )
                                )) {
                                return result;
                            }
                            action_mode_eax =
                                result.actor_action_mode.return_eax;
                            action_mode_edx =
                                result.actor_action_mode.return_edx;
                            action.current_actor_index =
                                static_cast<u16>(group_b_index);
                            if (state.opponent_text_present[group_b_index] !=
                                0U) {
                                if (!publish_text_message(
                                        context,
                                        port,
                                        result,
                                        {0x118U,
                                         0U,
                                         0x28U,
                                         state.opponent_text_token_base +
                                             group_b_index *
                                                 kLegacyBattleActionGroupBStride,
                                         0x40U}
                                    )) {
                                    return result;
                                }
                                action_mode_eax = result.text_messages.back()
                                                      .return_registers.eax;
                                action_mode_edx = result.text_messages.back()
                                                      .return_registers.edx;
                            }
                        }
                        if ((status & 0x2000U) != 0U) {
                            if (!apply_legacy_battle_actor_action_mode_call(
                                    action,
                                    context,
                                    result,
                                    source_token,
                                    6U,
                                    action_mode_eax,
                                    action_mode_edx,
                                    0x00457CA0U,
                                    logical_flags(
                                        static_cast<u32>(status >> 8U) & 0x20U
                                    )
                                )) {
                                return result;
                            }
                            action_mode_eax =
                                result.actor_action_mode.return_eax;
                            action_mode_edx =
                                result.actor_action_mode.return_edx;
                            action.current_actor_index =
                                static_cast<u16>(group_b_index);
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQuerySpecialAction,
                                {source_token}
                            )
                                .eax == 1U) {
                            state.special_action_latch = 1U;
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQueryPhaseMode,
                                {source_token}
                            )
                                .eax == 1U) {
                            state.phase_mode = 1U;
                            state.phase_progress = 0U;
                        }
                        state.branch_misc = 0U;
                        if (shared.action_side == 1U) {
                            u32 selected = state.random_target_index;
                            if (!validate_group_b(result, selected)) {
                                return result;
                            }
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_b_token(selected)}
                                )
                                    .eax == 1U) {
                                while (true) {
                                    ++state.random_target_index;
                                    selected = state.random_target_index;
                                    if (selected > 8U) {
                                        break;
                                    }
                                    if (!validate_group_b(result, selected)) {
                                        return result;
                                    }
                                    if (invoke(
                                            port,
                                            result,
                                            kCallQueryTerminal,
                                            {group_b_token(selected)}
                                        )
                                            .eax != 1U) {
                                        break;
                                    }
                                }
                            }
                            if (!validate_group_b(
                                    result, state.random_target_index
                                )) {
                                return result;
                            }
                            const auto target_terminal = invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {group_b_token(state.random_target_index)}
                            );
                            if (target_terminal.eax != 0U) {
                                goto action_decision_done;
                            }
                            if (!select_actor_target(
                                    static_cast<u16>(state.random_target_index),
                                    target_terminal.eax,
                                    target_terminal.edx,
                                    logical_flags(target_terminal.eax),
                                    0x00457D72U,
                                    0x00457D77U
                                )) {
                                return result;
                            }
                        } else {
                            u32 selected = state.random_target_index;
                            if (!validate_group_a(result, selected)) {
                                return result;
                            }
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_a_token(selected)}
                                )
                                    .eax == 1U) {
                                while (true) {
                                    ++state.random_target_index;
                                    selected = state.random_target_index;
                                    if (selected > 8U) {
                                        break;
                                    }
                                    if (!validate_group_a(result, selected)) {
                                        return result;
                                    }
                                    if (invoke(
                                            port,
                                            result,
                                            kCallQueryTerminal,
                                            {group_a_token(selected)}
                                        )
                                            .eax != 1U) {
                                        break;
                                    }
                                }
                            }
                            if (!validate_group_a(
                                    result, state.random_target_index
                                )) {
                                return result;
                            }
                            const u32 selected_token =
                                group_a_token(state.random_target_index);
                            const auto target_terminal = invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {selected_token}
                            );
                            if (target_terminal.eax == 0U) {
                                if (!select_actor_target(
                                        static_cast<u16>(
                                            state.random_target_index
                                        ),
                                        target_terminal.eax,
                                        target_terminal.edx,
                                        logical_flags(target_terminal.eax),
                                        0x00457DFBU,
                                        0x00457E00U
                                    )) {
                                    return result;
                                }
                                if (!increment_selected_group_a_start_gate(
                                        action,
                                        context,
                                        result,
                                        state.random_target_index,
                                        selected_token
                                    )) {
                                    return result;
                                }
                            }
                        }
                    } else {
                        const auto status_sequence = invoke(
                            port,
                            result,
                            kCallQueryStatusSequence,
                            {source_token, 0x0053BD40U}
                        );
                        if (!apply_legacy_battle_pending_actor_ready_action_modes(
                                action,
                                context,
                                result,
                                source_token,
                                status_sequence
                            )) {
                            return result;
                        }
                        u32 profile_flag_entry_eax = status_sequence.eax;
                        u32 profile_flag_entry_edx = status_sequence.edx;
                        if (status_sequence.eax == 1U) {
                            action.current_actor_index =
                                static_cast<u16>(group_b_index);
                            state.status_misc = 0U;
                            const auto special_action = invoke(
                                port,
                                result,
                                kCallQuerySpecialAction,
                                {source_token}
                            );
                            if (special_action.eax == 1U) {
                                state.special_action_latch = 1U;
                            }

                            profile_flag_entry_eax =
                                state.opponent_text_token_base +
                                group_b_index * kLegacyBattleActionGroupBStride;
                            profile_flag_entry_edx = special_action.edx;
                            if (state.opponent_text_present[group_b_index] !=
                                0U) {
                                if (!publish_text_message(
                                        context,
                                        port,
                                        result,
                                        {0x118U,
                                         0U,
                                         0x28U,
                                         profile_flag_entry_eax,
                                         0x40U}
                                    )) {
                                    return result;
                                }

                                const auto& text_message =
                                    result.text_messages.back();
                                profile_flag_entry_eax =
                                    text_message.return_registers.eax;
                                profile_flag_entry_edx =
                                    text_message.return_registers.edx;
                            }
                        }

                        const LegacyBattleActorGroupBElementState* actor =
                            nullptr;
                        if (context.startup != nullptr &&
                            context.startup->group_b_lifecycle != nullptr) {
                            actor = &(
                                *context.startup->group_b_lifecycle
                            )[group_b_index];
                        }

                        const auto profile_flag =
                            query_legacy_battle_group_b_action_profile_flag(
                                actor,
                                {
                                    .actor_token = source_token,
                                    .entry_eax = profile_flag_entry_eax,
                                    .entry_edx = profile_flag_entry_edx,
                                }
                            );
                        ++result.group_b_action_profile_flag_calls;
                        if (profile_flag.status !=
                            LegacyBattleGroupBActionProfileFlagStatus::
                                completed) {
                            result.status = LegacyBattleActionDispatchStatus::
                                group_b_action_profile_flag_typed_stop;
                            result.return_value = profile_flag.return_eax;
                            return result;
                        }

                        if (profile_flag.return_eax == 1U) {
                            shared.action_side = 1U;
                            state.random_target_index = group_b_index;
                        }
                        action.current_actor_index =
                            static_cast<u16>(group_b_index);
                        if (invoke(
                                port,
                                result,
                                kCallQueryPhaseMode,
                                {source_token}
                            )
                                .eax == 1U) {
                            state.phase_mode = 1U;
                            state.phase_progress = 0U;
                            goto action_decision_done;
                        }
                        if (state.phase_mode == 0U) {
                            if (shared.action_side == 1U) {
                                if (!validate_group_b(
                                        result, state.random_target_index
                                    )) {
                                    return result;
                                }
                                const auto target_terminal = invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_b_token(state.random_target_index)}
                                );
                                if (target_terminal.eax == 0U &&
                                    !select_actor_target(
                                        static_cast<u16>(
                                            state.random_target_index
                                        ),
                                        target_terminal.eax,
                                        (target_terminal.edx & 0xFFFF0000U) |
                                            static_cast<u16>(
                                                state.random_target_index
                                            ),
                                        logical_flags(target_terminal.eax),
                                        0x00457C18U,
                                        0x00457C1DU
                                    )) {
                                    return result;
                                }
                            } else {
                                if (!validate_group_a(
                                        result, state.random_target_index
                                    )) {
                                    return result;
                                }
                                const u32 selected_token =
                                    group_a_token(state.random_target_index);
                                const auto target_terminal = invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {selected_token}
                                );
                                if (target_terminal.eax == 0U) {
                                    if (!select_actor_target(
                                            static_cast<u16>(
                                                state.random_target_index
                                            ),
                                            target_terminal.eax,
                                            target_terminal.edx,
                                            logical_flags(target_terminal.eax),
                                            0x00457DFBU,
                                            0x00457E00U
                                        )) {
                                        return result;
                                    }
                                    if (!increment_selected_group_a_start_gate(
                                            action,
                                            context,
                                            result,
                                            state.random_target_index,
                                            selected_token
                                        )) {
                                        return result;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

action_decision_done:
    if (state.frame_enabled == 1U && shared.action_aux_gate == 0U &&
        shared.turn_resolution_bits == 0U) {
        auto decision_idle_request = context.actor_idle_state_request;
        decision_idle_request.actor_token = source_token;
        decision_idle_request.entry_return_address = 0x00457E44U;
        u32 decision_idle_state{};
        if (!query_idle_state(
                result,
                idle_state_owners,
                decision_idle_request,
                decision_idle_state
            )) {
            return result;
        }
        if (decision_idle_state == 1U) {
            stale_ebx = 1U;
            const bool active_actor =
                action.active_effect_target == group_b_index;
            LegacyBattleActionCallReply action_start_reply{};
            if (active_actor) {
                shared.action_block_gate = 1U;
                action.action_pending_aux = 1U;
                action_start_reply = invoke(
                    port, result, kCallPublishActionStart, {source_token}
                );
            }
            action.current_actor_index = static_cast<u16>(group_b_index);
            auto action_target_request =
                context.group_b_frame_action_target_requests[0U];
            action_target_request.actor_token = source_token;
            action_target_request.entry_eax = active_actor
                ? action_start_reply.eax
                : action.active_effect_target;
            action_target_request.entry_edx = active_actor
                ? action_start_reply.edx
                : result.actor_idle_state.return_edx;
            action_target_request.entry_return_address = 0x00457E8FU;
            if (!active_actor) {
                action_target_request.entry_flags =
                    subtract_flags(action.active_effect_target, group_b_index);
                action_target_request.entry_flags_known = true;
            }
            u32 action_target_value{};
            if (!query_action_target(
                    result,
                    {.action = &action, .startup = context.startup},
                    action_target_request,
                    action_target_value
                )) {
                return result;
            }
            const u32 target_index = low_word(action_target_value);
            auto nested_context = context;
            nested_context.actor_gate_decay_request_offset +=
                result.actor_gate_decay.calls;
            auto nested = dispatch_legacy_battle_opponent_action(
                action, port, nested_context, group_b_index, target_index
            );
            merge_nested(result, nested);
            if (nested.status != LegacyBattleActionDispatchStatus::completed) {
                return result;
            }
            if (nested.return_value == 1U) {
                action_target_request =
                    context.group_b_frame_action_target_requests[1U];
                action_target_request.actor_token = source_token;
                action_target_request.entry_eax = nested.return_value;
                action_target_request.entry_return_address = 0x00457EB3U;
                action_target_request.entry_flags = logical_flags(0U);
                action_target_request.entry_flags_known = true;
                if (!query_action_target(
                        result,
                        {.action = &action, .startup = context.startup},
                        action_target_request,
                        action_target_value
                    )) {
                    return result;
                }
                const u32 completed_target = to_bits(
                    static_cast<i32>(
                        std::bit_cast<i16>(low_word(action_target_value))
                    )
                );
                static_cast<void>(
                    invoke(port, result, kCallSelectionClear, {source_token})
                );
                (*context.startup->group_b_lifecycle)[group_b_index]
                    .action_execution.action_target = 0xFFFFU;
                const auto selection_complete = invoke(
                    port, result, kCallSelectionComplete, {source_token}
                );
                if (selection_complete.eax == 1U) {
                    u32 decay_entry_eax = selection_complete.eax;
                    u32 decay_entry_edx = selection_complete.edx;
                    auto decay_entry_flags =
                        subtract_flags(to_bits(action.group_a_count), 0U);
                    if (action.group_a_count > 0) {
                        for (i32 index = 0; index < action.group_a_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_a(result, uindex)) {
                                return result;
                            }
                            const u32 target = group_a_token(uindex);
                            if (!decay_actor_gates(
                                    action,
                                    context,
                                    result,
                                    target,
                                    0x00457EEEU,
                                    0x00457EF3U,
                                    decay_entry_eax,
                                    decay_entry_edx,
                                    decay_entry_flags
                                )) {
                                return result;
                            }
                            const auto queue_completion = invoke(
                                port,
                                result,
                                kCallQueryQueueCompletion,
                                {target}
                            );
                            u32 tail_edx = queue_completion.edx;
                            if (queue_completion.eax == 1U &&
                                state.group_a_completion_words[uindex] != 0U) {
                                state.group_a_completion_words[uindex] = 0U;
                                replace_low_word(
                                    shared.defeated_actor_packed, 0U
                                );
                                state.group_a_completion_slots[uindex] = 0U;
                                LegacyBattlePartyStartupRecord* party = nullptr;
                                if (context.startup != nullptr &&
                                    uindex < context.startup->party.size()) {
                                    party = &context.startup->party[uindex];
                                }
                                auto& actor =
                                    action.group_a_action_execution[uindex];
                                result.group_a_actor_cleanup =
                                    cleanup_legacy_battle_group_a_actor(
                                        {
                                            .actor = &actor,
                                            .workspace = party != nullptr
                                                ? &party->workspace
                                                : nullptr,
                                            .final_processing = party != nullptr
                                                ? &party->final_processing
                                                : nullptr,
                                            .item_effect = party != nullptr
                                                ? &party
                                                       ->item_effect_application
                                                : nullptr,
                                            .attribute_effect = party != nullptr
                                                ? &party->attribute_effect
                                                : nullptr,
                                            .actor_list = party != nullptr
                                                ? &party->actor_list
                                                : nullptr,
                                        },
                                        target
                                    );
                                ++result.group_a_actor_cleanup_calls;
                                if (result.group_a_actor_cleanup.status !=
                                    LegacyBattleGroupAActorCleanupStatus::
                                        completed) {
                                    result.status =
                                        LegacyBattleActionDispatchStatus::
                                            group_a_actor_cleanup_typed_stop;
                                    return result;
                                }
                                if (!reset_actor_runtime(
                                        state,
                                        context,
                                        result,
                                        target,
                                        0x00457F23U,
                                        0x00457F28U
                                    )) {
                                    return result;
                                }
                                const auto& reset_reply =
                                    result.actor_runtime_reset.last;
                                result.actor_progress_threshold_sync =
                                    synchronize_legacy_battle_actor_progress_threshold(
                                        party != nullptr ? &party->progress
                                                         : nullptr,
                                        context.startup != nullptr
                                            ? &context.startup->timing
                                            : nullptr,
                                        {
                                            .actor_token = target,
                                            .entry_eax = reset_reply.return_eax,
                                            .entry_edx = reset_reply.return_edx,
                                        }
                                    );
                                ++result.actor_progress_threshold_sync_calls;
                                if (result.actor_progress_threshold_sync
                                        .status !=
                                    LegacyBattleActorProgressThresholdSyncStatus::
                                        completed) {
                                    result
                                        .status = LegacyBattleActionDispatchStatus::
                                        actor_progress_threshold_sync_typed_stop;
                                    result.return_value =
                                        result.actor_progress_threshold_sync
                                            .return_eax;
                                    return result;
                                }
                                u16 table_value{};
                                if (!read_completion_value(
                                        state,
                                        result,
                                        to_bits(action.group_a_count),
                                        table_value
                                    )) {
                                    return result;
                                }
                                const u32 argument =
                                    (result.actor_progress_threshold_sync
                                         .return_eax &
                                     0xFFFF0000U) |
                                    table_value;
                                if (!publish_player_item_quantity(
                                        port, result, argument, 0U
                                    )) {
                                    return result;
                                }
                                tail_edx = 0U;
                            }
                            ++result.group_a_iterations;
                            decay_entry_eax = to_bits(action.group_a_count);
                            decay_entry_edx = tail_edx;
                            decay_entry_flags = subtract_flags(
                                uindex + 1U, to_bits(action.group_a_count)
                            );
                        }
                    }
                } else if (shared.action_side != 0U) {
                    const u32 times_three =
                        completed_target + completed_target * 2U;
                    const u32 times_twenty_four = times_three << 3U;
                    const u32 times_twenty_three =
                        times_twenty_four - completed_target;
                    const u32 times_sixty_nine =
                        times_twenty_three + times_twenty_three * 2U;
                    const u32 times_three_hundred_forty_five =
                        times_sixty_nine + times_sixty_nine * 4U;
                    const u32 times_one_thousand_three_hundred_eighty_one =
                        completed_target + times_three_hundred_forty_five * 4U;
                    if (!decay_actor_gates(
                            action,
                            context,
                            result,
                            group_b_token(completed_target),
                            0x00458025U,
                            0x0045802AU,
                            times_one_thousand_three_hundred_eighty_one,
                            times_three_hundred_forty_five,
                            subtract_flags(times_twenty_four, completed_target)
                        )) {
                        return result;
                    }
                } else {
                    if (!validate_group_a(result, completed_target)) {
                        return result;
                    }
                    const u32 target = group_a_token(completed_target);
                    const auto queue_completion = invoke(
                        port, result, kCallQueryQueueCompletion, {target}
                    );
                    u32 decay_entry_eax = queue_completion.eax;
                    u32 decay_entry_edx = queue_completion.edx;
                    auto decay_entry_flags =
                        subtract_flags(queue_completion.eax, 1U);
                    bool decay_entry_flags_known = true;
                    if (queue_completion.eax == 1U &&
                        state.group_a_completion_words[completed_target] !=
                            0U) {
                        state.group_a_completion_words[completed_target] = 0U;
                        replace_low_word(shared.defeated_actor_packed, 0U);
                        state.group_a_completion_slots[completed_target] = 0U;
                        if (!reset_actor_runtime(
                                state,
                                context,
                                result,
                                target,
                                0x00457FD0U,
                                0x00457FD5U
                            )) {
                            return result;
                        }
                        const auto& reset_reply =
                            result.actor_runtime_reset.last;
                        auto* const startup = context.startup;
                        result.actor_progress_threshold_sync =
                            synchronize_legacy_battle_actor_progress_threshold(
                                startup != nullptr
                                    ? &startup->party[completed_target].progress
                                    : nullptr,
                                startup != nullptr ? &startup->timing : nullptr,
                                {
                                    .actor_token = target,
                                    .entry_eax = reset_reply.return_eax,
                                    .entry_edx = reset_reply.return_edx,
                                }
                            );
                        ++result.actor_progress_threshold_sync_calls;
                        if (result.actor_progress_threshold_sync.status !=
                            LegacyBattleActorProgressThresholdSyncStatus::
                                completed) {
                            result.status = LegacyBattleActionDispatchStatus::
                                actor_progress_threshold_sync_typed_stop;
                            result.return_value =
                                result.actor_progress_threshold_sync.return_eax;
                            return result;
                        }
                        u16 table_value{};
                        if (!read_completion_value(
                                state,
                                result,
                                to_bits(action.group_a_count),
                                table_value
                            )) {
                            return result;
                        }
                        const u32 argument =
                            (result.actor_progress_threshold_sync.return_ecx &
                             0xFFFF0000U) |
                            table_value;
                        if (!publish_player_item_quantity(
                                port, result, argument, 0U
                            )) {
                            return result;
                        }
                        decay_entry_eax = result.player_item.return_token;
                        decay_entry_edx = 0U;
                        decay_entry_flags_known = false;
                    } else if (queue_completion.eax == 1U) {
                        decay_entry_flags = subtract_word_flags(
                            state.group_a_completion_words[completed_target], 0U
                        );
                    }
                    if (!decay_actor_gates(
                            action,
                            context,
                            result,
                            target,
                            0x00458002U,
                            0x00458007U,
                            decay_entry_eax,
                            decay_entry_edx,
                            decay_entry_flags,
                            decay_entry_flags_known
                        )) {
                        return result;
                    }
                }

                if (!reset_actor_runtime(
                        state,
                        context,
                        result,
                        source_token,
                        0x0045802CU,
                        0x00458031U
                    )) {
                    return result;
                }
                if ((shared.battle_byte_flags & 0x80U) != 0U) {
                    if (action.group_a_count > 0) {
                        for (i32 index = 0; index < action.group_a_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_a(result, uindex)) {
                                return result;
                            }
                            const u32 target = group_a_token(uindex);
                            if (shared.actor_ai_primary[uindex] == 0U &&
                                invoke(
                                    port, result, kCallQueryTerminal, {target}
                                )
                                        .eax == 0U) {
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPublishBattleBit,
                                    {target, 0U}
                                ));
                            }
                            ++result.group_a_iterations;
                        }
                    }
                    replace_low_byte(
                        shared.battle_byte_flags,
                        static_cast<u8>(shared.battle_byte_flags & 0x7FU)
                    );
                }
                state.selection_initialized = 0U;
                shared.action_block_gate = 0U;
                action.active_effect_gate = 0U;
                action.action_pending_aux = 0U;
                if (action.active_effect_target < 8U) {
                    action.active_effect_target = 0U;
                    shared.action_side = 0U;
                    shared.active_effect_tail.fill(0U);
                    action.active_effect_target = 0xFFFFFFFFU;
                    action.active_target_code = 0U;
                }
                shared.action_stage_word = 0U;
                shared.action_stage_word_b = 0U;
                shared.target_ready_gate = 0U;
                state.special_action_latch = 0U;
                state.phase_mode = 0U;
                state.phase_progress = 0U;
                replace_low_word(action.action_runtime_flags, 0U);
                state.status_words[group_b_index] = 0U;
                action.post_battle_counter = 0U;
                if (action.frame_effect.primary_suppression == 1U) {
                    action.frame_effect.fade_active = 1U;
                }
                if ((shared.global_phase_countdown & 0x7FFFU) != 0U) {
                    replace_low_word(
                        shared.global_phase_countdown,
                        static_cast<u16>(shared.global_phase_countdown - 1U)
                    );
                }

                if (!validate_group_a(result, completed_target)) {
                    return result;
                }
                const auto completion_effect = invoke(
                    port,
                    result,
                    kCallQueryCompletionEffect,
                    {group_a_token(completed_target)}
                );
                if (completion_effect.eax == 1U) {
                    if (!execute_legacy_battle_actor_effect_resource_slot_write_call(
                            {
                                .action = &action,
                                .startup = context.startup,
                            },
                            result.effect_resource_slot_write,
                            context.effect_resource_slot_write_requests,
                            source_token,
                            0x235EU,
                            completion_effect.eax,
                            completion_effect.edx,
                            0x0045815AU,
                            0x0045815FU,
                            subtract_flags(completion_effect.eax, 1U)
                        )) {
                        result.status = LegacyBattleActionDispatchStatus::
                            actor_effect_resource_slot_write_typed_stop;
                        result.return_value =
                            result.effect_resource_slot_write.last.return_eax;
                        return result;
                    }
                    if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
                            {
                                .action = &action,
                                .startup = context.startup,
                            },
                            result.actor_field_26b8_high_bit_set,
                            context.actor_field_26b8_high_bit_set_requests,
                            source_token,
                            result.effect_resource_slot_write.last.return_eax,
                            result.effect_resource_slot_write.last.return_edx,
                            0x00458166U,
                            result.effect_resource_slot_write.last.flags
                        )) {
                        result.status = LegacyBattleActionDispatchStatus::
                            actor_field_26b8_high_bit_set_typed_stop;
                        result.return_value =
                            result.actor_field_26b8_high_bit_set.last
                                .return_eax;
                        return result;
                    }
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishCompletionResource,
                        {source_token, state.completion_resource_token}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallSetCompletionMode, {source_token, 1U}
                    ));
                    synchronize_legacy_battle_actor_effect_resource_cursor_update(
                        {
                            .action = &action,
                            .startup = context.startup,
                        },
                        source_token,
                        1U
                    );
                    if (invoke(
                            port,
                            result,
                            kCallPrepareCompletionSurface,
                            {source_token,
                             state.completion_resource_token,
                             0U,
                             0U}
                        )
                            .eax == 1U) {
                        action.group_a_to_actor[group_b_index] = group_b_index;
                        state.completion_selected = 0xFFFFFFFFU;
                        state.completion_gate = 1U;
                        if (!fill_completion_surface(state, result)) {
                            return result;
                        }
                    }
                }
            } else {
                shared.action_block_gate = stale_ebx;
            }
        } else {
            shared.action_block_gate = stale_ebx;
        }
    }

    auto start_gate_request = context.actor_start_gate_request;
    start_gate_request.actor_token = source_token;
    start_gate_request.entry_eax =
        group_b_index * (kLegacyBattleActionGroupBStride >> 3U);
    start_gate_request.entry_return_address = 0x00458203U;
    start_gate_request.entry_flags =
        subtract_flags(group_b_index * 24U, group_b_index);
    start_gate_request.entry_flags_known = true;
    u32 start_gate{};
    if (!query_start_gate(
            result, start_gate_owners, start_gate_request, start_gate
        )) {
        return result;
    }
    const bool effect_mode = (low_word(start_gate) != 0U &&
                              (action.frame_effect.primary_suppression == 1U ||
                               action.frame_effect.split_suppression == 1U)) ||
        shared.global_effect_override == 1U;
    const auto effect_mode_reply = invoke(
        port,
        result,
        kCallPublishEffectMode,
        {source_token, effect_mode ? 1U : 0U}
    );
    if (!apply_legacy_battle_pending_actor_field_26b8_high_bit_set(
            action,
            context,
            result,
            source_token,
            0x00478BDBU,
            effect_mode_reply
        )) {
        return result;
    }
    if (!apply_legacy_battle_pending_actor_field_26b8_high_bit_clear(
            action, context, result, source_token, effect_mode_reply
        )) {
        return result;
    }

    if (state.pending_effect_ids[group_b_index] != 0xFFFFFFFFU) {
        SingleEffectPortAdapter effect_port(port);
        const auto effect = advance_legacy_battle_single_effect_frame(
            state.pending_effect_frame,
            effect_port,
            source_token,
            state.pending_effect_argument,
            group_b_index,
            {.action = &action, .startup = context.startup}
        );
        result.port_calls += effect.port_calls;
        if (effect.status != LegacyBattleSingleEffectFrameStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::effect_record_typed_stop;
            result.return_value = effect.return_value;
            return result;
        }
        if (effect.return_value == 1U) {
            state.pending_effect_ids[group_b_index] = 0xFFFFFFFFU;
        }
    }

    const u32 mapped_actor = action.group_a_to_actor[group_b_index];
    const auto final = advance_legacy_battle_final_actor_step(
        shared.final_actor_step,
        action,
        port,
        {
            .records = context.attack_order_records,
            .adjacent_intensity_record = context.attack_order_adjacent_record,
        },
        mapped_actor,
        0U,
        context.startup
    );
    merge_nested(result, final);
    if (final.status != LegacyBattleActionDispatchStatus::completed) {
        return result;
    }
    if (final.return_value == 1U) {
        action.group_a_to_actor[group_b_index] = 0xFFFFFFFFU;
        state.final_actor_state[group_b_index] = 0U;
        state.final_actor_targets[group_b_index] = 0xFFFFFFFFU;
        shared.queued_selection_word = 0xFFFFU;
        action.overlay_gate = 1U;
        state.final_gate = 0U;
    }
    return result;
}

}  // namespace openswd3::battle
