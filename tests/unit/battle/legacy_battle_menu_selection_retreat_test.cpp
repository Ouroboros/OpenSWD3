#include "openswd3/battle/legacy_battle_menu_selection_retreat.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::battle::LegacyBattleMenuSelectionRetreatBindings;
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

    [[nodiscard]] LegacyBattleMenuSelectionRetreatBindings bindings() {
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

void test_battle_menu_selection_retreat(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuSelectionRetreatStatus;
    using openswd3::battle::retreat_legacy_battle_menu_selection;

    {
        Fixture fixture;
        fixture.final_actor.pre_frame_gate_b = 9U;
        const auto result = retreat_legacy_battle_menu_selection(
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
        fixture.input.selection_index = 2U;
        fixture.startup.value_53bf22 = 2U;
        fixture.startup.value_524418 = 0x00010000U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0xAABB0000U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::completed &&
                fixture.input.selection_index == 7U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 7U && result.return_ecx == 0x00524413U &&
                result.return_edx == 0xAABB0002U && result.sample_calls == 1U,
            "case one decrements, wraps through the physical permission prefix, and preserves the mixed EDX high word"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.selection_index = 0U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::
                        permission_typed_stop &&
                fixture.input.selection_index == 0xFFFFFFFFU &&
                result.sample_calls == 1U &&
                fixture.input.mouse_action_gate == 0U,
            "case one stops at its first physical permission byte after the sample and decremented store"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame.list_selection = 1U;
        fixture.frame.panel_scroll_a = 2U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 1U &&
                fixture.frame.panel_scroll_a == 1U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 1U && result.sample_calls == 1U,
            "case two decrements the panel origin only after its list wraps"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_row_limit_c = 0U;
        fixture.frame.panel_scroll_b = 0U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.equipment_grid_selections.fill(9U);
        fixture.startup.values_52544c.fill(9U);
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::completed &&
                fixture.frame.grid_selection == 0U &&
                fixture.frame.panel_scroll_b == 0U &&
                fixture.frame.equipment_grid_selections[2U] == 0U &&
                fixture.startup.values_52544c[2U] == 0U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 2U && result.return_ecx == 0U &&
                result.return_edx == 0U,
            "case four commits the wrapped grid row and clamped scroll to the current equipment slot"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.current_equipment_selection = 4U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::
                        equipment_selection_typed_stop &&
                result.sample_calls == 1U &&
                fixture.input.mouse_action_gate == 1U,
            "case four stops at the first current-equipment store after its sample and gate"
        );
    }

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.frame.group_b_row_selection = 1U;
        auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.group_b_row_selection == 2U &&
                result.return_eax == 0U && result.sample_calls == 1U,
            "case five wraps row one back to row two while returning the decremented EAX"
        );

        fixture.message = 7U;
        fixture.frame.transition_value_a = 9U;
        fixture.frame.transition_value_b = 9U;
        fixture.frame.alternate_selection = 1U;
        fixture.frame.alternate_selection_limit = 3U;
        result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.transition_value_a == 0U &&
                fixture.frame.transition_value_b == 0U &&
                fixture.frame.alternate_selection == 3U &&
                result.return_eax == 0U && result.return_ecx == 3U,
            "case seven clears both transition values and wraps from one to the live maximum"
        );

        fixture.message = 8U;
        fixture.frame.panel_row_limit_b = 0xFFU;
        fixture.frame.narrow_list_selection = 1U;
        result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.narrow_list_selection == 0xFFFFFFFFU &&
                fixture.input.mouse_action_gate == 1U,
            "case eight sign-extends the byte row limit before the common sample path"
        );

        fixture.message = 27U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_row_limit_c = 0U;
        fixture.frame.panel_scroll_b = 0U;
        const auto samples_before = fixture.port.samples.size();
        result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 0U &&
                fixture.frame.panel_scroll_b == 0U &&
                fixture.port.samples.size() == samples_before,
            "case twenty-seven underflow clamps without taking the shared sample label"
        );

        fixture.message = 30U;
        fixture.frame.grid_selection = 1U;
        result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 10U &&
                fixture.input.mouse_action_gate == 1U,
            "case thirty wraps the first grid entry to ten before the shared sample"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 8U;
        fixture.metrics.group_b_count = 2U;
        fixture.metrics.group_b_order[2U] = 1U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::completed &&
                fixture.frame.target_cursor == 2U &&
                fixture.frame.target_actor_index == 1U &&
                fixture.input.action_kind == 2U &&
                fixture.input.mouse_action_gate == 1U &&
                fixture.frame.target_selection_gate == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_prepare_actor_origin
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection
                ) == 3U &&
                result.return_ecx == 2U,
            "case three retreats through group-B order and reconfigures every live actor before the selected actor"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.metrics.group_a_count = 3U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.input.action_kind = 1U;
        fixture.frame.target_actor_index = 7U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::completed &&
                fixture.input.action_kind == 3U &&
                fixture.frame.target_actor_index == 7U &&
                fixture.port.calls[1U].ecx ==
                    openswd3::battle::kLegacyBattleActionGroupABaseToken +
                        3U *
                            openswd3::battle::kLegacyBattleActionGroupAStride &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection
                ) == 11U,
            "small group-A selection retreats with the one-based action cursor and leaves the separate target actor global untouched"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.metrics.group_a_count = 12U;
        fixture.startup.block_520e90[5U] = 1U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::
                        actor_order_typed_stop &&
                fixture.frame.target_cursor == 12U &&
                result.sample_calls == 1U && fixture.port.calls.empty(),
            "large group-A count stops at the first physical actor-order read after the sample and cursor wrap"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.final_actor.actor_order[5U] = 2U;
        fixture.frame.target_markers.fill(9U);
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::completed &&
                fixture.frame.target_cursor == 5U &&
                fixture.frame.target_actor_index == 0U &&
                fixture.input.action_kind == 3U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_query_group_a_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection
                ) == 11U &&
                result.actor_iterations == 10U,
            "large group-A selection uses actor order, clears all ten markers, and preserves the one-based selected configure index"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.startup.block_520e90[5U] = 1U;
        fixture.final_actor.actor_order[5U] = 9U;
        fixture.frame.target_markers.fill(9U);
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::
                        group_a_actor_typed_stop &&
                fixture.input.action_kind == 10U &&
                std::ranges::all_of(
                    fixture.frame.target_markers,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.port.count(
                    LegacyBattleInputDispatchCall::
                        menu_retreat_configure_actor_selection
                ) == 10U &&
                fixture.input.mouse_action_gate == 0U &&
                fixture.frame.target_selection_gate == 0U,
            "one-based selected group-A index ten stops only after all ten reset calls and marker writes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.final_actor.active_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        const auto result = retreat_legacy_battle_menu_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuSelectionRetreatStatus::
                        group_b_order_typed_stop &&
                fixture.frame.target_cursor == 9U &&
                result.sample_calls == 1U && fixture.port.calls.empty(),
            "oversized group-B count stops at the first physical order-table read after the sample and cursor wrap"
        );
    }
}
