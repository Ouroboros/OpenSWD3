#pragma once

#include <climits>
#include <cstddef>
#include <span>

#include "core/types.h"

namespace swd3
{

constexpr u16 kTextEnd = 0x5125;  // "%Q": ends the texts of the event scripts and the names of maps.dat

// The control codes of a talk text (read as little-endian words).
constexpr u16 kNewLine = 0x4E25;    // "%N"
constexpr u16 kLine = 0x4C25;       // "%L"
constexpr u16 kPage = 0x5025;       // "%P"
constexpr u16 kSpeed = 0x5325;      // "%S" and a digit
constexpr u16 kColor = 0x4325;      // "%C" and a digit
constexpr u16 kShadeCode = 0x4425;  // "%D" and a digit (as the measuring sees it)
// The window drawing tests the shade code with its bytes swapped ("D%"), so it never sees "%D" (an original bug).
constexpr u16 kSwappedShadeCode = 0x2544;
constexpr u16 kStyle = 0x4725;   // "%G" and a digit
constexpr u16 kChoice = 0x4225;  // "%B" text "." (made "%b" once its click area is added)
constexpr u16 kChoiceSeen = 0x6225;
constexpr u16 kSound = 0x4125;  // "%A" number "." (made "%a" once its sound played)
constexpr u16 kSoundSeen = 0x6125;
constexpr u16 kKey = 0x4B25;  // "%K"
constexpr u16 kKeySeen = 0x6B25;
constexpr u16 kItemName = 0x5425;           // "%T" number ".": the name of an object of mon.dat
constexpr std::size_t kCodeBytes = 2;       // a control code
constexpr std::size_t kCodeDigitBytes = 3;  // a control code and its digit

// The little-endian word at `at` (0 past the end).
inline u16 TextWord(std::span<const u8> bytes, std::size_t at)
{
    return at + 1 < bytes.size() ? static_cast<u16>(bytes[at] | bytes[at + 1] << CHAR_BIT) : u16{0};
}

// Where the "%Q" of the text starting at `from` lies, read a byte at a time (the end of `bytes` when it is missing).
inline std::size_t TextEndAt(std::span<const u8> bytes, std::size_t from)
{
    std::size_t at = from;

    while (at + 1 < bytes.size() && TextWord(bytes, at) != kTextEnd)
    {
        ++at;
    }

    return at;
}

}  // namespace swd3
