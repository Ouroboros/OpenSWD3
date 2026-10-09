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
constexpr u32 kBattleMusicPathToken = 0x0053C198U;
constexpr u32 kMessageTextToken = 0x004A7838U;
constexpr u32 kTextModeEnabledToken = 0x004A7820U;
constexpr u32 kTextModeDisabledToken = 0x004A782CU;

// LST .data:004A7820, 004A782C and 004A7838. Keep the original encoded bytes.
constexpr std::array<compat::u8, 9> kTextModeEnabledBytes{
    0xB5U, 0xB4U, 0xB9U, 0xEFU, 0xC6U, 0x46U, 0xABU, 0xB4U, 0U
};
constexpr std::array<compat::u8, 9> kTextModeDisabledBytes{
    0xA5U, 0xBFU, 0xB1U, 0x60U, 0xC6U, 0x46U, 0xABU, 0xB4U, 0U
};
constexpr std::array<compat::u8, 7> kMessageBytes{
    0xB1U, 0x6AU, 0xA7U, 0xF0U, 0xC0U, 0xBBU, 0U
};

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

class DebugTextMessageAdapter final : public LegacyBattleTextMessagePort {
public:
    DebugTextMessageAdapter(
        LegacyBattleDebugHotkeyPort& port, LegacyBattleDebugHotkeyResult& result
    )
        : port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleTextMessageCallReply invoke_text_message(
        const LegacyBattleTextMessageCallRequest& request
    ) override {
        const auto reply = port_.invoke_debug_hotkey({
            .call = request.call == LegacyBattleTextMessageCall::allocate
                ? LegacyBattleDebugHotkeyCall::text_message_allocate
                : LegacyBattleDebugHotkeyCall::text_message_measure,
            .arguments = {request.argument},
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        if (reply.typed_stop) {
            result_.status =
                LegacyBattleDebugHotkeyStatus::port_call_typed_stop;
            result_.stopped_call =
                request.call == LegacyBattleTextMessageCall::allocate
                ? LegacyBattleDebugHotkeyCall::text_message_allocate
                : LegacyBattleDebugHotkeyCall::text_message_measure;
        }

        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .call_failed = reply.typed_stop
        };
    }

private:
    LegacyBattleDebugHotkeyPort& port_;
    LegacyBattleDebugHotkeyResult& result_;
};

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

    [[nodiscard]] bool display_text(const u32 text_token) {
        DebugTextMessageAdapter text_port(port_, result_);
        result_.text_messages.push_back(enqueue_legacy_battle_text_message(
            bindings_.startup.text_messages,
            bindings_.startup.reset.block_5214f8[0U],
            text_port,
            {
                .value_04 = 0x208U,
                .value_08 = 10U,
                .kind = 30U,
                .text_token = text_token,
                .flags = 2U,
            }
        ));
        ++result_.text_message_calls;
        const auto& message = result_.text_messages.back();
        if (message.status != LegacyBattleTextMessageStatus::completed) {
            if (result_.status == LegacyBattleDebugHotkeyStatus::completed) {
                result_.status =
                    LegacyBattleDebugHotkeyStatus::text_message_typed_stop;
            }

            return false;
        }
        return true;
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
        if (reply.publish_group_a_count) {
            bindings_.actor_metrics.group_a_count = reply.group_a_count;
        }
        if (reply.publish_group_b_count) {
            bindings_.actor_metrics.group_b_count = reply.group_b_count;
        }
        if (reply.publish_priority_actor) {
            bindings_.actor_metrics.priority_actor_index = reply.priority_actor;
        }

        if (reply.typed_stop) {
            result_.status =
                LegacyBattleDebugHotkeyStatus::port_call_typed_stop;
            result_.stopped_call = call;
            result_.stopped_object_token = object_token;
        }

        return reply;
    }

private:
    LegacyBattleDebugHotkeyBindings bindings_;
    LegacyBattleDebugHotkeyPort& port_;
    LegacyBattleDebugHotkeyResult& result_;
};

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
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
        .overflow = ((left ^ right) & (left ^ difference) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] bool adjust_actor_group(
    const LegacyBattleDebugHotkeyBindings& bindings,
    LegacyBattleDebugHotkeyResult& result,
    const LegacyBattleDebugHotkeyRequest& request,
    const u32& count_source,
    const u32 base_token,
    const u32 stride,
    const u32 x_delta,
    u32& edx
) noexcept {
    u32 count = count_source;
    u32 index{};
    u32 actor_token = base_token;
    LegacyBattleActorCoordinateFlags flags = subtract_flags(count, 0U);
    while (index < count) {
        result.actor_coordinate_adjustment =
            adjust_legacy_battle_actor_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    {
                        .action = &bindings.action,
                        .startup = &bindings.startup,
                    },
                    actor_token
                ),
                {
                    .actor_token = actor_token,
                    .x_delta = x_delta,
                    .y_delta = 0U,
                    .entry_eax = count,
                    .entry_edx = edx,
                    .entry_flags = flags,
                    .x_argument_readable =
                        request.actor_adjustment_x_argument_readable,
                    .y_argument_readable =
                        request.actor_adjustment_y_argument_readable,
                }
            );
        ++result.actor_coordinate_adjustment_calls;
        if (result.actor_coordinate_adjustment.status !=
            LegacyBattleActorCoordinateAdjustmentStatus::completed) {
            result.status = LegacyBattleDebugHotkeyStatus::
                actor_coordinate_adjustment_typed_stop;
            return false;
        }
        edx = result.actor_coordinate_adjustment.return_edx;
        ++result.actor_adjust_iterations;
        count = count_source;
        ++index;
        actor_token += stride;
        flags = subtract_flags(index, count);
    }
    return true;
}

}  // namespace

