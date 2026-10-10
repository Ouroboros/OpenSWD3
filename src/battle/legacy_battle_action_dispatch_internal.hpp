#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace openswd3::battle::action_dispatch_detail {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallLegacyRandom = 0x00439070U;
constexpr u32 kCallActorTerminal = 0x0047CE80U;
constexpr u32 kCallCommitVisual = 0x0047F150U;
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
constexpr u32 kCallResolveTarget = 0x00480AD0U;
constexpr u32 kCallSetMode = 0x0047F380U;
constexpr u32 kCallCommitTemporaryRecord = 0x0047E070U;
constexpr u32 kCallComputeValue = 0x00481010U;
constexpr u32 kCallPlayMessage = 0x00485610U;
constexpr u32 kCallSetSamplePan = 0x00485650U;
constexpr u32 kCallQueryTargetCode = 0x0047F910U;
constexpr u32 kCallTargetPhaseValues = 0x00484500U;
constexpr u32 kCallTargetPhaseDecode = 0x004019A0U;
constexpr u32 kCallTargetPhaseProperty = 0x0047CE70U;
constexpr u32 kCallTargetPhaseRelease = 0x004885A0U;
constexpr u32 kCallActionThirteenRender = 0x004170E0U;
constexpr u32 kCallCommitMessageRecord = 0x0047DBD0U;
constexpr u32 kCallPublishScene = 0x004707B0U;
constexpr u32 kCallLegacyStringCopy = 0x00499168U;
constexpr u32 kCallSetGlobalMode = 0x0047F900U;
constexpr u32 kCallPushState = 0x0047D810U;
constexpr u32 kCallPopState = 0x0047D830U;
constexpr u32 kCallSetScreenMode = 0x0047CC40U;
constexpr u32 kCallSelectSummon = 0x0047D350U;
constexpr u32 kCallSummonMode = 0x0047DAB0U;
constexpr u32 kCallActionTwentySevenSecondary = 0x004838D0U;
constexpr u32 kCallSpecialActionUpdate = 0x004831C0U;
constexpr u32 kCallSpecialTurnFrame = 0x00483B30U;
constexpr u32 kCallSimpleActorUpdate = 0x00482310U;
constexpr u32 kCallActorExit = 0x00482840U;
constexpr u32 kCallActionFourDirectEffect = 0x0047F940U;
constexpr u32 kCallActionFourOhTwoCoordinateUpdate = 0x00481FD0U;
constexpr u32 kCallActionFourOhTwoParticle = 0x004800F0U;
constexpr u32 kCallActionFourOhTwoParticleCommit = 0x004801A0U;
constexpr u32 kCallActionFourOhTwoCompletion = 0x0047FC40U;
constexpr u32 kCallSpecialFourOhNineCoordinateUpdate = 0x00484230U;
constexpr u32 kCallSpecialFourHundredWorkspace = 0x004820A0U;
constexpr u32 kCallSpecialFourHundredEffect = 0x004838D0U;
constexpr u32 kCallTargetEffectSkipGate = 0x0047CD60U;
constexpr u32 kCallTargetEffectApply = 0x0047D640U;
constexpr u32 kCallActorSuspended = 0x0047D930U;
constexpr u32 kCallClearPendingAction = 0x00482DA0U;
constexpr u32 kCallTargetEffectProperty = 0x0047CEC0U;
constexpr u32 kCallActorEffectMode = 0x0047CF00U;
constexpr u32 kTargetPhaseFrameResourceReturnAddress = 0x004710E4U;
constexpr u32 kTargetPhaseResourceObjectReadInstruction = 0x00471120U;
constexpr u32 kOpponentTargetPhaseFrameResourceReturnAddress = 0x00484034U;
constexpr u32 kOpponentTargetPhaseResourceWriteInstruction = 0x00484034U;
constexpr u32 kOpponentTargetPhasePresentationClearInstruction = 0x0048405EU;
constexpr u32 kOpponentTargetPhaseResourceObjectReadInstruction = 0x0048407EU;
constexpr u32 kHostSurfaceRowOffsetWriteInstruction = 0x00433EEFU;
constexpr u32 kActionDispatchTargetPhaseStartReturnAddress = 0x004546ACU;

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

