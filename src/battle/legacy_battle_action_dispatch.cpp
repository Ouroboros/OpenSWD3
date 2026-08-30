#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace openswd3::battle {

LegacyBattleGroupASummonMaterializationCallReply
LegacyBattleActionDispatchPort::invoke_group_a_summon_materialization(
    const LegacyBattleGroupASummonMaterializationCallRequest& request
) {
    LegacyBattleActionCallRequest call{};
    call.arguments[0U] = request.profile_token;
    switch (request.call) {
    case LegacyBattleGroupASummonMaterializationCall::allocate_profile:
        call.callee_token = kLegacyBattleGroupASummonAllocateCallToken;
        call.arguments[0U] = kLegacyBattleGroupASummonProfileSize;
        break;

    case LegacyBattleGroupASummonMaterializationCall::load_profile:
        call.callee_token = kLegacyBattleGroupASummonLoadCallToken;
        call.arguments[1U] = request.role_id;
        break;

    case LegacyBattleGroupASummonMaterializationCall::release_profile_text:
        call.callee_token = kLegacyBattleGroupASummonReleaseCallToken;
        break;

    case LegacyBattleGroupASummonMaterializationCall::report_missing_role:
        call.callee_token = kLegacyBattleGroupASummonDiagnosticCallToken;
        call.arguments = {
            request.window_token,
            request.diagnostic_text_token,
            0U,
            request.diagnostic_source_token,
            request.diagnostic_source_line,
        };
        break;
    }
    const auto reply = invoke(call);
    return {
        .eax = reply.eax,
        .ecx = reply.ecx,
        .edx = reply.edx,
        .profile_record = request.profile_record,
    };
}

namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallLegacyRandom = 0x00439070U;
constexpr u32 kCallQueryAction = 0x004786B0U;
constexpr u32 kCallQueryFallbackAction = 0x004786C0U;
constexpr u32 kCallActorTerminal = 0x0047CE80U;
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
constexpr u32 kCallTargetReady = 0x004751C0U;
constexpr u32 kCallResolveTarget = 0x00480AD0U;
constexpr u32 kCallTargetProperty = 0x00474B60U;
constexpr u32 kCallSetMode = 0x0047F380U;
constexpr u32 kCallEnablePresentation = 0x004787F0U;
constexpr u32 kCallCommitTemporaryRecord = 0x0047E070U;
constexpr u32 kCallRefreshTarget = 0x00478780U;
constexpr u32 kCallComputeValue = 0x00481010U;
constexpr u32 kCallPlayMessage = 0x00485610U;
constexpr u32 kCallSetSamplePan = 0x00485650U;
constexpr u32 kCallQueryTargetCode = 0x0047F910U;
constexpr u32 kCallQueryTargetDistance = 0x00477800U;
constexpr u32 kCallTargetPhaseValues = 0x00484500U;
constexpr u32 kCallTargetPhaseResource = 0x00478620U;
constexpr u32 kCallTargetPhaseCoordinates = 0x00478470U;
constexpr u32 kCallTargetPhaseDecode = 0x004019A0U;
constexpr u32 kCallTargetPhaseProperty = 0x0047CE70U;
constexpr u32 kCallTargetPhaseRelease = 0x004885A0U;
constexpr u32 kCallCommitTargetPhase = 0x00477710U;
constexpr u32 kCallActionFourReady = 0x004745B0U;
constexpr u32 kCallActionThirteenQueryOffsets = 0x00478400U;
constexpr u32 kCallActionThirteenQueryCoordinates = 0x004783B0U;
constexpr u32 kCallActionThirteenQueryBase = 0x00478470U;
constexpr u32 kCallActionThirteenRender = 0x004170E0U;
constexpr u32 kCallCommitMessageRecord = 0x0047DBD0U;
constexpr u32 kCallQueryLiveIndex = 0x004786E0U;
constexpr u32 kCallPrepareOpponent = 0x00478AE0U;
constexpr u32 kCallSelectOpponent = 0x00478A70U;
constexpr u32 kCallPublishScene = 0x004707B0U;
constexpr u32 kCallFinalizeSelection = 0x00478B30U;
constexpr u32 kCallBuildMessageToken = 0x00476DB0U;
constexpr u32 kCallPrepareMessageToken = 0x00478220U;
constexpr u32 kCallChoiceFirst = 0x00476160U;
constexpr u32 kCallChoiceSecond = 0x00476250U;
constexpr u32 kCallSetGlobalMode = 0x0047F900U;
constexpr u32 kCallPrepareTarget = 0x00478850U;
constexpr u32 kCallPushState = 0x0047D810U;
constexpr u32 kCallPopState = 0x0047D830U;
constexpr u32 kCallSetScreenMode = 0x0047CC40U;
constexpr u32 kCallSelectSummon = 0x0047D350U;
constexpr u32 kCallSummonMode = 0x0047DAB0U;
constexpr u32 kCallPrepareSummon = 0x004786F0U;
constexpr u32 kCallActionTwentySevenSecondary = 0x004838D0U;
constexpr u32 kCallSpecialActionUpdate = 0x004831C0U;
constexpr u32 kCallSpecialTurnFrame = 0x00483B30U;
constexpr u32 kCallActionSevenReady = 0x00479850U;
constexpr u32 kCallSimpleActorUpdate = 0x00482310U;
constexpr u32 kCallActorExit = 0x00482840U;
constexpr u32 kCallSpecialFourHundred = 0x00473C10U;
constexpr u32 kCallActorSuspended = 0x0047D930U;
constexpr u32 kCallClearPendingAction = 0x00482DA0U;
constexpr u32 kCallSpecialFourOhFive = 0x004731A0U;
constexpr u32 kCallSpecialFourOhSix = 0x004735B0U;
constexpr u32 kCallSpecialFourOhNine = 0x00474E60U;
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

[[nodiscard]] bool read_group_a_event_slot(
    const LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 index,
    u16& value
) noexcept {
    if (index < 40U) {
        if (context.startup_reset == nullptr) {
            result.status =
                LegacyBattleActionDispatchStatus::event_slot_typed_stop;
            return false;
        }
        const u32 packed = context.startup_reset->block_52022c[index / 2U];
        value = static_cast<u16>((index & 1U) == 0U ? packed : (packed >> 16U));
        return true;
    }
    const u32 tail_index = index - 40U;
    if (tail_index >= state.group_a_event_slots_tail.size()) {
        result.status = LegacyBattleActionDispatchStatus::event_slot_typed_stop;
        return false;
    }
    value = state.group_a_event_slots_tail[tail_index];
    return true;
}

