#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

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
        std::vector<u8> directions(0x200, 0xEE);
        std::vector<u8> out;
        const int runs = again ? 2 : 1;

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
    swd3::PathNodePool pool;
    pool.Init();
    swd3::BlockingMap map;
    map.width = 10;
    std::vector<swd3::u32> cells(100, 0);
    map.cells = cells;

    for (int y = 0; y < 10; ++y)
    {
        cells[static_cast<std::size_t>(y) * 10] = 0xFFFFFFFF;
        cells[static_cast<std::size_t>(y) * 10 + 9] = 0xFFFFFFFF;
        cells[static_cast<std::size_t>(y)] = 0xFFFFFFFF;
        cells[90 + static_cast<std::size_t>(y)] = 0xFFFFFFFF;
    }

    for (int y = 1; y < 7; ++y)
    {
        cells[static_cast<std::size_t>(y) * 10 + 5] = 0x40000000;
    }

    swd3test::RecordingReporter reporter;
    swd3::PathFinder finder(pool, map, reporter);
    std::vector<u8> directions(0x200);
    CHECK_EQ(finder.FindPath(2 * 16, 2 * 16, 7 * 16, 2 * 16, 1, 1, directions), 1);
    int x = 2;
    int y = 2;
    int steps = 0;
    const int dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    const int dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};

    for (const u8 direction : directions)
    {
        if (direction == swd3::PathFinder::End)
        {
            break;
        }

        // A direction byte is looked up from the tile minus the next one, so it names the opposite of the step.
        x -= dx[direction];
        y -= dy[direction];
        CHECK(map.cells[static_cast<std::size_t>(y) * 10 + x] == 0);
        ++steps;
    }

    CHECK_EQ(x, 7);
    CHECK_EQ(y, 2);
    CHECK(steps > 5);
    CHECK_EQ(finder.found, 1);
    CHECK_EQ(finder.FindPath(2 * 16, 2 * 16, 7 * 16, 3 * 16, 1, 1, directions), 0);
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
