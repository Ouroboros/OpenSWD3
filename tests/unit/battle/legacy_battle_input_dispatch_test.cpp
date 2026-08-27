#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

#include <algorithm>
#include <map>
#include <vector>

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u32;

class InputPort final : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const auto found = replies.find(request.call);
        return found == replies.end() ? default_reply : found->second;
    }

    void delay_input_milliseconds(const u32 milliseconds) override {
        delays.push_back(milliseconds);
    }

    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back({sound_id, static_cast<u32>(mix_level)});
        return {.eax = eax + 1U, .ecx = ecx + 2U, .edx = edx + 3U};
    }

    [[nodiscard]] u32 count(const LegacyBattleInputDispatchCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const LegacyBattleInputDispatchCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::map<LegacyBattleInputDispatchCall, LegacyBattleInputDispatchCallReply>
        replies;
    LegacyBattleInputDispatchCallReply default_reply{
        .eax = 0x11111111U,
        .ecx = 0x22222222U,
        .edx = 0x33333333U,
    };
    std::vector<u32> delays;
    std::vector<std::array<u32, 2>> samples;
};

struct Fixture {
    u32 render_abort{};
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::compat::u16 supplemental_count{};
    openswd3::battle::LegacyBattleFrameInputResolutionState frame_input;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    openswd3::battle::LegacyBattleContextPromptState prompt;
    u32 message{};
    u32 terminal{};
    u32 one_shot_interaction_state{};
    u32 target_ready_gate{};
    u32 outcome_darkening_gate{};
    openswd3::input_time_rng::LegacyInputNormalizationState input;
    openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    std::vector<openswd3::world_map::LegacyWorldInteractionHotspot> hotspots;
    InputPort port;

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchBindings
    bindings() {
        return {
            .render_abort_latch = render_abort,
            .startup_reset = startup,
            .startup_supplemental_count_word = supplemental_count,
            .frame_input_resolution = frame_input,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .debug_hotkeys = debug,
            .context_prompt = prompt,
            .message_state = message,
            .terminal_latch = terminal,
            .one_shot_interaction_state = one_shot_interaction_state,
            .target_ready_gate = target_ready_gate,
            .outcome_darkening_gate = outcome_darkening_gate,
            .input_records = input.records,
            .keyboard = keyboard,
            .dialogs = dialogs,
            .choice_hotspots = hotspots,
        };
    }
};

}  // namespace

