#include "test.hpp"

#include "openswd3/app/startup.hpp"

#include <vector>

namespace {

enum class Call {
    startup_sound,
    float_conversion,
    paths,
    scan_saves,
    dialog,
    initialize_game,
    reset_result_one,
    rebuild_previews,
    select_recent_group,
    synchronous_destroy,
};

class RecordingPorts final : public openswd3::app::StartupPorts {
public:
    RecordingPorts(const bool saves_exist, const openswd3::compat::i32 dialog_result)
        : saves_exist_(saves_exist), dialog_result_(dialog_result) {}

    void play_startup_sound() override {
        calls.push_back(Call::startup_sound);
    }
    void initialize_float_conversion() override {
        calls.push_back(Call::float_conversion);
    }
    void initialize_paths_and_directories() override {
        calls.push_back(Call::paths);
    }
    bool scan_save_slots() override {
        calls.push_back(Call::scan_saves);
        return saves_exist_;
    }
    openswd3::compat::i32 show_startup_dialog() override {
        calls.push_back(Call::dialog);
        return dialog_result_;
    }
    void initialize_game() override {
        calls.push_back(Call::initialize_game);
    }
    void reset_result_one_game_state() override {
        calls.push_back(Call::reset_result_one);
    }
    void rebuild_result_one_slot_previews() override {
        calls.push_back(Call::rebuild_previews);
    }
    void select_result_one_recent_save_group() override {
        calls.push_back(Call::select_recent_group);
    }
    void request_synchronous_destroy() override {
        calls.push_back(Call::synchronous_destroy);
    }

    std::vector<Call> calls;

private:
    bool saves_exist_{};
    openswd3::compat::i32 dialog_result_{};
};

void run_case(
    openswd3::test::Context& test,
    const bool saves_exist,
    const openswd3::compat::i32 dialog_result,
    const std::vector<Call>& expected
) {
    openswd3::app::StartupState state{true};
    RecordingPorts ports(saves_exist, dialog_result);
    test.expect_equal(
        openswd3::app::run_startup_custom_message(state, ports),
        dialog_result,
        "dialog result is preserved as an unnamed numeric contract"
    );
    test.expect_equal(ports.calls, expected, "startup custom-message call order");
    test.expect_equal(
        state.any_save_exists,
        saves_exist,
        "save-exists state is cleared and replaced by the scan result"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    const std::vector<Call> prefix{
        Call::startup_sound,
        Call::float_conversion,
        Call::paths,
        Call::scan_saves,
        Call::dialog,
    };

    auto result_one = prefix;
    result_one.insert(
        result_one.end(),
        {
            Call::initialize_game,
            Call::reset_result_one,
            Call::rebuild_previews,
            Call::select_recent_group,
        }
    );
    run_case(test, true, 1, result_one);

    auto result_two = prefix;
    result_two.push_back(Call::initialize_game);
    run_case(test, false, 2, result_two);

    auto result_six = prefix;
    result_six.push_back(Call::synchronous_destroy);
    run_case(test, true, 6, result_six);

    run_case(test, false, 3, prefix);
    run_case(test, false, 5, prefix);
    return test.exit_code();
}
