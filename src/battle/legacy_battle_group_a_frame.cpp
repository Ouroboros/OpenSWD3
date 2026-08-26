#include "openswd3/battle/legacy_battle_group_a_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryEffect = 0x004786D0U;
constexpr u32 kCallPublishEffectMode = 0x00478B60U;
constexpr u32 kCallPrepareAi = 0x0047DAD0U;
constexpr u32 kCallQueryGlobalGate = 0x0046E520U;
constexpr u32 kCallAdvanceAi = 0x0046EE60U;
constexpr u32 kCallQueryTerminal = 0x0047CE80U;
constexpr u32 kCallSetSelectionMode = 0x00478330U;
constexpr u32 kCallRandom = 0x00439070U;
constexpr u32 kCallQueryActorReady = 0x00480220U;
constexpr u32 kCallQueryPrimaryAi = 0x0047D880U;
constexpr u32 kCallQuerySecondaryAi = 0x0047D8D0U;
constexpr u32 kCallResetActor = 0x00478850U;
constexpr u32 kCallPlaySample = 0x00485610U;
constexpr u32 kCallQueryQueueMode = 0x00483820U;
constexpr u32 kCallPublishQueuedActor = 0x00464CC0U;
constexpr u32 kCallQueryQueueCompletion = 0x0047F920U;
constexpr u32 kCallQueryActorIdle = 0x004786A0U;
constexpr u32 kCallQueryActorAvailable = 0x0047C670U;
constexpr u32 kCallStartActorFrame = 0x0045EE70U;
constexpr u32 kCallQueryOneBasedTarget = 0x00478690U;
constexpr u32 kCallClearControl = 0x0047C660U;
constexpr u32 kCallClearPresentation = 0x0047CC50U;
constexpr u32 kCallSetDelay = 0x00478710U;
constexpr u32 kCallPublishSelection = 0x00478A70U;
constexpr u32 kCallFinalizeActor = 0x0046FFF0U;
constexpr u32 kCallSelectOpponent = 0x00478AA0U;
constexpr u32 kCallQueryOtherActor = 0x0047CEA0U;
constexpr u32 kCallPrepareSelection = 0x00478B30U;
constexpr u32 kCallSelectionComplete = 0x00478B40U;
constexpr u32 kCallFinalizeModeFour = 0x004708C0U;
constexpr u32 kCallQueryActionTarget = 0x004786E0U;
constexpr u32 kCallPrepareAction = 0x0047C690U;
constexpr u32 kCallClearActorAction = 0x00478B20U;
constexpr u32 kCallResetTarget = 0x00478AE0U;
constexpr u32 kCallPublishAllActors = 0x0047E950U;
constexpr u32 kCallClearNonterminal = 0x00483FF0U;
constexpr u32 kCallQueryTargetBusy = 0x00478690U;
constexpr u32 kCallPrepareTarget = 0x00478AC0U;
constexpr u32 kCallDisplayText = 0x004698E0U;
constexpr u32 kCallBeginActorAction = 0x00470820U;
constexpr u32 kCallAdvanceTurnGate = 0x00471540U;
constexpr u32 kCallResolveTarget = 0x00480AD0U;
constexpr u32 kCallCommitTurn = 0x004714B0U;
constexpr u32 kCallPublishTurnResult = 0x00483FD0U;

constexpr u32 kGroupBOneBeforeToken =
    kLegacyBattleActionGroupBBaseToken - kLegacyBattleActionGroupBStride;
constexpr u32 kActorSceneBaseToken = 0x004FE5D4U;
constexpr u32 kMessagePrimaryToken = 0x004A7808U;
constexpr u32 kMessageFinalToken = 0x004A77FCU;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u8 low_byte(const u32 value) noexcept {
    return static_cast<u8>(value);
}

