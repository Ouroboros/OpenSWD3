#include "swd3/bink.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "swd3/w32.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace bink {

struct Player {
    w32::HANDLE file = w32::INVALID_HANDLE_VALUE;
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
    std::vector<std::uint16_t> pixels;
    bool decoded = false;
    bool ended = false;
    bool started = false;
    std::uint64_t startNs = 0;
    std::uint64_t frameNs = 0;
};

namespace {

std::string g_error = "";

int readFile(void* opaque, std::uint8_t* buffer, int size) {
    auto* player = static_cast<Player*>(opaque);
    w32::DWORD read = 0;
    if (!w32::ReadFile(player->file, buffer, static_cast<w32::DWORD>(size), &read, nullptr) || read == 0) {
        return AVERROR_EOF;
    }
    return static_cast<int>(read);
}

std::int64_t seekFile(void* opaque, std::int64_t offset, int whence) {
    auto* player = static_cast<Player*>(opaque);
    if ((whence & ~AVSEEK_FORCE) == AVSEEK_SIZE) {
        return w32::GetFileSize(player->file, nullptr);
    }
    w32::DWORD method = w32::FILE_BEGIN;
    switch (whence & ~AVSEEK_FORCE) {
    case SEEK_CUR:
        method = w32::FILE_CURRENT;
        break;
    case SEEK_END:
        method = w32::FILE_END;
        break;
    default:
        break;
    }
    const w32::DWORD position = w32::SetFilePointer(player->file, static_cast<w32::LONG>(offset), nullptr, method);
    if (position == 0xFFFFFFFFu) {
        return -1;
    }
    return position;
}

AVCodecContext* openDecoder(const AVStream* stream) {
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        return nullptr;
    }
    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (context == nullptr || avcodec_parameters_to_context(context, stream->codecpar) < 0 ||
        avcodec_open2(context, codec, nullptr) < 0) {
        avcodec_free_context(&context);
        return nullptr;
    }
    return context;
}

void openAudioOutput(Player* player) {
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return;
    }
    const int channels = player->audio->ch_layout.nb_channels;
    const SDL_AudioSpec spec{SDL_AUDIO_S16, channels, player->audio->sample_rate};
    player->output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (player->output == nullptr) {
        return;
    }
    AVChannelLayout layout = player->audio->ch_layout;
    if (swr_alloc_set_opts2(&player->resampler, &layout, AV_SAMPLE_FMT_S16, player->audio->sample_rate, &layout,
                            player->audio->sample_fmt, player->audio->sample_rate, 0, nullptr) < 0 ||
        swr_init(player->resampler) < 0) {
        SDL_DestroyAudioStream(player->output);
        player->output = nullptr;
        return;
    }
    SDL_ResumeAudioStreamDevice(player->output);
}

void queueAudio(Player* player) {
    while (avcodec_receive_frame(player->audio, player->frame) == 0) {
        if (player->output != nullptr) {
            const int channels = player->audio->ch_layout.nb_channels;
            std::vector<std::int16_t> pcm(static_cast<std::size_t>(player->frame->nb_samples) * channels);
            auto* out = reinterpret_cast<std::uint8_t*>(pcm.data());
            const int converted =
                swr_convert(player->resampler, &out, player->frame->nb_samples,
                            const_cast<const std::uint8_t**>(player->frame->extended_data), player->frame->nb_samples);
            if (converted > 0) {
                SDL_PutAudioStreamData(player->output, pcm.data(), converted * channels * 2);
            }
        }
        av_frame_unref(player->frame);
    }
}

