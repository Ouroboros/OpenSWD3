#include "core/glide.h"

#include <algorithm>
#include <cstdlib>

namespace swd3
{

void Glide::Follow(i32 x, i32 y, u32 frame, bool continuous, u32 pace)
{
    if (!continuous)
    {
        *this = Glide{static_cast<double>(x), static_cast<double>(y), x, y, frame, 1, frame};
        return;
    }

    if (x == this->toX && y == this->toY)
    {
        return;
    }

    if (std::abs(x - this->toX) > Glide::SnapDistance || std::abs(y - this->toY) > Glide::SnapDistance)
    {
        *this = Glide{static_cast<double>(x), static_cast<double>(y), x, y, frame, 1, frame};
        return;
    }

    const std::array<double, 2> shown = this->Shown(static_cast<double>(frame));
    const u32 gap = frame - this->moved;
    this->fromX = shown[0];
    this->fromY = shown[1];
    this->toX = x;
    this->toY = y;
    this->start = frame;

    if (pace != 0)
    {
        this->frames = pace;
    }
    else if (gap >= 1 && gap <= Glide::MaxFrames)
    {
        this->frames = gap;
    }

    this->moved = frame;
}

std::array<double, 2> Glide::Shown(double time) const
{
    const double progress =
        std::clamp((time - static_cast<double>(this->start)) / static_cast<double>(this->frames), 0.0, 1.0);
    return {
        this->fromX + (this->toX - this->fromX) * progress,
        this->fromY + (this->toY - this->fromY) * progress,
    };
}

}  // namespace swd3
