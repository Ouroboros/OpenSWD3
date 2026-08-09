#include "openswd3/app/platform_backend_initialization.hpp"

namespace openswd3::app {

bool run_platform_backend_initialization(
    const std::string_view legacy_base_path,
    PlatformBackendState& state,
    PlatformBackendInitializationPorts& ports
) {
    state.input_backend_flags = 0U;
    if (!ports.initialize_input_backend()) {
        state.input_backend_flags |= kInputInitializationFailureBit;
        ports.report_input_initialization_failure();
        ports.request_synchronous_destroy();
        return false;
    }

    ports.start_audio_runtime(legacy_base_path);
    ports.initialize_audio_output(legacy_base_path);
    ports.initialize_midi_output(ports.query_audio_driver());
    ports.initialize_audio_stream_nodes(ports.query_audio_driver());

    const DisplayInitializationRequest request{
        kLegacyDisplayMode,
        kLegacyFrameWidth,
        kLegacyFrameHeight,
        kLegacyFrameDepthBits,
    };
    if (!ports.initialize_display_backend(request)) {
        state.process_flags |= kDisplayInitializationFailureBit;
        ports.report_display_initialization_failure();
        ports.request_synchronous_destroy();
        return false;
    }

    state.common_source_surface = ports.create_common_source_surface(
        kLegacyFrameWidth,
        kLegacyFrameHeight
    );
    state.display_active = 1U;
    const BackendToken audio_driver = ports.query_audio_driver();
    const BackendToken display_surface =
        ports.query_display_surface(kLegacyPrimarySurfaceSelector);
    ports.configure_video_audio(display_surface, 0U, audio_driver);
    return true;
}

}  // namespace openswd3::app
