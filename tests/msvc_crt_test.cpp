#include <array>
#include <cstdio>
#include <string>

#include "crt/msvc.h"
#include "test.h"

using swd3::crt::Sprintf;

TEST_CASE("msvc crt: Sprintf writes what the current runtime's snprintf writes")
{
    std::array<char, 0x40> expected = {};
    std::snprintf(expected.data(), expected.size(), "MAct[%4.1f/%4.1f] LTCor[%4d/%4d]", 10.25, 3.0625, 12, 3);
    CHECK(Sprintf("MAct[%4.1f/%4.1f] LTCor[%4d/%4d]", 10.25, 3.0625, 12, 3) == std::string(expected.data()));
    CHECK(Sprintf("ULck[%x] %d%%", 0x2A, 5) == "ULck[2a] 5%");
    CHECK(Sprintf("[%s]", std::string("name")) == "[name]");
}

TEST_CASE("msvc crt: Sprintf keeps the 255 characters of the game's buffers")
{
    constexpr std::size_t kBufferChars = 0xFF;
    const std::string longText(kBufferChars + 10, 'x');
    CHECK_EQ(Sprintf("%s", longText).size(), kBufferChars);
}

TEST_CASE("msvc crt: Ftol truncates towards zero")
{
    CHECK_EQ(swd3::crt::Ftol(2.75), 2);
    CHECK_EQ(swd3::crt::Ftol(-2.75), -2);
}
