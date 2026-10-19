#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

struct Picture;
class TswArchive;
class BattleUnit;
struct BattleServices;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the battle frame functions split into a part deciding the frame and a
// part drawing it, and what they hand over between them. The original functions run both parts in the original's
// order; the smooth battle draws again between frames.
namespace swd3::smooth
{

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

// The parts of the frame functions of `unit` (IdleFrame, WalkToTarget, ReturnHome): what a frame decides into a view,
// and the view drawn.
class UnitFrame
{
public:
    explicit UnitFrame(BattleUnit& unit);

    // IdleFrame: false when the unit is not drawn (hidden, staggering, busy, fading, its act or picture missing).
    bool IdleAdvance(BattleServices& services, int paused, IdleView& view);
    static void IdleDraw(BattleServices& services, const IdleView& view);
    // WalkToTarget: false when it draws nothing.
    bool WalkAdvance(BattleServices& services, const BattleUnit& target, WalkView& view);
    // ReturnHome: its result; the view keeps no picture when it draws nothing.
    int HomeAdvance(BattleServices& services, const BattleUnit& target, i32 id, i32 action, WalkView& view);
    static void DrawWalk(BattleServices& services, const WalkView& view);

private:
    int FlashStep();
    void ChooseOverlay(BattleServices& services, u16 frame, u16 picture, int offsetX, int offsetY, WalkView& view);

    BattleUnit& unit;
};

}  // namespace swd3::smooth
