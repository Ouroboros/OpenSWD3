#pragma once

#include <climits>
#include <cstddef>
#include <span>

#include "core/types.h"

namespace swd3
{

constexpr u16 kTextEnd = 0x5125;  // "%Q": ends the texts of the event scripts and the names of maps.dat

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
