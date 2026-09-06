#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"

#include <algorithm>
#include <bit>
#include <initializer_list>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryActor = 0x004786E0U;
constexpr u32 kCallActorStatus = 0x0047CE80U;
constexpr u32 kCallFeedback = 0x0047F150U;
constexpr u32 kCallQueryFinalActor = 0x0047F920U;
constexpr u32 kCallPublishRewardId = 0x004787D0U;
constexpr u32 kCallPublishRewardValue = 0x0047D640U;
constexpr u32 kCallPublishRewardMode = 0x0047CEC0U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleEffectCoordinatorGroupABaseToken +
        index * kLegacyBattleEffectCoordinatorGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleEffectCoordinatorGroupBBaseToken +
        index * kLegacyBattleEffectCoordinatorGroupBStride;
}

void replace_low_word(u32& value, const u16 low) noexcept {
    value = (value & 0xFFFF0000U) | static_cast<u32>(low);
}

void replace_high_word(u32& value, const u16 high) noexcept {
    value = (value & 0x0000FFFFU) | (static_cast<u32>(high) << 16U);
}

class Runner final {
public:
    Runner(
        LegacyBattleEffectCoordinatorState& state,
        std::span<LegacyBattleRewardScaleActorState> group_b_reward_scale,
        LegacyBattleStartupState& startup,
        LegacyBattleEffectCallPort& port,
        rendering::LegacyFramebuffer& framebuffer
    )
        : state_(state), group_b_reward_scale_(group_b_reward_scale),
          startup_(startup), port_(port),
          framebuffer_(framebuffer), metrics_(port.actor_metric_state()),
          publications_(port.actor_publication_state()),
          shift_(port.effect_shift_state()) {}

    LegacyBattleEffectCoordinatorResult result{};

    [[nodiscard]] LegacyBattleEffectCallReply invoke(
        const u32 callee,
        const std::initializer_list<u32> arguments = {},
        const u32 eax = 0U,
        const u32 ecx = 0U,
        const u32 edx = 0U
    ) {
        LegacyBattleEffectCallRequest request{};
        request.callee_token = callee;
        request.eax = eax;
        request.ecx = ecx;
        request.edx = edx;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        ++result.port_calls;
        return port_.invoke(request);
    }

    [[nodiscard]] bool validate_group_a(
        const u32 index, const LegacyBattleEffectCoordinatorStatus status
    ) {
        if (index < state_.group_a.size()) {
            return true;
        }
        result.status = status;
        return false;
    }

    [[nodiscard]] bool validate_group_b(
        const u32 index, const LegacyBattleEffectCoordinatorStatus status
    ) {
        if (index < state_.group_b.size()) {
            return true;
        }
        result.status = status;
        return false;
    }

    [[nodiscard]] bool validate_slot(const u32 index) {
        if (index < state_.processed_actor_slots.size()) {
            return true;
        }
        result.status =
            LegacyBattleEffectCoordinatorStatus::workspace_slot_typed_stop;
        return false;
    }

    void publish_actor_slot(const u32 actor_index) {
        publications_.slots[actor_index] = actor_index;
    }

    [[nodiscard]] bool query_actor(const u32 actor_token, i32& actor_index) {
        ++result.actor_query_calls;
        const auto reply = invoke(
            kCallQueryActor, {actor_token}, actor_token, actor_token, 0U
        );
        actor_index = static_cast<i32>(signed_word(reply.eax));
        return true;
    }

    [[nodiscard]] bool actor_status(const u32 actor_token, u32& status_value) {
        ++result.actor_status_calls;
        status_value =
            invoke(
                kCallActorStatus, {actor_token}, actor_token, actor_token, 0U
            )
                .eax;
        return true;
    }

    [[nodiscard]] bool single_effect(
        const u32 actor_token,
        const u32 argument_object_token,
        const u32 argument_mode_gate,
        const u32 source_value,
        const u32 slot,
        u32& return_value
    ) {
        ++result.effect_frame_calls;
        const auto child = advance_legacy_battle_effect_frame(
            state_,
            port_,
            actor_token,
            argument_object_token,
            argument_mode_gate,
            source_value,
            slot,
            {.startup = &startup_}
        );
        result.port_calls += child.port_calls;
        if (child.status != LegacyBattleEffectFrameStatus::completed) {
            result.status =
                LegacyBattleEffectCoordinatorStatus::effect_frame_typed_stop;
            return false;
        }
        return_value = child.return_value;
        return true;
    }

