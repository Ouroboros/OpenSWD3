#include "game/direction.h"

#include <cmath>
#include <cstdlib>

#include "crt/msvc.h"

namespace swd3
{

namespace
{

constexpr int kQuadrant = 0x5A;
constexpr int kStep = 5;
constexpr int kFarthest = 0x2328;
constexpr int kSineShift = 13;  // kSineTable holds sines times 1 << kSineShift
constexpr int kLastDegree = kQuadrant - 1;
constexpr int kFullTurn = 4 * kQuadrant;
constexpr int kSectors = 16;

}  // namespace

// swd32.exe 0x00411E20: the angle in degrees (a multiple of 5) from (toX, toY) to (fromX, fromY), found quadrant by
// quadrant in the sine table (the last of equally close entries wins); 0 when the points coincide
int AngleBetween(int fromX, int fromY, int toX, int toY, i32& distance)
{
    const int dx = fromX - toX;
    const int dy = fromY - toY;
    const auto length = static_cast<i32>(crt::Ftol(std::sqrt(static_cast<double>(dx * dx + dy * dy))));
    distance = length;

    if (length == 0)
    {
        return 0;
    }

    const int sine = std::abs((dy << kSineShift) / length);
    int base = 0;
    int index = 0;
    int step = kStep;

    if (dy < 0)
    {
        base = kQuadrant;
        index = kLastDegree;
        step = -kStep;

        if (dx >= 0)
        {
            base = 0;
            index = 0;
            step = kStep;
        }
    }
    else
    {
        base = 2 * kQuadrant;
        index = 0;
        step = kStep;

        if (dx >= 0)
        {
            base = 3 * kQuadrant;
            index = kLastDegree;
            step = -kStep;
        }
    }

    int best = kFarthest;
    int angle = 0;

    for (int candidate = base; candidate < base + kQuadrant; candidate += kStep)
    {
        const int difference = std::abs(sine - kSineTable[static_cast<std::size_t>(index)]);

        if (difference <= best)
        {
            best = difference;
            angle = candidate;
        }

        index += step;
    }

    return angle;
}

// swd32.exe 0x00411F00: the 16 sectors of 22.5 degrees as direction codes
int AngleToDirection(int angle)
{
    constexpr std::array<int, 17> kDirections = {3, 7, 7, 0, 0, 4, 4, 2, 2, 6, 6, 1, 1, 5, 5, 3, 0};
    return kDirections[static_cast<std::size_t>(kSectors * angle / kFullTurn)];
}

}  // namespace swd3
