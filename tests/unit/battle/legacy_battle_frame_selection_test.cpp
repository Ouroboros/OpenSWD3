#include "openswd3/battle/legacy_battle_frame_selection.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"

#include <array>
#include <bit>
#include <functional>
#include <memory>

#include "test.hpp"

namespace {

using namespace openswd3::battle;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct CallbackPort final : LegacyBattleAttackOrderDequeuePort {
    std::function<LegacyBattleAttackOrderDequeueActorReply(
        const LegacyBattleAttackOrderDequeueActorRequest&
    )>
        callback;

    LegacyBattleAttackOrderDequeueActorReply query_actor(
        const LegacyBattleAttackOrderDequeueActorRequest& request
    ) override {
        return callback(request);
    }
};

struct Fixture {
    LegacyBattleActionDispatchState action;
    LegacyBattleActorMetricState metrics;
    LegacyBattleFinalActorStepState final_actor;
    LegacyBattleScriptWorkspace workspace;
    LegacyBattleStartupState startup;
    std::array<LegacyBattleStartupResetRecord, 18> records{};
    std::array<LegacyBattleIntensityEffectRecord, 8> adjacent{};
    u16 delay{16U};
    LegacyBattleAttackOrderRuntimePort port{action, startup};

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
            action, metrics, final_actor, workspace, delay, records, adjacent
        };
    }
};

}  // namespace

