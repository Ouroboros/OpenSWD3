#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace openswd3::audio_video {

inline constexpr compat::i32 kLegacyVideoCanvasWidth = 640;
inline constexpr compat::i32 kLegacyVideoCanvasHeight = 480;
inline constexpr compat::i32 kLegacyVideoMaximumVolume = 0x8000;

using LegacyVideoHandle = compat::u32;

enum class LegacyVideoPixelFormat : compat::u8 {
    rgb555,
    rgb565,
};

struct LegacyVideoSummary {
    compat::i32 width{};
    compat::i32 height{};

    bool operator==(const LegacyVideoSummary&) const = default;
};

enum class LegacyVideoOpenDisposition : compat::u8 {
    opened,
    immediate_complete,
    failed,
};

struct LegacyVideoOpenResult {
    LegacyVideoOpenDisposition disposition{LegacyVideoOpenDisposition::failed};
    LegacyVideoHandle handle{};
    LegacyVideoSummary summary{};
};

struct LegacyVideoCopyRequest {
    std::span<compat::u16> destination;
    compat::i32 pitch_bytes{};
    compat::i32 destination_height{kLegacyVideoCanvasHeight};
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    LegacyVideoPixelFormat pixel_format{LegacyVideoPixelFormat::rgb565};
};

[[nodiscard]] std::string
legacy_bink_filename(std::string_view scripted_filename);

[[nodiscard]] std::filesystem::path build_legacy_video_path(
    const std::filesystem::path& configured_data_directory,
    std::string_view scripted_filename
);

class LegacyVideoBackend {
public:
    virtual ~LegacyVideoBackend() = default;

    [[nodiscard]] virtual LegacyVideoOpenResult
    open_video(std::string_view filename) = 0;
    [[nodiscard]] virtual std::string_view last_error() const = 0;
    virtual void close_video(LegacyVideoHandle handle) = 0;
    virtual void
    set_video_volume(LegacyVideoHandle handle, compat::i32 volume) = 0;

    [[nodiscard]] virtual bool
    wait_for_video_frame(LegacyVideoHandle handle) = 0;
    virtual void decode_video_frame(LegacyVideoHandle handle) = 0;
    [[nodiscard]] virtual compat::i32 copy_video_frame(
        LegacyVideoHandle handle, const LegacyVideoCopyRequest& request
    ) = 0;
    [[nodiscard]] virtual compat::u32
    video_frame_count(LegacyVideoHandle handle) = 0;
    [[nodiscard]] virtual compat::u32
    video_frame_number(LegacyVideoHandle handle) = 0;
    virtual void advance_video_frame(LegacyVideoHandle handle) = 0;
    virtual void service_video(LegacyVideoHandle handle) = 0;
};

class LegacyVideoFramePorts {
public:
    virtual ~LegacyVideoFramePorts() = default;

    [[nodiscard]] virtual std::span<compat::u16> video_destination_pixels() = 0;
    [[nodiscard]] virtual compat::i32 video_destination_pitch_bytes() = 0;
    [[nodiscard]] virtual LegacyVideoPixelFormat video_pixel_format() = 0;
    virtual void report_video_copy_failure() = 0;
    [[nodiscard]] virtual bool present_video_frame() = 0;
};

enum class LegacyVideoBeginStatus : compat::u8 {
    playing,
    completed,
    failed,
};

enum class LegacyVideoStepStatus : compat::u8 {
    inactive,
    waiting,
    frame_presented,
    completed,
};

struct LegacyVideoStepResult {
    LegacyVideoStepStatus status{LegacyVideoStepStatus::inactive};
    compat::i32 legacy_progress{-1};
    compat::i32 copy_result{};
    bool presentation_succeeded{};
};

class LegacyVideoPlayer final {
public:
    explicit LegacyVideoPlayer(LegacyVideoBackend& backend) noexcept;
    ~LegacyVideoPlayer();

    LegacyVideoPlayer(const LegacyVideoPlayer&) = delete;
    LegacyVideoPlayer& operator=(const LegacyVideoPlayer&) = delete;
    LegacyVideoPlayer(LegacyVideoPlayer&&) = delete;
    LegacyVideoPlayer& operator=(LegacyVideoPlayer&&) = delete;

    [[nodiscard]] LegacyVideoBeginStatus
    begin(std::string_view filename, compat::i32 volume);
    [[nodiscard]] bool close();
    [[nodiscard]] LegacyVideoStepResult step(LegacyVideoFramePorts& ports);

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] LegacyVideoSummary summary() const noexcept;
    [[nodiscard]] std::string_view last_error() const noexcept;
    [[nodiscard]] compat::i32 legacy_progress();

private:
    LegacyVideoBackend& backend_;
    LegacyVideoHandle handle_{};
    LegacyVideoSummary summary_{};
    std::string last_error_;
};

class ImmediateCompleteLegacyVideoBackend final : public LegacyVideoBackend {
public:
    [[nodiscard]] LegacyVideoOpenResult
    open_video(std::string_view filename) override;
    [[nodiscard]] std::string_view last_error() const override;
    void close_video(LegacyVideoHandle handle) override;
    void
    set_video_volume(LegacyVideoHandle handle, compat::i32 volume) override;
    [[nodiscard]] bool wait_for_video_frame(LegacyVideoHandle handle) override;
    void decode_video_frame(LegacyVideoHandle handle) override;
    [[nodiscard]] compat::i32 copy_video_frame(
        LegacyVideoHandle handle, const LegacyVideoCopyRequest& request
    ) override;
    [[nodiscard]] compat::u32
    video_frame_count(LegacyVideoHandle handle) override;
    [[nodiscard]] compat::u32
    video_frame_number(LegacyVideoHandle handle) override;
    void advance_video_frame(LegacyVideoHandle handle) override;
    void service_video(LegacyVideoHandle handle) override;
};

}  // namespace openswd3::audio_video
