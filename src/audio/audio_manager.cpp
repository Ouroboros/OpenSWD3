#include "audio/audio_manager.h"

namespace swd3
{

namespace
{

// The options' volume 0..11 as a Miles volume.
int ScaledVolume(int volume)
{
    return (volume << 7) / 11;
}

}  // namespace

// swd32.exe 0x00485340
AudioSystem::AudioSystem() = default;

// swd32.exe 0x00485350
AudioSystem::~AudioSystem()
{
    this->Shutdown();
}

// swd32.exe 0x00485360: the redist directory and Miles startup (the original also loads MSS32.DLL to read a string it
// does not use)
void AudioSystem::Startup(const std::string& redistDirectory)
{
    this->redistDirectory = redistDirectory;
    this->started = true;
}

// swd32.exe 0x004853C0
void AudioSystem::Shutdown()
{
    this->started = false;
}

AudioManager::AudioManager(AudioDevice& device)
    : midi(device),
      streams(device),
      effects(device),
      queue(this->streams, this->midi)
{
}

AudioSystem& AudioManager::System()
{
    return this->system;
}

MidiPlayer& AudioManager::Midi()
{
    return this->midi;
}

StreamPlayer& AudioManager::Streams()
{
    return this->streams;
}

SoundEffects& AudioManager::Effects()
{
    return this->effects;
}

MusicQueue& AudioManager::Queue()
{
    return this->queue;
}

// swd32.exe 0x00485610: sound `id` once, centred, at the options' volume 0..11
int AudioManager::PlaySound(u16 id, int volume)
{
    return this->effects.Play(id, ScaledVolume(volume), 0, 1);
}

// swd32.exe 0x00485650
int AudioManager::SetSoundPan(u16 id, int pan)
{
    return this->effects.SetPan(id, pan);
}

// swd32.exe 0x00485670
int AudioManager::PlaySoundAt(u16 id, u16 volume)
{
    this->effects.Play(id, ScaledVolume(volume), 0, 1);
    return 1;
}

// swd32.exe 0x00485710: fades the music out in one update
int AudioManager::StopMusic()
{
    return this->streams.FadeOut(AudioManager::MusicId, 1);
}

// swd32.exe 0x00485720
int AudioManager::StopSound(u16 id)
{
    this->effects.Stop(id);
    return 1;
}

// swd32.exe 0x00485740
int AudioManager::StopAllSounds()
{
    this->effects.StopAll();
    return 1;
}

// swd32.exe 0x00485830
bool AudioManager::IsMusicStopped()
{
    return this->streams.IsStopped(AudioManager::MusicId);
}

// swd32.exe 0x00485850
int AudioManager::SetMusicVolume(int volume)
{
    return this->streams.SetVolume(AudioManager::MusicId, ScaledVolume(volume));
}

// swd32.exe 0x00485880: mode 1 stops the music at once, mode 2 fades it over the set length
int AudioManager::UpdateMusicFade()
{
    if (this->musicFadeMode == 1)
    {
        this->streams.FadeOut(AudioManager::MusicId, 1);
        this->musicFadeMode = 0;
        this->musicFading = 0;
        return 0;
    }

    if (this->musicFadeMode == 2)
    {
        const int result = this->streams.FadeOut(AudioManager::MusicId, this->musicFadeLength);
        this->musicFading = this->musicFadeLength;
        return result;
    }

    return this->musicFadeMode - 2;
}

// swd32.exe 0x004858D0: whether a mode 2 fade has finished (which ends it)
int AudioManager::PollMusicFade()
{
    if (this->musicFadeMode == 0)
    {
        return 0;
    }

    if (this->musicFadeMode != 2)
    {
        return this->musicFadeMode - 2;
    }

    const bool stopped = this->streams.IsStopped(AudioManager::MusicId);

    if (stopped)
    {
        this->musicFadeMode = 0;
        this->musicFading = 0;
    }

    return stopped ? 1 : 0;
}

// swd32.exe 0x00485910
int AudioManager::SetMusicFade(int mode, int length)
{
    this->musicFadeMode = mode;

    if (mode == 2)
    {
        this->musicFadeLength = length;
        return length;
    }

    return mode;
}

void AudioManager::SetFadeState(int mode, int length, int fading)
{
    this->musicFadeMode = mode;
    this->musicFadeLength = length;
    this->musicFading = fading;
}

void AudioManager::SetFadeMode(int mode)
{
    this->musicFadeMode = mode;
}

void AudioManager::SetFadeLength(int length)
{
    this->musicFadeLength = length;
}

int AudioManager::FadeMode() const
{
    return this->musicFadeMode;
}

int AudioManager::Fading() const
{
    return this->musicFading;
}

}  // namespace swd3
