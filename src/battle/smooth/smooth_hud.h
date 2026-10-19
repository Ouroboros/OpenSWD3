#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class BattleHud;
struct BattleAnimation;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the draws of the battle's HUD (BattleHud) split into the part deciding
// the frame and the part drawing it.
namespace swd3::smooth
{

class HudFrame
{
public:
    explicit HudFrame(BattleHud& hud);

    // BattleHud::FillColor: it holds nothing of the frame, so it is drawn as it is.
    void FillColor(int x, int y, int width, int height, u32 color, Scene& scene);

    void DrawAdvance(const Picture* picture, int x, int y, int width, int height, u32 mode, Scene& scene);

    int DrawPictureFrame0Advance(int id, int x, int y, Scene& scene);

    int DrawPortraitAdvance(u16 index, int x, int y, int dim, Scene& scene);

    int DrawArrowAdvance(int x, int y, int slot, Scene& scene);

    int DrawBarAdvance(int id, int frame, int x, int y, int width, Scene& scene);

    int DrawPictureFrameAdvance(int id, int frame, int x, int y, Scene& scene);

    int DrawGaugeAdvance(int id, int x, int y, int fill, Scene& scene);

    int DrawGaugeAltAdvance(int id, int x, int y, int fill, Scene& scene);

    int DrawGaugeWideAdvance(int id, int x, int y, u16 fill, Scene& scene);

    int DrawSmallNumberAdvance(int font, int value, int x, int y, Scene& scene);

    u16 DrawNumberDigitAdvance(u32 divisor, Scene& scene);

    int DrawNumberAdvance(i16 font, int value, int x, int y, Scene& scene);

    int DrawSlotAnimationAdvance(int id, int slot, int x, int y, Scene& scene);

    int DrawEffectAtAdvance(int id, int action, int x, int y, int mirror, Scene& scene);

    int DrawCursorAnimationAdvance(int id, int x, int y, Scene& scene);

    u16 DrawDigitAdvance(int font, int digit, int x, int y, Scene& scene);

    int DrawScrollBarAdvance(u16 id, int x, int y, int rows, int offset, int style, Scene& scene);

    int DrawCoinTossAdvance(Scene& scene);

    int DrawTimingBarAdvance(int x, int y, Scene& scene);

    int DrawTallGaugeAdvance(int x, int y, int level, Scene& scene);

    int DrawAnimationAdvance(BattleAnimation& animation, Scene& scene);

    int PlayAnimationShiftedAdvance(BattleAnimation& animation, int, int amount, Scene& scene);

    void DrawTileBoxAdvance(int id, int x, int y, int columns, int rows, i8 fill, Scene& scene);

private:
    BattleHud& hud;
};

}  // namespace swd3::smooth
