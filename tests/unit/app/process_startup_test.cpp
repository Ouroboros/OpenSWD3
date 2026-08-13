#include "test.hpp"

#include "openswd3/app/process_startup.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace {

enum class Call {
    check_instance,
    initialize_default_key_bindings,
    command,
};

class RecordingInstancePorts final
    : public openswd3::app::ExistingInstancePorts {
public:
    explicit RecordingInstancePorts(std::vector<Call>& calls) : calls_(calls) {}

    bool matching_instance_exists() override {
        calls_.push_back(Call::check_instance);
        return exists;
    }

    bool exists{};

private:
    std::vector<Call>& calls_;
};

class RecordingCommandLinePorts final : public openswd3::app::CommandLinePorts {
public:
    explicit RecordingCommandLinePorts(std::vector<Call>& calls)
        : calls_(calls) {}

    void initialize_default_key_bindings() override {
        calls_.push_back(Call::initialize_default_key_bindings);
    }

    void run_legacy_command(
        const openswd3::compat::u8 selector, const std::string_view payload
    ) override {
        calls_.push_back(Call::command);
        seen_selector = selector;
        seen_payload = payload;
    }

    openswd3::compat::u8 seen_selector{};
    std::string_view seen_payload;

private:
    std::vector<Call>& calls_;
};

void test_gates(openswd3::test::Context& test) {
    using openswd3::app::ProcessStartupGateResult;
    using openswd3::app::run_process_startup_gates;

    std::vector<Call> calls;
    RecordingInstancePorts instance_ports(calls);
    RecordingCommandLinePorts command_ports(calls);

    instance_ports.exists = true;
    test.expect_equal(
        run_process_startup_gates("7payload", instance_ports, command_ports),
        ProcessStartupGateResult::exit_zero_existing_instance,
        "existing instance exits before command-line handling"
    );
    test.expect_equal(
        calls,
        std::vector<Call>{Call::check_instance},
        "existing-instance check is the first and only call"
    );

    calls.clear();
    instance_ports.exists = false;
    test.expect_equal(
        run_process_startup_gates("7payload", instance_ports, command_ports),
        ProcessStartupGateResult::exit_zero_command_line_handled,
        "nonempty command line is the second early exit"
    );
    test.expect_equal(
        calls,
        std::vector<Call>{
            Call::check_instance,
            Call::initialize_default_key_bindings,
            Call::command,
        },
        "command-line path runs only after instance check"
    );
    test.expect_equal(command_ports.seen_selector, 7U, "selector is preserved");
    test.expect_equal(
        command_ports.seen_payload,
        std::string_view{"payload"},
        "payload excludes the first byte"
    );

    calls.clear();
    test.expect_equal(
        run_process_startup_gates(
            std::optional<std::string_view>{std::string_view{}},
            instance_ports,
            command_ports
        ),
        ProcessStartupGateResult::continue_normal_startup,
        "empty command line reaches normal startup"
    );
    test.expect_equal(
        calls,
        std::vector<Call>{Call::check_instance},
        "empty command line has no command side effects"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_gates(test);
    return test.exit_code();
}
