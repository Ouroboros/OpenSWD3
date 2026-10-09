#include "openswd3/battle/legacy_battle_frame_completion.hpp"
#include "test.hpp"

#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleFrameCompletionStatus;
using openswd3::battle::update_legacy_battle_frame_completion;
using openswd3::compat::u32;

struct Fixture {
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup_owner{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState>
        action_owner{std::make_unique<
            openswd3::battle::LegacyBattleActionDispatchState>()};
    openswd3::battle::LegacyBattleStartupState& startup{*startup_owner};
    openswd3::battle::LegacyBattleActionDispatchState& action{*action_owner};
    openswd3::battle::LegacyBattleActorMetricState& actors{
        startup.actor_metrics
    };
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleOutcomeResolutionState outcome;
    std::vector<openswd3::battle::LegacyBattleActorFrameLinkedNode> nodes;
    u32 message_state{};

    Fixture() {
        actors.priority_actor_index = 0xFFFFFFFFU;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
    }

    openswd3::battle::LegacyBattleFrameCompletionBindings bindings() {
        return {
            .actors = actors,
            .final_actor = final_actor,
            .action = action,
            .outcome = outcome,
            .startup = startup,
            .message_state = message_state,
            .action_nodes = nodes,
        };
    }
};

}  // namespace

void test_battle_frame_completion(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 7U;
        fixture.actors.group_a_count = 0xFFFFFFFFU;
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.status == LegacyBattleFrameCompletionStatus::completed &&
                !result.group_a_committed && !result.group_b_committed &&
                fixture.message_state == 0U,
            "a selected actor skips both groups before any actor or node access"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_a_count = 3U;
        fixture.action.group_a_action_execution[0]
            .action_twenty_seven_motion_mode = 1U;
        fixture.startup.party[1].progress.scene_identity = 1U;
        fixture.action.group_a_action_execution[2]
            .action_twenty_seven_motion_mode = 2U;
        fixture.startup.party[2].progress.scene_identity = 0xFFFFFFFFU;
        fixture.startup.party[2].base_initialization.linked_action_head_token =
            1U;
        fixture.nodes = {{1U, 0U, 0x10004U}};
        fixture.action.phase_counter = 0x01010000U;
        fixture.final_actor.excluded_group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 0xFFU;
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.group_a_committed && !result.group_b_committed &&
                result.group_a_ready_count == 1U &&
                fixture.final_actor.removed_group_a_count == 0U &&
                fixture.message_state == 0x67U,
            "group A reads actual skip fields with exact-one tests and wraps the removed byte when publishing message 103"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_a_count = 1U;
        fixture.startup.party[0].base_initialization.linked_action_head_token =
            1U;
        fixture.nodes = {{1U, 2U, 0x40000U}, {2U, 99U, 4U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.group_a_committed && result.group_a_ready_count == 1U &&
                result.missing_action_node == 0U,
            "the query follows next pointers and stops on mask four before reading an invalid trailing link"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_a_count = 11U;
        for (auto& actor : fixture.startup.party) {
            actor.progress.scene_identity = 1U;
        }

        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.status ==
                    LegacyBattleFrameCompletionStatus::
                        group_a_fields_typed_stop &&
                result.stopped_index == 10U && fixture.message_state == 0U,
            "the eleventh group-A field access retains the existing boundary stop"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_b_count = 2U;
        fixture.action.packed_actor_counter = 0xAABBCCFFU;
        fixture.final_actor.terminal_mode = 7U;
        (*fixture.startup.group_b_lifecycle)[0]
            .base_initialization.linked_action_head_token = 1U;
        (*fixture.startup.group_b_lifecycle)[1]
            .base_initialization.linked_action_head_token = 2U;
        fixture.nodes = {{1U, 0U, 4U}, {2U, 0U, 4U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.group_b_committed && !result.group_a_committed &&
                result.group_b_ready_count == 2U &&
                fixture.action.packed_actor_counter == 0xAABBCC01U &&
                fixture.startup.reset.value_53c048 == 1U &&
                fixture.final_actor.terminal_mode == 0U &&
                fixture.message_state == 0x63U,
            "group B reads both owned actor lists and wraps only the packed low byte when publishing message 99"
        );
    }

    for (const u32 removed : {0U, 254U}) {
        Fixture fixture;
        fixture.actors.group_a_count = 1U;
        fixture.action.phase_counter = 0x00020000U;
        fixture.final_actor.removed_group_a_count =
            static_cast<openswd3::compat::u8>(removed);
        fixture.startup.party[0].base_initialization.linked_action_head_token =
            1U;
        fixture.nodes = {{1U, 0U, 4U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.group_a_committed == (removed == 254U) &&
                result.group_a_ready_count == 1U &&
                fixture.message_state == (removed == 254U ? 0x67U : 0U),
            "group A compares against 255 after byte subtraction underflows"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_b_count = 1U;
        auto result = update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.status == LegacyBattleFrameCompletionStatus::completed &&
                !result.group_b_committed && result.group_b_ready_count == 0U,
            "an empty actor list needs no node allocation and does not complete the group"
        );
        fixture.startup.group_b_lifecycle.reset();
        result = update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.status ==
                    LegacyBattleFrameCompletionStatus::
                        group_b_fields_typed_stop &&
                result.stopped_index == 0U && fixture.message_state == 0U,
            "a nonzero group count cannot silently use missing actor storage"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_b_count = 1U;
        fixture.outcome.darkening_gate = 1U;
        (*fixture.startup.group_b_lifecycle)[0]
            .base_initialization.linked_action_head_token = 1U;
        fixture.nodes = {{1U, 0U, 4U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            !result.group_b_committed && result.group_b_ready_count == 1U &&
                fixture.startup.reset.value_53c048 == 0U &&
                fixture.message_state == 0U,
            "darkening suppresses group-B completion without publishing its latch or message"
        );
    }

    for (const u32 mask : {0U, 2U, 0x40000U, 4U}) {
        Fixture fixture;
        fixture.actors.group_b_count = 1U;
        (*fixture.startup.group_b_lifecycle)[0]
            .base_initialization.linked_action_head_token = 1U;
        fixture.nodes = {{1U, 0U, mask ^ 4U}};
        auto bindings = fixture.bindings();
        fixture.nodes[0].status_mask = mask;
        const auto result = update_legacy_battle_frame_completion(bindings);
        test.expect_true(
            result.group_b_committed == (mask == 4U) &&
                fixture.message_state == (mask == 4U ? 0x63U : 0U),
            "only the actual node mask bit four completes the group"
        );
    }

    for (const u32 darkening : {0U, 1U}) {
        Fixture fixture;
        fixture.actors.group_a_count = 1U;
        fixture.actors.group_b_count = 1U;
        fixture.outcome.darkening_gate = darkening;
        fixture.startup.reset.value_53c048 = 1U;
        fixture.startup.party[0].base_initialization.linked_action_head_token =
            1U;
        fixture.nodes = {{1U, 0U, 4U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.group_a_committed == (darkening == 0U) &&
                !result.group_b_committed &&
                fixture.message_state == (darkening == 0U ? 0x67U : 0U),
            "darkening blocks group A and the shared completion latch blocks the group-B fallback"
        );
    }

    {
        Fixture fixture;
        fixture.actors.group_a_count = 2U;
        fixture.startup.party[0].base_initialization.linked_action_head_token =
            1U;
        fixture.startup.party[1].base_initialization.linked_action_head_token =
            2U;
        fixture.nodes = {{1U, 0U, 4U}, {2U, 3U, 0U}};
        const auto result =
            update_legacy_battle_frame_completion(fixture.bindings());
        test.expect_true(
            result.status ==
                    LegacyBattleFrameCompletionStatus::action_node_typed_stop &&
                result.stopped_index == 1U &&
                result.missing_action_node == 3U &&
                result.group_a_ready_count == 1U && fixture.message_state == 0U,
            "missing node storage preserves the scanned prefix and cannot become a fabricated false query"
        );
    }
}
