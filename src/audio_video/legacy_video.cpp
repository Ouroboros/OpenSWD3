#include "openswd3/audio_video/legacy_video.hpp"

#include <bit>
#include <new>

namespace openswd3::audio_video {
namespace {

[[nodiscard]] constexpr compat::i32
from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32
wrapping_negate(const compat::u32 value) noexcept {
    return from_bits(0U - value);
}

[[nodiscard]] constexpr compat::i32
legacy_video_volume(const compat::i32 volume) noexcept {
    if (volume < 0) {
        return 0;
    }
    return volume > kLegacyVideoMaximumVolume ? kLegacyVideoMaximumVolume
                                              : volume;
}

}  // namespace

std::string legacy_bink_filename(const std::string_view scripted_filename) {
    std::string filename{scripted_filename};
    for (const std::string_view extension : {".avi", ".mpg"}) {
        const std::size_t position = filename.find(extension);
        if (position != std::string::npos) {
            filename.replace(position, extension.size(), ".bik");
        }
    }
    return filename;
}

std::filesystem::path build_legacy_video_path(
    const std::filesystem::path& configured_data_directory,
    const std::string_view scripted_filename
) {
    return configured_data_directory / "Video" /
        legacy_bink_filename(scripted_filename);
}

LegacyVideoPlayer::LegacyVideoPlayer(LegacyVideoBackend& backend) noexcept
    : backend_(backend) {}

LegacyVideoPlayer::~LegacyVideoPlayer() {
    static_cast<void>(close());
}

LegacyVideoBeginStatus LegacyVideoPlayer::begin(
    const std::string_view filename, const compat::i32 volume
) {
    if (active()) {
        return LegacyVideoBeginStatus::failed;
    }

    const LegacyVideoOpenResult result = backend_.open_video(filename);
    if (result.disposition == LegacyVideoOpenDisposition::immediate_complete) {
        summary_ = {};
        last_error_.clear();
        return LegacyVideoBeginStatus::completed;
    }
    if (result.disposition != LegacyVideoOpenDisposition::opened ||
        result.handle == 0U) {
        summary_ = {};
        try {
            last_error_.assign(backend_.last_error());
        } catch (const std::bad_alloc&) {
            last_error_ = "video open failed";
        }
        return LegacyVideoBeginStatus::failed;
    }

    handle_ = result.handle;
    summary_ = result.summary;
    last_error_.clear();
    backend_.set_video_volume(handle_, legacy_video_volume(volume));
    return LegacyVideoBeginStatus::playing;
}

bool LegacyVideoPlayer::close() {
    if (!active()) {
        return false;
    }
    backend_.close_video(handle_);
    handle_ = 0U;
    summary_ = {};
    return true;
}

LegacyVideoStepResult LegacyVideoPlayer::step(LegacyVideoFramePorts& ports) {
    LegacyVideoStepResult result;
    if (!active()) {
        return result;
    }

    if (backend_.wait_for_video_frame(handle_)) {
        result.status = LegacyVideoStepStatus::waiting;
        return result;
    }

    const LegacyVideoDecodeStatus decode_status =
        backend_.decode_video_frame(handle_);
    if (decode_status == LegacyVideoDecodeStatus::completed) {
        backend_.service_video(handle_);
        static_cast<void>(close());
        result.status = LegacyVideoStepStatus::completed;
        return result;
    }
    if (decode_status == LegacyVideoDecodeStatus::failed) {
        try {
            last_error_.assign(backend_.last_error());
        } catch (const std::bad_alloc&) {
            last_error_ = "video decode failed";
        }
        static_cast<void>(close());
        result.status = LegacyVideoStepStatus::failed;
        return result;
    }

    const compat::i32 destination_x =
        (kLegacyVideoCanvasWidth - summary_.width) / 2;
    const compat::i32 destination_y =
        (kLegacyVideoCanvasHeight - summary_.height) / 2;

    result.copy_result = backend_.copy_video_frame(
        handle_,
        LegacyVideoCopyRequest{
            .destination = ports.video_destination_pixels(),
            .pitch_bytes = ports.video_destination_pitch_bytes(),
            .destination_height = kLegacyVideoCanvasHeight,
            .destination_x = destination_x,
            .destination_y = destination_y,
            .pixel_format = ports.video_pixel_format(),
        }
    );
    if (result.copy_result == 0) {
        ports.report_video_copy_failure();
    }

    const compat::u32 frame_count = backend_.video_frame_count(handle_);
    const compat::u32 frame_number = backend_.video_frame_number(handle_);
    result.legacy_progress = wrapping_negate(frame_count);
    if (frame_count > frame_number) {
        result.legacy_progress = from_bits(frame_count);
        backend_.advance_video_frame(handle_);
    }
    backend_.service_video(handle_);

    result.presentation_succeeded = ports.present_video_frame();
    if (result.legacy_progress > 0) {
        result.status = LegacyVideoStepStatus::frame_presented;
        return result;
    }

    static_cast<void>(close());
    result.status = LegacyVideoStepStatus::completed;
    return result;
}

bool LegacyVideoPlayer::active() const noexcept {
    return handle_ != 0U;
}

LegacyVideoSummary LegacyVideoPlayer::summary() const noexcept {
    return summary_;
}

std::string_view LegacyVideoPlayer::last_error() const noexcept {
    return last_error_;
}

compat::i32 LegacyVideoPlayer::legacy_progress() {
    if (!active()) {
        return -1;
    }
    const compat::u32 frame_number = backend_.video_frame_number(handle_);
    const compat::u32 frame_count = backend_.video_frame_count(handle_);
    return frame_number <= frame_count ? from_bits(frame_number)
                                       : wrapping_negate(frame_number);
}

LegacyVideoOpenResult
ImmediateCompleteLegacyVideoBackend::open_video(std::string_view) {
    return {
        .disposition = LegacyVideoOpenDisposition::immediate_complete,
    };
}

std::string_view ImmediateCompleteLegacyVideoBackend::last_error() const {
    return {};
}

void ImmediateCompleteLegacyVideoBackend::close_video(LegacyVideoHandle) {}

void ImmediateCompleteLegacyVideoBackend::set_video_volume(
    LegacyVideoHandle, compat::i32
) {}

bool ImmediateCompleteLegacyVideoBackend::wait_for_video_frame(
    LegacyVideoHandle
) {
    return false;
}

LegacyVideoDecodeStatus
ImmediateCompleteLegacyVideoBackend::decode_video_frame(LegacyVideoHandle) {
    return LegacyVideoDecodeStatus::completed;
}

compat::i32 ImmediateCompleteLegacyVideoBackend::copy_video_frame(
    LegacyVideoHandle, const LegacyVideoCopyRequest&
) {
    return 0;
}

compat::u32
ImmediateCompleteLegacyVideoBackend::video_frame_count(LegacyVideoHandle) {
    return 0U;
}

compat::u32
ImmediateCompleteLegacyVideoBackend::video_frame_number(LegacyVideoHandle) {
    return 0U;
}

void ImmediateCompleteLegacyVideoBackend::advance_video_frame(
    LegacyVideoHandle
) {}

void ImmediateCompleteLegacyVideoBackend::service_video(LegacyVideoHandle) {}

}  // namespace openswd3::audio_video
