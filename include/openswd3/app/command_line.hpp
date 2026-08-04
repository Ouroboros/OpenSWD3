#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>
#include <string_view>

namespace openswd3::app {

class CommandLinePorts {
public:
    virtual ~CommandLinePorts() = default;

    virtual void initialize_default_key_bindings() = 0;
    virtual void run_legacy_command(
        compat::u8 selector,
        std::string_view payload
    ) = 0;
};

[[nodiscard]] bool run_nonempty_command_line_path(
    std::optional<std::string_view> command_line,
    CommandLinePorts& ports
);

class RngSeedPorts {
public:
    virtual ~RngSeedPorts() = default;

    virtual compat::u32 read_time_seconds() = 0;
    virtual void seed_crt_rng(compat::u32 seed) = 0;
    virtual void seed_secondary_rng(compat::u32 seed) = 0;
};

void seed_two_rng_streams(RngSeedPorts& ports);

}  // namespace openswd3::app
