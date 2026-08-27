#include "openswd3/battle/legacy_battle_menu_context_retreat.hpp"

#include <cstddef>
#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;

struct SampleCall {
    u32 sound_id{};
    i32 mix_level{};
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

class MenuContextRetreatPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back({sound_id, mix_level, eax, ecx, edx});
        const std::size_t index = samples.size() - 1U;
        if (message_state != nullptr && index < next_messages.size()) {
            *message_state = next_messages[index];
        }
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    u32* message_state{};
    std::vector<u32> next_messages;
    std::vector<
        std::optional<openswd3::battle::LegacyBattleInputDispatchCallReply>>
        replies;
    std::vector<SampleCall> samples;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleFinalActorStepState actor;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    MenuContextRetreatPort port;

    [[nodiscard]] openswd3::battle::LegacyBattleMenuContextRetreatBindings
    bindings() {
        return {
            .startup_reset = startup,
            .final_actor = actor,
            .frame_input_resolution = frame,
            .input_dispatch = input,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_menu_context_retreat(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleMenuContextRetreatStatus;
    using openswd3::battle::retreat_legacy_battle_menu_context;

    {
        Fixture fixture;
        fixture.message = 3U;
        fixture.actor.pre_frame_gate_b = 9U;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            fixture.actor.pre_frame_gate_b == 0U && result.return_eax == 2U &&
                result.return_ecx == 3U && result.return_edx == 0x33U &&
                result.sample_calls == 0U && fixture.port.samples.empty(),
            "unhandled message clears the gate then returns the forced constant two and live message ECX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.action_kind = 5U;
        fixture.input.sample_mix_level = -7;
        fixture.startup.value_524414 = 1U;
        fixture.port.replies.push_back(
            openswd3::battle::LegacyBattleInputDispatchCallReply{
                .eax = 0xAAU, .ecx = 0xBBU, .edx = 0xCCU
            }
        );
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {.entry_edx = 0x33U}
        );
        test.expect_true(
            result.status == LegacyBattleMenuContextRetreatStatus::completed &&
                fixture.input.action_kind == 1U &&
                result.permission_reads == 1U && result.sample_calls == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U].sound_id == 0x2EU &&
                fixture.port.samples[0U].eax == 0xFFFFFFF9U &&
                fixture.port.samples[0U].ecx == 1U &&
                fixture.port.samples[0U].edx == 0x33U &&
                result.return_eax == 2U && result.return_ecx == 1U &&
                result.return_edx == 0xCCU,
            "message one retreats four slots then overwrites sample EAX and ECX with the constant and live message"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.action_kind = 5U;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.input.action_kind == 5U && result.permission_reads == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U].ecx == 0U,
            "zero permission restores the message-one action kind by adding four"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.action_kind = 13U;
        fixture.actor.pre_frame_gate_b = 9U;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {.entry_edx = 0x44U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuContextRetreatStatus::
                        permission_typed_stop &&
                fixture.actor.pre_frame_gate_b == 0U &&
                fixture.input.action_kind == 9U && result.return_eax == 9U &&
                result.return_ecx == 9U && result.return_edx == 0x44U &&
                result.permission_reads == 0U && fixture.port.samples.empty(),
            "retreat permission overflow stops after publishing the four-slot prefix"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.input.action_category_index = 0U;
        fixture.input.sample_mix_level = -4;
        fixture.frame.list_selection = 9U;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            fixture.input.action_category_index == 2U &&
                fixture.frame.list_selection == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U].eax == 2U &&
                fixture.port.samples[0U].ecx == 0xFFFFFFFCU &&
                fixture.port.samples[0U].edx == 0x77U &&
                result.return_eax == 2U,
            "message two wraps the decremented three-category index to constant two"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.current_equipment_selection = 0U;
        fixture.frame.equipment_grid_selections[3U] = 7U;
        fixture.startup.values_52544c[3U] = 9U;
        fixture.input.sample_mix_level = 5;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            fixture.frame.current_equipment_selection == 3U &&
                fixture.frame.grid_selection == 7U &&
                fixture.frame.panel_scroll_b == 9U &&
                result.equipment_selection_reads == 1U &&
                result.equipment_scroll_reads == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U].eax == 9U &&
                fixture.port.samples[0U].ecx == 5U &&
                fixture.port.samples[0U].edx == 7U,
            "message four wraps equipment category zero to three and restores both caches"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.frame.current_equipment_selection = 10U;
        fixture.input.sample_mix_level = 5;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {.entry_edx = 0x44U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleMenuContextRetreatStatus::
                        equipment_selection_typed_stop &&
                fixture.frame.current_equipment_selection == 9U &&
                result.return_eax == 9U && result.return_ecx == 5U &&
                result.return_edx == 0x44U &&
                result.equipment_selection_reads == 0U &&
                fixture.port.samples.empty(),
            "positive equipment overflow stops at the first real cache read after decrement"
        );
    }

    {
        Fixture fixture;
        fixture.message = 30U;
        fixture.frame.grid_selection = 4U;
        fixture.input.sample_mix_level = -2;
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {.entry_ecx = 0x55U}
        );
        test.expect_true(
            fixture.frame.grid_selection == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U].eax == 0xFFFFFFFFU &&
                fixture.port.samples[0U].ecx == 30U &&
                fixture.port.samples[0U].edx == 0xFFFFFFFEU &&
                result.return_eax == 0xFFFFFFFFU,
            "message thirty clamps the stored grid selection but preserves pre-clamp wrapped EAX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.input.action_kind = 5U;
        fixture.input.action_category_index = 0U;
        fixture.startup.value_524414 = 1U;
        fixture.frame.current_equipment_selection = 0U;
        fixture.frame.equipment_grid_selections[3U] = 7U;
        fixture.startup.values_52544c[3U] = 9U;
        fixture.port.message_state = &fixture.message;
        fixture.port.next_messages = {2U, 4U, 30U};
        const auto result = retreat_legacy_battle_menu_context(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleMenuContextRetreatStatus::completed &&
                result.sample_calls == 4U && result.port_calls == 4U &&
                fixture.port.samples.size() == 4U &&
                fixture.input.action_kind == 1U &&
                fixture.input.action_category_index == 2U &&
                fixture.frame.list_selection == 1U &&
                fixture.frame.current_equipment_selection == 3U &&
                fixture.frame.panel_scroll_b == 9U &&
                fixture.frame.grid_selection == 2U,
            "live message rechecks allow all four retreat branches after sample-side mutations"
        );
    }
}
