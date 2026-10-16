#include <cstring>

#include "core/random.h"
#include "data/globals.h"
#include "test.h"
#include "text/text_input.h"

using namespace swd3;

TEST_CASE("random.lcg and r250 seeding pattern") {
    LcgSeed(1);
    CHECK_EQ(LcgNext(), 0x015Au);
    R250Seed(1);
    CHECK_EQ(G.r250Index, 0);
    CHECK(G.r250Table[3] & 0x8000u);
    CHECK_EQ(G.r250Table[168], 1u);
    CHECK(G.r250Table[157] == 2u || G.r250Table[157] == 3u);
    const std::uint32_t first = G.r250Table[0] ^ G.r250Table[103];
    CHECK_EQ(R250Next(), first);
    CHECK_EQ(G.r250Index, 1);
    for (int i = 0; i < 100; ++i) {
        CHECK(R250Below(7) < 7u);
    }
}

TEST_CASE("text_input keeps whole Big5 characters") {
    TextInput input("AB\xa4\xa4", 3, 5, 6);
    char out[8];
    input.CopyText(out, sizeof(out));
    CHECK(std::strcmp(out, "AB") == 0);
    CHECK_EQ(input.GetCursor(), 0);
    CHECK_EQ(input.CursorRight(), 1);
    input.InsertText("\xa4\xa4");
    input.CopyText(out, sizeof(out));
    CHECK(std::strcmp(out, "A\xa4\xa4") == 0);
    CHECK_EQ(input.GetCursor(), 3);
    CHECK(input.CursorLeft() != 0);
    CHECK_EQ(input.GetCursor(), 1);
    input.DeleteAtCursor();
    input.CopyText(out, sizeof(out));
    CHECK(std::strcmp(out, "A") == 0);
    CHECK_EQ(TextFitLength("\xa4\xa4\xa4\xa4", 3), 2u);
}
