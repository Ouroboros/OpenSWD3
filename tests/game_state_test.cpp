#include <cstdio>
#include <cstring>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"
#include "game/direction.h"
#include "game/game_state.h"
#include "game_data.h"
#include "platform/surface.h"
#include "res/act_state.h"
#include "test.h"
#include "ui/marching_rect.h"

using swd3::GameState;
using swd3::u16;
using swd3::u8;

TEST_CASE("game state: constant tables match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(std::memcmp(exe.data() + (0x0049F250 - 0x400000), swd3::kSineTable.data(), swd3::kSineTable.size() * 2) == 0);
    CHECK(std::memcmp(exe.data() + (0x004994EC - 0x400000), GameState::kBitMasks.data(), 8) == 0);
}

TEST_CASE("game state: story flags")
{
    GameState state;
    CHECK(!state.GameFlag(13));
    CHECK_EQ(state.SetGameFlag(13), 0x20);
    CHECK(state.GameFlag(13));
    CHECK_EQ(state.SetGameFlagTo(8, 1), 0x21);
    CHECK_EQ(state.SetGameFlagTo(13, 2), 0x01);
    CHECK(!state.GameFlag(13));
    CHECK_EQ(state.ClearGameFlag(8), 0);
    CHECK_EQ(state.SetFrameInterval(5), 1);
    CHECK_EQ(state.frameInterval, 5);
}

TEST_CASE("game state: click regions are matched strictly inside, the head first")
{
    swd3::ClickRegions regions;
    regions.regions.push_front({0, 10, 10, 20, 20});
    regions.regions.push_front({0, 0, 0, 100, 100});
    CHECK_EQ(regions.Count(), 2);
    swd3::ClickRegion* found = nullptr;
    CHECK_EQ(regions.Find(15, 15, found), 0);
    CHECK(found == &regions.regions.front());
    CHECK_EQ(regions.Find(100, 50, found), 2);
    CHECK(found == nullptr);
    regions.state = {1, 2, 3};
    regions.Free();
    CHECK_EQ(regions.Count(), 0);
    CHECK_EQ(regions.state[2], 0);
}

TEST_CASE("game state: angles and directions")
{
    swd3::i32 distance = -1;
    CHECK_EQ(swd3::AngleBetween(5, 5, 5, 5, distance), 0);
    CHECK_EQ(distance, 0);
    CHECK_EQ(swd3::AngleBetween(0, -30, 0, 0, distance), 85);
    CHECK_EQ(distance, 30);
    CHECK_EQ(swd3::AngleToDirection(90), 0);
    CHECK_EQ(swd3::AngleToDirection(359), 3);
    GameState state;
    state.playerRole = 3;
    state.roles[3].x = 100;
    state.roles[3].y = 100;
    CHECK_EQ(
        state.DirectionToPlayer(100, 130),
        swd3::AngleToDirection(swd3::AngleBetween(100, 100, 100, 130, distance))
    );
}

TEST_CASE("game state: marching frame and the save thumbnail")
{
    swd3::Surface screen(640, 480);
    screen.Fill(0);
    swd3::Painter painter;
    CHECK(painter.Init(screen));
    swd3::MarchingRect frame;
    frame.Draw(screen, painter, 600, 10, 40, 10);
    CHECK_EQ(frame.phase, 0);
    frame.Draw(screen, painter, 10, 10, 4, 3);
    CHECK_EQ(frame.phase, 1);
    CHECK(screen.Row(10)[10] != 0);
    CHECK(screen.Row(13)[10] != 0);
    CHECK_EQ(screen.Row(11)[11], 0);

    for (int y = 0; y < 480; ++y)
    {
        for (int x = 0; x < 640; ++x)
        {
            screen.Row(y)[x] = static_cast<u16>(y * 7 + x);
        }
    }

    swd3::ShrinkScreenQuarter(screen);
    CHECK_EQ(screen.Pixels()[0], 0);
    CHECK_EQ(screen.Pixels()[1], 4);
    CHECK_EQ(screen.Pixels()[160], static_cast<u16>(4 * 7));
    CHECK_EQ(screen.Pixels()[119 * 160 + 159], static_cast<u16>(476 * 7 + 636));
    swd3::ActState act;
    act.position = 9;
    act.hold = 1;
    swd3::ResetPlayback(act);
    CHECK_EQ(act.position, 0);
    CHECK_EQ(act.unknown1C, -1);
    CHECK_EQ(act.hold, 0);
}
