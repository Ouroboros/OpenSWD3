#include "openswd3/story_scene/legacy_dialog_control.hpp"

#include "test.hpp"

namespace {

using openswd3::story_scene::LegacyDialogControlAction;
using openswd3::story_scene::LegacyDialogControlApplyStatus;
using openswd3::story_scene::LegacyDialogControlInput;
using openswd3::story_scene::LegacyDialogControlState;
using openswd3::story_scene::LegacyDialogClosePorts;
using openswd3::story_scene::LegacyDialogCloseState;
using openswd3::story_scene::LegacyDialogMessage;
using openswd3::story_scene::LegacyDialogRecord32;
using openswd3::story_scene::apply_legacy_dialog_control_action;
using openswd3::story_scene::advance_legacy_dialog_control;
using openswd3::story_scene::kLegacyDialogFlagCloseInitialized;
using openswd3::story_scene::kLegacyDialogFlagClosing;
using openswd3::story_scene::kLegacyDialogFlagConfirmArmed;
using openswd3::story_scene::kLegacyDialogFlagDirectRectangle;
using openswd3::story_scene::kLegacyDialogFlagHasChoices;
using openswd3::story_scene::kLegacyDialogFlagInteractive;
using openswd3::story_scene::kLegacyDialogFlagPageBoundary;
using openswd3::story_scene::kLegacyDialogFlagSelectionAccepted;
using openswd3::story_scene::kLegacyDialogFlagTerminated;
using openswd3::story_scene::kLegacyDialogFlagTimedAdvance;

class RecordingClosePorts final : public LegacyDialogClosePorts {
public:
    [[nodiscard]] bool close_role_dialog_action(
        const openswd3::compat::u16 role_index
    ) noexcept override {
        last_role_index = role_index;
        ++role_close_count;
        return role_close_success;
    }

    void close_detached_dialog() noexcept override {
        ++detached_close_count;
    }

