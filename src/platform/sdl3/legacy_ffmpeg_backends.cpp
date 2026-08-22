#include "openswd3/media_ffmpeg/legacy_ffmpeg_backends.hpp"

#include "openswd3/compat/types.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openswd3::media_ffmpeg {
namespace {

using audio_video::LegacyStreamHandle;
using audio_video::LegacyVideoHandle;
using compat::i32;
using compat::u32;

constexpr int kOutputSampleRate = 48'000;
constexpr int kOutputChannelCount = 2;
constexpr int kBytesPerOutputFrame =
    static_cast<int>(sizeof(float)) * kOutputChannelCount;

struct FormatInputDeleter {
    void operator()(AVFormatContext* context) const noexcept {
        avformat_close_input(&context);
    }
};

struct CodecContextDeleter {
    void operator()(AVCodecContext* context) const noexcept {
        avcodec_free_context(&context);
    }
};

struct PacketDeleter {
    void operator()(AVPacket* packet) const noexcept {
        av_packet_free(&packet);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* frame) const noexcept {
        av_frame_free(&frame);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* context) const noexcept {
        swr_free(&context);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* context) const noexcept {
        sws_freeContext(context);
    }
};

struct SdlAudioStreamDeleter {
    void operator()(SDL_AudioStream* stream) const noexcept {
        SDL_DestroyAudioStream(stream);
    }
};

using FormatInputPtr = std::unique_ptr<AVFormatContext, FormatInputDeleter>;
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using SdlAudioStreamPtr =
    std::unique_ptr<SDL_AudioStream, SdlAudioStreamDeleter>;

[[nodiscard]] std::string ffmpeg_error(const int code) {
    std::array<char, AV_ERROR_MAX_STRING_SIZE> buffer{};
    if (av_strerror(code, buffer.data(), buffer.size()) < 0) {
        return "unknown FFmpeg error";
    }
    return buffer.data();
}

[[nodiscard]] std::string normalized_media_filename(
    const std::filesystem::path& data_directory, const std::string_view filename
) {
    std::string normalized{filename};
    std::ranges::replace(normalized, '\\', '/');
    std::filesystem::path path{normalized};
    if (path.is_relative()) {
        path = data_directory / path;
    }
    return path.lexically_normal().string();
}

[[nodiscard]] FormatInputPtr
open_input(const std::string& filename, std::string& error) {
    AVFormatContext* raw_context{};
    const int open_result =
        avformat_open_input(&raw_context, filename.c_str(), nullptr, nullptr);
    if (open_result < 0) {
        error = "FFmpeg open failed: " + ffmpeg_error(open_result);
        return {};
    }
    FormatInputPtr context{raw_context};
    const int information_result =
        avformat_find_stream_info(context.get(), nullptr);
    if (information_result < 0) {
        error = "FFmpeg stream information failed: " +
            ffmpeg_error(information_result);
        return {};
    }
    return context;
}

[[nodiscard]] CodecContextPtr open_decoder(
    AVFormatContext& format,
    const AVMediaType media_type,
    int& stream_index,
    std::string& error
) {
    const AVCodec* decoder{};
    stream_index =
        av_find_best_stream(&format, media_type, -1, -1, &decoder, 0);
    if (stream_index < 0 || decoder == nullptr) {
        error = "FFmpeg media stream not found: " + ffmpeg_error(stream_index);
        return {};
    }

    CodecContextPtr context{avcodec_alloc_context3(decoder)};
    if (!context) {
        error = "FFmpeg decoder allocation failed";
        return {};
    }
    const int parameter_result = avcodec_parameters_to_context(
        context.get(), format.streams[stream_index]->codecpar
    );
    if (parameter_result < 0) {
        error = "FFmpeg decoder parameters failed: " +
            ffmpeg_error(parameter_result);
        return {};
    }
    const int open_result = avcodec_open2(context.get(), decoder, nullptr);
    if (open_result < 0) {
        error = "FFmpeg decoder open failed: " + ffmpeg_error(open_result);
        return {};
    }
    return context;
}

[[nodiscard]] SwrContextPtr
make_audio_resampler(const AVCodecContext& decoder, std::string& error) {
    SwrContext* raw_resampler{};
    AVChannelLayout output_layout = AV_CHANNEL_LAYOUT_STEREO;
    const int allocation_result = swr_alloc_set_opts2(
        &raw_resampler,
        &output_layout,
        AV_SAMPLE_FMT_FLT,
        kOutputSampleRate,
        &decoder.ch_layout,
        decoder.sample_fmt,
        decoder.sample_rate,
        0,
        nullptr
    );
    if (allocation_result < 0 || raw_resampler == nullptr) {
        error = "FFmpeg resampler allocation failed: " +
            ffmpeg_error(allocation_result);
        return {};
    }
    SwrContextPtr resampler{raw_resampler};
    const int initialize_result = swr_init(resampler.get());
    if (initialize_result < 0) {
        error = "FFmpeg resampler initialization failed: " +
            ffmpeg_error(initialize_result);
        return {};
    }
    return resampler;
}

[[nodiscard]] bool append_resampled_frame(
    SwrContext& resampler,
    const AVCodecContext& decoder,
    const AVFrame& frame,
    std::vector<float>& destination,
    std::string& error
) {
    const std::int64_t delayed_samples =
        swr_get_delay(&resampler, decoder.sample_rate);
    const std::int64_t maximum_samples_64 = av_rescale_rnd(
        delayed_samples + frame.nb_samples,
        kOutputSampleRate,
        decoder.sample_rate,
        AV_ROUND_UP
    );
    if (maximum_samples_64 <= 0 ||
        maximum_samples_64 > std::numeric_limits<int>::max()) {
        error = "FFmpeg resampler output size is invalid";
        return false;
    }
    const int maximum_samples = static_cast<int>(maximum_samples_64);
    std::vector<float> converted(
        static_cast<std::size_t>(maximum_samples) * kOutputChannelCount
    );
    std::array<std::uint8_t*, 1U> output{
        reinterpret_cast<std::uint8_t*>(converted.data()),
    };
    const int converted_samples = swr_convert(
        &resampler,
        output.data(),
        maximum_samples,
        const_cast<const std::uint8_t**>(frame.extended_data),
        frame.nb_samples
    );
    if (converted_samples < 0) {
        error = "FFmpeg audio conversion failed: " +
            ffmpeg_error(converted_samples);
        return false;
    }
    converted.resize(
        static_cast<std::size_t>(converted_samples) * kOutputChannelCount
    );
    destination.insert(destination.end(), converted.begin(), converted.end());
    return true;
}

[[nodiscard]] bool receive_audio_frames(
    AVCodecContext& decoder,
    SwrContext& resampler,
    AVFrame& frame,
    std::vector<float>& destination,
    std::string& error
) {
    while (true) {
        const int receive_result = avcodec_receive_frame(&decoder, &frame);
        if (receive_result == AVERROR(EAGAIN) ||
            receive_result == AVERROR_EOF) {
            return true;
        }
        if (receive_result < 0) {
            error =
                "FFmpeg audio decode failed: " + ffmpeg_error(receive_result);
            return false;
        }
        if (!append_resampled_frame(
                resampler, decoder, frame, destination, error
            )) {
            return false;
        }
        av_frame_unref(&frame);
    }
}

[[nodiscard]] bool decode_audio_file(
    const std::string& filename,
    std::vector<float>& samples,
    i32& total_milliseconds,
    std::string& error
) {
    FormatInputPtr format = open_input(filename, error);
    if (!format) {
        return false;
    }
    int stream_index{};
    CodecContextPtr decoder =
        open_decoder(*format, AVMEDIA_TYPE_AUDIO, stream_index, error);
    if (!decoder) {
        return false;
    }
    SwrContextPtr resampler = make_audio_resampler(*decoder, error);
    PacketPtr packet{av_packet_alloc()};
    FramePtr frame{av_frame_alloc()};
    if (!resampler || !packet || !frame) {
        error = "FFmpeg audio working-set allocation failed";
        return false;
    }

    while (av_read_frame(format.get(), packet.get()) >= 0) {
        if (packet->stream_index == stream_index) {
            const int send_result =
                avcodec_send_packet(decoder.get(), packet.get());
            if (send_result < 0 && send_result != AVERROR(EAGAIN)) {
                error = "FFmpeg audio packet submit failed: " +
                    ffmpeg_error(send_result);
                return false;
            }
            if (!receive_audio_frames(
                    *decoder, *resampler, *frame, samples, error
                )) {
                return false;
            }
        }
        av_packet_unref(packet.get());
    }

    const int flush_result = avcodec_send_packet(decoder.get(), nullptr);
    if (flush_result < 0 && flush_result != AVERROR_EOF) {
        error = "FFmpeg audio flush failed: " + ffmpeg_error(flush_result);
        return false;
    }
    if (!receive_audio_frames(*decoder, *resampler, *frame, samples, error)) {
        return false;
    }
    if (samples.empty()) {
        error = "FFmpeg audio stream decoded no samples";
        return false;
    }

    const std::uint64_t frame_count =
        samples.size() / static_cast<std::size_t>(kOutputChannelCount);
    const std::uint64_t milliseconds =
        frame_count * 1000U / static_cast<std::uint64_t>(kOutputSampleRate);
    total_milliseconds = static_cast<i32>(std::min<std::uint64_t>(
        milliseconds,
        static_cast<std::uint64_t>(std::numeric_limits<i32>::max())
    ));
    return true;
}

[[nodiscard]] SdlAudioStreamPtr open_audio_output(std::string& error) {
    if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0U &&
        !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        error =
            std::string{"SDL audio initialization failed: "} + SDL_GetError();
        return {};
    }
    const SDL_AudioSpec specification{
        .format = SDL_AUDIO_F32,
        .channels = kOutputChannelCount,
        .freq = kOutputSampleRate,
    };
    SdlAudioStreamPtr stream{SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification, nullptr, nullptr
    )};
    if (!stream) {
        error = std::string{"SDL audio stream open failed: "} + SDL_GetError();
    }
    return stream;
}

