#include "test.hpp"

#include "openswd3/app/frame_dispatch.hpp"
#include "openswd3/app/frame_runtime.hpp"

#include <vector>

namespace {

using openswd3::app::FrameCoordinatorState;
using openswd3::compat::i32;
using openswd3::compat::u16;

enum class Call {
    high_priority,
    battle_entry_release,
    battle_entry_close_map,
    battle_initialize,
    battle_clear_party,
    battle_step,
    audio,
    battle_zero_display,
    battle_zero_world_state,
    battle_zero_map,
    battle_zero_audio,
    battle_two_prepare,
    battle_two_clear,
    battle_two_finish,
    battle_three_clear,
    battle_three_remap,
    background_music,
    world_interaction,
    world_player,
    story,
    finish_world,
    prepare_special,
    standard_special,
    clear_play_time,
    sample_seconds,
    set_play_time_origin,
    set_menu_phase,
    clear_framebuffer,
    initialize_new_game,
    set_high_priority_submode,
    set_high_priority_auxiliary,
    reset_menu_previews,
    apply_name_overrides,
    load_fame,
    shop,
    close,
};

class RecordingPorts final : public openswd3::app::FrameRuntimePorts {
public:
    explicit RecordingPorts(const i32 battle_result = 1)
        : battle_result_(battle_result) {}

    void release_display_and_world_for_battle_entry() override {
        calls.push_back(Call::battle_entry_release);
    }
    void close_world_map_view() override {
        calls.push_back(Call::battle_entry_close_map);
    }
    void initialize_battle(const u16) override {
        calls.push_back(Call::battle_initialize);
    }
    void clear_party_battle_entry_bits() override {
        calls.push_back(Call::battle_clear_party);
    }
    i32 step_battle() override {
        calls.push_back(Call::battle_step);
        return battle_result_;
    }
    void maintain_audio() override {
        calls.push_back(Call::audio);
    }
    void rebuild_display_after_result_zero() override {
        calls.push_back(Call::battle_zero_display);
    }
    void set_result_zero_world_state() override {
        calls.push_back(Call::battle_zero_world_state);
    }
    void reopen_world_map_after_result_zero() override {
        calls.push_back(Call::battle_zero_map);
    }
    void resume_audio_after_result_zero() override {
        calls.push_back(Call::battle_zero_audio);
    }
    void prepare_result_two_internal_state() override {
        calls.push_back(Call::battle_two_prepare);
    }
    void clear_result_two_auxiliary_state() override {
        calls.push_back(Call::battle_two_clear);
    }
    void finish_result_two_mode_transition() override {
        calls.push_back(Call::battle_two_finish);
    }
    void clear_result_three_internal_state() override {
        calls.push_back(Call::battle_three_clear);
    }
    void remap_world_after_result_three() override {
        calls.push_back(Call::battle_three_remap);
    }
    void step_high_priority(FrameCoordinatorState& state) override {
        calls.push_back(Call::high_priority);
        state.process_flags |= flags_set_by_high_priority;
    }
    void update_background_music(FrameCoordinatorState&) override {
        calls.push_back(Call::background_music);
    }
    void step_world_interaction(FrameCoordinatorState&) override {
        calls.push_back(Call::world_interaction);
    }
    void step_world_player(FrameCoordinatorState& state) override {
        calls.push_back(Call::world_player);
        state.battle.special_mode_state = special_mode_set_by_world_player;
    }
    void step_story(FrameCoordinatorState& state) override {
        calls.push_back(Call::story);
        state.process_flags |= flags_set_by_story;
    }
    void finish_world_frame(FrameCoordinatorState&) override {
        calls.push_back(Call::finish_world);
    }
    void prepare_special_mode_objects(FrameCoordinatorState&) override {
        calls.push_back(Call::prepare_special);
    }
    openswd3::app::StandardSpecialModeEvent step_standard_special_mode(
        FrameCoordinatorState& state
    ) override {
        calls.push_back(Call::standard_special);
        state.process_flags |= flags_set_by_special;
        return special_mode_event;
    }
    void step_shop_mode(FrameCoordinatorState& state) override {
        calls.push_back(Call::shop);
        state.process_flags |= flags_set_by_special;
    }
    void request_synchronous_close() override {
        calls.push_back(Call::close);
    }

