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
constexpr u32 kCallResetActor = 0x00478850U;
constexpr u32 kCallQueryActorBlocked = 0x0047D930U;
constexpr u32 kCallQueryActorExcluded = 0x00478B50U;
constexpr u32 kCallQueryTargetBusy = 0x00478690U;
constexpr u32 kCallQueryIdle = 0x004786A0U;
constexpr u32 kCallClearControl = 0x0047C660U;
constexpr u32 kCallPrepareTarget = 0x00478AC0U;
constexpr u32 kCallPrepareSelection = 0x00478B30U;
constexpr u32 kCallPublishSelection = 0x00478A70U;
constexpr u32 kCallQuerySelectionMode = 0x00483820U;
constexpr u32 kCallRandomBounded = 0x00439070U;
constexpr u32 kCallSetActionMode = 0x00478710U;
constexpr u32 kCallPublishStatusMode = 0x0047D860U;
constexpr u32 kCallQuerySpecialAction = 0x0047D880U;
constexpr u32 kCallQueryPhaseMode = 0x0047D8D0U;
constexpr u32 kCallQueryStatusSequence = 0x00480220U;
constexpr u32 kCallLoadActionProfile = 0x00476A80U;
constexpr u32 kCallQueryActionTarget = 0x004786E0U;
constexpr u32 kCallPublishActionStart = 0x0047C690U;
constexpr u32 kCallSelectionClear = 0x00478B20U;
constexpr u32 kCallSelectionComplete = 0x00478B40U;
constexpr u32 kCallResetTarget = 0x00478AE0U;
constexpr u32 kCallQueryCompletionValue = 0x00478370U;
constexpr u32 kCallPublishBattleBit = 0x00483FF0U;
constexpr u32 kCallQueryCompletionEffect = 0x0047F360U;
constexpr u32 kCallPublishCompletionId = 0x004787D0U;
constexpr u32 kCallPublishCompletionSource = 0x00478780U;
constexpr u32 kCallPublishCompletionResource = 0x0047D640U;
constexpr u32 kCallSetCompletionMode = 0x0047CEC0U;
constexpr u32 kCallPrepareCompletionSurface = 0x0047F150U;
constexpr u32 kCallQueryEffect = 0x004786D0U;
constexpr u32 kCallPublishEffectMode = 0x00478B60U;

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

