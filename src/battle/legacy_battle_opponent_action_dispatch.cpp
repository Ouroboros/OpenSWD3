#include "openswd3/battle/legacy_battle_opponent_action_dispatch.hpp"

#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_execution.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <bit>
#include <cstring>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryAction = 0x004786B0U;
constexpr u32 kCallCommitVisual = 0x0047F150U;
constexpr u32 kCallSetDelay = 0x00478710U;
constexpr u32 kCallQuerySelection = 0x0047C680U;
constexpr u32 kCallQuerySpecial = 0x0047D8E0U;
constexpr u32 kCallAllocate = 0x00489E90U;
constexpr u32 kCallDelete = 0x00489D00U;
constexpr u32 kCallPrepareTargetPhase = 0x00484020U;
constexpr u32 kCallFinishTargetPhase = 0x004841B0U;
constexpr u32 kCallSetTargetMode = 0x004787F0U;
constexpr u32 kCallClearMode = 0x0047D870U;
constexpr u32 kCallTargetComplete = 0x00479850U;
constexpr u32 kCallActionTen = 0x0047F3C0U;
constexpr u32 kCallQueryModeB = 0x0047C950U;
constexpr u32 kCallQueryModeC = 0x0047C6B0U;
constexpr u32 kCallPushMode = 0x0047D830U;
constexpr u32 kCallPopMode = 0x0047D810U;
constexpr u32 kCallFinalizeMode = 0x0047D860U;
constexpr u32 kCallQuerySeventeen = 0x004763D0U;
constexpr u32 kCallInitializeOpponentWaves = 0x00476900U;
constexpr u32 kCallResetOpponent = 0x0047D350U;
constexpr u32 kCallMirrorOpponent = 0x0047F900U;
constexpr u32 kCallPrepareOpponentScratch = 0x00476DB0U;
constexpr u32 kCallUpdateOpponentScratch = 0x00478220U;
constexpr u32 kCallLoadOpponentProfile = 0x00476A80U;
constexpr u32 kCallQueryOpponentCondition = 0x0047CE80U;
constexpr u32 kCallActionTwoHundred = 0x00482310U;
constexpr u32 kCallActionThreeHundred = 0x00482840U;

