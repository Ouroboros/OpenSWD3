#include "platform/system_sound.h"

#include <SDL3/SDL.h>

namespace swd3
{

SystemSound::~SystemSound()
{
    this->Stop();

    if (this->startedAudio)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

bool SystemSound::Play(std::span<const u8> wave)
{
    this->Stop();

    if (wave.empty())
    {
        return false;
    }

    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            return false;
        }

        this->startedAudio = true;
    }

    SDL_AudioSpec spec{};
    Uint8* samples = nullptr;
    Uint32 length = 0;

    if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(wave.data(), wave.size()), true, &spec, &samples, &length))
    {
        return false;
    }

    this->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    const bool played = this->stream != nullptr &&
                        SDL_PutAudioStreamData(this->stream, samples, static_cast<int>(length)) &&
                        SDL_FlushAudioStream(this->stream) && SDL_ResumeAudioStreamDevice(this->stream);
    SDL_free(samples);
    return played;
}

void SystemSound::Stop()
{
    if (this->stream != nullptr)
    {
        SDL_DestroyAudioStream(this->stream);
        this->stream = nullptr;
    }
}

}  // namespace swd3
