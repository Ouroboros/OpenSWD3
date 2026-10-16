#include "random/random.h"

namespace swd3
{

namespace
{

constexpr u32 kHighBit = 0x8000;
constexpr u32 kLowMask = 0xFFFF;
constexpr int kBasisFirst = 3;
constexpr int kBasisStride = 11;
constexpr int kBasisEnd = 179;

}  // namespace

// swd32.exe 0x00438FA0: the table from the LCG, bit 15 set when another LCG draw exceeds 0x4000, then the 16 words
// 3, 14, ..., 168 forced to a triangular bit pattern (the original returns a leftover pointer).
void Random::Seed(u32 seed)
{
    this->LcgSeed(seed);
    this->index = 0;

    for (u32& word : this->table)
    {
        word = this->LcgNext();
    }

    for (u32& word : this->table)
    {
        if (this->LcgNext() > 0x4000)
        {
            word |= kHighBit;
        }
    }

    u32 bit = kHighBit;
    u32 mask = kLowMask;

    for (int i = kBasisFirst; i < kBasisEnd; i += kBasisStride)
    {
        this->table[static_cast<std::size_t>(i)] = bit | (mask & this->table[static_cast<std::size_t>(i)]);
        mask >>= 1;
        bit >>= 1;
    }
}

u32 Random::Step()
{
    const i32 current = this->index;
    const i32 other = current >= Random::kTap ? current - Random::kTap : current + (Random::kSize - Random::kTap);
    const u32 value = this->table[static_cast<std::size_t>(other)] ^ this->table[static_cast<std::size_t>(current)];
    this->table[static_cast<std::size_t>(current)] = value;
    this->index = current < Random::kSize - 1 ? current + 1 : 0;
    return value;
}

// swd32.exe 0x00439020
u32 Random::Next()
{
    return this->Step();
}

// swd32.exe 0x00439070: a value below `limit`; every attempt draws two values and keeps the second
u32 Random::Below(u32 limit)
{
    const u32 bound = limit * (kLowMask / limit);
    u32 value = 0;

    do
    {
        this->Next();
        value = this->Step();
    } while (value >= bound);

    return value % limit;
}

// swd32.exe 0x004390F0
u32 Random::LcgNext()
{
    this->lcg = 0x015A4E35 * this->lcg + 1;
    return (this->lcg >> 16) & 0x7FFF;
}

// swd32.exe 0x00439110
u32 Random::LcgSeed(u32 seed)
{
    this->lcg = seed;
    return seed;
}

}  // namespace swd3
