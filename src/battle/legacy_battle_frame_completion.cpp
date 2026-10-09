#include "openswd3/battle/legacy_battle_frame_completion.hpp"

#include <algorithm>
#include <bit>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

std::optional<bool> has_completion_action(
    u32 head,
    const std::span<const LegacyBattleActorFrameLinkedNode> nodes,
    LegacyBattleFrameCompletionResult& result
) {
    while (head != 0U) {
        const auto node = std::ranges::find(
            nodes, head, &LegacyBattleActorFrameLinkedNode::token
        );
        if (node == nodes.end()) {
            result.status =
                LegacyBattleFrameCompletionStatus::action_node_typed_stop;
            result.missing_action_node = head;
            return std::nullopt;
        }

        if ((node->status_mask & 4U) != 0U) {
            return true;
        }

        head = node->next_token;
    }

    return false;
}

}  // namespace

LegacyBattleFrameCompletionResult update_legacy_battle_frame_completion(
    LegacyBattleFrameCompletionBindings bindings
) {
    LegacyBattleFrameCompletionResult result;
    if (bindings.actors.priority_actor_index != 0xFFFFFFFFU) {
        return result;
    }

    u32 group_a_count = bindings.actors.group_a_count;
    for (u32 index = 0U; index < group_a_count; ++index) {
        if (index >= bindings.startup.party.size() ||
            index >= bindings.action.group_a_action_execution.size()) {
            result.status =
                LegacyBattleFrameCompletionStatus::group_a_fields_typed_stop;
            result.stopped_index = index;
            return result;
        }

        const auto& actor = bindings.startup.party[index];
        if (bindings.action.group_a_action_execution[index]
                    .action_twenty_seven_motion_mode == 1U ||
            actor.progress.scene_identity == 1U) {
            continue;
        }

        const auto ready = has_completion_action(
            actor.base_initialization.linked_action_head_token,
            bindings.action_nodes,
            result
        );
        if (!ready) {
            result.stopped_index = index;
            return result;
        }

        if (*ready) {
            result.group_a_ready_count =
                static_cast<u8>(result.group_a_ready_count + 1U);
        }

        group_a_count = bindings.actors.group_a_count;
    }

    if (result.group_a_ready_count != 0U) {
        const auto required = static_cast<u8>(
            static_cast<u8>(group_a_count) -
            static_cast<u8>(bindings.action.phase_counter >> 16U) -
            bindings.final_actor.excluded_group_a_count
        );
        const u32 available =
            static_cast<u32>(bindings.final_actor.removed_group_a_count) +
            result.group_a_ready_count;
        if (available >= required && bindings.outcome.darkening_gate == 0U) {
            bindings.final_actor.removed_group_a_count =
                static_cast<u8>(available);
            bindings.message_state = 0x67U;
            result.group_a_committed = true;
            return result;
        }
    }

    if (bindings.startup.reset.value_53c048 != 0U) {
        return result;
    }

    u32 group_b_count = bindings.actors.group_b_count;
    for (u32 index = 0U; index < group_b_count; ++index) {
        if (!bindings.startup.group_b_lifecycle ||
            index >= bindings.startup.group_b_lifecycle->size()) {
            result.status =
                LegacyBattleFrameCompletionStatus::group_b_fields_typed_stop;
            result.stopped_index = index;
            return result;
        }

        const auto& actor = (*bindings.startup.group_b_lifecycle)[index];
        const auto ready = has_completion_action(
            actor.base_initialization.linked_action_head_token,
            bindings.action_nodes,
            result
        );
        if (!ready) {
            result.stopped_index = index;
            return result;
        }

        if (*ready) {
            result.group_b_ready_count =
                static_cast<u8>(result.group_b_ready_count + 1U);
        }

        group_b_count = bindings.actors.group_b_count;
    }

    if (result.group_b_ready_count == 0U) {
        return result;
    }

    const u32 available = (bindings.action.packed_actor_counter & 0xFFU) +
        result.group_b_ready_count;
    if (static_cast<compat::i32>(available) <
            std::bit_cast<compat::i32>(group_b_count) ||
        bindings.outcome.darkening_gate != 0U) {
        return result;
    }

    bindings.action.packed_actor_counter =
        (bindings.action.packed_actor_counter & 0xFFFFFF00U) |
        static_cast<u8>(available);
    bindings.startup.reset.value_53c048 = 1U;
    bindings.final_actor.terminal_mode = 0U;
    bindings.message_state = 0x63U;
    result.group_b_committed = true;
    return result;
}

}  // namespace openswd3::battle
