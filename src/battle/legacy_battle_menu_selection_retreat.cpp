#include "openswd3/battle/legacy_battle_menu_selection_retreat.hpp"

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

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
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

LegacyBattleMenuSelectionRetreatResult retreat_legacy_battle_menu_selection(
    LegacyBattleMenuSelectionRetreatBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuSelectionRetreatRequest& request
) {
    LegacyBattleMenuSelectionRetreatResult result;
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
    const auto stop = [&](const LegacyBattleMenuSelectionRetreatStatus status) {
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
        eax = input.selection_index - 1U;
        input.selection_index = eax;
        auto permissions = permission_bytes(bindings.startup_reset);
        if (eax >= permissions.size()) {
            return stop(
                LegacyBattleMenuSelectionRetreatStatus::permission_typed_stop
            );
        }
        if (permissions[eax] == 0U) {
            edx = (edx & 0xFFFF0000U) |
                static_cast<u32>(bindings.startup_reset.value_53bf22);
            ecx = kPermissionBaseToken;
            while (true) {
                --eax;
                if (signed_bits(eax) < 1) {
                    eax =
                        static_cast<u32>(bindings.startup_reset.value_53bf22) +
                        5U;
                }
                if (eax >= permissions.size()) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
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
        eax = frame.list_selection - 1U;
        frame.list_selection = eax;
        if (signed_bits(eax) >= 1) {
            return common_sample_success();
        }
        frame.list_selection = 1U;
        eax = frame.panel_scroll_a;
        if (signed_bits(eax) > 0) {
            play_sample();
        }
        eax = frame.panel_scroll_a - 1U;
        frame.panel_scroll_a = eax;
        if (signed_bits(eax) >= 0) {
            input.mouse_action_gate = 1U;
            return finish();
        }
        input.mouse_action_gate = 1U;
        frame.panel_scroll_a = 0U;
        return finish();
    }
    case 3U: {
        eax = frame.target_selection_block;
        if (eax == 1U) {
            return finish();
        }
        play_sample();
        const u32 startup_index =
            mode_index(bindings.final_actor.active_actor_code);
        if (startup_index >= bindings.startup_reset.block_520e90.size()) {
            return stop(
                LegacyBattleMenuSelectionRetreatStatus::startup_mode_typed_stop
            );
        }
        if (bindings.startup_reset.block_520e90[startup_index] == 0U) {
            while (true) {
                eax = frame.target_cursor - 1U;
                frame.target_cursor = eax;
                if (signed_bits(eax) < 1) {
                    eax = bindings.metrics.group_b_count;
                    frame.target_cursor = eax;
                }
                if (eax >= bindings.metrics.group_b_order.size()) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            group_b_order_typed_stop
                    );
                }
                frame.target_actor_index = bindings.metrics.group_b_order[eax];
                if (frame.target_actor_index >= 8U) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            group_b_actor_typed_stop
                    );
                }
                const u32 actor_token = group_b_token(frame.target_actor_index);
                const auto candidate = call(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_query_group_b_candidate,
                    actor_token
                );
                if (candidate.eax != 1U) {
                    break;
                }
            }

            const u32 selected_token = group_b_token(frame.target_actor_index);
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_retreat_prepare_actor_origin,
                selected_token
            ));
            u32 actor_index = 0U;
            while (signed_bits(actor_index) <
                   signed_bits(bindings.metrics.group_b_count)) {
                if (actor_index >= 8U) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            group_b_actor_typed_stop
                    );
                }
                static_cast<void>(call(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection,
                    group_b_token(actor_index),
                    {0U}
                ));
                ++actor_index;
                ++result.actor_iterations;
            }
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_retreat_configure_actor_selection,
                selected_token,
                {1U}
            ));
            input.mouse_action_gate = 1U;
            frame.target_selection_gate = 1U;
            ecx = frame.target_actor_index + 1U;
            input.action_kind = ecx;
            return finish();
        }

        const auto remaining = [&]() {
            u32 value = bindings.metrics.group_a_count;
            value -=
                static_cast<u32>(bindings.final_actor.excluded_group_a_count);
            value -= static_cast<u32>(bindings.startup_supplemental_count_word);
            return value;
        };

        if (remaining() >= 4U) {
            ecx = frame.target_cursor - 1U;
            frame.target_cursor = ecx;
            if (signed_bits(ecx) < 1) {
                ecx = remaining();
                frame.target_cursor = ecx;
            }
            while (true) {
                if (ecx >= bindings.final_actor.actor_order.size()) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            actor_order_typed_stop
                    );
                }
                const u32 actor_index = bindings.final_actor.actor_order[ecx];
                frame.target_actor_index = actor_index;
                if (actor_index >=
                        bindings.final_actor.group_a_completion_flags.size() ||
                    actor_index >=
                        bindings.final_actor.group_a_slot_values.size()) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            group_a_actor_typed_stop
                    );
                }
                bool rejected =
                    bindings.final_actor
                            .group_a_completion_flags[actor_index] == 1U ||
                    bindings.final_actor.group_a_slot_values[actor_index] == 1U;
                if (!rejected) {
                    const auto candidate = call(
                        LegacyBattleInputDispatchCall::
                            menu_retreat_query_group_a_candidate,
                        group_a_token(actor_index)
                    );
                    rejected = candidate.eax == 1U;
                }
                if (!rejected) {
                    break;
                }
                ecx = frame.target_cursor - 1U;
                frame.target_cursor = ecx;
                if (signed_bits(ecx) < 1) {
                    ecx = remaining();
                    frame.target_cursor = ecx;
                }
                ++result.actor_iterations;
            }
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_retreat_prepare_actor_origin,
                group_a_token(frame.target_actor_index)
            ));
            eax = frame.target_actor_index;
            frame.target_actor_index = 0U;
            input.action_kind = eax + 1U;
        } else {
            ecx = input.action_kind;
            while (true) {
                --ecx;
                input.action_kind = ecx;
                if (signed_bits(ecx) < 1) {
                    ecx = bindings.metrics.group_a_count;
                    input.action_kind = ecx;
                }
                if (ecx >=
                        bindings.final_actor.group_a_completion_flags.size() ||
                    ecx >= bindings.final_actor.group_a_slot_values.size()) {
                    return stop(
                        LegacyBattleMenuSelectionRetreatStatus::
                            group_a_actor_typed_stop
                    );
                }
                bool rejected =
                    bindings.final_actor.group_a_completion_flags[ecx] == 1U ||
                    bindings.final_actor.group_a_slot_values[ecx] == 1U;
                if (!rejected) {
                    const auto candidate = call(
                        LegacyBattleInputDispatchCall::
                            menu_retreat_query_group_a_candidate,
                        group_a_token(ecx)
                    );
                    rejected = candidate.eax == 1U;
                }
                if (!rejected) {
                    break;
                }
                ++result.actor_iterations;
            }
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_retreat_prepare_actor_origin,
                group_a_token(input.action_kind)
            ));
        }

        for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
            static_cast<void>(call(
                LegacyBattleInputDispatchCall::
                    menu_retreat_configure_actor_selection,
                group_a_token(actor_index),
                {0U}
            ));
            if (actor_index >= frame.target_markers.size()) {
                return stop(
                    LegacyBattleMenuSelectionRetreatStatus::
                        target_marker_typed_stop
                );
            }
            frame.target_markers[actor_index] = 0U;
            ++result.actor_iterations;
        }
        const u32 selected_index = input.action_kind;
        if (selected_index >= 10U) {
            return stop(
                LegacyBattleMenuSelectionRetreatStatus::group_a_actor_typed_stop
            );
        }
        static_cast<void>(call(
            LegacyBattleInputDispatchCall::
                menu_retreat_configure_actor_selection,
            group_a_token(selected_index),
            {1U}
        ));
        input.mouse_action_gate = 1U;
        frame.target_selection_gate = 1U;
        return finish();
    }
    case 4U: {
        eax = frame.grid_selection - 1U;
        frame.grid_selection = eax;
        if (signed_bits(eax) < 1) {
            frame.grid_selection = frame.panel_row_limit_c != 0U ? 1U : 0U;
            eax = frame.panel_scroll_b - 1U;
            frame.panel_scroll_b = eax;
            if (signed_bits(eax) < 0) {
                frame.panel_scroll_b = 0U;
            }
        }
        play_sample();
        eax = frame.current_equipment_selection;
        edx = frame.grid_selection;
        ecx = frame.panel_scroll_b;
        input.mouse_action_gate = 1U;
        if (eax >= frame.equipment_grid_selections.size()) {
            return stop(
                LegacyBattleMenuSelectionRetreatStatus::
                    equipment_selection_typed_stop
            );
        }
        frame.equipment_grid_selections[eax] = edx;
        if (eax >= bindings.startup_reset.values_52544c.size()) {
            return stop(
                LegacyBattleMenuSelectionRetreatStatus::
                    equipment_scroll_typed_stop
            );
        }
        bindings.startup_reset.values_52544c[eax] = ecx;
        return finish();
    }
    case 5U:
        play_sample();
        eax = frame.group_b_row_selection - 1U;
        frame.group_b_row_selection = eax;
        if (signed_bits(eax) < 1) {
            frame.group_b_row_selection = 2U;
        }
        return finish();
    case 7U:
        frame.transition_value_a = 0U;
        frame.transition_value_b = 0U;
        play_sample();
        eax = frame.alternate_selection - 1U;
        frame.alternate_selection = eax;
        if (signed_bits(eax) < 1) {
            ecx = frame.alternate_selection_limit;
            frame.alternate_selection = ecx;
        }
        return finish();
    case 8U: {
        ecx = (ecx & 0xFFFFFF00U) | static_cast<u32>(frame.panel_row_limit_b);
        if (frame.panel_row_limit_b == 0U) {
            return finish();
        }
        eax = frame.narrow_list_selection - 1U;
        frame.narrow_list_selection = eax;
        if (signed_bits(eax) < 1) {
            edx = std::bit_cast<u32>(static_cast<i32>(
                static_cast<compat::i8>(frame.panel_row_limit_b)
            ));
            frame.narrow_list_selection = edx;
        }
        return common_sample_success();
    }
    case 27U:
        eax = frame.grid_selection - 1U;
        frame.grid_selection = eax;
        if (signed_bits(eax) >= 1) {
            return common_sample_success();
        }
        frame.grid_selection = frame.panel_row_limit_c != 0U ? 1U : 0U;
        eax = frame.panel_scroll_b - 1U;
        frame.panel_scroll_b = eax;
        if (signed_bits(eax) >= 0) {
            input.mouse_action_gate = 1U;
            return finish();
        }
        input.mouse_action_gate = 1U;
        frame.panel_scroll_b = 0U;
        return finish();
    case 30U:
        eax = frame.grid_selection - 1U;
        frame.grid_selection = eax;
        if (signed_bits(eax) < 1) {
            frame.grid_selection = 10U;
        }
        return common_sample_success();
    default:
        return finish();
    }
}

}  // namespace openswd3::battle
