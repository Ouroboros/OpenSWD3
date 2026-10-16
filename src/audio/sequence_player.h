#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"
#include "platform/audio_device.h"

namespace swd3
{

// A playing (or free) piece of a SequencePlayer: the Miles handle, the MIDI image or stream file name, the volume in
// 1/16 steps and the fade step removed per update while fading.
struct SequenceNode
{
    VoiceHandle voice;
    std::vector<u8> midi;
    std::string name;
    int id = 0;  // the Miles user data the players look nodes up by
    i32 fadeStep = 0;
    i32 volume = 0;
    u32 flags = 0;  // Fading
};

int ClampVolume(int volume);
int ClampPan(int pan);
int SetFadeStep(SequenceNode& node, int steps);

// Shared state of the MIDI player (0x004B7EB8) and the music stream player (0x004C9288): the driver, the open and
// enabled flags, an error text and the active and free node lists.
class SequencePlayer
{
public:
    static constexpr u32 Fading = 0x80000000u;

    explicit SequencePlayer(AudioDevice& device);
    virtual ~SequencePlayer();

    bool IsOpen() const;
    bool IsEnabled() const;
    std::unique_ptr<SequenceNode> AllocNode();
    SequenceNode* PushActive(std::unique_ptr<SequenceNode> node);
    bool FreeNode(std::unique_ptr<SequenceNode> node);
    const std::string& Error() const;

protected:
    AudioDevice& device;
    bool opened = false;
    bool enabled = false;
    i32 unknown4 = 0x1E;
    std::string error;
    std::list<std::unique_ptr<SequenceNode>> active;
    std::vector<std::unique_ptr<SequenceNode>> free;  // the last one is the head of the original's free list
};

// MIDI music through the MIDI mapper (the platform has no MIDI output, so opening fails as on a system without one).
class MidiPlayer : public SequencePlayer
{
public:
    explicit MidiPlayer(AudioDevice& device);
    ~MidiPlayer() override;

    bool Open(int unused);
    int Close();
    int Play(const std::string& fileName, int id, int volume, int loops);
    bool Update();
    bool IsStopped(int id);
    SequenceNode* Find(int id);
};

// Streamed music (MP3 files of Music\) with two slots.
class StreamPlayer : public SequencePlayer
{
public:
    explicit StreamPlayer(AudioDevice& device);
    ~StreamPlayer() override;

    bool Open();
    int Close();
    int SetVolume(int id, int volume);
    int Play(const std::string& fileName, int id, int volume, int loops);
    int FadeOut(int id, int steps);
    bool Update();
    bool IsStopped(int id);
    SequenceNode* Find(int id);
};

}  // namespace swd3