    [[nodiscard]] bool group_effect(
        const u32 actor_token,
        const u32 argument_object_token,
        const u32 argument_mode_gate,
        const u32 source_value,
        const u32 slot,
        const u32 group_wide_mode,
        u32& return_value
    ) {
        ++result.group_effect_frame_calls;
        const auto child = advance_legacy_battle_group_effect_frame(
            state_,
            port_,
            actor_token,
            argument_object_token,
            argument_mode_gate,
            source_value,
            slot,
            group_wide_mode,
            {.startup = &startup_}
        );
        result.port_calls += child.port_calls;
        if (child.status != LegacyBattleGroupEffectFrameStatus::completed) {
            result.status = LegacyBattleEffectCoordinatorStatus::
                group_effect_frame_typed_stop;
            return false;
        }
        return_value = child.return_value;
        return true;
    }

    [[nodiscard]] u32
    feedback(const u32 first, const u32 second, const u32 third) {
        ++result.feedback_calls;
        return invoke(kCallFeedback, {first, second, third}).eax;
    }

    [[nodiscard]] bool copy_actor_value(
        const u32 actor_token,
        const u32 profile_index,
        const u32 destination_index
    ) {
        if (profile_index >= startup_.party.size()) {
            result.status =
                LegacyBattleEffectCoordinatorStatus::group_a_actor_typed_stop;
            return false;
        }
        if (destination_index >= state_.group_b_copy_argument_words.size()) {
            result.status =
                LegacyBattleEffectCoordinatorStatus::group_b_actor_typed_stop;
            return false;
        }
        result.group_a_effect_reward =
            apply_legacy_battle_group_a_effect_rewards(
                &port_.group_a_reward_profile_state(),
                &startup_.party[profile_index]
                     .attribute_aggregation.embedded_profiles,
                &state_.group_b_copy_argument_words[destination_index],
                actor_token,
                kLegacyBattleEffectCoordinatorCopySourceToken,
                kLegacyBattleEffectCoordinatorGroupBCopyBaseToken +
                    destination_index *
                        kLegacyBattleEffectCoordinatorGroupBStride,
                port_
            );
        ++result.group_a_effect_reward_calls;
        result.port_calls += result.group_a_effect_reward.port_calls;
        if (result.group_a_effect_reward.status !=
            LegacyBattleGroupAEffectRewardApplicationStatus::completed) {
            result.status = LegacyBattleEffectCoordinatorStatus::
                group_a_effect_reward_typed_stop;
            return false;
        }
        return true;
    }

    void finalize_pair(const u32 first_actor, const u32 second_actor) {
        result.pair_transition = advance_legacy_battle_pair_transition(
            port_,
            {
                .primary_object_token = first_actor,
                .secondary_object_token = second_actor,
            }
        );
        ++result.pair_transition_calls;
        result.port_calls += result.pair_transition.port_calls;
    }

    [[nodiscard]] u32& pair_primary_value() noexcept {
        return port_.battle_pair_primary_value();
    }

    [[nodiscard]] u16& pair_secondary_value() noexcept {
        return port_.battle_pair_secondary_value();
    }

    [[nodiscard]] u32 query_final_actor(const u32 actor_token) {
        return invoke(kCallQueryFinalActor, {actor_token}, 0U, actor_token, 0U)
            .eax;
    }

