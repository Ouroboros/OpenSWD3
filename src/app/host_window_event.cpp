#include "openswd3/app/host_window_event.hpp"

namespace openswd3::app {

HostWindowEventResult dispatch_host_window_event(
    const HostWindowEvent& event,
    WindowEventState& window_state,
    WindowEventPorts& window_ports,
    DisplayLifecycleState& display_state,
    DisplayLifecyclePorts& display_ports
) {
    switch (event.kind) {
    case HostWindowEventKind::request_close:
        return HostWindowEventResult::request_close;
    case HostWindowEventKind::size:
        apply_display_transition(
            select_size_display_transition(
                window_state.runtime_initialized, event.value
            ),
            display_state,
            display_ports
        );
        break;
    case HostWindowEventKind::activation:
        apply_display_transition(
            select_activation_display_transition(
                window_state.runtime_initialized, event.value
            ),
            display_state,
            display_ports
        );
        break;
    case HostWindowEventKind::system_key_down:
        apply_display_transition(
            select_system_enter_display_transition(
                event.value, display_state.display_active
            ),
            display_state,
            display_ports
        );
        break;
    case HostWindowEventKind::key_release:
        handle_key_release(window_state, event.value, window_ports);
        break;
    case HostWindowEventKind::left_button_down:
        handle_left_button_down(window_state, event.value, window_ports);
        break;
    }
    return HostWindowEventResult::continue_running;
}

}  // namespace openswd3::app
