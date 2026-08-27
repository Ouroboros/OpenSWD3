#include "openswd3/battle/legacy_battle_menu_page_retreat.hpp"

#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleMenuPageRetreatBindings;
using openswd3::compat::i32;
using openswd3::compat::u32;

class PagePort final : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back({
            sound_id,
            static_cast<u32>(mix_level),
            eax,
            ecx,
            edx,
        });
        return {
            .eax = eax + 0x100U,
            .ecx = ecx + 0x200U,
            .edx = 0xAABBCCDDU,
        };
    }

    std::vector<std::array<u32, 5>> samples;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    PagePort port;

    [[nodiscard]] LegacyBattleMenuPageRetreatBindings bindings() {
        return {
            .startup_reset = startup,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .input_dispatch = input,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_menu_page_retreat(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuPageRetreatStatus;
    using openswd3::battle::retreat_legacy_battle_menu_page;

    {
        Fixture fixture;
        fixture.final_actor.pre_frame_gate_b = 9U;
        const auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status == LegacyBattleMenuPageRetreatStatus::completed &&
                result.return_eax == 0xFFFFFFE5U &&
                result.return_ecx == 0x22U && result.return_edx == 0x33U &&
                fixture.final_actor.pre_frame_gate_b == 0U &&
                fixture.port.samples.empty(),
            "default message returns message minus twenty-seven after clearing the pre-frame gate"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame.list_selection = 5U;
        fixture.frame.panel_scroll_a = 10U;
        const auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 1U &&
                fixture.frame.panel_scroll_a == 10U &&
                fixture.input.mouse_action_gate == 0U &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0xAABBCCDDU && result.sample_calls == 1U,
            "message two first returns list selection to one without moving the page"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.input.menu_action = 1U;
        fixture.frame.list_selection = 5U;
        fixture.frame.panel_scroll_a = 10U;
        auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.list_selection == 5U &&
                fixture.frame.panel_scroll_a == 3U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 1U && result.return_ecx == 3U &&
                result.return_edx == 0xAABBCCDDU,
            "message two subtracts seven when menu action already requests a page move"
        );

        fixture.input.menu_action = 0U;
        fixture.input.mouse_action_gate = 0U;
        fixture.frame.list_selection = 1U;
        fixture.frame.panel_scroll_a = 3U;
        result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_a == 0U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 1U && result.return_ecx == 0xFFFFFFFCU,
            "message two clamps a negative page while preserving the signed subtraction in ECX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 27U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 10U;
        auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 1U &&
                fixture.frame.panel_scroll_b == 10U &&
                fixture.input.mouse_action_gate == 0U &&
                result.return_eax == 1U && result.return_ecx == 0U,
            "message twenty-seven first returns grid selection to one"
        );

        fixture.input.menu_action = 1U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 6U;
        result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 4U &&
                fixture.frame.panel_scroll_b == 0U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 1U && result.return_ecx == 0xFFFFFFFFU,
            "message twenty-seven clamps page scroll while returning its negative ECX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 10U;
        fixture.frame.equipment_grid_selections.fill(9U);
        fixture.startup.values_52544c.fill(9U);
        const auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 1U &&
                fixture.frame.panel_scroll_b == 10U &&
                fixture.input.mouse_action_gate == 0U &&
                fixture.frame.equipment_grid_selections[2U] == 9U &&
                fixture.startup.values_52544c[2U] == 9U &&
                result.return_eax == 1U && result.return_ecx == 4U &&
                result.return_edx == 0xAABBCCDDU,
            "message four first returns grid selection to one without publishing equipment caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.input.menu_action = 1U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 10U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.equipment_grid_selections.fill(9U);
        fixture.startup.values_52544c.fill(9U);
        const auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleMenuPageRetreatStatus::completed &&
                fixture.frame.panel_scroll_b == 3U &&
                fixture.frame.equipment_grid_selections[2U] == 4U &&
                fixture.startup.values_52544c[2U] == 3U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 2U && result.return_ecx == 4U &&
                result.return_edx == 3U,
            "message four subtracts seven and publishes both current-equipment caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.input.menu_action = 1U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_scroll_b = 3U;
        fixture.frame.current_equipment_selection = 4U;
        const auto result = retreat_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuPageRetreatStatus::
                        equipment_selection_typed_stop &&
                fixture.frame.panel_scroll_b == 0U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 4U && result.return_ecx == 1U &&
                result.return_edx == 0U && result.sample_calls == 1U,
            "message four stops at the first equipment store after clamp and mouse gate"
        );
    }
}
