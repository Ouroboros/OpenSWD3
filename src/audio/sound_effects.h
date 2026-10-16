#pragma once

#include <array>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"
#include "platform/audio_device.h"

namespace swd3
{

// A sample voice of SoundEffects with the sound id it plays (Miles user data 0 in the original).
struct SampleNode
{
    VoiceHandle voice;
    int id = 0;
};

// An all.snd index entry: the sound image is shared by the samples playing it.
struct SoundEntry
{
    u32 offset = 0;     // 0: no sound
    u32 sizeFlags = 0;  // bits 0-25 the size, 26-27 the header kind (0: a fmt record without the RIFF header)
    i32 users = 0;      // playing samples of `data`
    std::shared_ptr<std::vector<u8>> data;
};

// Sound effects of all.snd (the object at 0x004C8450; the original derives it from cFile): the digital output, up to
// 16 sample voices, the 3000-entry index and the loaded sound images.
class SoundEffects
{
public:
    static constexpr int kSounds = 3000;

    explicit SoundEffects(AudioDevice& device);
    ~SoundEffects();
    SoundEffects(const SoundEffects&) = delete;
    SoundEffects& operator=(const SoundEffects&) = delete;

    bool Open(const std::string& directory);
    bool Close();
    bool IsOpen() const;
    bool IsEnabled() const;
    int Play(int id, int volume, int pan, int loops);
    int Stop(int id);
    bool StopAll();
    int SetVolume(int id, int volume);
    int SetPan(int id, int pan);
    bool Update();
    bool FreeNode(std::unique_ptr<SampleNode> node);
    std::unique_ptr<SampleNode> AllocNode();
    SampleNode* PushActive(std::unique_ptr<SampleNode> node);
    SampleNode* Find(int id);
    std::unique_ptr<SampleNode> Unlink(int id);
    bool HasDriver() const;
    bool LoadIndex(const std::string& directory);
    bool FreeIndex();
    std::shared_ptr<std::vector<u8>> LoadSound(int id);
    const std::string& Error() const;

private:
    void ReleaseSound(const SampleNode& node);

    AudioDevice& device;
    File file;
    bool driver = false;
    bool opened = false;
    bool enabled = false;
    int voices = 0;
    int rate = 0;
    int bits = 0;
    int channels = 0;
    std::string configuration;
    std::string error;
    std::list<std::unique_ptr<SampleNode>> active;
    std::vector<std::unique_ptr<SampleNode>> free;  // the last one is the head of the original's free list
    std::vector<SoundEntry> table;
};

}  // namespace swd3