[[nodiscard]] bool queue_samples(
    SDL_AudioStream& stream,
    const std::vector<float>& samples,
    std::string& error
) {
    const std::size_t byte_count = samples.size() * sizeof(float);
    if (byte_count >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        error = "decoded audio exceeds SDL queue limit";
        return false;
    }
    if (!SDL_PutAudioStreamData(
            &stream, samples.data(), static_cast<int>(byte_count)
        )) {
        error = std::string{"SDL audio queue failed: "} + SDL_GetError();
        return false;
    }
    return true;
}

[[nodiscard]] float legacy_gain(const i32 volume) noexcept {
    constexpr float maximum = 127.0F;
    return std::clamp(static_cast<float>(volume) / maximum, 0.0F, 1.0F);
}

class FfmpegStreamBackend final : public audio_video::LegacyStreamBackend {
public:
    explicit FfmpegStreamBackend(std::filesystem::path data_directory)
        : data_directory_(std::move(data_directory)) {}

    LegacyStreamHandle
    open_stream(u32, const std::string_view filename, i32) override {
        auto state = std::make_unique<StreamState>();
        const std::string path =
            normalized_media_filename(data_directory_, filename);
        if (!decode_audio_file(
                path, state->samples, state->total_milliseconds, last_error_
            )) {
            return 0U;
        }
        state->output = open_audio_output(last_error_);
        if (!state->output) {
            return 0U;
        }
        const LegacyStreamHandle handle = next_handle_++;
        streams_.emplace(handle, std::move(state));
        last_error_.clear();
        return handle;
    }

