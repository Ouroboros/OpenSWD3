#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class Game;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the background, the role panels and the battle frame itself
// (battle_scene.cpp, battle_panels.cpp, battle_frame.cpp) split into the part deciding the frame and the part drawing
// it.
namespace swd3::smooth
{

class BattleFrame
{
public:
    explicit BattleFrame(Game& game);

    void DrawBattleBackgroundAdvance(int shake, Scene& scene);

    void DrawRolePanelsAdvance(Scene& scene);

private:
    template <typename Next>
    void StepPanelGaugeAdvance(
        i32& shown,
        i32& target,
        Next next,
        int picture,
        int x,
        int y,
        i32 downNear,
        i32 downFar,
        Scene& scene
    );

    Game& game;
};

}  // namespace swd3::smooth
