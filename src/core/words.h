#pragma once

#include <climits>

#include "core/types.h"

// The two 16-bit halves of a 32-bit value (the original keeps pairs of words in one dword).
namespace swd3
{

constexpr int kWordBits = CHAR_BIT * sizeof(u16);

constexpr u16 LowWord(u32 value)
{
    return static_cast<u16>(value);
}

constexpr u16 HighWord(u32 value)
{
    return static_cast<u16>(value >> kWordBits);
}

constexpr u32 MakeWords(u16 high, u16 low)
{
    return static_cast<u32>(high) << kWordBits | low;
}

}  // namespace swd3
