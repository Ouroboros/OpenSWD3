#include "audio/sequence_player.h"

#include "file/file.h"

namespace swd3
{

namespace
{

constexpr int kStreamNodes = 2;

}  // namespace

// swd32.exe 0x00486260
int ClampVolume(int volume)
{
    if (volume > SequenceNode::MaxLevel)
    {
        return SequenceNode::MaxLevel;
    }

    if (volume < 0)
    {
        return 0;
    }

    return volume;
}

// swd32.exe 0x00486280: the pan -63..64 to the Miles range 0..127
int ClampPan(int pan)
{
    const int value = pan + SequenceNode::CenterPan;

    if (value > SequenceNode::MaxLevel)
    {
        return SequenceNode::MaxLevel;
    }

    if (value < 0)
    {
        return 0;
    }

    return value;
}

// swd32.exe 0x004868E0: the volume removed per update to fade out in `steps` updates (at least 1). The original
// divides by zero for 0 steps; it fades by 1 here.
int SetFadeStep(SequenceNode& node, int steps)
{
    const int step = steps != 0 ? node.volume / steps : 0;
    node.fadeStep = step;

    if (step == 0)
    {
        node.fadeStep = 1;
    }

    return step;
}

// swd32.exe 0x00486580
SequencePlayer::SequencePlayer(AudioDevice& device)
    : device(device)
{
}

SequencePlayer::~SequencePlayer() = default;

// swd32.exe 0x004866A0
bool SequencePlayer::IsOpen() const
{
    return this->opened;
}

// swd32.exe 0x004866B0
bool SequencePlayer::IsEnabled() const
{
    return this->enabled;
}

const std::string& SequencePlayer::Error() const
{
    return this->error;
}

// swd32.exe 0x00486A30: the head of the free list
std::unique_ptr<SequenceNode> SequencePlayer::AllocNode()
{
    if (this->free.empty())
    {
        return nullptr;
    }

    std::unique_ptr<SequenceNode> node = std::move(this->free.back());
    this->free.pop_back();
    return node;
}

// swd32.exe 0x00486A50: the node becomes the head of the active list
SequenceNode* SequencePlayer::PushActive(std::unique_ptr<SequenceNode> node)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    this->active.push_front(std::move(node));
    return this->active.front().get();
}

// swd32.exe 0x004852C0: the node becomes the head of the free list
bool SequencePlayer::FreeNode(std::unique_ptr<SequenceNode> node)
{
    if (node == nullptr)
    {
        return false;
    }

    this->free.push_back(std::move(node));
    return true;
}

MidiPlayer::MidiPlayer(AudioDevice& device)
    : SequencePlayer(device)
{
}

// swd32.exe 0x00484DC0
MidiPlayer::~MidiPlayer()
{
    this->Close();
}

// swd32.exe 0x00484DD0: the MIDI mapper, then device 0; one sequence handle
bool MidiPlayer::Open(int)
{
    if (!this->device.OpenMidi())
    {
        this->error = this->device.LastError();

        if (!this->device.OpenMidi())
        {
            this->error += " //";
            this->error += this->device.LastError();
            this->enabled = false;
            return false;
        }
    }

    this->opened = true;
    this->enabled = true;
    auto node = std::make_unique<SequenceNode>();
    node->voice = this->device.CreateVoice();
    this->free.push_back(std::move(node));
    return true;
}

// swd32.exe 0x00484ED0
int MidiPlayer::Close()
{
    if (!this->IsOpen())
    {
        return 0;
    }

    for (const auto& node : this->active)
    {
        this->device.End(node->voice);
        this->device.ReleaseVoice(node->voice);
    }

    this->active.clear();

    for (const auto& node : this->free)
    {
        this->device.ReleaseVoice(node->voice);
    }

    this->free.clear();
    this->opened = false;
    this->enabled = false;
    return 1;
}