    std::string_view last_error() const override {
        return last_error_;
    }

    void close_stream(const LegacyStreamHandle handle) override {
        streams_.erase(handle);
    }

    void set_stream_user_data(
        const LegacyStreamHandle handle, const u32 slot, const i32 value
    ) override {
        if (StreamState* state = find(handle);
            state != nullptr && slot < state->user_data.size()) {
            state->user_data[slot] = value;
        }
    }

    i32
    stream_user_data(const LegacyStreamHandle handle, const u32 slot) override {
        const StreamState* state = find(handle);
        return state != nullptr && slot < state->user_data.size()
            ? state->user_data[slot]
            : 0;
    }

    void set_stream_volume(
        const LegacyStreamHandle handle, const i32 volume
    ) override {
        if (StreamState* state = find(handle); state != nullptr) {
            state->volume = volume;
            static_cast<void>(
                SDL_SetAudioStreamGain(state->output.get(), legacy_gain(volume))
            );
        }
    }

    i32 stream_volume(const LegacyStreamHandle handle) override {
        const StreamState* state = find(handle);
        return state == nullptr ? 0 : state->volume;
    }

    void set_stream_loop_count(
        const LegacyStreamHandle handle, const i32 loop_count
    ) override {
        if (StreamState* state = find(handle); state != nullptr) {
            state->loop_count = loop_count;
        }
    }