constexpr u32 kOpponentScratchToken = 0x005246E0U;
constexpr u32 kOpponentRecordBaseToken = 0x005213A0U;
constexpr u32 kOpponentRecordStride = 0x20U;
[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u8 low_byte(const u32 value) noexcept {
    return static_cast<u8>(value);
}

constexpr void replace_low_word(u32& target, const u16 value) noexcept {
    target = (target & 0xFFFF0000U) | static_cast<u32>(value);
}

constexpr void replace_low_byte(u32& target, const u8 value) noexcept {
    target = (target & 0xFFFFFF00U) | static_cast<u32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] constexpr u32 opponent_record_token(const u32 index) noexcept {
    return kOpponentRecordBaseToken + index * kOpponentRecordStride;
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
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 callee,
    const std::array<u32, 8>& arguments = {},
    const u32 eax = 0U,
    const u32 ecx = 0U,
    const u32 edx = 0U
) {
    ++result.port_calls;
    LegacyBattleActionCallReply reply = port.invoke({
        .callee_token = callee,
        .arguments = arguments,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    if (reply.publish_accumulator) {
        port.battle_pair_primary_value() = reply.accumulator;
    }
    if (reply.publish_selection_word) {
        state.selection_word = reply.selection_word;
    }
    if (reply.publish_selection_high_word) {
        state.selection_high_word = reply.selection_high_word;
    }
    if (reply.publish_opponent_special_action) {
        state.opponent_special_action = reply.opponent_special_action;
    }
    if (reply.publish_opponent_spawn_count) {
        state.opponent_spawn_count = reply.opponent_spawn_count;
    }
    return reply;
}

class OpponentGroupBActionConfigurationPort final
    : public LegacyBattleGroupBActionConfigurationPort {
public:
    OpponentGroupBActionConfigurationPort(
        LegacyBattleActionDispatchState& state,
        LegacyBattleActionDispatchPort& port,
        LegacyBattleActionDispatchResult& result
    ) noexcept
        : state_(state), port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleGroupBActionConfigurationCallReply invoke(
        const LegacyBattleGroupBActionConfigurationCallRequest& request
    ) override {
        u32 callee = kCallPrepareOpponentScratch;
        switch (request.call) {
        case LegacyBattleGroupBActionConfigurationCall::
            load_resource_definition:
            break;

        case LegacyBattleGroupBActionConfigurationCall::load_action_profile:
            callee = kCallLoadOpponentProfile;
            break;

        case LegacyBattleGroupBActionConfigurationCall::release_resource_text:
            callee = kCallUpdateOpponentScratch;
            break;
        }
        const auto reply = ::openswd3::battle::invoke(
            state_,
            port_,
            result_,
            callee,
            {request.arguments[0U], request.arguments[1U]},
            request.eax,
            request.ecx,
            request.edx
        );
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = port_.group_b_action_configuration_typed_stop(callee),
            .resource_bytes = callee == kCallPrepareOpponentScratch
                ? port_.group_b_action_resource_bytes()
                : nullptr,
            .profile_buffer = callee == kCallLoadOpponentProfile
                ? port_.group_b_action_profile_buffer()
                : nullptr,
        };
    }

private:
    LegacyBattleActionDispatchState& state_;
    LegacyBattleActionDispatchPort& port_;
    LegacyBattleActionDispatchResult& result_;
};

[[nodiscard]] bool remove_attack_order_entry(
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 value
) {
    result.attack_order_remove = remove_legacy_battle_attack_order_entry(
        {
            .records = context.attack_order_records,
            .adjacent_intensity_record = context.attack_order_adjacent_record,
        },
        value
    );
    ++result.attack_order_remove_calls;
    if (result.attack_order_remove.status !=
        LegacyBattleAttackOrderRemoveStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::attack_order_remove_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] bool clear_framebuffer(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result
) noexcept {
    const u32 width = static_cast<u32>(context.raster.surface.width);
    const u32 height = static_cast<u32>(context.raster.surface.height);
    const u32 requested_pixels = width * height;
    state.frame_refresh_pending = 1U;
    auto pixels = context.framebuffer.physical_pixels();
    const std::size_t prefix =
        std::min<std::size_t>(requested_pixels, pixels.size());
    std::fill_n(pixels.begin(), prefix, 0xFFFFU);
    ++result.framebuffer_clear_calls;
    if (requested_pixels > pixels.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::framebuffer_typed_stop;
        return false;
    }
    return true;
}

void clear_selection_state(
    LegacyBattleActionDispatchState& state, LegacyBattleActionDispatchPort& port
) noexcept {
    port.battle_pair_primary_value() = 0U;
    state.selection_word = 0U;
    state.selection_high_word = 0U;
}

[[nodiscard]] bool advance_battle_stages(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    const auto metrics = rebuild_legacy_battle_actor_metrics(
        port,
        std::bit_cast<u32>(state.group_b_count),
        std::bit_cast<u32>(state.group_a_count)
    );
    result.port_calls += metrics.port_calls;
    state.group_b_count =
        std::bit_cast<i32>(port.actor_metric_state().group_b_count);
    state.group_a_count =
        std::bit_cast<i32>(port.actor_metric_state().group_a_count);
    if (metrics.status != LegacyBattleActorMetricStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_metric_typed_stop;
        return false;
    }
    auto& metric_state = port.actor_metric_state();
    const auto order = rebuild_legacy_battle_actor_order(
        metric_state,
        metric_state.group_b_count,
        metric_state.group_a_count,
        metric_state.entry_edx
    );
    if (order.status != LegacyBattleActorOrderStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_order_typed_stop;
        return false;
    }
    const auto group_b_order =
        rebuild_legacy_battle_group_b_order(metric_state);
    if (group_b_order.status != LegacyBattleGroupBOrderStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::group_b_order_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] LegacyBattleActionDispatchResult
completed(LegacyBattleActionDispatchResult result, const u32 value) noexcept {
    result.return_value = value;
    return result;
}

}  // namespace

LegacyBattleActionDispatchResult dispatch_legacy_battle_opponent_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const u32 group_b_index,
    const u32 target_index
) {
    LegacyBattleActionDispatchResult result;
    if (!validate_group_b(result, group_b_index)) {
        return result;
    }
    const u32 source_token = group_b_token(group_b_index);
    const auto action_reply =
        invoke(state, port, result, kCallQueryAction, {source_token});
    const u16 action = low_word(action_reply.eax);
    result.action_code = action;

    if (action > 100U) {
        if (action == 200U) {
            if (!validate_group_a(result, target_index)) {
                return result;
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallActionTwoHundred,
                {source_token, group_a_token(target_index)}
            ));
            return result;
        }
        if (action == 300U) {
            if (!validate_group_a(result, target_index)) {
                return result;
            }
            const auto reply = invoke(
                state,
                port,
                result,
                kCallActionThreeHundred,
                {source_token, group_a_token(target_index), 0xFFFFFFFFU, 0U}
            );
            if (reply.eax == 1U) {
                state.current_actor_index = 0xFFFFU;
                return completed(result, 1U);
            }
        }
        return result;
    }
    if (action == 100U) {
        return completed(result, 1U);
    }

    switch (action) {
    case 1U: {
        if (state.side_mode == 0U) {
            if (!validate_group_a(result, target_index)) {
                return result;
            }
            const u32 target_token = group_a_token(target_index);
            auto* const actor =
                context.startup == nullptr ||
                    context.startup->group_b_lifecycle == nullptr
                ? nullptr
                : &(*context.startup->group_b_lifecycle)[group_b_index];
            const auto execution =
                advance_legacy_battle_group_b_action_execution(
                    actor,
                    state.group_a_action_shared,
                    state,
                    port,
                    context,
                    {
                        .actor_token = source_token,
                        .target_token = target_token,
                        .entry_eax = target_index * 0x0BCDU,
                        .entry_edx = action_reply.edx,
                    }
                );
            ++result.group_b_action_execution_calls;
            result.port_calls += execution.port_calls;
            if (execution.status !=
                LegacyBattleGroupBActionExecutionStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    group_b_action_execution_typed_stop;
                return result;
            }
            if (execution.return_eax != 1U) {
                return result;
            }
            state.action_pending = 1U;
            state.action_pending_aux = 0U;
            replace_low_word(
                state.packed_action_state, static_cast<u16>(target_index)
            );
            result.pair_transition = advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = source_token,
                    .secondary_object_token = target_token,
                }
            );
            ++result.pair_transition_calls;
            result.port_calls += result.pair_transition.port_calls;
            if (state.blocking_effect == 0U &&
                invoke(
                    state,
                    port,
                    result,
                    kCallCommitVisual,
                    {target_token,
                     port.battle_pair_primary_value(),
                     state.selection_word,
                     state.selection_high_word}
                )
                        .eax == 1U) {
                state.selected_target_index = static_cast<u16>(target_index);
                if (!clear_framebuffer(state, context, result)) {
                    return result;
                }
            }
            clear_selection_state(state, port);
            state.current_actor_index = 0xFFFFU;
            static_cast<void>(
                invoke(state, port, result, kCallSetDelay, {source_token, 300U})
            );
            return result;
        }

        if (!validate_group_b(result, target_index)) {
            return result;
        }
        const u32 target_token = group_b_token(target_index);
        auto* const actor =
            context.startup == nullptr ||
                context.startup->group_b_lifecycle == nullptr
            ? nullptr
            : &(*context.startup->group_b_lifecycle)[group_b_index];
        const auto execution = advance_legacy_battle_group_b_action_execution(
            actor,
            state.group_a_action_shared,
            state,
            port,
            context,
            {
                .actor_token = source_token,
                .target_token = target_token,
                .entry_eax = target_index * 0x0565U,
                .entry_edx = action_reply.edx,
            }
        );
        ++result.group_b_action_execution_calls;
        result.port_calls += execution.port_calls;
        if (execution.status !=
            LegacyBattleGroupBActionExecutionStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                group_b_action_execution_typed_stop;
            return result;
        }
        if (execution.return_eax != 1U) {
            return result;
        }
        state.action_pending = 1U;
        state.action_pending_aux = 0U;
        replace_low_word(
            state.packed_action_state, static_cast<u16>(target_index)
        );
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {target_token, port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            if (target_index >= state.group_a_to_actor.size()) {
                result.status =
                    LegacyBattleActionDispatchStatus::actor_map_typed_stop;
                return result;
            }
            state.group_a_to_actor[target_index] = target_index;
            state.selected_target_index = static_cast<u16>(target_index);
            if (!clear_framebuffer(state, context, result)) {
                return result;
            }
        }
        port.battle_pair_primary_value() = 0U;
        state.current_actor_index = 0xFFFFU;
        return completed(result, 1U);
    }

    case 2U: {
        if ((state.action_runtime_flags & 0x8000U) == 0U) {
            replace_low_word(
                state.action_runtime_flags,
                static_cast<u16>(state.action_runtime_flags | 0x8000U)
            );
            state.active_actor_snapshot = low_word(
                invoke(state, port, result, kCallQuerySelection, {source_token})
                    .eax
            );
            state.target_identity.fill(0xFFFFFFFFU);
            replace_low_word(state.input_mode, 1U);
            state.selection_workspace.fill(0U);
            if (invoke(state, port, result, kCallQuerySpecial, {source_token})
                    .eax == 1U) {
                state.deformation_active = true;
                const u32 owner =
                    invoke(state, port, result, kCallAllocate, {0x2CU}).eax;
                state.deformation_owner_token = owner;
                if (owner != 0U) {
                    try {
                        state.deformation = std::make_unique<
                            asset_runtime::LegacyDeformationNode>(
                            asset_runtime::LegacyDeformationConfiguration{
                                .framebuffer_width = 640U,
                                .framebuffer_height = 480U,
                                .origin_x = 0,
                                .origin_y = 0,
                                .field_width = 200U,
                                .field_height = 200U,
                            }
                        );
                    } catch (...) {
                        static_cast<void>(
                            invoke(state, port, result, kCallDelete, {owner})
                        );
                        state.deformation_owner_token = 0U;
                        throw;
                    }
                }
            }
        }
        if ((state.action_runtime_flags & 1U) == 0U) {
            return result;
        }
        clear_selection_state(state, port);
        if (state.deformation_active && state.deformation) {
            state.deformation.reset();
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallDelete,
                {state.deformation_owner_token}
            ));
            state.deformation_owner_token = 0U;
        }
        state.deformation_active = false;
        state.frame_effect.fade_active = 1U;
        return completed(result, 1U);
    }

    case 6U: {
        if (low_word(state.phase_counter) == 0U) {
            if (!validate_group_a(result, target_index)) {
                return result;
            }
            const u32 target_token = group_a_token(target_index);
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPrepareTargetPhase,
                {source_token, target_index, target_token}
            ));
            static_cast<void>(invoke(
                state, port, result, kCallSetTargetMode, {target_token, 1U}
            ));
            static_cast<void>(
                invoke(state, port, result, kCallClearMode, {target_token, 1U})
            );
            if (!remove_attack_order_entry(
                    context, result, target_index + 8U
                )) {
                return result;
            }
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            state.frame_effect.primary_suppression = 1U;
            const auto refresh = refresh_legacy_battle_frame(
                port,
                std::bit_cast<u16>(state.frame_effect.red_factor),
                std::bit_cast<u16>(state.frame_effect.green_factor),
                std::bit_cast<u16>(state.frame_effect.blue_factor)
            );
            result.port_calls += refresh.port_calls;
            replace_low_word(state.phase_counter, 1U);
            replace_low_word(state.input_mode, 1U);
        }
        if (invoke(
                state,
                port,
                result,
                kCallFinishTargetPhase,
                {source_token, target_index}
            )
                .eax != 1U) {
            return result;
        }
        replace_low_word(state.input_mode, 1U);
        replace_low_word(state.phase_counter, 0U);
        state.frame_effect.primary_suppression = 0U;
        state.current_actor_index = 0xFFFFU;
        state.frame_effect.fade_active = 1U;
        return completed(result, 1U);
    }

    case 7U: {
        if (!validate_group_a(result, target_index)) {
            return result;
        }
        const u32 target_token = group_a_token(target_index);
        if (invoke(state, port, result, kCallTargetComplete, {target_token})
                .eax != 1U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, result, kCallSetDelay, {target_token, 0U})
        );
        const u32 stage = target_index + 4U;
        if (!remove_attack_order_entry(context, result, stage)) {
            return result;
        }
        if (target_index <= 3U) {
            replace_low_byte(
                state.packed_actor_counter,
                static_cast<u8>(state.packed_actor_counter + 1U)
            );
        }
        if (state.active_target_code == stage) {
            state.active_target_code = 0U;
        }
        if (state.active_effect_target == stage) {
            state.active_effect_target = 0xFFFFFFFFU;
            state.active_effect_gate = 0U;
        }
        return completed(result, 1U);
    }

    case 10U:
        static_cast<void>(
            invoke(state, port, result, kCallActionTen, {source_token, 0U})
        );
        return result;

    case 11U:
        if (invoke(
                state,
                port,
                result,
                kCallQueryModeB,
                {source_token, source_token}
            )
                .eax != 1U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, result, kCallPushMode, {source_token, 0x1000U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallClearMode, {source_token, 0U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallFinalizeMode, {source_token, 8U})
        );
        state.overlay_gate = 1U;
        return completed(result, 1U);

    case 12U:
        if (invoke(state, port, result, kCallQueryModeC, {source_token}).eax !=
            1U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, result, kCallClearMode, {source_token, 1U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallPopMode, {source_token, 0x1000U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallFinalizeMode, {source_token, 8U})
        );
        state.overlay_gate = 1U;
        if (static_cast<i32>(low_byte(state.opponent_processed_counter)) >=
            state.group_b_count) {
            port.battle_terminal_latch() = 0U;
            port.battle_message_state() = 0x63U;
        }
        return completed(result, 1U);

    case 15U: {
        if ((state.phase_counter & 0x8000U) == 0U) {
            replace_low_word(state.phase_counter, 0x8019U);
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallInitializeOpponentWaves,
                {source_token}
            ));
            OpponentGroupBActionConfigurationPort group_b_configuration(
                state, port, result
            );
            u32 wave = 0U;
            while (wave < state.opponent_spawn_count) {
                const u32 record_index = to_bits(state.group_b_count);
                if (!validate_group_b(result, record_index)) {
                    return result;
                }
                const u32 opponent_token = group_b_token(record_index);
                static_cast<void>(invoke(
                    state, port, result, kCallResetOpponent, {opponent_token}
                ));
                state.opponent_scratch.fill(std::byte{0});
                if (context.startup == nullptr) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_action_configuration_typed_stop;
                    return result;
                }
                if (context.startup->group_b_lifecycle == nullptr) {
                    context.startup->group_b_lifecycle =
                        std::make_shared<std::array<
                            LegacyBattleActorGroupBElementState,
                            kLegacyBattleActorGroupBElementCount>>();
                }
                auto& element =
                    (*context.startup->group_b_lifecycle)[record_index];
                element.object_token = opponent_token;
                if (element.resource_token == 0U) {
                    element.resource_token =
                        kLegacyBattleActorGroupBResourceStateBaseToken +
                        record_index * 0xA4U;
                }
                auto& record = element.action_record;
                record.action_id = state.opponent_special_action;
                record.position_x = 0xF0U;
                record.position_y = wave == 1U ? 0x15EU : 0xDCU;
                record.runtime_value = 0U;
                if (state.mirror_group_b_spawn == 1U) {
                    static_cast<void>(invoke(
                        state,
                        port,
                        result,
                        kCallMirrorOpponent,
                        {opponent_token, 1U}
                    ));
                    record.position_x =
                        static_cast<u16>(0x280U - record.position_x);
                }
                const u32 stale_edx = ((record_index * 0x20U) & 0xFFFF0000U) |
                    state.opponent_special_action;
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallPrepareOpponentScratch,
                    {kOpponentScratchToken, stale_edx}
                ));
                const auto update = invoke(
                    state,
                    port,
                    result,
                    kCallUpdateOpponentScratch,
                    {kOpponentScratchToken}
                );
                const u32 stale_eax =
                    (update.eax & 0xFFFF0000U) | state.opponent_special_action;
                const auto configuration =
                    configure_legacy_battle_group_b_action(
                        &element,
                        &record,
                        group_b_configuration,
                        stale_eax,
                        opponent_token,
                        opponent_record_token(record_index)
                    );
                if (configuration.status !=
                    LegacyBattleGroupBActionConfigurationStatus::completed) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_action_configuration_typed_stop;
                    return result;
                }
                static_cast<void>(invoke(
                    state, port, result, kCallPopMode, {opponent_token, 0x400U}
                ));
                ++state.group_b_count;
                ++result.group_b_iterations;
                if (!advance_battle_stages(state, port, result)) {
                    return result;
                }
                ++wave;
            }
        }
        const u16 decremented =
            static_cast<u16>(low_word(state.phase_counter) - 1U);
        replace_low_word(state.phase_counter, decremented);
        if ((decremented & 0x7FFFU) != 0U) {
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        replace_low_word(state.phase_counter, 0U);
        state.opponent_spawn_count = 0U;
        return completed(result, 1U);
    }

    case 17U: {
        if (invoke(state, port, result, kCallQuerySeventeen, {source_token})
                .eax != 1U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, result, kCallClearMode, {source_token, 1U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallFinalizeMode, {source_token, 8U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallSetTargetMode, {source_token, 1U})
        );
        state.overlay_gate = 1U;
        const u8 processed =
            static_cast<u8>(state.opponent_processed_counter + 1U);
        replace_low_byte(state.opponent_processed_counter, processed);
        if (static_cast<i32>(processed) >= state.group_b_count) {
            state.opponent_workspace.fill(0U);
            state.action_pending_aux = 1U;
            port.battle_terminal_latch() = 0U;
            port.battle_message_state() = 0x63U;
            for (std::size_t index = 0U;
                 index < state.opponent_workspace.size();
                 index += 7U) {
                state.opponent_workspace[index] = 0xFFFFFFFFU;
            }
        }
        if (state.opponent_special_action != 0U) {
            const auto condition = invoke(
                state,
                port,
                result,
                kCallQueryOpponentCondition,
                {kLegacyBattleActionGroupBBaseToken}
            );
            if (condition.eax == 0U &&
                to_bits(state.group_b_count) - processed == 1U) {
                state.group_b_count = 1;
                replace_low_byte(
                    state.opponent_processed_counter, low_byte(condition.eax)
                );
                state.opponent_special_action = 0U;
                state.post_battle_counter = 0U;
                if (!advance_battle_stages(state, port, result)) {
                    return result;
                }
            }
        }
        return completed(result, 1U);
    }

    default:
        return result;
    }
}

}  // namespace openswd3::battle
