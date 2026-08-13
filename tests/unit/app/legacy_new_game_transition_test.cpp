#include "test.hpp"

#include "openswd3/app/legacy_new_game_transition.hpp"

#include <vector>

namespace {

using openswd3::app::BattleTransitionState;
using openswd3::app::LegacyNewGameTransitionPorts;
using openswd3::compat::u32;

enum class Call {
    clear_accumulated_play_time,
    sample_epoch_seconds,
    set_play_time_origin,
    set_initial_menu_phase,
    clear_game_framebuffer,
    initialize_new_game_state_and_world,
    set_high_priority_submode,
    set_high_priority_auxiliary,
    reset_input_menu_and_save_previews,
    apply_new_game_name_overrides,
    load_fame_table,
};

class RecordingPorts final : public LegacyNewGameTransitionPorts {
public:
    void clear_accumulated_play_time() override {
        calls.push_back(Call::clear_accumulated_play_time);
    }

    u32 sample_epoch_seconds() override {
        calls.push_back(Call::sample_epoch_seconds);
        return sampled_seconds;
    }

    void set_play_time_origin(const u32 seconds) override {
        calls.push_back(Call::set_play_time_origin);
        received_seconds = seconds;
    }

    void set_initial_menu_phase(const u32 phase) override {
        calls.push_back(Call::set_initial_menu_phase);
        received_menu_phase = phase;
    }

    void clear_game_framebuffer() override {
        calls.push_back(Call::clear_game_framebuffer);
    }

    bool initialize_new_game_state_and_world() override {
        calls.push_back(Call::initialize_new_game_state_and_world);
        return world_ready;
    }

    void set_high_priority_submode(const u32 value) override {
        calls.push_back(Call::set_high_priority_submode);
        received_high_priority_submode = value;
    }

    void set_high_priority_auxiliary(const u32 value) override {
        calls.push_back(Call::set_high_priority_auxiliary);
        received_high_priority_auxiliary = value;
    }

    void reset_input_menu_and_save_previews() override {
        calls.push_back(Call::reset_input_menu_and_save_previews);
    }

    void apply_new_game_name_overrides() override {
        calls.push_back(Call::apply_new_game_name_overrides);
    }

    void load_fame_table() override {
        calls.push_back(Call::load_fame_table);
    }

    bool world_ready{true};
    u32 sampled_seconds{0x12345678U};
    u32 received_seconds{};
    u32 received_menu_phase{};
    u32 received_high_priority_submode{};
    u32 received_high_priority_auxiliary{};
    std::vector<Call> calls;
};

const std::vector<Call> kExpectedCalls{
    Call::clear_accumulated_play_time,
    Call::sample_epoch_seconds,
    Call::set_play_time_origin,
    Call::set_initial_menu_phase,
    Call::clear_game_framebuffer,
    Call::initialize_new_game_state_and_world,
    Call::set_high_priority_submode,
    Call::set_high_priority_auxiliary,
    Call::reset_input_menu_and_save_previews,
    Call::apply_new_game_name_overrides,
    Call::load_fame_table,
};

void test_exact_case_one_order(openswd3::test::Context& test) {
    BattleTransitionState state{7U, 8U, 0x80000003U, 9U};
    RecordingPorts ports;

    const auto result =
        openswd3::app::run_legacy_new_game_transition(state, ports);

    test.expect_equal(ports.calls, kExpectedCalls, "case one call order");
    test.expect_true(result.initial_world_ready, "world result is observable");
    test.expect_equal(
        result.sampled_epoch_seconds, 0x12345678U, "sampled time is returned"
    );
    test.expect_true(
        ports.received_seconds == 0x12345678U &&
            ports.received_menu_phase == 5U &&
            ports.received_high_priority_submode == 1U &&
            ports.received_high_priority_auxiliary == 1U,
        "literal state values match 0x004492BA branch"
    );
    test.expect_true(
        state.battle_request_value == 7U && state.battle_active == 8U &&
            state.special_mode_state == 0U && state.high_priority_state == 0U,
        "only the two directly written coordinator fields are changed"
    );
}

void test_checked_world_failure_does_not_skip_legacy_tail(
    openswd3::test::Context& test
) {
    BattleTransitionState state{0U, 0U, 3U, 4U};
    RecordingPorts ports;
    ports.world_ready = false;

    const auto result =
        openswd3::app::run_legacy_new_game_transition(state, ports);

    test.expect_false(
        result.initial_world_ready, "checked loader failure remains visible"
    );
    test.expect_equal(
        ports.calls,
        kExpectedCalls,
        "ignored original return cannot suppress post-load calls"
    );
    test.expect_true(
        state.special_mode_state == 0U && state.high_priority_state == 0U,
        "mode writes still occur after loader failure"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_case_one_order(test);
    test_checked_world_failure_does_not_skip_legacy_tail(test);
    return test.exit_code();
}
