// Golden generator for the path finder: runs tests/golden/path_scenarios.h through the legacy transliteration and
// writes "name hash" lines to $GOLDEN_OUT (see README.md). The blocking array is padded with blocked cells on both
// sides.

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "data/globals.h"
#include "golden/path_scenarios.h"
#include "role/path_finder.h"
#include "test.h"

using namespace swd3;

namespace
{

class LegacyPaths : public golden::PathBackend
{
public:
    std::vector<golden::u8> Find(const golden::PathScenario& scenario, bool again) override
    {
        constexpr std::size_t kPad = 0x1000;
        std::vector<std::int32_t> cells(scenario.cells.size() + 2 * kPad, -1);

        for (std::size_t i = 0; i < scenario.cells.size(); ++i)
        {
            cells[kPad + i] = static_cast<std::int32_t>(scenario.cells[i]);
        }

        G.mapBlocking = cells.data() + kPad;
        G.mapWidth = scenario.mapWidth;
        PathFinder finder;
        finder.SetMask(static_cast<int>(scenario.mask));
        std::vector<golden::u8> directions(0x200, 0xEE);
        std::vector<golden::u8> out;
        const int runs = again ? 2 : 1;

        for (int run = 0; run < runs; ++run)
        {
            const int result = finder.FindPath(scenario.fromX, scenario.fromY, scenario.toX, scenario.toY,
                                               scenario.width, scenario.height, directions.data());
            golden::Append32(out, static_cast<golden::u32>(result));

            for (const golden::u8 direction : directions)
            {
                out.push_back(direction);

                if (direction == 0xFF)
                {
                    break;
                }
            }
        }

        finder.Reset();
        golden::Append32(out, static_cast<golden::u32>(G.pathPoolFree));
        golden::Append32(out, static_cast<golden::u32>(G.pathPoolPeak));
        return out;
    }
};

}  // namespace

TEST_CASE("golden: legacy path finder")
{
    const char* output = std::getenv("GOLDEN_OUT");

    if (output == nullptr)
    {
        std::printf("  (GOLDEN_OUT not set, skipped)\n");
        return;
    }

    PathPoolInit();
    std::FILE* file = std::fopen(output, "w");
    LegacyPaths backend;
    golden::RunPaths(
        backend,
        [file](const std::string& name, golden::u64 hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    PathPoolRelease();
}
