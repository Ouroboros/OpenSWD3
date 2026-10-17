#include "platform/system_sound.h"

#include <SDL3/SDL.h>

#include "core/log.h"

namespace swd3
{

namespace
{

constexpr Uint64 kSlowPlayMilliseconds = 20;  // a Play this long is logged (it holds up the launcher's messages)

}  // namespace

// The audio subsystem stays for the Miles device; SDL_Quit ends it.
SystemSound::~SystemSound()
{
    if (this->stream != nullptr)
    {
        SDL_DestroyAudioStream(this->stream);
    }
}

// The device stream is opened by the first sound and kept: opening and closing the audio device for every sound (the
// launcher plays one each time the mouse lights a button) blocks the window's messages.
bool SystemSound::Play(std::span<const u8> wave)
{
    const Uint64 started = SDL_GetTicks();
    this->Stop();

    if (wave.empty())
    {
        return false;
    }

    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        return false;
    }

    SDL_AudioSpec spec{};
    Uint8* samples = nullptr;
    Uint32 length = 0;

    if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(wave.data(), wave.size()), true, &spec, &samples, &length))
    {
        return false;
    }

    if (this->stream == nullptr)
    {
        this->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    }
    else if (!SDL_SetAudioStreamFormat(this->stream, &spec, nullptr))
    {
        SDL_free(samples);
        return false;
    }

    const bool played = this->stream != nullptr &&
                        SDL_PutAudioStreamData(this->stream, samples, static_cast<int>(length)) &&
                        SDL_FlushAudioStream(this->stream) && SDL_ResumeAudioStreamDevice(this->stream);
    SDL_free(samples);
    const Uint64 spent = SDL_GetTicks() - started;

    if (spent >= kSlowPlayMilliseconds)
    {
        LogDebug(u8"system sound took %d ms (played=%d)", static_cast<int>(spent), played ? 1 : 0);
    }

    return played;
}

// The sound playing cut off; the device stream stays open for the next.
void SystemSound::Stop()
{
    if (this->stream != nullptr)
    {
        SDL_ClearAudioStream(this->stream);
    }
}

}  // namespace swd3
