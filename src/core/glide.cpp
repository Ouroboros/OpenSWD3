#include "core/glide.h"

#include <algorithm>
#include <cstdlib>

namespace swd3
{

void Glide::Follow(i32 x, i32 y, u32 frame, bool continuous, u32 pace)
{
    if (!continuous)
    {
        *this = Glide{static_cast<double>(x), static_cast<double>(y), x, y, static_cast<double>(frame), 1.0, frame};
        return;
    }

    if (x == this->toX && y == this->toY)
    {
        return;
    }

    if (std::abs(x - this->toX) > Glide::SnapDistance || std::abs(y - this->toY) > Glide::SnapDistance)
    {
        *this = Glide{static_cast<double>(x), static_cast<double>(y), x, y, static_cast<double>(frame), 1.0, frame};
        return;
    }

    const std::array<double, 2> shown = this->Shown(static_cast<double>(frame));
    const u32 gap = frame - this->moved;
    this->fromX = shown[0];
    this->fromY = shown[1];
    this->toX = x;
    this->toY = y;
    this->start = static_cast<double>(frame);

    if (pace != 0)
    {
        this->frames = static_cast<double>(pace);
    }
    else if (gap >= 1 && gap <= Glide::MaxFrames)
    {
        this->frames = static_cast<double>(gap);
    }

    this->moved = frame;
}

void Glide::Aim(i32 x, i32 y, i32 nowX, i32 nowY, u32 frame, bool continuous, double frames)
{
    std::array<double, 2> shown = this->Shown(static_cast<double>(frame));

    if (!continuous || std::abs(shown[0] - nowX) > Glide::SnapDistance ||
        std::abs(shown[1] - nowY) > Glide::SnapDistance)
    {
        shown = {static_cast<double>(nowX), static_cast<double>(nowY)};
    }

    if (std::abs(x - nowX) > Glide::SnapDistance || std::abs(y - nowY) > Glide::SnapDistance)
    {
        x = nowX;
        y = nowY;
    }

    this->fromX = shown[0];
    this->fromY = shown[1];
    this->toX = x;
    this->toY = y;
    this->start = static_cast<double>(frame);
    this->frames = std::max(frames, Glide::ShortestStretch);

    if (x != nowX || y != nowY)
    {
        this->moved = frame;
    }
}

void Glide::Retarget(double time, i32 x, i32 y, double arrive)
{
    const std::array<double, 2> shown = this->Shown(time);
    this->fromX = shown[0];
    this->fromY = shown[1];
    this->toX = x;
    this->toY = y;
    this->start = time;
    this->frames = std::max(arrive - time, Glide::ShortestStretch);
}

std::array<double, 2> Glide::Shown(double time) const
{
    const double progress = std::clamp((time - this->start) / this->frames, 0.0, 1.0);
    return {
        this->fromX + (this->toX - this->fromX) * progress,
        this->fromY + (this->toY - this->fromY) * progress,
    };
}

}  // namespace swd3
