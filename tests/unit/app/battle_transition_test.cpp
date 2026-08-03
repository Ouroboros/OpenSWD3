#include "test.hpp"

#include "openswd3/app/battle_transition.hpp"

#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

enum class Call {
    release_entry_objects,
    close_world_map,
    initialize_battle,
    clear_party_bits,
    step_battle,
    maintain_audio,
    rebuild_display_zero,
    set_world_state_zero,
    reopen_world_map_zero,
    resume_audio_zero,
    prepare_result_two,
    clear_result_two_auxiliary,
    finish_result_two,
    clear_result_three,
    remap_result_three,
};

struct Event {
    Call call{};
    u32 argument{};
    u32 request_value{};
    u32 battle_active{};
    u32 special_mode_state{};
    u32 high_priority_state{};

    bool operator==(const Event&) const = default;
};

class RecordingPorts final : public openswd3::app::BattleTransitionPorts {
public:
    RecordingPorts(openswd3::app::BattleTransitionState& state, const i32 result)
        : state_(state), result_(result) {}

    void release_display_and_world_for_battle_entry() override {
        record(Call::release_entry_objects);
    }
    void close_world_map_view() override {
        record(Call::close_world_map);
    }
    void initialize_battle(const u16 battle_id) override {
        record(Call::initialize_battle, battle_id);
    }
    void clear_party_battle_entry_bits() override {
        record(Call::clear_party_bits);
    }
    i32 step_battle() override {
        record(Call::step_battle);
        return result_;
    }
    void maintain_audio() override {
        record(Call::maintain_audio);
    }
    void rebuild_display_after_result_zero() override {
        record(Call::rebuild_display_zero);
    }
    void set_result_zero_world_state() override {
        record(Call::set_world_state_zero);
    }
    void reopen_world_map_after_result_zero() override {
        record(Call::reopen_world_map_zero);
    }
    void resume_audio_after_result_zero() override {
        record(Call::resume_audio_zero);
    }
    void prepare_result_two_internal_state() override {
        record(Call::prepare_result_two);
    }
    void clear_result_two_auxiliary_state() override {
        record(Call::clear_result_two_auxiliary);
    }
    void finish_result_two_mode_transition() override {
        record(Call::finish_result_two);
    }
    void clear_result_three_internal_state() override {
        record(Call::clear_result_three);
    }
    void remap_world_after_result_three() override {
        record(Call::remap_result_three);
    }

    std::vector<Event> events;

private:
    void record(const Call call, const u32 argument = 0U) {
        events.push_back(
            {
                call,
                argument,
                state_.battle_request_value,
                state_.battle_active,
                state_.special_mode_state,
                state_.high_priority_state,
            }
        );
    }

    openswd3::app::BattleTransitionState& state_;
    i32 result_{};
};

void test_entry_guards(openswd3::test::Context& test) {
    for (const auto& [state_value, blocked] : {
             std::pair{openswd3::app::BattleTransitionState{0x80000005U, 1, 0, 0}, false},
             std::pair{openswd3::app::BattleTransitionState{0x80000005U, 0, 0, 0}, true},
             std::pair{openswd3::app::BattleTransitionState{5, 0, 0, 0}, false},
         }) {
        auto state = state_value;
        RecordingPorts ports(state, 1);
        test.expect_false(
            openswd3::app::consume_battle_request(state, blocked, ports),
            "inactive, unblocked and tagged are all required"
        );
        test.expect_true(ports.events.empty(), "guarded requests call no entry port");
    }

    openswd3::app::BattleTransitionState tag_only{0x80000000U, 0, 0, 0};
    RecordingPorts tag_only_ports(tag_only, 1);
    test.expect_false(
        openswd3::app::consume_battle_request(tag_only, false, tag_only_ports),
        "zero low request does not enter battle"
    );
    test.expect_equal(tag_only.battle_request_value, 0U, "request tag is still cleared");
}

