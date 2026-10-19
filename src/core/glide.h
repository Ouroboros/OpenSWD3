#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

// The rebuild's smooth motion (deviations.md): how something the game frames draw at whole places is shown moving to
// where they drew it: from `fromX`, `fromY` to `toX`, `toY` over `frames` game frames from frame `start`, `frames`
// being the frames between its last two moves (a role that steps every few frames glides at an even pace instead of
// moving in one frame and standing in the others).
struct Glide
{
    static constexpr i32 SnapDistance = 0x80;  // moving further in a frame jumps there
    static constexpr u32 MaxFrames = 8;        // longer pauses between moves keep the last pace

    double fromX = 0.0;
    double fromY = 0.0;
    i32 toX = 0;
    i32 toY = 0;
    u32 start = 0;
    u32 frames = 1;
    u32 moved = 0;  // the frame of its last move

    // The place game frame `frame` drew; `continuous` when the frame before was followed too, `pace` the frames to
    // the next move when known (0 takes the frames since the last move).
    void Follow(i32 x, i32 y, u32 frame, bool continuous, u32 pace);
    // Where it is shown at `time` (game frames: the last frame plus how far the next is).
    std::array<double, 2> Shown(double time) const;
};

}  // namespace swd3
