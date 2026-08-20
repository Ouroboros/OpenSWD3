#include "openswd3/story_scene/legacy_dialog_runtime.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include "test.hpp"

#include <string_view>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::story_scene::LegacyDialogCaptionRequest;
using openswd3::story_scene::LegacyDialogChoiceBackgroundRequest;
using openswd3::story_scene::LegacyDialogCompositeRequest;
using openswd3::story_scene::LegacyDialogIndicatorKind;
using openswd3::story_scene::LegacyDialogIndicatorRequest;
using openswd3::story_scene::LegacyDialogMessage;
using openswd3::story_scene::LegacyDialogPanelDrawRequest;
using openswd3::story_scene::LegacyDialogRectangle;
using openswd3::story_scene::LegacyDialogRuntimeInput;
using openswd3::story_scene::LegacyDialogRuntimePorts;
using openswd3::story_scene::LegacyDialogRuntimeState;
using openswd3::story_scene::LegacyDialogRuntimeStatus;
using openswd3::story_scene::LegacyDialogMessageReleaseResult;
using openswd3::story_scene::LegacyDialogSegmentDrawRequest;
using openswd3::story_scene::clear_legacy_dialog_choice_chain;
using openswd3::story_scene::kLegacyDialogFlagTerminated;
using openswd3::story_scene::kLegacyDialogSurfaceHeight;
using openswd3::story_scene::kLegacyDialogSurfaceWidth;
using openswd3::story_scene::release_legacy_dialog_messages;
using openswd3::story_scene::update_draw_legacy_dialogs;

enum class Call {
    begin,
    clear,
    end,
    resolve_anchor,
    clip,
    panel,
    text,
    composite,
    indicator,
    caption,
    close_role,
    close_detached,
    release_owner,
    update_end,
    update_next,
    restore_destination,
};

class RecordingPorts final : public LegacyDialogRuntimePorts {
public:
    [[nodiscard]] bool
    begin_text_surface(const i32 width, const i32 height) noexcept override {
        calls.push_back(Call::begin);
        surface_width = width;
        surface_height = height;
        return begin_success;
    }

    void clear_text_surface() noexcept override {
        calls.push_back(Call::clear);
        ++clear_count;
    }

    void end_text_surface() noexcept override {
        calls.push_back(Call::end);
    }

    [[nodiscard]] bool resolve_role_anchor(
        const u16 role_index, i32& world_x, i32& world_y
    ) noexcept override {
        calls.push_back(Call::resolve_anchor);
        resolved_role = role_index;
        world_x = anchor_x;
        world_y = anchor_y;
        return anchor_success;
    }

    void
    set_dialog_clip(const LegacyDialogRectangle& rectangle) noexcept override {
        calls.push_back(Call::clip);
        clips.push_back(rectangle);
    }

    void draw_dialog_panel(
        const LegacyDialogPanelDrawRequest& request
    ) noexcept override {
        calls.push_back(Call::panel);
        panel = request.rectangle;
        panel_action = request.action;
        panel_opacity = request.opacity_step;
    }

    void composite_text_surface(
        const LegacyDialogCompositeRequest& request
    ) noexcept override {
        calls.push_back(Call::composite);
        composite = request;
    }

    void draw_dialog_indicator(
        const LegacyDialogIndicatorRequest& request
    ) noexcept override {
        calls.push_back(Call::indicator);
        indicator_kind = request.kind;
    }

    void draw_dialog_caption(
        const LegacyDialogCaptionRequest& request
    ) noexcept override {
        calls.push_back(Call::caption);
        caption_size = request.text.size();
        caption_action = request.action_pointer_32;
        live_caption_action = request.action;
    }

    void release_message_owner(const u16 role_index) noexcept override {
        calls.push_back(Call::release_owner);
        released_role = role_index;
    }

    [[nodiscard]] bool update_end_dialog_action() noexcept override {
        calls.push_back(Call::update_end);
        return end_action_success;
    }

    [[nodiscard]] bool update_next_page_action() noexcept override {
        calls.push_back(Call::update_next);
        return next_action_success;
    }

    void restore_text_destination(
        const i32 width, const i32 height
    ) noexcept override {
        calls.push_back(Call::restore_destination);
        restored_width = width;
        restored_height = height;
    }

    [[nodiscard]] bool
    draw_segment(const LegacyDialogSegmentDrawRequest&) noexcept override {
        calls.push_back(Call::text);
        return true;
    }

    void draw_selected_choice_background(
        const LegacyDialogChoiceBackgroundRequest&
    ) noexcept override {}

    void play_choice_sound() noexcept override {}

    [[nodiscard]] bool
    close_role_dialog_action(const u16 role_index) noexcept override {
        calls.push_back(Call::close_role);
        closed_role = role_index;
        return close_role_success;
    }

    void close_detached_dialog() noexcept override {
        calls.push_back(Call::close_detached);
    }