void write_group_a_event_slot(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    const u32 index,
    const u16 value
) noexcept {
    if (index < 40U) {
        u32& packed = context.startup_reset->block_52022c[index / 2U];
        if ((index & 1U) == 0U) {
            packed = (packed & 0xFFFF0000U) | value;
        } else {
            packed = (packed & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
        }
        return;
    }
    state.group_a_event_slots_tail[index - 40U] = value;
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

LegacyBattleTargetPhaseCheckResult check_legacy_battle_target_phase(
    const LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleActionMessageProfile* target_profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseCheckRequest& request
) {
    LegacyBattleTargetPhaseCheckResult result;
    if (target_profile == nullptr || request.target_token == 0U) {
        result.status =
            LegacyBattleTargetPhaseCheckStatus::target_profile_typed_stop;
        return result;
    }

    ++result.value_query_calls;
    const auto values = port.invoke({
        .callee_token = kCallTargetPhaseValues,
        .arguments = {request.target_token},
        .ecx = request.target_token,
    });
    result.sampled_metric = std::bit_cast<i32>(values.outputs[0U]);
    result.sampled_argument = std::bit_cast<i32>(values.outputs[1U]);

    if ((target_profile->phase_flags & 0x20U) != 0U ||
        (target_profile->phase_flags & 0x800U) == 0U ||
        target_profile->phase_limit > 0x15U) {
        return result;
    }
    if (actor == nullptr) {
        result.status =
            LegacyBattleTargetPhaseCheckStatus::actor_profile_typed_stop;
        return result;
    }

    const i32 actor_level = static_cast<i32>(actor->profile_level);
    const i32 target_level = static_cast<i32>(target_profile->level);
    const i32 target_advantage = target_level - actor_level;
    result.level_delta = target_advantage;
    if (target_advantage >= 12) {
        return result;
    }

    const i32 actor_advantage = actor_level - target_level;
    if (actor_advantage >= 10) {
        result.return_eax = 1U;
        return result;
    }
    if (target_advantage >= 7 && target_advantage <= 11) {
        result.return_eax = result.sampled_metric <= result.sampled_argument / 4
            ? 1U
            : 0U;
        return result;
    }

    const auto compare_random = [&](const i32 threshold) {
        ++result.random_calls;
        const auto random = port.invoke({
            .callee_token = kCallLegacyRandom,
            .arguments = {100U},
        });
        return std::bit_cast<i32>(random.eax) <= threshold;
    };
    const i32 third = result.sampled_argument / 3;
    if (actor_advantage >= 5 && actor_advantage < 10) {
        result.return_eax =
            result.sampled_metric <= third || compare_random(80) ? 1U : 0U;
        return result;
    }
    if (target_advantage >= 1 && target_advantage < 7) {
        result.return_eax = result.sampled_metric <= third ||
                compare_random(20 - 5 * target_advantage)
            ? 1U
            : 0U;
        return result;
    }
    if (actor_advantage < 0) {
        return result;
    }
    result.return_eax = result.sampled_metric <= third ||
            compare_random(10 * actor_advantage + 20)
        ? 1U
        : 0U;
    return result;
}

LegacyBattleTargetPhaseStartResult start_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    const LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleRenderGeometry* render_geometry,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseStartRequest& request
) {
    LegacyBattleTargetPhaseStartResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = request.entry_ecx;
    result.return_edx = request.entry_edx;
    if (phase == nullptr || actor == nullptr || request.target_token == 0U) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::target_object_typed_stop;
        return result;
    }

    const auto invoke_phase = [&](const u32 callee,
                                  const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = result.return_eax,
            .ecx = request.target_token,
            .edx = result.return_edx,
        });
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return reply;
    };

    const auto resource =
        invoke_phase(kCallTargetPhaseResource, {request.target_token});
    ++result.resource_query_calls;
    phase->resource_token = resource.eax;

    const auto coordinates =
        invoke_phase(kCallTargetPhaseCoordinates, {request.target_token});
    ++result.coordinate_query_calls;
    phase->decoded_resource_token = 0U;
    phase->emitter = {};
    result.presentation_dwords_zeroed = 0x16U;
    if (phase->resource_token == 0U || resource.outputs[0U] == 0U) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::resource_object_typed_stop;
        return result;
    }

    const auto decoded = invoke_phase(
        kCallTargetPhaseDecode, {resource.outputs[0U], 0U, 0U, 0U}
    );
    ++result.decode_calls;
    auto& emitter = phase->emitter;
    phase->decoded_resource_token = decoded.eax;
    emitter.source_pixels = decoded.resource_words;
    emitter.source_width = static_cast<u16>(resource.outputs[1U]);
    emitter.source_height = static_cast<u16>(resource.outputs[2U]);

    const i32 horizontal_delta = static_cast<i32>(
        std::bit_cast<i16>(static_cast<u16>(coordinates.outputs[0U]))
    );
    emitter.source_origin_x = horizontal_delta - 1;
    emitter.source_origin_y = static_cast<i32>(
        std::bit_cast<i16>(static_cast<u16>(coordinates.outputs[1U]))
    );

    const u32 source_x = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->source_x_offset))
    );
    const u32 source_y = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->source_y_offset))
    );
    const u32 target_x = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->position_x))
    );
    emitter.target_origin_x =
        std::bit_cast<i32>(source_x - source_y + target_x - 0x32U);
    const u32 target_y = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->position_y))
    );
    emitter.target_origin_y = std::bit_cast<i32>(
        target_y - std::bit_cast<u32>(actor->target_phase_y_adjustment) + 0x28U
    );
    emitter.target_width = 1;
    emitter.target_height = 1;
    emitter.distance_offset_base = 0x14U;
    emitter.lifetime_divisor = 0x1EU;
    emitter.remaining_batches = emitter.source_height < 0x64U
        ? static_cast<u16>(emitter.source_height - 0x0AU)
        : static_cast<u16>(emitter.source_height >> 1U);
    emitter.spawn_divisor = 0x28U;
    emitter.flags = 0x56U;
    emitter.published_value_2c = 5;
    emitter.published_value_30 = 5;
    emitter.published_value_34 = 5;

    const auto property =
        invoke_phase(kCallTargetPhaseProperty, {request.target_token});
    ++result.property_query_calls;
    if (property.eax == 1U) {
        emitter.flags = static_cast<u16>(emitter.flags | 1U);
    }
    phase->mode_flags = static_cast<compat::u8>(phase->mode_flags | 8U);

    if (render_geometry == nullptr) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::host_surface_typed_stop;
        return result;
    }
    result.host_surface = set_legacy_battle_host_surface(
        *render_geometry, request.surface_width, request.surface_height
    );
    ++result.host_surface_calls;
    if (result.host_surface.row_offsets.status ==
        LegacyBattleRowOffsetStatus::write_out_of_range) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::host_surface_typed_stop;
        return result;
    }

    phase->runtime_gate = 0U;
    phase->block_0df4.fill(0U);
    phase->action_record = {};
    phase->spawn_action_records = {};
    result.tail_dwords_zeroed = 8U + 0x26U + 0xBEU;
    result.return_eax = 0U;
    result.return_ecx = 0U;
    return result;
}

