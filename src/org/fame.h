#pragma once

#include <array>
#include <memory>
#include <span>
#include <vector>

#include "core/types.h"

namespace swd3
{

class Database;
class ErrorReporter;

// A counter (0x14 bytes in the original): fame and proficiency records by id. A list is held by a head of the same
// layout (the original's five-dword holder) whose id field carries the count of nodes; the lookups compare the head
// itself first, so the count can match an id.
struct CounterNode
{
    static constexpr std::size_t SavedBytes = 0xE;

    std::unique_ptr<CounterNode> next;  // +0x00
    i16 id = 0;                         // +0x04 the count in a head
    u16 value = 0;                      // +0x06
    u16 percent = 0;                    // +0x08
    u16 done = 0;                       // +0x0A
    std::array<u8, 6> unk0C = {};       // +0x0C
};

using CounterList = CounterNode;

void ClearCounterList(CounterList& list);
u16 AddCounterCapped(CounterList& list, i16 id, i16 amount);
u16 SetCounterCapped(CounterList& list, i16 id, u16 value);
i16 FindCounterValue(CounterList& list, i16 id);
int StepCounterProgress(CounterList& list, i16 id, u16 maximum, u16 scale);
void SetCounterProgress(CounterList& list, i16 id, u16 maximum, u16 value);
i16 FindCounterPercent(CounterList& list, i16 id);
void SetProficiency(CounterList& list, u16 id, u16 value, Database& database);
bool GetProficiency(CounterList& list, u16 id, u16& maximum, u16& value, Database& database);

// The three counter lists saved with the game (0x004ACBA8, 0x004B9F00, 0x004B8A00; the last also holds the
// proficiencies).
struct Fame
{
    // The save block: 1, the unpacked size, the packed size, then the 15-bit LZO data of the three lists.
    bool Save(std::vector<u8>& block, ErrorReporter& reporter) const;
    bool Load(std::span<const u8> block, ErrorReporter& reporter);
    void Free();

    std::array<CounterList, 3> lists;
};

}  // namespace swd3
