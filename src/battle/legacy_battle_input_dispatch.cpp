#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

#include <bit>
#include <cstddef>

#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"
#include "openswd3/battle/legacy_battle_actor_action_commit.hpp"
#include "openswd3/battle/legacy_battle_actor_action_cycle.hpp"
#include "openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp"
#include "openswd3/battle/legacy_battle_menu_context_advance.hpp"
#include "openswd3/battle/legacy_battle_menu_context_retreat.hpp"
#include "openswd3/battle/legacy_battle_menu_input_finalize.hpp"
#include "openswd3/battle/legacy_battle_menu_page_advance.hpp"
#include "openswd3/battle/legacy_battle_menu_page_retreat.hpp"
#include "openswd3/battle/legacy_battle_menu_selection_advance.hpp"
#include "openswd3/battle/legacy_battle_menu_selection_retreat.hpp"
#include "openswd3/battle/legacy_battle_target_selection_entry.hpp"

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

inline constexpr u32 kKeyboardToken = 0x004B8748U;
inline constexpr u32 kWorkspaceActorOffset = 2U;

class InputTextMessageAdapter final : public LegacyBattleTextMessagePort {
public:
    explicit InputTextMessageAdapter(LegacyBattleInputDispatchPort& port)
        : port_(port) {}

    [[nodiscard]] LegacyBattleTextMessageCallReply invoke_text_message(
        const LegacyBattleTextMessageCallRequest& request
    ) override {
        const auto reply = port_.invoke_input_dispatch({
            .call = request.call == LegacyBattleTextMessageCall::allocate
                ? LegacyBattleInputDispatchCall::text_message_allocate
                : LegacyBattleInputDispatchCall::text_message_measure,
            .arguments = {request.argument},
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

private:
    LegacyBattleInputDispatchPort& port_;
};

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 actor_code) noexcept {
    const u32 index = actor_code - 8U;
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 input_permission_byte(
    const LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    const u32 word = index < 4U ? reset.value_524414 : reset.value_524418;
    return word >> ((index & 3U) * 8U) & 0xFFU;
}

}  // namespace

LegacyBattleInputDispatchResult coordinate_legacy_battle_input_dispatch(
    LegacyBattleInputDispatchBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleInputDispatchRequest& request
) {
    LegacyBattleInputDispatchResult result;
    auto& state = port.battle_input_dispatch_state();
    InputTextMessageAdapter text_message_port(port);
    u32 eax = bindings.render_abort_latch;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto early = [&]() {
        result.returned_early = true;
        return finish();
    };
    const auto call = [&](const LegacyBattleInputDispatchCall operation,
                          const std::array<u32, 5>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_input_dispatch({
            .call = operation,
            .arguments = arguments,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        return reply;
    };
    const auto raw_key = [&](const u32 dik) {
        ++result.raw_key_queries;
        eax = input_time_rng::read_raw_key(bindings.keyboard, dik);
        ecx = kKeyboardToken;
        return eax;
    };
    const auto record =
        [&](const std::size_t index) -> input_time_rng::LegacyInputRecord* {
        if (index >= bindings.input_records.size()) {
            result.status =
                LegacyBattleInputDispatchStatus::input_record_typed_stop;
            return nullptr;
        }
        ++result.input_record_reads;
        return &bindings.input_records[index];
    };
    const auto write_record_zero = [&](const u32 held_count) {
        auto* const target = record(0U);
        if (target == nullptr) {
            return false;
        }
        target->rapid_press_multiplicity = 1U;
        target->held_sample_count = held_count;
        result.input_record_writes += 2U;
        return true;
    };
    const auto repeat = [&](const u32 held_count, const i32 divisor) {
        if (held_count == 1U) {
            return true;
        }
        if (signed_bits(held_count) < 15) {
            return false;
        }
        const i32 signed_count = signed_bits(held_count);
        const i32 quotient = signed_count / divisor;
        const i32 remainder = signed_count % divisor;
        eax = std::bit_cast<u32>(quotient);
        edx = std::bit_cast<u32>(remainder);
        return remainder == 1;
    };
    const auto retreat_menu_selection = [&]() {
        const auto nested = retreat_legacy_battle_menu_selection(
            {
                .startup_reset = bindings.startup_reset,
                .startup_supplemental_count_word =
                    bindings.startup_supplemental_count_word,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_selection_retreat_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status !=
            LegacyBattleMenuSelectionRetreatStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                menu_selection_retreat_typed_stop;
            return false;
        }
        return true;
    };
    const auto advance_menu_selection = [&]() {
        const auto nested = advance_legacy_battle_menu_selection(
            {
                .startup_reset = bindings.startup_reset,
                .startup_supplemental_count_word =
                    bindings.startup_supplemental_count_word,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_selection_advance_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status !=
            LegacyBattleMenuSelectionAdvanceStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                menu_selection_advance_typed_stop;
            return false;
        }
        return true;
    };
    const auto retreat_menu_page = [&]() {
        const auto nested = retreat_legacy_battle_menu_page(
            {
                .startup_reset = bindings.startup_reset,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_page_retreat_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleMenuPageRetreatStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::menu_page_retreat_typed_stop;
            return false;
        }
        return true;
    };
    const auto advance_menu_page = [&]() {
        const auto nested = advance_legacy_battle_menu_page(
            {
                .startup_reset = bindings.startup_reset,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_page_advance_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleMenuPageAdvanceStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::menu_page_advance_typed_stop;
            return false;
        }
        return true;
    };
    const auto finalize_menu_input = [&]() {
        const auto nested = finalize_legacy_battle_menu_input(
            {
                .startup_reset = bindings.startup_reset,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_input_finalize_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleMenuInputFinalizeStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::menu_input_finalize_typed_stop;
            return false;
        }
        return true;
    };
    const auto refresh_action_mode = [&]() {
        const auto nested = refresh_legacy_battle_action_mode(
            {
                .startup_reset = bindings.startup_reset,
                .source_state = bindings.action_mode_source,
                .party_presence = bindings.startup_party_presence,
                .startup_mode_flags = bindings.startup_mode_flags,
                .final_actor = bindings.final_actor,
                .frame_input = bindings.frame_input_resolution,
                .input_dispatch = state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.action_mode_refresh_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleActionModeRefreshStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::action_mode_refresh_typed_stop;
            return false;
        }
        return true;
    };
    const auto enter_target_selection = [&]() {
        const auto nested = enter_legacy_battle_target_selection(
            {
                .startup_reset = bindings.startup_reset,
                .text_messages = bindings.text_messages,
                .action_mode_source = bindings.action_mode_source,
                .startup_party_presence = bindings.startup_party_presence,
                .startup_mode_flags = bindings.startup_mode_flags,
                .party = bindings.party,
                .startup_supplemental_count_word =
                    bindings.startup_supplemental_count_word,
                .startup_mirror_mode = bindings.startup_mirror_mode,
                .frame_input_resolution = bindings.frame_input_resolution,
                .final_actor = bindings.final_actor,
                .action = bindings.action,
                .metrics = bindings.metrics,
                .debug_hotkeys = bindings.debug_hotkeys,
                .input_dispatch = state,
                .input_records = bindings.input_records,
                .target_selection_runtime =
                    port.battle_target_selection_runtime_state(),
                .dialogs = bindings.dialogs,
                .one_shot_interaction_state =
                    bindings.one_shot_interaction_state,
                .target_ready_gate = bindings.target_ready_gate,
                .outcome_darkening_gate = bindings.outcome_darkening_gate,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.target_selection_entry_calls;
        result.action_mode_refresh_calls += nested.action_mode_refresh_calls;
        result.target_selection_refresh_calls +=
            nested.target_selection_refresh_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status !=
            LegacyBattleTargetSelectionEntryStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                target_selection_entry_typed_stop;
            return false;
        }
        return true;
    };
    const auto commit_actor_action = [&]() {
        const auto nested = commit_legacy_battle_actor_action(
            {
                .startup_reset = bindings.startup_reset,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.actor_code = eax,
             .entry_eax = eax,
             .entry_ecx = ecx,
             .entry_edx = edx}
        );
        ++result.actor_action_commit_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleActorActionCommitStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::actor_action_commit_typed_stop;
            return false;
        }
        return true;
    };
    const auto cycle_actor_action = [&]() {
        const auto nested = cycle_legacy_battle_actor_action(
            {
                .startup_reset = bindings.startup_reset,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.actor_action_cycle_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleActorActionCycleStatus::completed) {
            result.status =
                LegacyBattleInputDispatchStatus::actor_action_cycle_typed_stop;
            return false;
        }
        return true;
    };
    const auto reverse_cycle_actor_action = [&]() {
        const auto nested = reverse_cycle_legacy_battle_actor_action(
            {
                .startup_reset = bindings.startup_reset,
                .final_actor = bindings.final_actor,
                .metrics = bindings.metrics,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.actor_action_reverse_cycle_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status !=
            LegacyBattleActorActionReverseCycleStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                actor_action_reverse_cycle_typed_stop;
            return false;
        }
        return true;
    };
    const auto retreat_menu_context = [&]() {
        const auto nested = retreat_legacy_battle_menu_context(
            {
                .startup_reset = bindings.startup_reset,
                .final_actor = bindings.final_actor,
                .frame_input_resolution = bindings.frame_input_resolution,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_context_retreat_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleMenuContextRetreatStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                menu_context_retreat_typed_stop;
            return false;
        }
        return true;
    };
    const auto advance_menu_context = [&]() {
        const auto nested = advance_legacy_battle_menu_context(
            {
                .startup_reset = bindings.startup_reset,
                .final_actor = bindings.final_actor,
                .frame_input_resolution = bindings.frame_input_resolution,
                .input_dispatch = state,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.menu_context_advance_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status != LegacyBattleMenuContextAdvanceStatus::completed) {
            result.status = LegacyBattleInputDispatchStatus::
                menu_context_advance_typed_stop;
            return false;
        }
        return true;
    };
    const auto invoke_operation = [&](const LegacyBattleInputDispatchCall op) {
        if (op ==
            LegacyBattleInputDispatchCall::
                reserved_actor_action_commit_direct_slot) {
            return commit_actor_action();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_actor_action_cycle_slot) {
            return cycle_actor_action();
        }
        if (op ==
            LegacyBattleInputDispatchCall::
                reserved_actor_action_reverse_cycle_slot) {
            return reverse_cycle_actor_action();
        }
        if (op ==
            LegacyBattleInputDispatchCall::
                reserved_menu_selection_retreat_slot) {
            return retreat_menu_selection();
        }
        if (op ==
            LegacyBattleInputDispatchCall::
                reserved_menu_selection_advance_slot) {
            return advance_menu_selection();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_menu_page_retreat_slot) {
            return retreat_menu_page();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_menu_page_advance_slot) {
            return advance_menu_page();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_menu_input_finalize_slot) {
            return finalize_menu_input();
        }
        if (op ==
            LegacyBattleInputDispatchCall::
                reserved_target_selection_entry_slot) {
            return enter_target_selection();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_menu_context_advance_slot) {
            return advance_menu_context();
        }
        if (op ==
            LegacyBattleInputDispatchCall::reserved_menu_context_retreat_slot) {
            return retreat_menu_context();
        }
        static_cast<void>(call(op));
        return true;
    };

    state.menu_action = 0U;
    if (eax == 1U) {
        return finish();
    }

    eax = bindings.message_state;
    ecx = bindings.final_actor.queued_actor_code;
    if (signed_bits(eax) < 2 && ecx != 0U &&
        bindings.dialogs.messages.empty()) {
        for (u32 index = 0U; index < 8U; ++index) {
            const u32 dik = index + 2U;
            if (raw_key(dik) == 0U) {
                continue;
            }
            if (dik == 6U) {
                if (state.selection_index != 5U) {
                    if (bindings.message_state == 0U) {
                        bindings.message_state = 1U;
                    }
                    state.action_kind = 6U;
                    bindings.final_actor.pre_frame_gate_a = 1U;
                    state.selection_index = 5U;
                    if (!enter_target_selection()) {
                        return finish();
                    }
                }
                bindings.message_state = 0U;
                bindings.final_actor.pre_frame_gate_a = 0U;
                state.selection_index = 1U;
                return early();
            }
            if (bindings.message_state == 0U) {
                bindings.message_state = 1U;
            }
            if (!refresh_action_mode()) {
                return finish();
            }
            if (input_permission_byte(bindings.startup_reset, index) != 1U) {
                return early();
            }
            state.action_kind = 6U;
            bindings.final_actor.pre_frame_gate_a = 1U;
            state.selection_index = index + 1U;
            if (!enter_target_selection()) {
                return finish();
            }
            return early();
        }
        ecx = bindings.final_actor.queued_actor_code;
    }

    eax = state.input_gate;
    auto* base_record = record(1U);
    if (base_record == nullptr) {
        return finish();
    }
    u32 base_held = base_record->held_sample_count;
    if (eax == 1U && base_held == 1U &&
        base_record->rapid_press_multiplicity != 0U) {
        state.input_latch |= 1U;
    }

    auto* source = record(9U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (signed_bits(eax) >= 1 && !write_record_zero(eax)) {
            return finish();
        }
    }

    source = record(2U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        const u32 held_count = eax;
        if (repeat(eax, 3)) {
            if (signed_bits(bindings.message_state) > 1) {
                return early();
            }
            state.selected_option_word = held_count == 1U ? 0U : 3U;
            state.action_kind = 1U;
            if (!invoke_operation(
                    LegacyBattleInputDispatchCall::
                        reserved_actor_action_cycle_slot
                )) {
                return finish();
            }
            if (bindings.message_state == 1U) {
                if (!enter_target_selection()) {
                    return finish();
                }
            }
            state.selected_option_word = 0xFFFFU;
            return early();
        }
    }

    source = record(18U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (repeat(eax, 3)) {
            if (state.retreat_block_word != 0U ||
                state.action_block_gate == 1U ||
                bindings.debug_hotkeys.actor_retarget_gate_53bf64 == 1U ||
                bindings.message_state == 99U ||
                bindings.message_state == 100U ||
                !bindings.dialogs.messages.empty()) {
                return early();
            }
            const u32 queried_actor = bindings.final_actor.queued_actor_code;
            ecx = group_a_token(queried_actor);
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::query_active_actor,
                {ecx, queried_actor}
            ));
            if (eax == 1U) {
                return early();
            }
            edx = bindings.metrics.group_b_count;
            ecx = static_cast<u32>(static_cast<compat::u8>(
                bindings.final_actor.excluded_group_a_count
            ));
            edx -= ecx;
            if (edx == 1U && state.retreat_target_word != 0xFFFFU) {
                return early();
            }
            const u32 retreat_actor = bindings.final_actor.queued_actor_code;
            if (retreat_actor != 0U) {
                ecx = group_a_token(retreat_actor);
                const auto* actor = retreat_actor <
                        bindings.action.group_a_action_execution.size()
                    ? &bindings.action.group_a_action_execution[retreat_actor]
                    : nullptr;
                result.actor_retreat_ready =
                    query_legacy_battle_actor_retreat_ready(
                        actor,
                        {
                            .actor_token = ecx,
                            .entry_eax = eax,
                            .entry_edx = edx,
                        }
                    );
                ++result.actor_retreat_ready_calls;
                if (result.actor_retreat_ready.status !=
                    LegacyBattleActorRetreatReadyStatus::completed) {
                    result.status = LegacyBattleInputDispatchStatus::
                        actor_retreat_ready_typed_stop;
                    return result;
                }
                eax = result.actor_retreat_ready.return_eax;
                ecx = result.actor_retreat_ready.return_ecx;
                edx = result.actor_retreat_ready.return_edx;
                if (eax == 0U ||
                    (bindings.debug_hotkeys.battle_mode_flags_53bc24 &
                     0x200U) != 0U) {
                    port.delay_input_milliseconds(20U);
                    ++result.delay_calls;
                    result.text_messages.push_back(
                        enqueue_legacy_battle_text_message(
                            bindings.text_messages,
                            bindings.startup_reset.block_5214f8[0U],
                            text_message_port,
                            {
                                .value_04 = 0x118U,
                                .value_08 = 10U,
                                .kind = 5U,
                                .text_token =
                                    kLegacyBattleInputWarningTextToken,
                                .flags = 0x40000002U,
                                .entry = {.eax = eax, .ecx = ecx, .edx = edx},
                            }
                        )
                    );
                    ++result.text_message_calls;
                    const auto& warning = result.text_messages.back();
                    result.port_calls +=
                        warning.allocation_calls + warning.measure_calls;
                    eax = warning.return_registers.eax;
                    ecx = warning.return_registers.ecx;
                    edx = warning.return_registers.edx;
                    if (warning.status !=
                        LegacyBattleTextMessageStatus::completed) {
                        result.status = LegacyBattleInputDispatchStatus::
                            text_message_typed_stop;
                        return result;
                    }
                    edx = std::bit_cast<u32>(state.sample_mix_level);
                    const auto sample = port.play_input_sample(
                        kLegacyBattleInputWarningSample,
                        state.sample_mix_level,
                        eax,
                        ecx,
                        edx
                    );
                    eax = sample.eax;
                    ecx = sample.ecx;
                    edx = sample.edx;
                    return early();
                }
                port.delay_input_milliseconds(50U);
                ++result.delay_calls;
                const u32 actor_code = bindings.final_actor.queued_actor_code;
                const u32 workspace_index = actor_code + kWorkspaceActorOffset;
                bindings.message_state = 0x11U;
                if (workspace_index >=
                    bindings.action.opponent_workspace.size()) {
                    result.status =
                        LegacyBattleInputDispatchStatus::workspace_typed_stop;
                    return finish();
                }
                bindings.action.opponent_workspace[workspace_index] = 0x11U;
                ecx = group_a_token(actor_code);
                static_cast<void>(call(
                    LegacyBattleInputDispatchCall::configure_retreat_actor,
                    {ecx, 1U, actor_code}
                ));
                const u32 live_actor = bindings.final_actor.queued_actor_code;
                eax = live_actor;
                bindings.final_actor.auxiliary_gate = 1U;
                bindings.final_actor.secondary_actor_code = live_actor;
                bindings.message_state = 0U;
                state.action_word = 0U;
                bindings.final_actor.published_actor_code = live_actor - 7U;
                state.retreat_block_word =
                    static_cast<u16>(state.retreat_block_word | 0x4000U);
                bindings.final_actor.queued_actor_code = 0U;
                bindings.final_actor.pre_frame_gate_a = 0U;
                bindings.final_actor.frame_gate_b = 1U;
                bindings.final_actor.frame_gate_a = 1U;
                state.frame_value_a = 0U;
                state.action_kind = 0U;
                state.frame_value_b = 4U;
                ecx = 0U;
            }
            base_record = record(1U);
            if (base_record == nullptr) {
                return finish();
            }
            base_held = base_record->held_sample_count;
        }
    }

    source = record(17U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (repeat(eax, 3) && signed_bits(bindings.message_state) < 2 &&
            bindings.metrics.group_a_count != 0U &&
            bindings.dialogs.messages.empty()) {
            bindings.final_actor.pre_frame_gate_b = 0U;
            bindings.terminal_latch = 1U;
            bindings.final_actor.action_execution_active = 1U;
            const u32 actor_code = bindings.final_actor.queued_actor_code;
            const u32 workspace_index = actor_code + kWorkspaceActorOffset;
            if (workspace_index >= bindings.action.opponent_workspace.size()) {
                result.status =
                    LegacyBattleInputDispatchStatus::workspace_typed_stop;
                return finish();
            }
            bindings.action.opponent_workspace[workspace_index] = 1U;
            bindings.final_actor.pre_frame_gate_a = 1U;
            state.selection_index = 1U;
            bindings.message_state = 3U;
            return early();
        }
    }

    source = record(14U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (signed_bits(eax) >= 1) {
            bindings.context_prompt.frame_counter = 0U;
            bindings.final_actor.pre_frame_gate_b = 1U;
            if (!write_record_zero(eax)) {
                return finish();
            }
        }
    }

    source = record(15U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        ecx = source->held_sample_count;
        if (repeat(ecx, 3)) {
            eax = (eax & 0xFFFF0000U) |
                static_cast<u32>(state.selected_option_word);
            bindings.final_actor.pre_frame_gate_b = 1U;
            if (state.selected_option_word != 0xFFFFU) {
                eax = static_cast<u32>(static_cast<i32>(
                    std::bit_cast<compat::i16>(state.selected_option_word)
                ));
                if (!invoke_operation(
                        LegacyBattleInputDispatchCall::
                            reserved_actor_action_commit_direct_slot
                    )) {
                    return finish();
                }
                if (bindings.message_state == 1U) {
                    if (!enter_target_selection()) {
                        return finish();
                    }
                }
                state.selected_option_word = 0xFFFFU;
                return early();
            }
            bindings.context_prompt.frame_counter = 0U;
            if (state.interaction_mode == 1U) {
                state.menu_action = 1U;
                if (!retreat_menu_selection()) {
                    return finish();
                }
                source = record(15U);
                if (source == nullptr) {
                    return finish();
                }
                ecx = source->held_sample_count;
            }
            if (state.interaction_mode == 2U) {
                state.menu_action = 2U;
                if (!advance_menu_selection()) {
                    return finish();
                }
                source = record(15U);
                if (source == nullptr) {
                    return finish();
                }
                ecx = source->held_sample_count;
            }
            if (state.interaction_mode == 3U) {
                state.menu_action = 3U;
                if (!retreat_menu_page()) {
                    return finish();
                }
                source = record(15U);
                if (source == nullptr) {
                    return finish();
                }
                ecx = source->held_sample_count;
            }
            if (state.interaction_mode == 4U) {
                state.menu_action = 4U;
                if (!advance_menu_page()) {
                    return finish();
                }
                source = record(15U);
                if (source == nullptr) {
                    return finish();
                }
                ecx = source->held_sample_count;
            }
            eax = static_cast<u32>(request.mouse_y);
            if (eax > request.mouse_lower_bound &&
                eax < request.mouse_upper_bound && ecx == 1U) {
                state.captured_mouse_y = eax;
                state.captured_mouse_aux = 0U;
            }
            if (state.mouse_action_gate == 1U && signed_bits(ecx) >= 1) {
                state.menu_action = 5U;
            }
            base_held = ecx;
            if (!write_record_zero(base_held)) {
                return finish();
            }
            bindings.final_actor.pre_frame_gate_b = 1U;
        }
    }

    source = record(12U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        ecx = source->held_sample_count;
        const i32 signed_count = signed_bits(ecx);
        eax = std::bit_cast<u32>(signed_count / 3);
        edx = std::bit_cast<u32>(signed_count % 3);
        if (signed_count % 3 == 1) {
            base_held = ecx;
            bindings.final_actor.pre_frame_gate_b = 0U;
            if (!write_record_zero(base_held)) {
                return finish();
            }
        }
    } else {
        base_record = record(0U);
        if (base_record == nullptr) {
            return finish();
        }
        if (base_record->rapid_press_multiplicity == 0U) {
            base_held = 0U;
        }
    }

    base_record = record(0U);
    if (base_record == nullptr) {
        return finish();
    }
    if (base_record->rapid_press_multiplicity != 0U) {
        bool trigger = base_held == 1U;
        if (!trigger && signed_bits(base_held) >= 15) {
            const i32 signed_count = signed_bits(base_held);
            ecx = 6U;
            eax = std::bit_cast<u32>(signed_count / 6);
            edx = std::bit_cast<u32>(signed_count % 6);
            trigger = signed_count % 6 == 1;
        }
        if (trigger) {
            if (signed_bits(state.signed_status) < 0) {
                state.signed_status |= 1U;
            }
            if ((bindings.final_actor.pre_frame_gate_b == 1U &&
                 state.choice_guard != 0U) ||
                (bindings.final_actor.pre_frame_gate_b != 1U &&
                 !bindings.choice_hotspots.empty())) {
                bindings.choice_hotspots.clear();
                story_scene::clear_legacy_dialog_choice_chain(bindings.dialogs);
                eax = 0U;
            }
            source = record(15U);
            if (source == nullptr) {
                return finish();
            }
            if (source->rapid_press_multiplicity == 0U) {
                bindings.final_actor.pre_frame_gate_b = 0U;
                bindings.context_prompt.frame_counter = 300U;
                bindings.final_actor.pre_frame_gate_a = 1U;
                state.interaction_mode = 0U;
            }
            if ((bindings.debug_hotkeys.battle_mode_flags_53bc24 & 0x20U) !=
                0U) {
                bindings.debug_hotkeys.battle_mode_flags_53bc24 &= 0xFFFFFFDFU;
            }
            if (!enter_target_selection()) {
                return finish();
            }
        }
    }

    const auto repeat_three_action =
        [&](const std::size_t index,
            const LegacyBattleInputDispatchCall first,
            const LegacyBattleInputDispatchCall second,
            const u32 menu_action) {
            auto* const input = record(index);
            if (input == nullptr || input->rapid_press_multiplicity == 0U) {
                return input != nullptr;
            }
            eax = input->held_sample_count;
            if (eax != 1U && signed_bits(eax) >= 15) {
                ecx = 3U;
            }
            if (!repeat(eax, 3) || !bindings.dialogs.messages.empty()) {
                return true;
            }
            if (!invoke_operation(first)) {
                return false;
            }
            if (!invoke_operation(second)) {
                return false;
            }
            state.menu_action = menu_action;
            return true;
        };
    if (!repeat_three_action(
            4U,
            LegacyBattleInputDispatchCall::reserved_menu_selection_retreat_slot,
            LegacyBattleInputDispatchCall::reserved_actor_action_cycle_slot,
            1U
        )) {
        return finish();
    }
    if (!repeat_three_action(
            6U,
            LegacyBattleInputDispatchCall::reserved_menu_selection_advance_slot,
            LegacyBattleInputDispatchCall::
                reserved_actor_action_reverse_cycle_slot,
            2U
        )) {
        return finish();
    }

    source = record(3U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (eax != 1U && signed_bits(eax) >= 15) {
            ecx = 3U;
        }
        if (repeat(eax, 3)) {
            if (!bindings.choice_hotspots.empty()) {
                --state.choice_selection_index;
                if ((state.choice_selection_index & 0x8000U) != 0U) {
                    eax = static_cast<u32>(bindings.choice_hotspots.size());
                    state.choice_selection_index = eax - 1U;
                }
            }
            if (bindings.dialogs.messages.empty()) {
                if (bindings.message_state == 3U) {
                    if (!retreat_menu_selection()) {
                        return finish();
                    }
                }
                if (!invoke_operation(
                        LegacyBattleInputDispatchCall::
                            reserved_actor_action_reverse_cycle_slot
                    )) {
                    return finish();
                }
                if (!invoke_operation(
                        LegacyBattleInputDispatchCall::
                            reserved_menu_context_retreat_slot
                    )) {
                    return finish();
                }
            }
        }
    }

    source = record(5U);
    if (source == nullptr) {
        return finish();
    }
    if (source->rapid_press_multiplicity != 0U) {
        eax = source->held_sample_count;
        if (eax != 1U && signed_bits(eax) >= 15) {
            ecx = 3U;
        }
        if (repeat(eax, 3)) {
            if (!bindings.choice_hotspots.empty()) {
                ++state.choice_selection_index;
                eax = static_cast<u32>(bindings.choice_hotspots.size()) - 1U;
                if (state.choice_selection_index > eax) {
                    state.choice_selection_index = 0U;
                }
            }
            if (bindings.dialogs.messages.empty()) {
                if (bindings.message_state == 3U) {
                    if (!advance_menu_selection()) {
                        return finish();
                    }
                }
                if (!invoke_operation(
                        LegacyBattleInputDispatchCall::
                            reserved_menu_context_advance_slot
                    )) {
                    return finish();
                }
                if (!invoke_operation(
                        LegacyBattleInputDispatchCall::
                            reserved_actor_action_cycle_slot
                    )) {
                    return finish();
                }
            }
        }
    }

    for (const auto [index, operation] :
         {std::pair{
              7U, LegacyBattleInputDispatchCall::reserved_menu_page_retreat_slot
          },
          std::pair{
              8U, LegacyBattleInputDispatchCall::reserved_menu_page_advance_slot
          }}) {
        source = record(index);
        if (source == nullptr) {
            return finish();
        }
        if (source->rapid_press_multiplicity == 0U) {
            continue;
        }
        const i32 signed_count = signed_bits(source->held_sample_count);
        ecx = 3U;
        eax = std::bit_cast<u32>(signed_count / 3);
        edx = std::bit_cast<u32>(signed_count % 3);
        if (signed_count % 3 == 1 && !invoke_operation(operation)) {
            return finish();
        }
    }

    base_record = record(0U);
    if (base_record == nullptr) {
        return finish();
    }
    if (base_record->rapid_press_multiplicity != 0U) {
        eax = base_record->held_sample_count;
        if (eax != 1U && signed_bits(eax) >= 15) {
            ecx = 3U;
        }
        if (repeat(eax, 3)) {
            if (!finalize_menu_input()) {
                return finish();
            }
            state.final_value_a = 0U;
            state.final_value_b = 0U;
        }
    }
    return finish();
}

}  // namespace openswd3::battle
