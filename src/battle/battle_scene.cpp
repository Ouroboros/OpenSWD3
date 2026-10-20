#include <algorithm>
#include <array>
#include <span>

#include "battle/battle_frame_constants.h"
#include "battle/smooth/smooth_battle.h"
#include "game/game.h"
#include "pic/picture_ops.h"

namespace swd3
{

namespace
{

}  // namespace

// The background shifted by `shake` (right for a positive value), with its animation shifted along.
void Game::ShakeBattleBackground(int shake)
{
    BattleState& battle = this->battleState;

    if (shake > 0)
    {
        ShiftPicture(battle.background.data, ShiftMode::Right, shake);
    }
    else
    {
        ShiftPicture(battle.background.data, ShiftMode::Left, -shake);
    }
}

// swd32.exe 0x00453580: the battle background of the frame. Unless the field is darkened it is drawn (shifted by
// `shake` with its animation, or animated in place with the grey band of the battle start growing from BandRow), the
// quake is cleared and the flash applied. While unit darkenUnit acts with the field darkened, the field is drawn darker
// by a level a few frames (a darkened copy, or tint steps), and when the darkening ends it is undone a level a frame.
void Game::DrawBattleBackground(int shake)
{
    smooth::Scene scene;
    smooth::BattleFrame(*this).DrawBattleBackgroundAdvance(shake, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x0045AF90: when the darkening tint has changed, the darkened copies of the field are drawn again: copy
// `level` is the background shifted by `level` times half the tint. The field is then darkened from level 1, and the
// frame is the painter's target again.
void Game::PrepareDarkenedCopies()
{
    BattleState& battle = this->battleState;
    const std::array<i16, 3> tint = {battle.darkRed, battle.darkGreen, battle.darkBlue};

    if (battle.darkCopyTint == tint)
    {
        return;
    }

    int level = 1;

    for (std::unique_ptr<Surface>& copy : battle.surfaces)
    {
        this->audioDevice.Serve();
        this->drawSurface = this->painter.LockSurface(*copy);
        this->painter.SetTarget(this->drawSurface);
        this->painter.UnlockSurface(*copy);
        this->painter.SetSource(battle.background.data.data());
        this->painter.DrawPicture(0, 0, Display::Width, Display::Height, Painter::Copy);
        const std::span<u16> field(this->drawSurface, kFieldPixels);
        this->painter.ShiftRed(field, level * (battle.darkRed >> 1));
        this->painter.ShiftGreenPairs(field, level * (battle.darkGreen >> 1));
        this->painter.ShiftBluePairs(field, level * (battle.darkBlue >> 1));
        ++level;
    }

    battle.darkCopyTint = tint;
    battle.darkLevel = 1;
    battle.darkenedAt = battle.unk53BD0C;
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);
}
}  // namespace swd3
