#include "openswd3/battle/legacy_battle_menu_selection_advance.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::battle::LegacyBattleMenuSelectionAdvanceBindings;
using openswd3::compat::i32;
using openswd3::compat::u32;

class MenuPort final : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const auto found = replies.find(request.call);
        if (found == replies.end() || found->second.empty()) {
            return default_reply;
        }
        const auto reply = found->second.front();
        found->second.erase(found->second.begin());
        return reply;
    }

    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        static_cast<void>(edx);
        samples.push_back({sound_id, static_cast<u32>(mix_level)});
        return {
            .eax = eax + 0x100U,
            .ecx = ecx + 0x200U,
            .edx = 0xAABBCCDDU,
        };
    }

    [[nodiscard]] u32 count(const LegacyBattleInputDispatchCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const auto& request) { return request.call == call; }
        ));
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::map<
        LegacyBattleInputDispatchCall,
        std::vector<LegacyBattleInputDispatchCallReply>>
        replies;
    LegacyBattleInputDispatchCallReply default_reply{
        .eax = 0U,
        .ecx = 0x22222222U,
        .edx = 0x33333333U,
    };
    std::vector<std::array<u32, 2>> samples;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::compat::u16 supplemental_count{};
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    MenuPort port;

    [[nodiscard]] LegacyBattleMenuSelectionAdvanceBindings bindings() {
        return {
            .startup_reset = startup,
            .startup_supplemental_count_word = supplemental_count,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .metrics = metrics,
            .input_dispatch = input,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_menu_selection_advance(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuSelectionAdvanceStatus;
    using openswd3::battle::advance_legacy_battle_menu_selection;

    {
        Fixture fixture;
        fixture.final_actor.pre_frame_gate_b = 9U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0xFFFFFFFFU && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                result.port_calls == 0U,
            "message zero preserves entry ECX and EDX after the subtract-range default"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.selection_index = 1U;
        fixture.startup.value_53bf22 = 2U;
        fixture.startup.value_524414 = 0x00010000U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.input.selection_index == 3U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 3U && result.return_ecx == 0x00524413U &&
                result.return_edx == 7U && result.sample_calls == 1U,
            "case one increments and wraps under the live permission upper bound"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.selection_index = 8U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        permission_typed_stop &&
                fixture.input.selection_index == 9U &&
                result.sample_calls == 1U &&
                fixture.input.mouse_action_gate == 0U,
            "case one stops at permission index nine after the sample and incremented store"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame.list_selection = 7U;
        fixture.frame.panel_scroll_a = 0U;
        fixture.frame.panel_row_limit_a = 9U;
        auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 7U &&
                fixture.frame.panel_scroll_a == 1U &&
                fixture.input.mouse_action_gate == 1U &&
                result.sample_calls == 1U,
            "case two advances the scroll when list row seven remains below the signed limit"
        );

        fixture.port.samples.clear();
        fixture.input.mouse_action_gate = 0U;
        fixture.frame.list_selection = 7U;
        fixture.frame.panel_scroll_a = 3U;
        fixture.frame.panel_row_limit_a = 8U;
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 7U &&
                fixture.frame.panel_scroll_a == 1U &&
                fixture.port.samples.empty() &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 1U,
            "case two clamps scroll to signed limit minus seven without playing a sample"
        );

        fixture.input.mouse_action_gate = 0U;
        fixture.frame.list_selection = 5U;
        fixture.frame.panel_row_limit_a = 4U;
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 4U &&
                fixture.port.samples.empty() && result.return_eax == 6U,
            "case two clamps an in-window row above the signed byte limit without a sample"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.grid_selection = 7U;
        fixture.frame.panel_scroll_b = 0U;
        fixture.frame.panel_row_limit_c = 9U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.equipment_grid_selections.fill(9U);
        fixture.startup.values_52544c.fill(9U);
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.frame.grid_selection == 7U &&
                fixture.frame.panel_scroll_b == 1U &&
                fixture.frame.equipment_grid_selections[2U] == 7U &&
                fixture.startup.values_52544c[2U] == 1U &&
                fixture.input.mouse_action_gate == 1U &&
                result.sample_calls == 1U && result.return_eax == 2U &&
                result.return_ecx == 7U && result.return_edx == 1U,
            "case four advances grid scroll and publishes both current-equipment caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.current_equipment_selection = 4U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_row_limit_c = 10U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        equipment_selection_typed_stop &&
                result.sample_calls == 1U &&
                fixture.input.mouse_action_gate == 0U,
            "case four stops at the first equipment store before its trailing mouse gate"
        );
    }

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.frame.group_b_row_selection = 2U;
        auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.group_b_row_selection == 1U &&
                result.return_eax == 3U && result.sample_calls == 1U,
            "case five wraps row two to row one while returning incremented EAX"
        );

        fixture.message = 7U;
        fixture.frame.transition_value_a = 9U;
        fixture.frame.transition_value_b = 9U;
        fixture.frame.alternate_selection = 3U;
        fixture.frame.alternate_selection_limit = 3U;
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.transition_value_a == 0U &&
                fixture.frame.transition_value_b == 0U &&
                fixture.frame.alternate_selection == 1U &&
                result.return_eax == 4U && result.return_ecx == 3U,
            "case seven clears transitions and wraps above the live maximum"
        );

        fixture.message = 8U;
        fixture.frame.panel_row_limit_b = 2U;
        fixture.frame.narrow_list_selection = 2U;
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.narrow_list_selection == 1U &&
                fixture.input.mouse_action_gate == 1U,
            "case eight wraps above its sign-extended byte limit before the shared sample"
        );

        fixture.message = 27U;
        fixture.frame.grid_selection = 7U;
        fixture.frame.panel_scroll_b = 3U;
        fixture.frame.panel_row_limit_c = 8U;
        const auto samples_before = fixture.port.samples.size();
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 7U &&
                fixture.frame.panel_scroll_b == 1U &&
                fixture.port.samples.size() == samples_before &&
                fixture.input.mouse_action_gate == 1U,
            "case twenty-seven clamps scroll without taking its sample label"
        );

        fixture.message = 30U;
        fixture.frame.grid_selection = 10U;
        result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 1U &&
                fixture.input.mouse_action_gate == 1U,
            "case thirty wraps grid ten to one before the shared sample"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 2U;
        fixture.frame.target_cursor = 2U;
        fixture.metrics.group_b_order[1U] = 1U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.frame.target_cursor == 1U &&
                fixture.frame.target_actor_index == 1U &&
                fixture.input.action_kind == 2U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 3U &&
                fixture.port.calls[0U].eax == 0x565U &&
                fixture.port.calls[0U].ecx ==
                    openswd3::battle::kLegacyBattleActionGroupBBaseToken +
                        openswd3::battle::kLegacyBattleActionGroupBStride &&
                fixture.port.calls[0U].edx == 0x159U &&
                fixture.port.calls[2U].eax == 2U &&
                fixture.port.calls[2U].ecx ==
                    openswd3::battle::kLegacyBattleActionGroupBBaseToken &&
                result.return_ecx == 2U,
            "case three wraps the group-B cursor to one and configures the selected target"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.frame.target_cursor = 0U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        group_b_actor_typed_stop &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 8U &&
                result.return_eax == 9U &&
                result.return_ecx ==
                    openswd3::battle::kLegacyBattleActionGroupBBaseToken +
                        8U *
                            openswd3::battle::kLegacyBattleActionGroupBStride &&
                result.return_edx == 0x33333333U,
            "group-B count nine stops at the ninth configure call after preserving eight actor updates"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 5U;
        fixture.final_actor.actor_order[1U] = 0U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        group_a_actor_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 1U && fixture.port.calls.empty(),
            "group-A code zero stops at the first completion-field access with its raw address registers"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 5U;
        fixture.final_actor.actor_order[1U] = 2U;
        fixture.frame.target_markers.fill(9U);
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.frame.target_cursor == 1U &&
                fixture.input.action_kind == 3U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 11U &&
                fixture.port.calls[0U].eax ==
                    2U * openswd3::battle::kLegacyBattleActionGroupAStride &&
                fixture.port.calls[0U].ecx ==
                    openswd3::battle::kLegacyBattleActionGroupABaseToken +
                        openswd3::battle::kLegacyBattleActionGroupAStride &&
                fixture.port.calls[0U].edx == 1U &&
                fixture.port.calls[1U].eax == 2U * 0x3EFU &&
                fixture.port.calls[1U].ecx ==
                    openswd3::battle::kLegacyBattleActionGroupABaseToken +
                        2U *
                            openswd3::battle::kLegacyBattleActionGroupAStride &&
                fixture.port.calls[1U].edx == 2U * 0xBCDU &&
                fixture.port.calls[2U].eax == 3U,
            "large group-A selection preserves query and prepare address registers before clearing markers"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 3U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.input.action_kind = 1U;
        fixture.port.replies[LegacyBattleInputDispatchCall::
                                 menu_advance_query_group_a_candidate] = {
            {.eax = 1U},
            {.eax = 0U},
        };
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.input.action_kind == 3U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_query_group_a_candidate
                ) == 2U,
            "small group-A rejection reloads the one-based action cursor before the next advance"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 1U;
        fixture.final_actor.actor_order[2U] = 1U;
        fixture.final_actor.actor_order[3U] = 2U;
        fixture.port.replies[LegacyBattleInputDispatchCall::
                                 menu_advance_query_group_a_candidate] = {
            {.eax = 1U},
            {.eax = 0U},
        };
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.frame.target_cursor == 3U &&
                fixture.input.action_kind == 3U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_query_group_a_candidate
                ) == 2U,
            "large group-A rejection advances with live bounds until the next eligible actor"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 12U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 9U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        actor_order_typed_stop &&
                fixture.frame.target_cursor == 10U &&
                result.return_eax == 12U && result.return_ecx == 0U &&
                result.return_edx == 10U && result.sample_calls == 1U,
            "large group-A count stops at actor-order entry ten with the original remaining registers"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 5U;
        fixture.final_actor.actor_order[1U] = 9U;
        fixture.frame.target_markers.fill(9U);
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::completed &&
                fixture.input.action_kind == 10U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 11U &&
                fixture.port.calls.back().ecx ==
                    openswd3::battle::kLegacyBattleActionGroupABaseToken +
                        9U *
                            openswd3::battle::kLegacyBattleActionGroupAStride &&
                fixture.input.mouse_action_gate == 1U,
            "one-based selected code ten maps to the tenth physical group-A object"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.frame.target_cursor = 5U;
        fixture.final_actor.actor_order[1U] = 10U;
        fixture.frame.target_markers.fill(9U);
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        group_a_actor_typed_stop &&
                fixture.input.action_kind == 0U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 9U; }
                ) &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_advance_configure_actor_selection
                ) == 0U &&
                result.return_eax == 10U * 0x3EFU &&
                result.return_ecx ==
                    openswd3::battle::kLegacyBattleActionGroupABaseToken +
                        10U *
                            openswd3::battle::kLegacyBattleActionGroupAStride &&
                result.return_edx == 10U * 0xBCDU,
            "large group-A code ten stops at the one-past prepare call after preserving its address registers"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.frame.target_cursor = 8U;
        const auto result = advance_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionAdvanceStatus::
                        group_b_order_typed_stop &&
                fixture.frame.target_cursor == 9U &&
                result.sample_calls == 1U && fixture.port.calls.empty(),
            "oversized group-B count stops at forward order entry nine"
        );
    }
}
