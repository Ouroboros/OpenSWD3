#include <cstring>
#include <vector>

#include "data/globals.h"
#include "game/game_state.h"
#include "test.h"

using namespace swd3;

TEST_CASE("game.flags set, test and clear single bits") {
    std::memset(G.gameFlags, 0, sizeof(G.gameFlags));
    CHECK(!GameFlag(0x1771));
    CHECK_EQ(SetGameFlag(0x1771), 0x02);
    CHECK(GameFlag(0x1771));
    CHECK(!GameFlag(0x1770));
    CHECK_EQ(SetGameFlagTo(0x1770, 2), 0x02);
    CHECK_EQ(SetGameFlagTo(0x1770, 1), 0x03);
    CHECK_EQ(ClearGameFlag(0x1771), 0x01);
    CHECK_EQ(G.gameFlags[0x1771 >> 3], 0x01);
    G.gameFlags[0x1771 >> 3] = 0;
}

TEST_CASE("game.angles snap to 5 degrees and map to direction codes") {
    std::int32_t distance = -1;
    CHECK_EQ(AngleBetween(0, 0, 100, 0, &distance), 180);
    CHECK_EQ(distance, 100);
    CHECK_EQ(AngleToDirection(180), 2);
    CHECK_EQ(AngleBetween(0, 0, 0, 100, &distance), 85);
    CHECK_EQ(AngleToDirection(85), 0);
    CHECK_EQ(AngleBetween(5, 5, 5, 5, &distance), 0);
    CHECK_EQ(distance, 0);
    CHECK_EQ(AngleToDirection(359), 3);
}

TEST_CASE("game.screen shrink and click regions") {
    std::vector<std::uint16_t> screen(640 * 480);
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            screen[y * 640 + x] = static_cast<std::uint16_t>((y << 8) ^ x);
        }
    }
    ShrinkScreenQuarter(screen.data());
    CHECK_EQ(screen[1], static_cast<std::uint16_t>(4));
    CHECK_EQ(screen[160], static_cast<std::uint16_t>(4 << 8));
    CHECK_EQ(screen[160 + 1], static_cast<std::uint16_t>((4 << 8) ^ 4));
    CHECK_EQ(screen[2 * 160 + 1], static_cast<std::uint16_t>((8 << 8) ^ 4));

    ClickRegion second{0, nullptr, 50, 50, 60, 60};
    ClickRegion first{0, &second, 0, 0, 10, 10};
    G.clickRegions = &first;
    CHECK_EQ(CountClickRegions(), 2);
    G.pointerX = 55;
    G.pointerY = 55;
    ClickRegion* found = nullptr;
    CHECK_EQ(FindClickRegion(&found), 1);
    CHECK(found == &second);
    G.pointerX = 50;
    CHECK_EQ(FindClickRegion(&found), 2);
    CHECK(found == nullptr);
    G.clickRegions = nullptr;
}
