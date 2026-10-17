#include <algorithm>
#include <cstdio>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"
#include "game/direction.h"
#include "game/game_state.h"
#include "game_data.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "res/act_state.h"
#include "role/role_control.h"
#include "role/role_path.h"
#include "test.h"
#include "ui/marching_rect.h"

using swd3::GameState;
using swd3::u16;
using swd3::u8;

namespace
{

// Where swd32.exe keeps the tables.
constexpr swd3::u32 kSineTableAt = 0x0049F250;
constexpr swd3::u32 kBitMasksAt = 0x004994EC;
constexpr std::array<swd3::u32, 2> kBlockedByCopies = {0x0049941C, 0x0049961C};
constexpr std::array<swd3::u32, 4> kStepCopies = {0x0049943C, 0x0049963C, 0x004994AC, 0x00499380};  // x, then y
constexpr std::array<swd3::u32, 2> kFacingCopies = {0x0049947C, 0x0049967C};
constexpr swd3::u32 kAheadXAt = 0x004993CC;
constexpr swd3::u32 kAheadYAt = 0x004993EC;
constexpr swd3::u32 kRunToggleAt = 0x004993C0;

constexpr int kFlag = 13;
constexpr int kNeighbourFlag = 8;  // in the byte of kFlag
constexpr int kNotOne = 2;         // SetGameFlagTo clears for anything but 1
constexpr int kFrameInterval = 5;

constexpr int kRegionLeft = 10;  // a small region inside a large one
constexpr int kRegionTop = 10;
constexpr int kRegionRight = 20;
constexpr int kRegionBottom = 20;
constexpr int kLargeSize = 100;
constexpr std::array<swd3::i32, 3> kRegionState = {1, 2, 3};

constexpr int kSamePoint = 5;
constexpr int kAbove = 30;
constexpr int kStraightUpAngle = 85;  // the sine table finds 85 degrees for a point straight above
constexpr int kRightAngle = 90;
constexpr int kDirectionAt90 = 0;
constexpr int kLastDegree = 359;
constexpr int kDirectionAt359 = 3;
constexpr int kPlayer = 3;
constexpr int kPlayerAt = 100;
constexpr int kBelowPlayer = 130;

constexpr int kFrameWidth = 40;  // a frame reaching the right edge is not drawn
constexpr int kFrameHeight = 10;
constexpr int kFrameX = 10;
constexpr int kFrameY = 10;
constexpr int kSmallWidth = 4;
constexpr int kSmallHeight = 3;
constexpr int kRowStep = 7;  // the test picture: pixel y * kRowStep + x
constexpr int kShrink = 4;   // ShrinkScreenQuarter keeps every fourth pixel and row
constexpr int kActPosition = 9;

}  // namespace

TEST_CASE("game state: constant tables match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    using swd3test::SameTable;
    CHECK(SameTable(exe, kSineTableAt, swd3::kSineTable));
    CHECK(SameTable(exe, kBitMasksAt, GameState::BitMasks));

    for (const swd3::u32 copy : kBlockedByCopies)
    {
        CHECK(SameTable(exe, copy, swd3::RolePath::BlockedBy));
    }

    for (const swd3::u32 copy : kStepCopies)
    {
        CHECK(SameTable(exe, copy, swd3::RolePath::StepX));
        CHECK(SameTable(exe, copy + sizeof(swd3::RolePath::StepX), swd3::RolePath::StepY));
    }

    for (const swd3::u32 copy : kFacingCopies)
    {
        CHECK(SameTable(exe, copy, swd3::RolePath::Facing));
    }

    CHECK(SameTable(exe, kAheadXAt, swd3::RoleControl::AheadX));
    CHECK(SameTable(exe, kAheadYAt, swd3::RoleControl::AheadY));
    CHECK(SameTable(exe, kRunToggleAt, swd3::RoleControl::RunToggle));
}

TEST_CASE("game state: story flags")
{
    GameState state;
    const u8 flagBit = GameState::BitMasks[kFlag & GameState::FlagBitMask];
    const u8 neighbourBit = GameState::BitMasks[kNeighbourFlag & GameState::FlagBitMask];
    CHECK(!state.GameFlag(kFlag));
    CHECK_EQ(state.SetGameFlag(kFlag), flagBit);
    CHECK(state.GameFlag(kFlag));
    CHECK_EQ(state.SetGameFlagTo(kNeighbourFlag, 1), flagBit | neighbourBit);
    CHECK_EQ(state.SetGameFlagTo(kFlag, kNotOne), neighbourBit);
    CHECK(!state.GameFlag(kFlag));
    CHECK_EQ(state.ClearGameFlag(kNeighbourFlag), 0);
    CHECK_EQ(state.SetFrameInterval(kFrameInterval), 1);
    CHECK_EQ(state.frameInterval, kFrameInterval);
}

