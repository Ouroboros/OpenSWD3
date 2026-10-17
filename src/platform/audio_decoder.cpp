#include "platform/audio_decoder.h"

#include <climits>
#include <cstring>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace swd3
{

namespace
{

constexpr int kNarrowBits = 8;  // the sample sizes of raw PCM
constexpr int kWideBits = 16;

class MemoryReader
{
public:
    MemoryReader(const u8* data, std::size_t size)
        : data(data),
          size(size)
    {
    }

    static int Read(void* opaque, u8* buffer, int bufferSize)
    {
        auto* reader = static_cast<MemoryReader*>(opaque);
        const std::size_t left = reader->size - reader->position;

        if (left == 0)
        {
            return AVERROR_EOF;
        }

        const std::size_t count =
            left < static_cast<std::size_t>(bufferSize) ? left : static_cast<std::size_t>(bufferSize);
        std::memcpy(buffer, reader->data + reader->position, count);
        reader->position += count;
        return static_cast<int>(count);
    }

    static i64 Seek(void* opaque, i64 offset, int whence)
    {
        auto* reader = static_cast<MemoryReader*>(opaque);
        i64 base = 0;

        switch (whence & ~AVSEEK_FORCE)
        {
            case SEEK_SET:
                base = 0;
                break;

            case SEEK_CUR:
                base = static_cast<i64>(reader->position);
                break;

            case SEEK_END:
                base = static_cast<i64>(reader->size);
                break;

            case AVSEEK_SIZE:
                return static_cast<i64>(reader->size);

            default:
                return -1;
        }

        const i64 target = base + offset;

        if (target < 0 || target > static_cast<i64>(reader->size))
        {
            return -1;
        }

        reader->position = static_cast<std::size_t>(target);
        return target;
    }

private:
    const u8* data;
    std::size_t size;
    std::size_t position = 0;
};

void AppendConverted(SwrContext* resampler, const u8** input, int inputSamples, std::vector<i16>& pcm)
{
    const int capacity = swr_get_out_samples(resampler, inputSamples);

    if (capacity <= 0)
    {
        return;
    }

    const std::size_t start = pcm.size();
    pcm.resize(start + static_cast<std::size_t>(capacity) * kMixChannels);
    u8* output = reinterpret_cast<u8*>(pcm.data() + start);
    const int produced = swr_convert(resampler, &output, capacity, input, inputSamples);
    pcm.resize(start + static_cast<std::size_t>(produced > 0 ? produced : 0) * kMixChannels);
}

}  // namespace

bool DecodeAudio(const u8* data, std::size_t size, std::vector<i16>& pcm)
{
    pcm.clear();
    MemoryReader reader(data, size);
    constexpr int kBufferSize = 0x8000;
    auto* ioBuffer = static_cast<u8*>(av_malloc(kBufferSize));
    AVIOContext* io =
        avio_alloc_context(ioBuffer, kBufferSize, 0, &reader, &MemoryReader::Read, nullptr, &MemoryReader::Seek);
    AVFormatContext* format = avformat_alloc_context();
    format->pb = io;
    bool decoded = false;
    AVCodecContext* codec = nullptr;
    SwrContext* resampler = nullptr;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    auto drain = [&]
    {
        while (avcodec_receive_frame(codec, frame) == 0)
        {
            AppendConverted(resampler, const_cast<const u8**>(frame->extended_data), frame->nb_samples, pcm);
        }
    };

    if (avformat_open_input(&format, nullptr, nullptr, nullptr) == 0 && avformat_find_stream_info(format, nullptr) >= 0)
    {
        const AVCodec* decoder = nullptr;
        const int stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);

        if (stream >= 0 && decoder != nullptr)
        {
            codec = avcodec_alloc_context3(decoder);
            avcodec_parameters_to_context(codec, format->streams[stream]->codecpar);

            if (avcodec_open2(codec, decoder, nullptr) == 0)
            {
                AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
                const int configured = swr_alloc_set_opts2(
                    &resampler,
                    &stereo,
                    AV_SAMPLE_FMT_S16,
                    kMixRate,
                    &codec->ch_layout,
                    codec->sample_fmt,
                    codec->sample_rate,
                    0,
                    nullptr
                );

                if (configured == 0 && swr_init(resampler) == 0)
                {
                    decoded = true;

                    while (av_read_frame(format, packet) >= 0)
                    {
                        if (packet->stream_index == stream && avcodec_send_packet(codec, packet) == 0)
                        {
                            drain();
                        }

                        av_packet_unref(packet);
                    }

                    avcodec_send_packet(codec, nullptr);
                    drain();
                    AppendConverted(resampler, nullptr, 0, pcm);
                }
            }
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&resampler);
    avcodec_free_context(&codec);
    avformat_close_input(&format);

    if (io != nullptr)
    {
        av_freep(&io->buffer);
        avio_context_free(&io);
    }

    return decoded;
}

bool ConvertPcm(const u8* data, std::size_t bytes, int channels, int rate, int bits, std::vector<i16>& pcm)
{
    pcm.clear();

    if (channels < 1 || channels > kMixChannels || rate <= 0 || (bits != kNarrowBits && bits != kWideBits))
    {
        return false;
    }

    AVChannelLayout input;
    av_channel_layout_default(&input, channels);
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    SwrContext* resampler = nullptr;
    const AVSampleFormat format = bits == kNarrowBits ? AV_SAMPLE_FMT_U8 : AV_SAMPLE_FMT_S16;
    const int configured =
        swr_alloc_set_opts2(&resampler, &stereo, AV_SAMPLE_FMT_S16, kMixRate, &input, format, rate, 0, nullptr);

    if (configured != 0 || swr_init(resampler) != 0)
    {
        swr_free(&resampler);
        return false;
    }

    const int frames = static_cast<int>(bytes / static_cast<std::size_t>(channels * bits / CHAR_BIT));
    const u8* planes[1] = {data};
    AppendConverted(resampler, planes, frames, pcm);
    AppendConverted(resampler, nullptr, 0, pcm);
    swr_free(&resampler);
    return true;
}

}  // namespace swd3
