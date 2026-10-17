#include <cstdio>
#include <cstring>
#include <vector>

#include "core/types.h"
#include "game_data.h"
#include "random/random.h"
#include "test.h"

using swd3::Random;
using swd3::u32;
using swd3::u8;

namespace
{

constexpr u32 kInitialTableAt = 0x004A6610;  // where swd32.exe keeps the R250 table and the LCG state
constexpr u32 kInitialLcgAt = 0x004A69F8;

constexpr u32 kLcgSeed = 7;
constexpr u32 kLcgMultiplier = 0x015A4E35;  // the seeding LCG: 15-bit values from bit kLcgShift
constexpr int kLcgShift = 16;
constexpr u32 kLcgMask = 0x7FFF;
constexpr u32 kSeed = 0x12345678;
constexpr int kDraws = 1000;
constexpr int kLimits = 97;     // Below is drawn with limits 1..kLimits
constexpr u32 kRange = 0xFFFF;  // Below rejects draws past the last multiple of the limit in this range

}  // namespace

TEST_CASE("random: initial table and LCG state match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kInitialTableAt, Random::InitialTable));
    CHECK_EQ(swd3test::ExeDword(exe, kInitialLcgAt), Random::InitialLcg);
}

TEST_CASE("random: LCG and the R250 sequence")
{
    Random random;
    CHECK_EQ(random.LcgSeed(kLcgSeed), kLcgSeed);
    CHECK_EQ(random.LcgNext(), ((kLcgMultiplier * kLcgSeed + 1) >> kLcgShift) & kLcgMask);
    Random unseeded;
    CHECK_EQ(unseeded.Next(), Random::InitialTable[Random::Size - Random::Tap] ^ Random::InitialTable[0]);
    Random first;
    Random second;
    first.Seed(kSeed);
    second.Seed(kSeed);

    for (int i = 0; i < kDraws; ++i)
    {
        const u32 limit = static_cast<u32>(i % kLimits + 1);
        const u32 value = first.Below(limit);
        u32 drawn = 0;

        do
        {
            second.Next();
            drawn = second.Next();
        } while (drawn >= limit * (kRange / limit));

        CHECK_EQ(value, drawn % limit);
        CHECK(value < limit);
    }
}
