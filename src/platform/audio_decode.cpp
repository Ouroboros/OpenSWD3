#include "platform/audio_decode.h"

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace swd3::platform {
namespace {

struct MemoryReader {
    const std::uint8_t* data;
    std::size_t size;
    std::size_t position;
};

int readPacket(void* opaque, std::uint8_t* buffer, int bufferSize) {
    auto* reader = static_cast<MemoryReader*>(opaque);
    const std::size_t left = reader->size - reader->position;
    if (left == 0) {
        return AVERROR_EOF;
    }
    const std::size_t n = left < static_cast<std::size_t>(bufferSize) ? left : static_cast<std::size_t>(bufferSize);
    std::memcpy(buffer, reader->data + reader->position, n);
    reader->position += n;
    return static_cast<int>(n);
}

std::int64_t seekPacket(void* opaque, std::int64_t offset, int whence) {
    auto* reader = static_cast<MemoryReader*>(opaque);
    std::int64_t base = 0;
    switch (whence & ~AVSEEK_FORCE) {
    case SEEK_SET:
        base = 0;
        break;
    case SEEK_CUR:
        base = static_cast<std::int64_t>(reader->position);
        break;
    case SEEK_END:
        base = static_cast<std::int64_t>(reader->size);
        break;
    case AVSEEK_SIZE:
        return static_cast<std::int64_t>(reader->size);
    default:
        return -1;
    }
    const std::int64_t target = base + offset;
    if (target < 0 || target > static_cast<std::int64_t>(reader->size)) {
        return -1;
    }
    reader->position = static_cast<std::size_t>(target);
    return target;
}

void appendConverted(SwrContext* resampler, const std::uint8_t** input, int inputSamples,
                     std::vector<std::int16_t>& pcm) {
    const int capacity = swr_get_out_samples(resampler, inputSamples);
    if (capacity <= 0) {
        return;
    }
    const std::size_t start = pcm.size();
    pcm.resize(start + static_cast<std::size_t>(capacity) * 2);
    std::uint8_t* output = reinterpret_cast<std::uint8_t*>(pcm.data() + start);
    const int produced = swr_convert(resampler, &output, capacity, input, inputSamples);
    pcm.resize(start + static_cast<std::size_t>(produced > 0 ? produced : 0) * 2);
}

}  // namespace

bool decodeAudio(const std::uint8_t* data, std::size_t size, std::vector<std::int16_t>& pcm) {
    pcm.clear();
    MemoryReader reader{data, size, 0};
    constexpr int kBufferSize = 0x8000;
    auto* ioBuffer = static_cast<std::uint8_t*>(av_malloc(kBufferSize));
    AVIOContext* io = avio_alloc_context(ioBuffer, kBufferSize, 0, &reader, &readPacket, nullptr, &seekPacket);
    AVFormatContext* format = avformat_alloc_context();
    format->pb = io;
    bool ok = false;
    AVCodecContext* codec = nullptr;
    SwrContext* resampler = nullptr;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (avformat_open_input(&format, nullptr, nullptr, nullptr) == 0 &&
        avformat_find_stream_info(format, nullptr) >= 0) {
        const AVCodec* decoder = nullptr;
        const int stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
        if (stream >= 0 && decoder != nullptr) {
            codec = avcodec_alloc_context3(decoder);
            avcodec_parameters_to_context(codec, format->streams[stream]->codecpar);
            if (avcodec_open2(codec, decoder, nullptr) == 0) {
                AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
                if (swr_alloc_set_opts2(&resampler, &stereo, AV_SAMPLE_FMT_S16, kMixRate, &codec->ch_layout,
                                        codec->sample_fmt, codec->sample_rate, 0, nullptr) == 0 &&
                    swr_init(resampler) == 0) {
                    ok = true;
                    while (av_read_frame(format, packet) >= 0) {
                        if (packet->stream_index == stream && avcodec_send_packet(codec, packet) == 0) {
                            while (avcodec_receive_frame(codec, frame) == 0) {
                                appendConverted(resampler, const_cast<const std::uint8_t**>(frame->extended_data),
                                                frame->nb_samples, pcm);
                            }
                        }
                        av_packet_unref(packet);
                    }
                    avcodec_send_packet(codec, nullptr);
                    while (avcodec_receive_frame(codec, frame) == 0) {
                        appendConverted(resampler, const_cast<const std::uint8_t**>(frame->extended_data),
                                        frame->nb_samples, pcm);
                    }
                    appendConverted(resampler, nullptr, 0, pcm);
                }
            }
        }
    }
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&resampler);
    avcodec_free_context(&codec);
    avformat_close_input(&format);
    if (io != nullptr) {
        av_freep(&io->buffer);
        avio_context_free(&io);
    }
    return ok;
}

bool convertPcm(const std::uint8_t* data, std::size_t bytes, int channels, int rate, int bits,
                std::vector<std::int16_t>& pcm) {
    pcm.clear();
    if (channels < 1 || channels > 2 || rate <= 0 || (bits != 8 && bits != 16)) {
        return false;
    }
    AVChannelLayout input;
    av_channel_layout_default(&input, channels);
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    SwrContext* resampler = nullptr;
    const AVSampleFormat format = bits == 8 ? AV_SAMPLE_FMT_U8 : AV_SAMPLE_FMT_S16;
    if (swr_alloc_set_opts2(&resampler, &stereo, AV_SAMPLE_FMT_S16, kMixRate, &input, format, rate, 0, nullptr) != 0 ||
        swr_init(resampler) != 0) {
        swr_free(&resampler);
        return false;
    }
    const int frames = static_cast<int>(bytes / static_cast<std::size_t>(channels * bits / 8));
    const std::uint8_t* planes[1] = {data};
    appendConverted(resampler, planes, frames, pcm);
    appendConverted(resampler, nullptr, 0, pcm);
    swr_free(&resampler);
    return true;
}

}  // namespace swd3::platform