[[nodiscard]] constexpr u8 byte_two(const u32 value) noexcept {
    return static_cast<u8>(value >> 16U);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

constexpr void replace_low_word(u32& target, const u16 value) noexcept {
    target = (target & 0xFFFF0000U) | static_cast<u32>(value);
}

constexpr void replace_high_word(u32& target, const u16 value) noexcept {
    target = (target & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

constexpr void replace_low_byte(u32& target, const u8 value) noexcept {
    target = (target & 0xFFFFFF00U) | static_cast<u32>(value);
}

constexpr void
replace_high_byte_of_low_word(u32& target, const u8 value) noexcept {
    target = (target & 0xFFFF00FFU) | (static_cast<u32>(value) << 8U);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] constexpr u32
one_based_group_a_token(const u32 one_based) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        (one_based - 1U) * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32
one_based_group_b_token(const u32 one_based) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        (one_based - 1U) * kLegacyBattleActionGroupBStride;
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

[[nodiscard]] bool validate_one_based_group_a(
    LegacyBattleActionDispatchResult& result, const u32 one_based
) noexcept {
    return one_based >= 1U && one_based <= 10U
        ? true
        : (result.status =
               LegacyBattleActionDispatchStatus::group_a_index_typed_stop,
           false);
}

[[nodiscard]] bool validate_one_based_group_b(
    LegacyBattleActionDispatchResult& result, const u32 one_based
) noexcept {
    return one_based >= 1U && one_based <= 8U
        ? true
        : (result.status =
               LegacyBattleActionDispatchStatus::group_b_index_typed_stop,
           false);
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

void reset_selection_gates(LegacyBattleGroupAFrameState& state) noexcept {
    state.final_actor_step.selection_gate = 0U;
    state.selection_aux_gate = 0U;
    state.action_pending_secondary = 0U;
    state.action.action_pending_aux = 0U;
    state.final_actor_step.active_actor_code = 0U;
}

void merge_nested_result(
    LegacyBattleActionDispatchResult& outer,
    const LegacyBattleActionDispatchResult& nested
) noexcept {
    outer.port_calls += nested.port_calls;
    outer.framebuffer_clear_calls += nested.framebuffer_clear_calls;
    outer.group_a_iterations += nested.group_a_iterations;
    outer.group_b_iterations += nested.group_b_iterations;
    outer.terminal_resets += nested.terminal_resets;
    outer.status_indicator_calls += nested.status_indicator_calls;
    outer.scale_scan_calls += nested.scale_scan_calls;
    outer.action_record_clear_calls += nested.action_record_clear_calls;
    if (nested.status != LegacyBattleActionDispatchStatus::completed) {
        outer.status = nested.status;
    }
}

[[nodiscard]] bool count_group_b_terminal(
    LegacyBattleGroupAFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    i32& terminal_count
) {
    terminal_count = 0;
    for (i32 index = 0; index < state.action.group_b_count; ++index) {
        const u32 uindex = to_bits(index);
        if (!validate_group_b(result, uindex)) {
            return false;
        }
        if (invoke(port, result, kCallQueryTerminal, {group_b_token(uindex)})
                .eax == 1U) {
            ++terminal_count;
        }
        ++result.group_b_iterations;
    }
    return true;
}

[[nodiscard]] bool finalize_frame(
    LegacyBattleGroupAFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_index
) {
    const auto nested = advance_legacy_battle_final_actor_step(
        state.final_actor_step, state.action, port, actor_index, 1U
    );
    merge_nested_result(result, nested);
    if (nested.status != LegacyBattleActionDispatchStatus::completed) {
        result.return_value = nested.return_value;
        return false;
    }
    if (nested.return_value == 1U) {
        state.final_action_gate = 0U;
        state.final_selected_word = 0xFFFFU;
    }
    result.return_value = 1U;
    return true;
}

}  // namespace

LegacyBattleActionDispatchResult advance_legacy_battle_group_a_frame(
    LegacyBattleGroupAFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const u32 group_a_index
) {
    LegacyBattleActionDispatchResult result;
    if (!validate_group_a(result, group_a_index)) {
        return result;
    }
    const u32 actor_token = group_a_token(group_a_index);
    auto& actor = state.actors[group_a_index];
    const bool effect_mode =
        (low_word(invoke(port, result, kCallQueryEffect, {actor_token}).eax) !=
             0U &&
         (state.action.frame_effect.primary_suppression == 1U ||
          state.action.frame_effect.split_suppression == 1U)) ||
        state.global_effect_override == 1U;
    static_cast<void>(invoke(
        port,
        result,
        kCallPublishEffectMode,
        {actor_token, effect_mode ? 1U : 0U}
    ));

    if (state.ai_coordination_enabled == 1U &&
        state.action.action_pending_aux == 0U &&
        state.action_pending_secondary == 0U) {
        static_cast<void>(invoke(port, result, kCallPrepareAi, {actor_token}));
        if (invoke(
                port, result, kCallQueryGlobalGate, {state.actor_gate_argument}
            )
                .eax == 1U) {
            static_cast<void>(
                invoke(port, result, kCallAdvanceAi, {actor_token})
            );
            if (state.actor_ai_primary[group_a_index] == 1U ||
                state.actor_ai_secondary[group_a_index] == 1U) {
                i32 terminal_count = 0;
                if (!count_group_b_terminal(
                        state, port, result, terminal_count
                    )) {
                    return result;
                }
                if (terminal_count < state.action.group_b_count) {
                    static_cast<void>(
                        invoke(port, result, kCallSetSelectionMode, {1U})
                    );
                    while (true) {
                        state.selected_opponent_one_based =
                            invoke(
                                port,
                                result,
                                kCallRandom,
                                {to_bits(state.action.group_b_count)}
                            )
                                .eax +
                            1U;
                        if (!validate_one_based_group_b(
                                result, state.selected_opponent_one_based
                            )) {
                            return result;
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {one_based_group_b_token(
                                    state.selected_opponent_one_based
                                )}
                            )
                                .eax != 1U) {
                            break;
                        }
                    }
                    state.final_actor_step.selection_gate = 1U;
                    const auto ready = invoke(
                        port,
                        result,
                        kCallQueryActorReady,
                        {kActorSceneBaseToken + group_a_index * 4U}
                    );
                    if (ready.eax == 1U) {
                        if (invoke(
                                port, result, kCallQueryPrimaryAi, {actor_token}
                            )
                                .eax == 1U) {
                            actor.special_ready = 1U;
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQuerySecondaryAi,
                                {actor_token}
                            )
                                .eax == 1U) {
                            actor.action_complete = 1U;
                        }
                    }
                } else {
                    static_cast<void>(
                        invoke(port, result, kCallResetActor, {actor_token})
                    );
                }
            } else {
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallPlaySample,
                    {0x2CU, state.sample_handle_value}
                ));
                std::size_t slot = 0U;
                while (slot < state.final_actor_step.actor_order.size() &&
                       state.final_actor_step.actor_order[slot] != 0U) {
                    ++slot;
                }
                if (slot < state.final_actor_step.actor_order.size()) {
                    if (invoke(port, result, kCallQueryQueueMode, {actor_token})
                            .eax == 1U) {
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPublishQueuedActor,
                            {group_a_index + 8U}
                        ));
                    } else {
                        state.final_actor_step.actor_order[slot] =
                            group_a_index + 8U;
                    }
                }
            }
        }
    }

    for (std::size_t index = 0U;
         state.final_actor_step.queued_actor_code == 0U &&
         state.action.current_actor_index == 0xFFFFU &&
         index < state.final_actor_step.actor_order.size() &&
         state.final_actor_step.actor_order[index] != 0U;
         ++index) {
        const u32 code = state.final_actor_step.actor_order[index];
        const u32 queued_index = code - 8U;
        if (!validate_group_a(result, queued_index)) {
            return result;
        }
        if (invoke(
                port,
                result,
                kCallQueryQueueCompletion,
                {group_a_token(queued_index)}
            )
                .eax != 1U) {
            state.final_actor_step.queued_actor_code = code;
            for (std::size_t shift = index;
                 shift + 1U < state.final_actor_step.actor_order.size();
                 ++shift) {
                state.final_actor_step.actor_order[shift] =
                    state.final_actor_step.actor_order[shift + 1U];
            }
            state.final_actor_step.actor_order.back() = 0U;
            break;
        }
    }

    if (actor.frame_started == 0U &&
        invoke(port, result, kCallQueryQueueCompletion, {actor_token}).eax ==
            0U &&
        invoke(port, result, kCallQueryActorIdle, {actor_token}).eax == 1U &&
        invoke(port, result, kCallQueryActorAvailable, {actor_token}).eax ==
            1U &&
        state.action_block_gate == 0U && state.action_aux_gate == 0U) {
        if (state.queued_selection_word != 0xFFFFU &&
            state.action.group_b_count -
                    static_cast<i32>(
                        low_byte(state.action.opponent_processed_counter)
                    ) <=
                1) {
            static_cast<void>(
                invoke(port, result, kCallResetActor, {actor_token})
            );
        }
        if (state.action.message_state == 0x63U &&
            state.actor_start_guard_word == 0U) {
            static_cast<void>(
                invoke(port, result, kCallResetActor, {actor_token})
            );
        }
        actor.frame_started = 1U;
        state.final_actor_step.active_actor_code = group_a_index + 8U;
        static_cast<void>(invoke(
            port,
            result,
            kCallStartActorFrame,
            {1U, group_a_index + 8U, 0xFFFFFFFFU}
        ));
    }

    if (state.actor_enabled[group_a_index] == 1U) {
        if (actor.action_complete != 1U) {
            if (actor.mode_gate != 0U) {
                if (!validate_one_based_group_a(
                        result, state.selected_actor_one_based
                    )) {
                    return result;
                }
                const u32 selected =
                    one_based_group_a_token(state.selected_actor_one_based);
                if (invoke(port, result, kCallQueryOneBasedTarget, {selected})
                            .eax == 0U &&
                    state.action.active_effect_target !=
                        state.selected_actor_one_based + 7U &&
                    invoke(port, result, kCallQueryActorIdle, {actor_token})
                            .eax == 0U) {
                    static_cast<void>(
                        invoke(port, result, kCallClearControl, {0U})
                    );
                    static_cast<void>(
                        invoke(port, result, kCallClearPresentation, {0U})
                    );
                    if (invoke(
                            port,
                            result,
                            kCallSetDelay,
                            {actor_token, actor.delay_mode}
                        )
                            .eax == 1U) {
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPublishSelection,
                            {state.selected_actor_one_based - 1U}
                        ));
                        static_cast<void>(invoke(
                            port, result, kCallFinalizeActor, {actor_token}
                        ));
                        static_cast<void>(
                            invoke(port, result, kCallSetSelectionMode, {0U})
                        );
                        reset_selection_gates(state);
                        state.ui_gate_a = 1U;
                        state.ui_gate_b = 1U;
                    }
                }
            } else if (
                invoke(port, result, kCallQueryActorIdle, {actor_token}).eax ==
                0U
            ) {
                static_cast<void>(
                    invoke(port, result, kCallClearPresentation, {0U})
                );
                if (state.actor_ai_primary[group_a_index] != 0U ||
                    state.actor_ai_secondary[group_a_index] != 0U) {
                    if (!validate_one_based_group_b(
                            result, state.selected_opponent_one_based
                        )) {
                        return result;
                    }
                    static_cast<void>(
                        invoke(port, result, kCallClearControl, {0U})
                    );
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSelectOpponent,
                        {one_based_group_b_token(
                            state.selected_opponent_one_based
                        )}
                    ));
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishSelection,
                        {state.selected_opponent_one_based - 1U}
                    ));
                    static_cast<void>(
                        invoke(port, result, kCallSetSelectionMode, {0U})
                    );
                    reset_selection_gates(state);
                    state.selected_opponent_one_based = 1U;
                } else if (
                    invoke(
                        port,
                        result,
                        kCallSetDelay,
                        {actor_token, actor.delay_mode}
                    )
                        .eax == 1U
                ) {
                    if (!validate_one_based_group_b(
                            result, state.selected_actor_one_based
                        )) {
                        return result;
                    }
                    static_cast<void>(
                        invoke(port, result, kCallClearControl, {0U})
                    );
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSelectOpponent,
                        {one_based_group_b_token(
                            state.selected_actor_one_based
                        )}
                    ));
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishSelection,
                        {state.selected_actor_one_based - 1U}
                    ));
                    static_cast<void>(
                        invoke(port, result, kCallSetSelectionMode, {0U})
                    );
                    static_cast<void>(
                        invoke(port, result, kCallFinalizeActor, {actor_token})
                    );
                    reset_selection_gates(state);
                    state.ui_gate_b = 1U;
                    state.ui_gate_c = 1U;
                }
            }
        } else {
            i32 selected_index = -1;
            if (state.selection_mode != 0U) {
                for (i32 index = 0; index < state.action.group_a_count;
                     ++index) {
                    const u32 uindex = to_bits(index);
                    if (!validate_group_a(result, uindex)) {
                        return result;
                    }
                    if (state.actor_ai_primary[uindex] != 1U &&
                        state.actor_ai_secondary[uindex] != 1U &&
                        invoke(
                            port,
                            result,
                            kCallQueryOtherActor,
                            {group_a_token(uindex)}
                        )
                                .eax != 1U &&
                        invoke(port, result, kCallQueryActorIdle, {actor_token})
                                .eax == 0U) {
                        static_cast<void>(
                            invoke(port, result, kCallClearControl, {0U})
                        );
                        ++actor.progress;
                    }
                    ++result.group_a_iterations;
                }
                const i32 threshold =
                    state.action.group_a_count -
                    static_cast<i32>(
                        low_byte(state.action.packed_actor_counter)
                    ) -
                    static_cast<i32>(high_word(state.defeated_actor_packed)) -
                    static_cast<i32>(state.excluded_actor_count);
                if (from_bits(actor.progress) >= threshold) {
                    selected_index = state.action.group_a_count - 1;
                    while (selected_index >= 0) {
                        if (!validate_group_a(
                                result, to_bits(selected_index)
                            )) {
                            return result;
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {group_a_token(to_bits(selected_index))}
                            )
                                .eax != 1U) {
                            break;
                        }
                        --selected_index;
                    }
                }
            } else {
                i32 terminal_like = 0;
                for (i32 index = 0; index < state.action.group_b_count;
                     ++index) {
                    const u32 uindex = to_bits(index);
                    if (!validate_group_b(result, uindex)) {
                        return result;
                    }
                    const u32 target = group_b_token(uindex);
                    if (invoke(port, result, kCallQueryTerminal, {target})
                            .eax == 1U) {
                        ++terminal_like;
                    } else if (
                        state.action.group_a_to_actor[uindex] == 0xFFFFFFFFU
                    ) {
                        static_cast<void>(
                            invoke(port, result, kCallClearControl, {0U})
                        );
                        static_cast<void>(
                            invoke(port, result, kCallSelectOpponent, {target})
                        );
                        ++actor.progress;
                    } else {
                        ++terminal_like;
                    }
                    ++result.group_b_iterations;
                }
                if (from_bits(actor.progress) >=
                    state.action.group_b_count - terminal_like) {
                    selected_index = 0;
                    while (selected_index < state.action.group_b_count) {
                        if (!validate_group_b(
                                result, to_bits(selected_index)
                            )) {
                            return result;
                        }
                        if (invoke(
                                port,
                                result,
                                kCallQueryTerminal,
                                {group_b_token(to_bits(selected_index))}
                            )
                                .eax != 1U) {
                            break;
                        }
                        ++selected_index;
                    }
                    if (selected_index >= state.action.group_b_count) {
                        selected_index = -1;
                    }
                }
            }
            if (selected_index >= 0) {
                state.target_ready_gate = 1U;
                static_cast<void>(
                    invoke(port, result, kCallPrepareSelection, {actor_token})
                );
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallPublishSelection,
                    {to_bits(selected_index)}
                ));
            }

            if (invoke(port, result, kCallSelectionComplete, {actor_token})
                    .eax == 1U) {
                if (state.actor_ai_primary[group_a_index] != 0U ||
                    state.actor_ai_secondary[group_a_index] != 0U) {
                    state.final_actor_step.selection_gate = 0U;
                    static_cast<void>(
                        invoke(port, result, kCallSetSelectionMode, {0U})
                    );
                    state.selection_aux_gate = 0U;
                    state.target_cleanup_gate = 0U;
                    state.target_ready_gate = 0U;
                } else if (
                    invoke(
                        port,
                        result,
                        kCallSetDelay,
                        {actor_token, actor.delay_mode}
                    )
                        .eax == 1U
                ) {
                    static_cast<void>(
                        invoke(port, result, kCallClearPresentation, {0U})
                    );
                    state.final_actor_step.selection_gate = 0U;
                    static_cast<void>(
                        invoke(port, result, kCallSetSelectionMode, {0U})
                    );
                    if (actor.delay_mode == 4U) {
                        static_cast<void>(invoke(
                            port, result, kCallFinalizeModeFour, {actor_token}
                        ));
                    }
                    state.selection_aux_gate = 0U;
                    state.target_cleanup_gate = 0U;
                    state.target_ready_gate = 0U;
                    state.ui_gate_a = 1U;
                    state.ui_gate_b = 1U;
                    state.selected_actor_one_based = 1U;
                    state.ui_gate_c = 1U;
                    reset_selection_gates(state);
                }
            }
        }
    }

    if (state.action.active_effect_target == group_a_index + 8U) {
        if (state.final_actor_step.action_execution_active != 0U) {
            state.action_aux_gate = 1U;
            state.action.action_pending_aux = 1U;
            state.action.current_actor_index =
                static_cast<u16>(state.action.active_effect_target);
            static_cast<void>(
                invoke(port, result, kCallPrepareAction, {actor_token})
            );
            const u16 target_index = low_word(
                invoke(port, result, kCallQueryActionTarget, {actor_token}).eax
            );
            const auto nested = dispatch_legacy_battle_action(
                state.action, port, context, group_a_index, target_index
            );
            merge_nested_result(result, nested);
            if (result.status != LegacyBattleActionDispatchStatus::completed) {
                return result;
            }
            if (nested.return_value == 1U) {
                const u16 completed_target = low_word(
                    invoke(port, result, kCallQueryActionTarget, {actor_token})
                        .eax
                );
                static_cast<void>(
                    invoke(port, result, kCallClearActorAction, {actor_token})
                );
                state.action_aux_gate = 0U;
                state.action_runtime_word = 0U;
                replace_high_byte_of_low_word(
                    state.action.opponent_processed_counter, 0U
                );
                state.action_stage_word = 0U;
                state.action.active_effect_gate = 0U;
                state.action.action_pending_aux = 0U;
                state.action_pending_secondary = 0U;
                state.selection_mode = 0U;
                state.final_actor_step.action_execution_active = 0U;

                if (completed_target != 0xFFFFU) {
                    if (invoke(
                            port, result, kCallSelectionComplete, {actor_token}
                        )
                            .eax == 1U) {
                        for (i32 index = 0; index < state.action.group_b_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_b(result, uindex)) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallResetTarget,
                                {group_b_token(uindex)}
                            ));
                            ++result.group_b_iterations;
                        }
                    } else {
                        const u32 reset_index = completed_target;
                        if (state.action_side != 0U) {
                            if (!validate_group_a(result, reset_index)) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallResetTarget,
                                {group_a_token(reset_index)}
                            ));
                        } else {
                            if (!validate_group_b(result, reset_index)) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallResetTarget,
                                {group_b_token(reset_index)}
                            ));
                        }
                    }
                    const auto post_action = advance_legacy_battle_post_action(
                        state.post_action,
                        state.final_actor_step,
                        state.action,
                        port,
                        group_a_index,
                        completed_target
                    );
                    merge_nested_result(result, post_action);
                    if (post_action.status !=
                        LegacyBattleActionDispatchStatus::completed) {
                        result.return_value = post_action.return_value;
                        return result;
                    }
                    if (!validate_group_b(result, completed_target)) {
                        return result;
                    }
                    const u32 completed_b = group_b_token(completed_target);
                    if (invoke(port, result, kCallQueryTerminal, {completed_b})
                            .eax == 1U) {
                        const u16 actor_target =
                            low_word(invoke(
                                         port,
                                         result,
                                         kCallQueryActionTarget,
                                         {completed_b}
                            )
                                         .eax);
                        if (actor_target != 0xFFFFU) {
                            if (!validate_group_a(result, actor_target)) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallResetTarget,
                                {group_a_token(actor_target)}
                            ));
                        }
                        const u32 remaining =
                            to_bits(state.action.group_b_count) +
                            byte_two(state.action.opponent_processed_counter) -
                            low_byte(state.action.opponent_processed_counter);
                        if (remaining <= 1U) {
                            for (i32 index = 0;
                                 index < state.action.group_a_count;
                                 ++index) {
                                const u32 uindex = to_bits(index);
                                if (!validate_group_a(result, uindex)) {
                                    return result;
                                }
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPublishAllActors,
                                    {group_a_token(uindex), 1U}
                                ));
                                ++result.group_a_iterations;
                            }
                        }
                    }
                    if ((state.battle_byte_flags & 0x80U) != 0U) {
                        for (i32 index = 0; index < state.action.group_b_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_b(result, uindex)) {
                                return result;
                            }
                            const u32 target = group_b_token(uindex);
                            if (invoke(
                                    port, result, kCallQueryTerminal, {target}
                                )
                                    .eax == 0U) {
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallClearNonterminal,
                                    {target, 0U}
                                ));
                            }
                            ++result.group_b_iterations;
                        }
                        replace_low_byte(
                            state.battle_byte_flags,
                            static_cast<u8>(state.battle_byte_flags & 0x7FU)
                        );
                    }
                }

                static_cast<void>(
                    invoke(port, result, kCallResetActor, {actor_token})
                );
                state.action.active_effect_target = 0U;
                state.action_side = 0U;
                state.active_effect_tail.fill(0U);
                actor.post_action_value = 0U;
                actor.scene_identity = 0U;
                state.shared_gate_4ff578 = 1U;
                actor.frame_started = 0U;
                state.shared_gate_4ff57c = 1U;
                state.shared_gate_4ff580 = 1U;
                state.shared_value_52544c = 0U;
                state.shared_gate_4ff584 = 1U;
                state.shared_value_525450 = 0U;
                state.shared_value_525454 = 0U;
                state.action_stage_word = 0U;
                state.action_stage_word_b = 0U;
                state.action.active_effect_target = 0xFFFFFFFFU;
                state.cleanup_word = 0U;
                replace_low_word(state.action.action_runtime_flags, 0U);
                state.action.post_battle_counter = 0U;
                state.shared_value_525458 = 0U;
                if (state.action.frame_effect.primary_suppression == 1U) {
                    state.action.frame_effect.fade_active = 1U;
                }
                if ((state.global_phase_countdown & 0x7FFFU) != 0U) {
                    replace_low_word(
                        state.global_phase_countdown,
                        static_cast<u16>(state.global_phase_countdown - 1U)
                    );
                }
            }
        } else if (state.action_block_gate == 0U) {
            const u16 queried_target = low_word(
                invoke(port, result, kCallQueryActionTarget, {actor_token}).eax
            );
            if (!validate_group_b(result, queried_target)) {
                return result;
            }
            u32 target_token = group_b_token(queried_target);
            if (invoke(port, result, kCallQueryTargetBusy, {target_token})
                    .eax == 0U) {
                state.final_actor_step.action_execution_active = 1U;
                state.action.current_actor_index =
                    static_cast<u16>(state.action.active_effect_target);
                if (invoke(port, result, kCallSelectionComplete, {actor_token})
                        .eax != 0U) {
                    if (state.action_side == 0U) {
                        bool any = false;
                        for (i32 index = 0; index < state.action.group_b_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_b(result, uindex)) {
                                return result;
                            }
                            const u32 candidate = group_b_token(uindex);
                            const auto first = invoke(
                                port, result, kCallQueryTerminal, {candidate}
                            );
                            if (first.eax != 1U) {
                                if (invoke(
                                        port,
                                        result,
                                        kCallQueryTerminal,
                                        {candidate}
                                    )
                                        .eax == 0U) {
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPrepareTarget,
                                        {candidate}
                                    ));
                                }
                                any = true;
                            }
                            ++result.group_b_iterations;
                        }
                        if (!any) {
                            state.final_actor_step.action_execution_active = 0U;
                        }
                    }
                } else if (
                    state.action_side == 0U &&
                    state.action_target_guard_high_word == 0U
                ) {
                    if (invoke(port, result, kCallQueryTerminal, {target_token})
                                .eax == 1U &&
                        state.action_runtime_word == 0U) {
                        i32 selected = 0;
                        while (selected < state.action.group_b_count) {
                            const u32 uindex = to_bits(selected);
                            if (!validate_group_b(result, uindex)) {
                                return result;
                            }
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_b_token(uindex)}
                                )
                                    .eax != 1U) {
                                break;
                            }
                            ++selected;
                        }
                        if (selected >= state.action.group_b_count) {
                            state.final_actor_step.action_execution_active = 0U;
                        } else {
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallClearActorAction,
                                {actor_token}
                            ));
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallResetTarget,
                                {kGroupBOneBeforeToken}
                            ));
                            if (static_cast<u16>(selected) == 0xFFFFU) {
                                state.final_actor_step.action_execution_active =
                                    0U;
                            } else {
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPublishSelection,
                                    {to_bits(selected)}
                                ));
                                target_token = group_b_token(to_bits(selected));
                            }
                        }
                    }
                    if (state.final_actor_step.action_execution_active == 1U) {
                        static_cast<void>(invoke(
                            port, result, kCallPrepareTarget, {target_token}
                        ));
                    }
                }
                if (state.final_actor_step.action_execution_active == 1U) {
                    if (state.actor_text_present[group_a_index] != 0U) {
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallDisplayText,
                            {0x118U,
                             0U,
                             0x28U,
                             state.actor_text_token +
                                 group_a_index *
                                     kLegacyBattleActionGroupAStride,
                             0x40U}
                        ));
                        state.action_text_runtime.fill(0U);
                    }
                    static_cast<void>(invoke(
                        port, result, kCallBeginActorAction, {actor_token}
                    ));
                }
            }
        }
    }

    if (state.action_block_gate == 0U &&
        invoke(port, result, kCallQueryTerminal, {actor_token}).eax == 0U &&
        state.actor_ai_secondary[group_a_index] == 0U &&
        state.actor_ai_primary[group_a_index] == 0U) {
        u16 turn = state.turn_resolution_bits;
        if ((turn & 0x4000U) != 0U) {
            state.action.action_pending_aux = 1U;
            state.action_pending_secondary = 1U;
            if (invoke(port, result, kCallAdvanceTurnGate, {actor_token, 0U})
                    .eax == 1U) {
                replace_low_word(
                    state.action.input_mode,
                    static_cast<u16>(state.action.input_mode + 1U)
                );
                const u32 threshold = to_bits(state.action.group_a_count) -
                    low_byte(state.action.packed_actor_counter) -
                    high_word(state.defeated_actor_packed) -
                    state.excluded_actor_count;
                if (low_word(state.action.input_mode) >= threshold) {
                    replace_high_word(state.action.phase_counter, 0U);
                    for (i32 index = 0; index < state.action.group_b_count;
                         ++index) {
                        const u32 uindex = to_bits(index);
                        if (!validate_group_b(result, uindex)) {
                            return result;
                        }
                        const u32 target = group_b_token(uindex);
                        if (invoke(port, result, kCallQueryTerminal, {target})
                                .eax != 1U) {
                            const auto resolved = invoke(
                                port, result, kCallResolveTarget, {target}
                            );
                            if (resolved.eax == 0U) {
                                result.status =
                                    LegacyBattleActionDispatchStatus::
                                        target_object_typed_stop;
                                return result;
                            }
                            u16 maximum = high_word(state.action.phase_counter);
                            const u16 candidate =
                                static_cast<u16>(resolved.object_flags);
                            if (candidate > maximum) {
                                maximum = candidate;
                                replace_high_word(
                                    state.action.phase_counter, maximum
                                );
                            }
                            if (state.action.group_a_to_actor[group_a_index] !=
                                0xFFFFFFFFU) {
                                replace_high_word(
                                    state.action.phase_counter, 0xC8U
                                );
                            }
                        }
                        ++result.group_b_iterations;
                    }
                    if (high_word(state.action.phase_counter) == 0U) {
                        state.turn_resolution_bits = 0U;
                        replace_low_word(state.action.input_mode, 1U);
                        replace_high_word(state.action.phase_counter, 0U);
                        state.action.action_pending_aux = 0U;
                        state.action_pending_secondary = 0U;
                    }
                    const u32 stale_turn_argument =
                        (to_bits(state.action.group_b_count) & 0xFFFF0000U) |
                        high_word(state.action.phase_counter);
                    if (invoke(
                            port,
                            result,
                            kCallCommitTurn,
                            {kLegacyBattleActionGroupABaseToken,
                             stale_turn_argument}
                        )
                            .eax == 1U) {
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPublishTurnResult,
                            {actor_token, 1U}
                        ));
                        state.turn_resolution_bits = 0x8000U;
                        state.selection_aux_gate = 0U;
                        state.final_actor_step.queued_actor_code = 0U;
                        replace_high_word(state.action.phase_counter, 0U);
                        u32 remaining = to_bits(state.action.group_a_count) -
                            low_byte(state.action.packed_actor_counter);
                        replace_low_word(
                            remaining,
                            static_cast<u16>(
                                low_word(remaining) -
                                high_word(state.defeated_actor_packed)
                            )
                        );
                        remaining -= state.excluded_actor_count;
                        replace_low_word(
                            state.action.input_mode, low_word(remaining)
                        );
                    } else {
                        for (i32 index = 0; index < state.action.group_a_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_a(result, uindex)) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallPublishTurnResult,
                                {group_a_token(uindex), 0U}
                            ));
                            ++result.group_a_iterations;
                        }
                        if (state.message_suppressed == 0U) {
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallDisplayText,
                                {0x118U,
                                 0xAU,
                                 0x1EU,
                                 kMessagePrimaryToken,
                                 0x80000002U}
                            ));
                        }
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPlaySample,
                            {0x8CU, state.sample_handle_value}
                        ));
                        state.turn_resolution_bits = 0U;
                        replace_low_word(state.action.input_mode, 1U);
                        replace_high_word(state.action.phase_counter, 0U);
                        state.action.action_pending_aux = 0U;
                        state.action_pending_secondary = 0U;
                    }
                }
            }
            turn = state.turn_resolution_bits;
        }
        if (std::bit_cast<i16>(turn) < 0) {
            state.action.action_pending_aux = 1U;
            state.action_pending_secondary = 1U;
            state.selection_aux_gate = 0U;
            state.final_actor_step.queued_actor_code = 0U;
            const u16 actor_bit = static_cast<u16>(1U << group_a_index);
            if ((turn & 0x7FFFU & actor_bit) == 0U &&
                invoke(port, result, kCallAdvanceTurnGate, {actor_token, 1U})
                        .eax == 1U) {
                state.action.overlay_gate = 1U;
                state.turn_resolution_bits = static_cast<u16>(turn | actor_bit);
                replace_low_byte(
                    state.action.packed_actor_counter,
                    static_cast<u8>(state.action.packed_actor_counter + 1U)
                );
                const u32 defeated =
                    low_byte(state.action.packed_actor_counter);
                const u32 threshold = to_bits(state.action.group_a_count) -
                    high_word(state.defeated_actor_packed) -
                    state.excluded_actor_count;
                if (defeated >= threshold) {
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallDisplayText,
                        {0x118U, 0xAU, 0x1EU, kMessageFinalToken, 0x80000002U}
                    ));
                    state.final_actor_step.actor_order.fill(0U);
                    state.action.message_state = 0x68U;
                    state.turn_resolution_bits = 0U;
                    state.action.active_effect_target = 0xFFFFFFFFU;
                    state.action.opponent_workspace.fill(0U);
                }
            }
        }
    }

    static_cast<void>(finalize_frame(state, port, result, group_a_index));
    return result;
}

}  // namespace openswd3::battle
