#include "core/random.h"

#include "data/globals.h"

namespace swd3 {

namespace {

constexpr int kSize = 250;
constexpr int kTap = 147;

std::uint32_t step() {
    const std::int32_t index = G.r250Index;
    const std::int32_t other = index >= kTap ? index - kTap : index + (kSize - kTap);
    const std::uint32_t value = G.r250Table[other] ^ G.r250Table[index];
    G.r250Table[index] = value;
    G.r250Index = index < kSize - 1 ? index + 1 : 0;
    return value;
}

}  // namespace

// swd32.exe 0x00438FA0: table from the LCG, bit 15 set when another LCG draw exceeds 0x4000, then the 16 basis
// words 3, 14, ..., 168 forced to a triangular bit pattern. The original returns a leftover table pointer.
void R250Seed(std::uint32_t seed) {
    LcgSeed(seed);
    G.r250Index = 0;
    for (int i = 0; i < kSize; ++i) {
        G.r250Table[i] = LcgNext();
    }
    for (int i = 0; i < kSize; ++i) {
        if (LcgNext() > 0x4000) {
            G.r250Table[i] |= 0x8000;
        }
    }
    std::uint32_t bit = 0x8000;
    std::uint32_t mask = 0xFFFF;
    for (int i = 3; i < 179; i += 11) {
        G.r250Table[i] = bit | (mask & G.r250Table[i]);
        mask >>= 1;
        bit >>= 1;
    }
}

// swd32.exe 0x00439020
std::uint32_t R250Next() { return step(); }

// swd32.exe 0x00439070: uniform value below `limit`; every attempt draws two values and keeps the second
std::uint32_t R250Below(std::uint32_t limit) {
    const std::uint32_t bound = limit * (0xFFFFu / limit);
    std::uint32_t value;
    do {
        R250Next();
        value = step();
    } while (value >= bound);
    return value % limit;
}

// swd32.exe 0x004390F0
std::uint32_t LcgNext() {
    G.lcgState = 0x015A4E35u * G.lcgState + 1;
    return (G.lcgState >> 16) & 0x7FFF;
}

// swd32.exe 0x00439110
std::uint32_t LcgSeed(std::uint32_t seed) {
    G.lcgState = seed;
    return seed;
}

}  // namespace swd3
