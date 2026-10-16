#pragma once

#include <climits>
#include <cstddef>
#include <span>

#include "core/types.h"
#include "core/words.h"

namespace swd3
{

// The commands of a path script (path.dat, run by RolePath::RunScript).
enum class PathCommand : u16
{
    Restart = 0,
    Stop = 1,
    SetAction = 2,
    SetDirection = 3,
    SetCount = 4,
    WaitCount = 5,
    WaitCountAnimated = 6,
    FindPath = 7,
    Step = 8,
    WaitClock = 9,
    JumpIfClockReached = 0xA,
    SetClock = 0xB,
    Jump = 0xC,
    JumpIfFlag = 0xD,
    JumpUnlessFlag = 0xE,
    JumpIfAllFlags = 0xF,
    JumpIfAnyFlag = 0x10,
    SetFlag = 0x11,
    ClearFlag = 0x12,
    SetVariable = 0x13,
    AddVariable = 0x14,
    SubtractVariable = 0x15,
    JumpIfVariableAtLeast = 0x16,
    JumpIfVariableAtMost = 0x17,
    StartPath = 0x18,
    JumpAfterMark = 0x19,
    MarkClock = 0x1A,
    HideRole = 0x1B,
    ShowRole = 0x1C,
    PlaceRole = 0x1D,
    Wander = 0x1E,
    SetAct = 0x1F,
    PlaySound = 0x20,
    FollowRole = 0x21,
    ShowLabel = 0x22,
    ClearLabel = 0x23,
    FollowBeside = 0x24,
};

// path.dat: a table of the start of every path (dwords relative to DataAt), the paths from DataAt. A path is a list of
// commands: a command word, then word operands; jump targets are dwords.
struct PathScript
{
    static constexpr std::size_t DataAt = 0x200;
    static constexpr std::size_t WordBytes = sizeof(u16);
    static constexpr std::size_t TargetBytes = sizeof(u32);
    static constexpr int OpcodeWords = 1;
    static constexpr int TargetWords = 2;
    static constexpr int StepShift = 1;  // path offsets in bytes to steps in words

    // The words of a command with `operands` word operands and `targets` jump targets.
    static constexpr int CommandWords(int operands, int targets = 0)
    {
        return OpcodeWords + operands + targets * TargetWords;
    }

    static u16 Word(std::span<const u8> data, std::size_t at)
    {
        return at + 1 < data.size() ? static_cast<u16>(data[at] | data[at + 1] << CHAR_BIT) : u16{0};
    }

    static u32 Dword(std::span<const u8> data, std::size_t at)
    {
        return MakeWords(Word(data, at + WordBytes), Word(data, at));
    }

    // The start of path `path` from DataAt.
    static u32 PathStart(std::span<const u8> data, std::size_t path)
    {
        return Dword(data, DataAt + TargetBytes * path);
    }
};

}  // namespace swd3
