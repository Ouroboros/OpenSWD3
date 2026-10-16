#include "talk/talk.h"
#include "test.h"

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
