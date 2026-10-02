#include "test.hpp"

void test_battle_group_b_action_composition_action_caller(
    openswd3::test::Context& test
);
void test_battle_group_b_action_profile_selection_action_caller(
    openswd3::test::Context& test
);
void test_battle_action_dispatch_invalid_group_a(openswd3::test::Context& test);
void test_battle_action_dispatch(openswd3::test::Context& test);
void test_battle_opponent_action_dispatch(openswd3::test::Context& test);
void test_battle_actor_runtime_reset(openswd3::test::Context& test);
void test_battle_actor_action_presentation(openswd3::test::Context& test);
void test_battle_actor_frame_presentation_entry(openswd3::test::Context& test);
void test_battle_actor_frame_original_default(openswd3::test::Context& test);
void test_battle_actor_frame_original_action_data(
    openswd3::test::Context& test
);
void test_battle_actor_frame_original_action_cpu_branches(
    openswd3::test::Context& test
);
void test_battle_actor_frame_original_reset_suffix(
    openswd3::test::Context& test
);
void test_battle_actor_frame_free_validation_prefix(
    openswd3::test::Context& test
);
void test_battle_actor_frame_free_assertion_prefix(
    openswd3::test::Context& test
);
void test_battle_actor_frame_rectangle_calls(openswd3::test::Context& test);
void test_battle_actor_frame_tsw_declared_count(openswd3::test::Context& test);
void test_battle_final_actor_step(openswd3::test::Context& test);
void test_battle_group_a_frame(openswd3::test::Context& test);
void test_battle_group_b_frame(openswd3::test::Context& test);

int main() {
    openswd3::test::Context test;
    test_battle_group_b_action_composition_action_caller(test);
    test_battle_group_b_action_profile_selection_action_caller(test);
    test_battle_action_dispatch_invalid_group_a(test);
    test_battle_action_dispatch(test);
    test_battle_opponent_action_dispatch(test);
    test_battle_actor_runtime_reset(test);
    test_battle_actor_action_presentation(test);
    test_battle_actor_frame_presentation_entry(test);
    test_battle_actor_frame_original_default(test);
    test_battle_actor_frame_original_action_data(test);
    test_battle_actor_frame_original_action_cpu_branches(test);
    test_battle_actor_frame_original_reset_suffix(test);
    test_battle_actor_frame_free_validation_prefix(test);
    test_battle_actor_frame_free_assertion_prefix(test);
    test_battle_actor_frame_rectangle_calls(test);
    test_battle_actor_frame_tsw_declared_count(test);
    test_battle_final_actor_step(test);
    test_battle_group_a_frame(test);
    test_battle_group_b_frame(test);
    return test.exit_code();
}
