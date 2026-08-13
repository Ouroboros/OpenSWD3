#include "test.hpp"

#include "openswd3/app/command_line.hpp"

#include <string>
#include <vector>

namespace {

enum class Call {
    default_key_bindings,
    legacy_command,
    read_time,
    seed_crt,
    seed_secondary,
};

struct Event {
    Call call{};
    openswd3::compat::u32 value{};
    std::string payload;

    bool operator==(const Event&) const = default;
};

class RecordingCommandLinePorts final : public openswd3::app::CommandLinePorts {
public:
    void initialize_default_key_bindings() override {
        events.push_back({Call::default_key_bindings, 0, {}});
    }
    void run_legacy_command(
        const openswd3::compat::u8 selector, const std::string_view payload
    ) override {
        events.push_back(
            {Call::legacy_command, selector, std::string(payload)}
        );
    }

    std::vector<Event> events;
};

class RecordingRngPorts final : public openswd3::app::RngSeedPorts {
public:
    openswd3::compat::u32 read_time_seconds() override {
        const auto value = time_values.at(time_index++);
        events.push_back({Call::read_time, value, {}});
        return value;
    }
    void seed_crt_rng(const openswd3::compat::u32 seed) override {
        events.push_back({Call::seed_crt, seed, {}});
    }
    void seed_secondary_rng(const openswd3::compat::u32 seed) override {
        events.push_back({Call::seed_secondary, seed, {}});
    }

    std::vector<openswd3::compat::u32> time_values{11, 22};
    std::size_t time_index{};
    std::vector<Event> events;
};

}  // namespace

int main() {
    openswd3::test::Context test;
    RecordingCommandLinePorts empty_ports;
    test.expect_false(
        openswd3::app::run_nonempty_command_line_path(
            std::nullopt, empty_ports
        ),
        "null command line continues normal startup"
    );
    test.expect_false(
        openswd3::app::run_nonempty_command_line_path(
            std::string_view{}, empty_ports
        ),
        "empty command line continues normal startup"
    );
    test.expect_true(
        empty_ports.events.empty(), "empty paths call no command port"
    );

    RecordingCommandLinePorts command_ports;
    test.expect_true(
        openswd3::app::run_nonempty_command_line_path(
            "5payload", command_ports
        ),
        "nonempty command line consumes normal startup"
    );
    const std::vector<Event> command_expected{
        {Call::default_key_bindings, 0, {}},
        {Call::legacy_command, 5, "payload"},
    };
    test.expect_equal(
        command_ports.events, command_expected, "command byte order"
    );

    RecordingCommandLinePorts wrapped_selector_ports;
    test.expect_true(
        openswd3::app::run_nonempty_command_line_path(
            "/x", wrapped_selector_ports
        ),
        "non-digit selector still consumes normal startup"
    );
    test.expect_equal(
        wrapped_selector_ports.events.back().value,
        0xFFU,
        "selector subtraction retains unsigned byte wrap"
    );

    RecordingRngPorts rng_ports;
    openswd3::app::seed_two_rng_streams(rng_ports);
    const std::vector<Event> rng_expected{
        {Call::read_time, 11, {}},
        {Call::seed_crt, 11, {}},
        {Call::read_time, 22, {}},
        {Call::seed_secondary, 22, {}},
    };
    test.expect_equal(
        rng_ports.events,
        rng_expected,
        "the two RNG streams use two independent time reads"
    );
    return test.exit_code();
}
