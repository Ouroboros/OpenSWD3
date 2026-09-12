#include "openswd3/battle/legacy_battle_post_action.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallResetActor = 0x00478850U;
constexpr u32 kCallQueryTerminal = 0x0047CE80U;
constexpr u32 kCallClearActorAction = 0x00478B20U;
constexpr u32 kCallResetTarget = 0x00478AE0U;
constexpr u32 kCallSetActorMode = 0x00478710U;
constexpr u32 kCallPublishTarget = 0x00478A70U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        index * kLegacyBattleActionGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleActionGroupBBaseToken +
        index * kLegacyBattleActionGroupBStride;
}

[[nodiscard]] constexpr bool even_parity(const compat::u8 value) noexcept {
    return (std::popcount(value) & 1) == 0;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<compat::u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] LegacyBattleActionCallReply invoke(
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchResult& result,
    const u32 callee,
    const std::array<u32, 8>& arguments = {}
) {
    ++result.port_calls;
    return port.invoke({.callee_token = callee, .arguments = arguments});
}

}  // namespace

LegacyBattleActionDispatchResult advance_legacy_battle_post_action(
    LegacyBattlePostActionState& state,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleStartupState* const startup,
    const compat::u32 source_group_a_index,
    const compat::u32 target_group_b_index,
    const LegacyBattleActorActionTargetRequest& action_target_request
) {
    LegacyBattleActionDispatchResult result;
    const u32 selected = action.selected_target_index;
    result.return_value = selected;
    if (selected != target_group_b_index) {
        return result;
    }

    const auto initial_reset = invoke(
        port, result, kCallResetActor, {group_b_token(target_group_b_index)}
    );
    u32 action_target_entry_edx = initial_reset.edx;
    u32 group_a_index = 0U;
    const u32 group_a_count = to_bits(action.group_a_count);
    if (group_a_count == 0U) {
        result.return_value = 0U;
        return result;
    }

    while (group_a_index < group_a_count) {
        const u32 actor_token = group_a_token(group_a_index);
        if (group_a_index != source_group_a_index) {
            auto request = action_target_request;
            request.actor_token = actor_token;
            request.entry_eax = group_a_index;
            request.entry_edx = action_target_entry_edx;
            request.entry_return_address = 0x0045AE51U;
            request.entry_flags =
                subtract_flags(group_a_index, source_group_a_index);
            request.entry_flags_known = true;
            result.actor_action_target =
                query_legacy_battle_actor_action_target(
                    resolve_legacy_battle_actor_action_target(
                        {.action = &action, .startup = startup}, actor_token
                    ),
                    request
                );
            result.actor_action_targets[result.actor_action_target_calls] =
                result.actor_action_target;
            ++result.actor_action_target_calls;
            if (result.actor_action_target.status !=
                LegacyBattleActorActionTargetStatus::completed) {
                result.status = LegacyBattleActionDispatchStatus::
                    actor_action_target_typed_stop;
                result.return_value = result.actor_action_target.return_eax;
                return result;
            }
            const i32 queried = static_cast<i32>(signed_word(
                static_cast<u16>(result.actor_action_target.return_eax)
            ));
            if ((to_bits(queried) & 0x00008000U) == 0U &&
                selected == to_bits(queried)) {
                i32 observed_group_b_count = action.group_b_count;
                bool published = false;
                if (observed_group_b_count > 0) {
                    i32 candidate = 0;
                    while (candidate < observed_group_b_count) {
                        if (to_bits(candidate) != selected) {
                            const u32 candidate_token =
                                group_b_token(to_bits(candidate));
                            if (invoke(
                                    port,
                                    result,
                                    kCallQueryTerminal,
                                    {candidate_token}
                                )
                                    .eax == 0U) {
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallClearActorAction,
                                    {actor_token}
                                ));
                                action.group_a_action_execution[group_a_index]
                                    .action_target = 0xFFFFU;
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallResetTarget,
                                    {group_b_token(to_bits(queried))}
                                ));
                                const auto publication = invoke(
                                    port,
                                    result,
                                    kCallPublishTarget,
                                    {actor_token, to_bits(candidate)}
                                );
                                action.group_a_action_execution[group_a_index]
                                    .action_target =
                                    static_cast<u16>(candidate);
                                action_target_entry_edx = publication.edx;
                                state.selection_rebuild_pending = 1U;
                                published = true;
                                break;
                            }
                            observed_group_b_count = action.group_b_count;
                        }
                        ++candidate;
                    }
                }

                if (!published &&
                    static_cast<u32>(action.packed_actor_counter & 0xFFU) +
                            1U ==
                        to_bits(observed_group_b_count)) {
                    static_cast<void>(invoke(
                        port, result, kCallClearActorAction, {actor_token}
                    ));
                    action.group_a_action_execution[group_a_index]
                        .action_target = 0xFFFFU;
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallResetTarget,
                        {group_b_token(to_bits(queried))}
                    ));
                    const auto mode = invoke(
                        port, result, kCallSetActorMode, {actor_token, 0U}
                    );
                    result.actor_availability_block =
                        set_legacy_battle_actor_availability_block(
                            &final_actor
                                 .group_a_availability_blocks[group_a_index],
                            {
                                .value = 0U,
                                .actor_token = actor_token,
                                .entry_eax = mode.eax,
                                .entry_edx = mode.edx,
                            }
                        );
                    ++result.actor_availability_block_calls;
                    if (result.actor_availability_block.status !=
                        LegacyBattleActorAvailabilityBlockStatus::completed) {
                        result.status = LegacyBattleActionDispatchStatus::
                            actor_availability_block_typed_stop;
                        result.return_value =
                            result.actor_availability_block.return_eax;
                        return result;
                    }
                    const auto final_reset =
                        invoke(port, result, kCallResetActor, {actor_token});
                    action_target_entry_edx = final_reset.edx;
                    final_actor.actor_order.fill(0U);
                    state.published_target_token = 0U;
                    final_actor.secondary_actor_code = 0U;
                    final_actor.queued_actor_code = 0U;
                    final_actor.active_actor_code = 0xFFFFFFFFU;
                    state.selection_workspace.fill(0U);
                }
            }
        }
        ++group_a_index;
        ++result.group_a_iterations;
    }
    result.return_value = group_a_index;
    return result;
}

}  // namespace openswd3::battle
