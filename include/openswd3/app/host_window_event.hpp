#pragma once

#include "openswd3/app/display_lifecycle.hpp"
#include "openswd3/app/window_events.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::app {

enum class HostWindowEventKind {
    request_close,
    size,
    activation,
    system_key_down,
    key_release,
    left_button_down,
};

struct HostWindowEvent {
    HostWindowEventKind kind{};
    compat::u32 value{};
};

enum class HostWindowEventResult {
    continue_running,
    request_close,
};

[[nodiscard]] HostWindowEventResult dispatch_host_window_event(
    const HostWindowEvent& event,
    WindowEventState& window_state,
    WindowEventPorts& window_ports,
    DisplayLifecycleState& display_state,
    DisplayLifecyclePorts& display_ports
);

}  // namespace openswd3::app
