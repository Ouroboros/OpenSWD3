#include "audio/sound_effects.h"

#include <algorithm>
#include <cstring>

#include "audio/sequence_player.h"

namespace swd3
{

namespace
{

constexpr int kMinRate = 0x2B11;
constexpr int kFullRate = 0xAC44;
constexpr u32 kIndexStart = 0x1C;
constexpr u32 kIndexRecordBytes = 0x2C;
constexpr u32 kSizeMask = 0x3FFFFFF;
constexpr std::string_view kRiffTemplate = "RIFF....WAVEfmt ....0123456789012345data..";
constexpr std::string_view kRiffTag = "RIFF";

// The RIFF header of a sound: its size, the fmt chunk size and data, the data tag and size.
constexpr std::size_t kRiffSizeAt = 4;
constexpr std::size_t kFmtSizeAt = 0x10;
constexpr std::size_t kFmtAt = 0x14;
constexpr u32 kFmtBytes = 0x10;
constexpr std::size_t kMovedFmtBytes = 0x12;
constexpr std::size_t kDataTagEnd = 0x26;  // the last two letters of "data", patched
constexpr std::size_t kDataSizeAt = 0x28;
constexpr u32 kBareFmtPad = 0x18;  // a kind 0 sound is its fmt record, put after the template
constexpr u32 kBareFmtKind = 0;
constexpr u32 kWholeRiffKind = 1;  // a whole RIFF image, left as it is

// An index record: the size (and the kind in kKindMask << kKindShift), the offset, the kind.
constexpr std::size_t kRecordSizeAt = 0x14;
constexpr std::size_t kRecordOffsetAt = 0x18;
constexpr std::size_t kRecordKindAt = 0x20;
constexpr u32 kKindMask = 3;
constexpr int kKindShift = 26;

constexpr int kStereo = 2;
constexpr int kWideSamples = 16;  // bits per sample, kNarrowSamples once the rate is too low
constexpr int kNarrowSamples = 8;
constexpr int kRateStep = 2;  // a failed device is tried again at half the rate
constexpr int kReservedChannels = 8;
constexpr int kMaxVoices = 0x10;

u32 ReadDword(const u8* bytes)
{
    u32 value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

void WriteDword(u8* bytes, u32 value)
{
    std::memcpy(bytes, &value, sizeof(value));
}

}  // namespace

// swd32.exe 0x00485930
SoundEffects::SoundEffects(AudioDevice& device)
    : device(device)
{
}

// swd32.exe 0x00485960
// swd32.exe 0x00498410 (EH unwind: the cFile base, closed by its destructor)
// swd32.exe 0x00498418 (EH handler of 0x00485960)
SoundEffects::~SoundEffects()
{
    this->Close();
}

const std::string& SoundEffects::Error() const
{
    return this->error;
}

// swd32.exe 0x004859B0: all.snd and a 44.1 kHz 16-bit stereo output; while the output refuses, the waveOut path is
// tried, then the rate halves (then 8 bits) down to 11 kHz; an emulated output is replaced by waveOut. Up to 16 sample
// voices (the mixer channels minus 8).
bool SoundEffects::Open(const std::u8string& directory)
{
    if (this->IsOpen())
    {
        return true;
    }

    if (!this->LoadIndex(directory))
    {
        this->error = "SFX file connot open.";
        return false;
    }

    this->channels = kStereo;
    this->rate = kFullRate;
    this->bits = kWideSamples;
    bool waveOut = false;

    while (true)
    {
        if (this->rate < kMinRate)
        {
            this->driver = false;
            return false;
        }

        if (!this->device.OpenOutput())
        {
            this->error = this->device.LastError();

            if (waveOut)
            {
                this->rate /= kRateStep;

                if (this->rate < kMinRate && this->bits == kWideSamples)
                {
                    this->rate = kFullRate;
                    this->bits = kNarrowSamples;
                }
            }
            else
            {
                waveOut = true;
            }

            continue;
        }

        this->configuration = this->device.Configuration();

        if (waveOut || this->configuration.find("Emulated") == std::string::npos)
        {
            break;
        }

        this->device.CloseOutput();
        waveOut = true;
    }

    this->driver = true;
    this->opened = true;
    this->enabled = true;
    this->voices = std::min(AudioDevice::MixerChannels - kReservedChannels, kMaxVoices);
    this->active.clear();

    for (int count = 0; count < this->voices; ++count)
    {
        auto node = std::make_unique<SampleNode>();
        node->voice = this->device.CreateVoice();
        this->device.ResetVoice(node->voice);
        this->free.push_back(std::move(node));
    }

    return true;
}

// swd32.exe 0x00485C20: the open flag stays set
bool SoundEffects::Close()
{
    if (!this->IsOpen())
    {
        return true;
    }

    for (const auto& node : this->active)
    {
        this->device.End(node->voice);
        this->device.ReleaseVoice(node->voice);
    }

    for (const auto& node : this->free)
    {
        this->device.End(node->voice);
        this->device.ReleaseVoice(node->voice);
    }

    this->active.clear();
    this->free.clear();
    this->FreeIndex();
    this->device.CloseOutput();
    return true;
}

// swd32.exe 0x00485CC0
bool SoundEffects::IsOpen() const
{
    return this->opened;
}

// swd32.exe 0x00485CD0
bool SoundEffects::IsEnabled() const
{
    return this->enabled;
}

// swd32.exe 0x00485CE0: sound `id` of all.snd on a free voice. The original also takes a sound image and its size;
// every caller passes none (so a non-RIFF sound is decoded with size 0). Always 0.
int SoundEffects::Play(int id, int volume, int pan, int loops)
{
    if (!this->IsOpen() || !this->IsEnabled() || id == 0)
    {
        return 0;
    }

    std::shared_ptr<std::vector<u8>> image = this->LoadSound(id);

    if (image == nullptr)
    {
        return 0;
    }

    std::unique_ptr<SampleNode> node = this->AllocNode();

    if (node == nullptr)
    {
        return 0;
    }

    this->device.ResetVoice(node->voice);
    const bool riff = std::memcmp(image->data(), kRiffTag.data(), kRiffTag.size()) == 0;
    const bool loaded = riff ? this->device.LoadWave(node->voice, image->data())
                             : this->device.LoadCompressed(node->voice, image->data(), 0);

    if (!loaded)
    {
        this->error = this->device.LastError();
        this->FreeNode(std::move(node));
        return 0;
    }

    SoundEntry& entry = this->table[static_cast<std::size_t>(id - 1)];
    ++entry.users;
    entry.data = image;
    node->id = id;
    this->device.SetVolume(node->voice, ClampVolume(volume));
    this->device.SetPan(node->voice, ClampPan(pan));
    this->device.SetLoops(node->voice, loops);
    this->device.Start(node->voice);
    this->PushActive(std::move(node));
    return 0;
}

// The image of a finished sample is released with its last user.
void SoundEffects::ReleaseSound(const SampleNode& node)
{
    SoundEntry& entry = this->table[static_cast<std::size_t>(node.id - 1)];
    --entry.users;

    if (entry.users == 0)
    {
        entry.data.reset();
    }
}

// swd32.exe 0x00485E90
int SoundEffects::Stop(int id)
{
    if (!this->IsOpen() || !this->IsEnabled() || id == 0)
    {
        return 0;
    }

    std::unique_ptr<SampleNode> node = this->Unlink(id);

    if (node == nullptr)
    {
        return 0;
    }

    this->device.End(node->voice);
    this->ReleaseSound(*node);
    this->FreeNode(std::move(node));
    return 0;
}

// swd32.exe 0x00485F30
bool SoundEffects::StopAll()
{
    if (!this->IsOpen() || !this->IsEnabled())
    {
        return false;
    }

    for (auto& node : this->active)
    {
        this->device.End(node->voice);
        this->ReleaseSound(*node);
        this->free.push_back(std::move(node));
    }

    this->active.clear();
    return true;
}

// swd32.exe 0x00485FE0
int SoundEffects::SetVolume(int id, int volume)
{
    if (!this->IsOpen() || !this->IsEnabled())
    {
        return 0;
    }

    SampleNode* node = this->Find(id);
    const int value = ClampVolume(volume);

    if (node != nullptr)
    {
        this->device.SetVolume(node->voice, value);
    }

    return 0;
}

// swd32.exe 0x00486030: the clamped pan
int SoundEffects::SetPan(int id, int pan)
{
    if (!this->IsOpen() || !this->IsEnabled())
    {
        return 0;
    }

    SampleNode* node = this->Find(id);
    const int value = ClampPan(pan);

    if (node != nullptr)
    {
        this->device.SetPan(node->voice, value);
    }

    return value;
}

// swd32.exe 0x00486080: retires the finished samples
bool SoundEffects::Update()
{
    if (!this->IsOpen() || !this->IsEnabled())
    {
        return false;
    }

    for (auto node = this->active.begin(); node != this->active.end();)
    {
        const auto status = static_cast<u32>(this->device.Status((*node)->voice));

        if (status != 0 && status <= static_cast<u32>(VoiceStatus::Done))
        {
            this->device.SetVolume((*node)->voice, 0);
            this->device.End((*node)->voice);
            this->ReleaseSound(**node);
            this->free.push_back(std::move(*node));
            node = this->active.erase(node);
        }
        else
        {
            ++node;
        }
    }

    return true;
}

// swd32.exe 0x00486160
bool SoundEffects::FreeNode(std::unique_ptr<SampleNode> node)
{
    if (node == nullptr)
    {
        return false;
    }

    this->free.push_back(std::move(node));
    return true;
}

// swd32.exe 0x00486190
std::unique_ptr<SampleNode> SoundEffects::AllocNode()
{
    if (this->free.empty())
    {
        return nullptr;
    }

    std::unique_ptr<SampleNode> node = std::move(this->free.back());
    this->free.pop_back();
    return node;
}

// swd32.exe 0x004861B0
SampleNode* SoundEffects::PushActive(std::unique_ptr<SampleNode> node)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    this->active.push_front(std::move(node));
    return this->active.front().get();
}

// swd32.exe 0x004861D0
SampleNode* SoundEffects::Find(int id)
{
    for (const auto& node : this->active)
    {
        if (node->id == id)
        {
            return node.get();
        }
    }

    return nullptr;
}

// swd32.exe 0x00486210
std::unique_ptr<SampleNode> SoundEffects::Unlink(int id)
{
    for (auto node = this->active.begin(); node != this->active.end(); ++node)
    {
        if ((*node)->id == id)
        {
            std::unique_ptr<SampleNode> found = std::move(*node);
            this->active.erase(node);
            return found;
        }
    }

    return nullptr;
}

// swd32.exe 0x004862A0: whether the digital driver is open
bool SoundEffects::HasDriver() const
{
    return this->driver;
}

// swd32.exe 0x004862B0: the 3000-entry index of <directory>all.snd (opened with OPEN_ALWAYS, so a missing file is
// created empty)
bool SoundEffects::LoadIndex(const std::u8string& directory)
{
    if (!this->file.Open(directory + u8"all.snd", File::Disposition::OpenAlways, File::Access::Read))
    {
        return false;
    }

    this->table.assign(SoundEffects::Sounds, SoundEntry{});
    this->file.SeekBegin(kIndexStart);
    std::vector<u8> index(SoundEffects::Sounds * kIndexRecordBytes, 0);
    u32 bytes = static_cast<u32>(index.size());
    this->file.Read(index.data(), bytes);

    for (int i = 0; i < SoundEffects::Sounds; ++i)
    {
        const u8* record = index.data() + kIndexRecordBytes * i;
        SoundEntry& entry = this->table[static_cast<std::size_t>(i)];
        entry.sizeFlags = (ReadDword(record + kRecordSizeAt) & kSizeMask) |
                          ((ReadDword(record + kRecordKindAt) & kKindMask) << kKindShift);
        entry.offset = ReadDword(record + kRecordOffsetAt);
    }

    return true;
}

// swd32.exe 0x00486430
bool SoundEffects::FreeIndex()
{
    if (this->table.empty())
    {
        return true;
    }

    this->table.clear();
    this->file.Close();
    return true;
}

// swd32.exe 0x00486490: sound `id` as a RIFF image. Kind 0 sounds are a fmt record without the RIFF header, which is
// rebuilt in front of it; every kind but 1 gets the header fields patched (kinds 2 and 3 over their own data). The
// original writes its 44-byte header template past the end of sounds shorter than that; the image is at least that
// long here. Ids beyond the index find no sound.
std::shared_ptr<std::vector<u8>> SoundEffects::LoadSound(int id)
{
    if (id < 1 || id > SoundEffects::Sounds || this->table.empty())
    {
        return nullptr;
    }

    const SoundEntry& entry = this->table[static_cast<std::size_t>(id - 1)];

    if (entry.offset == 0)
    {
        return nullptr;
    }

    this->file.SeekBegin(static_cast<i32>(entry.offset));
    u32 size = entry.sizeFlags & kSizeMask;
    const u32 kind = entry.sizeFlags >> kKindShift;
    const u32 pad = kind == kBareFmtKind ? kBareFmtPad : 0;
    auto image = std::make_shared<std::vector<u8>>(std::max<std::size_t>(size + pad, kRiffTemplate.size() + 1), 0);
    std::memcpy(image->data(), kRiffTemplate.data(), kRiffTemplate.size() + 1);
    this->file.Read(image->data() + pad, size);

    if (kind != kWholeRiffKind)
    {
        std::memmove(image->data() + kFmtAt, image->data() + kBareFmtPad, kMovedFmtBytes);
        WriteDword(image->data() + kFmtSizeAt, kFmtBytes);
        WriteDword(image->data() + kRiffSizeAt, size);
        image->at(kDataTagEnd) = 't';
        image->at(kDataTagEnd + 1) = 'a';
        WriteDword(image->data() + kDataSizeAt, size - kBareFmtPad);
    }

    return image;
}

}  // namespace swd3
