#include "platform/media_decoder.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "platform/file_system.h"
#include "platform/surface.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace swd3
{

namespace
{

constexpr int kIoBufferSize = 0x8000;
constexpr float kFullVolume = 32768.0F;

int ReadFile(void* opaque, u8* buffer, int size)
{
    const std::size_t read = std::fread(buffer, 1, static_cast<std::size_t>(size), static_cast<std::FILE*>(opaque));
    return read == 0 ? AVERROR_EOF : static_cast<int>(read);
}

i64 SeekFile(void* opaque, i64 offset, int whence)
{
    auto* file = static_cast<std::FILE*>(opaque);

    if ((whence & ~AVSEEK_FORCE) == AVSEEK_SIZE)
    {
        const long position = std::ftell(file);
        std::fseek(file, 0, SEEK_END);
        const long size = std::ftell(file);
        std::fseek(file, position, SEEK_SET);
        return size;
    }

    if (std::fseek(file, static_cast<long>(offset), whence & ~AVSEEK_FORCE) != 0)
    {
        return -1;
    }

    return std::ftell(file);
}

AVCodecContext* OpenCodec(const AVStream* stream)
{
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (codec == nullptr)
    {
        return nullptr;
    }

    AVCodecContext* context = avcodec_alloc_context3(codec);

    if (context == nullptr || avcodec_parameters_to_context(context, stream->codecpar) < 0 ||
        avcodec_open2(context, codec, nullptr) < 0)
    {
        avcodec_free_context(&context);
        return nullptr;
    }

    return context;
}

}  // namespace

struct MediaDecoder::State
{
    ~State()
    {
        if (this->output != nullptr)
        {
            SDL_DestroyAudioStream(this->output);
        }

        swr_free(&this->resampler);
        sws_freeContext(this->scaler);
        av_frame_free(&this->frame);
        av_packet_free(&this->packet);
        avcodec_free_context(&this->video);
        avcodec_free_context(&this->audio);
        avformat_close_input(&this->format);

        if (this->io != nullptr)
        {
            av_freep(&this->io->buffer);
            avio_context_free(&this->io);
        }

        if (this->file != nullptr)
        {
            std::fclose(this->file);
        }
    }

    void OpenAudioOutput()
    {
        if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            return;
        }

        const int channels = this->audio->ch_layout.nb_channels;
        const SDL_AudioSpec spec{SDL_AUDIO_S16, channels, this->audio->sample_rate};
        this->output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);

        if (this->output == nullptr)
        {
            return;
        }

        const int configured = swr_alloc_set_opts2(
            &this->resampler,
            &this->audio->ch_layout,
            AV_SAMPLE_FMT_S16,
            this->audio->sample_rate,
            &this->audio->ch_layout,
            this->audio->sample_fmt,
            this->audio->sample_rate,
            0,
            nullptr
        );

        if (configured < 0 || swr_init(this->resampler) < 0)
        {
            SDL_DestroyAudioStream(this->output);
            this->output = nullptr;
            return;
        }

        SDL_ResumeAudioStreamDevice(this->output);
    }

    void QueueAudio()
    {
        while (avcodec_receive_frame(this->audio, this->frame) == 0)
        {
            if (this->output != nullptr)
            {
                const int channels = this->audio->ch_layout.nb_channels;
                std::vector<i16> pcm(static_cast<std::size_t>(this->frame->nb_samples) * channels);
                u8* out = reinterpret_cast<u8*>(pcm.data());
                const int converted = swr_convert(
                    this->resampler,
                    &out,
                    this->frame->nb_samples,
                    const_cast<const u8**>(this->frame->extended_data),
                    this->frame->nb_samples
                );

                if (converted > 0)
                {
                    SDL_PutAudioStreamData(this->output, pcm.data(), converted * channels * 2);
                }
            }

            av_frame_unref(this->frame);
        }
    }

    bool StoreVideo()
    {
        if (avcodec_receive_frame(this->video, this->frame) != 0)
        {
            return false;
        }

        this->scaler = sws_getCachedContext(
            this->scaler,
            this->frame->width,
            this->frame->height,
            static_cast<AVPixelFormat>(this->frame->format),
            this->width,
            this->height,
            AV_PIX_FMT_RGB565LE,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );

        if (this->scaler != nullptr)
        {
            u8* planes[4] = {reinterpret_cast<u8*>(this->pixels.data()), nullptr, nullptr, nullptr};
            const int pitches[4] = {this->width * 2, 0, 0, 0};
            sws_scale(this->scaler, this->frame->data, this->frame->linesize, 0, this->frame->height, planes, pitches);
        }

        av_frame_unref(this->frame);
        return true;
    }

    // Reads packets up to the next video frame, queueing the audio met on the way.
    void DecodeNext()
    {
        while (!this->ended)
        {
            if (av_read_frame(this->format, this->packet) < 0)
            {
                this->ended = true;
                avcodec_send_packet(this->video, nullptr);
                this->StoreVideo();
                return;
            }

            const int index = this->packet->stream_index;

            if (index == this->audioIndex && this->audio != nullptr)
            {
                if (avcodec_send_packet(this->audio, this->packet) == 0)
                {
                    this->QueueAudio();
                }
            }
            else if (index == this->videoIndex)
            {
                const bool sent = avcodec_send_packet(this->video, this->packet) == 0;
                av_packet_unref(this->packet);

                if (sent && this->StoreVideo())
                {
                    return;
                }

                continue;
            }

            av_packet_unref(this->packet);
        }
    }

    std::FILE* file = nullptr;
    AVIOContext* io = nullptr;
    AVFormatContext* format = nullptr;
    AVCodecContext* video = nullptr;
    AVCodecContext* audio = nullptr;
    int videoIndex = -1;
    int audioIndex = -1;
    SwsContext* scaler = nullptr;
    SwrContext* resampler = nullptr;
    SDL_AudioStream* output = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    int width = 0;
    int height = 0;
    u32 frames = 0;
    u32 frameNum = 1;
    std::vector<u16> pixels;
    bool decoded = false;
    bool ended = false;
    bool started = false;
    u64 startNs = 0;
    u64 frameNs = 0;
};

