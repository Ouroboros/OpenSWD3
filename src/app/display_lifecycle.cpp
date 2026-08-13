#include "openswd3/app/display_lifecycle.hpp"

namespace openswd3::app {

namespace {

constexpr compat::u32 kLargeFontSize = 20U;
constexpr compat::u32 kMediumFontSize = 16U;
constexpr compat::u32 kSmallFontSize = 12U;
constexpr compat::u32 kRestoredFrameInterval = 35U;

}  // namespace

void deactivate_display(
    DisplayLifecycleState& state, DisplayLifecyclePorts& ports
) {
    if (!ports.display_backend_available()) {
        return;
    }

    ports.set_frame_interval(0U);
    ports.suspend_audio_output();
    ports.suspend_audio_streams();
    ports.maintain_audio();
    if (state.battle_active != 0U) {
        ports.suspend_battle_display();
    }
    ports.release_font(kLargeFontSize);
    ports.release_font(kMediumFontSize);
    ports.release_font(kSmallFontSize);
    state.display_active = 0U;
    state.transition_suppression = 1U;
    ports.minimize_window();
}

void reactivate_display(
    DisplayLifecycleState& state, DisplayLifecyclePorts& ports
) {
    if (!ports.display_backend_available()) {
        return;
    }

    ports.show_and_position_window();
    state.display_active = 1U;
    ports.restore_surfaces();
    ports.rebuild_framebuffer_binding();
    ports.rebuild_font(kLargeFontSize);
    ports.rebuild_font(kMediumFontSize);
    ports.rebuild_font(kSmallFontSize);
    state.transition_suppression = 0U;
    if (state.battle_active != 0U) {
        ports.resume_battle_display();
    }
    ports.finish_display_recovery();
    ports.set_frame_interval(kRestoredFrameInterval);
}

compat::i32 run_total_shutdown(ShutdownPorts& ports) {
    const auto run = [&ports](const ShutdownOperation operation) {
        ports.perform_shutdown_operation(operation);
    };
    const auto close = [&ports](const ShutdownCloseOperation operation) {
        if (!ports.perform_shutdown_close(operation)) {
            ports.report_shutdown_close_failure(operation);
        }
    };

    run(ShutdownOperation::release_font_20);
    run(ShutdownOperation::release_font_16);
    run(ShutdownOperation::release_font_12);
    run(ShutdownOperation::release_00406e00);
    run(ShutdownOperation::release_0045ea30);
    run(ShutdownOperation::release_00478110);
    run(ShutdownOperation::drain_list_004a9a2c);
    run(ShutdownOperation::release_0040f5e0);
    run(ShutdownOperation::release_0040f500);
    run(ShutdownOperation::release_0040f540);
    run(ShutdownOperation::release_0040f570);
    run(ShutdownOperation::release_0040dbc0);
    run(ShutdownOperation::release_0040f5a0);
    run(ShutdownOperation::release_0040f630);
    run(ShutdownOperation::release_0040f670);
    run(ShutdownOperation::drain_list_004ab2f4);
    run(ShutdownOperation::drain_list_004b89f4);
    run(ShutdownOperation::release_004020c0);
    run(ShutdownOperation::release_0040f3b0);
    run(ShutdownOperation::free_004a9a04);
    run(ShutdownOperation::free_004a9a08);
    run(ShutdownOperation::free_004a9a0c);
    run(ShutdownOperation::release_0040f410);
    run(ShutdownOperation::release_00433010);
    run(ShutdownOperation::release_00431960);
    run(ShutdownOperation::suspend_audio_output_00485710);
    run(ShutdownOperation::suspend_audio_streams_00485740);
    run(ShutdownOperation::free_004cae78);
    run(ShutdownOperation::free_004cd764);
    run(ShutdownOperation::release_common_source_surface);
    run(ShutdownOperation::release_display_surfaces_00437a50);
    run(ShutdownOperation::release_display_backend_00437ad0);
    run(ShutdownOperation::release_input_backend_004374e0);

    close(ShutdownCloseOperation::role_handle);
    close(ShutdownCloseOperation::path_view);
    close(ShutdownCloseOperation::path_mapping);
    close(ShutdownCloseOperation::path_handle);
    close(ShutdownCloseOperation::talk_handle);
    close(ShutdownCloseOperation::pixel_view);
    close(ShutdownCloseOperation::pixel_mapping);
    close(ShutdownCloseOperation::pixel_handle);

    run(ShutdownOperation::free_004b7404);
    run(ShutdownOperation::free_004b794c);
    run(ShutdownOperation::free_004b7948);
    run(ShutdownOperation::free_004c9a10);
    run(ShutdownOperation::free_004b8860);
    run(ShutdownOperation::free_004accd0);
    run(ShutdownOperation::drain_list_004acac0);
    run(ShutdownOperation::release_final_drawing_state_004258e0);
    run(ShutdownOperation::show_cursor);
    return 1;
}

}  // namespace openswd3::app