    std::vector<Call> calls;
    std::vector<LegacyDialogRectangle> clips;
    LegacyDialogRectangle panel{};
    LegacyDialogCompositeRequest composite{};
    LegacyDialogIndicatorKind indicator_kind{
        LegacyDialogIndicatorKind::next_page
    };
    i32 surface_width{};
    i32 surface_height{};
    i32 panel_opacity{};
    i32 anchor_x{50};
    i32 anchor_y{30};
    i32 restored_width{};
    i32 restored_height{};
    u32 clear_count{};
    u32 caption_action{};
    const openswd3::asset_runtime::LegacyActionRecord* panel_action{};
    const openswd3::asset_runtime::LegacyActionRecord* live_caption_action{};
    std::size_t caption_size{};
    u16 resolved_role{};
    u16 closed_role{};
    u16 released_role{};
    bool begin_success{true};
    bool anchor_success{true};
    bool close_role_success{true};
    bool end_action_success{true};
    bool next_action_success{true};
};

LegacyDialogMessage terminated_message(const u16 role_index) {
    LegacyDialogMessage message;
    message.record.role_index = role_index;
    message.record.transition_step = 4U;
    message.record.left = 100U;
    message.record.top = 80U;
    message.record.width = 160U;
    message.record.height = 66U;
    message.record.text_cursor_pointer_32 = 0x1234U;
    message.record.page_stop_pointer_32 = 0x1234U;
    message.text = {'%', 'Q'};
    return message;
}

void test_empty_chain_is_exact_early_return(openswd3::test::Context& test) {
    LegacyDialogRuntimeState state;
    RecordingPorts ports;

    const auto result = update_draw_legacy_dialogs(state, {}, ports);
    test.expect_true(
        result.status == LegacyDialogRuntimeStatus::idle && ports.calls.empty(),
        "a null legacy list head returns before allocating the text surface"
    );
}

void test_choice_chain_release_preserves_unrelated_state(
    openswd3::test::Context& test
) {
    LegacyDialogRuntimeState state;
    state.messages.emplace_back();
    state.messages.back().choices.push_back({1U, 2U, 3U, 4U, 5U});
    state.messages.emplace_back();
    state.messages.back().choices.push_back({1U, 6U, 7U, 8U, 9U});
    state.control.selection_state = 11U;
    state.control.advance_signal_state = 12U;
    state.close.flagged_dialog_counter = 13U;
    state.close.input_hold_state = 14U;
    state.close.close_mode_state = 15U;
    state.choice_chain_flags = 0x1234U;

    clear_legacy_dialog_choice_chain(state);

    test.expect_true(
        state.messages.front().choices.empty() &&
            state.messages.back().choices.empty() &&
            state.choice_chain_flags == 0U,
        "sub_40DBC0 releases hotspots and clears all sentinel dwords"
    );
    test.expect_true(
        state.control.selection_state == 11U &&
            state.control.advance_signal_state == 12U &&
            state.close.flagged_dialog_counter == 13U &&
            state.close.input_hold_state == 14U &&
            state.close.close_mode_state == 15U,
        "sub_40DBC0 sentinel reset does not clear unrelated dialog state"
    );
}

void test_complete_message_order_and_dimensions(openswd3::test::Context& test) {
    LegacyDialogRuntimeState state;
    LegacyDialogMessage message = terminated_message(7U);
    openswd3::asset_runtime::LegacyActionRecord frame_action{};
    openswd3::asset_runtime::LegacyActionRecord caption_action{};
    message.frame_action = &frame_action;
    message.caption_action = &caption_action;
    message.record.caption_action_pointer_32 = 0xABCDEFU;
    message.caption = {'N', 'a', 'm', 'e'};
    state.messages.push_back(std::move(message));
    RecordingPorts ports;

    const auto result = update_draw_legacy_dialogs(
        state,
        LegacyDialogRuntimeInput{
            .current_tick = 900U,
            .base_character_delay = 2U,
            .camera_left = 30,
            .camera_top = 10,
            .destination_width = 800,
            .destination_height = 600,
        },
        ports
    );

    const std::vector<Call> expected{
        Call::begin,
        Call::clear,
        Call::resolve_anchor,
        Call::panel,
        Call::clip,
        Call::text,
        Call::composite,
        Call::clear,
        Call::indicator,
        Call::caption,
        Call::end,
        Call::clip,
        Call::update_end,
        Call::update_next,
        Call::restore_destination,
    };
    test.expect_true(
        result.status == LegacyDialogRuntimeStatus::completed &&
            result.message_count == 1U && result.panel_draw_count == 1U &&
            result.composite_count == 1U && result.indicator_draw_count == 1U &&
            result.caption_draw_count == 1U && ports.calls == expected &&
            ports.surface_width == kLegacyDialogSurfaceWidth &&
            ports.surface_height == kLegacyDialogSurfaceHeight &&
            ports.composite.source_width == kLegacyDialogSurfaceWidth &&
            ports.composite.source_height == kLegacyDialogSurfaceHeight &&
            ports.composite.flags == 4U &&
            ports.indicator_kind == LegacyDialogIndicatorKind::end_dialog &&
            ports.caption_size == 4U && ports.caption_action == 0xABCDEFU &&
            ports.panel_action == &frame_action &&
            ports.live_caption_action == &caption_action &&
            ports.clips.back() == LegacyDialogRectangle{0, 0, 639, 479} &&
            ports.restored_width == 800 && ports.restored_height == 600,
        "the runtime preserves create, geometry, text, composite, caption and tail order"
    );
}

