#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class Game;
class BattleRole;
class BattleEnemy;
class BattleUnit;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the spell and attack frames of battle_damage.cpp split into the part
// deciding the frame and the part drawing it.
namespace swd3::smooth
{

class DamageFrame
{
public:
    explicit DamageFrame(Game& game);

    int DrainSpellFrameAdvance(BattleRole& role, BattleUnit& target, Scene& scene);

    int MagicAttackFrameAdvance(BattleRole& role, BattleUnit& target, Scene& scene);

    int EnemyAttackFrameAdvance(BattleUnit& enemy, BattleUnit& target, Scene& scene);

private:
    Game& game;
};

}  // namespace swd3::smooth
