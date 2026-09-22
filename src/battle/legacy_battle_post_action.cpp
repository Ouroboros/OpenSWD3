#include "openswd3/battle/legacy_battle_post_action.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryTerminal = 0x0047CE80U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] constexpr bool even_parity(const compat::u8 value) noexcept {
    return (std::popcount(value) & 1) == 0;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<compat::u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<compat::u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
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

}  // namespace

LegacyBattleActionDispatchResult advance_legacy_battle_post_action(
    LegacyBattlePostActionState& state,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleStartupState* const startup,
    const LegacyBattleActorRuntimeResetCallRequests& runtime_reset_requests,
    const compat::u32 source_group_a_index,
    const compat::u32 target_group_b_index,
    const LegacyBattleActorActionTargetRequest& action_target_request,
    const LegacyBattleActorActionModeRequest& action_mode_request,
    const std::size_t runtime_reset_request_offset,
    const LegacyBattleActorTargetSelectionRequestList&
        target_selection_requests,
    const std::size_t target_selection_request_offset,
    const LegacyBattleActorGateDecayCallRequests& gate_decay_requests,
    const std::size_t gate_decay_request_offset,
    const LegacyBattleActorActionTargetClearCallRequests&
        action_target_clear_requests,
    const std::size_t action_target_clear_request_offset
) {
    LegacyBattleActionDispatchResult result;
    const auto reset_actor = [&](const u32 actor_token,
                                 const u32 call_address,
                                 const u32 return_address,
                                 const u32 entry_eax = 0U,
                                 const u32 entry_edx = 0U) {
        if (execute_legacy_battle_actor_runtime_reset_call(
                {.action = &action, .startup = startup},
                random,
                result.actor_runtime_reset,
                runtime_reset_requests,
                actor_token,
                entry_eax,
                entry_edx,
                call_address,
                return_address,
                {},
                false,
                runtime_reset_request_offset
            )) {
            return true;
        }

        result.status =
            LegacyBattleActionDispatchStatus::actor_runtime_reset_typed_stop;
        result.return_value = result.actor_runtime_reset.last.return_eax;
        return false;
    };
    const auto select_actor_target = [&](const u32 actor_token,
                                         const u16 target_index,
                                         const LegacyBattleActionCallReply&
                                             entry) {
        if (execute_legacy_battle_actor_target_selection_call(
                result.actor_target_selection,
                target_selection_requests,
                {.action = &action, .startup = startup},
                0x0045AF7DU,
                0x0045AF82U,
                actor_token,
                target_index,
                entry.eax,
                entry.edx,
                entry.flags,
                true,
                target_selection_request_offset
            )) {
            return true;
        }

        result.status =
            LegacyBattleActionDispatchStatus::actor_target_selection_typed_stop;
        result.return_value = result.actor_target_selection.last.return_eax;
        return false;
    };
    const auto decay_actor_gates =
        [&](const u32 actor_token,
            const u32 call_address,
            const u32 return_address,
            const u32 entry_eax,
            const u32 entry_edx,
            const LegacyBattleActorCoordinateFlags& entry_flags) {
            if (execute_legacy_battle_actor_gate_decay_call(
                    result.actor_gate_decay,
                    gate_decay_requests,
                    {.action = &action, .startup = startup},
                    call_address,
                    return_address,
                    actor_token,
                    entry_eax,
                    entry_edx,
                    entry_flags,
                    true,
                    gate_decay_request_offset
                )) {
                return true;
            }

            result.status =
                LegacyBattleActionDispatchStatus::actor_gate_decay_typed_stop;
            result.return_value = result.actor_gate_decay.last.return_eax;
            return false;
        };
    const auto clear_actor_action_target =
        [&](const u32 actor_token,
            const u32 call_address,
            const u32 return_address,
            const u32 entry_eax,
            const u32 entry_edx,
            const LegacyBattleActorCoordinateFlags& entry_flags) {
            if (execute_legacy_battle_actor_action_target_clear_call(
                    result.actor_action_target_clear,
                    action_target_clear_requests,
                    {.action = &action, .startup = startup},
                    call_address,
                    return_address,
                    actor_token,
                    entry_eax,
                    entry_edx,
                    entry_flags,
                    true,
                    action_target_clear_request_offset
                )) {
                return true;
            }

            result.status = LegacyBattleActionDispatchStatus::
                actor_action_target_clear_typed_stop;
            result.return_value =
                result.actor_action_target_clear.last.return_eax;
            return false;
        };
    const u32 selected = action.selected_target_index;
    result.return_value = selected;
    if (selected != target_group_b_index) {
        return result;
    }

    if (!reset_actor(
            group_b_token(target_group_b_index), 0x0045AE1DU, 0x0045AE22U
        )) {
        return result;
    }
    u32 action_target_entry_edx = result.actor_runtime_reset.last.return_edx;
    u32 group_a_index = 0U;
    const u32 group_a_count = to_bits(action.group_a_count);
    if (group_a_count == 0U) {
        result.return_value = 0U;
        return result;
    }

    while (group_a_index < group_a_count) {
        const u32 actor_token = group_a_token(group_a_index);
        if (group_a_index != source_group_a_index) {
            auto request = action_target_request;
            request.actor_token = actor_token;
            request.entry_eax = group_a_index;
            request.entry_edx = action_target_entry_edx;
            request.entry_return_address = 0x0045AE51U;
            request.entry_flags =
                subtract_flags(group_a_index, source_group_a_index);
            request.entry_flags_known = true;
            result.actor_action_target =
                query_legacy_battle_actor_action_target(
                    resolve_legacy_battle_actor_action_target(
                        {.action = &action, .startup = startup}, actor_token
                    ),
                    request
                );
            result.actor_action_targets[result.actor_action_target_calls] =
                result.actor_action_target;
            ++result.actor_action_target_calls;
            if (result.actor_action_target.status !=
                LegacyBattleActorActionTargetStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_action_target_typed_stop;
                result.return_value = result.actor_action_target.return_eax;
                return result;
            }
            const i32 queried = static_cast<i32>(signed_word(
                static_cast<u16>(result.actor_action_target.return_eax)
            ));
            if ((to_bits(queried) & 0x00008000U) == 0U &&
                selected == to_bits(queried)) {
                i32 observed_group_b_count = action.group_b_count;
                bool published = false;
                if (observed_group_b_count > 0) {
                    i32 candidate = 0;
                    while (candidate < observed_group_b_count) {
                        if (to_bits(candidate) != selected) {
                            const u32 candidate_token =
                                group_b_token(to_bits(candidate));
                            const auto terminal = invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {candidate_token}
                            );
                            if (terminal.eax == 0U) {
                                if (!clear_actor_action_target(
                                        actor_token,
                                        0x0045AF58U,
                                        0x0045AF5DU,
                                        terminal.eax,
                                        terminal.edx,
                                        logical_flags(terminal.eax)
                                    )) {
                                    return result;
                                }
                                const u32 queried_index = to_bits(queried);
                                const u32 times_three =
                                    queried_index + queried_index * 2U;
                                const u32 times_twenty_four = times_three << 3U;
                                const u32 times_twenty_three =
                                    times_twenty_four - queried_index;
                                const u32 times_sixty_nine =
                                    times_twenty_three +
                                    times_twenty_three * 2U;
                                const u32 times_three_hundred_forty_five =
                                    times_sixty_nine + times_sixty_nine * 4U;
                                if (!decay_actor_gates(
                                        group_b_token(queried_index),
                                        0x0045AF75U,
                                        0x0045AF7AU,
                                        times_three_hundred_forty_five,
                                        terminal.edx,
                                        subtract_flags(
                                            times_twenty_four, queried_index
                                        )
                                    )) {
                                    return result;
                                }
                                const LegacyBattleActionCallReply target_reset{
                                    .eax =
                                        result.actor_gate_decay.last.return_eax,
                                    .ecx =
                                        result.actor_gate_decay.last.return_ecx,
                                    .edx =
                                        result.actor_gate_decay.last.return_edx,
                                    .flags = result.actor_gate_decay.last.flags,
                                };
                                if (!select_actor_target(
                                        actor_token,
                                        static_cast<u16>(candidate),
                                        target_reset
                                    )) {
                                    return result;
                                }
                                action_target_entry_edx =
                                    result.actor_target_selection.last
                                        .return_edx;
                                state.selection_rebuild_pending = 1U;
                                published = true;
                                break;
                            }
                            observed_group_b_count = action.group_b_count;
                        }
                        ++candidate;
                    }
                }

                const u32 packed_counter_next =
                    static_cast<u32>(action.packed_actor_counter & 0xFFU) + 1U;
                if (!published &&
                    packed_counter_next == to_bits(observed_group_b_count)) {
                    if (!clear_actor_action_target(
                            actor_token,
                            0x0045AEC2U,
                            0x0045AEC7U,
                            to_bits(observed_group_b_count),
                            packed_counter_next,
                            subtract_flags(
                                packed_counter_next,
                                to_bits(observed_group_b_count)
                            )
                        )) {
                        return result;
                    }
                    const u32 queried_index = to_bits(queried);
                    const u32 times_three = queried_index + queried_index * 2U;
                    const u32 times_twenty_four = times_three << 3U;
                    const u32 times_twenty_three =
                        times_twenty_four - queried_index;
                    const u32 times_sixty_nine =
                        times_twenty_three + times_twenty_three * 2U;
                    const u32 times_three_hundred_forty_five =
                        times_sixty_nine + times_sixty_nine * 4U;
                    if (!decay_actor_gates(
                            group_b_token(queried_index),
                            0x0045AEDFU,
                            0x0045AEE4U,
                            times_three_hundred_forty_five,
                            packed_counter_next,
                            subtract_flags(times_twenty_four, queried_index)
                        )) {
                        return result;
                    }
                    auto mode_request = action_mode_request;
                    mode_request.actor_token = actor_token;
                    mode_request.mode = 0U;
                    mode_request.entry_eax =
                        result.actor_gate_decay.last.return_eax;
                    mode_request.entry_edx =
                        result.actor_gate_decay.last.return_edx;
                    mode_request.entry_return_address = 0x0045AEECU;
                    mode_request.entry_flags =
                        result.actor_gate_decay.last.flags;
                    result.actor_action_mode =
                        set_legacy_battle_actor_action_mode(
                            resolve_legacy_battle_actor_action_mode(
                                {.action = &action, .startup = startup},
                                actor_token
                            ),
                            mode_request
                        );
                    result.actor_action_modes[0U] = result.actor_action_mode;
                    ++result.actor_action_mode_calls;
                    if (result.actor_action_mode.status !=
                        LegacyBattleActorActionModeStatus::completed) {
                        result.status = LegacyBattleActionDispatchStatus::
                            actor_action_mode_typed_stop;
                        result.return_value =
                            result.actor_action_mode.return_eax;
                        return result;
                    }
                    result.actor_availability_block =
                        set_legacy_battle_actor_availability_block(
                            &final_actor
                                 .group_a_availability_blocks[group_a_index],
                            {
                                .value = 0U,
                                .actor_token = actor_token,
                                .entry_eax =
                                    result.actor_action_mode.return_eax,
                                .entry_edx =
                                    result.actor_action_mode.return_edx,
                            }
                        );
                    ++result.actor_availability_block_calls;
                    if (result.actor_availability_block.status !=
                        LegacyBattleActorAvailabilityBlockStatus::completed) {
                        result.status = LegacyBattleActionDispatchStatus::
                            actor_availability_block_typed_stop;
                        result.return_value =
                            result.actor_availability_block.return_eax;
                        return result;
                    }
                    if (!reset_actor(
                            actor_token,
                            0x0045AEF6U,
                            0x0045AEFBU,
                            result.actor_availability_block.return_eax,
                            result.actor_availability_block.return_edx
                        )) {
                        return result;
                    }
                    action_target_entry_edx =
                        result.actor_runtime_reset.last.return_edx;
                    final_actor.actor_order.fill(0U);
                    state.published_target_token = 0U;
                    final_actor.secondary_actor_code = 0U;
                    final_actor.queued_actor_code = 0U;
                    final_actor.active_actor_code = 0xFFFFFFFFU;
                    state.selection_workspace.fill(0U);
                }
            }
        }
        ++group_a_index;
        ++result.group_a_iterations;
    }
    result.return_value = group_a_index;
    return result;
}

}  // namespace openswd3::battle
