#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::app {

struct StartupState {
    bool any_save_exists{};
};

class StartupPorts {
public:
    virtual ~StartupPorts() = default;

    virtual void play_startup_sound() = 0;
    virtual void initialize_default_key_bindings() = 0;
    virtual void initialize_paths_and_directories() = 0;
    virtual bool scan_save_slots() = 0;
    virtual compat::i32 show_startup_dialog() = 0;

    virtual void initialize_game() = 0;
    virtual void reset_result_one_game_state() = 0;
    virtual void rebuild_result_one_slot_previews() = 0;
    virtual void select_result_one_recent_save_group() = 0;
    virtual void request_synchronous_destroy() = 0;
};

[[nodiscard]] compat::i32 run_startup_custom_message(
    StartupState& state,
    StartupPorts& ports
);

}  // namespace openswd3::app
