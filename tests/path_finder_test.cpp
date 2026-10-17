#include <array>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "game/game_state.h"
#include "golden/path_scenarios.h"
#include "golden_check.h"
#include "map/blocking_map.h"
#include "recording_reporter.h"
#include "role/path_finder.h"
#include "test.h"

using golden::u64;
using golden::u8;

namespace
{

constexpr std::size_t kDirections = std::tuple_size_v<decltype(swd3::MoveRecord::directions)>;
constexpr u8 kUnwritten = 0xEE;
constexpr int kTwice = 2;
constexpr int kSide = 10;  // a 10 x 10 map walled in, a wall at kWallX from the top border down to kWallEnd
constexpr int kWallX = 5;
constexpr int kWallEnd = 7;
constexpr int kStartX = 2;
constexpr int kStartY = 2;
constexpr int kGoalX = 7;
// The step of each direction byte (0 up left, clockwise).
constexpr std::array<int, 8> kStepX = {-1, 0, 1, 1, 1, 0, -1, -1};
constexpr std::array<int, 8> kStepY = {-1, -1, -1, 0, 1, 1, 1, 0};

class FinderBackend : public golden::PathBackend
{
public:
    FinderBackend()
    {
        this->pool.Init();
    }

    std::vector<u8> Find(const golden::PathScenario& scenario, bool again) override
    {
        swd3::BlockingMap map;
        map.width = scenario.mapWidth;
        map.cells = scenario.cells;
        swd3::PathFinder finder(this->pool, map, this->reporter);
        finder.SetMask(static_cast<int>(scenario.mask));
        std::vector<u8> directions(kDirections, kUnwritten);
        std::vector<u8> out;
        const int runs = again ? kTwice : 1;

        for (int run = 0; run < runs; ++run)
        {
            const int result = finder.FindPath(
                scenario.fromX,
                scenario.fromY,
                scenario.toX,
                scenario.toY,
                scenario.width,
                scenario.height,
                directions
            );
            golden::Append32(out, static_cast<golden::u32>(result));

            for (const u8 direction : directions)
            {
                out.push_back(direction);

                if (direction == swd3::PathFinder::End)
                {
                    break;
                }
            }
        }

        finder.Reset();
        golden::Append32(out, static_cast<golden::u32>(this->pool.free));
        golden::Append32(out, static_cast<golden::u32>(this->pool.peak));
        return out;
    }

private:
    swd3::PathNodePool pool;
    swd3test::RecordingReporter reporter;
};

}  // namespace

TEST_CASE("path finder: a path around a wall, and the second search of one finder")
{
    using swd3::MapHeader;
    swd3::PathNodePool pool;
    pool.Init();
    swd3::BlockingMap map;
    map.width = kSide;
    std::vector<swd3::u32> cells(static_cast<std::size_t>(kSide * kSide), 0);
    map.cells = cells;

    auto cell = [](int x, int y)
    {
        return static_cast<std::size_t>(y * kSide + x);
    };

    for (int along = 0; along < kSide; ++along)
    {
        cells[cell(0, along)] = MapHeader::Outside;
        cells[cell(kSide - 1, along)] = MapHeader::Outside;
        cells[cell(along, 0)] = MapHeader::Outside;
        cells[cell(along, kSide - 1)] = MapHeader::Outside;
    }

    for (int y = 1; y < kWallEnd; ++y)
    {
        cells[cell(kWallX, y)] = MapHeader::Wall;
    }

    swd3test::RecordingReporter reporter;
    swd3::PathFinder finder(pool, map, reporter);
    std::vector<u8> directions(kDirections);
    constexpr int kTile = MapHeader::TileSize;
    CHECK_EQ(finder.FindPath(kStartX * kTile, kStartY * kTile, kGoalX * kTile, kStartY * kTile, 1, 1, directions), 1);
    int x = kStartX;
    int y = kStartY;
    int steps = 0;

    for (const u8 direction : directions)
    {
        if (direction == swd3::PathFinder::End)
        {
            break;
        }

        // A direction byte is looked up from the tile minus the next one, so it names the opposite of the step.
        x -= kStepX[direction];
        y -= kStepY[direction];
        CHECK(map.cells[cell(x, y)] == 0);
        ++steps;
    }

    CHECK_EQ(x, kGoalX);
    CHECK_EQ(y, kStartY);
    CHECK(steps > kGoalX - kStartX);  // longer than the way straight through the wall
    CHECK_EQ(finder.found, 1);
    CHECK_EQ(
        finder.FindPath(kStartX * kTile, kStartY * kTile, kGoalX * kTile, (kStartY + 1) * kTile, 1, 1, directions),
        0
    );
    finder.Reset();
    CHECK_EQ(pool.peak, swd3::PathNodePool::Initial);
    CHECK(pool.Release());
    CHECK(reporter.messages.empty());
}

TEST_CASE("path finder: matches the golden results of the legacy path finder")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/path_golden.txt");

    if (golden.empty())
    {
        std::printf("  (golden file not found, skipped)\n");
        return;
    }

    FinderBackend backend;
    swd3test::GoldenCheck check(golden);
    golden::RunPaths(backend, std::ref(check));
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
}
