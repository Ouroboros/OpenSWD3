#include "platform/audio_device.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>

#include "platform/audio_decoder.h"
#include "platform/file_system.h"

namespace swd3
{

namespace
{

constexpr int kUnityGain = 0x100;  // the mixer gains are fixed point
constexpr int kPanHalf = 63;       // the pan distance at which a side reaches unity gain
constexpr int kMillisecondsPerSecond = 1000;

u32 ReadDword(const u8* bytes)
{
    u32 value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

u16 ReadWord(const u8* bytes)
{
    u16 value = 0;
    std::memcpy(&value, bytes, 2);
    return value;
}

// The PCM data of a RIFF WAVE image (format tag 1 only).
bool LoadRiff(const u8* image, std::vector<i16>& pcm)
{
    if (std::memcmp(image, "RIFF", 4) != 0 || std::memcmp(image + 8, "WAVE", 4) != 0)
    {
        return false;
    }

    const u8* chunk = image + 12;
    const u8* end = image + 8 + ReadDword(image + 4);
    int channels = 0;
    int rate = 0;
    int bits = 0;

    while (chunk + 8 <= end)
    {
        const u32 size = ReadDword(chunk + 4);

        if (std::memcmp(chunk, "fmt ", 4) == 0)
        {
            if (ReadWord(chunk + 8) != 1)
            {
                return false;
            }

            channels = ReadWord(chunk + 10);
            rate = static_cast<int>(ReadDword(chunk + 12));
            bits = ReadWord(chunk + 22);
        }
        else if (std::memcmp(chunk, "data", 4) == 0)
        {
            return ConvertPcm(chunk + 8, size, channels, rate, bits, pcm);
        }

        chunk += 8 + ((size + 1) & ~1u);
    }

    return false;
}

}  // namespace

AudioDevice::~AudioDevice()
{
    this->CloseOutput();
}

bool AudioDevice::OpenOutput()
{
    if (this->output != nullptr)
    {
        return true;
    }

    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        this->lastError = SDL_GetError();
        return false;
    }

    const SDL_AudioSpec spec{SDL_AUDIO_S16, kMixChannels, kMixRate};
    this->output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioDevice::Feed, this);

    if (this->output == nullptr)
    {
        this->lastError = SDL_GetError();
        return false;
    }

    SDL_ResumeAudioStreamDevice(this->output);
    return true;
}

void AudioDevice::CloseOutput()
{
    if (this->output != nullptr)
    {
        SDL_DestroyAudioStream(this->output);
        this->output = nullptr;
    }
}

std::string AudioDevice::Configuration() const
{
    return "SDL3 mixer";
}

const std::string& AudioDevice::LastError() const
{
    return this->lastError;
}

// swd32.exe 0x00485330: the import thunk of AIL_serve
void AudioDevice::Serve()
{
}

VoiceHandle AudioDevice::CreateVoice()
{
    return std::make_shared<AudioVoice>();
}

void AudioDevice::ReleaseVoice(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    this->Detach(voice);
}

void AudioDevice::ResetVoice(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    this->Detach(voice);
    *voice = AudioVoice{};
}

bool AudioDevice::LoadWave(const VoiceHandle& voice, const u8* image)
{
    auto pcm = std::make_shared<std::vector<i16>>();

    if (!LoadRiff(image, *pcm))
    {
        this->lastError = "Unsupported sample file";
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->pcm = pcm;
    voice->frame = 0;
    return true;
}

bool AudioDevice::LoadCompressed(const VoiceHandle& voice, const u8* data, std::size_t size)
{
    auto pcm = std::make_shared<std::vector<i16>>();

    if (!DecodeAudio(data, size, *pcm))
    {
        this->lastError = "Cannot decode sample file";
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->pcm = pcm;
    voice->frame = 0;
    return true;
}

VoiceHandle AudioDevice::OpenStream(const std::string& path)
{
    std::ifstream file(ResolvePath(path), std::ios::binary);

    if (!file)
    {
        this->lastError = "Cannot open stream file";
        return nullptr;
    }

    const std::vector<u8> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    auto pcm = std::make_shared<std::vector<i16>>();

    if (!DecodeAudio(data.data(), data.size(), *pcm))
    {
        this->lastError = "Cannot decode stream file";
        return nullptr;
    }

    VoiceHandle voice = this->CreateVoice();
    voice->pcm = pcm;
    return voice;
}

void AudioDevice::Start(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    if (voice->pcm == nullptr)
    {
        return;
    }

    voice->frame = 0;
    voice->status = VoiceStatus::Playing;
    this->Attach(voice);
}

void AudioDevice::End(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->status = VoiceStatus::Done;
    this->Detach(voice);
}

VoiceStatus AudioDevice::Status(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    return voice->status;
}

void AudioDevice::SetVolume(const VoiceHandle& voice, int volume)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->volume = std::clamp(volume, 0, AudioVoice::MaxLevel);
}

int AudioDevice::Volume(const VoiceHandle& voice)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    return voice->volume;
}