bool storeVideo(Player* player) {
    if (avcodec_receive_frame(player->video, player->frame) != 0) {
        return false;
    }
    const int width = player->video->width;
    const int height = player->video->height;
    player->scaler = sws_getCachedContext(player->scaler, player->frame->width, player->frame->height,
                                          static_cast<AVPixelFormat>(player->frame->format), width, height,
                                          AV_PIX_FMT_RGB565LE, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (player->scaler != nullptr) {
        std::uint8_t* planes[4] = {reinterpret_cast<std::uint8_t*>(player->pixels.data()), nullptr, nullptr,
                                   nullptr};
        const int pitches[4] = {width * 2, 0, 0, 0};
        sws_scale(player->scaler, player->frame->data, player->frame->linesize, 0, player->frame->height, planes,
                  pitches);
    }
    av_frame_unref(player->frame);
    return true;
}

// reads packets up to the next video frame, queueing the audio met on the way
void decodeNext(Player* player) {
    while (!player->ended) {
        if (av_read_frame(player->format, player->packet) < 0) {
            player->ended = true;
            avcodec_send_packet(player->video, nullptr);
            storeVideo(player);
            return;
        }
        const int index = player->packet->stream_index;
        if (index == player->audioIndex && player->audio != nullptr) {
            if (avcodec_send_packet(player->audio, player->packet) == 0) {
                queueAudio(player);
            }
        } else if (index == player->videoIndex) {
            const bool sent = avcodec_send_packet(player->video, player->packet) == 0;
            av_packet_unref(player->packet);
            if (sent && storeVideo(player)) {
                return;
            }
            continue;
        }
        av_packet_unref(player->packet);
    }
}

void destroy(Player* player) {
    if (player->output != nullptr) {
        SDL_DestroyAudioStream(player->output);
    }
    swr_free(&player->resampler);
    sws_freeContext(player->scaler);
    av_frame_free(&player->frame);
    av_packet_free(&player->packet);
    avcodec_free_context(&player->video);
    avcodec_free_context(&player->audio);
    avformat_close_input(&player->format);
    if (player->io != nullptr) {
        av_freep(&player->io->buffer);
        avio_context_free(&player->io);
    }
    if (player->file != w32::INVALID_HANDLE_VALUE) {
        w32::CloseHandle(player->file);
    }
    delete player;
}

}  // namespace

S32 BinkOpenDirectSound(U32) { return 1; }

S32 BinkOpenMiles(U32) { return 1; }

S32 BinkSetSoundSystem(SoundSystemOpen open, U32 param) { return open != nullptr ? open(param) : 0; }

S32 BinkDDSurfaceType(void*) { return static_cast<S32>(BINKSURFACE565); }