LegacyBattleTargetPhaseSpawnFrameResult
advance_legacy_battle_target_phase_spawn_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleTargetPhaseSpawnFrameRequest& request
) {
    LegacyBattleTargetPhaseSpawnFrameResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U ||
        request.slot >= phase->spawn_action_records.size()) {
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::actor_state_typed_stop;
        return result;
    }
    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_frame = [&](const u32 callee,
                            const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_summon_frame({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = phase->spawn_action_records[request.slot];
    record.action_id = request.action_id;
    record.base_variant = request.action_variant;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    LegacyBattleLineRaster raster{};
    raster.start_x = static_cast<i32>(std::bit_cast<i16>(static_cast<u16>(
        request.target_x - record.draw_offset_x
    )));
    raster.start_y = static_cast<i32>(std::bit_cast<i16>(static_cast<u16>(
        request.target_y - record.draw_offset_y
    )));
    const u32 end_x =
        signed_word_bits(actor->position_x) +
        signed_word_bits(actor->render_x_base) -
        signed_word_bits(actor->source_y_offset) - record.draw_offset_x;
    const u32 end_y =
        signed_word_bits(actor->position_y) +
        signed_word_bits(actor->render_y_base) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        record.draw_offset_y;
    raster.end_x = std::bit_cast<i32>(end_x);
    raster.end_y = std::bit_cast<i32>(end_y);

    const u32 iteration_limit =
        request.iterations * phase->spawn_counters[request.slot];
    if (std::bit_cast<i32>(iteration_limit) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_limit));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    const u32 counter = phase->spawn_counters[request.slot] + 1U;
    phase->spawn_counters[request.slot] = counter;
    registers.eax = counter;
    replace_low_word(registers.eax, record.field_58);
    ++result.sample_calls;
    static_cast<void>(invoke_frame(
        kCallPlayMessage, {registers.eax, 0x004AB784U}
    ));
    record.field_58 = 0U;

    const u32 boundary =
        to_bits(raster.end_x) - actor->spawn_completion_offset;
    const u32 current_x =
        to_bits(raster.start_x) + to_bits(raster.current_x);
    ++result.render_calls;
    if (std::bit_cast<i32>(current_x) >= std::bit_cast<i32>(boundary)) {
        static_cast<void>(invoke_frame(
            kCallActionThirteenRender,
            {
                boundary,
                to_bits(raster.end_y),
                frame.width,
                frame.height,
                record.mode_flags,
                0U,
            }
        ));
        phase->spawn_counters[request.slot] = 0U;
        phase->block_0df4.fill(0U);
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_frame(
        kCallActionThirteenRender,
        {
            current_x,
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleTargetPhaseAdvanceResult advance_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* action_shared,
    asset_runtime::LegacyActionUpdater* action_updater,
    rendering::LegacyFramePieceProvider* frame_provider,
    LegacyBattleImageParticleNodePool* nodes,
    input_time_rng::LegacyCrtRng* rng,
    LegacyBattleImageParticleSharedState* shared,
    LegacyBattleImageParticleDiagnostics* diagnostics,
    const LegacyBattleImageParticleSurface& surface,
    rendering::LegacyPixelConversionState* pixel_format,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseAdvanceRequest& request
) {
    LegacyBattleTargetPhaseAdvanceResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = request.entry_ecx;
    result.return_edx = request.entry_edx;
    if (phase == nullptr || request.target_token == 0U) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::target_object_typed_stop;
        return result;
    }

    phase->tick = static_cast<u16>(phase->tick + 1U);
    phase->emitter.published_value_2c = 5;
    phase->emitter.published_value_30 = 5;
    phase->emitter.published_value_34 = 5;
    phase->active_gate = 1U;
    if (nodes == nullptr || rng == nullptr || shared == nullptr ||
        diagnostics == nullptr || pixel_format == nullptr) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::particle_frame_typed_stop;
        return result;
    }

    result.particle_frame = update_legacy_battle_image_particles(
        phase->emitter,
        surface,
        request.time_seed,
        request.spawn_stack_snapshot,
        *nodes,
        *rng,
        *shared,
        *diagnostics,
        *pixel_format
    );
    ++result.particle_frame_calls;
    if (result.particle_frame.status !=
        LegacyBattleImageParticleFrameStatus::completed) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::particle_frame_typed_stop;
        return result;
    }

    if (result.particle_frame.legacy_return_value == 1) {
        phase->tick = 0U;
        phase->active_gate = 0U;
        if (phase->decoded_resource_token != 0U) {
            static_cast<void>(port.invoke({
                .callee_token = kCallTargetPhaseRelease,
                .arguments = {phase->decoded_resource_token},
                .eax = phase->decoded_resource_token,
                .ecx = request.target_token,
                .edx = result.return_edx,
            }));
            ++result.port_calls;
            ++result.resource_release_calls;
        }
        phase->decoded_resource_token = 0U;
        phase->emitter = {};
        result.presentation_dwords_zeroed = 0x16U;
        phase->spawn_counters.fill(0U);
        result.spawn_counter_clears = 5U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.tail_dwords_zeroed = 8U + 0x26U;
        result.return_eax = 1U;
        return result;
    }

    if (phase->emitter.remaining_batches == 0U) {
        result.return_eax = 0U;
        return result;
    }

    const u32 horizontal = std::bit_cast<u32>(phase->emitter.source_origin_x);
    const u32 vertical = std::bit_cast<u32>(phase->emitter.source_origin_y);
    const u32 width_quarter =
        static_cast<u32>(phase->emitter.source_width) >> 2U;
    const u32 height = phase->emitter.source_height;
    const u32 derived = phase->emitter.remaining_batches;
    const auto spawn = [&](const u32 kind,
                           const u32 index,
                           const u32 x,
                           const u32 y,
                           const u32 iterations) {
        ++result.spawn_calls;
        if (actor == nullptr || action_shared == nullptr ||
            action_updater == nullptr || frame_provider == nullptr ||
            index >= result.spawn_frames.size()) {
            result.status =
                LegacyBattleTargetPhaseAdvanceStatus::spawn_frame_typed_stop;
            return false;
        }
        result.spawn_frames[index] =
            advance_legacy_battle_target_phase_spawn_frame(
                phase,
                actor,
                action_shared,
                port,
                *action_updater,
                *frame_provider,
                {
                    .actor_token = request.target_token,
                    .action_id = 0x186AU,
                    .action_variant = kind,
                    .slot = index,
                    .target_x = x,
                    .target_y = y,
                    .iterations = iterations,
                }
            );
        ++result.spawn_frame_calls;
        result.port_calls += result.spawn_frames[index].port_calls;
        if (result.spawn_frames[index].status !=
            LegacyBattleTargetPhaseSpawnFrameStatus::completed) {
            result.status =
                LegacyBattleTargetPhaseAdvanceStatus::spawn_frame_typed_stop;
            return false;
        }
        return true;
    };

    if (!spawn(
            1U,
            0U,
            horizontal + width_quarter,
            vertical + height - derived - 5U,
            0x0EU
        )) {
        return result;
    }
    const i16 signed_tick = std::bit_cast<i16>(phase->tick);
    if (signed_tick >= 10 &&
        !spawn(
            2U,
            1U,
            horizontal + width_quarter,
            vertical + height - derived - 0x0AU,
            0x0AU
        )) {
        return result;
    }
    if (signed_tick >= 20 &&
        !spawn(
            3U,
            2U,
            horizontal + width_quarter,
            vertical + height - derived - 0x0FU,
            0x0CU
        )) {
        return result;
    }
    if (signed_tick >= 30 &&
        !spawn(
            1U,
            3U,
            horizontal + width_quarter,
            vertical + height - derived,
            8U
        )) {
        return result;
    }
    if (signed_tick >= 40 &&
        !spawn(
            1U,
            4U,
            horizontal + width_quarter,
            vertical + height - derived + 5U,
            0x10U
        )) {
        return result;
    }
    result.return_eax = 0U;
    return result;
}

LegacyBattleActionThirteenResult advance_legacy_battle_action_thirteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionThirteenRequest& request
) {
    LegacyBattleActionThirteenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionThirteenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    phase->action_record.action_id = 0x186BU;
    phase->action_record.base_variant = 0U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            phase->action_record.field_4a,
            phase->action_record.field_4c,
            frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionThirteenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionThirteenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    actor->turn_target_x_offset =
        static_cast<u16>(phase->action_record.draw_offset_x);
    actor->turn_render_flags = phase->action_record.mode_flags;
    if (phase->render_toggle_gate == 1U) {
        const compat::u8 low =
            static_cast<compat::u8>(actor->turn_render_flags);
        actor->turn_render_flags =
            (actor->turn_render_flags & 0xFFFFFF00U) |
            static_cast<u32>((low & 1U) != 0U ? low & 0xFEU : low | 1U);
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width -
            static_cast<u16>(phase->action_record.draw_offset_x)
        );
    }

    registers.ecx = request.opponent_token;
    ++result.coordinate_query_calls;
    const auto offsets = invoke_action(
        kCallActionThirteenQueryOffsets, {request.opponent_token}
    );
    u32 endpoint_x{};
    u32 endpoint_y{};
    if (low_word(offsets.outputs[0U]) == 0U ||
        low_word(offsets.outputs[1U]) == 0U) {
        ++result.coordinate_query_calls;
        const auto coordinates = invoke_action(
            kCallActionThirteenQueryCoordinates, {request.opponent_token}
        );
        endpoint_x = static_cast<u16>(
            low_word(coordinates.outputs[0U]) - actor->turn_target_x_offset
        );
        endpoint_y = static_cast<u16>(
            low_word(coordinates.outputs[1U]) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    } else {
        ++result.coordinate_query_calls;
        const auto base = invoke_action(
            kCallActionThirteenQueryBase, {request.opponent_token}
        );
        endpoint_x = base.outputs[0U] + static_cast<u16>(
            low_word(offsets.outputs[0U]) - actor->turn_target_x_offset
        );
        endpoint_y = base.outputs[1U] + static_cast<u16>(
            low_word(offsets.outputs[1U]) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    }

    LegacyBattleLineRaster raster{};
    const u32 start_x =
        signed_word_bits(actor->source_x_offset) -
        signed_word_bits(actor->source_y_offset) +
        signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    const u32 start_y =
        signed_word_bits(actor->render_y_base) +
        signed_word_bits(actor->position_y) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        phase->action_record.draw_offset_y;
    raster.start_x = std::bit_cast<i32>(start_x);
    raster.start_y = std::bit_cast<i32>(start_y);
    raster.end_x = static_cast<i32>(signed_low_word(endpoint_x));
    raster.end_y = static_cast<i32>(signed_low_word(endpoint_y));

    bool completed = false;
    const u32 iteration_bits = phase->runtime_gate << 3U;
    if (std::bit_cast<i32>(iteration_bits) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            registers.edx = to_bits(raster.current_x) + to_bits(raster.start_x);
            if (registers.edx == to_bits(raster.end_x)) {
                completed = true;
                break;
            }
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_bits));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    phase->runtime_gate += 1U;

    replace_low_word(registers.edx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.edx, 0x004AB784U}
    ));
    phase->action_record.field_58 = 0U;

    ++result.render_calls;
    if (completed) {
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                signed_word_bits(static_cast<u16>(endpoint_x)),
                signed_word_bits(static_cast<u16>(endpoint_y)),
                frame.width,
                frame.height,
                actor->turn_render_flags,
                0U,
            }
        ));
        phase->runtime_gate = 0U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(raster.start_x) + to_bits(raster.current_x),
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionFourteenResult advance_legacy_battle_action_fourteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionFourteenRequest& request
) {
    LegacyBattleActionFourteenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionFourteenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    phase->action_record.action_id = 0x186BU;
    phase->action_record.base_variant = 1U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            phase->action_record.field_4a,
            phase->action_record.field_4c,
            frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionFourteenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionFourteenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    registers.ecx = request.opponent_token;
    ++result.coordinate_query_calls;
    const auto offsets = invoke_action(
        kCallActionThirteenQueryOffsets, {request.opponent_token}
    );
    u32 endpoint_x{};
    u32 endpoint_y{};
    if (low_word(offsets.outputs[0U]) == 0U ||
        low_word(offsets.outputs[1U]) == 0U) {
        ++result.coordinate_query_calls;
        const auto coordinates = invoke_action(
            kCallActionThirteenQueryCoordinates, {request.opponent_token}
        );
        endpoint_x = static_cast<u16>(
            low_word(coordinates.outputs[0U]) -
            static_cast<u16>(phase->action_record.draw_offset_x)
        );
        endpoint_y = static_cast<u16>(
            low_word(coordinates.outputs[1U]) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    } else {
        ++result.coordinate_query_calls;
        const auto base = invoke_action(
            kCallActionThirteenQueryBase, {request.opponent_token}
        );
        endpoint_x = base.outputs[0U] + static_cast<u16>(
            low_word(offsets.outputs[0U]) -
            static_cast<u16>(phase->action_record.draw_offset_x)
        );
        endpoint_y = base.outputs[1U] + static_cast<u16>(
            low_word(offsets.outputs[1U]) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    }

    LegacyBattleLineRaster raster{};
    raster.start_x = static_cast<i32>(signed_low_word(endpoint_x));
    raster.start_y = static_cast<i32>(signed_low_word(endpoint_y));
    const u32 end_x =
        signed_word_bits(actor->position_x) +
        signed_word_bits(actor->render_x_base) -
        signed_word_bits(actor->source_y_offset) -
        phase->action_record.draw_offset_x;
    const u32 end_y =
        signed_word_bits(actor->position_y) +
        signed_word_bits(actor->render_y_base) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        phase->action_record.draw_offset_y;
    raster.end_x = std::bit_cast<i32>(end_x);
    raster.end_y = std::bit_cast<i32>(end_y);

    bool completed = false;
    const u32 iteration_bits = phase->runtime_gate << 3U;
    if (std::bit_cast<i32>(iteration_bits) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            registers.edx = to_bits(raster.current_x) + to_bits(raster.start_x);
            if (registers.edx == to_bits(raster.end_x)) {
                completed = true;
                break;
            }
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_bits));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    phase->runtime_gate += 1U;

    replace_low_word(registers.ecx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.ecx, 0x004AB784U}
    ));
    phase->action_record.field_58 = 0U;
    const u32 render_flags = phase->action_record.mode_flags;

    ++result.render_calls;
    if (completed) {
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                to_bits(raster.end_x),
                to_bits(raster.end_y),
                frame.width,
                frame.height,
                render_flags,
                0U,
            }
        ));
        phase->runtime_gate = 0U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(raster.start_x) + to_bits(raster.current_x),
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            render_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyThreeResult
advance_legacy_battle_action_twenty_three(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyThreeRequest& request
) {
    LegacyBattleActionTwentyThreeResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };

    auto& record = actor->primary_action_record;
    record.action_id = actor->profile_value;
    record.base_variant = 0x2BU;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentyThreeStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    if (record.field_8c != 0U) {
        record = {};
        ++result.action_record_clears;
        result.return_eax = 1U;
        return result;
    }
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::phase_state_typed_stop;
        return result;
    }

    if (phase->render_toggle_gate == 1U && record.field_1c == 0U) {
        u32 flags = record.mode_flags;
        if ((flags & 1U) != 0U) {
            flags = (flags & 0xFFFFFF00U) |
                    (static_cast<u32>(static_cast<compat::u8>(flags)) & 0xFEU);
        } else {
            flags |= 1U;
        }
        const u32 old_x = record.draw_offset_x;
        record.mode_flags = flags;
        record.field_1c = flags | 0x00008000U;
        record.draw_offset_x = static_cast<u32>(frame.width) - old_x;
    }

    registers.ecx = request.opponent_token;
    ++result.coordinate_query_calls;
    const auto coordinates = invoke_action(
        kCallActionThirteenQueryCoordinates, {request.opponent_token}
    );
    const i16 relative_x = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinates.outputs[0U]) -
        static_cast<u16>(record.draw_offset_x)
    ));
    const i16 relative_y = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinates.outputs[1U]) -
        static_cast<u16>(record.draw_offset_y)
    ));

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 draw_motion = request.skip_primary == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = draw_motion;
    shared->draw_motion_b = draw_motion;
    shared->draw_motion_c = draw_motion;

    registers.ecx = actor->turn_frame_token;
    replace_low_word(registers.ecx, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.ecx, 0x004AB784U}
    ));
    ++result.sample_pan_calls;
    if (relative_x <= 0x140) {
        replace_low_word(registers.eax, record.field_58);
        static_cast<void>(invoke_action(
            kCallSetSamplePan, {registers.eax, 0xFFFFFFF0U}
        ));
    } else {
        replace_low_word(registers.edx, record.field_58);
        static_cast<void>(invoke_action(
            kCallSetSamplePan, {registers.edx, 0x10U}
        ));
    }

    const u32 modified_flags = (record.mode_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(static_cast<i32>(relative_x) - 5),
            record.draw_offset_y + to_bits(static_cast<i32>(relative_y)) -
                shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(static_cast<i32>(relative_x)),
            to_bits(static_cast<i32>(relative_y)),
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));

    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyThreeMessageResult
