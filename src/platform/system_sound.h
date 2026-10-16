#pragma once

#include <span>

#include "core/types.h"

struct SDL_AudioStream;

namespace swd3
{

// PlaySoundA with SND_MEMORY | SND_ASYNC: a WAV image played on a stream of its own (outside the Miles voices); a new
// sound stops the one playing.
class SystemSound
{
public:
    SystemSound() = default;
    ~SystemSound();
    SystemSound(const SystemSound&) = delete;
    SystemSound& operator=(const SystemSound&) = delete;

    bool Play(std::span<const u8> wave);
    void Stop();

private:
    SDL_AudioStream* stream = nullptr;
    bool startedAudio = false;
};

}  // namespace swd3