TEST_CASE("game state: click regions are matched strictly inside, the head first")
{
    swd3::ClickRegions regions;
    regions.regions.push_front({0, kRegionLeft, kRegionTop, kRegionRight, kRegionBottom});
    regions.regions.push_front({0, 0, 0, kLargeSize, kLargeSize});
    CHECK_EQ(regions.Count(), static_cast<int>(regions.regions.size()));
    swd3::ClickRegion* found = nullptr;
    CHECK_EQ(regions.Find((kRegionLeft + kRegionRight) / 2, (kRegionTop + kRegionBottom) / 2, found), 0);
    CHECK(found == &regions.regions.front());
    CHECK_EQ(regions.Find(kLargeSize, kLargeSize / 2, found), regions.Count());
    CHECK(found == nullptr);
    regions.state = kRegionState;
    regions.Free();
    CHECK_EQ(regions.Count(), 0);
    CHECK(
        std::all_of(
            regions.state.begin(),
            regions.state.end(),
            [](swd3::i32 value)
            {
                return value == 0;
            }
        )
    );
}

TEST_CASE("game state: angles and directions")
{
    swd3::i32 distance = -1;
    CHECK_EQ(swd3::AngleBetween(kSamePoint, kSamePoint, kSamePoint, kSamePoint, distance), 0);
    CHECK_EQ(distance, 0);
    CHECK_EQ(swd3::AngleBetween(0, -kAbove, 0, 0, distance), kStraightUpAngle);
    CHECK_EQ(distance, kAbove);
    CHECK_EQ(swd3::AngleToDirection(kRightAngle), kDirectionAt90);
    CHECK_EQ(swd3::AngleToDirection(kLastDegree), kDirectionAt359);
    GameState state;
    state.playerRole = kPlayer;
    state.roles[kPlayer].x = kPlayerAt;
    state.roles[kPlayer].y = kPlayerAt;
    CHECK_EQ(
        state.DirectionToPlayer(kPlayerAt, kBelowPlayer),
        swd3::AngleToDirection(swd3::AngleBetween(kPlayerAt, kPlayerAt, kPlayerAt, kBelowPlayer, distance))
    );
}

TEST_CASE("game state: marching frame and the save thumbnail")
{
    using swd3::Display;
    swd3::Surface screen(Display::Width, Display::Height);
    screen.Fill(0);
    swd3::Painter painter;
    CHECK(painter.Init(screen));
    swd3::MarchingRect frame;
    frame.Draw(screen, painter, Display::Width - kFrameWidth, kFrameY, kFrameWidth, kFrameHeight);
    CHECK_EQ(frame.phase, 0);
    frame.Draw(screen, painter, kFrameX, kFrameY, kSmallWidth, kSmallHeight);
    CHECK_EQ(frame.phase, 1);
    CHECK(screen.Row(kFrameY)[kFrameX] != 0);
    CHECK(screen.Row(kFrameY + kSmallHeight)[kFrameX] != 0);
    CHECK_EQ(screen.Row(kFrameY + 1)[kFrameX + 1], 0);

    for (int y = 0; y < Display::Height; ++y)
    {
        for (int x = 0; x < Display::Width; ++x)
        {
            screen.Row(y)[x] = static_cast<u16>(y * kRowStep + x);
        }
    }

    swd3::ShrinkScreenQuarter(screen);
    constexpr int kShrunkWidth = Display::Width / kShrink;
    constexpr int kShrunkHeight = Display::Height / kShrink;
    const u16* shrunk = screen.Pixels();
    CHECK_EQ(shrunk[0], 0);
    CHECK_EQ(shrunk[1], kShrink);
    CHECK_EQ(shrunk[kShrunkWidth], static_cast<u16>(kShrink * kRowStep));
    CHECK_EQ(
        shrunk[(kShrunkHeight - 1) * kShrunkWidth + kShrunkWidth - 1],
        static_cast<u16>((Display::Height - kShrink) * kRowStep + Display::Width - kShrink)
    );
    swd3::ActState act;
    act.position = kActPosition;
    act.hold = 1;
    swd3::ResetPlayback(act);
    CHECK_EQ(act.position, 0);
    CHECK_EQ(act.unknown1C, -1);
    CHECK_EQ(act.hold, 0);
}
