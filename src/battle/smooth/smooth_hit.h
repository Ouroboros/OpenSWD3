#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class BattleUnit;
class Game;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the hit frames of the battle (battle_hit.cpp) split into the part
// deciding the frame and the part drawing it.
namespace swd3::smooth
{

class HitFrame
{
public:
    explicit HitFrame(Game& game);

    int PlayHitActAdvance(BattleUnit& unit, u32 act, int index, Scene& scene);

    int FadeHitActAdvance(BattleUnit& unit, u32 act, i32 action, int index, Scene& scene);

    bool DrawHitPictureAdvance(
        BattleUnit& target,
        BattleUnit& attacker,
        u32 act,
        int index,
        u16& x,
        u16& y,
        int& originX,
        Scene& scene
    );

    void RippleHitFrameAdvance(BattleUnit& target, BattleUnit& attacker, int index, u16& targetX, Scene& scene);

    int HitTargetFrameAdvance(BattleUnit& target, BattleUnit& attacker, u32 act, int index, Scene& scene);

    int HitAllFrameAdvance(BattleUnit& target, BattleUnit& attacker, u32 act, int index, int all, Scene& scene);

private:
    Game& game;
};

}  // namespace swd3::smooth
