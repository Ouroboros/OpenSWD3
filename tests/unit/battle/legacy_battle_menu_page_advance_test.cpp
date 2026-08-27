#include "openswd3/battle/legacy_battle_menu_page_advance.hpp"

#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleMenuPageAdvanceBindings;
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

    [[nodiscard]] LegacyBattleMenuPageAdvanceBindings bindings() {
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

void test_battle_menu_page_advance(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuPageAdvanceStatus;
    using openswd3::battle::advance_legacy_battle_menu_page;

    {
        Fixture fixture;
        fixture.final_actor.pre_frame_gate_b = 9U;
        const auto result = advance_legacy_battle_menu_page(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status == LegacyBattleMenuPageAdvanceStatus::completed &&
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
        fixture.input.sample_mix_level = 0x30;
        fixture.frame.panel_row_limit_a = 6U;
        fixture.frame.panel_scroll_a = 9U;
        const auto result = advance_legacy_battle_menu_page(
            fixture.bindings(),
            fixture.port,
            {.entry_ecx = 0x11223344U, .entry_edx = 0x55U}
        );
        test.expect_true(
            result.return_eax == 0x130U && result.return_ecx == 0x11223506U &&
                result.return_edx == 0xAABBCCDDU &&
                fixture.frame.panel_scroll_a == 9U &&
                fixture.input.mouse_action_gate == 0U &&
                result.sample_calls == 1U &&
                fixture.port.samples[0U][2U] == 0x30U,
            "message two signed-byte limit below seven returns the sample registers with only CL replaced"
        );

        fixture.frame.panel_row_limit_a = 0x80U;
        const auto signed_result = advance_legacy_battle_menu_page(
            fixture.bindings(),
            fixture.port,
            {.entry_ecx = 0x01020304U, .entry_edx = 0x66U}
        );
        test.expect_true(
            signed_result.return_ecx == 0x01020580U &&
                fixture.input.mouse_action_gate == 0U,
            "message two treats row limits above one hundred twenty-seven as signed negative bytes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.frame.panel_row_limit_a = 20U;
        fixture.frame.list_selection = 1U;
        const auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {.entry_ecx = 0x12340000U}
        );
        test.expect_true(
            fixture.frame.list_selection == 7U &&
                fixture.frame.panel_scroll_a == 0U &&
                fixture.input.mouse_action_gate == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x12340214U &&
                result.return_edx == 0xAABBCCDDU,
            "message two first moves selection to seven while retaining the sample-backed ECX high bytes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.input.menu_action = 1U;
        fixture.frame.panel_row_limit_a = 20U;
        fixture.frame.panel_scroll_a = 0U;
        auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_a == 7U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 7U && result.return_ecx == 20U &&
                result.return_edx == 14U,
            "message two advances a full page while returning the next-page end in EDX"
        );

        fixture.frame.panel_scroll_a = 7U;
        fixture.input.mouse_action_gate = 0U;
        result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_a == 13U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 13U && result.return_ecx == 20U &&
                result.return_edx == 21U,
            "message two clamps the page start to signed row limit minus seven"
        );

        fixture.frame.panel_scroll_a = 0x7FFFFFF9U;
        fixture.frame.list_selection = 7U;
        result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_a == 0U &&
                fixture.frame.list_selection == 20U &&
                result.return_eax == 0x80000000U && result.return_ecx == 20U &&
                result.return_edx == 0x80000007U,
            "message two preserves the wrapped negative EAX after zero-clamping its shared page"
        );
    }

    {
        Fixture fixture;
        fixture.message = 27U;
        fixture.frame.panel_row_limit_c = 5U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_scroll_b = 9U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.equipment_grid_selections.fill(99U);
        fixture.startup.values_52544c.fill(99U);
        const auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.grid_selection == 5U &&
                fixture.frame.equipment_grid_selections[2U] == 5U &&
                fixture.startup.values_52544c[2U] == 9U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 5U && result.return_ecx == 2U &&
                result.return_edx == 9U,
            "message twenty-seven normalization also publishes the legacy equipment caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 27U;
        fixture.input.menu_action = 1U;
        fixture.frame.panel_row_limit_c = 20U;
        fixture.frame.panel_scroll_b = 7U;
        auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_b == 13U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 20U && result.return_ecx == 13U &&
                result.return_edx == 21U,
            "message twenty-seven clamps the grid page while retaining the pre-clamp EDX"
        );

        fixture.frame.panel_row_limit_c = 5U;
        fixture.frame.panel_scroll_b = 0U;
        fixture.frame.grid_selection = 7U;
        result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_b == 0U &&
                fixture.frame.grid_selection == 5U && result.return_eax == 5U &&
                result.return_ecx == 0xFFFFFFFEU && result.return_edx == 14U,
            "message twenty-seven zero-clamps a short grid and preserves negative ECX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.input.menu_action = 1U;
        fixture.frame.panel_row_limit_c = 20U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 0U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.equipment_grid_selections.fill(99U);
        fixture.startup.values_52544c.fill(99U);
        auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_b == 7U &&
                fixture.frame.equipment_grid_selections[2U] == 4U &&
                fixture.startup.values_52544c[2U] == 7U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 7U && result.return_ecx == 2U &&
                result.return_edx == 4U,
            "message four advances and publishes the original grid with the new page"
        );

        fixture.frame.panel_row_limit_c = 5U;
        fixture.frame.grid_selection = 7U;
        fixture.frame.panel_scroll_b = 0U;
        fixture.frame.equipment_grid_selections.fill(99U);
        fixture.startup.values_52544c.fill(99U);
        result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.panel_scroll_b == 0U &&
                fixture.frame.grid_selection == 5U &&
                fixture.frame.equipment_grid_selections[2U] == 5U &&
                fixture.startup.values_52544c[2U] == 0U &&
                result.return_eax == 0U && result.return_ecx == 2U &&
                result.return_edx == 5U,
            "message four short-grid clamp replaces grid and both equipment caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.panel_row_limit_c = 9U;
        fixture.frame.grid_selection = 1U;
        fixture.frame.panel_scroll_b = 3U;
        fixture.frame.current_equipment_selection = 4U;
        const auto result = advance_legacy_battle_menu_page(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuPageAdvanceStatus::
                        equipment_selection_typed_stop &&
                fixture.frame.grid_selection == 7U &&
                fixture.frame.panel_scroll_b == 3U &&
                fixture.input.mouse_action_gate == 1U &&
                result.return_eax == 7U && result.return_ecx == 4U &&
                result.return_edx == 3U && result.sample_calls == 1U,
            "grid normalization stops at the first equipment store after preserving earlier writes"
        );
    }
}