LegacyBattleDebugHotkeyCallReply invoke_legacy_battle_debug_group_a_record_call(
    LegacyBattleGroupAConfigurationState& configuration,
    const LegacyBattleActorProgressState& progress,
    const LegacyBattleGroupAActionExecutionState& action,
    LegacyBattleDebugRecordPort& records,
    const LegacyBattleDebugHotkeyCallRequest& request
) {
    LegacyBattleDebugHotkeyCallReply reply{
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
        .typed_stop = true
    };
    const auto read = [&](const u32 token,
                          const std::size_t offset,
                          const std::size_t width,
                          u32& value) {
        const auto bytes = records.debug_record_bytes(token);
        if (offset > bytes.size() || width > bytes.size() - offset) {
            return false;
        }

        value = 0U;
        for (std::size_t i = 0U; i < width; ++i) {
            value |= std::to_integer<u32>(bytes[offset + i]) << (i * 8U);
        }

        return true;
    };
    const auto write = [&](const u32 token,
                           const std::size_t offset,
                           const std::size_t width,
                           const u32 value) {
        auto bytes = records.debug_record_bytes(token);
        if (offset > bytes.size() || width > bytes.size() - offset) {
            return false;
        }

        for (std::size_t i = 0U; i < width; ++i) {
            bytes[offset + i] = static_cast<std::byte>(value >> (i * 8U));
        }

        return true;
    };
    using Call = LegacyBattleDebugHotkeyCall;
    if (request.call == Call::reset_group_a_secondary) {
        if (action.action_twenty_seven_motion_mode != 1U &&
            progress.scene_identity != 1U) {
            if (request.arguments[0U] != 0xFFFFFFFFU) {
                return reply;
            }

            if (!write(configuration.auxiliary_record_token, 0U, 4U, 56U)) {
                return reply;
            }

            u32 current{};
            if (!read(configuration.auxiliary_record_token, 0U, 4U, current)) {
                return reply;
            }

            // With EDI=-1 both the signed >=56 and <=0 exits return here.
        }

        reply.typed_stop = false;
        return reply;
    }

    if (request.call != Call::reset_group_a_primary &&
        request.call != Call::configure_group_a) {
        return reply;
    }

    if (!configuration.source_runtime_value_read_accessible) {
        return reply;
    }

    const bool party = configuration.source_runtime_value == 1U;
    if ((!party && configuration.profile_token == 0U) ||
        (party && request.call == Call::reset_group_a_primary &&
         configuration.actor_record_token == 0U)) {
        reply.typed_stop = false;
        return reply;
    }

    if (request.call == Call::reset_group_a_primary && !party) {
        const u32 value = sign_extend_word(request.arguments[0U]);
        if (!write(configuration.profile_token, 0x4CU, 4U, value) ||
            !write(configuration.profile_token, 0x64U, 2U, value)) {
            return reply;
        }
    } else {
        constexpr std::array<std::size_t, 3> source_offsets{
            0x26U, 0x28U, 0x16U
        };
        for (std::size_t index = 0U; index < 3U; ++index) {
            const u16 value = low_word(request.arguments[index]);
            if (request.call == Call::reset_group_a_primary) {
                const std::size_t current = 4U + index * 2U;
                const std::size_t maximum = 10U + index * 2U;
                if (std::bit_cast<i16>(value) > 0 &&
                    (!write(
                         configuration.actor_record_token, maximum, 2U, value
                     ) ||
                     !write(
                         configuration.actor_record_token, current, 2U, value
                     ))) {
                    return reply;
                }

                if (value == 0xFFFFU) {
                    u32 restored{};
                    // Each -1 branch retains its loaded record reference
                    // across the max read and current write.
                    const u32 token = configuration.actor_record_token;
                    if (!read(token, maximum, 2U, restored) ||
                        !write(token, current, 2U, restored)) {
                        return reply;
                    }
                }
            } else if (std::bit_cast<i16>(value) > 0) {
                const u32 token = party ? configuration.source_record_token
                                        : configuration.profile_token;
                const std::size_t offset =
                    party ? source_offsets[index] : 0x56U + index * 2U;
                if (!write(token, offset, 2U, value)) {
                    return reply;
                }
            }
        }
    }

    reply.typed_stop = false;
    return reply;
}

