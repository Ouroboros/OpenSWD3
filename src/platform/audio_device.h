#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/types.h"

struct SDL_AudioStream;

namespace swd3
{

// Status of a voice, with the values of Miles' SMP_* constants.
enum class VoiceStatus : u32
{
    Free = 1,
    Done = 2,
    Playing = 4,
    Stopped = 8,
    PlayingButReleased = 16,
};

// One sound of the mixer: a Miles sample (a sound image) or stream (a music file), decoded to the mixer format.
struct AudioVoice
{
    std::shared_ptr<const std::vector<i16>> pcm;
    std::size_t frame = 0;
    int loops = 1;  // 0: forever
    int volume = 127;
    int pan = 64;
    VoiceStatus status = VoiceStatus::Done;
};

using VoiceHandle = std::shared_ptr<AudioVoice>;

// The digital audio output (what the game reaches through Miles: a waveOut driver, samples, streams; there is no MIDI
// device). Voices are mixed on SDL's audio thread.
class AudioDevice
{
public:
    // Mixer channels Miles reports (DIG_MIXER_CHANNELS); the game uses this minus 8, at most 16.
    static constexpr int kMixerChannels = 64;

    AudioDevice() = default;
    ~AudioDevice();
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    bool OpenOutput();
    void CloseOutput();
    std::string Configuration() const;
    const std::string& LastError() const;
    // AIL_serve: the mixer runs on its own thread, there is nothing to serve.
    void Serve();

    VoiceHandle CreateVoice();
    void ReleaseVoice(const VoiceHandle& voice);
    void ResetVoice(const VoiceHandle& voice);
    bool LoadWave(const VoiceHandle& voice, const u8* image);
    bool LoadCompressed(const VoiceHandle& voice, const u8* data, std::size_t size);
    VoiceHandle OpenStream(const std::string& path);
    void Start(const VoiceHandle& voice);
    void End(const VoiceHandle& voice);
    VoiceStatus Status(const VoiceHandle& voice);
    void SetVolume(const VoiceHandle& voice, int volume);
    int Volume(const VoiceHandle& voice);
    void SetPan(const VoiceHandle& voice, int pan);
    void SetLoops(const VoiceHandle& voice, int loops);
    void Position(const VoiceHandle& voice, int& totalMilliseconds, int& currentMilliseconds);
    bool OpenMidi();

    // Mixes `frames` frames of the playing voices (the audio thread; public for tests).
    void Mix(std::vector<i16>& out, int frames);

private:
    static void Feed(void* user, SDL_AudioStream* stream, int additional, int total);
    void Attach(const VoiceHandle& voice);
    void Detach(const VoiceHandle& voice);

    std::recursive_mutex mutex;
    SDL_AudioStream* output = nullptr;
    std::vector<VoiceHandle> playing;
    std::vector<i32> mix;
    std::string lastError;
};

}  // namespace swd3
