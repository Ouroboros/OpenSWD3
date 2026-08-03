#include "openswd3/app/initialization.hpp"

namespace openswd3::app {

bool run_total_initialization(
    InitializationState& state,
    InitializationPorts& ports
) {
    state.transition_suppression = 0U;
    state.frame_counter = 0U;
    ports.hide_cursor();

    // 0x00424B90 does not test the return from 0x00424EF0.
    static_cast<void>(ports.initialize_platform_backends());
    ports.configure_input_and_audio_paths();

    if (!ports.initialize_software_drawing()) {
        ports.report_software_drawing_failure();
        ports.request_synchronous_destroy();
        return false;
    }

    ports.check_legacy_memory_capacity();
    ports.initialize_resource_database();
    ports.initialize_render_resources();

    if (!ports.initialize_frame_interval_35()) {
        ports.report_frame_clock_failure();
        state.transition_suppression = 1U;
        ports.request_synchronous_destroy();
        return false;
    }

    ports.initialize_story_world_and_asset_state();
    state.special_mode_state = kInitialSpecialModeState;
    return true;
}

compat::i32 run_initialization_dialog_wrapper(
    InitializationState& state,
    InitializationPorts& ports
) {
    static_cast<void>(run_total_initialization(state, ports));
    return 1;
}

}  // namespace openswd3::app
