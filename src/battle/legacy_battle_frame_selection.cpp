#include "openswd3/battle/legacy_battle_frame_selection.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"

#include <bit>

namespace openswd3::battle {

LegacyBattleFrameSelectionResult prepare_legacy_battle_frame_selection(
    LegacyBattleFrameSelectionBindings bindings
) {
    LegacyBattleFrameSelectionResult result;
    auto mode = bindings.action.action_pending_aux;
    auto selected = bindings.metrics.priority_actor_index;
    if (mode == 1U && selected == 0xFFFFFFFFU) {
        mode = 0U;
        bindings.action.action_pending_aux = mode;
    }

    // A short external record mapping fails at the first queue-head load,
    // after the preceding mode correction has already been published.
    if (bindings.records.empty()) {
        result.status = LegacyBattleFrameSelectionStatus::queue_head_typed_stop;
        return result;
    }

    if (bindings.records.front().value_00 != 0xFFFFFFFFU &&
        bindings.final_actor.selection_gate == 0U &&
        bindings.action.frame_enabled == 1U && mode == 0U) {
        if (bindings.delay >= 0x10U) {
            result.dequeue = dequeue_legacy_battle_attack_order_entry({
                .records = bindings.records,
                .adjacent_intensity_records =
                    bindings.adjacent_intensity_records,
                .output =
                    {
                        .value_00 = &bindings.metrics.priority_actor_index,
                        .tail_dwords =
                            bindings.metrics.priority_actor_record_tail,
                    },
                .party = bindings.party,
                .party_actions = bindings.action.group_a_action_execution,
            });
            if (result.dequeue->status !=
                LegacyBattleAttackOrderDequeueStatus::completed) {
                result.status =
                    LegacyBattleFrameSelectionStatus::dequeue_typed_stop;
                return result;
            }

            selected = bindings.metrics.priority_actor_index;
            if (selected != 0xFFFFFFFFU) {
                bindings.delay = 0U;
                bindings.final_actor.selection_gate = 1U;
                bindings.script_workspace.coordinate_y =
                    std::bit_cast<compat::i32>(selected);
            }
        } else {
            bindings.delay = static_cast<compat::u16>(bindings.delay + 1U);
        }
    }

    if (selected == 0xFFFFFFFFU) {
        const auto queued_actor = bindings.final_actor.queued_actor_code;
        bindings.action.actor_progress_gate = 1U;
        if (queued_actor != 0U) {
            bindings.action.actor_progress_gate = 0U;
        }
    } else {
        bindings.action.actor_progress_gate = 0U;
    }

    return result;
}

}  // namespace openswd3::battle
