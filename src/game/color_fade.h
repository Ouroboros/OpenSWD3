#pragma once

#include "core/types.h"

namespace swd3
{

// A colour fade: the steps added each frame, the levels reached and the levels it fades to (start*, restored once the
// frames run out).
struct ColorFade
{
    i32 frames = 0;
    float stepRed = 0;
    float stepGreen = 0;
    float stepBlue = 0;
    float red = 0;
    float green = 0;
    float blue = 0;
    float startRed = 0;
    float startGreen = 0;
    float startBlue = 0;
};

}  // namespace swd3
