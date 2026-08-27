#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"

#include <algorithm>
#include <bit>
#include <initializer_list>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupAStride = 0x2F34U;
constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x2B28U;
constexpr u32 kSpecialQueryToken = 0x004E80FCU;
constexpr u32 kBattleMusicPathToken = 0x0053C198U;
constexpr u32 kMessageTextToken = 0x004A7838U;
constexpr u32 kTextModeEnabledToken = 0x004A7820U;
constexpr u32 kTextModeDisabledToken = 0x004A782CU;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u32 sign_extend_word(const u32 value) noexcept {
    return std::bit_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(low_word(value)))
    );
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kGroupABaseToken + kGroupAStride * index;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kGroupBBaseToken + kGroupBStride * index;
}

[[nodiscard]] constexpr u32 retarget_group_b_token(const u32 index) noexcept {
    u32 value = index + index * 2U;
    value <<= 3U;
    value -= index;
    value += value * 2U;
    value += value * 4U;
    value = index + value * 4U;
    return kGroupBBaseToken + value * 8U;
}

[[nodiscard]] constexpr u32 retarget_group_a_token(const u32 index) noexcept {
    const u32 relative = index - 8U;
    u32 value = relative << 6U;
    value -= relative;
    value <<= 4U;
    value -= relative;
    value += value * 2U;
    return kGroupABaseToken + value * 4U;
}

[[nodiscard]] constexpr u32
action_block_group_b_token(const u32 index) noexcept {
    u32 value = index + index * 2U;
    value <<= 3U;
    value -= index;
    value += value * 2U;
    const u32 scaled = value + value * 4U;
    value = index + scaled * 4U;
    return kGroupBBaseToken + value * 8U;
}

[[nodiscard]] constexpr u32 toggle_zero_nonzero(const u32 value) noexcept {
    return value == 0U ? 1U : 0U;
}

[[nodiscard]] constexpr u32 toggle_exact_one(const u32 value) noexcept {
    return value == 1U ? 0U : 1U;
}

[[nodiscard]] constexpr u32
signed_increment_modulo_two(const u32 value) noexcept {
    const i32 incremented = std::bit_cast<i32>(value + 1U);
    return std::bit_cast<u32>(incremented % 2);
}

class Runner final {
public:
    Runner(
        LegacyBattleDebugHotkeyBindings bindings,
        LegacyBattleDebugHotkeyPort& port,
        LegacyBattleDebugHotkeyResult& result
    )
        : bindings_(bindings), port_(port), result_(result) {}

    [[nodiscard]] u32
    key(const input_time_rng::LegacyKeyboardSnapshot& keyboard,
        const u32 code) {
        ++result_.raw_key_queries;
        return input_time_rng::read_raw_key(keyboard, code);
    }

    void delay(const u32 milliseconds) {
        port_.delay_milliseconds(milliseconds);
        ++result_.delay_calls;
    }

    [[nodiscard]] LegacyBattleDebugHotkeyCallReply invoke(
        const LegacyBattleDebugHotkeyCall call,
        const u32 object_token = 0U,
        const std::initializer_list<u32> arguments = {}
    ) {
        LegacyBattleDebugHotkeyCallRequest request{};
        request.call = call;
        request.object_token = object_token;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        const auto reply = port_.invoke_debug_hotkey(request);
        ++result_.port_calls;
        if (reply.publish_group_a_count) {
            bindings_.actor_metrics.group_a_count = reply.group_a_count;
        }
        if (reply.publish_group_b_count) {
            bindings_.actor_metrics.group_b_count = reply.group_b_count;
        }
        if (reply.publish_priority_actor) {
            bindings_.actor_metrics.priority_actor_index = reply.priority_actor;
        }
        return reply;
    }

private:
    LegacyBattleDebugHotkeyBindings bindings_;
    LegacyBattleDebugHotkeyPort& port_;
    LegacyBattleDebugHotkeyResult& result_;
};

