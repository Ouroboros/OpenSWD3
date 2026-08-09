#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>

namespace openswd3::audio_video {

inline constexpr compat::u32 kLegacyMediaWaitFlag = 0x00000010U;
inline constexpr compat::u32 kLegacyCloseRequestFlag = 0x00000004U;

enum class LegacyMediaLocationStatus {
    unavailable,
    available,
};

struct LegacyMediaLocationResult {
    LegacyMediaLocationStatus status{LegacyMediaLocationStatus::unavailable};
    std::filesystem::path game_directory;
    std::filesystem::path marker_path;
    bool used_original_disc_layout{};
};

class LegacyMediaAcquisitionPorts {
public:
    virtual ~LegacyMediaAcquisitionPorts() = default;

    virtual void service_audio() = 0;
    [[nodiscard]] virtual bool file_exists(
        const std::filesystem::path& path
    ) = 0;
};

void begin_legacy_media_wait(compat::u32& process_flags) noexcept;
void complete_legacy_media_wait(compat::u32& process_flags) noexcept;
void cancel_legacy_media_wait(compat::u32& process_flags) noexcept;

[[nodiscard]] std::filesystem::path legacy_optical_media_marker_path(
    const std::filesystem::path& media_root
);

[[nodiscard]] LegacyMediaLocationResult resolve_configured_legacy_media(
    const std::filesystem::path& configured_data_directory,
    compat::u32& process_flags,
    LegacyMediaAcquisitionPorts& ports
);

}  // namespace openswd3::audio_video