void test_battle_frame_selection(openswd3::test::Context& test) {
    for (const u32 mode : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        for (const u32 enabled : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            for (const u32 active : {0U, 1U, 2U, 0xFFFFFFFFU}) {
                auto f = std::make_unique<Fixture>();
                f->action.action_pending_aux = mode;
                f->action.frame_enabled = enabled;
                f->final_actor.selection_gate = active;
                f->records[0].value_00 = 5U;
                f->delay = 15U;
                const auto result = prepare_legacy_battle_frame_selection(
                    f->bindings(), f->port
                );
                const bool eligible =
                    mode <= 1U && enabled == 1U && active == 0U;
                test.expect_true(
                    result.status ==
                            LegacyBattleFrameSelectionStatus::completed &&
                        !result.dequeue_called &&
                        f->action.action_pending_aux ==
                            (mode == 1U ? 0U : mode) &&
                        f->delay == (eligible ? 16U : 15U) &&
                        f->final_actor.selection_gate == active &&
                        f->workspace.coordinate_y == -2 &&
                        f->action.actor_progress_gate == 1U,
                    "frame selection uses exact DWORD gates and WORD wait"
                );
            }
        }
    }

    for (const u32 selected : {0U, 5U, 0x80000000U, 0xFFFFFFFFU}) {
        for (const u32 queued : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            auto f = std::make_unique<Fixture>();
            f->action.action_pending_aux = 1U;
            f->metrics.priority_actor_index = selected;
            f->final_actor.queued_actor_code = queued;
            const auto result =
                prepare_legacy_battle_frame_selection(f->bindings(), f->port);
            test.expect_true(
                !result.dequeue_called && f->delay == 16U &&
                    f->action.action_pending_aux ==
                        (selected == 0xFFFFFFFFU ? 0U : 1U) &&
                    f->workspace.coordinate_y == -2 &&
                    f->action.actor_progress_gate ==
                        (selected == 0xFFFFFFFFU && queued == 0U ? 1U : 0U),
                "empty queue preserves wait and uses both interaction inputs"
            );
        }
    }

    {
        auto f = std::make_unique<Fixture>();
        f->records[0].value_00 = 8U;
        f->action.group_a_action_execution[0].special_mode = 1U;
        const auto result =
            prepare_legacy_battle_frame_selection(f->bindings(), f->port);
        test.expect_true(
            result.dequeue_called && result.dequeue.output_dwords == 7U &&
                f->metrics.priority_actor_index == 0xFFFFFFFFU &&
                f->delay == 16U && f->final_actor.selection_gate == 0U &&
                f->workspace.coordinate_y == -2 &&
                f->action.actor_progress_gate == 1U &&
                f->records[0].value_00 == 8U,
            "dequeued empty record preserves wait active and script coordinate"
        );
    }

    for (const bool returned : {false, true}) {
        auto f = std::make_unique<Fixture>();
        f->records[0].value_00 = 8U;
        CallbackPort port;
        port.callback = [&](const auto& request) {
            f->metrics.priority_actor_index = 17U;
            f->records[0].value_00 = 0x80000000U;
            f->action.frame_enabled = 0U;
            f->action.action_pending_aux = 2U;
            f->final_actor.selection_gate = 3U;
            f->final_actor.queued_actor_code = 4U;
            return LegacyBattleAttackOrderDequeueActorReply{
                .eax = 0U,
                .ecx = request.actor_token,
                .edx = request.stale_edx,
                .callee_returned = returned
            };
        };
        const auto result =
            prepare_legacy_battle_frame_selection(f->bindings(), port);
        test.expect_true(
            result.status ==
                    (returned ? LegacyBattleFrameSelectionStatus::completed
                              : LegacyBattleFrameSelectionStatus::
                                    dequeue_typed_stop) &&
                f->action.frame_enabled == 0U &&
                f->action.action_pending_aux == 2U &&
                f->metrics.priority_actor_index ==
                    (returned ? 0x80000000U : 17U) &&
                f->final_actor.selection_gate == (returned ? 1U : 3U) &&
                f->workspace.coordinate_y ==
                    (returned
                         ? std::bit_cast<openswd3::compat::i32>(0x80000000U)
                         : -2) &&
                f->delay == (returned ? 0U : 16U) &&
                f->action.actor_progress_gate == (returned ? 0U : 99U),
            "query completion rereads output and failure preserves callback prefix"
        );
    }

    for (const u16 delay : {u16{0U}, u16{15U}, u16{16U}, u16{0xFFFFU}}) {
        auto f = std::make_unique<Fixture>();
        f->delay = delay;
        f->records[0] = {5U, 11U, 0x1234U, 0xABCDU, 13U, 14U, 15U, 16U};
        const auto result =
            prepare_legacy_battle_frame_selection(f->bindings(), f->port);
        const bool dequeued = delay >= 16U;
        test.expect_true(
            result.status == LegacyBattleFrameSelectionStatus::completed &&
                result.dequeue_called == dequeued &&
                f->delay == (dequeued ? 0U : delay + 1U) &&
                f->metrics.priority_actor_index ==
                    (dequeued ? 5U : 0xFFFFFFFFU) &&
                f->final_actor.selection_gate == (dequeued ? 1U : 0U) &&
                f->workspace.coordinate_y == (dequeued ? 5 : -2) &&
                f->action.actor_progress_gate == (dequeued ? 0U : 1U),
            "frame selection dequeues at unsigned WORD threshold sixteen"
        );
        if (dequeued) {
            test.expect_true(
                f->metrics.priority_actor_record_tail ==
                    std::array<u32, 6>{11U, 0xABCD1234U, 13U, 14U, 15U, 16U},
                "frame selection publishes all seven physical DWORDs"
            );
        }
    }

    for (const u32 bits : {0U, 0x40U, 0x4000U}) {
        for (const u32 special : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            auto f = std::make_unique<Fixture>();
            f->records[0].value_00 = 8U;
            f->records[1].value_00 = 5U;
            f->startup.party[0].progress.mode_gate = bits;
            f->action.group_a_action_execution[0].special_mode = special;
            const auto result =
                prepare_legacy_battle_frame_selection(f->bindings(), f->port);
            const bool skipped = (bits & 0x40U) != 0U || special == 1U;
            test.expect_true(
                result.status == LegacyBattleFrameSelectionStatus::completed &&
                    result.dequeue.actor_query_calls == 1U &&
                    f->metrics.priority_actor_index == (skipped ? 5U : 8U),
                "live dequeue query reads byte bit six and exact special mode"
            );
        }
    }

    {
        auto f = std::make_unique<Fixture>();
        f->records[0].value_00 = 7U;
        const auto result =
            prepare_legacy_battle_frame_selection(f->bindings(), f->port);
        test.expect_true(
            result.status ==
                    LegacyBattleFrameSelectionStatus::dequeue_typed_stop &&
                result.dequeue.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        actor_query_typed_stop &&
                result.dequeue.output_dwords == 0U &&
                f->records[0].value_00 == 7U && f->delay == 16U &&
                f->metrics.priority_actor_index == 0xFFFFFFFFU &&
                f->workspace.coordinate_y == -2 &&
                f->action.actor_progress_gate == 99U,
            "unmapped actor query stops before record copies and frame stores"
        );
    }

    {
        auto f = std::make_unique<Fixture>();
        f->records[0].value_00 = 0x40000008U;
        const auto result =
            prepare_legacy_battle_frame_selection(f->bindings(), f->port);
        test.expect_true(
            result.status == LegacyBattleFrameSelectionStatus::completed &&
                result.dequeue.actor_query_calls == 1U &&
                f->metrics.priority_actor_index == 0x40000008U &&
                f->workspace.coordinate_y == 0x40000008,
            "wrapped actor code resolves the actual fixed address"
        );
    }

    {
        auto f = std::make_unique<Fixture>();
        f->action.action_pending_aux = 1U;
        auto bindings = f->bindings();
        bindings.records = {};
        const auto result =
            prepare_legacy_battle_frame_selection(bindings, f->port);
        test.expect_true(
            result.status ==
                    LegacyBattleFrameSelectionStatus::queue_head_typed_stop &&
                f->action.action_pending_aux == 0U &&
                f->action.actor_progress_gate == 99U,
            "missing queue retains mode correction before first failed load"
        );
    }
}
