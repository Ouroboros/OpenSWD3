#include "test.hpp"

#include "openswd3/app/frame_preparation.hpp"

#include <vector>

namespace {

using openswd3::compat::u32;

enum class CallKind {
    yield,
    seconds,
    milliseconds,
    query_flag,
    clear_flag,
    set_flag,
    primary_operation,
    party_member,
    sample_input,
    normalize_input,
};

struct Call {
    CallKind kind{};
    u32 value{};

    bool operator==(const Call&) const = default;
};

class RecordingPorts final : public openswd3::app::FramePreparationPorts {
public:
    void yield() override {
        calls.push_back({CallKind::yield, 0U});
    }

    u32 read_seconds() override {
        calls.push_back({CallKind::seconds, 0U});
        return seconds;
    }

    u32 read_milliseconds() override {
        calls.push_back({CallKind::milliseconds, 0U});
        return milliseconds;
    }

    bool query_internal_flag(const u32 index) override {
        calls.push_back({CallKind::query_flag, index});
        if (index == openswd3::app::kPrimaryTransitionFlag) {
            return primary_flag;
        }
        if (index == openswd3::app::kSecondaryTransitionFlag) {
            return secondary_flag;
        }
        return false;
    }

    void clear_internal_flag(const u32 index) override {
        calls.push_back({CallKind::clear_flag, index});
    }

    void set_internal_flag(const u32 index) override {
        calls.push_back({CallKind::set_flag, index});
    }

    void perform_primary_transition_operation(
        const openswd3::app::PrimaryTransitionOperation operation
    ) override {
        calls.push_back(
            {CallKind::primary_operation, static_cast<u32>(operation)}
        );
    }

    void release_and_clear_party_member_transition(
        const u32 member_index
    ) override {
        calls.push_back({CallKind::party_member, member_index});
    }

    void sample_input_device() override {
        calls.push_back({CallKind::sample_input, 0U});
    }

    void normalize_input() override {
        calls.push_back({CallKind::normalize_input, 0U});
    }