// swd32.exe 0x00484F60: loads a MIDI file and starts it as sequence `id`. A file that cannot be opened loses the node;
// an initialization result of -1 is reported but played anyway. (Without a MIDI device the player never opens.)
// swd32.exe 0x004983F0 (EH unwind: frees the node allocated for the sequence, held by its unique_ptr)
// swd32.exe 0x004983FB (EH handler of 0x00484F60)
int MidiPlayer::Play(const std::u8string& fileName, int id, int volume, int loops)
{
    if (!this->IsOpen() || !this->IsEnabled() || id == 0)
    {
        return 0;
    }

    std::unique_ptr<SequenceNode> node = this->AllocNode();

    if (node == nullptr)
    {
        return 0;
    }

    node->midi.clear();
    node->fadeStep = 0;
    File file;

    if (!file.Open(fileName, File::Disposition::OpenAlways, File::Access::Read))
    {
        this->error = this->device.LastError();
        return 0;
    }

    u32 size = file.Size();
    node->midi.assign(size, 0);
    file.Read(node->midi.data(), size);
    file.Close();

    if (!this->device.LoadCompressed(node->voice, node->midi.data(), node->midi.size()))
    {
        this->error = this->device.LastError();
        node->midi.clear();
        this->FreeNode(std::move(node));
        return 0;
    }

    node->volume = SequenceNode::VolumeScale * ClampVolume(volume);
    node->id = id;
    this->device.SetVolume(node->voice, node->volume / SequenceNode::VolumeScale);
    this->device.SetLoops(node->voice, loops);
    this->device.Start(node->voice);
    this->PushActive(std::move(node));
    return id;
}

// swd32.exe 0x00485180: fades and retires sequences. A sequence in an unexpected state is retired when the one before
// it was; the original seeds that flag with an uninitialised stack slot, here it starts false (see deviations.md).
bool MidiPlayer::Update()
{
    if (!this->IsOpen() || !this->IsEnabled())
    {
        return false;
    }

    bool retiredLast = false;

    for (auto node = this->active.begin(); node != this->active.end();)
    {
        bool retire = false;
        SequenceNode& sequence = **node;

        if ((sequence.flags & SequencePlayer::Fading) == 0)
        {
            switch (this->device.Status(sequence.voice))
            {
                case VoiceStatus::Free:
                case VoiceStatus::Done:
                    retire = true;
                    break;

                case VoiceStatus::Playing:
                case VoiceStatus::Stopped:
                case VoiceStatus::PlayingButReleased:
                    retiredLast = false;
                    break;

                default:
                    retire = retiredLast;
                    break;
            }
        }
        else
        {
            sequence.volume -= sequence.fadeStep;
            const int volume = ClampVolume(sequence.volume / SequenceNode::VolumeScale);

            if (volume != 0)
            {
                this->device.SetVolume(sequence.voice, volume);
                retiredLast = false;
            }
            else
            {
                retire = true;
            }
        }

        if (!retire)
        {
            ++node;
            continue;
        }

        retiredLast = true;
        this->device.End(sequence.voice);
        sequence.midi.clear();
        sequence.fadeStep = 0;
        sequence.flags = 0;
        this->free.push_back(std::move(*node));
        node = this->active.erase(node);
    }

    return false;
}

// swd32.exe 0x00485290: without an id, whether nothing plays
bool MidiPlayer::IsStopped(int id)
{
    if (id != 0)
    {
        return this->Find(id) == nullptr;
    }

    return this->active.empty();
}

// swd32.exe 0x004852F0
SequenceNode* MidiPlayer::Find(int id)
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

StreamPlayer::StreamPlayer(AudioDevice& device)
    : SequencePlayer(device)
{
}

// swd32.exe 0x004865A0
StreamPlayer::~StreamPlayer()
{
    this->Close();
}

// swd32.exe 0x004865B0: two stream slots on the digital driver of the sound effects
bool StreamPlayer::Open()
{
    this->opened = true;
    this->enabled = true;

    for (int i = 0; i < kStreamNodes; ++i)
    {
        this->free.push_back(std::make_unique<SequenceNode>());
    }

    return true;
}

