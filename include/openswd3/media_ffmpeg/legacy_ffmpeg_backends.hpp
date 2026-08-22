#pragma once

#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/audio_video/legacy_video.hpp"

#include <filesystem>
#include <memory>
#include <string_view>

#if defined(_WIN32)
#if defined(OPENSWD3_MEDIA_FFMPEG_BUILD)
#define OPENSWD3_MEDIA_FFMPEG_API __declspec(dllexport)
#else
#define OPENSWD3_MEDIA_FFMPEG_API __declspec(dllimport)
#endif
#else
#define OPENSWD3_MEDIA_FFMPEG_API
#endif

namespace openswd3::media_ffmpeg {

[[nodiscard]] OPENSWD3_MEDIA_FFMPEG_API
    std::unique_ptr<audio_video::LegacyStreamBackend>
    make_legacy_stream_backend(std::filesystem::path data_directory);

[[nodiscard]] OPENSWD3_MEDIA_FFMPEG_API
    std::unique_ptr<audio_video::LegacyVideoBackend>
    make_legacy_video_backend();

[[nodiscard]] OPENSWD3_MEDIA_FFMPEG_API std::string_view
linked_ffmpeg_version() noexcept;

}  // namespace openswd3::media_ffmpeg

#undef OPENSWD3_MEDIA_FFMPEG_API
