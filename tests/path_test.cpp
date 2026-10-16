#include <cstring>
#include <vector>

#include "data/globals.h"
#include "role/path_finder.h"
#include "test.h"

using namespace swd3;

namespace {

// 8x8 map with a wall in column 4 from row 0 to row 5.
struct MapFixture {
    std::vector<std::int32_t> cells = std::vector<std::int32_t>(64, 0);

    MapFixture() {
        for (int row = 0; row <= 5; ++row) {
            cells[row * 8 + 4] = 0x40000000;
        }
        G.mapWidth = 8;
        G.mapHeight = 8;
        G.mapBlocking = cells.data();
        PathPoolInit();
    }
    ~MapFixture() { PathPoolRelease(); }
};

}  // namespace

TEST_CASE("path.finds a route around a wall and encodes directions") {
    MapFixture map;
    PathFinder finder;
    std::uint8_t directions[64];
    CHECK_EQ(finder.FindPath(2 * 16, 2 * 16, 6 * 16, 2 * 16, 1, 1, directions), 1);
    int steps = 0;
    int x = 2;
    int y = 2;
    // emitted codes are opposite to the expansion directions: 0 moves (+1, +1), 1 (0, +1), ... 7 (+1, 0)
    static const int dx[8] = {1, 0, -1, -1, -1, 0, 1, 1};
    static const int dy[8] = {1, 1, 1, 0, -1, -1, -1, 0};
    while (directions[steps] != 0xFF) {
        CHECK(directions[steps] < 8);
        x += dx[directions[steps]];
        y += dy[directions[steps]];
        CHECK(map.cells[y * 8 + x] == 0);
        ++steps;
    }
    CHECK_EQ(x, 6);
    CHECK_EQ(y, 2);
    CHECK(steps >= 4);
    CHECK_EQ(finder.m_found, 1);
    CHECK_EQ(G.pathPoolPeak, 8);

    // m_found stays set, so the next non-trivial search is rejected
    CHECK_EQ(finder.FindPath(1 * 16, 1 * 16, 1 * 16, 6 * 16, 1, 1, directions), 0);
    CHECK_EQ(directions[0], 0xFF);
    // an unreachable target clears it again
    CHECK_EQ(finder.FindPath(1 * 16, 1 * 16, 4 * 16, 0, 1, 1, directions), 0);
    CHECK_EQ(finder.m_found, 0);
    CHECK_EQ(finder.FindPath(1 * 16, 1 * 16, 1 * 16, 6 * 16, 1, 1, directions), 1);
    finder.Reset();
    // every node is back in the pool, so releasing it counts down to zero
    CHECK(PathPoolRelease());
    PathPoolInit();
}