inline void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

inline void replace_high_word(u32& destination, const u16 value) noexcept {
    destination =
        (destination & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

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

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right) noexcept {
    const u32 sum = left + right;
    return {
        .carry = sum < left,
        .parity = has_even_parity(sum),
        .auxiliary_carry = ((left ^ right ^ sum) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = sum == 0U,
        .sign = (sum & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ sum)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_word_flags(const u16 left, const u16 right) noexcept {
    const u16 difference = static_cast<u16>(left - right);
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x8000U) != 0U,
        .overflow = ((left ^ right) & (left ^ difference) & 0x8000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_byte_flags(const u8 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_word_flags(const u16 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
increment_flags(const u32 before, const bool carry) noexcept {
    const u32 value = before + 1U;
    return {
        .carry = carry,
        .parity = has_even_parity(value),
        .auxiliary_carry = (before & 0xFU) == 0xFU,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = before == 0x7FFFFFFFU,
    };
}

[[nodiscard]] inline LegacyBattleActorCoordinateQueryResult query_coordinates(
    const LegacyBattleActorCoordinateOwners& owners,
    const u32 actor_token,
    u32& output_x,
    u32& output_y,
    const u32 output_x_token,
    const u32 output_y_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags
) noexcept {
    u16 output_x_word = low_word(output_x);
    u16 output_y_word = low_word(output_y);
    auto result = query_legacy_battle_actor_coordinates(
        resolve_legacy_battle_actor_coordinates(owners, actor_token),
        &output_x_word,
        &output_y_word,
        {
            .actor_token = actor_token,
            .output_x_token = output_x_token,
            .output_y_token = output_y_token,
            .entry_eax = entry_eax,
            .entry_edx = entry_edx,
            .entry_flags = entry_flags,
        }
    );
    if (result.output_writes >= 1U) {
        replace_low_word(output_x, output_x_word);
    }
    if (result.output_writes >= 2U) {
        replace_low_word(output_y, output_y_word);
    }
    return result;
}

[[nodiscard]] inline LegacyBattleActorBaseCoordinateQueryResult
query_base_coordinates(
    const LegacyBattleActorCoordinateOwners& owners,
    const u32 actor_token,
    u32& output_x,
    u32& output_y,
    const u32 output_x_token,
    const u32 output_y_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags
) noexcept {
    u16 output_x_word = low_word(output_x);
    u16 output_y_word = low_word(output_y);
    auto result = query_legacy_battle_actor_base_coordinates(
        resolve_legacy_battle_actor_coordinates(owners, actor_token),
        &output_x_word,
        &output_y_word,
        {
            .actor_token = actor_token,
            .output_x_token = output_x_token,
            .output_y_token = output_y_token,
            .entry_eax = entry_eax,
            .entry_edx = entry_edx,
            .entry_flags = entry_flags,
        }
    );
    if (result.output_writes >= 1U) {
        replace_low_word(output_x, output_x_word);
    }
    if (result.output_writes >= 2U) {
        replace_low_word(output_y, output_y_word);
    }
    return result;
}

[[nodiscard]] inline LegacyBattleActorRenderOffsetQueryResult
query_render_offsets(
    const LegacyBattleActorCoordinateOwners& owners,
    const u32 actor_token,
    u32& output_x,
    u32& output_y,
    const u32 output_x_token,
    const u32 output_y_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 entry_esi,
    const LegacyBattleActorCoordinateFlags& entry_flags
) noexcept {
    u16 output_x_word = low_word(output_x);
    u16 output_y_word = low_word(output_y);
    auto result = query_legacy_battle_actor_render_offsets(
        resolve_legacy_battle_actor_render_offsets(owners, actor_token),
        &output_x_word,
        &output_y_word,
        {
            .actor_token = actor_token,
            .output_x_token = output_x_token,
            .output_y_token = output_y_token,
            .entry_eax = entry_eax,
            .entry_edx = entry_edx,
            .entry_esi = entry_esi,
            .entry_flags = entry_flags,
        }
    );
    if (result.output_writes >= 1U) {
        replace_low_word(output_x, output_x_word);
    }
    if (result.output_writes >= 2U) {
        replace_low_word(output_y, output_y_word);
    }
    return result;
}

[[nodiscard]] inline LegacyBattleActionCallReply invoke(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    const u32 callee,
    const std::array<u32, 8>& arguments = {}
) {
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

[[nodiscard]] inline bool activate_actor_presentation(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known = true
) noexcept {
    if (execute_legacy_battle_actor_presentation_activation_call(
            {
                .action = &state,
                .startup = context.startup,
            },
            result.actor_presentation_activation,
            context.actor_presentation_activation_requests,
            actor_token,
            1U,
            entry_eax,
            entry_edx,
            call_address,
            return_address,
            entry_flags,
            entry_flags_known
        )) {
        return true;
    }
    result.status = LegacyBattleActionDispatchStatus::
        actor_presentation_activation_typed_stop;
    result.return_value = result.actor_presentation_activation.last.return_eax;
    return false;
}

class ActionCompositionPortAdapter final
    : public LegacyBattleGroupBActionCompositionPort {
public:
    ActionCompositionPortAdapter(LegacyBattleActionDispatchPort& port) noexcept
        : port_(port) {}

    [[nodiscard]] inline LegacyBattleGroupBActionCompositionCallReply invoke(
        const LegacyBattleGroupBActionCompositionCallRequest& request
    ) override {
        u32 callee{};
        switch (request.call) {
        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_resource_definition:
            return {
                .eax = 0U,
                .ecx = 0U,
                .edx = 0U,
                .typed_stop = true,
                .resource_definition = nullptr,
                .profile_buffer = nullptr,
            };

        case LegacyBattleGroupBActionCompositionCall::copy_action_text:
            callee = kCallLegacyStringCopy;
            break;

        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_action_profile:
            return {
                .eax = 0U,
                .ecx = 0U,
                .edx = 0U,
                .typed_stop = true,
                .resource_definition = nullptr,
                .profile_buffer = nullptr,
            };
        }
        LegacyBattleActionCallRequest call{
            .callee_token = callee,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        call.arguments[0U] = request.arguments[0U];
        call.arguments[1U] = request.arguments[1U];
        const auto reply = port_.invoke(call);
        LegacyBattleGroupBActionCompositionCallReply mapped{
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = port_.group_b_action_configuration_typed_stop(callee),
            .resource_definition = nullptr,
            .profile_buffer = nullptr,
        };

        return mapped;
    }

private:
    LegacyBattleActionDispatchPort& port_;
};

[[nodiscard]] inline bool remove_attack_order_entry(
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 value
) {
    result.attack_order_remove = remove_legacy_battle_attack_order_entry(
        {
            .records = context.attack_order_records,
            .adjacent_intensity_records =
                context.attack_order_adjacent_intensity_records,
        },
        value
    );
    if (result.attack_order_remove->status !=
        LegacyBattleAttackOrderRemoveStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::attack_order_remove_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] inline bool publish_text_message(
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
    if (message.status != LegacyBattleTextMessageStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::text_message_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] inline bool publish_player_item_quantity(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 item_id,
    const u32 quantity_selector
) {
    result.player_item = advance_legacy_battle_player_item_quantity(
        port, item_id, quantity_selector
    );
    ++result.player_item_calls;
    if (result.player_item.status !=
        LegacyBattlePlayerItemQuantityStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::player_item_typed_stop;
        return false;
    }
    return true;
}

[[nodiscard]] inline bool refresh_shared_frame(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result
) {
    const auto refresh = refresh_legacy_battle_frame(port);
    if (refresh.status != LegacyBattleFrameRefreshStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::frame_refresh_typed_stop;
        return false;
    }

    return true;
}

[[nodiscard]] inline bool rebuild_shared_actor_metrics(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleStartupState* const startup,
    LegacyBattleActionDispatchResult& result
) {
    const auto metrics = rebuild_legacy_battle_actor_metrics(
        port,
        to_bits(state.group_b_count),
        to_bits(state.group_a_count),
        {.action = &state, .startup = startup}
    );
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

[[nodiscard]] inline bool rebuild_shared_actor_order(
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

[[nodiscard]] inline bool clear_framebuffer(
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result
) noexcept {
    // This path reads startup globals; an absent optional binding is a stop.
    if (context.startup == nullptr) {
        result.status =
            LegacyBattleActionDispatchStatus::framebuffer_typed_stop;
        return false;
    }

    const u32 requested_pixels =
        legacy_battle_window_fill_byte_count(*context.startup) >> 1U;
    context.screen_flash.active = 1U;
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

[[nodiscard]] inline bool update_effect_score(
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

[[nodiscard]] inline bool read_group_a_event_slot(
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

inline void write_group_a_event_slot(
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

[[nodiscard]] inline bool publish_target(
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

[[nodiscard]] inline bool query_internal_flag(
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

[[nodiscard]] inline bool clear_internal_flag(
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

inline void append_nested_actor_action_mode(
    LegacyBattleActionDispatchResult& result,
    const LegacyBattleActorActionModeResult& nested
) noexcept {
    result.actor_action_mode = nested;
    result.actor_action_modes[result.actor_action_mode_calls] = nested;
    ++result.actor_action_mode_calls;
}

inline void append_nested_actor_field_26b8_high_bit_set(
    LegacyBattleActorField26b8HighBitSetCallTrace& destination,
    const LegacyBattleActorField26b8HighBitSetCallTrace& nested
) noexcept {
    for (u32 index = 0U; index < nested.calls; ++index) {
        destination.return_addresses[destination.calls] =
            nested.return_addresses[index];
        ++destination.calls;
    }
    if (nested.calls != 0U) {
        destination.last = nested.last;
    }
}

template <typename Registers>
[[nodiscard]] inline bool apply_actor_field_26b8_high_bit_set_call(
    LegacyBattleStartupState* const startup,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    const u32 actor_token,
    const u32 return_address,
    Registers& registers
) noexcept {
    const auto& seeded = requests.calls[trace.calls];
    const bool completed =
        execute_legacy_battle_actor_field_26b8_high_bit_set_call(
            {
                .startup = startup,
            },
            trace,
            requests,
            actor_token,
            registers.eax,
            registers.edx,
            return_address,
            seeded.entry_flags,
            seeded.entry_flags_known
        );
    registers.eax = trace.last.return_eax;
    registers.ecx = trace.last.return_ecx;
    registers.edx = trace.last.return_edx;
    return completed;
}

struct ActionDispatchRunner {
    LegacyBattleActionDispatchState& state;
    LegacyBattleActionDispatchPort& port;
    LegacyBattleActionDispatchContext& context;
    u32 group_a_index;
    u32 group_b_index;
    u32 actor_token{};
    u16 action{};
    LegacyBattleActionCallReply reply{};
    LegacyBattleActionDispatchResult result{};

    [[nodiscard]] bool require_group_b();
    [[nodiscard]] u32 side_token(u32 index);
    [[nodiscard]] bool begin_action(u32 target_token);
    [[nodiscard]] bool release_actor_resource();
    [[nodiscard]] LegacyBattleActionDispatchResult run();
    [[nodiscard]] LegacyBattleActionDispatchResult dispatch_extended();
    [[nodiscard]] LegacyBattleActionDispatchResult dispatch_low();
    [[nodiscard]] LegacyBattleActionDispatchResult dispatch_high();
};

}  // namespace openswd3::battle::action_dispatch_detail
