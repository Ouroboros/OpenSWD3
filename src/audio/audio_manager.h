#pragma once

#include <string>

#include "audio/music_queue.h"
#include "audio/sequence_player.h"
#include "audio/sound_effects.h"
#include "core/types.h"

namespace swd3
{

class AudioDevice;

// Miles startup (the object at 0x004A9AC8).
class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    void Startup(const std::u8string& redistDirectory);
    void Shutdown();

private:
    bool started = false;
    std::u8string redistDirectory;
};

// The game's audio: Miles, the MIDI and stream players, the sound effects, the music queue, and the wrappers the game
// calls (volumes 0..11 of the options scaled to 0..128, the music as stream 100 with its fade state).
class AudioManager
{
public:
    static constexpr int MusicId = 100;

    // The modes of `musicFadeMode`.
    enum MusicFade : int
    {
        NoFade = 0,
        StopAtOnce = 1,      // the music stops on the next UpdateMusicFade
        FadeOverLength = 2,  // it fades out over musicFadeLength
    };

    static constexpr int ShortestFade = 1;  // the fade length of StopAtOnce

    explicit AudioManager(AudioDevice& device);

    AudioSystem& System();
    MidiPlayer& Midi();
    StreamPlayer& Streams();
    SoundEffects& Effects();
    MusicQueue& Queue();

    int PlaySound(u16 id, int volume);
    int SetSoundPan(u16 id, int pan);
    int PlaySoundAt(u16 id, u16 volume);
    int StopMusic();
    int StopSound(u16 id);
    int StopAllSounds();
    bool IsMusicStopped();
    int SetMusicVolume(int volume);
    int UpdateMusicFade();
    int PollMusicFade();
    int SetMusicFade(int mode, int length);
    // The fade globals as a new game sets them.
    void SetFadeState(int mode, int length, int fading);
    void SetFadeMode(int mode);
    void SetFadeLength(int length);
    int FadeMode() const;
    int Fading() const;

private:
    AudioSystem system;
    MidiPlayer midi;
    StreamPlayer streams;
    SoundEffects effects;
    MusicQueue queue;
    int musicFadeMode = 0;    // 0x004B7380
    int musicFadeLength = 0;  // 0x004B74F0
    int musicFading = 0;      // 0x004B7378
};

}  // namespace swd3