    openswd3::compat::u16 last_role_index{};
    unsigned role_close_count{};
    unsigned detached_close_count{};
    bool role_close_success{true};
};

void test_interactive_confirm_arms_then_closes(openswd3::test::Context& test) {
    LegacyDialogRecord32 record{
        .flags = kLegacyDialogFlagInteractive,
        .character_delay = 4U,
        .character_countdown = 3U,
    };
    LegacyDialogControlState state;
    const LegacyDialogControlInput input{
        .global_confirm_latch_state = 1U,
    };

    const auto armed = advance_legacy_dialog_control(record, input, state);
    test.expect_true(
        armed.action == LegacyDialogControlAction::parse_text &&
            (record.flags & kLegacyDialogFlagConfirmArmed) != 0U &&
            (record.flags & kLegacyDialogFlagClosing) == 0U &&
            record.character_countdown == 2U &&
            record.display_counter == 0x24U && state.advance_signal_state == 1U,
        "interactive input first arms bit 31 and leaves text active"
    );

    const auto closing = advance_legacy_dialog_control(record, input, state);
    test.expect_true(
        closing.action == LegacyDialogControlAction::parse_text &&
            (record.flags & kLegacyDialogFlagClosing) != 0U &&
            (record.flags & kLegacyDialogFlagCloseInitialized) != 0U &&
            record.character_delay == 0xFFFFU &&
            record.character_countdown == 0xFFFFU,
        "the next interactive input initializes the original closing state"
    );
}

void test_page_selection_one_advances_and_two_closes(
    openswd3::test::Context& test
) {
    LegacyDialogRecord32 advance_record{
        .flags = kLegacyDialogFlagPageBoundary,
    };
    LegacyDialogControlState advance_state{.advance_signal_state = 1U};
    const auto advance = advance_legacy_dialog_control(
        advance_record,
        LegacyDialogControlInput{.initial_selection_state = 1U},
        advance_state
    );
    test.expect_true(
        advance.action == LegacyDialogControlAction::advance_page &&
            advance_record.character_countdown == 0x8FFFU &&
            (advance_record.flags & kLegacyDialogFlagSelectionAccepted) != 0U,
        "selection state one takes the page-advance branch"
    );

    LegacyDialogRecord32 close_record{
        .flags = kLegacyDialogFlagPageBoundary,
    };
    LegacyDialogControlState close_state{.advance_signal_state = 1U};
    const auto close = advance_legacy_dialog_control(
        close_record,
        LegacyDialogControlInput{.initial_selection_state = 2U},
        close_state
    );
    test.expect_true(
        close.action == LegacyDialogControlAction::close_message &&
            close_state.selection_state == 2U,
        "selection state two bypasses advance and consumes accepted close"
    );
}

void test_advance_signal_filters_terminated_selection(
    openswd3::test::Context& test
) {
    LegacyDialogRecord32 record{
        .flags = kLegacyDialogFlagTerminated,
        .character_countdown = 5U,
    };
    LegacyDialogControlState state;
    const auto result = advance_legacy_dialog_control(
        record, LegacyDialogControlInput{.initial_selection_state = 1U}, state
    );
    test.expect_true(
        result.action == LegacyDialogControlAction::parse_text &&
            state.selection_state == 0U &&
            (record.flags & kLegacyDialogFlagSelectionAccepted) == 0U &&
            (record.flags & 1U) != 0U && record.character_countdown == 4U,
        "terminated selection is rejected unless the shared advance signal equals one"
    );
}

void test_timed_page_advance_resets_start(openswd3::test::Context& test) {
    LegacyDialogRecord32 record{
        .flags = kLegacyDialogFlagPageBoundary | kLegacyDialogFlagTimedAdvance,
        .lifetime_limit = 5U,
        .lifetime_started_at = 100U,
    };
    LegacyDialogControlState state;
    const auto result = advance_legacy_dialog_control(
        record, LegacyDialogControlInput{.current_tick = 701U}, state
    );
    test.expect_true(
        result.action == LegacyDialogControlAction::advance_page &&
            result.elapsed_sampled && result.elapsed_deciseconds == 6U &&
            record.lifetime_started_at == 0U && state.selection_state == 1U,
        "non-terminated timed advance uses floor(wrapped milliseconds/100) and clears +0x10"
    );
}

void test_direct_rectangle_fade_and_sentinel(openswd3::test::Context& test) {
    LegacyDialogRecord32 fading{
        .flags = kLegacyDialogFlagDirectRectangle | kLegacyDialogFlagTerminated,
        .lifetime_limit = 1U,
        .display_counter = 1U,
    };
    LegacyDialogControlState fading_state;
    const auto faded = advance_legacy_dialog_control(
        fading, LegacyDialogControlInput{.current_tick = 200U}, fading_state
    );
    test.expect_true(
        faded.action == LegacyDialogControlAction::close_message &&
            fading.display_counter == 0U && fading_state.selection_state == 1U,
        "expired direct rectangles decrement +0x18 and close exactly at zero"
    );

    LegacyDialogRecord32 sentinel{
        .flags = kLegacyDialogFlagDirectRectangle,
        .lifetime_limit = 0xFFFFU,
        .lifetime_started_at = 0U,
    };
    LegacyDialogControlState sentinel_state;
    const auto held = advance_legacy_dialog_control(
        sentinel, LegacyDialogControlInput{.current_tick = 50U}, sentinel_state
    );
    test.expect_true(
        held.action == LegacyDialogControlAction::parse_text &&
            sentinel.display_counter == 0x10U &&
            sentinel_state.selection_state == 0U,
        "the FFFF direct-rectangle sentinel holds display counter at sixteen"
    );
}

void test_choice_chain_and_transition_gates(openswd3::test::Context& test) {
    LegacyDialogRecord32 empty_choices{
        .flags = kLegacyDialogFlagHasChoices,
    };
    LegacyDialogControlState empty_state;
    const auto empty = advance_legacy_dialog_control(
        empty_choices, LegacyDialogControlInput{}, empty_state
    );
    test.expect_true(
        empty.action == LegacyDialogControlAction::close_message &&
            empty_choices.display_counter == 1U &&
            empty_state.selection_state == 1U,
        "a choice-bearing message with no hotspot chain requests close"
    );

    LegacyDialogRecord32 active_choices{
        .flags = kLegacyDialogFlagHasChoices,
    };
    LegacyDialogControlState active_state;
    const auto active = advance_legacy_dialog_control(
        active_choices,
        LegacyDialogControlInput{
            .initial_selection_state = 1U,
            .choice_chain_active = true,
            .transition_in_progress = true,
        },
        active_state
    );
    test.expect_true(
        active.action ==
                LegacyDialogControlAction::skip_text_during_transition &&
            active_choices.display_counter == 0x20U,
        "opening transition skips text after preserving choice-chain state"
    );
}

void test_apply_page_advance_restores_saved_style(
    openswd3::test::Context& test
) {
    LegacyDialogMessage message;
    message.text = {'A', '%', 'Q'};
    message.page_stop_index = 0U;
    message.record.flags = kLegacyDialogFlagPageBoundary |
        kLegacyDialogFlagSelectionAccepted | 0x100U |
        kLegacyDialogFlagTerminated;
    message.record.saved_foreground_index = 4U;
    message.record.saved_secondary_index = 5U;
    message.record.saved_text_style = 0x84U;
    LegacyDialogControlState control;
    LegacyDialogCloseState close;
    RecordingClosePorts ports;

    const auto result = apply_legacy_dialog_control_action(
        message, LegacyDialogControlAction::advance_page, control, close, ports
    );
    test.expect_true(
        result.status == LegacyDialogControlApplyStatus::completed &&
            result.page_advanced && message.text_cursor_index == 0U &&
            message.page_stop_index == 1U &&
            message.record.flags == kLegacyDialogFlagTerminated &&
            message.record.foreground_index == 4U &&
            message.record.secondary_index == 5U &&
            message.record.text_style == 0x84U &&
            message.record.saved_text_style == 0U,
        "advance copies +0x40 to +0x3C, skips one ASCII byte and restores saved style"
    );

    message.page_stop_index = message.text.size();
    const auto invalid = apply_legacy_dialog_control_action(
        message, LegacyDialogControlAction::advance_page, control, close, ports
    );
    test.expect_equal(
        invalid.status,
        LegacyDialogControlApplyStatus::source_out_of_bounds,
        "modern page advance isolates an invalid legacy pointer boundary"
    );
}

void test_apply_close_preserves_flagged_counter(openswd3::test::Context& test) {
    LegacyDialogMessage message;
    message.record.flags =
        openswd3::story_scene::kLegacyDialogFlagCloseRoleAction;
    message.record.role_index = 7U;
    message.record.text_cursor_pointer_32 = 0x12345678U;
    LegacyDialogControlState control{
        .selection_state = 2U,
        .advance_signal_state = 1U,
    };
    LegacyDialogCloseState close{
        .flagged_dialog_counter = 0x8001U,
        .input_hold_state = 9U,
    };
    RecordingClosePorts ports;
    ports.role_close_success = false;

    const auto result = apply_legacy_dialog_control_action(
        message, LegacyDialogControlAction::close_message, control, close, ports
    );
    test.expect_true(
        result.message_closed &&
            result.nonfatal_role_action_failure_count == 1U &&
            ports.role_close_count == 1U && ports.last_role_index == 7U &&
            !message.active && message.record.text_cursor_pointer_32 == 0U &&
            close.flagged_dialog_counter == 0x8000U &&
            close.close_mode_state == 0x0CU && close.input_hold_state == 0U &&
            control.selection_state == 0U && control.advance_signal_state == 0U,
        "close preserves counter bit 15, treats role action failure as nonfatal and clears latches"
    );

    LegacyDialogMessage detached;
    detached.record.role_index = 0xFFFDU;
    const auto detached_result = apply_legacy_dialog_control_action(
        detached,
        LegacyDialogControlAction::close_message,
        control,
        close,
        ports
    );
    test.expect_true(
        detached_result.message_closed && ports.detached_close_count == 1U,
        "FFFD close uses the detached-dialog side effect"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_interactive_confirm_arms_then_closes(test);
    test_page_selection_one_advances_and_two_closes(test);
    test_advance_signal_filters_terminated_selection(test);
    test_timed_page_advance_resets_start(test);
    test_direct_rectangle_fade_and_sentinel(test);
    test_choice_chain_and_transition_gates(test);
    test_apply_page_advance_restores_saved_style(test);
    test_apply_close_preserves_flagged_counter(test);
    return test.exit_code();
}
