#include "openswd3/battle/legacy_battle_menu_input_finalize.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kGroupBBaseToken = 0x00525508U;
inline constexpr u32 kGroupBCodeZeroToken = 0x005229E0U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_index(const u32 actor_code) noexcept {
    return actor_code - 8U;
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kGroupABaseToken + index * kGroupAStride;
}

}  // namespace

LegacyBattleMenuInputFinalizeResult finalize_legacy_battle_menu_input(
    LegacyBattleMenuInputFinalizeBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuInputFinalizeRequest& request
) {
    LegacyBattleMenuInputFinalizeResult result;
    auto& startup = bindings.startup_reset;
    auto& frame = bindings.frame_input_resolution;
    auto& final_actor = bindings.final_actor;
    auto& input = bindings.input_dispatch;
    u32 eax = 1U;
    u32 ecx = input.selected_actor_cleanup_gate;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto invoke = [&](const LegacyBattleInputDispatchCall call) {
        ++result.port_calls;
        const auto reply = port.invoke_input_dispatch({
            .call = call,
            .arguments = {0U},
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };
    const auto clear_animation = [&]() {
        input.selection_animation_frame_a = 0U;
        input.selection_animation_frame_b = 0U;
    };
    const auto call_active_group_a = [&](const bool publish_edx) {
        const u32 code = final_actor.queued_actor_code;
        const u32 index = group_a_index(code);
        eax = index * 0x3EFU;
        if (publish_edx) {
            edx = index * 0xBCDU;
        }
        ecx = group_a_token(index);
        if (index >= kGroupACount) {
            result.status = LegacyBattleMenuInputFinalizeStatus::
                active_group_a_actor_typed_stop;
            return false;
        }
        invoke(
            LegacyBattleInputDispatchCall::
                menu_finalize_reset_active_group_a_actor
        );
        ++result.active_group_a_reset_calls;
        return true;
    };
    const auto call_active_group_a_case_two = [&]() {
        const u32 code = final_actor.queued_actor_code;
        const u32 index = group_a_index(code);
        edx = code;
        eax = index * 0xBCDU;
        ecx = group_a_token(index);
        if (index >= kGroupACount) {
            result.status = LegacyBattleMenuInputFinalizeStatus::
                active_group_a_actor_typed_stop;
            return false;
        }
        invoke(
            LegacyBattleInputDispatchCall::
                menu_finalize_reset_active_group_a_actor
        );
        ++result.active_group_a_reset_calls;
        return true;
    };
    const auto reset_selection_workspace = [&]() {
        input.selection_cache_gate_b = 0U;
        input.selection_workspace[0U] = 0U;
        input.selection_cache_gate_a = 0U;
        input.selection_workspace[1U] = 0U;
        input.selection_cache_gate_c = 0U;
        input.selection_workspace[2U] = 0U;
        input.selection_runtime_gate = 0U;
        input.selection_workspace[3U] = 0U;
        frame.target_selection_gate = 1U;
        input.selection_animation_phase = 5U;
        input.selection_workspace[4U] = 0U;
    };

    final_actor.pre_frame_gate_b = 0U;
    input.mouse_action_gate = 1U;

    if (ecx == 1U) {
        const u32 code = final_actor.published_actor_code;
        bindings.message_state = 0U;
        input.mouse_action_gate = 0U;
        eax = code * 0x159U;
        ecx = kGroupBCodeZeroToken + code * kGroupBStride;
        const u32 index = code - 1U;
        if (index >= kGroupBCount) {
            result.status = LegacyBattleMenuInputFinalizeStatus::
                selected_group_b_actor_typed_stop;
            return finish();
        }
        invoke(LegacyBattleInputDispatchCall::menu_finalize_reset_actor);
        ++result.actor_reset_calls;
        edx = 0U;
        input.selection_cache_gate_b = 0U;
        startup.value_4ff0b0 = edx;
        input.selection_cache_gate_a = 0U;
        startup.value_4ff0b4 = edx;
        input.selected_actor_cleanup_gate = 0U;
        startup.value_53bf22 = 0U;
        frame.selection_actor_code = 0xFFFFFFFFU;
        startup.value_4ff0b8 = edx;
        return finish();
    }

    ecx = bindings.message_state;
    switch (ecx) {
    case 1U:
        eax = 0U;
        bindings.message_state = 0U;
        startup.value_4ff0b0 = eax;
        input.selection_runtime_gate = 0U;
        startup.value_4ff0b4 = eax;
        startup.value_53bf22 = 0U;
        input.mouse_action_gate = 0U;
        clear_animation();
        input.selection_animation_phase = 5U;
        startup.value_4ff0b8 = eax;
        return finish();

    case 2U:
        ecx = 0U;
        edx = final_actor.queued_actor_code;
        input.selection_workspace[0U] = ecx;
        bindings.message_state = eax;
        input.selection_workspace[1U] = ecx;
        frame.list_selection = eax;
        input.selection_workspace[2U] = ecx;
        frame.target_selection_gate = eax;
        input.selection_workspace[3U] = ecx;
        input.selection_cache_gate_b = 0U;
        input.selection_workspace[4U] = ecx;
        input.selection_cache_gate_a = 0U;
        input.selection_cache_gate_c = 0U;
        input.selection_runtime_gate = 0U;
        input.selection_animation_phase = 5U;
        if (!call_active_group_a_case_two()) {
            return finish();
        }
        clear_animation();
        return finish();

    case 3U: {
        if (input.selected_group_a_index == 0xFFFFU) {
            bindings.message_state = eax;
            input.selection_animation_phase = 5U;
            input.selection_runtime_gate = 0U;
        } else {
            frame.transition_value_a = 0U;
            frame.transition_value_b = 0U;
            bindings.message_state = 7U;
        }
        frame.target_action_available = eax;
        input.selected_actor_reset_gate = 0U;
        if (!call_active_group_a(true)) {
            return finish();
        }

        const u32 group_b_count = bindings.metrics.group_b_count;
        eax = group_b_count;
        if (signed_bits(group_b_count) > 0) {
            for (u32 index = 0U;
                 signed_bits(index) < signed_bits(group_b_count);
                 ++index) {
                ecx = kGroupBBaseToken + index * kGroupBStride;
                if (index >= kGroupBCount) {
                    result.status = LegacyBattleMenuInputFinalizeStatus::
                        group_b_actor_typed_stop;
                    return finish();
                }
                invoke(
                    LegacyBattleInputDispatchCall::menu_finalize_reset_actor
                );
                ++result.actor_reset_calls;
                eax = group_b_count;
            }
        }

        for (u32 index = 0U; index < kGroupACount; ++index) {
            ecx = group_a_token(index);
            invoke(LegacyBattleInputDispatchCall::menu_finalize_reset_actor);
            ++result.actor_reset_calls;
            if (index >= frame.target_markers.size()) {
                result.status = LegacyBattleMenuInputFinalizeStatus::
                    group_a_marker_typed_stop;
                return finish();
            }
            frame.target_markers[index] = 0U;
        }

        eax = input.action_kind;
        bool publish_action = true;
        switch (eax) {
        case 2U:
            input.selection_mode_cache = 0U;
            bindings.message_state = eax;
            break;
        case 3U:
            bindings.message_state = 4U;
            break;
        case 27U:
        case 30U:
            bindings.message_state = eax;
            break;
        case 4U:
            bindings.message_state = 8U;
            break;
        default:
            publish_action = false;
            break;
        }

        if (!publish_action) {
            clear_animation();
            eax = input.fallback_action_kind;
            input.action_kind = eax;
            return finish();
        }

        ecx = final_actor.queued_actor_code;
        frame.target_selection_block = 0U;
        input.selection_target_cache = 0U;
        startup.value_53bfd0 = 0U;
        const u32 index = group_a_index(ecx);
        eax = index * 5U;
        if (index >= startup.block_4fe5d4.size()) {
            result.status = LegacyBattleMenuInputFinalizeStatus::
                group_a_selection_cache_typed_stop;
            return finish();
        }
        startup.block_4fe5d4[index] = 0U;
        eax *= 4U;
        clear_animation();
        const u32 attack_index = index * 5U;
        if (attack_index + 2U >= startup.block_520e90.size()) {
            result.status = LegacyBattleMenuInputFinalizeStatus::
                attack_order_cache_typed_stop;
            return finish();
        }
        startup.block_520e90[attack_index + 2U] = 0U;
        startup.block_520e90[attack_index] = 0U;
        return finish();
    }

    case 4U:
        bindings.message_state = eax;
        frame.panel_scroll_b = 0U;
        input.selection_animation_phase = 5U;
        input.selection_cache_gate_b = 0U;
        input.selection_cache_gate_a = 0U;
        input.selection_cache_gate_c = 0U;
        input.selection_runtime_gate = 0U;
        if (!call_active_group_a(true)) {
            return finish();
        }
        clear_animation();
        return finish();

    case 5U:
        bindings.message_state = 4U;
        if (!call_active_group_a(false)) {
            return finish();
        }
        clear_animation();
        return finish();

    case 7U:
        bindings.message_state = 0U;
        input.selection_cache_gate_a = 0U;
        input.selection_cache_gate_b = 0U;
        clear_animation();
        frame.alternate_selection_limit = 2U;
        frame.alternate_selection = eax;
        input.action_kind = eax;
        return finish();

    case 8U:
        bindings.message_state = eax;
        frame.grid_selection = eax;
        frame.narrow_list_selection = eax;
        reset_selection_workspace();
        edx = 0U;
        if (!call_active_group_a(false)) {
            return finish();
        }
        clear_animation();
        return finish();

    case 27U:
        bindings.message_state = eax;
        frame.grid_selection = eax;
        frame.target_selection_gate = eax;
        edx = input.fallback_action_kind;
        eax = 0U;
        input.action_kind = edx;
        input.selection_workspace[0U] = eax;
        frame.panel_scroll_b = 0U;
        input.selection_workspace[1U] = eax;
        input.selection_cache_gate_b = 0U;
        input.selection_workspace[2U] = eax;
        input.selection_cache_gate_a = 0U;
        input.selection_workspace[3U] = eax;
        input.selection_cache_gate_c = 0U;
        input.selection_runtime_gate = 0U;
        input.selection_animation_phase = 5U;
        input.selection_workspace[4U] = eax;
        if (!call_active_group_a(true)) {
            return finish();
        }
        clear_animation();
        return finish();

    case 30U:
        ecx = input.fallback_action_kind;
        bindings.message_state = eax;
        input.action_kind = ecx;
        frame.grid_selection = eax;
        reset_selection_workspace();
        edx = 0U;
        if (!call_active_group_a(false)) {
            return finish();
        }
        clear_animation();
        return finish();

    default:
        clear_animation();
        return finish();
    }
}

}  // namespace openswd3::battle
