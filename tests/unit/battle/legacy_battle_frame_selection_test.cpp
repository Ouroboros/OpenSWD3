#include "openswd3/battle/legacy_battle_frame_selection.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"

#include <array>
#include <limits>
#include <memory>

#include "test.hpp"

namespace {

using namespace openswd3::battle;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct Fixture {
    LegacyBattleActionDispatchState action;
    LegacyBattleActorMetricState metrics;
    LegacyBattleFinalActorStepState final_actor;
    LegacyBattleScriptWorkspace workspace;
    LegacyBattleStartupState startup;
    std::array<LegacyBattleStartupResetRecord, 18> records{};
    std::array<LegacyBattleIntensityEffectRecord, 8> adjacent{};
    u16 delay{16U};

    Fixture() {
        for (auto& record : records) {
            record.value_00 = 0xFFFFFFFFU;
        }

        metrics.priority_actor_index = 0xFFFFFFFFU;
        final_actor.selection_gate = 0U;
        final_actor.queued_actor_code = 0U;
        workspace.coordinate_y = -2;
        action.actor_progress_gate = 99U;
    }

    LegacyBattleFrameSelectionBindings bindings() {
        return {
            action,
            metrics,
            final_actor,
            workspace,
            delay,
            records,
            adjacent,
            startup.party
        };
    }
};

}  // namespace

