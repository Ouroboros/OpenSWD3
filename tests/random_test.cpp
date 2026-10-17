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
    CHECK_EQ(random.LcgSeed(7), 7u);
    CHECK_EQ(random.LcgNext(), ((0x015A4E35u * 7u + 1u) >> 16) & 0x7FFFu);
    Random unseeded;
    CHECK_EQ(unseeded.Next(), Random::InitialTable[103] ^ Random::InitialTable[0]);
    Random first;
    Random second;
    first.Seed(0x12345678);
    second.Seed(0x12345678);

    for (int i = 0; i < 1000; ++i)
    {
        const u32 limit = static_cast<u32>(i % 97 + 1);
        const u32 value = first.Below(limit);
        u32 drawn = 0;

        do
        {
            second.Next();
            drawn = second.Next();
        } while (drawn >= limit * (0xFFFFu / limit));

        CHECK_EQ(value, drawn % limit);
        CHECK(value < limit);
    }
}