void test_close_is_composited_then_removed(openswd3::test::Context& test) {
    LegacyDialogRuntimeState state;
    LegacyDialogMessage message = terminated_message(3U);
    message.record.flags = kLegacyDialogFlagTerminated;
    state.messages.push_back(std::move(message));
    state.control.selection_state = 2U;
    state.control.advance_signal_state = 1U;
    RecordingPorts ports;
    ports.close_role_success = false;
    ports.end_action_success = false;
    ports.next_action_success = false;

    const auto result = update_draw_legacy_dialogs(state, {}, ports);
    const auto close = std::ranges::find(ports.calls, Call::close_role);
    const auto composite = std::ranges::find(ports.calls, Call::composite);
    const auto release = std::ranges::find(ports.calls, Call::release_owner);
    test.expect_true(
        result.status == LegacyDialogRuntimeStatus::completed &&
            result.removed_message_count == 1U &&
            result.nonfatal_action_failure_count == 3U &&
            state.messages.empty() && close < composite &&
            composite < release && ports.closed_role == 3U &&
            ports.released_role == 3U && state.control.selection_state == 0U &&
            state.control.advance_signal_state == 0U,
        "close side effects precede this frame's composite and tail cleanup erases the node"
    );
}

void test_surface_and_anchor_failures_release_correctly(
    openswd3::test::Context& test
) {
    LegacyDialogRuntimeState surface_state;
    surface_state.messages.push_back(terminated_message(1U));
    RecordingPorts surface_ports;
    surface_ports.begin_success = false;
    const auto unavailable =
        update_draw_legacy_dialogs(surface_state, {}, surface_ports);
    test.expect_true(
        unavailable.status == LegacyDialogRuntimeStatus::surface_unavailable &&
            surface_ports.calls == std::vector<Call>{Call::begin},
        "surface creation failure stops before the original loop"
    );

    LegacyDialogRuntimeState anchor_state;
    anchor_state.messages.push_back(terminated_message(2U));
    RecordingPorts anchor_ports;
    anchor_ports.anchor_success = false;
    const auto missing =
        update_draw_legacy_dialogs(anchor_state, {}, anchor_ports);
    test.expect_true(
        missing.status == LegacyDialogRuntimeStatus::role_anchor_unavailable &&
            !anchor_ports.calls.empty() &&
            anchor_ports.calls.back() == Call::end,
        "a checked missing role anchor still releases the reusable surface"
    );
}

void test_message_chain_release_lifecycle(openswd3::test::Context& test) {
    LegacyDialogRuntimeState state;
    for (u32 index = 0U; index < 3U; ++index) {
        state.messages.emplace_back();
        state.messages.back().text = {
            static_cast<openswd3::compat::u8>(index + 1U),
            static_cast<openswd3::compat::u8>('%'),
            static_cast<openswd3::compat::u8>('Q'),
        };
        state.messages.back().caption = {
            static_cast<openswd3::compat::u8>(index + 4U)
        };
    }
    state.control.selection_state = 5U;
    state.control.advance_signal_state = 6U;
    state.close.flagged_dialog_counter = 0x1234BEEFU;
    state.close.close_mode_state = 7U;
    state.close.input_hold_state = 8U;

    const LegacyDialogMessageReleaseResult released =
        release_legacy_dialog_messages(state);
    test.expect_true(
        released.text_release_count == 3U &&
            released.node_release_count == 3U &&
            released.preserved_lock_value == 0x8000U &&
            state.messages.empty() &&
            state.close.flagged_dialog_counter == 0x8000U,
        "sub_40F5A0 releases each text before its message and preserves only bit 15"
    );
    test.expect_true(
        state.control.selection_state == 5U &&
            state.control.advance_signal_state == 6U &&
            state.close.close_mode_state == 7U &&
            state.close.input_hold_state == 8U,
        "sub_40F5A0 leaves unrelated dialog globals untouched"
    );

    const LegacyDialogMessageReleaseResult empty =
        release_legacy_dialog_messages(state);
    test.expect_true(
        empty.text_release_count == 0U && empty.node_release_count == 0U &&
            empty.preserved_lock_value == 0x8000U,
        "sub_40F5A0 still applies the bit-15 mask to an empty chain"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_empty_chain_is_exact_early_return(test);
    test_choice_chain_release_preserves_unrelated_state(test);
    test_complete_message_order_and_dimensions(test);
    test_close_is_composited_then_removed(test);
    test_surface_and_anchor_failures_release_correctly(test);
    test_message_chain_release_lifecycle(test);
    return test.exit_code();
}
