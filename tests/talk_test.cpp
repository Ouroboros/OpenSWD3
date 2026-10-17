#include <cstdio>
#include <cstring>
#include <vector>

#include "game_data.h"
#include "talk/talk.h"
#include "test.h"

using swd3::u8;

namespace
{

constexpr swd3::u32 kWindowShiftXAt = 0x004994F4;  // where swd32.exe keeps the window shift tables
constexpr swd3::u32 kWindowShiftYAt = 0x00499514;

}  // namespace

TEST_CASE("talk: numbers of the sound code")
{
    int value = 7;
    CHECK(swd3::Talk::ParseNumber("123", value));
    CHECK_EQ(value, 123);
    CHECK(swd3::Talk::ParseNumber("-45", value));
    CHECK_EQ(value, -45);
    CHECK(swd3::Talk::ParseNumber("+9", value));
    CHECK_EQ(value, 9);
    CHECK(swd3::Talk::ParseNumber("999999999", value));
    CHECK_EQ(value, 999999999);
    value = 7;
    CHECK(!swd3::Talk::ParseNumber("1234567890", value));
    CHECK(!swd3::Talk::ParseNumber("12a", value));
    CHECK_EQ(value, 7);
    CHECK(swd3::Talk::ParseNumber("", value));
    CHECK_EQ(value, 0);
}

TEST_CASE("talk: window shift tables match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kWindowShiftXAt, swd3::Talk::WindowShiftX));
    CHECK(swd3test::SameTable(exe, kWindowShiftYAt, swd3::Talk::WindowShiftY));
}
