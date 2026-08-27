#include "openswd3/battle/legacy_battle_menu_selection_advance.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kSelectionSample = 0x2EU;
inline constexpr u32 kPermissionBaseToken = 0x00524413U;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

inline constexpr u32 kGroupASelectionBaseToken =
    kLegacyBattleActionGroupABaseToken - kLegacyBattleActionGroupAStride;

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_a_selection_token(const u32 code) noexcept {
    return kGroupASelectionBaseToken + code * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] std::array<u8, 9>
permission_bytes(const LegacyBattleStartupResetBlocks& reset) noexcept {
    return {
        reset.value_524413,
        static_cast<u8>(reset.value_524414),
        static_cast<u8>(reset.value_524414 >> 8U),
        static_cast<u8>(reset.value_524414 >> 16U),
        static_cast<u8>(reset.value_524414 >> 24U),
        static_cast<u8>(reset.value_524418),
        static_cast<u8>(reset.value_524418 >> 8U),
        static_cast<u8>(reset.value_524418 >> 16U),
        static_cast<u8>(reset.value_524418 >> 24U),
    };
}

}  // namespace

LegacyBattleMenuSelectionAdvanceResult advance_legacy_battle_menu_selection(
    LegacyBattleMenuSelectionAdvanceBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuSelectionAdvanceRequest& request
) {
    LegacyBattleMenuSelectionAdvanceResult result;
    auto& frame = bindings.frame_input_resolution;
    auto& input = bindings.input_dispatch;
    u32 eax = bindings.message_state - 1U;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto stop = [&](const LegacyBattleMenuSelectionAdvanceStatus status) {
        result.status = status;
        return finish();
    };
    const auto call = [&](const LegacyBattleInputDispatchCall operation,
                          const u32 actor_token,
                          const std::array<u32, 5>& arguments = {}) {
        ecx = actor_token;
        const auto reply = port.invoke_input_dispatch({
            .call = operation,
            .arguments = arguments,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        return reply;
    };
    const auto play_sample = [&]() {
        edx = std::bit_cast<u32>(input.sample_mix_level);
        const auto reply = port.play_input_sample(
            kSelectionSample, input.sample_mix_level, eax, ecx, edx
        );
        ++result.port_calls;
        ++result.sample_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };
    const auto common_sample_success = [&]() {
        play_sample();
        input.mouse_action_gate = 1U;
        return finish();
    };
    const auto mode_index = [&](const u32 actor_code) {
        return actor_code * 5U - 40U;
    };
    const auto load_group_b_object_registers = [&](const u32 index) {
        ecx = index;
        eax = index * 0x565U;
        edx = index * 0x159U;
        ecx = kLegacyBattleActionGroupBBaseToken + eax * 8U;
    };
    const auto load_group_a_completion_registers = [&](const u32 code) {
        ecx = code;
        eax = code * kLegacyBattleActionGroupAStride;
    };
    const auto load_group_a_query_registers = [&](const u32 code) {
        load_group_a_completion_registers(code);
        ecx = group_a_selection_token(code);
    };
    const auto load_group_a_prepare_registers = [&](const u32 code) {
        ecx = code;
        eax = code * 0x3EFU;
        edx = code * 0xBCDU;
        ecx = kLegacyBattleActionGroupABaseToken + edx * 4U;
    };
    const auto load_group_a_selection_registers = [&](const u32 code) {
        ecx = code;
        eax = code * 0xBCDU;
        ecx = group_a_selection_token(code);
    };

    bindings.final_actor.pre_frame_gate_b = 0U;
    if (eax > 29U) {
        return finish();
    }

    constexpr std::array<u8, 30> kSwitchIndices{
        0U, 1U, 2U, 3U, 4U, 9U, 5U, 6U, 9U, 9U, 9U, 9U, 9U, 9U, 9U,
        9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 7U, 9U, 9U, 8U,
    };
    ecx = kSwitchIndices[eax];

    switch (bindings.message_state) {
    case 1U: {
        eax = input.selection_index;
        if (signed_bits(eax) > 8) {
            input.selection_index = 1U;
            return finish();
        }
        play_sample();
        eax = input.selection_index + 1U;
        input.selection_index = eax;
        auto permissions = permission_bytes(bindings.startup_reset);
        if (eax >= permissions.size()) {
            return stop(
                LegacyBattleMenuSelectionAdvanceStatus::permission_typed_stop
            );
        }
        if (permissions[eax] == 0U) {
            ecx = static_cast<u32>(bindings.startup_reset.value_53bf22);
            edx = ecx + 5U;
            ecx = kPermissionBaseToken;
            while (true) {
                ++eax;
                if (signed_bits(eax) > signed_bits(edx)) {
                    eax = 1U;
                }
                if (eax >= permissions.size()) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            permission_typed_stop
                    );
                }
                if (permissions[eax] != 0U) {
                    break;
                }
            }
        }
        input.mouse_action_gate = 1U;
        input.selection_index = eax;
        return finish();
    }
    case 2U: {
        eax = frame.list_selection + 1U;
        frame.list_selection = eax;
        if (signed_bits(eax) > 7) {
            ecx = frame.panel_scroll_a + 1U;
            frame.list_selection = 7U;
            eax = std::bit_cast<u32>(static_cast<i32>(
                static_cast<compat::i8>(frame.panel_row_limit_a)
            ));
            frame.panel_scroll_a = ecx;
            ecx += 7U;
            if (signed_bits(ecx) > signed_bits(eax)) {
                eax -= 7U;
                frame.panel_scroll_a = eax;
                input.mouse_action_gate = 1U;
                return finish();
            }
            return common_sample_success();
        }
        ecx = std::bit_cast<u32>(
            static_cast<i32>(static_cast<compat::i8>(frame.panel_row_limit_a))
        );
        if (signed_bits(eax) > signed_bits(ecx)) {
            frame.list_selection = ecx;
            input.mouse_action_gate = 1U;
            return finish();
        }
        return common_sample_success();
    }
    case 3U: {
        eax = frame.target_selection_block;
        if (eax == 1U) {
            return finish();
        }
        play_sample();
        const u32 startup_index =
            mode_index(bindings.final_actor.queued_actor_code);
        if (startup_index >= bindings.startup_reset.block_520e90.size()) {
            return stop(
                LegacyBattleMenuSelectionAdvanceStatus::startup_mode_typed_stop
            );
        }
        if (bindings.startup_reset.block_520e90[startup_index] == 0U) {
            while (true) {
                eax = frame.target_cursor + 1U;
                ecx = bindings.metrics.group_b_count;
                frame.target_cursor = eax;
                if (signed_bits(eax) > signed_bits(ecx)) {
                    eax = 1U;
                    frame.target_cursor = eax;
                }
                if (eax >= bindings.metrics.group_b_order.size()) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            group_b_order_typed_stop
                    );
                }
                frame.target_actor_index = bindings.metrics.group_b_order[eax];
                load_group_b_object_registers(frame.target_actor_index);
                if (frame.target_actor_index >= 8U) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            group_b_actor_typed_stop
                    );
                }
                const auto candidate = call(
                    LegacyBattleInputDispatchCall::
                        menu_advance_query_group_b_candidate,
                    ecx
                );
                if (candidate.eax != 1U) {
                    break;
                }
            }

            load_group_b_object_registers(frame.target_actor_index);
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_advance_prepare_actor_origin,
                ecx
            ));
            eax = bindings.metrics.group_b_count;
            u32 actor_index = 0U;
            while (signed_bits(actor_index) < signed_bits(eax)) {
                ecx = group_b_token(actor_index);
                if (actor_index >= 8U) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            group_b_actor_typed_stop
                    );
                }
                static_cast<void>(call(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection,
                    ecx,
                    {0U}
                ));
                eax = bindings.metrics.group_b_count;
                ++actor_index;
                ++result.actor_iterations;
            }
            load_group_b_object_registers(frame.target_actor_index);
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_advance_configure_actor_selection,
                ecx,
                {1U}
            ));
            input.mouse_action_gate = 1U;
            frame.target_selection_gate = 1U;
            ecx = frame.target_actor_index + 1U;
            input.action_kind = ecx;
            return finish();
        }

        eax = bindings.metrics.group_a_count;
        edx = static_cast<u32>(bindings.final_actor.excluded_group_a_count);
        ecx = static_cast<u32>(bindings.startup_supplemental_count_word);
        eax -= edx;
        eax -= ecx;

        if (eax >= 4U) {
            edx = frame.target_cursor + 1U;
            frame.target_cursor = edx;
            if (signed_bits(edx) > signed_bits(eax)) {
                edx = 1U;
                frame.target_cursor = edx;
            }
            while (true) {
                if (edx >= bindings.final_actor.actor_order.size()) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            actor_order_typed_stop
                    );
                }
                const u32 actor_code = bindings.final_actor.actor_order[edx];
                frame.target_actor_index = actor_code;
                load_group_a_completion_registers(actor_code);
                if (actor_code == 0U ||
                    actor_code >
                        bindings.final_actor.group_a_completion_flags.size() ||
                    actor_code >
                        bindings.final_actor.group_a_slot_values.size()) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            group_a_actor_typed_stop
                    );
                }
                const u32 physical_index = actor_code - 1U;
                bool rejected =
                    bindings.final_actor
                            .group_a_completion_flags[physical_index] == 1U ||
                    bindings.final_actor.group_a_slot_values[physical_index] ==
                        1U;
                if (!rejected) {
                    load_group_a_query_registers(actor_code);
                    const auto candidate = call(
                        LegacyBattleInputDispatchCall::
                            menu_advance_query_group_a_candidate,
                        ecx
                    );
                    rejected = candidate.eax == 1U;
                }
                if (!rejected) {
                    break;
                }
                edx = frame.target_cursor;
                ecx = bindings.metrics.group_a_count;
                eax = static_cast<u32>(
                    bindings.final_actor.excluded_group_a_count
                );
                ++edx;
                ecx -= eax;
                eax =
                    static_cast<u32>(bindings.startup_supplemental_count_word);
                frame.target_cursor = edx;
                ecx -= eax;
                if (signed_bits(edx) > signed_bits(ecx)) {
                    edx = 1U;
                    frame.target_cursor = edx;
                }
                ++result.actor_iterations;
            }
            load_group_a_prepare_registers(frame.target_actor_index);
            if (frame.target_actor_index >= 10U) {
                return stop(
                    LegacyBattleMenuSelectionAdvanceStatus::
                        group_a_actor_typed_stop
                );
            }
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_advance_prepare_actor_origin,
                ecx
            ));
            eax = frame.target_actor_index;
            frame.target_actor_index = 0U;
            ++eax;
            input.action_kind = eax;
        } else {
            while (true) {
                ecx = input.action_kind;
                eax = bindings.metrics.group_a_count;
                ++ecx;
                input.action_kind = ecx;
                if (signed_bits(ecx) > signed_bits(eax)) {
                    ecx = 1U;
                    input.action_kind = ecx;
                }
                load_group_a_completion_registers(ecx);
                if (ecx == 0U ||
                    ecx >
                        bindings.final_actor.group_a_completion_flags.size() ||
                    ecx > bindings.final_actor.group_a_slot_values.size()) {
                    return stop(
                        LegacyBattleMenuSelectionAdvanceStatus::
                            group_a_actor_typed_stop
                    );
                }
                const u32 physical_index = ecx - 1U;
                bool rejected =
                    bindings.final_actor
                            .group_a_completion_flags[physical_index] == 1U ||
                    bindings.final_actor.group_a_slot_values[physical_index] ==
                        1U;
                if (!rejected) {
                    const u32 actor_code = ecx;
                    load_group_a_query_registers(actor_code);
                    const auto candidate = call(
                        LegacyBattleInputDispatchCall::
                            menu_advance_query_group_a_candidate,
                        ecx
                    );
                    rejected = candidate.eax == 1U;
                }
                if (!rejected) {
                    break;
                }
                ++result.actor_iterations;
            }
            load_group_a_selection_registers(input.action_kind);
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_advance_prepare_actor_origin,
                ecx
            ));
        }

        for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_advance_configure_actor_selection,
                group_a_token(actor_index),
                {0U}
            ));
            if (actor_index >= frame.target_markers.size()) {
                return stop(
                    LegacyBattleMenuSelectionAdvanceStatus::
                        target_marker_typed_stop
                );
            }
            frame.target_markers[actor_index] = 0U;
            ++result.actor_iterations;
        }
        const u32 selected_code = input.action_kind;
        load_group_a_selection_registers(selected_code);
        if (selected_code == 0U || selected_code > 10U) {
            return stop(
                LegacyBattleMenuSelectionAdvanceStatus::group_a_actor_typed_stop
            );
        }
        static_cast<void>(call(
            LegacyBattleInputDispatchCall::
                menu_advance_configure_actor_selection,
            ecx,
            {1U}
        ));
        input.mouse_action_gate = 1U;
        frame.target_selection_gate = 1U;
        return finish();
    }
    case 4U: {
        ecx = frame.grid_selection + 1U;
        frame.grid_selection = ecx;
        if (signed_bits(ecx) > 7) {
            ecx = frame.panel_scroll_b + 1U;
            eax = static_cast<u32>(frame.panel_row_limit_c);
            frame.panel_scroll_b = ecx;
            ecx += 7U;
            frame.grid_selection = 7U;
            if (signed_bits(ecx) > signed_bits(eax)) {
                eax -= 7U;
                frame.panel_scroll_b = eax;
            } else {
                play_sample();
            }
        } else {
            eax = static_cast<u32>(frame.panel_row_limit_c);
            if (signed_bits(ecx) > signed_bits(eax)) {
                frame.grid_selection = eax;
            } else {
                play_sample();
            }
        }
        eax = frame.current_equipment_selection;
        ecx = frame.grid_selection;
        edx = frame.panel_scroll_b;
        if (eax >= frame.equipment_grid_selections.size()) {
            return stop(
                LegacyBattleMenuSelectionAdvanceStatus::
                    equipment_selection_typed_stop
            );
        }
        frame.equipment_grid_selections[eax] = ecx;
        if (eax >= bindings.startup_reset.values_52544c.size()) {
            return stop(
                LegacyBattleMenuSelectionAdvanceStatus::
                    equipment_scroll_typed_stop
            );
        }
        bindings.startup_reset.values_52544c[eax] = edx;
        input.mouse_action_gate = 1U;
        return finish();
    }
    case 5U:
        play_sample();
        eax = frame.group_b_row_selection + 1U;
        frame.group_b_row_selection = eax;
        if (signed_bits(eax) > 2) {
            frame.group_b_row_selection = 1U;
        }
        return finish();
    case 7U:
        frame.transition_value_a = 0U;
        frame.transition_value_b = 0U;
        play_sample();
        eax = frame.alternate_selection + 1U;
        ecx = frame.alternate_selection_limit;
        frame.alternate_selection = eax;
        if (signed_bits(eax) > signed_bits(ecx)) {
            frame.alternate_selection = 1U;
        }
        return finish();
    case 8U: {
        ecx = (ecx & 0xFFFFFF00U) | static_cast<u32>(frame.panel_row_limit_b);
        if (frame.panel_row_limit_b == 0U) {
            return finish();
        }
        eax = frame.narrow_list_selection;
        edx = std::bit_cast<u32>(
            static_cast<i32>(static_cast<compat::i8>(frame.panel_row_limit_b))
        );
        ++eax;
        frame.narrow_list_selection = eax;
        if (signed_bits(eax) > signed_bits(edx)) {
            frame.narrow_list_selection = 1U;
        }
        return common_sample_success();
    }
    case 27U: {
        ecx = frame.grid_selection + 1U;
        frame.grid_selection = ecx;
        if (signed_bits(ecx) > 7) {
            ecx = frame.panel_scroll_b + 1U;
            eax = static_cast<u32>(frame.panel_row_limit_c);
            frame.panel_scroll_b = ecx;
            ecx += 7U;
            frame.grid_selection = 7U;
            if (signed_bits(ecx) > signed_bits(eax)) {
                eax -= 7U;
                frame.panel_scroll_b = eax;
                input.mouse_action_gate = 1U;
                return finish();
            }
            return common_sample_success();
        }
        eax = static_cast<u32>(frame.panel_row_limit_c);
        if (signed_bits(ecx) > signed_bits(eax)) {
            frame.grid_selection = eax;
            input.mouse_action_gate = 1U;
            return finish();
        }
        return common_sample_success();
    }
    case 30U:
        eax = frame.grid_selection + 1U;
        frame.grid_selection = eax;
        if (signed_bits(eax) > 10) {
            frame.grid_selection = 1U;
        }
        return common_sample_success();
    default:
        return finish();
    }
}

}  // namespace openswd3::battle
