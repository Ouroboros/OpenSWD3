#include "openswd3/battle/legacy_battle_menu_input_finalize.hpp"

#include <algorithm>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::battle::LegacyBattleMenuInputFinalizeBindings;
using openswd3::compat::u32;

class FinalizePort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        if (calls.size() <= replies.size()) {
            return replies[calls.size() - 1U];
        }
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<LegacyBattleInputDispatchCallReply> replies;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    FinalizePort port;

    Fixture() {
        final_actor.active_actor_code = 8U;
    }

    [[nodiscard]] LegacyBattleMenuInputFinalizeBindings bindings() {
        return {
            .startup_reset = startup,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .metrics = metrics,
            .input_dispatch = input,
            .message_state = message,
        };
    }
};

[[nodiscard]] bool all_zero(const auto& values) {
    return std::ranges::all_of(values, [](const auto value) {
        return value == 0U;
    });
}

}  // namespace

void test_battle_menu_input_finalize(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuInputFinalizeStatus;
    using openswd3::battle::finalize_legacy_battle_menu_input;

    {
        Fixture fixture;
        fixture.input.selected_actor_cleanup_gate = 1U;
        fixture.input.selected_group_b_actor_code = 1U;
        fixture.input.selection_cache_gate_a = 9U;
        fixture.input.selection_cache_gate_b = 9U;
        fixture.startup.value_4ff0b0 = 9U;
        fixture.startup.value_4ff0b4 = 9U;
        fixture.startup.value_4ff0b8 = 9U;
        fixture.startup.value_53bf22 = 9U;
        fixture.frame.selection_actor_code = 9U;
        fixture.port.replies.push_back({
            .eax = 0xAAU,
            .ecx = 0xBBU,
            .edx = 0xCCU,
        });
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x55U}
        );
        test.expect_true(
            result.status == LegacyBattleMenuInputFinalizeStatus::completed &&
                fixture.message == 0U &&
                fixture.input.mouse_action_gate == 0U &&
                fixture.input.selected_actor_cleanup_gate == 0U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.startup.value_4ff0b0 == 0U &&
                fixture.startup.value_4ff0b4 == 0U &&
                fixture.startup.value_4ff0b8 == 0U &&
                fixture.startup.value_53bf22 == 0U &&
                fixture.frame.selection_actor_code == 0xFFFFFFFFU &&
                result.return_eax == 0xAAU && result.return_ecx == 0xBBU &&
                result.return_edx == 0U && fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].eax == 0x159U &&
                fixture.port.calls[0U].ecx == 0x00525508U &&
                fixture.port.calls[0U].edx == 0x55U,
            "selected group-B cleanup calls its one-based actor then clears the original cache set"
        );
    }

    {
        Fixture fixture;
        fixture.message = 9U;
        fixture.input.selected_actor_cleanup_gate = 1U;
        fixture.input.selected_group_b_actor_code = 0U;
        fixture.input.selection_cache_gate_a = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x33U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuInputFinalizeStatus::
                        selected_group_b_actor_typed_stop &&
                fixture.message == 0U &&
                fixture.input.mouse_action_gate == 0U &&
                fixture.input.selected_actor_cleanup_gate == 1U &&
                fixture.input.selection_cache_gate_a == 9U &&
                fixture.port.calls.empty() && result.return_eax == 0U &&
                result.return_ecx == 0x005229E0U && result.return_edx == 0x33U,
            "selected group-B code zero stops at the one-before-base actor call after preserving prior writes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.selection_runtime_gate = 9U;
        fixture.input.selection_animation_frame_a = 9U;
        fixture.input.selection_animation_frame_b = 9U;
        fixture.input.selection_animation_phase = 9U;
        fixture.startup.value_4ff0b0 = 9U;
        fixture.startup.value_4ff0b4 = 9U;
        fixture.startup.value_4ff0b8 = 9U;
        fixture.startup.value_53bf22 = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            fixture.message == 0U && fixture.input.mouse_action_gate == 0U &&
                fixture.input.selection_runtime_gate == 0U &&
                fixture.input.selection_animation_frame_a == 0U &&
                fixture.input.selection_animation_frame_b == 0U &&
                fixture.input.selection_animation_phase == 5U &&
                fixture.startup.value_4ff0b0 == 0U &&
                fixture.startup.value_4ff0b4 == 0U &&
                fixture.startup.value_4ff0b8 == 0U &&
                fixture.startup.value_53bf22 == 0U && result.return_eax == 0U &&
                result.return_ecx == 1U && result.return_edx == 0x77U &&
                fixture.port.calls.empty(),
            "message one closes the menu and clears only its fixed cache and animation set"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.input.selection_workspace.fill(9U);
        fixture.input.selection_animation_frame_a = 9U;
        fixture.input.selection_animation_frame_b = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            fixture.message == 1U && fixture.frame.list_selection == 1U &&
                fixture.frame.target_selection_gate == 1U &&
                all_zero(fixture.input.selection_workspace) &&
                fixture.input.selection_animation_phase == 5U &&
                fixture.input.selection_animation_frame_a == 0U &&
                fixture.input.selection_animation_frame_b == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 8U && fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        menu_finalize_reset_active_group_a_actor &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 0x005029D0U &&
                fixture.port.calls[0U].edx == 8U,
            "message two rebuilds selection state then resets the active group-A actor"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.final_actor.active_actor_code = 7U;
        fixture.input.selection_animation_frame_a = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuInputFinalizeStatus::
                        active_group_a_actor_typed_stop &&
                fixture.message == 1U &&
                fixture.input.selection_animation_frame_a == 9U &&
                fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "message two stops on a one-before-base active actor before post-call animation clears"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.input.action_kind = 2U;
        fixture.input.selected_option_word = 0xFFFFU;
        fixture.metrics.group_b_count = 2U;
        fixture.frame.target_markers.fill(9U);
        fixture.startup.block_4fe5d4.fill(9U);
        fixture.startup.block_520e90.fill(9U);
        fixture.input.selection_animation_frame_a = 9U;
        fixture.input.selection_animation_frame_b = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleMenuInputFinalizeStatus::completed &&
                result.active_group_a_reset_calls == 1U &&
                result.actor_reset_calls == 12U &&
                fixture.port.calls.size() == 13U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        menu_finalize_reset_active_group_a_actor &&
                fixture.port.calls[1U].ecx == 0x00525508U &&
                fixture.port.calls[2U].ecx == 0x00528030U &&
                fixture.port.calls[3U].ecx == 0x005029D0U &&
                fixture.port.calls[12U].ecx == 0x0051D2A4U &&
                all_zero(fixture.frame.target_markers) &&
                fixture.message == 2U &&
                fixture.frame.target_action_available == 1U &&
                fixture.frame.target_selection_block == 0U &&
                fixture.input.selection_mode_cache == 0U &&
                fixture.input.selection_target_cache == 0U &&
                fixture.startup.value_53bfd0 == 0U &&
                fixture.startup.block_4fe5d4[0U] == 0U &&
                fixture.startup.block_520e90[0U] == 0U &&
                fixture.startup.block_520e90[2U] == 0U &&
                fixture.startup.block_520e90[1U] == 9U &&
                result.return_eax == 0U && result.return_ecx == 8U &&
                result.return_edx == 0U,
            "message three resets the active actor, live group B, ten group-A actors and matched action caches"
        );
    }

    {
        struct ActionCase {
            u32 action_kind;
            u32 expected_message;
        };
        for (const auto item : {
                 ActionCase{3U, 4U},
                 ActionCase{4U, 8U},
                 ActionCase{27U, 27U},
                 ActionCase{30U, 30U},
             }) {
            Fixture fixture;
            fixture.message = 3U;
            fixture.input.selected_option_word = 0U;
            fixture.input.action_kind = item.action_kind;
            fixture.metrics.group_b_count = 0U;
            fixture.frame.transition_value_a = 9U;
            fixture.frame.transition_value_b = 9U;
            const auto result = finalize_legacy_battle_menu_input(
                fixture.bindings(), fixture.port, {}
            );
            test.expect_true(
                result.status ==
                        LegacyBattleMenuInputFinalizeStatus::completed &&
                    fixture.message == item.expected_message &&
                    fixture.frame.transition_value_a == 0U &&
                    fixture.frame.transition_value_b == 0U &&
                    fixture.port.calls.size() == 11U,
                "message three maps action kinds three, four, twenty-seven and thirty after the non-sentinel option path"
            );
        }
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.input.action_kind = 9U;
        fixture.input.fallback_action_kind = 30U;
        fixture.metrics.group_b_count = 0U;
        fixture.startup.block_4fe5d4.fill(9U);
        fixture.startup.block_520e90.fill(9U);
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.message == 1U && fixture.input.action_kind == 30U &&
                fixture.startup.block_4fe5d4[0U] == 9U &&
                fixture.startup.block_520e90[0U] == 9U &&
                result.return_eax == 30U && result.return_ecx == 0x0051D2A4U &&
                fixture.port.calls.size() == 11U,
            "message three unmatched action restores fallback kind without clearing matched-action caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.input.action_kind = 2U;
        fixture.metrics.group_b_count = 9U;
        fixture.frame.target_markers.fill(9U);
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuInputFinalizeStatus::
                        group_b_actor_typed_stop &&
                result.active_group_a_reset_calls == 1U &&
                result.actor_reset_calls == 8U &&
                fixture.port.calls.size() == 9U && result.return_eax == 9U &&
                result.return_ecx == 0x0053AE48U && result.return_edx == 0U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 9U; }
                ),
            "message three live group-B count nine stops at the ninth actor after eight completed resets"
        );
    }

    {
        struct MessageCase {
            u32 message;
            u32 expected_message;
            u32 expected_edx;
        };
        for (const auto item : {
                 MessageCase{4U, 1U, 0U},
                 MessageCase{5U, 4U, 0x44U},
                 MessageCase{8U, 1U, 0U},
                 MessageCase{27U, 1U, 0U},
                 MessageCase{30U, 1U, 0U},
             }) {
            Fixture fixture;
            fixture.message = item.message;
            fixture.input.fallback_action_kind = 27U;
            fixture.frame.panel_scroll_b = 9U;
            fixture.input.selection_workspace.fill(9U);
            const auto result = finalize_legacy_battle_menu_input(
                fixture.bindings(), fixture.port, {.entry_edx = 0x44U}
            );
            test.expect_true(
                result.status ==
                        LegacyBattleMenuInputFinalizeStatus::completed &&
                    fixture.message == item.expected_message &&
                    fixture.input.selection_animation_frame_a == 0U &&
                    fixture.input.selection_animation_frame_b == 0U &&
                    fixture.port.calls.size() == 1U &&
                    result.return_ecx == 0x005029D0U &&
                    result.return_edx == item.expected_edx,
                "messages four, five, eight, twenty-seven and thirty reset the active actor through their distinct register paths"
            );
        }
    }

    {
        Fixture fixture;
        fixture.message = 7U;
        fixture.input.selection_cache_gate_a = 9U;
        fixture.input.selection_cache_gate_b = 9U;
        fixture.input.selection_animation_frame_a = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            fixture.message == 0U &&
                fixture.frame.alternate_selection_limit == 2U &&
                fixture.frame.alternate_selection == 1U &&
                fixture.input.action_kind == 1U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_animation_frame_a == 0U &&
                result.return_eax == 1U && result.return_ecx == 7U &&
                result.return_edx == 0x66U && fixture.port.calls.empty(),
            "message seven closes the alternate selector without resetting an actor"
        );
    }

    {
        Fixture fixture;
        fixture.message = 6U;
        fixture.input.selection_animation_frame_a = 9U;
        fixture.input.selection_animation_frame_b = 9U;
        fixture.input.selection_cache_gate_a = 9U;
        const auto result = finalize_legacy_battle_menu_input(
            fixture.bindings(), fixture.port, {.entry_edx = 0x88U}
        );
        test.expect_true(
            fixture.message == 6U && fixture.input.mouse_action_gate == 1U &&
                fixture.input.selection_animation_frame_a == 0U &&
                fixture.input.selection_animation_frame_b == 0U &&
                fixture.input.selection_cache_gate_a == 9U &&
                result.return_eax == 1U && result.return_ecx == 6U &&
                result.return_edx == 0x88U && fixture.port.calls.empty(),
            "default jump-table cases only clear animation frames after the entry gates"
        );
    }
}