void test_entry_sequence(openswd3::test::Context& test) {
    openswd3::app::BattleTransitionState state{0x80012345U, 0, 0, 0};
    RecordingPorts ports(state, 1);
    test.expect_true(
        openswd3::app::consume_battle_request(state, false, ports),
        "valid tagged request enters battle"
    );
    const std::vector<Event> expected{
        {Call::release_entry_objects, 0, 0x00012345U, 0, 0, 0},
        {Call::close_world_map, 0, 0x00012345U, 0, 0, 0},
        {Call::initialize_battle, 0x2345U, 0x00012345U, 0, 0, 0},
        {Call::clear_party_bits, 0, 0x00012345U, 1, 0, 0},
    };
    test.expect_equal(ports.events, expected, "battle entry sequence and state timing");
}

void test_result_zero(openswd3::test::Context& test) {
    openswd3::app::BattleTransitionState state{9, 1, 7, 8};
    RecordingPorts ports(state, 0);
    test.expect_equal(
        openswd3::app::run_battle_frame(state, ports),
        0,
        "result zero is returned unchanged"
    );
    const std::vector<Event> expected{
        {Call::step_battle, 0, 9, 1, 7, 8},
        {Call::maintain_audio, 0, 9, 1, 7, 8},
        {Call::rebuild_display_zero, 0, 0, 0, 7, 8},
        {Call::set_world_state_zero, 0, 0, 0, 7, 8},
        {Call::reopen_world_map_zero, 0, 0, 0, 7, 8},
        {Call::resume_audio_zero, 0, 0, 0, 7, 8},
    };
    test.expect_equal(ports.events, expected, "result zero transition order");
}

void test_result_two(openswd3::test::Context& test) {
    openswd3::app::BattleTransitionState state{9, 1, 7, 8};
    RecordingPorts ports(state, 2);
    test.expect_equal(
        openswd3::app::run_battle_frame(state, ports),
        2,
        "result two is returned unchanged"
    );
    const std::vector<Event> expected{
        {Call::step_battle, 0, 9, 1, 7, 8},
        {Call::maintain_audio, 0, 9, 1, 7, 8},
        {Call::prepare_result_two, 0, 9, 1, 7, 8},
        {Call::clear_result_two_auxiliary, 0, 0, 0, 0x80000004U, 0},
        {Call::finish_result_two, 0, 0, 0, 0x80000004U, 0},
    };
    test.expect_equal(ports.events, expected, "result two transition order");
}

void test_result_three(openswd3::test::Context& test) {
    openswd3::app::BattleTransitionState state{9, 1, 7, 8};
    RecordingPorts ports(state, 3);
    test.expect_equal(
        openswd3::app::run_battle_frame(state, ports),
        3,
        "result three is returned unchanged"
    );
    const std::vector<Event> expected{
        {Call::step_battle, 0, 9, 1, 7, 8},
        {Call::maintain_audio, 0, 9, 1, 7, 8},
        {Call::clear_result_three, 0, 0, 0, 7, 8},
        {Call::remap_result_three, 0, 0, 0, 7, 8},
    };
    test.expect_equal(ports.events, expected, "result three transition order");
}

void test_other_result(openswd3::test::Context& test) {
    openswd3::app::BattleTransitionState state{9, 1, 7, 8};
    RecordingPorts ports(state, -1);
    test.expect_equal(
        openswd3::app::run_battle_frame(state, ports),
        -1,
        "other results are returned unchanged"
    );
    const std::vector<Event> expected{
        {Call::step_battle, 0, 9, 1, 7, 8},
        {Call::maintain_audio, 0, 9, 1, 7, 8},
    };
    test.expect_equal(ports.events, expected, "other results return after audio");
    test.expect_equal(state.battle_active, 1U, "other results preserve battle activity");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_entry_guards(test);
    test_entry_sequence(test);
    test_result_zero(test);
    test_result_two(test);
    test_result_three(test);
    test_other_result(test);
    return test.exit_code();
}