consume_legacy_battle_action_twenty_three_message(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActionMessageProfile* profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleActionTwentyThreeMessageRequest& request
) {
    LegacyBattleActionTwentyThreeMessageResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (profile == nullptr) {
        result.status = LegacyBattleActionTwentyThreeMessageStatus::
            profile_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_message = [&](const u32 callee,
                              const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };

    registers.eax = request.profile_token;
    if (profile->message_code == 0U) {
        replace_low_word(registers.eax, 0x61A8U);
        result.return_eax = registers.eax;
        return result;
    }
    if (actor == nullptr || request.actor_token == 0U) {
        result.status = LegacyBattleActionTwentyThreeMessageStatus::
            actor_state_typed_stop;
        return result;
    }

    registers.ecx = request.actor_token;
    ++result.percent_refresh_calls;
    const auto refreshed = invoke_message(kCallQueryPercent, {0x17U});
    actor->message_percent = low_word(refreshed.eax);
    if (actor->message_percent < 100U) {
        ++result.random_calls;
        const auto random = invoke_message(kCallLegacyRandom, {10U});
        const u32 ratio = static_cast<u32>(actor->message_percent) / 25U;
        registers.eax = ratio;
        const u16 random_value = low_word(random.eax);
        const u16 adjusted = ratio <= random_value
            ? static_cast<u16>(random_value - ratio)
            : 0U;
        if (adjusted >= profile->acceptance_threshold) {
            replace_low_word(registers.eax, 0U);
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
    }

    replace_low_word(registers.eax, profile->message_code);
    profile->message_code = 0U;
    ++result.message_code_clears;
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyFiveReadyResult
query_legacy_battle_action_twenty_five_ready(
    const LegacyBattleActionMessageProfile* target_profile
) noexcept {
    if (target_profile == nullptr) {
        return {
            .status = LegacyBattleActionTwentyFiveReadyStatus::
                target_profile_typed_stop,
        };
    }
    return {.return_eax = 1U};
}

LegacyBattleActionTwentyFourResult advance_legacy_battle_action_twenty_four(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyFourRequest& request
) {
    LegacyBattleActionTwentyFourResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentyFourStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = actor->primary_action_record;
    record.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    record.base_variant = 0x28U;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentyFourStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentyFourStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentyFourStatus::phase_state_typed_stop;
        return result;
    }

    actor->turn_target_x_offset = static_cast<u16>(record.draw_offset_x);
    actor->turn_render_flags = record.mode_flags;
    actor->source_x_offset = actor->secondary_auxiliary_word;
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width - static_cast<u16>(record.draw_offset_x)
        );
        if (actor->secondary_auxiliary_word != 0U) {
            actor->source_x_offset = static_cast<u16>(
                frame.width - actor->secondary_auxiliary_word
            );
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    shared->draw_motion_a = 0xFFFFFFFAU;
    shared->draw_motion_b = 0xFFFFFFFAU;
    shared->draw_motion_c = 0xFFFFFFFAU;

    registers.eax = actor->turn_frame_token;
    replace_low_word(registers.eax, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.eax, 0x004AB784U}
    ));
    replace_low_word(registers.edx, record.field_58);
    ++result.sample_pan_calls;
    static_cast<void>(invoke_action(
        kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U}
    ));

    const u32 modified_flags = (record.mode_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    const u32 draw_x = signed_word_bits(actor->position_x) -
                       record.draw_offset_x;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - record.draw_offset_y,
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));

    if ((actor->action_flags & 9U) != 0U) {
        const u16 special_return =
            static_cast<u16>(actor->copied_runtime_word | 0x8000U);
        actor->action_flags = 0U;
        replace_low_word(registers.eax, special_return);
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if (record.field_8c != 1U) {
        replace_low_word(registers.eax, 0U);
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->turn_completion_latch = 0U;
    record = {};
    ++result.action_record_clears;
    replace_low_word(registers.eax, 2U);
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentySevenResult
advance_legacy_battle_action_twenty_seven(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentySevenRequest& request
) {
    LegacyBattleActionTwentySevenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentySevenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = actor->primary_action_record;
    record.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    record.base_variant = 0x30U;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentySevenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentySevenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentySevenStatus::phase_state_typed_stop;
        return result;
    }

    actor->turn_target_x_offset = static_cast<u16>(record.draw_offset_x);
    actor->turn_render_flags = record.mode_flags;
    actor->source_x_offset = actor->secondary_auxiliary_word;
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width - static_cast<u16>(record.draw_offset_x)
        );
        if (actor->secondary_auxiliary_word != 0U) {
            actor->source_x_offset = static_cast<u16>(
                frame.width - actor->secondary_auxiliary_word
            );
        }
    }

    registers.ecx = request.target_token;
    ++result.coordinate_query_calls;
    const auto coordinates = invoke_action(
        kCallActionThirteenQueryCoordinates, {request.target_token}
    );
    const i16 relative_y = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinates.outputs[1U]) -
        static_cast<u16>(record.draw_offset_y)
    ));

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = motion;
    shared->draw_motion_b = motion;
    shared->draw_motion_c = motion;

    registers.edx = actor->turn_frame_token;
    replace_low_word(registers.edx, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.edx, 0x004AB784U}
    ));

    const u32 draw_x = signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    ++result.sample_pan_calls;
    if (std::bit_cast<i32>(draw_x) >= 0x140) {
        registers.eax = draw_x;
        replace_low_word(registers.eax, record.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.eax, 0x10U})
        );
    } else {
        replace_low_word(registers.edx, record.field_58);
        static_cast<void>(invoke_action(
            kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U}
        ));
    }

    const u32 modified_flags =
        (actor->turn_render_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            record.draw_offset_y +
                to_bits(static_cast<i32>(relative_y)) -
                shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - record.draw_offset_y,
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if ((actor->action_flags & 9U) != 0U) {
        actor->action_flags = 0U;
        actor->action_runtime_gate = 0x8000U;
        registers.ecx = request.target_token;
        ++result.target_refresh_calls;
        static_cast<void>(
            invoke_action(kCallRefreshTarget, {request.target_token})
        );
        registers.ecx = request.actor_token;
        ++result.effect_compute_calls;
        const auto computed = invoke_action(
            kCallComputeValue,
            {
                request.target_token,
                coordinates.outputs[0U],
                coordinates.outputs[1U],
            }
        );
        i32 effect = static_cast<i32>(std::bit_cast<i16>(low_word(computed.eax)));
        if (effect >= 0x270F) {
            effect = 0x270F;
        }
        result.effect_value = effect;
        shared->last_effect_value = effect;
        port.battle_pair_primary_value() += std::bit_cast<u32>(effect);

        registers.ecx = request.target_token;
        ++result.effect_publish_calls;
        static_cast<void>(invoke_action(
            kCallPublishSignedValue,
            {request.target_token, std::bit_cast<u32>(effect)}
        ));
        registers.ecx = request.target_token;
        ++result.effect_publish_calls;
        static_cast<void>(invoke_action(
            kCallTargetPhaseProperty, {request.target_token, 1U}
        ));
    }

    if (actor->action_runtime_gate != 0x8000U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    auto& secondary = actor->action_twenty_seven_record;
    secondary.action_id = actor->copied_runtime_word;
    secondary.base_variant = 0U;
    registers.ecx = request.actor_token;
    ++result.secondary_record_calls;
    static_cast<void>(invoke_action(
        kCallActionTwentySevenSecondary,
        {
            request.target_token,
            request.actor_token + 0x630U,
            0U,
            0U,
        }
    ));
    shared->turn_frame_source_token = actor->turn_frame_token;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            signed_word_bits(actor->draw_x),
            signed_word_bits(actor->draw_y),
            frame.width,
            frame.height,
            actor->render_flags,
            0U,
        }
    ));

    if (record.field_8c != 1U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->action_runtime_gate = 0U;
    secondary = {};
    record = {};
    result.action_record_clears += 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleDualRecordActionResult
advance_legacy_battle_dual_record_action(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleDualRecordActionRequest& request
) {
    LegacyBattleDualRecordActionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleDualRecordActionStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& primary = actor->primary_action_record;
    primary.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    primary.base_variant = 0x2DU;
    ++result.action_update_calls;
    const auto primary_updated = context.action_updater.update(primary);
    if (primary_updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            primary.field_4a, primary.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleDualRecordActionStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleDualRecordActionStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleDualRecordActionStatus::phase_state_typed_stop;
        return result;
    }

    actor->source_x_offset = primary.field_76;
    actor->turn_render_flags = primary.mode_flags;
    actor->turn_target_x_offset = static_cast<u16>(primary.draw_offset_x);
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            static_cast<u16>(frame.width) -
            static_cast<u16>(primary.draw_offset_x)
        );
        if (primary.field_76 != 0U) {
            actor->source_x_offset = static_cast<u16>(
                static_cast<u16>(frame.width) - primary.field_76
            );
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = motion;
    shared->draw_motion_b = motion;
    shared->draw_motion_c = motion;

    registers.edx = actor->turn_frame_token;
    replace_low_word(registers.edx, primary.field_58);
    ++result.sample_play_calls;
    static_cast<void>(invoke_action(
        kCallPlayMessage, {registers.edx, 0x004AB784U}
    ));

    const u32 draw_x = signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    registers.eax = draw_x;
    ++result.sample_pan_calls;
    if (std::bit_cast<i32>(draw_x) >= 0x140) {
        replace_low_word(registers.edx, primary.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.edx, 0x10U})
        );
    } else {
        replace_low_word(registers.ecx, primary.field_58);
        static_cast<void>(invoke_action(
            kCallSetSamplePan, {registers.ecx, 0xFFFFFFF0U}
        ));
    }

    const u32 modified_flags =
        (actor->turn_render_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    primary.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - primary.draw_offset_y,
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if ((primary.field_5a & 9U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    auto& secondary = actor->action_twenty_seven_record;
    secondary.action_id = request.secondary_action_id;
    secondary.base_variant = 0U;
    ++result.action_update_calls;
    const auto secondary_updated = context.action_updater.update(secondary);
    if (secondary_updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece secondary_frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            secondary.field_4a, secondary.field_4c, secondary_frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleDualRecordActionStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    shared->turn_frame_source_token = actor->turn_frame_token;

    actor->source_x_offset = secondary.field_76;
    actor->turn_render_flags = secondary.mode_flags;
    actor->turn_target_x_offset = static_cast<u16>(secondary.draw_offset_x);
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            static_cast<u16>(secondary_frame.width) -
            static_cast<u16>(secondary.draw_offset_x)
        );
        if (secondary.field_76 != 0U) {
            actor->source_x_offset = static_cast<u16>(
                static_cast<u16>(secondary_frame.width) - secondary.field_76
            );
        }
    }

    registers.ecx = request.coordinate_token;
    ++result.coordinate_query_calls;
    const auto coordinates = invoke_action(
        kCallActionThirteenQueryCoordinates, {request.coordinate_token}
    );
    const u16 draw_x_word = static_cast<u16>(
        low_word(coordinates.outputs[0U]) - actor->turn_target_x_offset
    );
    const u16 draw_y_word = static_cast<u16>(
        low_word(coordinates.outputs[1U]) -
        static_cast<u16>(secondary.draw_offset_y)
    );
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            signed_word_bits(draw_x_word),
            signed_word_bits(draw_y_word),
            secondary_frame.width,
            secondary_frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if (secondary.field_8c != 1U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    secondary = {};
    primary = {};
    result.action_record_clears = 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSpecialFiveHundredResult
advance_legacy_battle_special_five_hundred(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleSpecialFiveHundredRequest& request
) {
    LegacyBattleSpecialFiveHundredResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFiveHundredStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    const auto update_registers = [&](const LegacyBattleActionCallReply& reply) {
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
    };
    const auto signed_record_word = [](const u16 value) {
        return static_cast<i32>(std::bit_cast<i16>(value));
    };

    auto& special = actor->special_action_record;
    special.base_variant = actor->special_profile_variant;
    actor->turn_completion_latch = 1U;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    ++result.special_update_calls;
    ++result.port_calls;
    auto reply = port.invoke_special_action_update(
        {
            .callee_token = kCallSpecialActionUpdate,
            .arguments = {
                request.source_token,
                request.actor_token + 0x0AF0U,
            },
            .eax = special.action_id,
            .ecx = request.actor_token,
            .edx = request.source_token,
        },
        special
    );
    update_registers(reply);

    registers.edx = 0x4000U;
    u16 flags = special.field_5a;
    if ((flags & 2U) != 0U) {
        if (special.field_24 != 0U) {
            actor->action_runtime_gate |= 0x4000U;
            actor->turn_action_record.action_id = special.field_24;
            actor->turn_action_record.base_variant = special.field_28;
        }
        if ((flags & 0x0200U) != 0U) {
            special.external_mode = 1U;
        }
        special.field_24 = 0U;
        special.field_5a = static_cast<u16>(flags & 0xFFFDU);
        special.field_28 = 0U;
    }

    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        registers.edx = special.field_78;
        ++result.turn_frame_calls;
        ++result.port_calls;
        reply = port.invoke_special_turn_frame(
            {
                .callee_token = kCallSpecialTurnFrame,
                .arguments = {
                    request.actor_token + 0x0468U,
                    static_cast<u32>(special.field_78),
                },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->turn_action_record
        );
        update_registers(reply);
        if (reply.eax == 1U) {
            actor->action_runtime_gate &= ~0x4000U;
            special.field_5a = 0U;
            special.external_mode = 0U;
            actor->turn_action_record = {};
            ++result.action_record_clears;
        }
    }

    flags = special.field_5a;
    if ((flags & 8U) != 0U) {
        if ((flags & 0x0400U) != 0U) {
            if (shared == nullptr) {
                result.status =
                    LegacyBattleSpecialFiveHundredStatus::shared_state_typed_stop;
                result.return_eax = registers.eax;
                result.return_ecx = registers.ecx;
                result.return_edx = registers.edx;
                return result;
            }
            port.battle_color_initialization_gate() = 1U;
            result.color_initialization =
                initialize_legacy_battle_color_accumulation(
                    port.battle_color_accumulation_state(),
                    {
                        .current_red = signed_record_word(special.field_7a),
                        .current_green = signed_record_word(special.field_7c),
                        .current_blue = signed_record_word(special.field_7e),
                        .target_red = signed_record_word(special.field_80),
                        .target_green = signed_record_word(special.field_82),
                        .target_blue = signed_record_word(special.field_84),
                        .countdown = signed_record_word(special.field_86),
                    }
                );
            ++result.color_initialization_calls;
            registers.eax = result.color_initialization.return_eax;
            registers.ecx = result.color_initialization.return_ecx;
            registers.edx = result.color_initialization.return_edx;
            special.field_5a = static_cast<u16>(special.field_5a & 0xFBFFU);
        }
        if (shared == nullptr) {
            result.status =
                LegacyBattleSpecialFiveHundredStatus::shared_state_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        if ((shared->action_completion_flags & 0x8000U) == 0U) {
            shared->action_completion_flags |= 0x8000U;
        }
    }

    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFiveHundredStatus::shared_state_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if ((shared->action_completion_flags & 1U) == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->action_runtime_gate = 0U;
    special = {};
    ++result.action_record_clears;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSummonFrameResult advance_legacy_battle_summon_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleSummonFrameRequest& request
) {
    LegacyBattleSummonFrameResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSummonFrameStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_frame = [&](const u32 callee,
                            const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_summon_frame({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    const auto publish_motion = [&]() {
        const i32 threshold =
            static_cast<i32>(std::bit_cast<i16>(actor->turn_threshold));
        const i32 motion = threshold / 2 - 0x1F;
        const u32 bits = to_bits(motion);
        shared->draw_motion_a = bits;
        shared->draw_motion_b = bits;
        shared->draw_motion_c = bits;
    };

    phase->action_record.action_id = actor->summon_action_id;
    phase->action_record.base_variant = 0x24U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    const bool frame_available = frame_provider.load_frame_piece(
        phase->action_record.field_4a,
        phase->action_record.field_4c,
        frame
    );
    actor->turn_frame_token =
        frame_available ? request.actor_token + 0x254CU : 0U;

    replace_low_word(registers.ecx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(invoke_frame(
        kCallPlayMessage, {registers.ecx, 0x004AB784U}
    ));
    actor->turn_sample_word = 0U;
    actor->summon_render_flags = phase->action_record.mode_flags;
    actor->summon_x_offset = phase->action_record.draw_offset_x;
    if (phase->render_toggle_gate == 0U) {
        const compat::u8 low =
            static_cast<compat::u8>(actor->summon_render_flags);
        actor->summon_render_flags =
            (actor->summon_render_flags & 0xFFFFFF00U) |
            static_cast<u32>((low & 1U) != 0U ? low & 0xFEU : low | 1U);
        if (!frame_available) {
            result.status =
                LegacyBattleSummonFrameStatus::frame_owner_typed_stop;
            result.return_eax = 0U;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        actor->summon_x_offset =
            static_cast<u32>(frame.width) - actor->summon_x_offset;
    }

    if (actor->summon_phase == 0U) {
        if (shared == nullptr) {
            result.status =
                LegacyBattleSummonFrameStatus::shared_state_typed_stop;
            return result;
        }
        publish_motion();
        actor->turn_threshold =
            static_cast<u16>(actor->turn_threshold + 2U);
        if (std::bit_cast<i16>(actor->turn_threshold) > 0x3E) {
            actor->summon_phase = 1U;
        }
    }
    if (actor->summon_phase == 1U) {
        if (shared == nullptr) {
            result.status =
                LegacyBattleSummonFrameStatus::shared_state_typed_stop;
            return result;
        }
        publish_motion();
        actor->turn_threshold =
            static_cast<u16>(actor->turn_threshold - 2U);
        phase->tick = static_cast<u16>(phase->tick + 1U);
        if (std::bit_cast<i16>(actor->turn_threshold) <= 0) {
            actor->summon_phase = 2U;
        }
    }

    if (!frame_available) {
        result.status =
            LegacyBattleSummonFrameStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if (shared == nullptr) {
        result.status =
            LegacyBattleSummonFrameStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    ++result.render_calls;
    static_cast<void>(invoke_frame(
        kCallActionThirteenRender,
        {
            request.position_x - actor->summon_x_offset,
            request.position_y - phase->action_record.draw_offset_y,
            frame.width,
            frame.height,
            actor->summon_render_flags | 4U,
            0U,
        }
    ));
    if (actor->summon_phase != 2U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    ++result.sample_calls;
    static_cast<void>(invoke_frame(
        kCallPlayMessage, {0x6AU, 0x004AB784U}
    ));
    actor->turn_threshold = 0U;
    actor->summon_render_flags = 0U;
    actor->summon_x_offset = 0U;
    actor->summon_phase = 0U;
    phase->tick = 0U;
    actor->summon_completion_word = 0U;
    phase->action_record = {};
    phase->spawn_action_records = {};
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

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
    const auto begin_action = [&](const u32 target_token) -> bool {
        if (context.startup == nullptr ||
            group_a_index >= state.group_a_action_execution.size() ||
            group_a_index >= context.startup->party.size()) {
            result.status = LegacyBattleActionDispatchStatus::
                group_a_action_execution_typed_stop;
            return false;
        }
        const u32 skip_primary =
            group_a_index < context.group_a_skip_primary.size()
            ? context.group_a_skip_primary[group_a_index]
            : 0U;
        const u32 skip_secondary =
            group_a_index < context.group_a_skip_secondary.size()
            ? context.group_a_skip_secondary[group_a_index]
            : 0U;
        auto& party = context.startup->party[group_a_index];
        result.group_a_action_execution =
            advance_legacy_battle_group_a_action_execution(
                &state.group_a_action_execution[group_a_index],
                state.group_a_action_shared,
                state,
                party.progress,
                party.item_effect_application,
                actor_token,
                target_token,
                0U,
                skip_primary,
                skip_secondary,
                port
            );
        ++result.group_a_action_execution_calls;
        result.port_calls += result.group_a_action_execution.port_calls;
        if (result.group_a_action_execution.status !=
            LegacyBattleGroupAActionExecutionStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                group_a_action_execution_typed_stop;
            return false;
        }
        return result.group_a_action_execution.return_eax == 1U;
    };
    const auto release_actor_resource = [&]() -> bool {
        if (context.startup == nullptr ||
            group_a_index >= context.startup->party.size()) {
            result.status = LegacyBattleActionDispatchStatus::
                group_a_actor_list_action_typed_stop;
            return false;
        }

        auto& party = context.startup->party[group_a_index];
        result.actor_resource_release = release_legacy_battle_actor_resource(
            &party.actor_list,
            &party.workspace,
            actor_token,
            {.entry_edx = state.selection_source}
        );
        ++result.actor_resource_release_calls;
        if (result.actor_resource_release.status !=
            LegacyBattleActorListQueryStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                group_a_actor_list_action_typed_stop;
            return false;
        }

        return true;
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
            result.special_five_hundred =
                advance_legacy_battle_special_five_hundred(
                    &state.group_a_action_execution[group_a_index],
                    &state.group_a_action_shared,
                    port,
                    {
                        .actor_token = actor_token,
                        .source_token = target_token,
                    }
                );
            ++result.special_five_hundred_calls;
            result.port_calls += result.special_five_hundred.port_calls;
            if (result.special_five_hundred.status !=
                LegacyBattleSpecialFiveHundredStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    special_five_hundred_typed_stop;
                return result;
            }
            if (result.special_five_hundred.return_eax != 1U) {
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
        if (!begin_action(target_token)) {
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
        } else if (context.scripted_resource_release_test_compat) {
            state.computed_selection_word =
                low_word(invoke(
                             state,
                             port,
                             result,
                             kCallComputeSelection,
                             {state.selection_source, state.selection_context}
                )
                             .eax);
        } else {
            if (!release_actor_resource()) {
                return result;
            }

            state.computed_selection_word =
                result.actor_resource_release.output_word;
        }
        state.frame_effect.fade_active = 1U;
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        result.retreat_commit = commit_legacy_battle_retreat(
            {
                .packed_actor_counter = state.packed_actor_counter,
                .text_messages = context.text_messages,
                .text_message_head = context.startup_reset == nullptr
                    ? nullptr
                    : &context.startup_reset->block_5214f8[0U],
                .group_a_actions = state.group_a_action_execution,
            },
            port,
            group_a_index
        );
        ++result.retreat_commit_calls;
        result.port_calls += result.retreat_commit.port_calls;
        if (result.retreat_commit.status !=
            LegacyBattleRetreatCommitStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::text_message_typed_stop;
        }
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
    case 6U: {
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
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U, 0xAU, 0x28U, text_token, 0x80000002U}
                )) {
                return result;
            }
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
            result.target_phase_check = check_legacy_battle_target_phase(
                &state.group_a_action_execution[group_a_index],
                &state.group_b_message_profiles[group_b_index],
                port,
                {.target_token = group_b_token(group_b_index)}
            );
            ++result.target_phase_check_calls;
            result.port_calls += result.target_phase_check.value_query_calls +
                result.target_phase_check.random_calls;
            if (result.target_phase_check.status !=
                LegacyBattleTargetPhaseCheckStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_check_typed_stop;
                return result;
            }
            if (result.target_phase_check.return_eax == 1U) {
                state.phase_condition_aux = 1U;
            } else if (state.phase_condition_aux != 1U) {
                state.phase_condition = 0U;
                replace_low_word(state.phase_counter, 0x28U);
                return result;
            }
            if (context.startup == nullptr) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_start_typed_stop;
                return result;
            }
            result.target_phase_start = start_legacy_battle_target_phase(
                &state.group_a_target_phases[group_a_index],
                &state.group_a_action_execution[group_a_index],
                &context.startup->render_geometry,
                port,
                {
                    .target_token = group_b_token(group_b_index),
                    .surface_width = context.raster.surface.width,
                    .surface_height = context.raster.surface.height,
                }
            );
            ++result.target_phase_start_calls;
            result.port_calls += result.target_phase_start.port_calls;
            if (result.target_phase_start.status !=
                LegacyBattleTargetPhaseStartStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    target_phase_start_typed_stop;
                return result;
            }
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
        if (context.startup == nullptr) {
            result.status = LegacyBattleActionDispatchStatus::
                target_phase_advance_typed_stop;
            return result;
        }
        const auto& render_geometry = context.startup->render_geometry;
        std::span<const u32> surface_row_offsets;
        if (render_geometry.surface_row_offsets != nullptr &&
            render_geometry.surface_height > 0) {
            surface_row_offsets = {
                render_geometry.surface_row_offsets.get(),
                static_cast<std::size_t>(render_geometry.surface_height),
            };
        }
        result.target_phase_advance = advance_legacy_battle_target_phase(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            &context.action_updater,
            &context.frame_provider,
            &state.target_phase_particle_nodes,
            &state.target_phase_particle_rng,
            &state.target_phase_particle_shared,
            &state.target_phase_particle_diagnostics,
            {
                .width = context.raster.surface.width,
                .height = context.raster.surface.height,
                .row_offsets = surface_row_offsets,
                .pixels = context.framebuffer.physical_pixels(),
            },
            &context.shared_effects.pixel_conversion,
            port,
            {
                .target_token = actor_token,
                .time_seed = context.target_phase_time_seed,
                .spawn_stack_snapshot =
                    context.target_phase_spawn_stack_snapshot,
            }
        );
        ++result.target_phase_advance_calls;
        result.port_calls += result.target_phase_advance.port_calls;
        if (result.target_phase_advance.status !=
            LegacyBattleTargetPhaseAdvanceStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                target_phase_advance_typed_stop;
            return result;
        }
        if (result.target_phase_advance.return_eax == 1U) {
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
            if (!remove_attack_order_entry(context, result, group_b_index)) {
                return result;
            }
            state.selected_target_index = static_cast<u16>(group_b_index);
            replace_low_word(state.phase_counter, 0x1EU);
            const u16 status = state.group_b_status_words[group_b_index];
            if (!publish_player_item_quantity(port, result, status, 1U)) {
                return result;
            }
        }
        return result;
    }
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
        if (!remove_attack_order_entry(context, result, group_b_index)) {
            return result;
        }
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
        result.action_thirteen = advance_legacy_battle_action_thirteen(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = group_a_token(group_a_index),
                .opponent_token = group_b_token(group_b_index),
            }
        );
        ++result.action_thirteen_calls;
        if (result.action_thirteen.status !=
            LegacyBattleActionThirteenStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_thirteen_typed_stop;
            return result;
        }
        if (result.action_thirteen.return_eax != 1U) {
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
            const u32 index = group_a_index * 10U + slot;
            u16 value = 0U;
            if (!read_group_a_event_slot(
                    state, context, result, index, value
                )) {
                return result;
            }
            if (value == 0U) {
                write_group_a_event_slot(
                    state, context, index, static_cast<u16>(group_b_index + 1U)
                );
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
        result.action_fourteen = advance_legacy_battle_action_fourteen(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = group_a_token(group_a_index),
                .opponent_token = group_b_token(group_b_index),
            }
        );
        ++result.action_fourteen_calls;
        if (result.action_fourteen.status !=
            LegacyBattleActionFourteenStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_fourteen_typed_stop;
            return result;
        }
        if (result.action_fourteen.return_eax != 1U) {
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
            LegacyBattleGroupAConfigurationState* summon_state = nullptr;
            LegacyBattleGroupAPlacementRecord summon_source{};
            const LegacyBattleGroupAPlacementRecord* summon_source_view =
                nullptr;
            u32 summon_window_token = 0U;
            if (context.startup != nullptr) {
                auto& party = context.startup->party[summon_index];
                summon_state = &party.configuration;
                summon_source = {
                    .prefix = party.placement_prefix,
                    .role_id = party.role_id,
                    .position_x = party.position_x,
                    .position_y = party.position_y,
                    .field_1a = party.placement_field_1a,
                    .active = party.active,
                };
                summon_source_view = &summon_source;
                summon_window_token = context.startup->window_token;
            }
            result.summon_materialization =
                materialize_legacy_battle_group_a_summon(
                    summon_state,
                    summon_source_view,
                    group_a_token(summon_index),
                    0x0053AF70U + summon_index * 0x20U,
                    summon_window_token,
                    port
                );
            ++result.summon_materialization_calls;
            result.port_calls += result.summon_materialization.port_calls;
            if (result.summon_materialization.status !=
                LegacyBattleGroupASummonMaterializationStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    summon_materialization_typed_stop;
                return result;
            }
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
        result.summon_frame = advance_legacy_battle_summon_frame(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context.action_updater,
            context.frame_provider,
            {
                .actor_token = group_a_token(group_a_index),
                .position_x = state.summon_target_x[summon_index],
                .position_y = state.summon_target_y[summon_index],
            }
        );
        ++result.summon_frame_calls;
        if (result.summon_frame.status !=
            LegacyBattleSummonFrameStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::summon_frame_typed_stop;
            return result;
        }
        if (result.summon_frame.return_eax != 1U) {
            return result;
        }
        if (!remove_attack_order_entry(
                context, result, static_cast<u32>(summon_index + 8U)
            )) {
            return result;
        }
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
        const u32 skip_primary =
            group_a_index < context.group_a_skip_primary.size()
            ? context.group_a_skip_primary[group_a_index]
            : 0U;
        result.action_twenty_three =
            advance_legacy_battle_action_twenty_three(
                &state.group_a_target_phases[group_a_index],
                &state.group_a_action_execution[group_a_index],
                &state.group_a_action_shared,
                port,
                context,
                {
                    .actor_token = actor_token,
                    .opponent_token = group_b_token(group_b_index),
                    .skip_primary = skip_primary,
                }
            );
        ++result.action_twenty_three_calls;
        result.port_calls += result.action_twenty_three.port_calls;
        if (result.action_twenty_three.status !=
            LegacyBattleActionTwentyThreeStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_three_typed_stop;
            return result;
        }
        if (result.action_twenty_three.return_eax != 1U) {
            return result;
        }
        result.action_twenty_three_message =
            consume_legacy_battle_action_twenty_three_message(
                &state.group_a_action_execution[group_a_index],
                &state.group_b_message_profiles[group_b_index],
                port,
                {
                    .actor_token = actor_token,
                    .profile_token = group_b_token(group_b_index) + 0x0CU,
                }
            );
        ++result.action_twenty_three_message_calls;
        result.port_calls += result.action_twenty_three_message.port_calls;
        if (result.action_twenty_three_message.status !=
            LegacyBattleActionTwentyThreeMessageStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_three_message_typed_stop;
            return result;
        }
        const u16 message_code =
            low_word(result.action_twenty_three_message.return_eax);
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
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U, 0xAU, 0x32U, 0x0053C16CU, 0x80000002U}
                )) {
                return result;
            }
            if (!publish_player_item_quantity(port, result, message_code, 1U)) {
                return result;
            }
        } else {
            static_cast<void>(invoke(
                state, port, result, kCallPlayMessage, {0x116U, 0x004AB784U}
            ));
            if (!publish_text_message(
                    context,
                    port,
                    result,
                    {0x118U,
                     0xAU,
                     0x1EU,
                     message_code == 0x61A8U ? 0x004A77BCU : 0x004A77B0U,
                     0x80000002U}
                )) {
                return result;
            }
        }
        static_cast<void>(invoke(state, port, result, kCallSetDelay, {0x12CU}));
        return result;
    }
    case 24U: {
        if (!require_group_b()) {
            return result;
        }
        result.action_twenty_four = advance_legacy_battle_action_twenty_four(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {.actor_token = actor_token}
        );
        ++result.action_twenty_four_calls;
        result.port_calls += result.action_twenty_four.port_calls;
        if (result.action_twenty_four.status !=
            LegacyBattleActionTwentyFourStatus::completed) {
            result.status =
                LegacyBattleActionDispatchStatus::action_twenty_four_typed_stop;
            return result;
        }
        replace_low_word(
            state.packed_action_state,
            low_word(result.action_twenty_four.return_eax)
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
            result.action_twenty_five_ready =
                query_legacy_battle_action_twenty_five_ready(
                    &state.group_b_message_profiles[group_b_index]
                );
            ++result.action_twenty_five_ready_calls;
            if (result.action_twenty_five_ready.status !=
                LegacyBattleActionTwentyFiveReadyStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    action_twenty_five_ready_typed_stop;
                return result;
            }
            if (result.action_twenty_five_ready.return_eax == 1U) {
                if (!publish_text_message(
                        context,
                        port,
                        result,
                        {0x118U, 0xAU, 0x32U, 0x004A77A4U, 0x80000002U}
                    )) {
                    return result;
                }
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
                if (!remove_attack_order_entry(
                        context, result, group_b_index
                    )) {
                    return result;
                }
                state.current_actor_index = 0xFFFFU;
            } else {
                if (!publish_text_message(
                        context,
                        port,
                        result,
                        {0x118U, 0xAU, 0x32U, 0x004A7798U, 0x80000002U}
                    )) {
                    return result;
                }
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
        if (!begin_action(group_b_token(group_b_index))) {
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
        result.action_twenty_seven =
            advance_legacy_battle_action_twenty_seven(
                &state.group_a_target_phases[group_a_index],
                &state.group_a_action_execution[group_a_index],
                &state.group_a_action_shared,
                port,
                context,
                {
                    .actor_token = group_a_token(group_a_index),
                    .target_token = group_b_token(group_b_index),
                }
            );
        ++result.action_twenty_seven_calls;
        result.port_calls += result.action_twenty_seven.port_calls;
        if (result.action_twenty_seven.status !=
            LegacyBattleActionTwentySevenStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                action_twenty_seven_typed_stop;
            return result;
        }
        if (result.action_twenty_seven.return_eax != 1U) {
            return result;
        }
        if (context.scripted_resource_release_test_compat) {
            state.computed_selection_word =
                low_word(invoke(
                             state,
                             port,
                             result,
                             kCallComputeSelection,
                             {4U, state.selection_context}
                )
                             .eax);
        } else {
            if (!release_actor_resource()) {
                return result;
            }

            state.computed_selection_word =
                result.actor_resource_release.output_word;
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
        LegacyBattleGroupAActionExecutionState* action_actor =
            &state.group_a_action_execution[group_a_index];
        LegacyBattleTargetPhaseState* action_phase =
            &state.group_a_target_phases[group_a_index];
        u32 object_token = actor_token;
        if (action == 29U) {
            if (!require_group_b()) {
                return result;
            }
            auto& owned_actor =
                state.group_b_action_execution[group_b_index];
            auto& owned_phase = state.group_b_target_phases[group_b_index];
            if (owned_actor == nullptr) {
                owned_actor = std::make_unique<
                    LegacyBattleGroupAActionExecutionState>();
            }
            if (owned_phase == nullptr) {
                owned_phase = std::make_unique<LegacyBattleTargetPhaseState>();
            }
            action_actor = owned_actor.get();
            action_phase = owned_phase.get();
            object_token = group_b_token(group_b_index);
        }
        result.dual_record_action = advance_legacy_battle_dual_record_action(
            action_phase,
            action_actor,
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = object_token,
                .coordinate_token = object_token,
                .secondary_action_id = required_action,
            }
        );
        ++result.dual_record_action_calls;
        result.port_calls += result.dual_record_action.port_calls;
        if (result.dual_record_action.status !=
            LegacyBattleDualRecordActionStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                dual_record_action_typed_stop;
            return result;
        }
        if (result.dual_record_action.return_eax != 1U) {
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
        result.dual_record_action = advance_legacy_battle_dual_record_action(
            &state.group_a_target_phases[group_a_index],
            &state.group_a_action_execution[group_a_index],
            &state.group_a_action_shared,
            port,
            context,
            {
                .actor_token = actor_token,
                .coordinate_token = actor_token,
                .secondary_action_id = 0x17BAU,
            }
        );
        ++result.dual_record_action_calls;
        result.port_calls += result.dual_record_action.port_calls;
        if (result.dual_record_action.status !=
            LegacyBattleDualRecordActionStatus::completed) {
            result.status = LegacyBattleActionDispatchStatus::
                dual_record_action_typed_stop;
            return result;
        }
        if (result.dual_record_action.return_eax != 1U) {
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
