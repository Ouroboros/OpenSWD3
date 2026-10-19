#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class BattleRole;
class BattleUnit;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the frame functions of the party roles split into the part deciding
// the frame and the part drawing it.
namespace swd3::smooth
{

class RoleFrame
{
public:
    explicit RoleFrame(BattleRole& role);

    // BattleRole::AttackFrame: its result.
    int AttackAdvance(BattleServices& services, BattleUnit& target, int slot, Scene& scene);

    int CastAtAdvance(BattleServices& services, const BattleUnit& target, i32 spell, Scene& scene);

    int ThrowActAdvance(BattleServices& services, const BattleUnit& target, Scene& scene);

    int StepBackAdvance(BattleServices& services, int mode, Scene& scene);

    int GlowActAdvance(BattleServices& services, int x, int y, Scene& scene);

    int CatchActAdvance(BattleServices& services, const BattleUnit& target, Scene& scene);

    u16 AttackPoseAdvance(BattleServices& services, Scene& scene);

    int PoseAtTargetAdvance(BattleServices& services, const BattleUnit& target, Scene& scene);

    void FlyExtraActAdvance(
        BattleServices& services,
        i32 id,
        i32 action,
        int slot,
        i16 x,
        i16 y,
        int steps,
        Scene& scene
    );

    int DirectAttackAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int ChargeOnTargetAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int MagicFrameAdvance(BattleServices& services, const BattleUnit& target, Scene& scene);

    int ChargeAttackFrameAdvance(BattleServices& services, BattleUnit& target, int slot, Scene& scene);

    int BoomerangFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int SwarmSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int BeamSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int LeapSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene);

    int AbsorbFrameAdvance(BattleServices& services, Scene& scene);

private:
    BattleRole& role;
};

}  // namespace swd3::smooth
