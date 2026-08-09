#pragma once

#include "openswd3/compat/types.hpp"

#include <cstdint>
#include <string_view>

namespace openswd3::app {

using BackendToken = std::uintptr_t;

inline constexpr compat::u32 kInputInitializationFailureBit = 0x02U;
inline constexpr compat::u32 kDisplayInitializationFailureBit = 0x04U;
inline constexpr compat::u32 kLegacyDisplayMode = 0x4E22U;
inline constexpr compat::u32 kLegacyPrimarySurfaceSelector = 0x2711U;
inline constexpr compat::u32 kLegacyFrameWidth = 640U;
inline constexpr compat::u32 kLegacyFrameHeight = 480U;
inline constexpr compat::u32 kLegacyFrameDepthBits = 16U;

struct PlatformBackendState {
    compat::u32 input_backend_flags{};
    compat::u32 process_flags{};
    compat::u32 display_active{};
    BackendToken common_source_surface{};
};

struct DisplayInitializationRequest {
    compat::u32 mode{};
    compat::u32 width{};
    compat::u32 height{};
    compat::u32 depth_bits{};

    friend bool operator==(
        const DisplayInitializationRequest&,
        const DisplayInitializationRequest&
    ) = default;
};

class PlatformBackendInitializationPorts {
public:
    virtual ~PlatformBackendInitializationPorts() = default;

    // Normalized boundary: true corresponds to 0x00436FB0 returning zero.
    [[nodiscard]] virtual bool initialize_input_backend() = 0;
    virtual void report_input_initialization_failure() = 0;
    virtual void request_synchronous_destroy() = 0;

    virtual void start_audio_runtime(std::string_view legacy_base_path) = 0;
    virtual void initialize_audio_output(std::string_view legacy_base_path) = 0;
    [[nodiscard]] virtual BackendToken query_audio_driver() = 0;
    virtual void initialize_midi_output(BackendToken driver) = 0;
    virtual void initialize_audio_stream_nodes(BackendToken driver) = 0;

    [[nodiscard]] virtual bool initialize_display_backend(
        DisplayInitializationRequest request
    ) = 0;
    virtual void report_display_initialization_failure() = 0;
    [[nodiscard]] virtual BackendToken create_common_source_surface(
        compat::u32 width,
        compat::u32 height
    ) = 0;
    [[nodiscard]] virtual BackendToken query_display_surface(
        compat::u32 selector
    ) = 0;
    virtual void configure_video_audio(
        BackendToken display_surface,
        compat::u8 use_direct_sound,
        BackendToken audio_driver
    ) = 0;
};

[[nodiscard]] bool run_platform_backend_initialization(
    std::string_view legacy_base_path,
    PlatformBackendState& state,
    PlatformBackendInitializationPorts& ports
);

}  // namespace openswd3::app
