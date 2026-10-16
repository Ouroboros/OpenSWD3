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
    std::string name;
    int id = 0;
    int type = 0;  // 1: stream, 2: MIDI
};

// Two alternating playlists of two pieces each (the object at 0x004BA6E8).
class MusicQueue
{
public:
    MusicQueue(StreamPlayer& streams, MidiPlayer& midi);
    ~MusicQueue();

    int ClearSlots(int list);
    int Update();

private:
    StreamPlayer& streams;
    MidiPlayer& midi;
    // The original keeps three words at +0x00..+0x08 (0x1E, 0, 0) that nothing reads.
    i32 mode = 3;     // 1: list 1, 2: list 2
    i32 pending = 3;  // the mode to switch to (3: none)
    i32 index2 = 0;
    i32 loop2 = 0;
    i32 index1 = 0;
    i32 loop1 = 0;
    i32 volume = 0x7F;
    MusicSlot current;
    std::array<MusicSlot, 2> list2;
    std::array<MusicSlot, 2> list1;
};

}  // namespace swd3