    void clear_accumulated_play_time() override {
        calls.push_back(Call::clear_play_time);
    }
    openswd3::compat::u32 sample_epoch_seconds() override {
        calls.push_back(Call::sample_seconds);
        return 123U;
    }
    void set_play_time_origin(openswd3::compat::u32) override {
        calls.push_back(Call::set_play_time_origin);
    }
    void set_initial_menu_phase(openswd3::compat::u32) override {
        calls.push_back(Call::set_menu_phase);
    }
    void clear_game_framebuffer() override {
        calls.push_back(Call::clear_framebuffer);
    }
    bool initialize_new_game_state_and_world() override {
        calls.push_back(Call::initialize_new_game);
        return true;
    }
    void set_high_priority_submode(openswd3::compat::u32) override {
        calls.push_back(Call::set_high_priority_submode);
    }
    void set_high_priority_auxiliary(openswd3::compat::u32) override {
        calls.push_back(Call::set_high_priority_auxiliary);
    }
    void reset_input_menu_and_save_previews() override {
        calls.push_back(Call::reset_menu_previews);
    }
    void apply_new_game_name_overrides() override {
        calls.push_back(Call::apply_name_overrides);
    }
    void load_fame_table() override {
        calls.push_back(Call::load_fame);
    }

    openswd3::compat::u32 flags_set_by_high_priority{};
    openswd3::compat::u32 flags_set_by_story{};
    openswd3::compat::u32 flags_set_by_special{};
    openswd3::compat::u32 special_mode_set_by_world_player{};
    openswd3::app::StandardSpecialModeEvent special_mode_event{
        openswd3::app::StandardSpecialModeEvent::none
    };
    std::vector<Call> calls;

private:
    i32 battle_result_{};
};

FrameCoordinatorState make_state() {
    return {1, 0, 0, false, {0, 0, 0, 0}};
}

void test_high_priority(openswd3::test::Context& test) {
    auto state = make_state();
    state.battle.high_priority_state = 1;
    RecordingPorts ports;
    ports.flags_set_by_high_priority = 0x04;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "high-priority path reaches common tail"
    );
    test.expect_equal(
        ports.calls,
        std::vector{Call::high_priority, Call::audio, Call::close},
        "high-priority path excludes battle/world and checks close after audio"
    );
}

void test_battle_early_return(openswd3::test::Context& test) {
    auto state = make_state();
    state.process_flags = 0x04;
    state.battle.battle_request_value = 0x80000005U;
    RecordingPorts ports;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, ports),
        openswd3::app::FrameRunOutcome::battle_early_return,
        "battle always uses its early-return path"
    );
    const std::vector<Call> expected{
        Call::battle_entry_release,
        Call::battle_entry_close_map,
        Call::battle_initialize,
        Call::battle_clear_party,
        Call::battle_step,
        Call::audio,
    };
    test.expect_equal(
        ports.calls,
        expected,
        "battle does not reach common close check even when close bit is set"
    );
}

void test_world(openswd3::test::Context& test) {
    auto state = make_state();
    RecordingPorts ports;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "world reaches common tail"
    );
    const std::vector<Call> expected{
        Call::background_music,
        Call::world_interaction,
        Call::world_player,
        Call::story,
        Call::finish_world,
        Call::audio,
    };
    test.expect_equal(ports.calls, expected, "ordinary world frame call order");
}