void test_battle_input_dispatch(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.render_abort = 1U;
        fixture.port.battle_input_dispatch_state().menu_action = 9U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.return_eax == 1U && result.raw_key_queries == 0U &&
                result.input_record_reads == 0U &&
                fixture.port.battle_input_dispatch_state().menu_action == 0U,
            "render-abort returns its live value after clearing the menu action"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.startup.value_524414 = 1U;
        fixture.keyboard[2U] = 0x80U;
        fixture.port
            .replies[LegacyBattleInputDispatchCall::refresh_action_mode] = {
            .eax = 0x10U, .ecx = 0x20U, .edx = 0x30U
        };
        fixture.port.replies
            [LegacyBattleInputDispatchCall::target_selection_refresh_state] = {
            .eax = 0x40U, .ecx = 0x50U, .edx = 0x60U
        };
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.returned_early && result.raw_key_queries == 1U &&
                fixture.message == 1U &&
                fixture.final_actor.pre_frame_gate_a == 1U &&
                fixture.port.battle_input_dispatch_state().selection_index ==
                    1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::refresh_action_mode
                ) == 1U &&
                result.target_selection_entry_calls == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        target_selection_refresh_state
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_target_selection_entry_slot
                ) == 0U &&
                result.return_eax == 0x40U && result.return_ecx == 0x50U &&
                result.return_edx == 0x60U,
            "the first permitted direct key refreshes, publishes selection one, and returns the commit registers"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 7U;
        fixture.startup.value_524414 = 1U;
        fixture.keyboard[2U] = 0x80U;
        fixture.target_ready_gate = 1U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        target_selection_entry_typed_stop &&
                result.target_selection_entry_calls == 1U &&
                fixture.message == 1U &&
                fixture.final_actor.pre_frame_gate_a == 1U &&
                fixture.port.battle_input_dispatch_state().selection_index ==
                    1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_target_selection_entry_slot
                ) == 0U,
            "direct target-selection typed-stop preserves the permitted-key prefix and emits no opaque slot"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.startup.value_524414 = 0xFFFFFFFFU;
        fixture.startup.value_524418 = 0xFFFFFFFFU;
        fixture.port.battle_input_dispatch_state().selection_index = 5U;
        fixture.message = 0U;
        fixture.keyboard[6U] = 0x80U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.returned_early && result.raw_key_queries == 5U &&
                fixture.message == 0U &&
                fixture.final_actor.pre_frame_gate_a == 0U &&
                fixture.port.battle_input_dispatch_state().selection_index ==
                    1U &&
                fixture.port.calls.empty(),
            "direct key six resets the live selection even when it was already mode five"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 3U;
        fixture.input.records[18U].rapid_press_multiplicity = 1U;
        fixture.input.records[18U].held_sample_count = 1U;
        fixture.port
            .replies[LegacyBattleInputDispatchCall::query_active_actor] = {
            .eax = 0U
        };
        fixture.port
            .replies[LegacyBattleInputDispatchCall::query_retreat_actor] = {
            .eax = 0U, .ecx = 0x10U, .edx = 0x20U
        };
        fixture.port.battle_input_dispatch_state().sample_mix_level = -7;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.returned_early &&
                fixture.port.delays == std::vector<u32>{20U} &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::display_retreat_warning
                ) == 1U &&
                fixture.port.samples ==
                    std::vector<std::array<u32, 2>>{{0x8CU, 0xFFFFFFF9U}} &&
                result.return_eax == 0x11111112U &&
                result.return_ecx == 0x22222224U &&
                result.return_edx == 0xFFFFFFFCU,
            "blocked retreat preserves the 20ms warning, text request, signed mix level, and sample tail"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_b_count = 4U;
        fixture.input.records[18U].rapid_press_multiplicity = 1U;
        fixture.input.records[18U].held_sample_count = 1U;
        fixture.port
            .replies[LegacyBattleInputDispatchCall::query_active_actor] = {
            .eax = 0U
        };
        fixture.port
            .replies[LegacyBattleInputDispatchCall::query_retreat_actor] = {
            .eax = 1U
        };
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                fixture.port.delays == std::vector<u32>{50U} &&
                fixture.action.opponent_workspace[11U] == 0x11U &&
                fixture.final_actor.secondary_actor_code == 9U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.final_actor.published_actor_code == 2U &&
                fixture.final_actor.auxiliary_gate == 1U &&
                fixture.final_actor.frame_gate_a == 1U &&
                fixture.final_actor.frame_gate_b == 1U &&
                fixture.port.battle_input_dispatch_state().retreat_block_word ==
                    0x4000U,
            "successful retreat preparation writes the actor workspace and all live transition gates in order"
        );
        const u32 expected_actor_token =
            openswd3::battle::kLegacyBattleActionGroupABaseToken +
            (9U - 8U) * openswd3::battle::kLegacyBattleActionGroupAStride;
        test.expect_true(
            fixture.port.calls.size() == 3U,
            "successful retreat performs exactly three typed battle calls"
        );
        test.expect_true(
            fixture.port.calls.size() >= 3U &&
                fixture.port.calls[0U].arguments[0U] == expected_actor_token,
            "active-actor query receives the exact group-A object token"
        );
        test.expect_true(
            fixture.port.calls.size() >= 3U &&
                fixture.port.calls[1U].arguments[0U] == expected_actor_token,
            "retreat query receives the exact group-A object token"
        );
        test.expect_true(
            fixture.port.calls.size() >= 3U &&
                fixture.port.calls[2U].arguments[0U] == expected_actor_token,
            "retreat configuration receives the exact group-A object token"
        );
        test.expect_true(
            fixture.port.calls.size() >= 3U &&
                fixture.port.calls[2U].arguments[1U] == 1U &&
                fixture.port.calls[2U].arguments[2U] == 9U,
            "retreat configuration preserves the original stack arguments"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 1U;
        fixture.input.records[17U].rapid_press_multiplicity = 1U;
        fixture.input.records[17U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.returned_early && fixture.terminal == 1U &&
                fixture.final_actor.action_execution_active == 1U &&
                fixture.action.opponent_workspace[10U] == 1U &&
                fixture.final_actor.pre_frame_gate_a == 1U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                fixture.port.battle_input_dispatch_state().selection_index ==
                    1U &&
                fixture.message == 3U,
            "confirm input writes the physical actor-plus-two workspace slot before its immediate return"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 0x200U;
        fixture.metrics.group_a_count = 1U;
        fixture.input.records[17U].rapid_press_multiplicity = 1U;
        fixture.input.records[17U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        workspace_typed_stop &&
                fixture.terminal == 1U &&
                fixture.final_actor.action_execution_active == 1U &&
                fixture.message == 0U,
            "confirm input stops at the first out-of-workspace actor store after publishing its earlier gates"
        );
    }

    {
        Fixture fixture;
        fixture.input.records[0U].rapid_press_multiplicity = 1U;
        fixture.input.records[0U].held_sample_count = 99U;
        fixture.input.records[1U].held_sample_count = 1U;
        fixture.input.records[15U].rapid_press_multiplicity = 0U;
        fixture.hotspots.push_back(
            {.left = 1U, .top = 2U, .right = 3U, .bottom = 4U}
        );
        fixture.dialogs.choice_chain_flags = 0x1000U;
        fixture.debug.battle_mode_flags_53bc24 = 0x123U;
        fixture.final_actor.pre_frame_gate_b = 0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                fixture.hotspots.empty() &&
                fixture.dialogs.choice_chain_flags == 0U &&
                fixture.final_actor.pre_frame_gate_a == 1U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                fixture.prompt.frame_counter == 300U &&
                fixture.debug.battle_mode_flags_53bc24 == 0x103U &&
                result.target_selection_entry_calls == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        target_selection_refresh_state
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_target_selection_entry_slot
                ) == 0U,
            "base confirm clears the choice chain, resets prompt cadence, clears only mode bit 0x20, and commits"
        );
    }

    {
        Fixture fixture;
        fixture.input.records[3U].rapid_press_multiplicity = 1U;
        fixture.input.records[3U].held_sample_count = 1U;
        fixture.hotspots = {
            {.left = 1U},
            {.left = 2U},
            {.left = 3U},
        };
        fixture.port.battle_input_dispatch_state().choice_selection_index = 0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                fixture.port.battle_input_dispatch_state()
                        .choice_selection_index == 2U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::confirm_secondary
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::commit_left
                ) == 1U,
            "left choice input preserves the low-word sign wrap and direct hotspot count"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.input.records[4U].rapid_press_multiplicity = 1U;
        fixture.input.records[4U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_selection_retreat_calls == 1U &&
                fixture.port.battle_input_dispatch_state().menu_action == 1U &&
                fixture.frame_input.target_selection_gate == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::confirm_primary
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_selection_retreat_slot
                ) == 0U,
            "record four directly retreats the live menu selection before its existing confirmation call"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 0x100U;
        fixture.final_actor.pre_frame_gate_b = 9U;
        fixture.input.records[4U].rapid_press_multiplicity = 1U;
        fixture.input.records[4U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        menu_selection_retreat_typed_stop &&
                result.menu_selection_retreat_calls == 1U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::confirm_primary
                ) == 0U,
            "menu-retreat typed-stop preserves its sample and blocks the following confirmation"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.input.records[6U].rapid_press_multiplicity = 1U;
        fixture.input.records[6U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_selection_advance_calls == 1U &&
                fixture.port.battle_input_dispatch_state().menu_action == 2U &&
                fixture.frame_input.target_selection_gate == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::confirm_secondary
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_selection_advance_slot
                ) == 0U,
            "record six directly advances the live menu selection before its existing confirmation call"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 0x100U;
        fixture.final_actor.pre_frame_gate_b = 9U;
        fixture.input.records[6U].rapid_press_multiplicity = 1U;
        fixture.input.records[6U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        menu_selection_advance_typed_stop &&
                result.menu_selection_advance_calls == 1U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::confirm_secondary
                ) == 0U,
            "menu-advance typed-stop preserves its sample and blocks the following confirmation"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame_input.grid_selection = 1U;
        fixture.frame_input.panel_scroll_b = 10U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().interaction_mode = 3U;
        fixture.input.records[15U].rapid_press_multiplicity = 1U;
        fixture.input.records[15U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_page_retreat_calls == 1U &&
                fixture.frame_input.panel_scroll_b == 3U &&
                fixture.port.battle_input_dispatch_state().menu_action == 5U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_page_retreat_slot
                ) == 0U,
            "interaction mode three directly retreats the page before publishing its mouse action"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame_input.grid_selection = 1U;
        fixture.frame_input.panel_scroll_b = 10U;
        fixture.input.records[7U].rapid_press_multiplicity = 1U;
        fixture.input.records[7U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleInputDispatchStatus::completed,
            "record seven page-retreat dispatch completes"
        );
        test.expect_true(
            result.menu_page_retreat_calls == 1U,
            "record seven invokes page retreat once"
        );
        test.expect_true(
            fixture.frame_input.panel_scroll_b == 3U,
            "record seven publishes the retreated grid page"
        );
        test.expect_true(
            fixture.port.battle_input_dispatch_state().mouse_action_gate == 1U,
            "record seven publishes the page mouse gate"
        );
        test.expect_true(
            fixture.port.count(
                LegacyBattleInputDispatchCall::reserved_menu_page_retreat_slot
            ) == 0U,
            "record seven emits no opaque page-retreat slot"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame_input.grid_selection = 1U;
        fixture.frame_input.panel_scroll_b = 3U;
        fixture.frame_input.current_equipment_selection = 4U;
        fixture.input.records[7U].rapid_press_multiplicity = 1U;
        fixture.input.records[7U].held_sample_count = 1U;
        fixture.input.records[8U].rapid_press_multiplicity = 1U;
        fixture.input.records[8U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleInputDispatchStatus::
                    menu_page_retreat_typed_stop,
            "record seven propagates page-retreat typed-stop"
        );
        test.expect_true(
            result.menu_page_retreat_calls == 1U,
            "typed record seven invokes page retreat once"
        );
        test.expect_true(
            fixture.frame_input.panel_scroll_b == 0U,
            "typed record seven preserves the clamped grid page"
        );
        test.expect_true(
            fixture.port.battle_input_dispatch_state().mouse_action_gate == 1U,
            "typed record seven preserves the page mouse gate"
        );
        test.expect_true(
            fixture.port.count(
                LegacyBattleInputDispatchCall::reserved_menu_page_advance_slot
            ) == 0U,
            "page-retreat typed-stop blocks the following mode-four input"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame_input.panel_row_limit_a = 20U;
        fixture.frame_input.list_selection = 7U;
        fixture.frame_input.panel_scroll_a = 0U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().interaction_mode = 4U;
        fixture.input.records[15U].rapid_press_multiplicity = 1U;
        fixture.input.records[15U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_page_advance_calls == 1U &&
                fixture.frame_input.panel_scroll_a == 7U &&
                fixture.port.battle_input_dispatch_state().menu_action == 5U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_page_advance_slot
                ) == 0U,
            "interaction mode four directly advances the page before publishing its mouse action"
        );
    }

    {
        Fixture fixture;
        fixture.message = 27U;
        fixture.frame_input.panel_row_limit_c = 20U;
        fixture.frame_input.grid_selection = 1U;
        fixture.frame_input.current_equipment_selection = 2U;
        fixture.input.records[8U].rapid_press_multiplicity = 1U;
        fixture.input.records[8U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_page_advance_calls == 1U &&
                fixture.frame_input.grid_selection == 7U &&
                fixture.frame_input.equipment_grid_selections[2U] == 7U &&
                fixture.startup.values_52544c[2U] == 0U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_page_advance_slot
                ) == 0U,
            "record eight directly advances and publishes the live grid page"
        );
    }

    {
        Fixture fixture;
        fixture.message = 27U;
        fixture.frame_input.panel_row_limit_c = 20U;
        fixture.frame_input.grid_selection = 1U;
        fixture.frame_input.current_equipment_selection = 4U;
        fixture.input.records[8U].rapid_press_multiplicity = 1U;
        fixture.input.records[8U].held_sample_count = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        menu_page_advance_typed_stop &&
                result.menu_page_advance_calls == 1U &&
                fixture.frame_input.grid_selection == 7U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U,
            "record-eight page advance propagates its typed-stop after preserving normalization"
        );
    }

    {
        Fixture fixture;
        fixture.input.records[0U].rapid_press_multiplicity = 1U;
        fixture.input.records[0U].held_sample_count = 1U;
        fixture.port.battle_input_dispatch_state().final_value_a = 9U;
        fixture.port.battle_input_dispatch_state().final_value_b = 9U;
        fixture.port.battle_input_dispatch_state().selection_animation_frame_a =
            9U;
        fixture.port.battle_input_dispatch_state().selection_animation_frame_b =
            9U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        completed &&
                result.menu_input_finalize_calls == 1U &&
                fixture.port.battle_input_dispatch_state().final_value_a ==
                    0U &&
                fixture.port.battle_input_dispatch_state().final_value_b ==
                    0U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_animation_frame_a == 0U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_animation_frame_b == 0U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        reserved_menu_input_finalize_slot
                ) == 0U,
            "record zero directly finalizes menu input before clearing its trailing values"
        );
    }

    {
        Fixture fixture;
        fixture.input.records[0U].rapid_press_multiplicity = 1U;
        fixture.input.records[0U].held_sample_count = 1U;
        fixture.port.battle_input_dispatch_state().final_value_a = 9U;
        fixture.port.battle_input_dispatch_state().final_value_b = 9U;
        fixture.port.battle_input_dispatch_state().selected_actor_cleanup_gate =
            1U;
        fixture.final_actor.published_actor_code = 0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                fixture.bindings(), fixture.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleInputDispatchStatus::
                        menu_input_finalize_typed_stop &&
                result.menu_input_finalize_calls == 1U &&
                fixture.port.battle_input_dispatch_state().final_value_a ==
                    9U &&
                fixture.port.battle_input_dispatch_state().final_value_b == 9U,
            "menu-finalize typed-stop blocks the caller trailing-value clears"
        );
    }

    {
        Fixture fixture;
        std::array<openswd3::input_time_rng::LegacyInputRecord, 2>
            short_input{};
        auto bindings = fixture.bindings();
        bindings.input_records = short_input;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_input_dispatch(
                bindings, fixture.port, {}
            );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleInputDispatchStatus::
                    input_record_typed_stop,
            "a short input-record owner stops at the first real record-nine read"
        );
    }
}
