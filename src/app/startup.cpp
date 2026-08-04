#include "openswd3/app/startup.hpp"

namespace openswd3::app {

compat::i32 run_startup_custom_message(
    StartupState& state,
    StartupPorts& ports
) {
    ports.play_startup_sound();
    ports.initialize_default_key_bindings();
    ports.initialize_paths_and_directories();
    state.any_save_exists = false;
    if (ports.scan_save_slots()) {
        state.any_save_exists = true;
    }

    const compat::i32 dialog_result = ports.show_startup_dialog();
    switch (dialog_result) {
        case 1:
            ports.initialize_game();
            ports.reset_result_one_game_state();
            ports.rebuild_result_one_slot_previews();
            ports.select_result_one_recent_save_group();
            break;
        case 2:
            ports.initialize_game();
            break;
        case 6:
            ports.request_synchronous_destroy();
            break;
        default:
            break;
    }
    return dialog_result;
}

}  // namespace openswd3::app