void test_world_gate_mutations(openswd3::test::Context& test) {
    auto state = make_state();
    RecordingPorts ports;
    ports.special_mode_set_by_world_player = 1;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "world gate mutation still reaches common tail"
    );
    test.expect_equal(
        ports.calls,
        std::vector{
            Call::background_music,
            Call::world_interaction,
            Call::world_player,
            Call::audio,
        },
        "world mutations are observed by the later story gate"
    );

    state = make_state();
    RecordingPorts closing_ports;
    closing_ports.flags_set_by_story = 0x04;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, closing_ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "story close request still reaches common tail"
    );
    test.expect_equal(
        closing_ports.calls,
        std::vector{
            Call::background_music,
            Call::world_interaction,
            Call::world_player,
            Call::story,
            Call::audio,
            Call::close,
        },
        "story close bit skips world finishing but reaches common close"
    );
}

void test_special_modes(openswd3::test::Context& test) {
    for (const auto& [mode, handler] : {
             std::pair{0x80000001U, Call::standard_special},
             std::pair{0x80000002U, Call::shop},
         }) {
        auto state = make_state();
        state.battle.special_mode_state = mode;
        RecordingPorts ports;
        ports.flags_set_by_special = 0x04;
        test.expect_equal(
            openswd3::app::run_accepted_frame(state, ports),
            openswd3::app::FrameRunOutcome::common_tail_completed,
            "special mode reaches common tail"
        );
        const std::vector<Call> expected{
            Call::background_music,
            Call::audio,
            Call::prepare_special,
            handler,
            Call::audio,
            Call::close,
        };
        test.expect_equal(
            ports.calls,
            expected,
            "special mode has pre-handler and common-tail audio maintenance"
        );
    }

    auto state = make_state();
    state.battle.special_mode_state = 7;
    RecordingPorts ports;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "unknown special mode reaches common tail"
    );
    test.expect_equal(
        ports.calls,
        std::vector{
            Call::background_music,
            Call::audio,
            Call::prepare_special,
            Call::audio,
        },
        "unknown low mode calls no handler but still reaches common tail"
    );

    state = make_state();
    state.battle.special_mode_state = 3U;
    RecordingPorts new_game_ports;
    new_game_ports.special_mode_event =
        openswd3::app::StandardSpecialModeEvent::
            commit_new_game_004492ba;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, new_game_ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "committed new game reaches the common frame tail"
    );
    test.expect_equal(
        new_game_ports.calls,
        std::vector{
            Call::background_music,
            Call::audio,
            Call::prepare_special,
            Call::standard_special,
            Call::clear_play_time,
            Call::sample_seconds,
            Call::set_play_time_origin,
            Call::set_menu_phase,
            Call::clear_framebuffer,
            Call::initialize_new_game,
            Call::set_high_priority_submode,
            Call::set_high_priority_auxiliary,
            Call::reset_menu_previews,
            Call::apply_name_overrides,
            Call::load_fame,
            Call::audio,
        },
        "special-mode event commits the transition before the common tail"
    );
    test.expect_equal(
        state.battle.special_mode_state,
        0U,
        "new-game commit releases ordinary world execution next frame"
    );

    state = make_state();
    state.battle.special_mode_state = 3U;
    RecordingPorts close_ports;
    close_ports.special_mode_event =
        openswd3::app::StandardSpecialModeEvent::request_close_00449320;
    test.expect_equal(
        openswd3::app::run_accepted_frame(state, close_ports),
        openswd3::app::FrameRunOutcome::common_tail_completed,
        "initial-menu close request reaches the common frame tail"
    );
    test.expect_equal(
        state.process_flags,
        openswd3::app::kProcessCloseRequested,
        "assembly case 3 sets process flag 0x04"
    );
    test.expect_equal(
        close_ports.calls,
        std::vector{
            Call::background_music,
            Call::audio,
            Call::prepare_special,
            Call::standard_special,
            Call::audio,
            Call::close,
        },
        "initial-menu close request is consumed synchronously in the same frame"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_high_priority(test);
    test_battle_early_return(test);
    test_world(test);
    test_world_gate_mutations(test);
    test_special_modes(test);
    return test.exit_code();
}
