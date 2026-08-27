#include "openswd3/battle/legacy_battle_target_selection_entry.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kGroupBBaseToken = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;
inline constexpr u32 kSelectionTextToken = 0x0053C16CU;
inline constexpr u32 kSelectionPrimaryOutputToken = 0x0053BD44U;
inline constexpr u32 kSelectionActorOutputAToken = 0x0053BF4AU;
inline constexpr u32 kSelectionActorOutputBToken = 0x0053BF4EU;
inline constexpr u32 kSelectionSample = 0x2DU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 signed_word(const u16 value) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(value));
}

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFFFF00U) | (low & 0xFFU);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kGroupABaseToken + index * kGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index_bits) noexcept {
    return kGroupBBaseToken + index_bits * kGroupBStride;
}

}  // namespace

LegacyBattleTargetSelectionEntryResult enter_legacy_battle_target_selection(
    LegacyBattleTargetSelectionEntryBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleTargetSelectionEntryRequest& request
) {
    LegacyBattleTargetSelectionEntryResult result;
    auto& frame = bindings.frame_input_resolution;
    auto& final_actor = bindings.final_actor;
    auto& action = bindings.action;
    auto& input = bindings.input_dispatch;
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto invoke = [&](const LegacyBattleInputDispatchCall call,
                            const std::array<u32, 5>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_input_dispatch({
            .call = call,
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
    const auto refresh_action = [&]() {
        bindings.message_state = 1U;
        input.action_kind = 1U;
        invoke(LegacyBattleInputDispatchCall::refresh_action_mode);
    };
    const auto refresh_state = [&]() {
        const auto nested = refresh_legacy_battle_target_selection(
            {
                .startup_reset = bindings.startup_reset,
                .startup_supplemental_count_word =
                    bindings.startup_supplemental_count_word,
                .startup_mirror_mode = bindings.startup_mirror_mode,
                .frame_input_resolution = frame,
                .final_actor = final_actor,
                .action = action,
                .metrics = bindings.metrics,
                .debug_hotkeys = bindings.debug_hotkeys,
                .input_dispatch = input,
                .runtime = bindings.target_selection_runtime,
                .target_ready_gate = bindings.target_ready_gate,
                .message_state = bindings.message_state,
            },
            port,
            {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
        );
        ++result.target_selection_refresh_calls;
        result.port_calls += nested.port_calls;
        eax = nested.return_eax;
        ecx = nested.return_ecx;
        edx = nested.return_edx;
        if (nested.status !=
            LegacyBattleTargetSelectionRefreshStatus::completed) {
            result.status = LegacyBattleTargetSelectionEntryStatus::
                target_selection_refresh_typed_stop;
        }
    };
    const auto compute_group_a = [&](const u32 actor_code) {
        const u32 index = actor_code - 8U;
        eax = index * 0xBCDU;
        ecx = group_a_token(index);
        return index;
    };
    const auto compute_group_b_primary = [&]() {
        const i32 index = signed_word(input.selected_group_b_index);
        const u32 bits = std::bit_cast<u32>(index);
        eax = bits * 0x159U;
        ecx = group_b_token(bits);
        return index;
    };
    const auto compute_group_b_secondary = [&]() {
        const i32 index = signed_word(input.selected_group_b_index);
        const u32 bits = std::bit_cast<u32>(index);
        eax = bits * 0x565U;
        edx = bits * 0x159U;
        ecx = group_b_token(bits);
        return index;
    };
    const auto valid_group_b = [](const i32 index) {
        return index >= 0 && static_cast<u32>(index) < kGroupBCount;
    };

    if (input.retreat_block_word != 0U) {
        return finish();
    }

    eax = input.input_gate;
    if (eax == 1U || bindings.outcome_darkening_gate == 1U ||
        (action.message_gate & 0x80000000U) != 0U) {
        return finish();
    }

    eax = replace_low_byte(eax, frame.target_selection_suppression);
    ecx = bindings.message_state;
    if (frame.target_selection_suppression == 0U) {
        edx = bindings.metrics.group_b_count;
        eax = static_cast<u32>(
            static_cast<compat::u8>(action.packed_actor_counter)
        );
        edx -= eax;
        if (signed_bits(edx) <= 1 && input.retreat_target_word != 0xFFFFU &&
            ecx != 99U) {
            bindings.message_state = 0U;
            return finish();
        }
    }

    if (ecx == 110U) {
        eax = replace_low_word(eax, input.target_transition_word);
        if (static_cast<u16>(eax) < 30U) {
            input.target_transition_word = 29U;
            return finish();
        }
        if (static_cast<u16>(eax) == 30U) {
            input.target_transition_word = 100U;
            return finish();
        }
    }

    eax = bindings.dialogs.messages.empty() ? 0U : 1U;
    if (eax != 0U) {
        bindings.one_shot_interaction_state = 1U;
        return finish();
    }

    if (bindings.target_ready_gate != 1U) {
        refresh_state();
        return finish();
    }

    eax = final_actor.queued_actor_code;
    if (eax == 0U || (ecx != 0U && input.selected_option_word == 0xFFFFU)) {
        refresh_state();
        return finish();
    }

    input.selected_option_word = 0xFFFFU;
    u32 group_a_index = compute_group_a(eax);
    if (group_a_index >= kGroupACount) {
        result.status = LegacyBattleTargetSelectionEntryStatus::
            active_group_a_actor_typed_stop;
        return finish();
    }
    invoke(LegacyBattleInputDispatchCall::query_active_actor);
    if (eax == 1U) {
        return finish();
    }

    ecx = std::bit_cast<u32>(input.sample_mix_level);
    const auto sample = port.play_input_sample(
        kSelectionSample, input.sample_mix_level, eax, ecx, edx
    );
    ++result.port_calls;
    ++result.sample_calls;
    eax = sample.eax;
    ecx = sample.ecx;
    edx = sample.edx;

    const u32 configured_actor_code = final_actor.queued_actor_code;
    edx = configured_actor_code;
    input.mouse_action_gate = 1U;
    frame.target_selection_gate = 1U;
    input.selection_animation_phase = 5U;
    group_a_index = compute_group_a(configured_actor_code);
    if (group_a_index >= kGroupACount) {
        result.status = LegacyBattleTargetSelectionEntryStatus::
            active_group_a_actor_typed_stop;
        return finish();
    }
    const auto configured = invoke(
        LegacyBattleInputDispatchCall::target_selection_configure_actor,
        {kSelectionActorOutputAToken, kSelectionActorOutputBToken}
    );
    input.selection_actor_origin_x = configured.output_word_a;
    input.selection_actor_origin_y = configured.output_word_b;

    if (input.selected_group_b_index == 0xFFFFU) {
        refresh_action();
        return finish();
    }
    ecx = final_actor.queued_actor_code;
    edx = std::bit_cast<u32>(signed_word(input.selected_group_a_index));
    ecx -= 8U;
    if (edx != ecx) {
        refresh_action();
        return finish();
    }

    bindings.message_state = 7U;
    frame.alternate_selection_limit = 2U;
    for (u32 scan = 0U; scan < 3U; ++scan) {
        const i32 selected = compute_group_b_primary();
        if (!valid_group_b(selected)) {
            result.status = LegacyBattleTargetSelectionEntryStatus::
                selected_group_b_actor_typed_stop;
            return finish();
        }
        invoke(
            LegacyBattleInputDispatchCall::target_selection_scan_primary,
            {scan, kSelectionTextToken, kSelectionPrimaryOutputToken}
        );
        ++result.primary_scan_calls;
        if (eax == 1U) {
            ++frame.alternate_selection_limit;
        }
    }
    for (u32 scan = 0U; scan < 2U; ++scan) {
        const i32 selected = compute_group_b_secondary();
        if (!valid_group_b(selected)) {
            result.status = LegacyBattleTargetSelectionEntryStatus::
                selected_group_b_actor_typed_stop;
            return finish();
        }
        invoke(
            LegacyBattleInputDispatchCall::target_selection_scan_secondary,
            {scan, kSelectionTextToken}
        );
        ++result.secondary_scan_calls;
        if (eax == 1U) {
            ++frame.alternate_selection_limit;
        }
    }
    frame.transition_value_a = 0U;
    return finish();
}

}  // namespace openswd3::battle