class ActionProfileModePortAdapter final
    : public LegacyBattleGroupBActionProfileModePort {
public:
    ActionProfileModePortAdapter(
        LegacyBattleActionDispatchPort& port,
        LegacyBattleActionDispatchResult& result
    ) noexcept
        : port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleGroupBActionProfileModeLoadReply
    load_action_profile(
        const LegacyBattleGroupBActionProfileModeLoadRequest& request
    ) override {
        ++result_.port_calls;
        LegacyBattleActionCallRequest call{
            .callee_token = kCallLoadActionProfile,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        call.arguments[0U] = request.destination_token;
        call.arguments[1U] = request.profile_id;
        const auto reply = port_.invoke(call);
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = port_.group_b_action_configuration_typed_stop(
                kCallLoadActionProfile
            ),
            .profile_buffer = port_.group_b_action_profile_buffer(),
        };
    }

private:
    LegacyBattleActionDispatchPort& port_;
    LegacyBattleActionDispatchResult& result_;
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
    result.group_a_actor_cleanup_calls +=
        nested.group_a_actor_cleanup_calls;
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
                const auto reply =
                    invoke(port, result, kCallResetActor, {source_token});
                result.return_value = reply.eax;
                return result;
            }
            if (invoke(port, result, kCallQueryTerminal, {source_token}).eax ==
                1U) {
                shared.selection_mode = 0U;
                action.active_effect_gate = 0U;
                shared.action_block_gate = 0U;
                action.action_pending_aux = 0U;
                action.active_effect_target = 0xFFFFFFFFU;
                const auto reply =
                    invoke(port, result, kCallResetActor, {source_token});
                result.return_value = reply.eax;
                return result;
            }

            if (state.phase_mode == 1U) {
                if (shared.action_aux_gate == 0U &&
                    shared.action_block_gate == 0U) {
                    if (shared.action_side != 0U) {
                        shared.target_ready_gate = 1U;
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPublishSelection,
                            {source_token, 0U}
                        ));
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
                                            .eax != 1U &&
                                    invoke(
                                        port,
                                        result,
                                        kCallQueryActorExcluded,
                                        {target}
                                    )
                                            .eax != 1U &&
                                    invoke(
                                        port,
                                        result,
                                        kCallQueryTargetBusy,
                                        {target}
                                    )
                                            .eax == 0U &&
                                    invoke(
                                        port,
                                        result,
                                        kCallQueryIdle,
                                        {source_token}
                                    )
                                            .eax == 0U) {
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallClearControl,
                                        {target, 0U}
                                    ));
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPrepareTarget,
                                        {target}
                                    ));
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
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPrepareSelection,
                                        {source_token}
                                    ));
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPublishSelection,
                                        {source_token, selected}
                                    ));
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
                        opponent = &(*context.startup->group_b_lifecycle)
                            [group_b_index];
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

            if (invoke(port, result, kCallQueryIdle, {source_token}).eax ==
                0U) {
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
                    status_actor = &(*context.startup->group_b_lifecycle)
                        [group_b_index];
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
                    static_cast<void>(invoke(
                        port, result, kCallPublishSelection, {source_token, 0U}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallSetActionMode, {source_token, 0x11U}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallPublishStatusMode, {source_token, 2U}
                    ));
                } else {
                    const bool status_branch = std::bit_cast<i16>(status) < 0 ||
                        (status & 0x6000U) != 0U;
                    if (status_branch) {
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
                                actor = &(*context.startup->group_b_lifecycle)
                                    [group_b_index];
                            }
                            ActionProfileModePortAdapter adapter(port, result);
                            result.group_b_action_profile_mode =
                                compose_legacy_battle_group_b_action_profile_mode(
                                    actor,
                                    adapter,
                                    {
                                        .actor_token = source_token,
                                        .entry_eax = 0x8000U,
                                        .entry_ecx = source_token,
                                        .entry_edx =
                                            stale_special_selection_pending,
                                    }
                                );
                            ++result.group_b_action_profile_mode_calls;
                            if (result.group_b_action_profile_mode.status !=
                                LegacyBattleGroupBActionProfileModeStatus::
                                    completed) {
                                result.status =
                                    LegacyBattleActionDispatchStatus::
                                        group_b_action_profile_mode_typed_stop;
                                result.return_value = result
                                    .group_b_action_profile_mode.return_eax;
                                return result;
                            }
                            state.status_action_value = result
                                .group_b_action_profile_mode.return_eax;
                            action.current_actor_index =
                                static_cast<u16>(group_b_index);
                        }
                        if ((status & 0x4000U) != 0U) {
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallSetActionMode,
                                {source_token, 2U}
                            ));
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
                            }
                        }
                        if ((status & 0x2000U) != 0U) {
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallSetActionMode,
                                {source_token, 6U}
                            ));
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
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {group_b_token(state.random_target_index)}
                                )
                                    .eax != 0U) {
                                goto action_decision_done;
                            }
                            static_cast<void>(invoke(
                                port,
                                result,
                                kCallPublishSelection,
                                {source_token, state.random_target_index}
                            ));
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
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {selected_token}
                                )
                                    .eax == 0U) {
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPublishSelection,
                                    {source_token, state.random_target_index}
                                ));
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPrepareTarget,
                                    {selected_token}
                                ));
                            }
                        }
                    } else {
                        const auto status_sequence = invoke(
                            port,
                            result,
                            kCallQueryStatusSequence,
                            {source_token, 0x0053BD40U}
                        );
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
                                group_b_index *
                                    kLegacyBattleActionGroupBStride;
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
                            actor = &(*context.startup->group_b_lifecycle)
                                [group_b_index];
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
                                if (invoke(
                                        port,
                                        result,
                                        kCallQueryTerminal,
                                        {group_b_token(
                                            state.random_target_index
                                        )}
                                    )
                                        .eax == 0U) {
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPublishSelection,
                                        {source_token,
                                         state.random_target_index}
                                    ));
                                }
                            } else {
                                if (!validate_group_a(
                                        result, state.random_target_index
                                    )) {
                                    return result;
                                }
                                const u32 selected_token =
                                    group_a_token(state.random_target_index);
                                if (invoke(
                                        port,
                                        result,
                                        kCallQueryTerminal,
                                        {selected_token}
                                    )
                                        .eax == 0U) {
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPublishSelection,
                                        {source_token,
                                         state.random_target_index}
                                    ));
                                    static_cast<void>(invoke(
                                        port,
                                        result,
                                        kCallPrepareTarget,
                                        {selected_token}
                                    ));
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
        const auto idle = invoke(port, result, kCallQueryIdle, {source_token});
        if (idle.eax == 1U) {
            stale_ebx = 1U;
            if (action.active_effect_target == group_b_index) {
                shared.action_block_gate = 1U;
                action.action_pending_aux = 1U;
                static_cast<void>(invoke(
                    port, result, kCallPublishActionStart, {source_token}
                ));
            }
            action.current_actor_index = static_cast<u16>(group_b_index);
            const u32 target_index = low_word(
                invoke(port, result, kCallQueryActionTarget, {source_token}).eax
            );
            auto nested = dispatch_legacy_battle_opponent_action(
                action, port, context, group_b_index, target_index
            );
            merge_nested(result, nested);
            if (nested.status != LegacyBattleActionDispatchStatus::completed) {
                return result;
            }
            if (nested.return_value == 1U) {
                u32 completed_target = low_word(
                    invoke(port, result, kCallQueryActionTarget, {source_token})
                        .eax
                );
                static_cast<void>(
                    invoke(port, result, kCallSelectionClear, {source_token})
                );
                if (invoke(port, result, kCallSelectionComplete, {source_token})
                        .eax == 1U) {
                    if (action.group_a_count > 0) {
                        for (i32 index = 0; index < action.group_a_count;
                             ++index) {
                            const u32 uindex = to_bits(index);
                            if (!validate_group_a(result, uindex)) {
                                return result;
                            }
                            const u32 target = group_a_token(uindex);
                            static_cast<void>(
                                invoke(port, result, kCallResetTarget, {target})
                            );
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryQueueCompletion,
                                    {target}
                                )
                                        .eax == 1U &&
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
                                auto& actor = action.group_a_action_execution[
                                    uindex
                                ];
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
                                                ? &party->item_effect_application
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
                                static_cast<void>(invoke(
                                    port, result, kCallResetActor, {target}
                                ));
                                const auto stale = invoke(
                                    port,
                                    result,
                                    kCallQueryCompletionValue,
                                    {target}
                                );
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
                                    (stale.eax & 0xFFFF0000U) | table_value;
                                if (!publish_player_item_quantity(
                                        port, result, argument, 0U
                                    )) {
                                    return result;
                                }
                            }
                            ++result.group_a_iterations;
                        }
                    }
                } else if (shared.action_side != 0U) {
                    if (!validate_group_b(result, completed_target)) {
                        return result;
                    }
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallResetTarget,
                        {group_b_token(completed_target)}
                    ));
                } else {
                    if (!validate_group_a(result, completed_target)) {
                        return result;
                    }
                    const u32 target = group_a_token(completed_target);
                    if (invoke(
                            port, result, kCallQueryQueueCompletion, {target}
                        )
                                .eax == 1U &&
                        state.group_a_completion_words[completed_target] !=
                            0U) {
                        state.group_a_completion_words[completed_target] = 0U;
                        replace_low_word(shared.defeated_actor_packed, 0U);
                        state.group_a_completion_slots[completed_target] = 0U;
                        static_cast<void>(
                            invoke(port, result, kCallResetActor, {target})
                        );
                        const auto stale = invoke(
                            port, result, kCallQueryCompletionValue, {target}
                        );
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
                            (stale.ecx & 0xFFFF0000U) | table_value;
                        if (!publish_player_item_quantity(
                                port, result, argument, 0U
                            )) {
                            return result;
                        }
                    }
                    static_cast<void>(
                        invoke(port, result, kCallResetTarget, {target})
                    );
                }

                static_cast<void>(
                    invoke(port, result, kCallResetActor, {source_token})
                );
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
                if (invoke(
                        port,
                        result,
                        kCallQueryCompletionEffect,
                        {group_a_token(completed_target)}
                    )
                        .eax == 1U) {
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishCompletionId,
                        {source_token, 0x235EU}
                    ));
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishCompletionSource,
                        {source_token}
                    ));
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPublishCompletionResource,
                        {source_token, state.completion_resource_token}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallSetCompletionMode, {source_token, 1U}
                    ));
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

    const bool effect_mode =
        (low_word(invoke(port, result, kCallQueryEffect, {source_token}).eax) !=
             0U &&
         (action.frame_effect.primary_suppression == 1U ||
          action.frame_effect.split_suppression == 1U)) ||
        shared.global_effect_override == 1U;
    static_cast<void>(invoke(
        port,
        result,
        kCallPublishEffectMode,
        {source_token, effect_mode ? 1U : 0U}
    ));

    if (state.pending_effect_ids[group_b_index] != 0xFFFFFFFFU) {
        SingleEffectPortAdapter effect_port(port);
        const auto effect = advance_legacy_battle_single_effect_frame(
            state.pending_effect_frame,
            effect_port,
            source_token,
            state.pending_effect_argument,
            group_b_index
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