    u32 seconds{77U};
    u32 milliseconds{100U};
    bool primary_flag{};
    bool secondary_flag{};
    std::vector<Call> calls;
};

openswd3::app::FramePreparationState make_state() {
    openswd3::app::FramePreparationState state{};
    state.display_active = 1U;
    state.frame_interval_milliseconds = 35U;
    return state;
}

void test_entry_gates(openswd3::test::Context& test) {
    using openswd3::app::FramePreparationOutcome;

    auto state = make_state();
    state.process_flags = 0x10U;
    RecordingPorts ports;
    test.expect_equal(
        openswd3::app::run_frame_preparation(state, ports),
        FramePreparationOutcome::return_immediately,
        "process bit 0x10 returns before all ports"
    );
    test.expect_true(ports.calls.empty(), "suppressed frame has no side effects");

    state = make_state();
    state.display_active = 0U;
    test.expect_equal(
        openswd3::app::run_frame_preparation(state, ports),
        FramePreparationOutcome::yielded_display_inactive,
        "zero display yields and returns"
    );
    test.expect_equal(
        ports.calls,
        std::vector<Call>{{CallKind::yield, 0U}},
        "inactive display calls only yield"
    );
}

void test_interval_gate_and_wrap(openswd3::test::Context& test) {
    using openswd3::app::FramePreparationOutcome;

    auto state = make_state();
    state.previous_accepted_frame_milliseconds = 90U;
    state.frame_interval_milliseconds = 11U;
    RecordingPorts ports;
    test.expect_equal(
        openswd3::app::run_frame_preparation(state, ports),
        FramePreparationOutcome::interval_not_elapsed,
        "elapsed below threshold rejects the frame"
    );
    test.expect_equal(state.sampled_seconds, 77U, "seconds are stored before rejection");
    test.expect_equal(state.sampled_milliseconds, 100U, "milliseconds are stored before rejection");
    test.expect_equal(state.previous_accepted_frame_milliseconds, 90U, "rejection preserves previous accepted time");
    test.expect_equal(
        ports.calls,
        std::vector<Call>{
            {CallKind::seconds, 0U},
            {CallKind::milliseconds, 0U},
        },
        "rejected frame stops before migrations and input"
    );

    state = make_state();
    state.previous_accepted_frame_milliseconds = 0xFFFFFFF0U;
    state.frame_interval_milliseconds = 0x20U;
    ports.calls.clear();
    ports.milliseconds = 0x10U;
    test.expect_equal(
        openswd3::app::run_frame_preparation(state, ports),
        FramePreparationOutcome::accepted,
        "unsigned wrap difference equal to threshold accepts"
    );
    test.expect_equal(state.previous_accepted_frame_milliseconds, 0x10U, "accepted frame stores now, not previous plus interval");
    test.expect_equal(state.frame_delta_milliseconds, 0x10U, "input delta uses independent previous value");

    state = make_state();
    state.previous_accepted_frame_milliseconds = 100U;
    state.frame_interval_milliseconds = 0U;
    ports.calls.clear();
    ports.milliseconds = 100U;
    test.expect_equal(
        openswd3::app::run_frame_preparation(state, ports),
        FramePreparationOutcome::accepted,
        "zero threshold accepts the same millisecond"
    );
}

void test_input_prefix_and_sample_order(openswd3::test::Context& test) {
    auto state = make_state();
    state.frame_interval_milliseconds = 0U;
    state.input_backend_flags = 1U;
    state.special_mode_state = 7U;
    state.previous_input_milliseconds = 40U;
    RecordingPorts ports;

    static_cast<void>(openswd3::app::run_frame_preparation(state, ports));
    const std::vector<Call> expected{
        {CallKind::seconds, 0U},
        {CallKind::milliseconds, 0U},
        {CallKind::clear_flag, 3U},
        {CallKind::clear_flag, 4U},
        {CallKind::sample_input, 0U},
        {CallKind::normalize_input, 0U},
    };
    test.expect_equal(ports.calls, expected, "accepted frame prefix call order");
    test.expect_equal(state.current_frame_milliseconds, 100U, "current frame time");
    test.expect_equal(state.frame_delta_milliseconds, 60U, "delta precedes input sampling");
    test.expect_equal(state.previous_input_milliseconds, 100U, "input previous time advances to now");
}

void test_primary_transition(openswd3::test::Context& test) {
    using enum openswd3::app::PrimaryTransitionOperation;

    auto state = make_state();
    state.frame_interval_milliseconds = 0U;
    state.primary_countdown = 0U;
    state.value_004b72c4 = 0xFFFFU;
    state.party_member_count = 3U;
    state.value_004a93d4 = 0x1234U;
    state.value_004b7bc4 = 0x5678U;
    state.value_004b72b4 = 1U;
    state.value_004b72c0 = 2U;
    state.value_004b72be = 3U;
    state.value_004b72b0 = 4U;
    state.value_004b72a4 = 5U;
    state.value_004b72a8 = 6U;
    RecordingPorts ports;
    ports.primary_flag = true;

    static_cast<void>(openswd3::app::run_frame_preparation(state, ports));
    const std::vector<Call> expected{
        {CallKind::seconds, 0U},
        {CallKind::milliseconds, 0U},
        {CallKind::query_flag, 0x10U},
        {CallKind::clear_flag, 0x10U},
        {CallKind::clear_flag, 0x12U},
        {CallKind::set_flag, 0x11U},
        {CallKind::primary_operation, static_cast<u32>(release_0040f5e0)},
        {CallKind::primary_operation, static_cast<u32>(release_0040f500)},
        {CallKind::primary_operation, static_cast<u32>(release_0040f540)},
        {CallKind::primary_operation, static_cast<u32>(release_0040f570)},
        {CallKind::primary_operation, static_cast<u32>(release_0040dbc0)},
        {CallKind::primary_operation, static_cast<u32>(release_0040f5a0)},
        {CallKind::party_member, 1U},
        {CallKind::party_member, 2U},
        {CallKind::query_flag, 0x4AU},
        {CallKind::sample_input, 0U},
        {CallKind::normalize_input, 0U},
    };
    test.expect_equal(ports.calls, expected, "primary transition exact order");
    test.expect_equal(state.primary_countdown, 0xFFFFFFFFU, "primary countdown clamps to minus one");
    test.expect_equal(state.value_004b72b4, 0U, "004b72b4 clears");
    test.expect_equal(state.value_004b72c0, 0U, "004b72c0 clears");
    test.expect_equal(state.value_004b72be, 0x1234U, "004a93d4 copies to 004b72be");
    test.expect_equal(state.value_004b72c4, 0x5678U, "004b7bc4 copies to 004b72c4");
    test.expect_equal(state.value_004b72b0, 0U, "004b72b0 clears");
    test.expect_equal(state.value_004b72a4, 0U, "004b72a4 clears");
    test.expect_equal(state.value_004b72a8, 0U, "004b72a8 clears");
}

void test_transition_guards_and_secondary(openswd3::test::Context& test) {
    auto state = make_state();
    state.frame_interval_milliseconds = 0U;
    state.primary_countdown = 0U;
    state.value_004b72c4 = 0xFFFFU;
    state.high_priority_state = 1U;
    state.secondary_countdown = 0U;
    RecordingPorts ports;
    ports.primary_flag = true;
    ports.secondary_flag = true;

    static_cast<void>(openswd3::app::run_frame_preparation(state, ports));
    test.expect_equal(state.primary_countdown, 0xFFFFFFFFU, "blocked primary countdown still decrements");
    test.expect_equal(state.value_004b72c4, 0xFFFFU, "high-priority gate blocks primary cleanup");
    test.expect_equal(state.secondary_countdown, 0xFFFFFFFFU, "secondary transition still triggers");
    const std::vector<Call> suffix{
        {CallKind::query_flag, 0x4AU},
        {CallKind::clear_flag, 0x4AU},
        {CallKind::set_flag, 0x4BU},
        {CallKind::sample_input, 0U},
        {CallKind::normalize_input, 0U},
    };
    test.expect_equal(
        std::vector<Call>(ports.calls.end() - 5, ports.calls.end()),
        suffix,
        "secondary transition flag order"
    );

    state = make_state();
    state.frame_interval_milliseconds = 0U;
    state.special_mode_state = 1U;
    ports.calls.clear();
    static_cast<void>(openswd3::app::run_frame_preparation(state, ports));
    test.expect_equal(
        ports.calls,
        std::vector<Call>{
            {CallKind::seconds, 0U},
            {CallKind::milliseconds, 0U},
            {CallKind::sample_input, 0U},
            {CallKind::normalize_input, 0U},
        },
        "nonzero special mode skips both countdown flag queries"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_entry_gates(test);
    test_interval_gate_and_wrap(test);
    test_input_prefix_and_sample_order(test);
    test_primary_transition(test);
    test_transition_guards_and_secondary(test);
    return test.exit_code();
}
