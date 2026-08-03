#pragma once

#include "openswd3/app/command_line.hpp"

#include <optional>
#include <string_view>

namespace openswd3::app {

class ExistingInstancePorts {
public:
    virtual ~ExistingInstancePorts() = default;

    [[nodiscard]] virtual bool matching_instance_exists() = 0;
};

enum class ProcessStartupGateResult {
    continue_normal_startup,
    exit_zero_existing_instance,
    exit_zero_command_line_handled,
};

[[nodiscard]] ProcessStartupGateResult run_process_startup_gates(
    std::optional<std::string_view> command_line,
    ExistingInstancePorts& instance_ports,
    CommandLinePorts& command_line_ports
);

}  // namespace openswd3::app