MediaDecoder::MediaDecoder() = default;

MediaDecoder::~MediaDecoder() = default;

bool MediaDecoder::Open(const std::string& path)
{
    this->Close();
    auto state = std::make_unique<State>();
    state->file = std::fopen(ResolvePath(path).c_str(), "rb");

    if (state->file == nullptr)
    {
        this->error = "Error opening file: " + path;
        return false;
    }

    auto* buffer = static_cast<u8*>(av_malloc(kIoBufferSize));
    state->io = avio_alloc_context(buffer, kIoBufferSize, 0, state->file, &ReadFile, nullptr, &SeekFile);
    state->format = avformat_alloc_context();

    if (state->io == nullptr || state->format == nullptr)
    {
        this->error = "Out of memory";
        return false;
    }

    state->format->pb = state->io;

    if (avformat_open_input(&state->format, nullptr, av_find_input_format("bink"), nullptr) < 0)
    {
        this->error = "Not a Bink file: " + path;
        return false;
    }

    state->videoIndex = av_find_best_stream(state->format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    state->audioIndex = av_find_best_stream(state->format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (state->videoIndex < 0)
    {
        this->error = "No video track";
        return false;
    }

    const AVStream* stream = state->format->streams[state->videoIndex];
    state->video = OpenCodec(stream);

    if (state->video == nullptr)
    {
        this->error = "Unsupported Bink video";
        return false;
    }

    if (state->audioIndex >= 0)
    {
        state->audio = OpenCodec(state->format->streams[state->audioIndex]);

        if (state->audio != nullptr)
        {
            state->OpenAudioOutput();
        }
    }

    state->packet = av_packet_alloc();
    state->frame = av_frame_alloc();
    state->width = state->video->width;
    state->height = state->video->height;
    state->pixels.assign(static_cast<std::size_t>(state->width) * state->height, 0);
    state->frames = static_cast<u32>(stream->nb_frames != 0 ? stream->nb_frames : stream->duration);

    if (stream->avg_frame_rate.num != 0)
    {
        state->frameNs =
            1000000000ULL * static_cast<u64>(stream->avg_frame_rate.den) / static_cast<u64>(stream->avg_frame_rate.num);
    }

    this->state = std::move(state);
    return true;
}

void MediaDecoder::Close()
{
    this->state.reset();
}

bool MediaDecoder::IsOpen() const
{
    return this->state != nullptr;
}

const std::string& MediaDecoder::Error() const
{
    return this->error;
}

int MediaDecoder::Width() const
{
    return this->state != nullptr ? this->state->width : 0;
}

int MediaDecoder::Height() const
{
    return this->state != nullptr ? this->state->height : 0;
}

u32 MediaDecoder::Frames() const
{
    return this->state != nullptr ? this->state->frames : 0;
}

u32 MediaDecoder::FrameNum() const
{
    return this->state != nullptr ? this->state->frameNum : 0;
}

void MediaDecoder::SetVolume(int volume)
{
    if (this->state != nullptr && this->state->output != nullptr)
    {
        SDL_SetAudioStreamGain(this->state->output, static_cast<float>(volume) / kFullVolume);
    }
}

void MediaDecoder::DecodeFrame()
{
    if (this->state == nullptr)
    {
        return;
    }

    if (!this->state->started)
    {
        this->state->started = true;
        this->state->startNs = SDL_GetTicksNS();
    }

    if (!this->state->decoded)
    {
        this->state->DecodeNext();
        this->state->decoded = true;
    }
}

bool MediaDecoder::CopyTo(Surface& surface, int x, int y) const
{
    if (this->state == nullptr || x < 0 || y < 0)
    {
        return false;
    }

    const int rows = std::min(this->state->height, surface.Height() - y);
    const int columns = std::min(this->state->width, surface.Width() - x);

    for (int row = 0; row < rows; ++row)
    {
        std::memcpy(
            surface.Row(y + row) + x,
            this->state->pixels.data() + static_cast<std::size_t>(row) * this->state->width,
            static_cast<std::size_t>(columns) * 2
        );
    }

    return true;
}

void MediaDecoder::NextFrame()
{
    if (this->state == nullptr)
    {
        return;
    }

    if (this->state->frameNum < this->state->frames)
    {
        ++this->state->frameNum;
    }

    this->state->decoded = false;
}

bool MediaDecoder::Wait() const
{
    if (this->state == nullptr || !this->state->started)
    {
        return false;
    }

    const u64 due = this->state->startNs + this->state->frameNs * (this->state->frameNum - 1);
    return SDL_GetTicksNS() < due;
}

}  // namespace swd3