void test_battle_frame_selection(openswd3::test::Context& test) {
    for (const u32 mode : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        for (const u32 enabled : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            for (const u32 active : {0U, 1U, 2U, 0xFFFFFFFFU}) {
                auto fixture = std::make_unique<Fixture>();
                fixture->action.action_pending_aux = mode;
                fixture->action.frame_enabled = enabled;
                fixture->final_actor.selection_gate = active;
                fixture->records[0].value_00 = 5U;
                fixture->delay = 15U;
                const auto result =
                    prepare_legacy_battle_frame_selection(fixture->bindings());
                const bool eligible =
                    mode <= 1U && enabled == 1U && active == 0U;
                test.expect_true(
                    result.status ==
                            LegacyBattleFrameSelectionStatus::completed &&
                        !result.dequeue &&
                        fixture->action.action_pending_aux ==
                            (mode == 1U ? 0U : mode) &&
                        fixture->delay == (eligible ? 16U : 15U) &&
                        fixture->final_actor.selection_gate == active &&
                        fixture->workspace.coordinate_y == -2 &&
                        fixture->action.actor_progress_gate == 1U,
                    "frame selection uses exact DWORD gates and WORD wait"
                );
            }
        }
    }

    for (const u32 selected : {0U, 5U, 0x80000000U, 0xFFFFFFFFU}) {
        for (const u32 queued : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            auto fixture = std::make_unique<Fixture>();
            fixture->action.action_pending_aux = 1U;
            fixture->metrics.priority_actor_index = selected;
            fixture->final_actor.queued_actor_code = queued;
            const auto result =
                prepare_legacy_battle_frame_selection(fixture->bindings());
            test.expect_true(
                !result.dequeue && fixture->delay == 16U &&
                    fixture->action.action_pending_aux ==
                        (selected == 0xFFFFFFFFU ? 0U : 1U) &&
                    fixture->workspace.coordinate_y == -2 &&
                    fixture->action.actor_progress_gate ==
                        (selected == 0xFFFFFFFFU && queued == 0U ? 1U : 0U),
                "empty queue preserves wait and uses both interaction inputs"
            );
        }
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 8U;
        fixture->action.group_a_action_execution[0].special_mode = 1U;
        const auto result =
            prepare_legacy_battle_frame_selection(fixture->bindings());
        test.expect_true(
            result.dequeue && result.dequeue->output_dwords == 7U &&
                fixture->metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture->delay == 16U &&
                fixture->final_actor.selection_gate == 0U &&
                fixture->workspace.coordinate_y == -2 &&
                fixture->action.actor_progress_gate == 1U &&
                fixture->records[0].value_00 == 8U,
            "dequeued empty record preserves wait active and script coordinate"
        );
    }

    for (const u16 delay : {u16{0U}, u16{15U}, u16{16U}, u16{0xFFFFU}}) {
        auto fixture = std::make_unique<Fixture>();
        fixture->delay = delay;
        fixture->records[0] = {5U, 11U, 0x1234U, 0xABCDU, 13U, 14U, 15U, 16U};
        const auto result =
            prepare_legacy_battle_frame_selection(fixture->bindings());
        const bool dequeued = delay >= 16U;
        test.expect_true(
            result.status == LegacyBattleFrameSelectionStatus::completed &&
                result.dequeue.has_value() == dequeued &&
                fixture->delay == (dequeued ? 0U : delay + 1U) &&
                fixture->metrics.priority_actor_index ==
                    (dequeued ? 5U : 0xFFFFFFFFU) &&
                fixture->final_actor.selection_gate == (dequeued ? 1U : 0U) &&
                fixture->workspace.coordinate_y == (dequeued ? 5 : -2) &&
                fixture->action.actor_progress_gate == (dequeued ? 0U : 1U),
            "frame selection dequeues at unsigned WORD threshold sixteen"
        );
        if (dequeued) {
            test.expect_true(
                fixture->metrics.priority_actor_record_tail ==
                    std::array<u32, 6>{11U, 0xABCD1234U, 13U, 14U, 15U, 16U},
                "frame selection publishes all seven physical DWORDs"
            );
        }
    }

    for (const u32 bits : {0U, 0x40U, 0x4000U}) {
        for (const u32 special : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            auto fixture = std::make_unique<Fixture>();
            fixture->records[0].value_00 = 8U;
            fixture->records[1].value_00 = 5U;
            fixture->startup.party[0].progress.mode_gate = bits;
            fixture->action.group_a_action_execution[0].special_mode = special;
            const auto result =
                prepare_legacy_battle_frame_selection(fixture->bindings());
            const bool skipped = (bits & 0x40U) != 0U || special == 1U;
            test.expect_true(
                result.status == LegacyBattleFrameSelectionStatus::completed &&
                    result.dequeue &&
                    result.dequeue->selected_index == (skipped ? 1U : 0U) &&
                    fixture->metrics.priority_actor_index ==
                        (skipped ? 5U : 8U),
                "live dequeue reads byte bit six and exact special state"
            );
        }
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->action.action_pending_aux = 1U;
        fixture->records[0].value_00 = 7U;
        const auto result =
            prepare_legacy_battle_frame_selection(fixture->bindings());
        test.expect_true(
            result.status ==
                    LegacyBattleFrameSelectionStatus::dequeue_typed_stop &&
                result.dequeue &&
                result.dequeue->status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        actor_query_typed_stop &&
                result.dequeue->output_dwords == 0U &&
                fixture->records[0].value_00 == 7U && fixture->delay == 16U &&
                fixture->action.action_pending_aux == 0U &&
                fixture->metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture->workspace.coordinate_y == -2 &&
                fixture->action.actor_progress_gate == 99U,
            "unmapped actor retains mode correction and stops before frame stores"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 0x40000008U;
        const auto result =
            prepare_legacy_battle_frame_selection(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleFrameSelectionStatus::completed &&
                result.dequeue && result.dequeue->selected_index == 0U &&
                fixture->metrics.priority_actor_index == 0x40000008U &&
                fixture->workspace.coordinate_y == 0x40000008,
            "wrapped actor code resolves the actual fixed address"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 0x80000000U;
        const auto result =
            prepare_legacy_battle_frame_selection(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleFrameSelectionStatus::completed &&
                fixture->metrics.priority_actor_index == 0x80000000U &&
                fixture->workspace.coordinate_y ==
                    std::numeric_limits<openswd3::compat::i32>::min() &&
                fixture->delay == 0U &&
                fixture->final_actor.selection_gate == 1U,
            "selection rereads copied output and publishes its signed bit pattern"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->action.action_pending_aux = 1U;
        auto bindings = fixture->bindings();
        bindings.records = {};
        const auto result = prepare_legacy_battle_frame_selection(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleFrameSelectionStatus::queue_head_typed_stop &&
                !result.dequeue && fixture->action.action_pending_aux == 0U &&
                fixture->action.actor_progress_gate == 99U,
            "missing queue retains mode correction before first failed load"
        );
    }
}