    [[nodiscard]] bool publish_reward(
        const u32 actor_index,
        const u32 actor_token,
        const u32 value_token,
        u32& value
    ) {
        auto* actor = actor_index < group_b_reward_scale_.size()
            ? &group_b_reward_scale_[actor_index]
            : nullptr;
        result.reward_scale = scale_legacy_battle_reward(
            actor,
            &value,
            port_,
            {
                .actor_token = actor_token,
                .entry_ecx = actor_token,
            }
        );
        ++result.reward_scale_calls;
        result.port_calls += result.reward_scale.port_calls;
        if (result.reward_scale.status !=
            LegacyBattleRewardScaleStatus::completed) {
            result.status =
                LegacyBattleEffectCoordinatorStatus::reward_scale_typed_stop;
            return false;
        }
        if (result.reward_scale.return_eax == 1U && signed_dword(value) > 0) {
            static_cast<void>(invoke(
                kCallPublishRewardId, {kLegacyBattleEffectCoordinatorRewardId}
            ));
            static_cast<void>(invoke(kCallPublishRewardValue, {value}));
            static_cast<void>(invoke(kCallPublishRewardMode, {1U}));
            static_cast<void>(feedback(0U, value, 0U));
        }
        static_cast<void>(value_token);
        return true;
    }

    [[nodiscard]] bool fill_framebuffer() {
        ++result.framebuffer_fill_calls;
        const auto& geometry = framebuffer_.geometry().surface;
        const u32 width = std::bit_cast<u32>(geometry.width);
        const u32 height = std::bit_cast<u32>(geometry.height);
        const u32 requested = width * height;
        auto pixels = framebuffer_.physical_pixels();
        const std::size_t owned = pixels.size();
        const std::size_t prefix = std::min<std::size_t>(requested, owned);
        std::fill_n(pixels.begin(), prefix, static_cast<u16>(0xFFFFU));
        if (static_cast<std::size_t>(requested) > owned) {
            result.status =
                LegacyBattleEffectCoordinatorStatus::framebuffer_typed_stop;
            return false;
        }
        return true;
    }

    void clear_feedback() noexcept {
        pair_primary_value() = 0U;
        pair_secondary_value() = 0U;
        replace_high_word(shift_.packed_reward, 0U);
    }

    void clear_primary_workspace() noexcept {
        state_.primary.fill({});
    }

    void clear_feedback_arrays() noexcept {
        state_.feedback_primary.fill(0U);
        state_.feedback_secondary.fill(0U);
        state_.feedback_tertiary.fill(0U);
    }

    [[nodiscard]] bool publish_feedback_fill(
        const u32 actor_index,
        u32& render_counter,
        const bool publish_group_a_word,
        const bool publish_group_b_word,
        const bool publish_actor_slot
    ) {
        ++render_counter;
        state_.framebuffer_dirty_latch = 1U;
        if (publish_group_a_word) {
            state_.group_a_feedback_actor = static_cast<u16>(actor_index);
        }
        if (publish_group_b_word) {
            state_.group_b_feedback_actor = static_cast<u16>(actor_index);
        }
        if (publish_actor_slot) {
            if (!validate_slot(actor_index)) {
                return false;
            }
            publications_.slots[actor_index] = actor_index;
        }
        return fill_framebuffer();
    }

    [[nodiscard]] u32 current_argument_mode(
        const bool current_is_group_a, const u32 index
    ) const noexcept {
        return current_is_group_a ? state_.group_a[index].argument_mode_gate
                                  : state_.group_b[index].argument_mode_gate;
    }

    LegacyBattleEffectCoordinatorState& state_;
    std::span<LegacyBattleRewardScaleActorState> group_b_reward_scale_;
    LegacyBattleStartupState& startup_;
    LegacyBattleEffectCallPort& port_;
    rendering::LegacyFramebuffer& framebuffer_;
    LegacyBattleActorMetricState& metrics_;
    LegacyBattleActorPublicationState& publications_;
    LegacyBattleEffectShiftState& shift_;
};

[[nodiscard]] bool reached_signed(const u32 value, const u32 target) noexcept {
    return signed_dword(value) >= signed_dword(target);
}

}  // namespace

LegacyBattleEffectCoordinatorState::LegacyBattleEffectCoordinatorState() {
    processed_actor_slots.fill(0xFFFFFFFFU);
}

