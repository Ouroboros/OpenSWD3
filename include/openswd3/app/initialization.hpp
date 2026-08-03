#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::app {

inline constexpr compat::u32 kInitialSpecialModeState = 0x80000003U;

struct InitializationState {
    compat::u32 transition_suppression{};
    compat::u32 frame_counter{};
    compat::u32 special_mode_state{};
};

class InitializationPorts {
public:
    virtual ~InitializationPorts() = default;

    virtual void hide_cursor() = 0;
    virtual bool initialize_platform_backends() = 0;
    virtual void configure_input_and_audio_paths() = 0;
    virtual bool initialize_software_drawing() = 0;
    virtual void report_software_drawing_failure() = 0;
    virtual void check_legacy_memory_capacity() = 0;
    virtual void initialize_resource_database() = 0;
    virtual void initialize_render_resources() = 0;
    virtual bool initialize_frame_interval_35() = 0;
    virtual void report_frame_clock_failure() = 0;
    virtual void request_synchronous_destroy() = 0;
    virtual void initialize_story_world_and_asset_state() = 0;
};

[[nodiscard]] bool run_total_initialization(
    InitializationState& state,
    InitializationPorts& ports
);

[[nodiscard]] compat::i32 run_initialization_dialog_wrapper(
    InitializationState& state,
    InitializationPorts& ports
);

}  // namespace openswd3::app
