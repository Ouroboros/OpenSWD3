#include "audio/music_queue.h"

#include "audio/sequence_player.h"
#include "platform/message_box.h"

namespace swd3
{

// swd32.exe 0x004853E0
MusicQueue::MusicQueue(StreamPlayer& streams, MidiPlayer& midi)
    : streams(streams),
      midi(midi)
{
}

// swd32.exe 0x00485440
MusicQueue::~MusicQueue()
{
    this->ClearSlots(MusicQueue::SecondList);
    this->ClearSlots(MusicQueue::FirstList);
}

// swd32.exe 0x00485460
int MusicQueue::ClearSlots(int list)
{
    if (list == MusicQueue::FirstList)
    {
        this->list1.fill(MusicSlot{});
    }
    else if (list == MusicQueue::SecondList)
    {
        this->list2.fill(MusicSlot{});
    }

    return 0;
}

// swd32.exe 0x004854B0: once the current piece has stopped, applies a pending list switch and starts the next entry
// of the current list; every stream start is preceded by MessageBeep(0)
int MusicQueue::Update()
{
    if (this->current.type == MusicSlot::Stream)
    {
        if (!this->streams.IsStopped(this->current.id))
        {
            return 0;
        }
    }
    else if (this->current.type == MusicSlot::Midi)
    {
        if (!this->midi.IsStopped(this->current.id))
        {
            return 0;
        }
    }

    this->current = MusicSlot{};
    const i32 switchTo = this->pending;

    if (switchTo > 0)
    {
        if (switchTo <= MusicQueue::SecondList)
        {
            this->mode = switchTo;
            this->pending = 0;

            if (switchTo == MusicQueue::FirstList)
            {
                this->index1 = 0;
            }
            else
            {
                this->index2 = 0;
            }
        }
        else if (switchTo == MusicQueue::Silent)
        {
            this->mode = 0;
            this->pending = 0;
        }
    }

    i32* index = nullptr;
    i32* loop = nullptr;
    MusicSlot* slot = nullptr;

    if (this->mode == MusicQueue::FirstList)
    {
        index = &this->index1;

        if (*index >= static_cast<i32>(MusicQueue::Pieces))
        {
            return 0;
        }

        slot = &this->list1[static_cast<std::size_t>(*index)];
        loop = &this->loop1;
    }
    else if (this->mode == MusicQueue::SecondList)
    {
        index = &this->index2;

        if (*index >= static_cast<i32>(MusicQueue::Pieces))
        {
            return 0;
        }

        slot = &this->list2[static_cast<std::size_t>(*index)];
        loop = &this->loop2;
    }
    else
    {
        return 0;
    }

    this->current = *slot;

    if (!this->current.name.empty())
    {
        if (this->current.type == MusicSlot::Stream)
        {
            PlaySystemBeep();
            this->streams.Play(this->current.name, this->current.id, this->volume, MusicQueue::PlayOnce);
        }
        else if (this->current.type == MusicSlot::Midi)
        {
            this->midi.Play(this->current.name, this->current.id, this->volume, MusicQueue::PlayOnce);
        }
    }

    ++*index;

    if (*loop == 1 && *index >= static_cast<i32>(MusicQueue::Pieces))
    {
        *index = 0;
    }

    return 0;
}

}  // namespace swd3
