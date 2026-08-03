#include "openswd3/app/window_events.hpp"

namespace openswd3::app {

namespace {

constexpr compat::u32 kLegacySizeRestored = 0U;
constexpr compat::u32 kLegacySizeMinimized = 1U;
constexpr compat::u32 kLegacySizeMaximized = 2U;
constexpr compat::u32 kLegacySizeMaxHide = 3U;
constexpr compat::u32 kLegacySizeMaxShow = 4U;

constexpr compat::u8 kLegacyKeyEnter = 0x0DU;
constexpr compat::u32 kLegacyKeyEscape = 0x1BU;
constexpr compat::u32 kLegacyKeyScreenshot = 0x50U;
constexpr compat::u32 kLegacyKeyPause = 0x77U;

constexpr compat::u32 kMinimumScreenshotSpaceMebibytes = 0x40U;

void release_active_video(
    WindowEventState& state,
    WindowEventPorts& ports
) {
    ports.release_active_video();
    state.process_flags &= ~kProcessVideoActive;
}

}  // namespace

DisplayTransition select_size_display_transition(
    const compat::u32 runtime_initialized,
    const compat::u32 size_code
) noexcept {
    if (runtime_initialized == 0U) {
        return DisplayTransition::none;
    }

    switch (size_code) {
    case kLegacySizeMinimized:
    case kLegacySizeMaxHide:
    case kLegacySizeMaxShow:
        return DisplayTransition::deactivate;
    case kLegacySizeRestored:
    case kLegacySizeMaximized:
        return DisplayTransition::reactivate;
    default:
        return DisplayTransition::none;
    }
}

DisplayTransition select_activation_display_transition(
    const compat::u32 runtime_initialized,
    const compat::u32 activation_value
) noexcept {
    if (runtime_initialized == 0U) {
        return DisplayTransition::none;
    }
    return static_cast<compat::u16>(activation_value) == 0U
               ? DisplayTransition::deactivate
               : DisplayTransition::reactivate;
}

DisplayTransition select_system_enter_display_transition(
    const compat::u32 key_value,
    const compat::u32 display_active
) noexcept {
    if (static_cast<compat::u8>(key_value) != kLegacyKeyEnter) {
        return DisplayTransition::none;
    }
    return display_active == 1U ? DisplayTransition::deactivate
                                : DisplayTransition::reactivate;
}

void apply_display_transition(
    const DisplayTransition transition,
    DisplayLifecycleState& state,
    DisplayLifecyclePorts& ports
) {
    switch (transition) {
    case DisplayTransition::deactivate:
        deactivate_display(state, ports);
        return;
    case DisplayTransition::reactivate:
        reactivate_display(state, ports);
        return;
    case DisplayTransition::none:
        return;
    }
}

bool should_intercept_system_command(const compat::u32 command) noexcept {
    return command == 0xF130U || command == 0xF140U ||
           command == 0xF170U || command == 0xF090U;
}

WindowMessagePrefixResult run_window_message_prefix(
    const compat::u32 text_input_active,
    const compat::u32 message,
    const compat::u32 first_parameter,
    const compat::u32 second_parameter,
    TextInputMessagePorts& ports
) {
    if (text_input_active == 0U) {
        return WindowMessagePrefixResult::continue_dispatch;
    }

    return ports.filter_text_input_message(
               message,
               first_parameter,
               second_parameter
           ) == 0U
               ? WindowMessagePrefixResult::consume_and_return_one
               : WindowMessagePrefixResult::continue_dispatch;
}

void handle_key_release(
    WindowEventState& state,
    const compat::u32 key_value,
    WindowEventPorts& ports
) {
    const bool video_active =
        (state.process_flags & kProcessVideoActive) != 0U;

    if (video_active && key_value == kLegacyKeyEscape) {
        release_active_video(state, ports);
    } else if (key_value == kLegacyKeyPause) {
        state.frame_execution_gate =
            video_active ? 1U
                         : (state.frame_execution_gate == 0U ? 1U : 0U);
    }

    if (state.runtime_initialized == 0U ||
        (state.process_flags & kProcessIdleSuppression) != 0U ||
        key_value != kLegacyKeyScreenshot) {
        return;
    }

    if (ports.free_disk_space_mebibytes() >
        kMinimumScreenshotSpaceMebibytes) {
        ports.capture_legacy_screenshot();
    }
}

void handle_left_button_down(
    WindowEventState& state,
    const compat::u32 button_state,
    WindowEventPorts& ports
) {
    if ((state.process_flags & kProcessVideoActive) != 0U &&
        button_state == kLegacyKeyEscape) {
        release_active_video(state, ports);
    }
}

void handle_window_destroy(
    const WindowEventState& state,
    ShutdownPorts& shutdown_ports,
    ProcessExitPorts& exit_ports
) {
    if (state.runtime_initialized != 0U ||
        (state.process_flags & kProcessCloseRequested) != 0U) {
        static_cast<void>(run_total_shutdown(shutdown_ports));
    }
    exit_ports.uninitialize_com();
    exit_ports.post_quit_message_zero();
}

}  // namespace openswd3::app