LegacyBattleEffectCoordinatorResult advance_legacy_battle_effect_coordinator(
    LegacyBattleEffectCoordinatorState& state,
    std::span<LegacyBattleRewardScaleActorState> group_b_reward_scale,
    LegacyBattleStartupState& startup,
    LegacyBattleEffectCallPort& port,
    rendering::LegacyFramebuffer& framebuffer,
    const u32 ui_state,
    const u32 focus_actor
) {
    Runner run(state, group_b_reward_scale, startup, port, framebuffer);
    auto& result = run.result;
    auto& metrics = port.actor_metric_state();
    auto& shift = port.effect_shift_state();

    if ((ui_state & 0x00008000U) == 0U || (ui_state & 1U) != 0U) {
        result.return_value = 0U;
        return result;
    }

    const u32 current_actor = metrics.priority_actor_index;
    if (current_actor >= 8U) {
        const u32 current_index = current_actor - 8U;
        if (!run.validate_group_a(
                current_index,
                LegacyBattleEffectCoordinatorStatus::
                    current_group_a_actor_typed_stop
            )) {
            return result;
        }
        const u32 current_token = group_a_token(current_index);
        i32 target_signed = 0;
        if (!run.query_actor(current_token, target_signed)) {
            return result;
        }
        const u32 target_index = std::bit_cast<u32>(target_signed);
        if (current_index >= state.group_a_arguments.size()) {
            result.status = LegacyBattleEffectCoordinatorStatus::
                current_group_a_argument_typed_stop;
            return result;
        }
        const u32 source_value = state.group_a_arguments[current_index];
        const u32 argument_mode =
            run.current_argument_mode(true, current_index);

        if (state.group_a_global_gate != 0U) {
            if (state.group_a_effect_mode == 1U) {
                u32 child_return = 0U;
                if (!run.group_effect(
                        group_b_token(target_index),
                        current_token,
                        argument_mode,
                        source_value,
                        0U,
                        1U,
                        child_return
                    )) {
                    return result;
                }
                if (child_return != 1U) {
                    result.return_value = 0U;
                    return result;
                }

                if (metrics.group_a_mode == 0U) {
                    u32 index = 0U;
                    while (signed_dword(index) <
                           signed_dword(metrics.group_b_count)) {
                        if (!run.validate_group_b(
                                index,
                                LegacyBattleEffectCoordinatorStatus::
                                    group_b_actor_typed_stop
                            ) ||
                            !run.validate_slot(index)) {
                            return result;
                        }
                        ++result.group_b_iterations;
                        u32 actor_state = 0U;
                        if (!run.actor_status(
                                group_b_token(index), actor_state
                            )) {
                            return result;
                        }
                        if (actor_state != 1U) {
                            state.processed_actor_slots[index] = index;
                            state.actor_activity_latch = 1U;
                            state.group_activity_latch = 1U;
                            if (state.primary_suppression == 0U &&
                                run.feedback(
                                    state.feedback_primary[index],
                                    state.feedback_secondary[index],
                                    state.feedback_tertiary[index]
                                ) == 1U) {
                                if (!run.publish_feedback_fill(
                                        index,
                                        state.group_a_render_count,
                                        false,
                                        true,
                                        true
                                    )) {
                                    return result;
                                }
                            }
                            run.clear_feedback();
                            ++state.completed_count;
                            state.feedback_primary[index] = 0U;
                            state.feedback_secondary[index] = 0U;
                            state.feedback_tertiary[index] = 0U;
                        }
                        ++index;
                    }
                }
                run.clear_feedback();
                state.completed_count = 0U;
            } else if (metrics.group_a_mode == 0U) {
                if (state.scan_limit == 0U) {
                    result.return_value = 0U;
                    return result;
                }
                u32 index = 0U;
                while (true) {
                    if (!run.validate_group_b(
                            index,
                            LegacyBattleEffectCoordinatorStatus::
                                group_b_actor_typed_stop
                        ) ||
                        !run.validate_slot(index)) {
                        return result;
                    }
                    ++result.group_b_iterations;
                    u32 actor_state = 0U;
                    if (!run.actor_status(group_b_token(index), actor_state)) {
                        return result;
                    }
                    if (actor_state == 1U) {
                        if (static_cast<u32>(state.scan_limit) <
                            metrics.group_b_count) {
                            state.scan_limit =
                                static_cast<u16>(state.scan_limit + 1U);
                            if (signed_dword(state.required_completion_count) >
                                1) {
                                --state.required_completion_count;
                            }
                        }
                        state.scan_delay_counter = 0U;
                    } else {
                        if (static_cast<u32>(state.scan_limit) <
                            metrics.group_b_count) {
                            state.scan_delay_counter =
                                static_cast<u16>(state.scan_delay_counter + 1U);
                            if (state.scan_delay_counter >=
                                state.scan_delay_threshold) {
                                state.scan_limit =
                                    static_cast<u16>(state.scan_limit + 1U);
                                state.scan_delay_counter = 0U;
                            }
                        }
                        u32 child_return = 0U;
                        if (!run.single_effect(
                                group_b_token(index),
                                current_token,
                                argument_mode,
                                source_value,
                                index,
                                child_return
                            )) {
                            return result;
                        }
                        if (child_return == 1U &&
                            state.processed_actor_slots[index] == 0xFFFFFFFFU) {
                            state.processed_actor_slots[index] = index;
                            state.actor_activity_latch = 1U;
                            state.group_activity_latch = 1U;
                            if (state.primary_suppression == 0U &&
                                run.feedback(
                                    state.feedback_primary[index],
                                    state.feedback_secondary[index],
                                    state.feedback_tertiary[index]
                                ) == 1U) {
                                if (!run.publish_feedback_fill(
                                        index,
                                        state.group_a_render_count,
                                        false,
                                        true,
                                        true
                                    )) {
                                    return result;
                                }
                            }
                            run.clear_feedback();
                            ++state.completed_count;
                            state.feedback_primary[index] = 0U;
                            state.feedback_secondary[index] = 0U;
                            state.feedback_tertiary[index] = 0U;
                            if (reached_signed(
                                    state.completed_count,
                                    state.required_completion_count
                                )) {
                                break;
                            }
                        }
                    }
                    ++index;
                    if (index >= static_cast<u32>(state.scan_limit)) {
                        result.return_value = 0U;
                        return result;
                    }
                }
                state.completed_count = 0U;
                state.scan_limit = 1U;
            } else {
                if (state.scan_limit == 0U) {
                    result.return_value = 0U;
                    return result;
                }
                u32 index = 0U;
                while (true) {
                    if (!run.validate_group_a(
                            index,
                            LegacyBattleEffectCoordinatorStatus::
                                group_a_actor_typed_stop
                        ) ||
                        !run.validate_slot(index)) {
                        return result;
                    }
                    ++result.group_a_iterations;
                    if (static_cast<u32>(state.scan_limit) <
                        metrics.group_a_count) {
                        state.scan_delay_counter =
                            static_cast<u16>(state.scan_delay_counter + 1U);
                        if (state.scan_delay_counter >=
                            state.scan_delay_threshold) {
                            state.scan_limit =
                                static_cast<u16>(state.scan_limit + 1U);
                            state.scan_delay_counter = 0U;
                        }
                    }
                    const auto& actor = state.group_a[index];
                    if (actor.guard_ac0 != 1U && actor.guard_ac1 != 1U) {
                        u32 child_return = 0U;
                        if (!run.single_effect(
                                group_a_token(index),
                                current_token,
                                argument_mode,
                                source_value,
                                index,
                                child_return
                            )) {
                            return result;
                        }
                        if (child_return == 1U &&
                            state.processed_actor_slots[index] == 0xFFFFFFFFU) {
                            state.processed_actor_slots[index] = index;
                            state.actor_activity_latch = 1U;
                            state.group_activity_latch = 1U;
                            if (state.primary_suppression == 0U &&
                                run.feedback(
                                    state.feedback_primary[index],
                                    state.feedback_secondary[index],
                                    state.feedback_tertiary[index]
                                ) == 1U) {
                                if (!run.publish_feedback_fill(
                                        index,
                                        state.group_a_render_count,
                                        true,
                                        false,
                                        false
                                    )) {
                                    return result;
                                }
                            }
                            run.clear_feedback();
                            ++state.completed_count;
                            state.feedback_primary[index] = 0U;
                            state.feedback_secondary[index] = 0U;
                            state.feedback_tertiary[index] = 0U;
                            if (reached_signed(
                                    state.completed_count,
                                    state.required_completion_count
                                )) {
                                break;
                            }
                        }
                    }
                    ++index;
                    if (index >= static_cast<u32>(state.scan_limit)) {
                        result.return_value = 0U;
                        return result;
                    }
                }
                state.completed_count = 0U;
                state.scan_limit = 1U;
            }

            run.clear_primary_workspace();
            run.clear_feedback_arrays();
            result.return_value = 1U;
            return result;
        }

        u32 child_return = 0U;
        const bool target_group_b = metrics.group_a_mode == 0U;
        const u32 target_token = target_group_b ? group_b_token(target_index)
                                                : group_a_token(target_index);
        if (state.group_a_effect_mode == 1U) {
            if (!run.group_effect(
                    target_token,
                    current_token,
                    argument_mode,
                    source_value,
                    0U,
                    0U,
                    child_return
                )) {
                return result;
            }
        } else if (!run.single_effect(
                       target_token,
                       current_token,
                       argument_mode,
                       source_value,
                       0U,
                       child_return
                   )) {
            return result;
        }
        if (child_return != 1U) {
            result.return_value = 0U;
            return result;
        }

        i32 queried = 0;
        if (!run.query_actor(current_token, queried)) {
            return result;
        }
        state.queried_actor_word = static_cast<u16>(queried);
        if (target_group_b) {
            replace_high_word(
                state.selected_actor_pair, static_cast<u16>(target_index)
            );
            state.actor_activity_latch = 1U;
            if (!run.validate_slot(target_index)) {
                return result;
            }
            state.processed_actor_slots[target_index] = target_index;
            if (state.group_a_effect_mode != 1U) {
                run.finalize_pair(current_token, target_token);
            }
            if (state.primary_suppression == 0U &&
                run.feedback(
                    run.pair_primary_value(),
                    run.pair_secondary_value(),
                    static_cast<u32>(signed_word(shift.packed_reward >> 16U))
                ) == 1U) {
                if (!run.publish_feedback_fill(
                        target_index,
                        state.group_a_render_count,
                        false,
                        true,
                        true
                    )) {
                    return result;
                }
            }
            run.clear_feedback();
        } else {
            replace_low_word(
                state.selected_actor_pair, static_cast<u16>(target_index)
            );
            if (state.group_a_effect_mode == 1U) {
                static_cast<void>(run.feedback(
                    run.pair_primary_value(),
                    run.pair_secondary_value(),
                    static_cast<u32>(signed_word(shift.packed_reward >> 16U))
                ));
                if (!run.validate_slot(target_index)) {
                    return result;
                }
                state.processed_actor_slots[target_index] = target_index;
            } else {
                if (state.primary_suppression == 0U &&
                    run.feedback(
                        state.feedback_primary[0],
                        run.pair_secondary_value(),
                        static_cast<u32>(
                            signed_word(shift.packed_reward >> 16U)
                        )
                    ) == 1U) {
                    state.group_a_feedback_actor =
                        static_cast<u16>(target_index);
                    if (!run.fill_framebuffer()) {
                        return result;
                    }
                }
                state.feedback_primary[0] = 0U;
            }
            run.clear_feedback();
        }
        run.clear_primary_workspace();
        result.return_value = 1U;
        return result;
    }

    const u32 current_index = current_actor;
    if (!run.validate_group_b(
            current_index,
            LegacyBattleEffectCoordinatorStatus::
                current_group_b_actor_typed_stop
        )) {
        return result;
    }
    const u32 current_token = group_b_token(current_index);
    i32 target_signed = 0;
    if (!run.query_actor(current_token, target_signed)) {
        return result;
    }
    const u32 target_index = std::bit_cast<u32>(target_signed);
    const u32 argument_mode = run.current_argument_mode(false, current_index);

    if (state.group_b_global_gate == 0U) {
        const bool group_effect_mode = state.group_b_effect_mode == 1U;
        const bool target_group_b =
            !group_effect_mode && metrics.group_b_mode != 0U;
        const u32 target_token = target_group_b ? group_b_token(target_index)
                                                : group_a_token(target_index);
        u32 child_return = 0U;
        if (group_effect_mode) {
            if (!run.group_effect(
                    target_token,
                    current_token,
                    argument_mode,
                    state.group_b_argument,
                    0U,
                    0U,
                    child_return
                )) {
                return result;
            }
        } else if (!run.single_effect(
                       target_token,
                       current_token,
                       argument_mode,
                       state.group_b_argument,
                       0U,
                       child_return
                   )) {
            return result;
        }
        if (child_return != 1U) {
            if (group_effect_mode) {
                if (!run.publish_reward(
                        current_index,
                        current_token,
                        0x0052441CU,
                        run.pair_primary_value()
                    )) {
                    return result;
                }
            }
            result.return_value = 0U;
            return result;
        }

        if (!target_group_b &&
            !run.copy_actor_value(target_token, target_index, current_index)) {
            return result;
        }
        i32 queried = 0;
        if (!run.query_actor(current_token, queried)) {
            return result;
        }
        state.queried_actor_word = static_cast<u16>(queried);
        if (target_group_b) {
            replace_high_word(
                state.selected_actor_pair, static_cast<u16>(target_index)
            );
        } else {
            replace_low_word(
                state.selected_actor_pair, static_cast<u16>(target_index)
            );
        }
        state.actor_activity_latch = 1U;

        if (state.primary_suppression == 0U) {
            const u32 feedback_first = group_effect_mode
                ? run.pair_primary_value()
                : state.feedback_primary[0];
            const u32 feedback_second = target_group_b || group_effect_mode
                ? 0U
                : run.pair_secondary_value();
            const u32 feedback_third = target_group_b || group_effect_mode
                ? 0U
                : static_cast<u32>(signed_word(shift.packed_reward >> 16U));
            if (run.feedback(feedback_first, feedback_second, feedback_third) ==
                1U) {
                if (group_effect_mode) {
                    run.pair_primary_value() = 0xFFFFFFFFU;
                }
                if (!target_group_b) {
                    state.framebuffer_dirty_latch = 1U;
                }
                if (target_group_b) {
                    state.group_b_feedback_actor =
                        static_cast<u16>(target_index);
                } else {
                    state.group_a_feedback_actor =
                        static_cast<u16>(target_index);
                    if (!group_effect_mode) {
                        run.publish_actor_slot(target_index);
                    }
                }
                if (!run.fill_framebuffer()) {
                    return result;
                }
            }
        }

        if (!group_effect_mode && !target_group_b) {
            run.finalize_pair(current_token, target_token);
        }
        if (!target_group_b && run.query_final_actor(target_token) == 1U &&
            target_index == focus_actor - 8U) {
            state.focus_release_latch = 0U;
        }
        if (!group_effect_mode && !target_group_b) {
            if (!run.publish_reward(
                    current_index,
                    current_token,
                    0x005242B0U,
                    state.feedback_primary[0]
                )) {
                return result;
            }
        }

        run.clear_feedback();
        state.feedback_primary[0] = 0U;
        run.clear_primary_workspace();
        result.return_value = 1U;
        return result;
    }

    if (state.group_b_effect_mode == 1U) {
        u32 child_return = 0U;
        if (!run.group_effect(
                group_a_token(target_index),
                current_token,
                argument_mode,
                state.group_b_argument,
                0U,
                1U,
                child_return
            )) {
            return result;
        }
        if (child_return != 1U) {
            result.return_value = 0U;
            return result;
        }
        u32 index = 0U;
        if (signed_dword(metrics.group_a_count) <= 0) {
            result.return_value = 0U;
            return result;
        }
        while (true) {
            if (!run.validate_group_a(
                    index,
                    LegacyBattleEffectCoordinatorStatus::
                        group_a_actor_typed_stop
                ) ||
                !run.validate_slot(index)) {
                return result;
            }
            ++result.group_a_iterations;
            u32 actor_state = 0U;
            if (!run.actor_status(group_a_token(index), actor_state)) {
                return result;
            }
            const auto& actor = state.group_a[index];
            if (actor_state != 1U && actor.guard_ac1 != 1U &&
                actor.guard_ac0 != 1U) {
                if (!run.copy_actor_value(
                        group_a_token(index), index, current_index
                    )) {
                    return result;
                }
                if (state.feedback_primary[index] > 0U) {
                    run.pair_primary_value() = state.feedback_primary[index];
                }
                run.finalize_pair(current_token, group_a_token(index));
                state.processed_actor_slots[index] = index;
                state.actor_activity_latch = 1U;
                state.group_activity_latch = 1U;
                if (state.primary_suppression == 0U &&
                    run.feedback(state.feedback_primary[index], 0U, 0U) == 1U) {
                    if (!run.publish_feedback_fill(
                            index,
                            state.group_a_render_count,
                            false,
                            false,
                            false
                        )) {
                        return result;
                    }
                }
                if (!run.publish_reward(
                        current_index,
                        current_token,
                        0x005242B0U + index * 4U,
                        state.feedback_primary[index]
                    )) {
                    return result;
                }
                run.clear_feedback();
                state.feedback_primary[index] = 0U;
                ++state.completed_count;
                if (state.completed_count == state.completion_target_count) {
                    break;
                }
            }
            ++index;
            if (signed_dword(index) >= signed_dword(metrics.group_a_count)) {
                result.return_value = 0U;
                return result;
            }
        }
        state.completed_count = 0U;
        run.clear_primary_workspace();
        state.feedback_primary.fill(0U);
        result.return_value = 1U;
        return result;
    }

    const bool scan_group_b = metrics.group_b_mode != 0U;
    if (state.scan_limit == 0U) {
        result.return_value = 0U;
        return result;
    }
    u32 index = 0U;
    while (true) {
        if (scan_group_b) {
            if (!run.validate_group_b(
                    index,
                    LegacyBattleEffectCoordinatorStatus::
                        group_b_actor_typed_stop
                ) ||
                !run.validate_slot(index)) {
                return result;
            }
            ++result.group_b_iterations;
        } else {
            if (!run.validate_group_a(
                    index,
                    LegacyBattleEffectCoordinatorStatus::
                        group_a_actor_typed_stop
                ) ||
                !run.validate_slot(index)) {
                return result;
            }
            ++result.group_a_iterations;
        }

        const u32 dynamic_count =
            scan_group_b ? metrics.group_b_count : metrics.group_a_count;
        if (static_cast<u32>(state.scan_limit) < dynamic_count) {
            state.scan_delay_counter =
                static_cast<u16>(state.scan_delay_counter + 1U);
            if (state.scan_delay_counter >= state.scan_delay_threshold) {
                state.scan_limit = static_cast<u16>(state.scan_limit + 1U);
                state.scan_delay_counter = 0U;
            }
        }

        const u32 actor_token =
            scan_group_b ? group_b_token(index) : group_a_token(index);
        u32 actor_state = 0U;
        if (!run.actor_status(actor_token, actor_state)) {
            return result;
        }
        bool eligible = actor_state != 1U;
        if (!scan_group_b) {
            const auto& actor = state.group_a[index];
            eligible =
                eligible && actor.guard_ac1 != 1U && actor.guard_ac0 != 1U;
        }
        if (eligible) {
            u32 child_return = 0U;
            if (!run.single_effect(
                    actor_token,
                    current_token,
                    argument_mode,
                    state.group_b_argument,
                    index,
                    child_return
                )) {
                return result;
            }
            if (child_return == 1U &&
                state.processed_actor_slots[index] == 0xFFFFFFFFU) {
                if (!scan_group_b &&
                    !run.copy_actor_value(actor_token, index, current_index)) {
                    return result;
                }
                state.processed_actor_slots[index] = index;
                state.actor_activity_latch = 1U;
                state.group_activity_latch = 1U;
                if (state.primary_suppression == 0U &&
                    run.feedback(state.feedback_primary[index], 0U, 0U) == 1U) {
                    u32& counter = state.group_b_render_count;
                    if (!run.publish_feedback_fill(
                            index, counter, false, false, false
                        )) {
                        return result;
                    }
                }
                if (!scan_group_b) {
                    if (!run.publish_reward(
                            current_index,
                            current_token,
                            0x005242B0U + index * 4U,
                            state.feedback_primary[index]
                        )) {
                        return result;
                    }
                }
                run.clear_feedback();
                state.feedback_primary[index] = 0U;
                ++state.completed_count;
                if (state.completed_count == state.completion_target_count) {
                    break;
                }
            }
        }
        ++index;
        if (index >= static_cast<u32>(state.scan_limit)) {
            result.return_value = 0U;
            return result;
        }
    }

    state.scan_limit = 1U;
    run.clear_primary_workspace();
    state.feedback_primary.fill(0U);
    state.completed_count = 0U;
    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