    void start_stream(const LegacyStreamHandle handle) override {
        StreamState* state = find(handle);
        if (state == nullptr) {
            return;
        }
        static_cast<void>(SDL_ClearAudioStream(state->output.get()));
        if (!queue_samples(*state->output, state->samples, last_error_)) {
            state->completed = true;
            return;
        }
        state->remaining_replays =
            state->loop_count > 1 ? state->loop_count - 1 : 0;
        state->infinite_loop = state->loop_count == 0;
        state->started = true;
        state->completed = false;
        static_cast<void>(SDL_ResumeAudioStreamDevice(state->output.get()));
    }

    u32 stream_status(const LegacyStreamHandle handle) override {
        StreamState* state = find(handle);
        if (state == nullptr || state->completed) {
            return 2U;
        }
        if (!state->started) {
            return 8U;
        }
        const int queued = SDL_GetAudioStreamQueued(state->output.get());
        if (queued > 0) {
            return 4U;
        }
        if (state->infinite_loop || state->remaining_replays > 0) {
            if (!state->infinite_loop) {
                --state->remaining_replays;
            }
            if (queue_samples(*state->output, state->samples, last_error_)) {
                return 4U;
            }
        }
        state->completed = true;
        return 2U;
    }

    void stream_ms_position(
        const LegacyStreamHandle handle,
        i32& total_milliseconds,
        i32& current_milliseconds
    ) override {
        const StreamState* state = find(handle);
        if (state == nullptr) {
            total_milliseconds = 0;
            current_milliseconds = 0;
            return;
        }
        total_milliseconds = state->total_milliseconds;
        const int queued = SDL_GetAudioStreamQueued(state->output.get());
        const std::size_t total_bytes = state->samples.size() * sizeof(float);
        const std::size_t queued_bytes = queued > 0
            ? std::min(total_bytes, static_cast<std::size_t>(queued))
            : 0U;
        const std::size_t consumed_bytes = total_bytes - queued_bytes;
        const std::uint64_t milliseconds =
            static_cast<std::uint64_t>(consumed_bytes) * 1000U /
            static_cast<std::uint64_t>(
                kOutputSampleRate * kBytesPerOutputFrame
            );
        current_milliseconds = static_cast<i32>(std::min<std::uint64_t>(
            milliseconds, static_cast<std::uint64_t>(state->total_milliseconds)
        ));
    }

private:
    struct StreamState {
        SdlAudioStreamPtr output;
        std::vector<float> samples;
        std::array<i32, 8U> user_data{};
        i32 volume{};
        i32 loop_count{1};
        i32 remaining_replays{};
        i32 total_milliseconds{};
        bool infinite_loop{};
        bool started{};
        bool completed{};
    };

    [[nodiscard]] StreamState* find(const LegacyStreamHandle handle) {
        const auto found = streams_.find(handle);
        return found == streams_.end() ? nullptr : found->second.get();
    }

