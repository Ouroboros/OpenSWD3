#include "openswd3/audio_video/legacy_media_acquisition.hpp"

namespace openswd3::audio_video {
namespace {

constexpr char kLegacyMediaMarkerFilename[] = "swd3_dvd.dat";
constexpr char kLegacyDiscGameDirectory[] = "swd3";

}  // namespace

void begin_legacy_media_wait(compat::u32& process_flags) noexcept {
    process_flags |= kLegacyMediaWaitFlag;
}

void complete_legacy_media_wait(compat::u32& process_flags) noexcept {
    process_flags &= ~kLegacyMediaWaitFlag;
}

void cancel_legacy_media_wait(compat::u32& process_flags) noexcept {
    // 0x00411C66 sets the close bit but does not clear the wait bit.
    process_flags |= kLegacyCloseRequestFlag;
}

std::filesystem::path
legacy_optical_media_marker_path(const std::filesystem::path& media_root) {
    return media_root / kLegacyDiscGameDirectory / kLegacyMediaMarkerFilename;
}

LegacyMediaLocationResult resolve_configured_legacy_media(
    const std::filesystem::path& configured_data_directory,
    compat::u32& process_flags,
    LegacyMediaAcquisitionPorts& ports
) {
    ports.service_audio();
    begin_legacy_media_wait(process_flags);

    const std::filesystem::path direct_marker =
        configured_data_directory / kLegacyMediaMarkerFilename;
    if (ports.file_exists(direct_marker)) {
        complete_legacy_media_wait(process_flags);
        return {
            LegacyMediaLocationStatus::available,
            configured_data_directory,
            direct_marker,
            false,
        };
    }

    const std::filesystem::path disc_marker =
        legacy_optical_media_marker_path(configured_data_directory);
    if (ports.file_exists(disc_marker)) {
        complete_legacy_media_wait(process_flags);
        return {
            LegacyMediaLocationStatus::available,
            configured_data_directory / kLegacyDiscGameDirectory,
            disc_marker,
            true,
        };
    }

    // The original blocks forever until media appears or input requests exit.
    // A configured cross-platform data directory has no optical-drive event
    // source, so return an explicit unavailable result and leave no stale wait.
    complete_legacy_media_wait(process_flags);
    return {};
}

}  // namespace openswd3::audio_video
