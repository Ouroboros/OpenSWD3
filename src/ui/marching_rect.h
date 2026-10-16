#pragma once

#include "core/types.h"

namespace swd3
{

class Painter;
class Surface;

// The selection frame of moving grey dashes; its shade phase advances with every frame drawn (0x004CC2D4).
class MarchingRect
{
public:
    void Draw(Surface& surface, const Painter& painter, int x, int y, int width, int height);

    i32 phase = 0;
};

// Every fourth pixel of every fourth row of a 640x480 screen into its first 160x120 pixels (the save thumbnail).
void ShrinkScreenQuarter(Surface& screen);

}  // namespace swd3
