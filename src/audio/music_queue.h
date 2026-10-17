#pragma once

#include <array>
#include <string>

#include "core/types.h"

namespace swd3
{

class MidiPlayer;
class StreamPlayer;

// A queued piece of music.
struct MusicSlot
{
    static constexpr int Stream = 1;  // `type`
    static constexpr int Midi = 2;

    std::u8string name;
    int id = 0;
    int type = 0;  // Stream or Midi
};

// Two alternating playlists of two pieces each (the object at 0x004BA6E8).
class MusicQueue
{
public:
    static constexpr int FirstList = 1;  // the lists, and the modes playing them
    static constexpr int SecondList = 2;
    static constexpr int Silent = 3;  // a pending mode: none (an active mode: stop)
    static constexpr std::size_t Pieces = 2;
    static constexpr i32 MaxVolume = 0x7F;
    static constexpr int PlayOnce = 1;

    MusicQueue(StreamPlayer& streams, MidiPlayer& midi);
    ~MusicQueue();

    int ClearSlots(int list);
    int Update();

private:
    StreamPlayer& streams;
    MidiPlayer& midi;
    // The original keeps three words at +0x00..+0x08 (0x1E, 0, 0) that nothing reads.
    i32 mode = Silent;     // FirstList or SecondList
    i32 pending = Silent;  // the mode to switch to
    i32 index2 = 0;
    i32 loop2 = 0;
    i32 index1 = 0;
    i32 loop1 = 0;
    i32 volume = MaxVolume;
    MusicSlot current;
    std::array<MusicSlot, Pieces> list2;
    std::array<MusicSlot, Pieces> list1;
};

}  // namespace swd3
