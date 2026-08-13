#pragma once

#include "openswd3/app/display_lifecycle.hpp"
#include "openswd3/app/frame_dispatch.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::app {

enum class DisplayTransition {
    none,
    deactivate,
    reactivate,
};

[[nodiscard]] DisplayTransition select_size_display_transition(
    compat::u32 runtime_initialized, compat::u32 size_code
) noexcept;

[[nodiscard]] DisplayTransition select_activation_display_transition(
    compat::u32 runtime_initialized, compat::u32 activation_value
) noexcept;

[[nodiscard]] DisplayTransition select_system_enter_display_transition(
    compat::u32 key_value, compat::u32 display_active
) noexcept;

void apply_display_transition(
    DisplayTransition transition,
    DisplayLifecycleState& state,
    DisplayLifecyclePorts& ports
);

[[nodiscard]] bool
should_intercept_system_command(compat::u32 command) noexcept;

enum class WindowMessagePrefixResult {
    continue_dispatch,
    consume_and_return_one,
};

class TextInputMessagePorts {
public:
    virtual ~TextInputMessagePorts() = default;

    [[nodiscard]] virtual compat::u32 filter_text_input_message(
        compat::u32 message,
        compat::u32 first_parameter,
        compat::u32 second_parameter
    ) = 0;
};

[[nodiscard]] WindowMessagePrefixResult run_window_message_prefix(
    compat::u32 text_input_active,
    compat::u32 message,
    compat::u32 first_parameter,
    compat::u32 second_parameter,
    TextInputMessagePorts& ports
);

struct WindowEventState {
    compat::u32 runtime_initialized{};
    compat::u32 process_flags{};
    compat::u32 frame_execution_gate{};
};

class WindowEventPorts {
public:
    virtual ~WindowEventPorts() = default;

    virtual void release_active_video() = 0;
    [[nodiscard]] virtual compat::u32 free_disk_space_mebibytes() = 0;
    virtual void capture_legacy_screenshot() = 0;
};

void handle_key_release(
    WindowEventState& state, compat::u32 key_value, WindowEventPorts& ports
);

void handle_left_button_down(
    WindowEventState& state, compat::u32 button_state, WindowEventPorts& ports
);

class ProcessExitPorts {
public:
    virtual ~ProcessExitPorts() = default;

    virtual void uninitialize_com() = 0;
    virtual void post_quit_message_zero() = 0;
};

void handle_window_destroy(
    const WindowEventState& state,
    ShutdownPorts& shutdown_ports,
    ProcessExitPorts& exit_ports
);

}  // namespace openswd3::app
