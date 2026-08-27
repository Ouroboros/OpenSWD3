#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryAction = 0x004786B0U;
constexpr u32 kCallQueryFallbackAction = 0x004786C0U;
constexpr u32 kCallActorTerminal = 0x0047CE80U;
constexpr u32 kCallBeginAction = 0x0046F8C0U;
constexpr u32 kCallCommitVisual = 0x0047F150U;
constexpr u32 kCallSetDelay = 0x00478710U;
constexpr u32 kCallQueryActorClass = 0x00482E90U;
constexpr u32 kCallQueryPercent = 0x00482F10U;
constexpr u32 kCallPublishSignedValue = 0x0047D640U;
constexpr u32 kCallSetActorAction = 0x004830A0U;
constexpr u32 kCallQuerySelection = 0x0047C680U;
constexpr u32 kCallQueryModeC = 0x0047C6B0U;
constexpr u32 kCallClearMode = 0x0047D870U;
constexpr u32 kCallFinalizeMode = 0x0047D860U;
constexpr u32 kCallQueryModeB = 0x0047C950U;
constexpr u32 kCallQuerySpecial = 0x0047D8E0U;
constexpr u32 kCallComputeSelection = 0x00470E20U;
constexpr u32 kCallActionReady = 0x00472CE0U;
constexpr u32 kCallTargetReady = 0x004751C0U;
constexpr u32 kCallResolveTarget = 0x00480AD0U;
constexpr u32 kCallTargetProperty = 0x00474B60U;
constexpr u32 kCallSetMode = 0x0047F380U;
constexpr u32 kCallEnablePresentation = 0x004787F0U;
constexpr u32 kCallCommitTemporaryRecord = 0x0047E070U;
constexpr u32 kCallRefreshTarget = 0x00478780U;
constexpr u32 kCallComputeValue = 0x00481010U;
constexpr u32 kCallPlayMessage = 0x00485610U;
constexpr u32 kCallShowMessage = 0x004698E0U;
constexpr u32 kCallQueryTargetCode = 0x0047F910U;
constexpr u32 kCallQueryTargetDistance = 0x00477800U;
constexpr u32 kCallCheckTargetPhase = 0x00472730U;
constexpr u32 kCallStartTargetPhase = 0x004710D0U;
constexpr u32 kCallCheckActorPhase = 0x00471270U;
constexpr u32 kCallCommitTargetPhase = 0x00477710U;
constexpr u32 kCallUpdateTarget = 0x0045EFB0U;
constexpr u32 kCallActionFourReady = 0x004745B0U;
constexpr u32 kCallActionThirteenReady = 0x004717F0U;
constexpr u32 kCallCommitMessageRecord = 0x0047DBD0U;
constexpr u32 kCallActionFourteenReady = 0x00471AD0U;
constexpr u32 kCallQueryLiveIndex = 0x004786E0U;
constexpr u32 kCallPrepareOpponent = 0x00478AE0U;
constexpr u32 kCallSelectOpponent = 0x00478A70U;
constexpr u32 kCallPublishScene = 0x004707B0U;
constexpr u32 kCallFinalizeSelection = 0x00478B30U;
constexpr u32 kCallActionTwentyThreeReady = 0x004721F0U;
constexpr u32 kCallQueryMessageCode = 0x00472430U;
constexpr u32 kCallBuildMessageToken = 0x00476DB0U;
constexpr u32 kCallPrepareMessageToken = 0x00478220U;
constexpr u32 kCallActionTwentyFour = 0x004724D0U;
constexpr u32 kCallChoiceFirst = 0x00476160U;
constexpr u32 kCallChoiceSecond = 0x00476250U;
constexpr u32 kCallActionTwentyFiveReady = 0x00472710U;
constexpr u32 kCallSetGlobalMode = 0x0047F900U;
constexpr u32 kCallPrepareTarget = 0x00478850U;
constexpr u32 kCallPushState = 0x0047D810U;
constexpr u32 kCallPopState = 0x0047D830U;
constexpr u32 kCallSetScreenMode = 0x0047CC40U;
constexpr u32 kCallSelectSummon = 0x0047D350U;
constexpr u32 kCallSummonMode = 0x0047DAB0U;
constexpr u32 kCallPrepareSummon = 0x004786F0U;
constexpr u32 kCallBuildSummon = 0x0046E890U;
constexpr u32 kCallSummonReady = 0x00471D60U;
constexpr u32 kCallActionTwentySevenReady = 0x004728E0U;
constexpr u32 kCallActionSevenReady = 0x00479850U;
constexpr u32 kCallSimpleActorUpdate = 0x00482310U;
constexpr u32 kCallActorExit = 0x00482840U;
constexpr u32 kCallSpecialFourHundred = 0x00473C10U;
constexpr u32 kCallActorSuspended = 0x0047D930U;
constexpr u32 kCallClearPendingAction = 0x00482DA0U;
constexpr u32 kCallSpecialFourOhFive = 0x004731A0U;
constexpr u32 kCallSpecialFourOhSix = 0x004735B0U;
constexpr u32 kCallSpecialFourOhNine = 0x00474E60U;
constexpr u32 kCallSpecialFiveHundred = 0x00473010U;
constexpr u32 kCallSpecialFourOhTwo = 0x00474BA0U;

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        static_cast<u32>(kLegacyBattleActionGroupAStride * index);
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        static_cast<u32>(kLegacyBattleActionGroupBStride * index);
}

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr i16 signed_low_word(const u32 value) noexcept {
    return std::bit_cast<i16>(low_word(value));
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

void replace_high_word(u32& destination, const u16 value) noexcept {
    destination =
        (destination & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

[[nodiscard]] LegacyBattleActionCallReply invoke(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 callee,
    const std::array<u32, 8>& arguments = {}
) {
    ++result.port_calls;
    LegacyBattleActionCallReply reply =
        port.invoke({.callee_token = callee, .arguments = arguments});
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

void refresh_shared_frame(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    const auto refresh = refresh_legacy_battle_frame(
        port,
        std::bit_cast<u16>(state.frame_effect.red_factor),
        std::bit_cast<u16>(state.frame_effect.green_factor),
        std::bit_cast<u16>(state.frame_effect.blue_factor)
    );
    result.port_calls += refresh.port_calls;
}

[[nodiscard]] bool rebuild_shared_actor_metrics(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    const auto metrics = rebuild_legacy_battle_actor_metrics(
        port, to_bits(state.group_b_count), to_bits(state.group_a_count)
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
    return true;
}

[[nodiscard]] bool rebuild_shared_actor_order(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
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
    const std::size_t writable =
        std::min<std::size_t>(requested_pixels, pixels.size());
    std::fill_n(pixels.begin(), writable, static_cast<u16>(0xFFFFU));
    ++result.framebuffer_clear_calls;
    if (requested_pixels > pixels.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::framebuffer_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] bool update_effect_score(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchResult& result,
    const u32 group_a_index,
    const u32 delta
) noexcept {
    if (group_a_index >= state.group_a_to_actor.size()) {
        result.status = LegacyBattleActionDispatchStatus::actor_map_typed_stop;
        return false;
    }
    const u32 actor_index = state.group_a_to_actor[group_a_index];
    if (actor_index >= state.actor_effect_score.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::effect_score_typed_stop;
        return false;
    }
    state.actor_effect_score[actor_index] += delta;
    return true;
}

[[nodiscard]] bool publish_target(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchResult& result,
    const u32 target_index
) noexcept {
    if (target_index >= state.target_identity.size() ||
        target_index >= state.selected_group_b_identity.size()) {
        result.status =
            LegacyBattleActionDispatchStatus::target_table_typed_stop;
        return false;
    }
    replace_high_word(
        state.packed_action_state, static_cast<u16>(target_index)
    );
    state.target_identity[target_index] = target_index;
    state.action_pending_aux = 0U;
    return true;
}

[[nodiscard]] bool query_internal_flag(
    const std::span<compat::u8> flags, const u32 index, bool& value
) noexcept {
    const std::size_t byte_index = index >> 3U;
    if (byte_index >= flags.size()) {
        return false;
    }
    value =
        (flags[byte_index] & static_cast<compat::u8>(1U << (index & 7U))) != 0U;
    return true;
}

[[nodiscard]] bool clear_internal_flag(
    const std::span<compat::u8> flags, const u32 index
) noexcept {
    const std::size_t byte_index = index >> 3U;
    if (byte_index >= flags.size()) {
        return false;
    }
    flags[byte_index] &=
        static_cast<compat::u8>(~static_cast<compat::u8>(1U << (index & 7U)));
    return true;
}

}  // namespace

LegacyBattleActionDispatchResult dispatch_legacy_battle_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const u32 group_a_index,
    const u32 group_b_index
) {
    LegacyBattleActionDispatchResult result;
    if (group_a_index >= 10U) {
        result.status =
            LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
        return result;
    }
    const u32 actor_token = group_a_token(group_a_index);
    LegacyBattleActionCallReply reply =
        invoke(state, port, result, kCallQueryAction, {actor_token});
    u16 action = low_word(reply.eax);
    result.action_code = action;
    reply = invoke(state, port, result, kCallActorTerminal, {actor_token});
    if (reply.eax == 1U) {
        result.return_value = 1U;
        return result;
    }
    if (action == 0U) {
        reply = invoke(
            state, port, result, kCallQueryFallbackAction, {actor_token}
        );
        action = low_word(reply.eax);
        result.action_code = action;
        if (action == 0U) {
            result.return_value = 1U;
            return result;
        }
    }

    const auto require_group_b = [&]() -> bool {
        if (group_b_index >= 8U) {
            result.status =
                LegacyBattleActionDispatchStatus::group_b_index_typed_stop;
            return false;
        }
        return true;
    };
    const auto side_token = [&](const u32 index) -> u32 {
        return state.side_mode != 0U ? group_a_token(index)
                                     : group_b_token(index);
    };

    if (action == 0x63U) {
        static_cast<void>(
            invoke(state, port, result, kCallSetScreenMode, {0U})
        );
        static_cast<void>(
            invoke(state, port, result, kCallSetGlobalMode, {0U})
        );
        state.stored_group_b_index = 0xFFFFU;
        state.stored_group_a_index = 0xFFFFU;
        state.current_actor_index = 0xFFFFU;
        state.result_mode = 1U;
        state.battle_submode = 2U;
        ++result.terminal_resets;
        result.return_value = 1U;
        return result;
    }

    if (action > 0x63U) {
        if (action <= 0x194U) {
            if (action == 0x64U) {
                result.return_value = 1U;
                return result;
            }
            if (action == 0xC8U) {
                if ((state.side_mode != 0U && group_b_index >= 10U) ||
                    (state.side_mode == 0U && !require_group_b())) {
                    return result;
                }
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallSimpleActorUpdate,
                    {side_token(group_b_index)}
                ));
                return result;
            }
            if (action == 0x12CU) {
                if ((state.side_mode != 0U && group_b_index >= 10U) ||
                    (state.side_mode == 0U && !require_group_b())) {
                    return result;
                }
                reply = invoke(
                    state,
                    port,
                    result,
                    kCallActorExit,
                    {side_token(group_b_index), 0xFFFFFFFFU, 0U}
                );
                if (reply.eax == 1U) {
                    state.current_actor_index = 0xFFFFU;
                    result.return_value = 1U;
                }
                return result;
            }
            if (action == 0x190U || action == 0x192U) {
                if (!require_group_b()) {
                    return result;
                }
                if (action == 0x192U) {
                    state.frame_effect.red_factor = -12;
                    state.frame_effect.green_factor = -12;
                    state.frame_effect.blue_factor = -12;
                    state.frame_effect.primary_suppression = 1U;
                    state.frame_effect.alternate_surface_mode = 1U;
                    reply = invoke(
                        state,
                        port,
                        result,
                        kCallSpecialFourOhTwo,
                        {group_b_token(group_b_index)}
                    );
                } else {
                    reply = invoke(
                        state,
                        port,
                        result,
                        kCallSpecialFourHundred,
                        {group_b_token(group_b_index), 0U}
                    );
                }
                if (reply.eax != 1U) {
                    return result;
                }
                state.action_pending = 1U;
                if (!publish_target(state, result, group_b_index) ||
                    !update_effect_score(state, result, group_a_index, 2U)) {
                    return result;
                }
                if (state.blocking_effect == 0U) {
                    reply = invoke(
                        state,
                        port,
                        result,
                        kCallCommitVisual,
                        {port.battle_pair_primary_value(), 0U, 0U}
                    );
                    if (reply.eax == 1U) {
                        state.selected_target_index =
                            static_cast<u16>(group_b_index);
                        state.selected_group_b_identity[group_b_index] =
                            group_b_index;
                        port.battle_pair_primary_value() = 0xFFFFFFFFU;
                        if (!clear_framebuffer(state, context, result)) {
                            return result;
                        }
                        static_cast<void>(
                            invoke(state, port, result, kCallSetDelay, {0x12CU})
                        );
                        if (!update_effect_score(
                                state, result, group_a_index, 5U
                            )) {
                            return result;
                        }
                    }
                }
                port.battle_pair_primary_value() = 0U;
                static_cast<void>(
                    invoke(state, port, result, kCallClearPendingAction, {0U})
                );
                static_cast<void>(
                    invoke(state, port, result, kCallSetDelay, {0x12CU})
                );
                return result;
            }
            if (action == 0x194U) {
                if (!require_group_b()) {
                    return result;
                }
                u16 phase = state.special_phase;
                if ((phase & 0x7FFFU) == 0U) {
                    state.special_phase = 1U;
                    for (i32 index = 0; index < state.group_a_count; ++index) {
                        ++result.group_a_iterations;
                        if (index >= 10) {
                            result.status = LegacyBattleActionDispatchStatus::
                                group_a_index_typed_stop;
                            return result;
                        }
                        if (index != static_cast<i32>(group_a_index)) {
                            reply = invoke(
                                state,
                                port,
                                result,
                                kCallActorSuspended,
                                {group_a_token(static_cast<u32>(index))}
                            );
                            if (reply.eax != 1U) {
                                static_cast<void>(invoke(
                                    state, port, result, kCallPushState, {4U}
                                ));
                                static_cast<void>(invoke(
                                    state, port, result, kCallPushState, {0x40U}
                                ));
                            }
                        }
                    }
                    for (i32 index = 0; index < state.group_b_count; ++index) {
                        ++result.group_b_iterations;
                        if (index >= 8) {
                            result.status = LegacyBattleActionDispatchStatus::
                                group_b_index_typed_stop;
                            return result;
                        }
                        static_cast<void>(
                            invoke(state, port, result, kCallPushState, {0x40U})
                        );
                        static_cast<void>(
                            invoke(state, port, result, kCallPushState, {4U})
                        );
                    }
                    phase = state.special_phase;
                }
                if ((phase & 0x7FFFU) == 1U) {
                    state.special_phase = 2U;
                    state.frame_effect.split_extent = 1U;
                    state.frame_effect.split_suppression = 1U;
                    phase = 2U;
                }
                if ((phase & 0x7FFFU) == 2U) {
                    reply = invoke(
                        state,
                        port,
                        result,
                        kCallActionFourReady,
                        {group_b_token(group_b_index)}
                    );
                    if (reply.eax == 1U) {
                        state.action_pending = 1U;
                        if (!publish_target(state, result, group_b_index) ||
                            !update_effect_score(
                                state, result, group_a_index, 2U
                            )) {
                            return result;
                        }
                        if (state.blocking_effect == 0U &&
                            invoke(
                                state,
                                port,
                                result,
                                kCallCommitVisual,
                                {port.battle_pair_primary_value(), 0U, 0U}
                            )
                                    .eax == 1U) {
                            state.selected_target_index =
                                static_cast<u16>(group_b_index);
                            state.selected_group_b_identity[group_b_index] =
                                group_b_index;
                            port.battle_pair_primary_value() = 0xFFFFFFFFU;
                            if (!clear_framebuffer(state, context, result) ||
                                !update_effect_score(
                                    state, result, group_a_index, 5U
                                )) {
                                return result;
                            }
                            static_cast<void>(invoke(
                                state, port, result, kCallSetDelay, {0x12CU}
                            ));
                        }
                        port.battle_pair_primary_value() = 0U;
                        state.special_phase = 0U;
                        state.frame_effect.split_extent = 0U;
                        state.frame_effect.split_suppression = 0U;
                        static_cast<void>(invoke(
                            state, port, result, kCallClearPendingAction, {0U}
                        ));
                        static_cast<void>(
                            invoke(state, port, result, kCallSetDelay, {0x12CU})
                        );
                        for (i32 index = 0; index < state.group_a_count;
                             ++index) {
                            ++result.group_a_iterations;
                            if (index >= 10) {
                                result.status =
                                    LegacyBattleActionDispatchStatus::
                                        group_a_index_typed_stop;
                                return result;
                            }
                            if (index != static_cast<i32>(group_a_index)) {
                                static_cast<void>(invoke(
                                    state, port, result, kCallPopState, {4U}
                                ));
                                static_cast<void>(invoke(
                                    state, port, result, kCallPopState, {0x40U}
                                ));
                            }
                        }
                        for (i32 index = 0; index < state.group_b_count;
                             ++index) {
                            ++result.group_b_iterations;
                            if (index >= 8) {
                                result.status =
                                    LegacyBattleActionDispatchStatus::
                                        group_b_index_typed_stop;
                                return result;
                            }
                            static_cast<void>(
                                invoke(state, port, result, kCallPopState, {4U})
                            );
                            static_cast<void>(invoke(
                                state, port, result, kCallPopState, {0x40U}
                            ));
                        }
                    }
                }
                return result;
            }
            return result;
        }

        if (!require_group_b()) {
            return result;
        }
        const u32 target_token = group_b_token(group_b_index);
        if (action == 0x195U || action == 0x196U) {
            reply = invoke(
                state,
                port,
                result,
                action == 0x195U ? kCallSpecialFourOhFive
                                 : kCallSpecialFourOhSix,
                {target_token}
            );
            if (reply.eax != 1U) {
                return result;
            }
        } else if (action == 0x199U) {
            reply = invoke(
                state, port, result, kCallSpecialFourOhNine, {target_token}
            );
            if (reply.eax != 1U) {
                return result;
            }
        } else if (action == 0x1F4U) {
            reply = invoke(
                state, port, result, kCallSpecialFiveHundred, {target_token}
            );
            if (reply.eax != 1U) {
                return result;
            }
        } else {
            return result;
        }

        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index) ||
            !update_effect_score(state, result, group_a_index, 2U)) {
            return result;
        }
        if (action != 0x1F4U && state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            if (action == 0x195U || action == 0x196U || action == 0x199U) {
                port.battle_pair_primary_value() = 0xFFFFFFFFU;
            }
            if (action == 0x199U) {
                static_cast<void>(
                    invoke(state, port, result, kCallSetScreenMode, {1U})
                );
                replace_low_word(state.scan_push_state, 0x8000U);
            }
            if (!clear_framebuffer(state, context, result) ||
                !update_effect_score(state, result, group_a_index, 5U)) {
                return result;
            }
        }
        port.battle_pair_primary_value() = 0U;
        if (action == 0x199U) {
            state.current_actor_index = 0xFFFFU;
            static_cast<void>(
                invoke(state, port, result, kCallSetDelay, {0x12CU})
            );
        }
        static_cast<void>(
            invoke(state, port, result, kCallClearPendingAction, {0U})
        );
        if (action != 0x199U) {
            static_cast<void>(
                invoke(state, port, result, kCallSetDelay, {0x12CU})
            );
        }
        if (action == 0x199U) {
            result.return_value = 1U;
        }
        return result;
    }

    switch (action) {
    case 1U: {
        if (!require_group_b()) {
            return result;
        }
        const u32 target_token = state.side_mode != 0U
            ? group_a_token(group_b_index)
            : group_b_token(group_b_index);
        reply =
            invoke(state, port, result, kCallBeginAction, {target_token, 0U});
        if (reply.eax != 1U) {
            return result;
        }
        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index)) {
            return result;
        }
        if (state.blocking_effect == 0U) {
            reply = invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            );
            if (reply.eax == 1U) {
                state.selected_target_index = static_cast<u16>(group_b_index);
                state.selected_group_b_identity[group_b_index] = group_b_index;
                if (!clear_framebuffer(state, context, result)) {
                    return result;
                }
                static_cast<void>(
                    invoke(state, port, result, kCallSetDelay, {0x12CU})
                );
            }
        }

        if (state.side_mode != 0U) {
            result.pair_transition = advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = actor_token,
                    .secondary_object_token = group_b_token(group_b_index),
                }
            );
            ++result.pair_transition_calls;
            result.port_calls += result.pair_transition.port_calls;
        }
        const u16 actor_class = low_word(
            invoke(state, port, result, kCallQueryActorClass, {actor_token}).eax
        );
        if (actor_class == 8U) {
            const u16 percent = low_word(
                invoke(state, port, result, kCallQueryPercent, {0x38U}).eax
            );
            state.signed_action_value = 0;
            const u32 base = (10U * port.battle_pair_primary_value()) / 100U;
            if (state.side_mode != 0U) {
                port.battle_pair_primary_value() = base +
                    (static_cast<u32>(percent) *
                     port.battle_pair_primary_value()) /
                        100U;
            } else {
                port.battle_pair_primary_value() = base +
                    (static_cast<u32>(percent) *
                     (port.battle_pair_primary_value() - base)) /
                        100U;
                static_cast<void>(
                    invoke(state, port, result, 0x004787D0U, {0x246FU})
                );
                port.battle_pair_primary_value() =
                    0U - port.battle_pair_primary_value();
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallPublishSignedValue,
                    {port.battle_pair_primary_value()}
                ));
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallCommitVisual,
                    {port.battle_pair_primary_value(), 0U, 0U}
                ));
                static_cast<void>(
                    invoke(state, port, result, 0x0047CF00U, {8U})
                );
                static_cast<void>(
                    invoke(state, port, result, 0x0047CEC0U, {1U})
                );
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallSetActorAction,
                {0x004B8A00U, 0x38U, 5U}
            ));
        }
        if (state.side_mode == 0U) {
            result.pair_transition = advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = actor_token,
                    .secondary_object_token = target_token,
                }
            );
            ++result.pair_transition_calls;
            result.port_calls += result.pair_transition.port_calls;
        }
        port.battle_pair_primary_value() = 0U;
        state.selection_high_word = 0U;
        state.selection_word = 0U;
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        return result;
    }
    case 2U:
    case 3U: {
        if ((state.action_runtime_flags & 0x8000U) == 0U) {
            replace_low_word(
                state.action_runtime_flags,
                static_cast<u16>(state.action_runtime_flags | 0x8000U)
            );
            state.active_actor_snapshot = low_word(
                invoke(state, port, result, kCallQuerySelection, {actor_token})
                    .eax
            );
            state.target_identity.fill(0xFFFFFFFFU);
            replace_low_word(state.input_mode, 1U);
            state.selection_workspace.fill(0U);
            if (action == 2U &&
                invoke(state, port, result, kCallQuerySpecial, {actor_token})
                        .eax == 1U) {
                state.deformation_active = true;
                reply = invoke(state, port, result, 0x00489E90U, {0x2CU});
                state.deformation_owner_token = reply.eax;
                if (reply.eax != 0U) {
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
                        static_cast<void>(invoke(
                            state,
                            port,
                            result,
                            0x00489D00U,
                            {state.deformation_owner_token}
                        ));
                        state.deformation_owner_token = 0U;
                        state.deformation_active = false;
                        throw;
                    }
                }
            }
            const i32 side_count = state.side_mode != 0U ? state.group_a_count
                                                         : state.group_b_count;
            if (state.available_actor_count > side_count) {
                state.available_actor_count = side_count;
            }
        }

        if ((state.action_runtime_flags & 1U) == 0U) {
            return result;
        }
        port.battle_pair_primary_value() = 0U;
        state.selection_word = 0U;
        state.selection_high_word = 0U;
        if (action == 2U) {
            if ((state.battle_flags & 0x20U) != 0U) {
                return result;
            }
            if (state.deformation_active) {
                state.deformation.reset();
                if (state.deformation_owner_token != 0U) {
                    static_cast<void>(invoke(
                        state,
                        port,
                        result,
                        0x00489D00U,
                        {state.deformation_owner_token}
                    ));
                }
            }
            state.deformation_owner_token = 0U;
            state.deformation_active = false;
        } else {
            state.computed_selection_word =
                low_word(invoke(
                             state,
                             port,
                             result,
                             kCallComputeSelection,
                             {state.selection_source, state.selection_context}
                )
                             .eax);
        }
        state.frame_effect.fade_active = 1U;
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        result.retreat_commit = commit_legacy_battle_retreat(
            {.packed_actor_counter = state.packed_actor_counter},
            port,
            group_a_index
        );
        ++result.retreat_commit_calls;
        result.port_calls += result.retreat_commit.port_calls;
        return result;
    }
    case 4U:
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionFourReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index) ||
            !update_effect_score(state, result, group_a_index, 2U)) {
            return result;
        }
        if (state.blocking_effect == 0U) {
            reply = invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            );
            if (reply.eax == 1U) {
                state.selected_target_index = static_cast<u16>(group_b_index);
                state.selected_group_b_identity[group_b_index] = group_b_index;
                port.battle_pair_primary_value() = 0xFFFFFFFFU;
                if (!clear_framebuffer(state, context, result)) {
                    return result;
                }
                static_cast<void>(
                    invoke(state, port, result, kCallSetDelay, {0x12CU})
                );
                if (!update_effect_score(state, result, group_a_index, 5U)) {
                    return result;
                }
            }
        }
        port.battle_pair_primary_value() = 0U;
        static_cast<void>(
            invoke(state, port, result, kCallClearPendingAction, {0U})
        );
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        return result;
    case 5U:
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 6U:
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) > 1U) {
            replace_low_word(
                state.phase_counter,
                static_cast<u16>(low_word(state.phase_counter) - 1U)
            );
            if (low_word(state.phase_counter) != 2U) {
                return result;
            }
            const u32 message_id =
                state.phase_condition == 1U ? 0x117U : 0x116U;
            const u32 text_token =
                state.phase_condition == 1U ? 0x004A77F0U : 0x004A77E4U;
            static_cast<void>(invoke(
                state, port, result, kCallPlayMessage, {message_id, 0x004AB784U}
            ));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallShowMessage,
                {0x118U, 0xAU, 0x28U, text_token, 0x80000002U}
            ));
            state.frame_effect.primary_suppression = 0U;
            state.frame_effect.red_factor = 0;
            state.frame_effect.green_factor = 0;
            state.frame_effect.blue_factor = 0;
            state.current_actor_index = 0xFFFFU;
            replace_low_word(state.phase_counter, 0U);
            state.selected_target_index = 0xFFFFU;
            state.phase_condition_aux = 0U;
            state.frame_effect.fade_active = 1U;
            const u32 low_byte = state.packed_actor_counter & 0xFFU;
            const u32 third_byte = (state.packed_actor_counter >> 16U) & 0xFFU;
            if (low_byte - third_byte >=
                static_cast<u32>(state.group_b_count)) {
                state.phase_terminal = 0U;
                port.battle_message_state() = 0x63U;
            }
            result.return_value = 1U;
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            const i16 target_code =
                signed_low_word(invoke(
                                    state,
                                    port,
                                    result,
                                    kCallQueryTargetCode,
                                    {group_b_token(group_b_index)}
                )
                                    .eax);
            const u16 distance =
                low_word(invoke(
                             state,
                             port,
                             result,
                             kCallQueryTargetDistance,
                             {0x004B9F00U, static_cast<u32>(target_code)}
                )
                             .eax);
            if (distance >= 0x14U) {
                state.phase_condition_aux = 1U;
            }
            reply = invoke(
                state,
                port,
                result,
                kCallCheckTargetPhase,
                {group_b_token(group_b_index)}
            );
            if (reply.eax == 1U) {
                state.phase_condition_aux = 1U;
            } else if (state.phase_condition_aux != 1U) {
                state.phase_condition = 0U;
                replace_low_word(state.phase_counter, 0x28U);
                return result;
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallStartTargetPhase,
                {group_b_token(group_b_index)}
            ));
            state.phase_condition = 1U;
            static_cast<void>(
                invoke(state, port, result, kCallEnablePresentation, {1U})
            );
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
            if (low_word(invoke(
                             state,
                             port,
                             result,
                             kCallQueryTargetCode,
                             {group_b_token(group_b_index)}
                )
                             .eax) == 0x1CU) {
                static_cast<void>(
                    invoke(state, port, result, kCallClearMode, {1U})
                );
            }
        }
        reply =
            invoke(state, port, result, kCallCheckActorPhase, {actor_token});
        if (reply.eax == 1U) {
            static_cast<void>(
                invoke(state, port, result, kCallEnablePresentation, {1U})
            );
            const i16 target_code =
                signed_low_word(invoke(
                                    state,
                                    port,
                                    result,
                                    kCallQueryTargetCode,
                                    {group_b_token(group_b_index)}
                )
                                    .eax);
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallCommitTargetPhase,
                {0x004B9F00U, static_cast<u32>(target_code), 1U}
            ));
            state.packed_actor_counter =
                (state.packed_actor_counter & 0xFFFFFF00U) |
                static_cast<compat::u8>(state.packed_actor_counter + 1U);
            static_cast<void>(
                invoke(state, port, result, kCallUpdateTarget, {group_b_index})
            );
            state.selected_target_index = static_cast<u16>(group_b_index);
            replace_low_word(state.phase_counter, 0x1EU);
            const u16 status = state.group_b_status_words[group_b_index];
            if (!publish_player_item_quantity(port, result, status, 1U)) {
                return result;
            }
        }
        return result;
    case 7U:
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionSevenReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0U}));
        static_cast<void>(
            invoke(state, port, result, kCallUpdateTarget, {group_b_index})
        );
        if (group_b_index < 4U) {
            state.packed_actor_counter =
                (state.packed_actor_counter & 0xFFFFFF00U) |
                static_cast<compat::u8>(state.packed_actor_counter + 1U);
        }
        result.return_value = 1U;
        return result;
    case 11U:
    case 12U:
        reply = invoke(
            state,
            port,
            result,
            action == 11U ? kCallQueryModeB : kCallQueryModeC,
            {actor_token}
        );
        if (reply.eax != 1U) {
            return result;
        }
        static_cast<void>(invoke(
            state, port, result, kCallClearMode, {action == 11U ? 0U : 1U}
        ));
        static_cast<void>(invoke(state, port, result, kCallFinalizeMode, {8U}));
        state.overlay_gate = 1U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 13U:
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionThirteenReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.temporary_record.fill(0U);
        state.temporary_record[0x19U] |= 0x20U;
        state.current_actor_index = 0xFFFFU;
        static_cast<void>(
            invoke(state, port, result, kCallCommitMessageRecord, {0x004FF140U})
        );
        state.current_actor_index = 0xFFFFU;
        state.frame_effect.fade_active = 1U;
        state.frame_effect.primary_suppression = 0U;
        state.frame_effect.red_factor = 0;
        state.frame_effect.green_factor = 0;
        state.frame_effect.blue_factor = 0;
        replace_low_word(state.phase_counter, 0U);
        for (u32 slot = 0U; slot < 8U; ++slot) {
            ++result.group_a_iterations;
            const std::size_t index = group_a_index * 10U + slot;
            if (index >= state.group_a_event_slots.size()) {
                result.status =
                    LegacyBattleActionDispatchStatus::event_slot_typed_stop;
                return result;
            }
            if (state.group_a_event_slots[index] == 0U) {
                state.group_a_event_slots[index] =
                    static_cast<u16>(group_b_index + 1U);
                result.return_value = 1U;
                return result;
            }
        }
        result.return_value = 1U;
        return result;
    case 14U:
        if (!require_group_b()) {
            return result;
        }
        if (low_word(state.phase_counter) == 0U) {
            state.frame_effect.red_factor = -12;
            state.frame_effect.green_factor = -12;
            state.frame_effect.blue_factor = -12;
            replace_low_word(state.phase_counter, 1U);
            state.frame_effect.primary_suppression = 1U;
            refresh_shared_frame(state, port, result);
            state.frame_effect.stage = 1;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionFourteenReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.frame_effect.primary_suppression = 0U;
        state.frame_effect.red_factor = 0;
        state.frame_effect.green_factor = 0;
        state.frame_effect.blue_factor = 0;
        replace_low_word(state.phase_counter, 0U);
        state.frame_effect.fade_active = 1U;
        port.battle_message_state() = 0x62U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 15U: {
        if (low_word(state.phase_counter) == 0U) {
            const u16 count = static_cast<u16>(state.summon_packed >> 16U);
            if ((state.battle_flags & 4U) == 0U && count < 2U) {
                replace_high_word(
                    state.summon_packed, static_cast<u16>(count + 1U)
                );
                ++state.group_a_count;
            }
            if (group_a_index >= state.group_a_status_words.size()) {
                result.status =
                    LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
                return result;
            }
            const u16 summon_index = state.group_a_status_words[group_a_index];
            replace_low_word(state.summon_packed, summon_index);
            if (summon_index >= 10U) {
                result.status =
                    LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
                return result;
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallSelectSummon,
                {group_a_token(summon_index)}
            ));
            static_cast<void>(
                invoke(state, port, result, kCallSummonMode, {1U})
            );
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPrepareSummon,
                {group_a_token(summon_index)}
            ));
            static_cast<void>(
                invoke(state, port, result, kCallClearMode, {1U})
            );
            if (state.summon_gate == 0U) {
                static_cast<void>(
                    invoke(state, port, result, kCallSetGlobalMode, {1U})
                );
            }
            static_cast<void>(
                invoke(state, port, result, kCallBuildSummon, {summon_index})
            );
            replace_low_word(state.phase_counter, 1U);
            state.summon_x = 0U;
            state.summon_y = 0U;
        }
        state.frame_effect.red_factor = -12;
        state.frame_effect.green_factor = -12;
        state.frame_effect.blue_factor = -12;
        state.frame_effect.primary_suppression = 1U;
        refresh_shared_frame(state, port, result);
        const u16 summon_index = low_word(state.summon_packed);
        if (summon_index >= state.summon_target_x.size()) {
            result.status =
                LegacyBattleActionDispatchStatus::group_a_index_typed_stop;
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallSummonReady,
            {state.summon_target_x[summon_index],
             state.summon_target_y[summon_index]}
        );
        if (reply.eax != 1U) {
            return result;
        }
        static_cast<void>(invoke(
            state,
            port,
            result,
            kCallUpdateTarget,
            {static_cast<u32>(summon_index + 8U)}
        ));
        state.battle_flags &= 0xFFFFFFFBU;
        state.summon_runtime[summon_index] = 0U;
        replace_low_word(state.summon_packed, 0U);
        state.group_a_status_words[group_a_index] = 0U;
        state.summon_status = 0x80U;
        state.message_aux = 0U;
        state.current_actor_index = 0xFFFFU;
        replace_low_word(state.phase_counter, 0U);
        if (!rebuild_shared_actor_metrics(state, port, result) ||
            !rebuild_shared_actor_order(port, result)) {
            return result;
        }
        result.return_value = 1U;
        return result;
    }
    case 17U:
        if (state.result_mode == 0U) {
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 22U: {
        if ((state.action_runtime_flags & 0x8000U) == 0U) {
            const rendering::LegacyBlitClipRectangle clip{
                .left = context.raster.clip_left,
                .top = context.raster.clip_top,
                .width = context.raster.clip_width,
                .height = context.raster.clip_height,
            };
            result.status_indicator = advance_legacy_battle_status_indicator(
                state.status_indicator,
                context.framebuffer,
                clip,
                context.shared_request,
                context.shared_effects,
                context.jitter,
                context.action_updater,
                context.frame_provider,
                context.bounded_random,
                context.indicator_sound,
                context.status_indicator_action_eax_snapshot
            );
            ++result.status_indicator_calls;
            if (result.status_indicator.status ==
                LegacyBattleStatusIndicatorStatus::blit_typed_stop) {
                result.status = LegacyBattleActionDispatchStatus::
                    status_indicator_typed_stop;
                return result;
            }
            if (result.status_indicator.return_value != 1U) {
                return result;
            }
            state.active_actor_snapshot = 6U;
            const bool group_a_side = state.side_selection_word != 0U;
            const u32 live_base = kLegacyBattleActionGroupABaseToken;
            const u16 selected = low_word(
                invoke(state, port, result, kCallQueryLiveIndex, {live_base})
                    .eax
            );
            if (selected >= 8U) {
                result.status =
                    LegacyBattleActionDispatchStatus::group_b_index_typed_stop;
                return result;
            }
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPrepareOpponent,
                {group_b_token(selected)}
            ));
            state.available_actor_count = 0;
            if (group_a_side) {
                state.side_mode = 1U;
                for (i32 index = 0; index < state.group_a_count; ++index) {
                    ++result.group_a_iterations;
                    if (index >= 10) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_a_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            result,
                            kCallActorTerminal,
                            {group_a_token(static_cast<u32>(index))}
                        )
                            .eax != 1U) {
                        ++state.available_actor_count;
                    }
                }
                i32 first = 0;
                while (first < state.group_a_count) {
                    if (first >= 10) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_a_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            result,
                            kCallActorTerminal,
                            {group_a_token(static_cast<u32>(first))}
                        )
                            .eax == 0U) {
                        static_cast<void>(invoke(
                            state,
                            port,
                            result,
                            kCallSelectOpponent,
                            {static_cast<u32>(first)}
                        ));
                        break;
                    }
                    ++first;
                }
            } else {
                for (i32 index = 0; index < state.group_b_count; ++index) {
                    ++result.group_b_iterations;
                    if (index >= 8) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            result,
                            kCallActorTerminal,
                            {group_b_token(static_cast<u32>(index))}
                        )
                            .eax != 1U) {
                        ++state.available_actor_count;
                    }
                }
                i32 first = 0;
                while (first < state.group_b_count) {
                    if (first >= 8) {
                        result.status = LegacyBattleActionDispatchStatus::
                            group_b_index_typed_stop;
                        return result;
                    }
                    if (invoke(
                            state,
                            port,
                            result,
                            kCallActorTerminal,
                            {group_b_token(static_cast<u32>(first))}
                        )
                            .eax == 0U) {
                        static_cast<void>(invoke(
                            state,
                            port,
                            result,
                            kCallSelectOpponent,
                            {static_cast<u32>(first)}
                        ));
                        break;
                    }
                    ++first;
                }
            }
            state.scene_value = 1U;
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPublishScene,
                {0x5FDU, 0x004FE5D4U + 4U * group_a_index}
            ));
            static_cast<void>(invoke(
                state, port, result, kCallFinalizeSelection, {actor_token}
            ));
            state.action_runtime_flags |= 0x8000U;
            return result;
        }
        if ((state.action_runtime_flags & 1U) == 0U) {
            return result;
        }
        state.frame_effect.fade_active = 1U;
        result.return_value = 1U;
        return result;
    }
    case 23U: {
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionTwentyThreeReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        const u16 message_code = low_word(invoke(
                                              state,
                                              port,
                                              result,
                                              kCallQueryMessageCode,
                                              {group_b_token(group_b_index)}
        )
                                              .eax);
        if (message_code != 0U && message_code < 0x61A8U) {
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallBuildMessageToken,
                {0x00453BC28U, message_code}
            ));
            static_cast<void>(invoke(
                state, port, result, kCallPrepareMessageToken, {0x00453BC28U}
            ));
            static_cast<void>(invoke(
                state, port, result, kCallPlayMessage, {0x117U, 0x004AB784U}
            ));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallShowMessage,
                {0x118U, 0xAU, 0x32U, 0x0053C16CU, 0x80000002U}
            ));
            if (!publish_player_item_quantity(port, result, message_code, 1U)) {
                return result;
            }
        } else {
            static_cast<void>(invoke(
                state, port, result, kCallPlayMessage, {0x116U, 0x004AB784U}
            ));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallShowMessage,
                {0x118U,
                 0xAU,
                 0x1EU,
                 message_code == 0x61A8U ? 0x004A77BCU : 0x004A77B0U,
                 0x80000002U}
            ));
        }
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        return result;
    }
    case 24U: {
        if (!require_group_b()) {
            return result;
        }
        replace_low_word(
            state.packed_action_state,
            low_word(invoke(
                         state,
                         port,
                         result,
                         kCallActionTwentyFour,
                         {group_b_token(group_b_index), 0U}
            )
                         .eax)
        );
        if ((low_word(state.packed_action_state) & 0x8000U) != 0U) {
            for (i32 index = 0; index < state.group_b_count; ++index) {
                ++result.group_b_iterations;
                if (index >= 8) {
                    result.status = LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop;
                    return result;
                }
                const u32 index_u32 = static_cast<u32>(index);
                if (invoke(
                        state,
                        port,
                        result,
                        kCallActorTerminal,
                        {group_b_token(index_u32)}
                    )
                        .eax == 1U) {
                    continue;
                }
                port.actor_metric_state().group_b_order[index_u32] = index_u32;
                reply = invoke(
                    state,
                    port,
                    result,
                    kCallComputeValue,
                    {group_b_token(index_u32),
                     state.selection_word,
                     state.selection_high_word}
                );
                i32 value = signed_low_word(reply.eax);
                if (value >= 0x270F) {
                    value = 0x270F;
                }
                state.signed_action_value = value;
                port.battle_pair_primary_value() += static_cast<u32>(value);
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallRefreshTarget,
                    {group_b_token(index_u32)}
                ));
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallPublishSignedValue,
                    {port.battle_pair_primary_value()}
                ));
                static_cast<void>(
                    invoke(state, port, result, 0x0047CEC0U, {1U})
                );
                if (state.blocking_effect == 0U &&
                    invoke(
                        state,
                        port,
                        result,
                        kCallCommitVisual,
                        {port.battle_pair_primary_value(), 0U, 0U}
                    )
                            .eax == 1U) {
                    state.frame_refresh_pending = 1U;
                    state.selected_target_index = static_cast<u16>(index);
                    state.selected_group_b_identity[index_u32] = index_u32;
                    if (!clear_framebuffer(state, context, result)) {
                        return result;
                    }
                }
                port.battle_pair_primary_value() = 0U;
            }
            state.message_aux = low_word(state.packed_action_state) & 0x7FFFU;
        }
        if (low_word(state.packed_action_state) == 2U) {
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        replace_low_word(state.packed_action_state, 0U);
        return result;
    }
    case 25U:
        if (!require_group_b()) {
            return result;
        }
        if (state.stored_group_b_index == 0xFFFFU) {
            reply = invoke(
                state,
                port,
                result,
                kCallActionTwentyFiveReady,
                {group_b_token(group_b_index)}
            );
            if (reply.eax == 1U) {
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallShowMessage,
                    {0x118U, 0xAU, 0x32U, 0x004A77A4U, 0x80000002U}
                ));
                static_cast<void>(
                    invoke(state, port, result, kCallSetGlobalMode, {1U})
                );
                state.stored_group_b_index = static_cast<u16>(group_b_index);
                state.stored_group_a_index = static_cast<u16>(group_a_index);
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallPrepareTarget,
                    {group_b_token(group_b_index)}
                ));
                static_cast<void>(invoke(
                    state, port, result, kCallUpdateTarget, {group_b_index}
                ));
                state.current_actor_index = 0xFFFFU;
            } else {
                static_cast<void>(invoke(
                    state,
                    port,
                    result,
                    kCallShowMessage,
                    {0x118U, 0xAU, 0x32U, 0x004A7798U, 0x80000002U}
                ));
            }
            result.return_value = 1U;
            return result;
        }
        if (state.stored_group_b_index >= state.group_b_status_words.size()) {
            result.status =
                LegacyBattleActionDispatchStatus::target_table_typed_stop;
            return result;
        }
        state.choice_cursor = state.choice_state + 1U;
        state.choice_commit = 1U;
        state.group_b_status_words[state.stored_group_b_index] = 0x8000U;
        if (state.message_gate != 0U) {
            state.group_b_status_words[state.stored_group_b_index] = 0x4000U;
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallChoiceFirst,
                {state.message_gate, port.battle_message_state()}
            ));
            state.message_gate = 0U;
        }
        if (state.message_aux != 0U) {
            state.group_b_status_words[state.stored_group_b_index] = 0x8000U;
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallChoiceSecond,
                {1U, port.battle_message_state()}
            ));
        }
        state.group_b_status_words[state.stored_group_b_index] |=
            static_cast<u16>(state.choice_cursor - 1U);
        result.attack_order = append_legacy_battle_attack_order_entry(
            context.attack_order_records,
            2U,
            state.stored_group_b_index,
            state.choice_cursor - 1U,
            0U
        );
        ++result.attack_order_calls;
        if (result.attack_order.status !=
            LegacyBattleAttackOrderEntryStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::attack_order_typed_stop;
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 26U: {
        state.phase_condition = 1U;
        if (low_word(state.scan_push_state) == 1U) {
            static_cast<void>(
                invoke(state, port, result, kCallPushState, {0x40U})
            );
        }
        result.scale_scan = draw_legacy_battle_scale_scan(
            state.scale_scan,
            context.framebuffer,
            context.shared_request,
            context.shared_effects,
            context.jitter,
            context.frame_provider,
            0x140,
            0xC8
        );
        ++result.scale_scan_calls;
        if (result.scale_scan.status !=
            LegacyBattleScaleScanStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::scale_scan_typed_stop;
            return result;
        }
        const auto finish_scan = [&]() {
            static_cast<void>(
                invoke(state, port, result, kCallPopState, {0x40U})
            );
            static_cast<void>(
                invoke(state, port, result, kCallSetDelay, {0x12CU})
            );
            state.scan_word = 0U;
            state.phase_condition = 0U;
            replace_low_word(state.scan_dialog_state, 0U);
            replace_low_word(state.scan_push_state, 0U);
        };
        if (result.scale_scan.return_value == 1U &&
            (state.scan_runtime & 0x8000U) == 0U) {
            finish_scan();
            return result;
        }
        if ((state.scan_runtime & 1U) == 0U) {
            return result;
        }
        state.scan_runtime = 0x8000U;
        if (low_word(state.scan_dialog_state) == 0U) {
            if (state.scan_word != 0U) {
                state.scan_word = 0U;
                static_cast<void>(invoke(
                    state, port, result, kCallPlayMessage, {0x2CU, 0x004AB784U}
                ));
                static_cast<void>(
                    invoke(state, port, result, kCallPopState, {0x40U})
                );
            } else {
                replace_low_word(state.scan_dialog_state, 1U);
                state.scan_word = 0U;
            }
        }
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallBeginAction,
            {group_b_token(group_b_index), 0U}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.scan_runtime &= 0xFFFU;
        state.action_pending = 1U;
        if (!publish_target(state, result, group_b_index)) {
            return result;
        }
        state.phase_condition = 0U;
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            port.battle_pair_primary_value() = 0xFFFFFFFFU;
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            state.frame_refresh_pending = 1U;
            static_cast<void>(
                invoke(state, port, result, kCallSetScreenMode, {1U})
            );
            if (!clear_framebuffer(state, context, result)) {
                return result;
            }
            replace_low_word(state.scan_push_state, 0x8000U);
        }
        port.battle_pair_primary_value() = 0U;
        static_cast<void>(invoke(state, port, result, kCallPushState, {0x40U}));
        if ((state.scan_push_state & 0x8000U) != 0U) {
            finish_scan();
        }
        return result;
    }
    case 27U:
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionTwentySevenReady,
            {group_b_token(group_b_index)}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.computed_selection_word =
            low_word(invoke(
                         state,
                         port,
                         result,
                         kCallComputeSelection,
                         {4U, state.selection_context}
            )
                         .eax);
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            state.frame_refresh_pending = 1U;
            if (!clear_framebuffer(state, context, result)) {
                return result;
            }
        }
        port.battle_pair_primary_value() = 0U;
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    case 28U:
    case 29U:
    case 32U: {
        const u32 required_action = action == 29U ? 0x1791U : 0x1965U;
        const u32 object_token = action == 29U
            ? (require_group_b() ? group_b_token(group_b_index) : 0U)
            : actor_token;
        if (result.status != LegacyBattleActionDispatchStatus::completed) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallActionReady,
            {object_token, required_action}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.temporary_record.fill(0U);
        const u16 percent = low_word(
            invoke(state, port, result, kCallQueryPercent, {action}).eax
        );
        state.temporary_record_flags = action == 28U ? 0x10000000U
            : action == 29U                          ? 0x08000000U
                                                     : 0x02000000U;
        state.temporary_record_mode =
            static_cast<compat::u8>((4U * percent) / 100U + 2U);
        static_cast<void>(invoke(
            state,
            port,
            result,
            kCallCommitTemporaryRecord,
            {state.temporary_record_flags, state.temporary_record_mode}
        ));
        if (action == 29U) {
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallRefreshTarget,
                {group_b_token(group_b_index)}
            ));
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    }
    case 31U: {
        port.battle_message_state() = 0U;
        if (state.message_gate == 0U) {
            state.message_gate = 0x80000000U;
            rendering::initialize_legacy_countdown(
                state.countdown,
                context.countdown_flags,
                {
                    .minutes = 0,
                    .seconds = 5,
                    .primary_transition_value = 0U,
                    .mode = 1,
                }
            );
            static_cast<void>(invoke(
                state, port, result, 0x004783B0U, {0x0053BF50U, 0x0053BF52U}
            ));
            static_cast<void>(clear_legacy_battle_action_record(
                state.persistent_action_record
            ));
            ++result.action_record_clear_calls;
        }
        bool escape_pressed{};
        if (!query_internal_flag(
                context.internal_flags, 0x4BU, escape_pressed
            )) {
            result.status =
                LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
            return result;
        }
        if (escape_pressed) {
            if (!clear_internal_flag(context.internal_flags, 0x4BU)) {
                result.status =
                    LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
                return result;
            }
            state.message_gate = 0U;
            state.message_aux = 0U;
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        if ((state.message_gate & 1U) == 0U) {
            return result;
        }
        static_cast<void>(
            invoke(state, port, result, kCallPlayMessage, {0x2EU, 0x004AB784U})
        );
        static_cast<void>(
            clear_legacy_battle_action_record(state.persistent_action_record)
        );
        ++result.action_record_clear_calls;
        state.message_gate = 0x80000000U;
        state.message_aux = 1U;
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallComputeValue,
            {group_b_token(group_b_index),
             state.selection_word,
             state.selection_high_word}
        );
        port.battle_pair_primary_value() =
            static_cast<u32>(signed_low_word(reply.eax));
        static_cast<void>(invoke(
            state,
            port,
            result,
            kCallRefreshTarget,
            {group_b_token(group_b_index)}
        ));
        static_cast<void>(invoke(
            state,
            port,
            result,
            kCallPublishSignedValue,
            {port.battle_pair_primary_value()}
        ));
        static_cast<void>(invoke(state, port, result, 0x0047CEC0U, {1U}));
        if (state.blocking_effect == 0U &&
            invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {port.battle_pair_primary_value(), 0U, 0U}
            )
                    .eax == 1U) {
            if (!clear_internal_flag(context.internal_flags, 0x4AU)) {
                result.status =
                    LegacyBattleActionDispatchStatus::internal_flag_typed_stop;
                return result;
            }
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            state.frame_refresh_pending = 1U;
            if (!clear_framebuffer(state, context, result)) {
                return result;
            }
            port.battle_pair_primary_value() = 0U;
            state.message_gate = 0U;
            state.current_actor_index = 0xFFFFU;
            result.return_value = 1U;
            return result;
        }
        port.battle_pair_primary_value() = 0U;
        return result;
    }
    case 33U:
        if (!require_group_b()) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallTargetReady,
            {group_b_token(group_b_index), 0x1791U}
        );
        if (reply.eax != 1U) {
            return result;
        }
        state.current_actor_index = 0xFFFFU;
        reply = invoke(
            state,
            port,
            result,
            kCallResolveTarget,
            {group_b_token(group_b_index)}
        );
        if (reply.eax == 0U) {
            result.status =
                LegacyBattleActionDispatchStatus::target_object_typed_stop;
            return result;
        }
        if ((reply.object_flags & 0x20U) != 0U) {
            result.return_value = 1U;
            return result;
        }
        reply = invoke(state, port, result, kCallQueryPercent, {0x21U});
        if (invoke(
                state, port, result, kCallTargetProperty, {low_word(reply.eax)}
            )
                .eax == 1U) {
            static_cast<void>(invoke(state, port, result, kCallSetMode, {7U}));
            static_cast<void>(
                invoke(state, port, result, kCallEnablePresentation, {1U})
            );
            state.selected_target_index = static_cast<u16>(group_b_index);
            state.selected_group_b_identity[group_b_index] = group_b_index;
            state.frame_refresh_pending = 1U;
            if (!clear_framebuffer(state, context, result)) {
                return result;
            }
        }
        result.return_value = 1U;
        return result;
    case 34U:
    case 35U:
    case 36U:
        reply = invoke(
            state, port, result, kCallActionReady, {actor_token, 0x17BAU}
        );
        if (reply.eax != 1U) {
            return result;
        }
        reply = invoke(
            state,
            port,
            result,
            kCallComputeValue,
            {actor_token, state.selection_word, state.selection_high_word}
        );
        state.signed_action_value = signed_low_word(reply.eax);
        if (action == 34U) {
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPublishSignedValue,
                {static_cast<u32>(state.signed_action_value)}
            ));
            static_cast<void>(invoke(state, port, result, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {static_cast<u32>(state.signed_action_value), 0U, 0U}
            ));
            state.signed_action_value = 0;
        } else if (action == 35U) {
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPublishSignedValue,
                {static_cast<u32>(static_cast<i16>(state.selection_word))}
            ));
            static_cast<void>(invoke(state, port, result, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {0U, state.selection_word, 0U}
            ));
            state.selection_word = 0U;
        } else {
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallPublishSignedValue,
                {static_cast<u32>(static_cast<i16>(state.selection_high_word))}
            ));
            static_cast<void>(invoke(state, port, result, 0x0047CEC0U, {1U}));
            static_cast<void>(invoke(
                state,
                port,
                result,
                kCallCommitVisual,
                {0U, 0U, state.selection_high_word}
            ));
            state.selection_high_word = 0U;
        }
        state.current_actor_index = 0xFFFFU;
        result.return_value = 1U;
        return result;
    default:
        return result;
    }
}

}  // namespace openswd3::battle