// swd32.exe 0x00486610
int StreamPlayer::Close()
{
    if (!this->IsOpen())
    {
        return 0;
    }

    for (const auto& node : this->active)
    {
        if (node->voice != nullptr)
        {
            this->device.End(node->voice);
        }
    }

    this->active.clear();
    this->free.clear();
    this->opened = false;
    this->enabled = false;
    return 1;
}

// swd32.exe 0x004866C0: the volume Miles reports back; -1 for an unknown id
int StreamPlayer::SetVolume(int id, int volume)
{
    if (!this->IsOpen())
    {
        return 0;
    }

    SequenceNode* node = this->Find(id);

    if (node == nullptr)
    {
        return -1;
    }

    node->volume = SequenceNode::VolumeScale * ClampVolume(volume);
    this->device.SetVolume(node->voice, ClampVolume(volume));
    return this->device.Volume(node->voice);
}

// swd32.exe 0x00486730: opens and starts the stream (no open or enabled check)
int StreamPlayer::Play(const std::u8string& fileName, int id, int volume, int loops)
{
    if (id == 0 || this->Find(id) != nullptr)
    {
        return 0;
    }

    std::unique_ptr<SequenceNode> node = this->AllocNode();

    if (node == nullptr)
    {
        return 0;
    }

    node->name.clear();
    node->fadeStep = 0;
    node->voice = this->device.OpenStream(fileName);

    if (node->voice == nullptr)
    {
        this->error = this->device.LastError();
        this->FreeNode(std::move(node));
        return 0;
    }

    node->name = fileName;
    node->volume = SequenceNode::VolumeScale * ClampVolume(volume);
    node->id = id;
    this->device.SetVolume(node->voice, ClampVolume(volume));
    this->device.SetLoops(node->voice, loops);
    this->device.Start(node->voice);
    this->PushActive(std::move(node));
    return id;
}

// swd32.exe 0x00486860: fades the stream out over `steps` updates
int StreamPlayer::FadeOut(int id, int steps)
{
    if (!this->IsOpen() || !this->IsEnabled() || id == 0)
    {
        return 0;
    }

    SequenceNode* node = this->Find(id);

    if (node == nullptr)
    {
        return 0;
    }

    int total = 0;
    int current = 0;
    this->device.Position(node->voice, total, current);
    node->flags |= SequencePlayer::Fading;
    SetFadeStep(*node, steps);
    return id;
}

// swd32.exe 0x00486900: like the MIDI update for streams; a finished stream is muted before it closes. The open and
// enabled flags are not checked.
bool StreamPlayer::Update()
{
    if (this->active.empty())
    {
        return false;
    }

    bool retiredLast = false;

    for (auto node = this->active.begin(); node != this->active.end();)
    {
        bool retire = false;
        SequenceNode& stream = **node;

        if ((stream.flags & SequencePlayer::Fading) == 0)
        {
            switch (this->device.Status(stream.voice))
            {
                case VoiceStatus::Done:
                    this->device.SetVolume(stream.voice, 0);
                    retire = true;
                    break;

                case VoiceStatus::Playing:
                case VoiceStatus::Stopped:
                case VoiceStatus::PlayingButReleased:
                    retiredLast = false;
                    break;

                default:
                    retire = retiredLast;
                    break;
            }
        }
        else
        {
            stream.volume -= stream.fadeStep;
            const int volume = ClampVolume(stream.volume / SequenceNode::VolumeScale);

            if (volume != 0)
            {
                this->device.SetVolume(stream.voice, volume);
                retiredLast = false;
            }
            else
            {
                retire = true;
            }
        }

        if (!retire)
        {
            ++node;
            continue;
        }

        retiredLast = true;
        this->device.SetVolume(stream.voice, 0);
        this->device.End(stream.voice);
        stream.voice = nullptr;
        stream.name.clear();
        stream.fadeStep = 0;
        stream.flags = 0;
        this->free.push_back(std::move(*node));
        node = this->active.erase(node);
    }

    return false;
}

// swd32.exe 0x00486A10
bool StreamPlayer::IsStopped(int id)
{
    return this->Find(id) == nullptr;
}

// swd32.exe 0x00486A70
SequenceNode* StreamPlayer::Find(int id)
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

}  // namespace swd3
