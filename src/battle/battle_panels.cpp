#include <array>
#include <cstring>
#include <string_view>

#include "battle/battle_frame_constants.h"
#include "battle/smooth/smooth_battle.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "ui/ui_draw.h"

namespace swd3
{

// swd32.exe 0x00459D10: the battle panels. The units with autoControl set are listed in boxes (name, hit points, gauge;
// the last one glows while listGlow is on). Every other role in battle that is not guarding gets its panel: the
// highlight of the current role, the name or its gauge, the portrait and effect icons, the block gauge (with the
// arrow), and the hit point, spirit and stamina gauges and numbers moving towards their values. With the automatic
// battle on, its box slides in.
void Game::DrawRolePanels()
{
    smooth::Scene scene;
    smooth::BattleFrame(*this).DrawRolePanelsAdvance(scene);
    scene.Show(this->battleServices);
}

}  // namespace swd3
