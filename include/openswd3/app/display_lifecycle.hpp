#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::app {

struct DisplayLifecycleState {
    compat::u32 display_active{};
    compat::u32 transition_suppression{};
    compat::u32 battle_active{};
};

class DisplayLifecyclePorts {
public:
    virtual ~DisplayLifecyclePorts() = default;

    [[nodiscard]] virtual bool display_backend_available() = 0;
    virtual void set_frame_interval(compat::u32 milliseconds) = 0;
    virtual void suspend_audio_output() = 0;
    virtual void suspend_audio_streams() = 0;
    virtual void maintain_audio() = 0;
    virtual void suspend_battle_display() = 0;
    virtual void release_font(compat::u32 point_size) = 0;
    virtual void minimize_window() = 0;

    virtual void show_and_position_window() = 0;
    virtual void restore_surfaces() = 0;
    virtual void rebuild_framebuffer_binding() = 0;
    virtual void rebuild_font(compat::u32 point_size) = 0;
    virtual void resume_battle_display() = 0;
    virtual void finish_display_recovery() = 0;
};

void deactivate_display(
    DisplayLifecycleState& state,
    DisplayLifecyclePorts& ports
);

void reactivate_display(
    DisplayLifecycleState& state,
    DisplayLifecyclePorts& ports
);

enum class ShutdownOperation {
    release_font_20,
    release_font_16,
    release_font_12,
    release_00406e00,
    release_0045ea30,
    release_00478110,
    drain_list_004a9a2c,
    release_0040f5e0,
    release_0040f500,
    release_0040f540,
    release_0040f570,
    release_0040dbc0,
    release_0040f5a0,
    release_0040f630,
    release_0040f670,
    drain_list_004ab2f4,
    drain_list_004b89f4,
    release_004020c0,
    release_0040f3b0,
    free_004a9a04,
    free_004a9a08,
    free_004a9a0c,
    release_0040f410,
    release_00433010,
    release_00431960,
    suspend_audio_output_00485710,
    suspend_audio_streams_00485740,
    free_004cae78,
    free_004cd764,
    release_common_source_surface,
    release_display_surfaces_00437a50,
    release_display_backend_00437ad0,
    release_input_backend_004374e0,
    free_004b7404,
    free_004b794c,
    free_004b7948,
    free_004c9a10,
    free_004b8860,
    free_004accd0,
    drain_list_004acac0,
    release_final_drawing_state_004258e0,
    show_cursor,
};

enum class ShutdownCloseOperation {
    role_handle,
    path_view,
    path_mapping,
    path_handle,
    talk_handle,
    pixel_view,
    pixel_mapping,
    pixel_handle,
};

class ShutdownPorts {
public:
    virtual ~ShutdownPorts() = default;

    virtual void perform_shutdown_operation(ShutdownOperation operation) = 0;
    [[nodiscard]] virtual bool perform_shutdown_close(
        ShutdownCloseOperation operation
    ) = 0;
    virtual void report_shutdown_close_failure(
        ShutdownCloseOperation operation
    ) = 0;
};

[[nodiscard]] compat::i32 run_total_shutdown(ShutdownPorts& ports);

}  // namespace openswd3::app