    [[nodiscard]] const StreamState*
    find(const LegacyStreamHandle handle) const {
        const auto found = streams_.find(handle);
        return found == streams_.end() ? nullptr : found->second.get();
    }

    std::filesystem::path data_directory_;
    std::unordered_map<LegacyStreamHandle, std::unique_ptr<StreamState>>
        streams_;
    LegacyStreamHandle next_handle_{1U};
    std::string last_error_;
};

class FfmpegVideoBackend final : public audio_video::LegacyVideoBackend {
public:
    audio_video::LegacyVideoOpenResult
    open_video(const std::string_view filename) override {
        auto state = std::make_unique<VideoState>();
        state->format = open_input(std::string{filename}, last_error_);
        if (!state->format) {
            return {};
        }
        state->video_decoder = open_decoder(
            *state->format,
            AVMEDIA_TYPE_VIDEO,
            state->video_stream_index,
            last_error_
        );
        if (!state->video_decoder) {
            return {};
        }
        state->packet.reset(av_packet_alloc());
        state->video_frame.reset(av_frame_alloc());
        state->audio_frame.reset(av_frame_alloc());
        if (!state->packet || !state->video_frame || !state->audio_frame) {
            last_error_ = "FFmpeg video working-set allocation failed";
            return {};
        }

        AVStream* const video_stream =
            state->format->streams[state->video_stream_index];
        state->frame_rate =
            av_guess_frame_rate(state->format.get(), video_stream, nullptr);
        if (state->frame_rate.num <= 0 || state->frame_rate.den <= 0) {
            state->frame_rate = AVRational{30, 1};
        }
        if (video_stream->duration > 0) {
            const std::int64_t frame_count = av_rescale_q(
                video_stream->duration,
                video_stream->time_base,
                av_inv_q(state->frame_rate)
            );
            if (frame_count > 0) {
                state->frame_count = static_cast<u32>(std::min<std::int64_t>(
                    frame_count, std::numeric_limits<u32>::max()
                ));
            }
        }
        if (state->frame_count == 0U) {
            state->frame_count = std::numeric_limits<u32>::max();
        }
        state->frame_duration_nanoseconds = 1'000'000'000ULL *
            static_cast<std::uint64_t>(state->frame_rate.den) /
            static_cast<std::uint64_t>(state->frame_rate.num);
        state->start_ticks = SDL_GetTicksNS();

        std::string ignored_audio_error;
        state->audio_decoder = open_decoder(
            *state->format,
            AVMEDIA_TYPE_AUDIO,
            state->audio_stream_index,
            ignored_audio_error
        );
        if (state->audio_decoder) {
            state->audio_resampler = make_audio_resampler(
                *state->audio_decoder, ignored_audio_error
            );
            if (state->audio_resampler) {
                state->audio_output = open_audio_output(ignored_audio_error);
            }
        }

        const LegacyVideoHandle handle = next_handle_++;
        const audio_video::LegacyVideoSummary summary{
            .width = state->video_decoder->width,
            .height = state->video_decoder->height,
        };
        videos_.emplace(handle, std::move(state));
        last_error_.clear();
        return {
            .disposition = audio_video::LegacyVideoOpenDisposition::opened,
            .handle = handle,
            .summary = summary,
        };
    }

    std::string_view last_error() const override {
        return last_error_;
    }

    void close_video(const LegacyVideoHandle handle) override {
        videos_.erase(handle);
    }

    void set_video_volume(
        const LegacyVideoHandle handle, const i32 volume
    ) override {
        VideoState* state = find(handle);
        if (state == nullptr || !state->audio_output) {
            return;
        }
        const float gain = std::clamp(
            static_cast<float>(volume) /
                static_cast<float>(audio_video::kLegacyVideoMaximumVolume),
            0.0F,
            1.0F
        );
        static_cast<void>(
            SDL_SetAudioStreamGain(state->audio_output.get(), gain)
        );
    }