HBINK BinkOpen(const char* name, U32) {
    auto* player = new Player();
    player->file = w32::CreateFileA(name, w32::GENERIC_READ, 1, nullptr, w32::OPEN_EXISTING,
                                    w32::FILE_ATTRIBUTE_NORMAL, nullptr);
    if (player->file == w32::INVALID_HANDLE_VALUE) {
        g_error = std::string("Error opening file: ") + name;
        destroy(player);
        return nullptr;
    }
    constexpr int kBufferSize = 0x8000;
    auto* buffer = static_cast<std::uint8_t*>(av_malloc(kBufferSize));
    player->io = avio_alloc_context(buffer, kBufferSize, 0, player, &readFile, nullptr, &seekFile);
    player->format = avformat_alloc_context();
    if (player->io == nullptr || player->format == nullptr) {
        g_error = "Out of memory";
        destroy(player);
        return nullptr;
    }
    player->format->pb = player->io;
    if (avformat_open_input(&player->format, nullptr, av_find_input_format("bink"), nullptr) < 0) {
        g_error = std::string("Not a Bink file: ") + name;
        destroy(player);
        return nullptr;
    }
    player->videoIndex = av_find_best_stream(player->format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    player->audioIndex = av_find_best_stream(player->format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (player->videoIndex < 0) {
        g_error = "No video track";
        destroy(player);
        return nullptr;
    }
    const AVStream* videoStream = player->format->streams[player->videoIndex];
    player->video = openDecoder(videoStream);
    if (player->video == nullptr) {
        g_error = "Unsupported Bink video";
        destroy(player);
        return nullptr;
    }
    if (player->audioIndex >= 0) {
        player->audio = openDecoder(player->format->streams[player->audioIndex]);
        if (player->audio != nullptr) {
            openAudioOutput(player);
        }
    }
    player->packet = av_packet_alloc();
    player->frame = av_frame_alloc();
    player->pixels.assign(static_cast<std::size_t>(player->video->width) * player->video->height, 0);
    auto* handle = new BINK{};
    handle->player = player;
    handle->Width = static_cast<U32>(player->video->width);
    handle->Height = static_cast<U32>(player->video->height);
    handle->Frames = static_cast<U32>(videoStream->nb_frames != 0 ? videoStream->nb_frames : videoStream->duration);
    handle->FrameNum = 1;
    handle->FrameRate = static_cast<U32>(videoStream->avg_frame_rate.num);
    handle->FrameRateDiv = static_cast<U32>(videoStream->avg_frame_rate.den);
    if (handle->FrameRate != 0) {
        player->frameNs = 1000000000ull * handle->FrameRateDiv / handle->FrameRate;
    }
    return handle;
}

const char* BinkGetError() { return g_error.c_str(); }

void BinkGetSummary(HBINK bink, BINKSUMMARY* summary) {
    std::memset(summary, 0, sizeof(*summary));
    if (bink == nullptr) {
        return;
    }
    summary->Width = bink->Width;
    summary->Height = bink->Height;
    summary->FileFrameRate = bink->FrameRate;
    summary->FileFrameRateDiv = bink->FrameRateDiv;
    summary->FrameRate = bink->FrameRate;
    summary->FrameRateDiv = bink->FrameRateDiv;
    if (bink->FrameRate != 0) {
        summary->TotalTime =
            static_cast<U32>(1000ull * bink->Frames * bink->FrameRateDiv / bink->FrameRate);
    }
}

void BinkSetVolume(HBINK bink, S32 volume) {
    if (bink != nullptr && bink->player->output != nullptr) {
        SDL_SetAudioStreamGain(bink->player->output, static_cast<float>(volume) / 32768.0f);
    }
}

void BinkClose(HBINK bink) {
    if (bink == nullptr) {
        return;
    }
    destroy(bink->player);
    delete bink;
}

S32 BinkDoFrame(HBINK bink) {
    Player* player = bink->player;
    if (!player->started) {
        player->started = true;
        player->startNs = SDL_GetTicksNS();
    }
    if (!player->decoded) {
        decodeNext(player);
        player->decoded = true;
    }
    return 0;
}

S32 BinkCopyToBuffer(HBINK bink, void* dest, S32 destPitch, U32 destHeight, U32 destX, U32 destY, U32 flags) {
    if (bink == nullptr || dest == nullptr || flags != BINKSURFACE565) {
        return 0;
    }
    const Player* player = bink->player;
    const U32 rows = destY < destHeight ? std::min(bink->Height, destHeight - destY) : 0;
    for (U32 row = 0; row < rows; ++row) {
        auto* line = static_cast<std::uint8_t*>(dest) + static_cast<std::size_t>(destY + row) * destPitch + destX * 2;
        std::memcpy(line, player->pixels.data() + static_cast<std::size_t>(row) * bink->Width, bink->Width * 2);
    }
    return 1;
}

void BinkNextFrame(HBINK bink) {
    bink->LastFrameNum = bink->FrameNum;
    if (bink->FrameNum < bink->Frames) {
        ++bink->FrameNum;
    }
    bink->player->decoded = false;
}

void BinkService(HBINK) {}

S32 BinkWait(HBINK bink) {
    const Player* player = bink->player;
    if (!player->started) {
        return 0;
    }
    const std::uint64_t due = player->startNs + player->frameNs * (bink->FrameNum - 1);
    return SDL_GetTicksNS() < due ? 1 : 0;
}

}  // namespace bink
