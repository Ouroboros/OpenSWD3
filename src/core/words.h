#pragma once

#include <climits>

#include "core/types.h"

// The two 16-bit halves of a 32-bit value and the two bytes of a 16-bit one (the original packs them together).
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

constexpr u8 LowByte(u16 value)
{
    return static_cast<u8>(value);
}

constexpr u8 HighByte(u16 value)
{
    return static_cast<u8>(value >> CHAR_BIT);
}

constexpr u16 MakeBytes(u8 high, u8 low)
{
    return static_cast<u16>(high << CHAR_BIT | low);
}

}  // namespace swd3
