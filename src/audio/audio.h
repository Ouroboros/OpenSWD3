#pragma once

#include <cstdint>

#include "file/file.h"
#include "swd3/mss.h"

namespace swd3 {

// Miles startup (global object at 0x004A9AC8).
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    void Startup(const char* redistDirectory);
    void Shutdown();

    std::int32_t m_started;
};

static_assert(sizeof(AudioSystem) == 4);

// Playing / free entry of a SequencePlayer (0x18 bytes).
struct SequenceNode {
    void* handle;            // +0x00 HSEQUENCE or HSTREAM
    char* data;              // +0x04 MIDI image or stream file name
    std::int32_t fadeStep;   // +0x08 volume * 16 removed per update while fading
    std::int32_t volume;     // +0x0C volume * 16
    std::uint32_t flags;     // +0x10 bit 31: fading out
    SequenceNode* next;      // +0x14
};

// Shared layout of the MIDI player (0x004B7EB8) and the music stream player (0x004C9288), size 0x440. The
// update loops treat the 0x18 bytes at +0x410 as a node whose `next` is m_active.
class SequencePlayer {
public:
    SequencePlayer();
    bool IsOpen();
    bool IsEnabled();
    SequenceNode* AllocNode();
    SequenceNode* PushActive(SequenceNode* node);
    bool FreeNode(SequenceNode* node);

    void* m_driver;                  // +0x000 HMDIDRIVER / HDIGDRIVER
    std::int32_t m_unknown4;         // +0x004 0x1E
    std::int32_t m_opened;           // +0x008
    std::int32_t m_enabled;          // +0x00C
    char m_error[0x400];             // +0x010
    std::uint8_t m_sentinel[0x14];   // +0x410
    SequenceNode* m_active;          // +0x424
    std::uint8_t m_pad428[0x14];     // +0x428
    SequenceNode* m_free;            // +0x43C
};

static_assert(sizeof(SequencePlayer) == 0x440);

class MidiPlayer : public SequencePlayer {
public:
    ~MidiPlayer();
    bool Open(int unused);
    int Close();
    int Play(const char* fileName, int id, int volume, int loops);
    bool Update();
    bool IsStopped(int id);
    SequenceNode* Find(int id);
};

class StreamPlayer : public SequencePlayer {
public:
    ~StreamPlayer();
    bool Open(mss::HDIGDRIVER driver);
    int Close();
    int SetVolume(int id, int volume);
    int Play(const char* fileName, int id, int volume, int loops);
    int FadeOut(int id, int steps);
    bool Update();
    bool IsStopped(int id);
    SequenceNode* Find(int id);
};

static_assert(sizeof(MidiPlayer) == 0x440 && sizeof(StreamPlayer) == 0x440);

int SetFadeStep(SequenceNode* node, int steps);
int ClampVolume(int volume);
int ClampPan(int pan);

// Sample node of SoundEffects (8 bytes).
struct SampleNode {
    mss::HSAMPLE sample;
    SampleNode* next;
};

// all.snd index entry kept by SoundEffects (16 bytes).
struct SoundEntry {
    std::uint32_t offset;     // 0: no sound
    std::uint32_t sizeFlags;  // bits 0-25 size, 26-27 header kind (0: fmt record without RIFF header)
    std::int32_t users;       // playing samples using `data`
    std::uint8_t* data;       // loaded RIFF image
};

// Sound effects from all.snd (global object at 0x004C8450, size 0x680), derived from File.
class SoundEffects : public File {
public:
    SoundEffects();
    ~SoundEffects();
    bool Open(const char* directory);
    bool Close();
    bool IsOpen();
    bool IsEnabled();
    int Play(std::uint8_t* data, int id, int volume, int pan, int loops, int size);
    int Stop(int id);
    bool StopAll();
    int SetVolume(int id, int volume);
    int SetPan(int id, int pan);
    bool Update();
    bool FreeNode(SampleNode* node);
    SampleNode* AllocNode();
    SampleNode* PushActive(SampleNode* node);
    SampleNode* Find(int id);
    SampleNode* Unlink(int id);
    mss::HDIGDRIVER GetDriver();
    bool LoadIndex(const char* directory);
    bool FreeIndex();
    std::uint8_t* LoadSound(int id);

    mss::HDIGDRIVER m_driver;       // +0x050
    std::int32_t m_opened;          // +0x054
    std::int32_t m_enabled;         // +0x058
    std::int32_t m_voices;          // +0x05C
    std::int32_t m_rate;            // +0x060
    std::int32_t m_bits;            // +0x064
    std::int32_t m_channels;        // +0x068
    char m_config[0x200];           // +0x06C
    char m_error[0x400];            // +0x26C
    std::uint32_t m_sentinel;       // +0x66C node whose `next` is m_active
    SampleNode* m_active;           // +0x670
    std::uint32_t m_pad674;         // +0x674
    SampleNode* m_free;             // +0x678
    SoundEntry* m_table;            // +0x67C 3000 entries
};

static_assert(sizeof(SoundEffects) == 0x680);

// Music queue entry (20 bytes).
struct MusicSlot {
    char* name;
    std::int32_t id;
    std::int32_t type;  // 1: stream, 2: MIDI
    std::int32_t reserved[2];
};

// Alternating music playlists (global object at 0x004BA6E8).
class MusicQueue {
public:
    MusicQueue();
    ~MusicQueue();
    int ClearSlots(int kind);
    int Update();

    std::int32_t m_unknown0;     // +0x000 0x1E
    std::int32_t m_unknown4;     // +0x004
    std::int32_t m_unknown8;     // +0x008
    std::int32_t m_mode;         // +0x00C 1: list 1, 2: list 2
    std::int32_t m_pending;      // +0x010 mode to switch to (3: none)
    std::int32_t m_index2;       // +0x014
    std::int32_t m_loop2;        // +0x018
    std::int32_t m_index1;       // +0x01C
    std::int32_t m_loop1;        // +0x020
    std::int32_t m_volume;       // +0x024
    std::uint8_t m_pad28[0x400];
    MusicSlot m_current;         // +0x428
    MusicSlot m_list2[2];        // +0x43C
    MusicSlot m_list1[2];        // +0x464
};

static_assert(sizeof(MusicQueue) == 0x48C);

int PlaySound(std::uint16_t id, int volume);
int SetSoundPan(std::uint16_t id, int pan);
int PlaySoundAt(std::uint16_t id, std::uint16_t volume);
int StopMusic();
int StopSound(std::uint16_t id);
int StopAllSounds();
bool IsMusicStopped();
int SetMusicVolume(int volume);
int UpdateMusicFade();
int PollMusicFade();
int SetMusicFade(int mode, int length);

}  // namespace swd3
