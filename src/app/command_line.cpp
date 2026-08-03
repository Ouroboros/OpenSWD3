#include "openswd3/app/command_line.hpp"

namespace openswd3::app {

bool run_nonempty_command_line_path(
    const std::optional<std::string_view> command_line,
    CommandLinePorts& ports
) {
    if (!command_line.has_value() || command_line->empty()) {
        return false;
    }

    ports.initialize_float_conversion();
    const compat::u8 selector = static_cast<compat::u8>(
        static_cast<compat::u8>((*command_line)[0]) -
        static_cast<compat::u8>('0')
    );
    ports.run_legacy_command(selector, command_line->substr(1));
    return true;
}

void seed_two_rng_streams(RngSeedPorts& ports) {
    ports.seed_crt_rng(ports.read_time_seconds());
    ports.seed_secondary_rng(ports.read_time_seconds());
}

}  // namespace openswd3::app
