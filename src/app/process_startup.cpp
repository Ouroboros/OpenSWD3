#include "openswd3/app/process_startup.hpp"

namespace openswd3::app {

ProcessStartupGateResult run_process_startup_gates(
    const std::optional<std::string_view> command_line,
    ExistingInstancePorts& instance_ports,
    CommandLinePorts& command_line_ports
) {
    if (instance_ports.matching_instance_exists()) {
        return ProcessStartupGateResult::exit_zero_existing_instance;
    }
    if (run_nonempty_command_line_path(command_line, command_line_ports)) {
        return ProcessStartupGateResult::exit_zero_command_line_handled;
    }
    return ProcessStartupGateResult::continue_normal_startup;
}

}  // namespace openswd3::app
