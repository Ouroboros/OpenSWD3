#include "openswd3/battle/legacy_battle_post_action.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallResetActor = 0x00478850U;
constexpr u32 kCallQueryTarget = 0x004786E0U;
constexpr u32 kCallQueryTerminal = 0x0047CE80U;
constexpr u32 kCallClearActorAction = 0x00478B20U;
constexpr u32 kCallResetTarget = 0x00478AE0U;
constexpr u32 kCallSetActorMode = 0x00478710U;
constexpr u32 kCallConfigureActor = 0x00478330U;
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
    const compat::u32 source_group_a_index,
    const compat::u32 target_group_b_index
) {
    LegacyBattleActionDispatchResult result;
    const u32 selected = static_cast<u32>(action.selected_target_index);
    result.return_value = selected;
    if (selected != target_group_b_index) {
        return result;
    }

    static_cast<void>(invoke(
        port, result, kCallResetActor, {group_b_token(target_group_b_index)}
    ));
    u32 group_a_index = 0U;
    const u32 group_a_count = to_bits(action.group_a_count);
    if (group_a_count == 0U) {
        result.return_value = 0U;
        return result;
    }

    while (group_a_index < group_a_count) {
        const u32 actor_token = group_a_token(group_a_index);
        if (group_a_index != source_group_a_index) {
            const auto target =
                invoke(port, result, kCallQueryTarget, {actor_token});
            const i32 queried =
                static_cast<i32>(signed_word(static_cast<u16>(target.eax)));
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
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallResetTarget,
                                    {group_b_token(to_bits(queried))}
                                ));
                                static_cast<void>(invoke(
                                    port,
                                    result,
                                    kCallPublishTarget,
                                    {actor_token, to_bits(candidate)}
                                ));
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
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallResetTarget,
                        {group_b_token(to_bits(queried))}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallSetActorMode, {actor_token, 0U}
                    ));
                    static_cast<void>(invoke(
                        port, result, kCallConfigureActor, {actor_token, 0U}
                    ));
                    static_cast<void>(
                        invoke(port, result, kCallResetActor, {actor_token})
                    );
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
