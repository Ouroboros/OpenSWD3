#include "openswd3/battle/legacy_battle_target_selection_entry.hpp"

#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::battle::LegacyBattleTargetSelectionEntryBindings;
using openswd3::compat::i32;
using openswd3::compat::u32;

class TargetSelectionPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back(
            {sound_id, static_cast<u32>(mix_level), eax, ecx, edx}
        );
        if (sample_reply.has_value()) {
            return *sample_reply;
        }
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<std::optional<LegacyBattleInputDispatchCallReply>> replies;
    std::vector<std::array<u32, 5>> samples;
    std::optional<LegacyBattleInputDispatchCallReply> sample_reply;
};

struct Fixture {
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    u32 one_shot_interaction_state{};
    u32 target_ready_gate{};
    u32 outcome_darkening_gate{};
    u32 message{};
    TargetSelectionPort port;

    [[nodiscard]] LegacyBattleTargetSelectionEntryBindings bindings() {
        return {
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .input_dispatch = port.battle_input_dispatch_state(),
            .dialogs = dialogs,
            .one_shot_interaction_state = one_shot_interaction_state,
            .target_ready_gate = target_ready_gate,
            .outcome_darkening_gate = outcome_darkening_gate,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_target_selection_entry(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleTargetSelectionEntryStatus;
    using openswd3::battle::enter_legacy_battle_target_selection;

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().retreat_block_word = 1U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0x11U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "nonzero entry word returns before loading any target state"
        );
    }

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().input_gate = 1U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "input gate one returns its loaded EAX and preserves caller ECX and EDX"
        );
    }

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().input_gate = 7U;
        fixture.outcome_darkening_gate = 1U;
        const auto outcome = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_ecx = 2U, .entry_edx = 3U}
        );
        Fixture message_gate;
        message_gate.port.battle_input_dispatch_state().input_gate = 9U;
        message_gate.action.message_gate = 0x80000000U;
        const auto message = enter_legacy_battle_target_selection(
            message_gate.bindings(),
            message_gate.port,
            {.entry_ecx = 4U, .entry_edx = 5U}
        );
        test.expect_true(
            outcome.return_eax == 7U && outcome.return_ecx == 2U &&
                outcome.return_edx == 3U && message.return_eax == 9U &&
                message.return_ecx == 4U && message.return_edx == 5U &&
                fixture.port.calls.empty() && message_gate.port.calls.empty(),
            "outcome and signed-message gates return after the shared input-gate load"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.metrics.group_b_count = 3U;
        fixture.action.packed_actor_counter = 2U;
        fixture.port.battle_input_dispatch_state().retreat_target_word = 4U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x99U}
        );
        test.expect_true(
            fixture.message == 0U && result.return_eax == 2U &&
                result.return_ecx == 2U && result.return_edx == 1U &&
                fixture.port.calls.empty(),
            "remaining group-B count at most one clears the message when a retreat target is present"
        );
    }

    {
        Fixture below;
        below.message = 110U;
        below.frame.target_selection_suppression = 1U;
        below.port.battle_input_dispatch_state().input_gate = 0xAABBCC00U;
        below.port.battle_input_dispatch_state().target_transition_word = 29U;
        const auto below_result = enter_legacy_battle_target_selection(
            below.bindings(), below.port, {}
        );
        Fixture equal;
        equal.message = 110U;
        equal.frame.target_selection_suppression = 1U;
        equal.port.battle_input_dispatch_state().target_transition_word = 30U;
        const auto equal_result = enter_legacy_battle_target_selection(
            equal.bindings(), equal.port, {}
        );
        test.expect_true(
            below.port.battle_input_dispatch_state().target_transition_word ==
                    29U &&
                below_result.return_eax == 0xAABB001DU &&
                equal.port.battle_input_dispatch_state()
                        .target_transition_word == 100U &&
                (equal_result.return_eax & 0xFFFFU) == 30U &&
                below.port.calls.empty() && equal.port.calls.empty(),
            "message one hundred ten clamps low transition words and promotes exactly thirty"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.dialogs.messages.emplace_back();
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 7U}
        );
        test.expect_true(
            fixture.one_shot_interaction_state == 1U &&
                result.return_eax == 1U && result.return_ecx == 4U &&
                result.return_edx == 0U && fixture.port.calls.empty(),
            "a live dialog publishes the one-shot interaction state and returns a canonical nonzero head token"
        );
    }

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.frame.target_selection_suppression = 1U;
        fixture.port.battle_input_dispatch_state().input_gate = 0x12340000U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 0xAAU,
                .ecx = 0xBBU,
                .edx = 0xCCU,
            }
        );
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x44U}
        );
        test.expect_true(
            fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_refresh_state &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 5U &&
                fixture.port.calls[0U].edx == 0x44U &&
                result.return_eax == 0xAAU && result.return_ecx == 0xBBU &&
                result.return_edx == 0xCCU,
            "disabled target readiness refreshes state with the current branch registers"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 1U,
                .ecx = 0x77U,
                .edx = 0x88U,
            }
        );
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            fixture.port.battle_input_dispatch_state().selected_option_word ==
                    0xFFFFU &&
                fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::query_active_actor &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 0x005029D0U &&
                fixture.port.calls[0U].edx == 0U && result.return_eax == 1U &&
                result.return_ecx == 0x77U && result.return_edx == 0x88U &&
                fixture.port.samples.empty(),
            "a completed active actor query returns before sample and target setup"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 7U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        active_group_a_actor_typed_stop &&
                fixture.port.battle_input_dispatch_state()
                        .selected_option_word == 0xFFFFU &&
                fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 0U,
            "one-before-base active group-A code stops at the first actor query after cache clear"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        fixture.port.battle_input_dispatch_state().selected_group_b_index =
            0xFFFFU;
        fixture.port.battle_input_dispatch_state().sample_mix_level = 6;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0x10U, .edx = 0x20U
            },
            std::nullopt,
            LegacyBattleInputDispatchCallReply{
                .eax = 0x30U, .ecx = 0x40U, .edx = 0x50U
            },
        };
        fixture.port.sample_reply = LegacyBattleInputDispatchCallReply{
            .eax = 0xAAU,
            .ecx = 0xBBU,
            .edx = 0xCCU,
        };
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.port_calls == 4U && result.sample_calls == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U] ==
                    std::array<u32, 5>{0x2DU, 6U, 0U, 6U, 0x20U} &&
                fixture.port.calls.size() == 3U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_configure_actor &&
                fixture.port.calls[1U].arguments[0U] == 0x0053BF4AU &&
                fixture.port.calls[1U].arguments[1U] == 0x0053BF4EU &&
                fixture.port.calls[1U].eax == 0U &&
                fixture.port.calls[1U].ecx == 0x005029D0U &&
                fixture.port.calls[1U].edx == 8U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleInputDispatchCall::refresh_action_mode &&
                fixture.message == 1U &&
                fixture.port.battle_input_dispatch_state().action_kind == 1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U &&
                fixture.frame.target_selection_gate == 1U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_animation_phase == 5U &&
                result.return_eax == 0x30U && result.return_ecx == 0x40U &&
                result.return_edx == 0x50U,
            "unfinished active actor plays selection sound, configures the actor and refreshes when no group-B target exists"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& input = fixture.port.battle_input_dispatch_state();
        input.selected_option_word = 3U;
        input.selected_group_b_index = 0U;
        input.selected_group_a_index = 0U;
        fixture.frame.transition_value_a = 9U;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{.eax = 0U},
            std::nullopt,
            LegacyBattleInputDispatchCallReply{.eax = 1U},
            LegacyBattleInputDispatchCallReply{.eax = 0U},
            LegacyBattleInputDispatchCallReply{.eax = 1U},
            LegacyBattleInputDispatchCallReply{.eax = 1U},
            LegacyBattleInputDispatchCallReply{.eax = 0U},
        };
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::completed &&
                result.port_calls == 8U && result.sample_calls == 1U &&
                result.primary_scan_calls == 3U &&
                result.secondary_scan_calls == 2U &&
                fixture.port.calls.size() == 7U && fixture.message == 7U &&
                fixture.frame.alternate_selection_limit == 5U &&
                fixture.frame.transition_value_a == 0U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_scan_primary &&
                fixture.port.calls[2U].arguments[0U] == 0U &&
                fixture.port.calls[2U].arguments[1U] == 0x0053C16CU &&
                fixture.port.calls[2U].arguments[2U] == 0x0053BD44U &&
                fixture.port.calls[2U].eax == 0U &&
                fixture.port.calls[2U].ecx == 0x00525508U &&
                fixture.port.calls[5U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_scan_secondary &&
                fixture.port.calls[5U].eax == 0U &&
                fixture.port.calls[5U].ecx == 0x00525508U &&
                fixture.port.calls[5U].edx == 0U,
            "matching selected actors scan three primary and two secondary entries and publish the exact visible count"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& input = fixture.port.battle_input_dispatch_state();
        input.selected_option_word = 3U;
        input.selected_group_b_index = 8U;
        input.selected_group_a_index = 0U;
        fixture.frame.transition_value_a = 9U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        selected_group_b_actor_typed_stop &&
                result.port_calls == 3U && result.sample_calls == 1U &&
                fixture.port.calls.size() == 2U && fixture.message == 7U &&
                fixture.frame.alternate_selection_limit == 2U &&
                fixture.frame.transition_value_a == 9U &&
                result.return_eax == 0xAC8U &&
                result.return_ecx == 0x0053AE48U && result.return_edx == 0U,
            "selected group-B index eight stops on the first scan after preserving setup and list-prefix writes"
        );
    }
}