void AudioDevice::SetPan(const VoiceHandle& voice, int pan)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->pan = std::clamp(pan, 0, AudioVoice::MaxLevel);
}

void AudioDevice::SetLoops(const VoiceHandle& voice, int loops)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    voice->loops = loops;
}

void AudioDevice::Position(const VoiceHandle& voice, int& totalMilliseconds, int& currentMilliseconds)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    const std::size_t frames = voice->pcm != nullptr ? voice->pcm->size() / kMixChannels : 0;
    totalMilliseconds = static_cast<int>(frames * kMillisecondsPerSecond / kMixRate);
    currentMilliseconds = static_cast<int>(voice->frame * kMillisecondsPerSecond / kMixRate);
}

bool AudioDevice::OpenMidi()
{
    this->lastError = "No MIDI output device";
    return false;
}

void AudioDevice::Attach(const VoiceHandle& voice)
{
    if (std::find(this->playing.begin(), this->playing.end(), voice) == this->playing.end())
    {
        this->playing.push_back(voice);
    }
}

void AudioDevice::Detach(const VoiceHandle& voice)
{
    this->playing.erase(std::remove(this->playing.begin(), this->playing.end(), voice), this->playing.end());
}

void AudioDevice::Mix(std::vector<i16>& out, int frames)
{
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    this->mix.assign(static_cast<std::size_t>(frames) * kMixChannels, 0);

    for (const VoiceHandle& voice : this->playing)
    {
        if (voice->status != VoiceStatus::Playing || voice->pcm == nullptr)
        {
            continue;
        }

        const std::vector<i16>& pcm = *voice->pcm;
        const std::size_t total = pcm.size() / kMixChannels;
        const i32 left = std::min<i32>(kUnityGain, (AudioVoice::MaxLevel - voice->pan) * kUnityGain / kPanHalf);
        const i32 right = std::min<i32>(kUnityGain, voice->pan * kUnityGain / kPanHalf);
        const i32 gain = voice->volume * kUnityGain / AudioVoice::MaxLevel;

        for (int i = 0; i < frames; ++i)
        {
            if (voice->frame >= total)
            {
                if (voice->loops == 1 || total == 0)
                {
                    voice->status = VoiceStatus::Done;
                    break;
                }

                if (voice->loops > 1)
                {
                    --voice->loops;
                }

                voice->frame = 0;
            }

            this->mix[static_cast<std::size_t>(i) * kMixChannels] +=
                pcm[voice->frame * kMixChannels] * gain / kUnityGain * left / kUnityGain;
            this->mix[static_cast<std::size_t>(i) * kMixChannels + 1] +=
                pcm[voice->frame * kMixChannels + 1] * gain / kUnityGain * right / kUnityGain;
            ++voice->frame;
        }
    }

    out.resize(this->mix.size());

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        out[i] = static_cast<i16>(
            std::clamp<i32>(this->mix[i], std::numeric_limits<i16>::min(), std::numeric_limits<i16>::max())
        );
    }
}

void SDLCALL AudioDevice::Feed(void* user, SDL_AudioStream* stream, int additional, int)
{
    if (additional <= 0)
    {
        return;
    }

    auto* device = static_cast<AudioDevice*>(user);
    std::vector<i16> out;
    device->Mix(out, additional / static_cast<int>(kMixChannels * sizeof(i16)));
    SDL_PutAudioStreamData(stream, out.data(), static_cast<int>(out.size() * sizeof(i16)));
}

}  // namespace swd3