    bool wait_for_video_frame(const LegacyVideoHandle handle) override {
        const VideoState* state = find(handle);
        if (state == nullptr) {
            return false;
        }
        const std::uint64_t due = state->start_ticks +
            static_cast<std::uint64_t>(state->frame_number) *
                state->frame_duration_nanoseconds;
        return SDL_GetTicksNS() < due;
    }

    void decode_video_frame(const LegacyVideoHandle handle) override {
        VideoState* state = find(handle);
        if (state == nullptr || state->video_frame_ready) {
            return;
        }
        while (true) {
            const int buffered_result = avcodec_receive_frame(
                state->video_decoder.get(), state->video_frame.get()
            );
            if (buffered_result == 0) {
                state->video_frame_ready = true;
                ++state->frame_number;
                return;
            }
            if (buffered_result != AVERROR(EAGAIN) &&
                buffered_result != AVERROR_EOF) {
                last_error_ = "FFmpeg video decode failed: " +
                    ffmpeg_error(buffered_result);
                return;
            }

            const int read_result =
                av_read_frame(state->format.get(), state->packet.get());
            if (read_result < 0) {
                if (!state->video_flushed) {
                    state->video_flushed = true;
                    static_cast<void>(
                        avcodec_send_packet(state->video_decoder.get(), nullptr)
                    );
                    continue;
                }
                if (state->frame_count == std::numeric_limits<u32>::max()) {
                    state->frame_count = state->frame_number;
                }
                return;
            }

            if (state->packet->stream_index == state->video_stream_index) {
                const int send_result = avcodec_send_packet(
                    state->video_decoder.get(), state->packet.get()
                );
                if (send_result < 0 && send_result != AVERROR(EAGAIN)) {
                    last_error_ = "FFmpeg video packet submit failed: " +
                        ffmpeg_error(send_result);
                }
            } else if (
                state->audio_decoder && state->audio_resampler &&
                state->audio_output &&
                state->packet->stream_index == state->audio_stream_index
            ) {
                decode_audio_packet(*state);
            }
            av_packet_unref(state->packet.get());
        }
    }

    i32 copy_video_frame(
        const LegacyVideoHandle handle,
        const audio_video::LegacyVideoCopyRequest& request
    ) override {
        VideoState* state = find(handle);
        if (state == nullptr || !state->video_frame_ready ||
            request.pitch_bytes <= 0 || request.destination_x < 0 ||
            request.destination_y < 0) {
            return 0;
        }
        const i32 width = state->video_decoder->width;
        const i32 height = state->video_decoder->height;
        if (request.destination_x + width >
                audio_video::kLegacyVideoCanvasWidth ||
            request.destination_y + height > request.destination_height) {
            return 0;
        }
        const std::size_t required_bytes =
            static_cast<std::size_t>(request.destination_y + height - 1) *
                static_cast<std::size_t>(request.pitch_bytes) +
            static_cast<std::size_t>(request.destination_x + width) *
                sizeof(compat::u16);
        if (required_bytes > request.destination.size_bytes()) {
            return 0;
        }

        const AVPixelFormat destination_format =
            request.pixel_format == audio_video::LegacyVideoPixelFormat::rgb555
            ? AV_PIX_FMT_RGB555LE
            : AV_PIX_FMT_RGB565LE;
        SwsContext* const raw_scaler = sws_getCachedContext(
            state->scaler.release(),
            width,
            height,
            static_cast<AVPixelFormat>(state->video_frame->format),
            width,
            height,
            destination_format,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );
        state->scaler.reset(raw_scaler);
        if (!state->scaler) {
            last_error_ = "FFmpeg video scaler allocation failed";
            return 0;
        }

        std::array<std::uint8_t*, 4U> destination_data{};
        std::array<int, 4U> destination_linesize{};
        destination_data[0] =
            reinterpret_cast<std::uint8_t*>(request.destination.data()) +
            static_cast<std::size_t>(request.destination_y) *
                static_cast<std::size_t>(request.pitch_bytes) +
            static_cast<std::size_t>(request.destination_x) *
                sizeof(compat::u16);
        destination_linesize[0] = request.pitch_bytes;
        const int rows = sws_scale(
            state->scaler.get(),
            state->video_frame->data,
            state->video_frame->linesize,
            0,
            height,
            destination_data.data(),
            destination_linesize.data()
        );
        return rows == height ? 1 : 0;
    }

