#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

struct Picture;
class TswArchive;

// The rebuild's smooth battle (deviations.md): what the battle frame functions split into a part deciding the frame
// and a part drawing it hand over between them.

// A picture a view draws: its archive id and frame, taken again when drawn (a cached picture goes when the cache
// trims for another).
struct PictureRef
{
    u32 id = 0;
    u32 frame = 0;
    bool shown = false;

    const Picture* Get(TswArchive& archive) const;
};

// What IdleFrame draws of the unit, as its frame decides it (IdleAdvance) for IdleDraw: the body at (x, y) with its
// flags (mirrored: the mirror bits alone), the shadow, outlines, smear, blend, wave, barrier flash, grey and red
// copies, the gold over a gold guard, the thrown act and the overlay; thrownLost: the thrown act did not run (the
// frame ends after the body). `motion` is the rebuild's smooth battle reckoning.
struct IdleView
{
    PictureRef picture;
    int x = 0;
    int y = 0;
    u32 flags = 0;
    u32 mirrored = 0;
    bool body = false;
    bool shadow = false;
    int shadowX = 0;
    int shadowY = 0;
    int shadowTint = 0;
    bool whiteOutline = false;
    bool outline = false;
    bool smear = false;
    bool signal = false;
    int alpha = 0;
    int wave = 0;
    bool held = false;
    bool flash = false;
    int flashTint = 0;
    int flashX = 0;
    int flashY = 0;
    bool gray = false;
    bool red = false;
    bool gold = false;
    int goldValue = 0;
    int goldX = 0;
    int goldY = 0;
    PictureRef thrown;
    int thrownX = 0;
    int thrownY = 0;
    u32 thrownFlags = 0;
    bool thrownLost = false;
    PictureRef overlay;
    int overlayX = 0;
    int overlayY = 0;
    u32 overlayFlags = 0;
    std::array<int, 3> motion = {};
};

// What a walk (WalkToTarget, ReturnHome) draws, as its frame decides it: the picture at (x, y) with its flags, the
// shadow for the place at shadowY, the overlay of a walk home. `motion` is the rebuild's smooth battle reckoning.
struct WalkView
{
    PictureRef picture;
    int x = 0;
    int y = 0;
    u32 flags = 0;
    bool shadow = false;
    int shadowY = 0;
    int shadowTint = 0;
    PictureRef overlay;
    int overlayX = 0;
    int overlayY = 0;
    u32 overlayFlags = 0;
    std::array<int, 2> motion = {};
};

}  // namespace swd3
