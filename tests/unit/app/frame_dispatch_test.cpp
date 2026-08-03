#include "test.hpp"

#include "openswd3/app/frame_dispatch.hpp"

namespace {

using openswd3::app::ActiveFrameBranch;
using openswd3::app::ActiveFrameState;
using openswd3::app::BattleExitAction;
using openswd3::app::FrameEntryAction;
using openswd3::app::FrameEntryState;
using openswd3::app::IdleAction;
using openswd3::app::IdleState;
using openswd3::app::SpecialModeHandler;
using openswd3::app::StoryGateState;
using openswd3::test::Context;

void test_idle_dispatch(Context& test) {
    test.expect_equal(
        openswd3::app::select_idle_action({1, 0x21, 1, 0}),
        IdleAction::step_video_then_audio,
        "video precedes both idle suppression gates"
    );
    test.expect_equal(
        openswd3::app::select_idle_action({1, 0x01, 0, 1}),
        IdleAction::yield,
        "process idle suppression yields"
    );
    test.expect_equal(
        openswd3::app::select_idle_action({1, 0, 1, 1}),
        IdleAction::yield,
        "transition suppression yields"
    );
    test.expect_equal(
        openswd3::app::select_idle_action({1, 0, 0, 0}),
        IdleAction::step_game_frame,
        "running state reaches the game frame"
    );
    test.expect_equal(
        openswd3::app::select_idle_action({0, 0x20, 0, 1}),
        IdleAction::present_pause,
        "disabled frame gate ignores video and presents pause when display equals one"
    );
    test.expect_equal(
        openswd3::app::select_idle_action({0, 0, 0, 2}),
        IdleAction::yield,
        "pause presentation uses the original display-active equals-one test"
    );
}

void test_frame_entry(Context& test) {
    test.expect_equal(
        openswd3::app::select_frame_entry_action({0x10, 1}),
        FrameEntryAction::return_immediately,
        "process bit 0x10 returns before display handling"
    );
    test.expect_equal(
        openswd3::app::select_frame_entry_action({0, 0}),
        FrameEntryAction::yield,
        "inactive display yields"
    );
    test.expect_equal(
        openswd3::app::select_frame_entry_action({0, 2}),
        FrameEntryAction::sample_time,
        "frame entry accepts any nonzero display value"
    );
}

void test_active_frame_branch(Context& test) {
    test.expect_equal(
        openswd3::app::select_active_frame_branch({1, 1, 2}),
        ActiveFrameBranch::high_priority,
        "high-priority state excludes battle and special modes"
    );
    test.expect_equal(
        openswd3::app::select_active_frame_branch({0, 1, 2}),
        ActiveFrameBranch::battle,
        "battle excludes world and special modes"
    );
    test.expect_equal(
        openswd3::app::select_active_frame_branch({0, 0, 0}),
        ActiveFrameBranch::world,
        "zero special-mode state selects world"
    );
    test.expect_equal(
        openswd3::app::select_active_frame_branch({0, 0, 0x80000004U}),
        ActiveFrameBranch::special_mode,
        "nonzero tagged mode selects special-mode dispatch"
    );
}

void test_story_gate(Context& test) {
    const StoryGateState open{1, 0, 0, 0, 0};
    test.expect_true(openswd3::app::should_step_story(open), "all five gates open");
    test.expect_false(
        openswd3::app::should_step_story({0, 0, 0, 0, 0}),
        "frame execution gate is required"
    );
    test.expect_false(
        openswd3::app::should_step_story({1, 1, 0, 0, 0}),
        "transition suppression blocks story"
    );
    test.expect_false(
        openswd3::app::should_step_story({1, 0, 1, 0, 0}),
        "special mode blocks story"
    );
    test.expect_false(
        openswd3::app::should_step_story({1, 0, 0, 1, 0}),
        "battle blocks story"
    );
    test.expect_false(
        openswd3::app::should_step_story({1, 0, 0, 0, 1}),
        "high-priority state blocks story"
    );
}

void test_mode_and_battle_results(Context& test) {
    test.expect_equal(
        openswd3::app::select_special_mode_handler(0x80000002U),
        SpecialModeHandler::shop_mode_2,
        "tagged mode two selects shop"
    );
    for (const auto mode : {1U, 3U, 4U, 5U, 6U}) {
        test.expect_equal(
            openswd3::app::select_special_mode_handler(0x80000000U | mode),
            SpecialModeHandler::standard_modes_1_3_4_5_6,
            "tagged standard mode selects the shared handler"
        );
    }
    test.expect_equal(
        openswd3::app::select_special_mode_handler(7),
        SpecialModeHandler::none,
        "out-of-range low mode has no handler"
    );

    test.expect_equal(
        openswd3::app::select_battle_exit_action(0),
        BattleExitAction::restore_world_for_result_0,
        "battle result zero restores world"
    );
    test.expect_equal(
        openswd3::app::select_battle_exit_action(2),
        BattleExitAction::request_special_mode_4_for_result_2,
        "battle result two requests tagged mode four"
    );
    test.expect_equal(
        openswd3::app::select_battle_exit_action(3),
        BattleExitAction::remap_world_for_result_3,
        "battle result three remaps world"
    );
    test.expect_equal(
        openswd3::app::select_battle_exit_action(1),
        BattleExitAction::keep_battle_active_and_return,
        "battle result one keeps battle active"
    );
    test.expect_equal(
        openswd3::app::select_battle_exit_action(-1),
        BattleExitAction::keep_battle_active_and_return,
        "all other nonzero results keep the original direct return behavior"
    );
}

void test_close_request(Context& test) {
    test.expect_false(openswd3::app::should_request_close(0), "zero flags stay open");
    test.expect_true(
        openswd3::app::should_request_close(0x24),
        "close bit remains visible alongside video bit"
    );
}

}  // namespace

int main() {
    Context test;
    test_idle_dispatch(test);
    test_frame_entry(test);
    test_active_frame_branch(test);
    test_story_gate(test);
    test_mode_and_battle_results(test);
    test_close_request(test);
    return test.exit_code();
}