std::span<const compat::u8>
legacy_battle_debug_text_bytes(const u32 token) noexcept {
    switch (token) {
    case kTextModeEnabledToken:
        return kTextModeEnabledBytes;

    case kTextModeDisabledToken:
        return kTextModeDisabledBytes;

    case kMessageTextToken:
        return kMessageBytes;

    default:
        return {};
    }
}

LegacyBattleDebugHotkeyResult coordinate_legacy_battle_debug_hotkeys(
    const input_time_rng::LegacyKeyboardSnapshot& keyboard,
    LegacyBattleDebugHotkeyState& state,
    LegacyBattleDebugHotkeyBindings bindings,
    LegacyBattleDebugHotkeyPort& port,
    const LegacyBattleDebugHotkeyRequest& request
) {
    LegacyBattleDebugHotkeyResult result;
    result.actor_coordinate_registers_known =
        request.actor_adjustment_entry_edx_known;
    Runner runner(bindings, port, result);
    const auto reset_actor = [&](const u32 actor_token,
                                 const u32 call_address,
                                 const u32 return_address,
                                 const u32 entry_eax = 0U,
                                 const u32 entry_edx = 0U) {
        if (execute_legacy_battle_actor_runtime_reset_call(
                {.action = &bindings.action, .startup = &bindings.startup},
                bindings.bounded_random,
                result.actor_runtime_reset,
                request.actor_runtime_reset_requests,
                actor_token,
                entry_eax,
                entry_edx,
                call_address,
                return_address
            )) {
            return true;
        }

        result.status =
            LegacyBattleDebugHotkeyStatus::actor_runtime_reset_typed_stop;
        result.return_value = result.actor_runtime_reset.last.return_eax;
        return false;
    };

    const u32& developer_tools_enabled =
        bindings.developer_tools_enabled != nullptr
        ? *bindings.developer_tools_enabled
        : state.developer_tools_enabled;
    if (developer_tools_enabled == 1U) {
        const bool left_control = runner.key(keyboard, 0x1DU) != 0U;
        const bool right_control =
            left_control ? false : runner.key(keyboard, 0x9DU) != 0U;
        result.control_chord_active = left_control || right_control;

        if (result.control_chord_active) {
            if (runner.key(keyboard, 0x3DU) != 0U) {
                if (runner
                        .invoke(
                            LegacyBattleDebugHotkeyCall::suspend_audio_output
                        )
                        .typed_stop) {
                    return result;
                }
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
                if (!runner.display_text(kMessageTextToken)) {
                    return result;
                }
            }

            if (runner.key(keyboard, 0x2CU) != 0U) {
                runner.delay(200U);
                u32 index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    const u32 token = group_a_token(index);
                    if (runner
                            .invoke(
                                LegacyBattleDebugHotkeyCall::
                                    reset_group_a_primary,
                                token,
                                {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU}
                            )
                            .typed_stop) {
                        return result;
                    }

                    if (runner
                            .invoke(
                                LegacyBattleDebugHotkeyCall::
                                    reset_group_a_secondary,
                                token,
                                {0xFFFFFFFFU}
                            )
                            .typed_stop) {
                        return result;
                    }

                    ++index;
                    ++result.group_a_iterations;
                }

                index = 0U;
                while (index < std::bit_cast<u32>(
                                   bindings.actor_metrics.group_a_count
                               )) {
                    if (runner
                            .invoke(
                                LegacyBattleDebugHotkeyCall::configure_group_a,
                                group_a_token(index),
                                {0x26ACU, 0x9BU, 0xC8U}
                            )
                            .typed_stop) {
                        return result;
                    }

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
                    if (index >= bindings.startup.party.size() ||
                        index >=
                            bindings.action.group_a_action_execution.size()) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            group_a_runtime_typed_stop;
                        return result;
                    }

                    if (bindings.action.group_a_action_execution[index]
                                .action_twenty_seven_motion_mode != 1U &&
                        bindings.startup.party[index].progress.scene_identity !=
                            1U) {
                        if (runner
                                .invoke(
                                    LegacyBattleDebugHotkeyCall::
                                        publish_actor_value,
                                    group_a_token(index),
                                    {80U, 0xFFFFFFF6U, 0xFFFFFFF6U}
                                )
                                .typed_stop) {
                            return result;
                        }
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
                    if (index >= bindings.startup.party.size() ||
                        index >=
                            bindings.action.group_a_action_execution.size()) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            group_a_runtime_typed_stop;
                        return result;
                    }

                    if (bindings.action.group_a_action_execution[index]
                                .action_twenty_seven_motion_mode != 1U &&
                        bindings.startup.party[index].progress.scene_identity !=
                            1U) {
                        if (runner
                                .invoke(
                                    LegacyBattleDebugHotkeyCall::
                                        publish_actor_value,
                                    group_a_token(index),
                                    {500U, 0xFFFFFFFBU, 0xFFFFFFFBU}
                                )
                                .typed_stop) {
                            return result;
                        }
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
                    if (runner
                            .invoke(
                                LegacyBattleDebugHotkeyCall::
                                    publish_actor_value,
                                group_b_token(index),
                                {10U, 0U, 0U}
                            )
                            .typed_stop) {
                        return result;
                    }

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
                bindings.action.action_pending_aux = 0U;
                bindings.action.selection_cache_gate_b = 0U;
                bindings.actor_metrics.priority_actor_index = 0xFFFFFFFFU;

                u32 current_index = 0xFFFFFFFFU;
                if (state.actor_retarget_gate_53bf64 == 1U) {
                    state.actor_retarget_gate_53bf64 = 0U;
                    auto action_target_request =
                        request.special_action_target_request;
                    action_target_request.actor_token =
                        kLegacyBattleDebugSpecialActorToken;
                    action_target_request.entry_eax = 1U;
                    action_target_request.entry_return_address = 0x0045DBE8U;
                    action_target_request.entry_flags =
                        subtract_flags(kGroupABaseToken, 0x0001A8D4U);
                    action_target_request.entry_flags_known = true;
                    result.actor_action_target =
                        query_legacy_battle_actor_action_target(
                            resolve_legacy_battle_actor_action_target(
                                {.action = &bindings.action,
                                 .startup = &bindings.startup,
                                 .debug_hotkeys = &state},
                                kLegacyBattleDebugSpecialActorToken
                            ),
                            action_target_request
                        );
                    ++result.actor_action_target_calls;
                    if (result.actor_action_target.status !=
                        LegacyBattleActorActionTargetStatus::completed) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            actor_action_target_typed_stop;
                        result.return_value =
                            result.actor_action_target.return_eax;
                        return result;
                    }
                    current_index =
                        sign_extend_word(result.actor_action_target.return_eax);
                    const u32 times_three = current_index + current_index * 2U;
                    const u32 times_twenty_four = times_three << 3U;
                    const u32 times_twenty_three =
                        times_twenty_four - current_index;
                    const u32 times_sixty_nine =
                        times_twenty_three + times_twenty_three * 2U;
                    const u32 times_three_hundred_forty_five =
                        times_sixty_nine + times_sixty_nine * 4U;
                    const u32 times_one_thousand_three_hundred_eighty_one =
                        current_index + times_three_hundred_forty_five * 4U;
                    if (!execute_legacy_battle_actor_gate_decay_call(
                            result.actor_gate_decay,
                            request.actor_gate_decay_requests,
                            {.action = &bindings.action,
                             .startup = &bindings.startup},
                            0x0045DC03U,
                            0x0045DC08U,
                            retarget_group_b_token(current_index),
                            times_one_thousand_three_hundred_eighty_one,
                            times_three_hundred_forty_five,
                            subtract_flags(times_twenty_four, current_index)
                        )) {
                        result.status = LegacyBattleDebugHotkeyStatus::
                            actor_gate_decay_typed_stop;
                        result.return_value =
                            result.actor_gate_decay.last.return_eax;
                        return result;
                    }
                    current_index = bindings.actor_metrics.priority_actor_index;
                    const u32 relative_index = current_index - 8U;
                    u32 runtime_eax = relative_index << 6U;
                    runtime_eax -= relative_index;
                    runtime_eax <<= 4U;
                    runtime_eax -= relative_index;
                    runtime_eax += runtime_eax * 2U;
                    if (!reset_actor(
                            retarget_group_a_token(current_index),
                            0x0045DC27U,
                            0x0045DC2CU,
                            runtime_eax,
                            current_index
                        )) {
                        return result;
                    }
                    current_index = bindings.actor_metrics.priority_actor_index;
                }

                if (bindings.actor_frames == nullptr) {
                    result.status = LegacyBattleDebugHotkeyStatus::
                        actor_frame_state_typed_stop;
                    return result;
                }
                if (bindings.actor_frames->shared.action_block_gate == 1U) {
                    bindings.actor_frames->shared.action_block_gate = 0U;
                    if (!reset_actor(
                            action_block_group_b_token(current_index),
                            0x0045DC58U,
                            0x0045DC5DU
                        )) {
                        return result;
                    }
                }
            }

            if (runner.key(keyboard, 0x3FU) != 0U) {
                if (runner
                        .invoke(
                            LegacyBattleDebugHotkeyCall::suspend_audio_output
                        )
                        .typed_stop) {
                    return result;
                }

                if (runner
                        .invoke(
                            LegacyBattleDebugHotkeyCall::restart_battle_music,
                            0U,
                            {kBattleMusicPathToken, 0U}
                        )
                        .typed_stop) {
                    return result;
                }
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
                if (!runner.display_text(text_token)) {
                    return result;
                }
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
                    if (actor.typed_stop) {
                        return result;
                    }

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
                        if (runner
                                .invoke(
                                    LegacyBattleDebugHotkeyCall::
                                        publish_actor_value,
                                    token,
                                    {30000U, 0U, 0U}
                                )
                                .typed_stop) {
                            return result;
                        }
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
                bindings.action.action_pending_aux = 1U;
                bindings.action.selection_cache_gate_b = 1U;
                bindings.final_actor.actor_order.fill(0U);
                std::fill_n(
                    bindings.action.opponent_workspace.begin(), 10U, 0U
                );
                for (auto& record : bindings.startup.reset.records_524788) {
                    record = {};
                    record.value_00 = 0xFFFFFFFFU;
                }
                bindings.effect_coordinator.group_a_feedback_actor = 0xFFFFU;
                bindings.effect_coordinator.completed_count = 0U;
                state.actor_retarget_gate_53bf64 = 0U;
                bindings.action.resolution_latch = 0U;
                state.committed_actor_code = 0U;
                bindings.final_actor.queued_actor_code = 0U;
                bindings.actor_metrics.priority_actor_index = 0xFFFFFFFFU;
                bindings.message_state = 0U;
                result.full_reset_applied = true;
            }
        }

        u32 actor_adjustment_edx = request.actor_adjustment_entry_edx;
        if (runner.key(keyboard, 0x23U) != 0U) {
            if (!adjust_actor_group(
                    bindings,
                    result,
                    request,
                    bindings.actor_metrics.group_a_count,
                    kGroupABaseToken,
                    kGroupAStride,
                    10U,
                    actor_adjustment_edx
                ) ||
                !adjust_actor_group(
                    bindings,
                    result,
                    request,
                    bindings.actor_metrics.group_b_count,
                    kGroupBBaseToken,
                    kGroupBStride,
                    10U,
                    actor_adjustment_edx
                )) {
                return result;
            }
            bindings.effect_shift.actor_delta = 10;
        }

        if (runner.key(keyboard, 0x24U) != 0U) {
            if (!adjust_actor_group(
                    bindings,
                    result,
                    request,
                    bindings.actor_metrics.group_a_count,
                    kGroupABaseToken,
                    kGroupAStride,
                    0xFFFFFFF6U,
                    actor_adjustment_edx
                ) ||
                !adjust_actor_group(
                    bindings,
                    result,
                    request,
                    bindings.actor_metrics.group_b_count,
                    kGroupBBaseToken,
                    kGroupBStride,
                    0xFFFFFFF6U,
                    actor_adjustment_edx
                )) {
                return result;
            }
            bindings.effect_shift.actor_delta = -10;
        }
    }

    if (runner.key(keyboard, 0x19U) != 0U) {
        state.screenshot_request = toggle_exact_one(state.screenshot_request);
    }

    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