void display_text(Runner& runner, const u32 text_token) {
    static_cast<void>(runner.invoke(
        LegacyBattleDebugHotkeyCall::display_text,
        0U,
        {0x208U, 10U, 30U, text_token, 2U}
    ));
}

}  // namespace

LegacyBattleDebugHotkeyResult coordinate_legacy_battle_debug_hotkeys(
    const input_time_rng::LegacyKeyboardSnapshot& keyboard,
    LegacyBattleDebugHotkeyState& state,
    LegacyBattleDebugHotkeyBindings bindings,
    LegacyBattleDebugHotkeyPort& port
) {
    LegacyBattleDebugHotkeyResult result;
    Runner runner(bindings, port, result);

    if (state.developer_tools_enabled == 1U) {
        const bool left_control = runner.key(keyboard, 0x1DU) != 0U;
        const bool right_control =
            left_control ? false : runner.key(keyboard, 0x9DU) != 0U;
        result.control_chord_active = left_control || right_control;

        if (result.control_chord_active) {
            if (runner.key(keyboard, 0x3DU) != 0U) {
                static_cast<void>(runner.invoke(
                    LegacyBattleDebugHotkeyCall::suspend_audio_output
                ));
            }

            if (runner.key(keyboard, 0x3BU) != 0U) {
                runner.delay(200U);
                state.toggle_5244e0 = toggle_zero_nonzero(state.toggle_5244e0);
            }

            if (runner.key(keyboard, 0x2DU) != 0U) {
                runner.delay(200U);
                state.toggle_53af68 = toggle_zero_nonzero(state.toggle_53af68);
            }

            if (runner.key(keyboard, 0x25U) != 0U) {
                if (state.message_latch_53ceb8 == 0U) {
                    state.message_latch_53ceb8 = 1U;
                }
                display_text(runner, kMessageTextToken);
            }

            if (runner.key(keyboard, 0x2CU) != 0U) {
                runner.delay(200U);
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    const u32 token = group_a_token(index);
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::reset_group_a_primary,
                        token,
                        {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU}
                    ));
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::reset_group_a_secondary,
                        token,
                        {0xFFFFFFFFU}
                    ));
                    ++index;
                    ++result.group_a_iterations;
                }

                index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::configure_group_a,
                        group_a_token(index),
                        {0x26ACU, 0x9BU, 0xC8U}
                    ));
                    ++index;
                    ++result.group_a_iterations;
                }
            }

            if (runner.key(keyboard, 0x20U) != 0U) {
                runner.delay(200U);
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    if (bindings.actor_frames == nullptr ||
                        index >= bindings.actor_frames->shared.actor_ai_primary
                                     .size()) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            group_a_runtime_typed_stop;
                        return result;
                    }
                    if (bindings.actor_frames->shared.actor_ai_primary[index] !=
                            1U &&
                        bindings.actor_frames->shared
                                .actor_ai_secondary[index] != 1U) {
                        static_cast<void>(runner.invoke(
                            LegacyBattleDebugHotkeyCall::publish_actor_value,
                            group_a_token(index),
                            {80U, 0xFFFFFFF6U, 0xFFFFFFF6U}
                        ));
                    }
                    ++index;
                    ++result.group_a_iterations;
                }
            }

            if (runner.key(keyboard, 0x21U) != 0U) {
                runner.delay(100U);
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    if (bindings.actor_frames == nullptr ||
                        index >= bindings.actor_frames->shared.actor_ai_primary
                                     .size()) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            group_a_runtime_typed_stop;
                        return result;
                    }
                    if (bindings.actor_frames->shared.actor_ai_primary[index] !=
                            1U &&
                        bindings.actor_frames->shared
                                .actor_ai_secondary[index] != 1U) {
                        static_cast<void>(runner.invoke(
                            LegacyBattleDebugHotkeyCall::publish_actor_value,
                            group_a_token(index),
                            {500U, 0xFFFFFFFBU, 0xFFFFFFFBU}
                        ));
                    }
                    ++index;
                    ++result.group_a_iterations;
                }
            }

            if (runner.key(keyboard, 0x2FU) != 0U) {
                runner.delay(200U);
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_b_count
                               )) {
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::publish_actor_value,
                        group_b_token(index),
                        {10U, 0U, 0U}
                    ));
                    ++index;
                    ++result.group_b_iterations;
                }
            }

            if (runner.key(keyboard, 0x43U) != 0U) {
                runner.delay(200U);
                bindings.player_control.speed_mode =
                    signed_increment_modulo_two(
                        bindings.player_control.speed_mode
                    );
            }

            if (runner.key(keyboard, 0x12U) != 0U) {
                result.return_value = 0U;
                result.early_return_zero = true;
                return result;
            }

            if (runner.key(keyboard, 0x2EU) != 0U) {
                state.selection_status_word_53c050 =
                    (state.selection_status_word_53c050 & 0xFFFF0000U) |
                    static_cast<u16>(state.selection_status_word_53c050 | 1U);
                bindings.final_actor.selection_gate = 0U;
                bindings.actor_metrics.priority_actor_index = 0U;
                state.selection_workspace_tail.fill(0U);
                bindings.final_actor.frame_gate_b = 0U;
                bindings.final_actor.frame_gate_a = 0U;
                if (bindings.actor_frames != nullptr) {
                    bindings.actor_frames->shared.action.action_pending_aux =
                        0U;
                    port.outcome_resolution_state().resolution_latch = 0U;
                }
                bindings.actor_metrics.priority_actor_index = 0xFFFFFFFFU;

                u32 current_index = 0xFFFFFFFFU;
                if (state.actor_retarget_gate_53bf64 == 1U) {
                    state.actor_retarget_gate_53bf64 = 0U;
                    const auto query = runner.invoke(
                        LegacyBattleDebugHotkeyCall::query_special_index,
                        kSpecialQueryToken
                    );
                    current_index = sign_extend_word(query.eax);
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::reset_special_group_b,
                        retarget_group_b_token(current_index)
                    ));
                    current_index = bindings.actor_metrics.priority_actor_index;
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::reset_actor,
                        retarget_group_a_token(current_index)
                    ));
                    current_index = bindings.actor_metrics.priority_actor_index;
                }

                if (bindings.actor_frames == nullptr) {
                    result.status = LegacyBattleDebugHotkeyStatus::
                        actor_frame_state_typed_stop;
                    return result;
                }
                if (bindings.actor_frames->shared.action_block_gate == 1U) {
                    bindings.actor_frames->shared.action_block_gate = 0U;
                    static_cast<void>(runner.invoke(
                        LegacyBattleDebugHotkeyCall::reset_actor,
                        action_block_group_b_token(current_index)
                    ));
                }
            }

            if (runner.key(keyboard, 0x3FU) != 0U) {
                static_cast<void>(runner.invoke(
                    LegacyBattleDebugHotkeyCall::suspend_audio_output
                ));
                static_cast<void>(runner.invoke(
                    LegacyBattleDebugHotkeyCall::restart_battle_music,
                    0U,
                    {kBattleMusicPathToken, 0U}
                ));
            }

            if (runner.key(keyboard, 0x3CU) != 0U) {
                runner.delay(200U);
                u32 low = state.battle_mode_flags_53bc24 & 0xFFU;
                u32 text_token = kTextModeEnabledToken;
                if (state.text_mode_toggle_53c02c == 1U) {
                    low &= 0xFDU;
                    state.text_mode_toggle_53c02c = 0U;
                    text_token = kTextModeDisabledToken;
                } else {
                    low |= 2U;
                    state.text_mode_toggle_53c02c = 1U;
                }
                state.battle_mode_flags_53bc24 =
                    (state.battle_mode_flags_53bc24 & 0xFFFFFF00U) | low;
                display_text(runner, text_token);
            }

            if (runner.key(keyboard, 0x11U) != 0U) {
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_b_count
                               )) {
                    const u32 token = group_b_token(index);
                    const auto actor = runner.invoke(
                        LegacyBattleDebugHotkeyCall::query_actor_status, token
                    );
                    if (actor.eax != 1U) {
                        if (index >= bindings.actor_publication.slots.size() ||
                            index >=
                                bindings.startup.reset.block_5242b0.size()) {
                            result.status = LegacyBattleDebugHotkeyStatus::
                                group_b_publication_typed_stop;
                            return result;
                        }
                        bindings.actor_publication.slots[index] = index;
                        bindings.startup.reset.block_5242b0[index] = 0U;
                        static_cast<void>(runner.invoke(
                            LegacyBattleDebugHotkeyCall::publish_actor_value,
                            token,
                            {30000U, 0U, 0U}
                        ));
                    }
                    ++index;
                    ++result.group_b_iterations;
                }

                bindings.effect_coordinator.group_a_render_count =
                    std::bit_cast<u32>(bindings.actor_metrics.group_b_count);
                if (bindings.actor_frames == nullptr) {
                    result.status = LegacyBattleDebugHotkeyStatus::
                        actor_frame_state_typed_stop;
                    return result;
                }
                bindings.actor_frames->shared.target_ready_gate = 1U;
                bindings.final_actor.frame_gate_b = 1U;
                bindings.final_actor.frame_gate_a = 1U;
                bindings.actor_frames->shared.action.action_pending_aux = 1U;
                port.outcome_resolution_state().resolution_latch = 1U;
                bindings.final_actor.actor_order.fill(0U);
                state.block_53af30.fill(0U);
                for (auto& record : bindings.startup.reset.records_524788) {
                    record = {};
                    record.value_00 = 0xFFFFFFFFU;
                }
                bindings.effect_coordinator.group_a_feedback_actor = 0xFFFFU;
                bindings.effect_coordinator.completed_count = 0U;
                state.actor_retarget_gate_53bf64 = 0U;
                bindings.actor_frames->shared.selection_aux_gate = 0U;
                state.reset_gate_53bd50 = 0U;
                bindings.final_actor.queued_actor_code = 0U;
                bindings.actor_metrics.priority_actor_index = 0xFFFFFFFFU;
                bindings.message_state = 0U;
                result.full_reset_applied = true;
            }
        }
    }

    if (runner.key(keyboard, 0x23U) != 0U) {
        u32 index = 0U;
        while (index <
               std::bit_cast<u32>(bindings.actor_metrics.group_a_count)) {
            static_cast<void>(runner.invoke(
                LegacyBattleDebugHotkeyCall::adjust_actor,
                group_a_token(index),
                {10U, 0U}
            ));
            ++index;
            ++result.actor_adjust_iterations;
        }
        index = 0U;
        while (index <
               std::bit_cast<u32>(bindings.actor_metrics.group_b_count)) {
            static_cast<void>(runner.invoke(
                LegacyBattleDebugHotkeyCall::adjust_actor,
                group_b_token(index),
                {10U, 0U}
            ));
            ++index;
            ++result.actor_adjust_iterations;
        }
        bindings.effect_shift.actor_delta = 10;
    }

    if (runner.key(keyboard, 0x24U) != 0U) {
        u32 index = 0U;
        while (index <
               std::bit_cast<u32>(bindings.actor_metrics.group_a_count)) {
            static_cast<void>(runner.invoke(
                LegacyBattleDebugHotkeyCall::adjust_actor,
                group_a_token(index),
                {0xFFFFFFF6U, 0U}
            ));
            ++index;
            ++result.actor_adjust_iterations;
        }
        index = 0U;
        while (index <
               std::bit_cast<u32>(bindings.actor_metrics.group_b_count)) {
            static_cast<void>(runner.invoke(
                LegacyBattleDebugHotkeyCall::adjust_actor,
                group_b_token(index),
                {0xFFFFFFF6U, 0U}
            ));
            ++index;
            ++result.actor_adjust_iterations;
        }
        bindings.effect_shift.actor_delta = -10;
    }

    if (runner.key(keyboard, 0x19U) != 0U) {
        state.screenshot_request = toggle_exact_one(state.screenshot_request);
    }

    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