    u32 video_frame_count(const LegacyVideoHandle handle) override {
        const VideoState* state = find(handle);
        return state == nullptr ? 0U : state->frame_count;
    }

    u32 video_frame_number(const LegacyVideoHandle handle) override {
        const VideoState* state = find(handle);
        return state == nullptr ? 0U : state->frame_number;
    }

    void advance_video_frame(const LegacyVideoHandle handle) override {
        VideoState* state = find(handle);
        if (state == nullptr) {
            return;
        }
        av_frame_unref(state->video_frame.get());
        state->video_frame_ready = false;
    }

    void service_video(const LegacyVideoHandle handle) override {
        VideoState* state = find(handle);
        if (state != nullptr && state->audio_output && !state->audio_started &&
            SDL_GetAudioStreamQueued(state->audio_output.get()) > 0) {
            state->audio_started = true;
            static_cast<void>(
                SDL_ResumeAudioStreamDevice(state->audio_output.get())
            );
        }
    }

private:
    struct VideoState {
        FormatInputPtr format;
        CodecContextPtr video_decoder;
        CodecContextPtr audio_decoder;
        PacketPtr packet;
        FramePtr video_frame;
        FramePtr audio_frame;
        SwrContextPtr audio_resampler;
        SwsContextPtr scaler;
        SdlAudioStreamPtr audio_output;
        int video_stream_index{-1};
        int audio_stream_index{-1};
        AVRational frame_rate{30, 1};
        u32 frame_count{};
        u32 frame_number{};
        std::uint64_t frame_duration_nanoseconds{};
        std::uint64_t start_ticks{};
        bool video_frame_ready{};
        bool video_flushed{};
        bool audio_started{};
    };

    void decode_audio_packet(VideoState& state) {
        const int send_result =
            avcodec_send_packet(state.audio_decoder.get(), state.packet.get());
        if (send_result < 0 && send_result != AVERROR(EAGAIN)) {
            return;
        }
        while (true) {
            const int receive_result = avcodec_receive_frame(
                state.audio_decoder.get(), state.audio_frame.get()
            );
            if (receive_result == AVERROR(EAGAIN) ||
                receive_result == AVERROR_EOF) {
                return;
            }
            if (receive_result < 0) {
                return;
            }
            std::vector<float> samples;
            std::string error;
            if (append_resampled_frame(
                    *state.audio_resampler,
                    *state.audio_decoder,
                    *state.audio_frame,
                    samples,
                    error
                )) {
                static_cast<void>(
                    queue_samples(*state.audio_output, samples, error)
                );
            }
            av_frame_unref(state.audio_frame.get());
        }
    }

    [[nodiscard]] VideoState* find(const LegacyVideoHandle handle) {
        const auto found = videos_.find(handle);
        return found == videos_.end() ? nullptr : found->second.get();
    }

    [[nodiscard]] const VideoState* find(const LegacyVideoHandle handle) const {
        const auto found = videos_.find(handle);
        return found == videos_.end() ? nullptr : found->second.get();
    }

    std::unordered_map<LegacyVideoHandle, std::unique_ptr<VideoState>> videos_;
    LegacyVideoHandle next_handle_{1U};
    std::string last_error_;
};

}  // namespace

std::unique_ptr<audio_video::LegacyStreamBackend>
make_legacy_stream_backend(std::filesystem::path data_directory) {
    return std::make_unique<FfmpegStreamBackend>(std::move(data_directory));
}

std::unique_ptr<audio_video::LegacyVideoBackend> make_legacy_video_backend() {
    return std::make_unique<FfmpegVideoBackend>();
}

std::string_view linked_ffmpeg_version() noexcept {
    return av_version_info();
}

}  // namespace openswd3::media_ffmpeg
