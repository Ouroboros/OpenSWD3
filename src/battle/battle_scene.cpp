#include <algorithm>
#include <array>
#include <span>

#include "game/game.h"
#include "pic/picture_ops.h"

namespace swd3
{

namespace
{

constexpr std::size_t kFieldPixels = static_cast<std::size_t>(Display::Width) * BattleState::FieldHeight;

void CopySurface(const Surface& source, Surface& target)
{
    for (int row = 0; row < std::min(source.Height(), target.Height()); ++row)
    {
        std::copy_n(source.Row(row), std::min(source.Width(), target.Width()), target.Row(row));
    }
}

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
// quake is cleared and the flash applied. While unit unk4A7630 acts with the field darkened, the field is drawn darker
// by a level a few frames (a darkened copy, or tint steps), and when the darkening ends it is undone a level a frame.
void Game::DrawBattleBackground(int shake)
{
    BattleState& battle = this->battleState;
    Picture& background = battle.background;
    const std::span<u16> field(this->drawSurface, kFieldPixels);

    auto drawBackground = [this, &background](u32 mode)
    {
        this->painter.SetSource(background.data.data());
        this->painter.DrawPicture(0, 0, background.width, background.height, mode);
    };

    this->painter.SetSource(background.data.data());
    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);

    if (battle.unk53BF94 == 0 && battle.unk53BF98 == 0)
    {
        if (shake != 0)
        {
            this->ShakeBattleBackground(shake);
            drawBackground(Painter::Copy);
            this->battleHud.PlayAnimationShifted(battle.backgroundAnimation, 0, shake);
        }
        else
        {
            drawBackground(Painter::Copy);
            this->battleHud.DrawAnimation(battle.backgroundAnimation);

            if (battle.grayBandShown == 1)
            {
                u16 band = battle.grayBand;

                if (band < BattleState::BandFull)
                {
                    band = band < BattleState::BandFastFrom ? static_cast<u16>(band << 1)
                                                            : static_cast<u16>(band + BattleState::BandStep);
                    battle.grayBand = band;
                }

                this->painter.SetClipClamped(0, BattleState::BandRow - band, Display::Width, BattleState::BandRow);
                drawBackground(Painter::GrayUnder);
                this->painter
                    .SetClipClamped(0, BattleState::BandRow, Display::Width, battle.grayBand + BattleState::BandRow);
                drawBackground(Painter::GrayUnder);
            }
        }

        battle.shake = 0;

        if (battle.flashing == 1)
        {
            DrawOptions& options = this->painter.Options();
            const int level = battle.flashLevel;
            options.tintRed = level;
            options.tintGreen = level;
            options.tintBlue = level;
            this->painter.ShiftRed(field, level);
            this->painter.ShiftGreenPairs(field, options.tintGreen);
            this->painter.ShiftBluePairs(field, options.tintBlue);
            battle.flashLevel = static_cast<i8>(battle.flashLevel - BattleState::FlashStep);

            if (battle.flashLevel == 0)
            {
                battle.flashLevel = BattleState::FlashStart;
                battle.flashing = 0;
            }
        }
    }

    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    i16 level = battle.darkLevel;

    if (battle.unk4A7630 == battle.unk53AE70 && (battle.unk53BF94 == 1 || battle.unk53BF98 == 1))
    {
        if (battle.darkenByTint != 0)
        {
            if (shake != 0)
            {
                this->ShakeBattleBackground(shake);
                this->battleHud.PlayAnimationShifted(battle.backgroundAnimation, 0, shake);
                battle.shake = 0;
            }

            drawBackground(Painter::Copy);
            this->painter.TintPixels(
                field,
                battle.darkLevel * (battle.darkRed >> 1),
                battle.darkLevel * (battle.darkGreen >> 1),
                battle.darkLevel * (battle.darkBlue >> 1)
            );
            level = battle.darkLevel;
        }
        else if (level >= 1)
        {
            CopySurface(*battle.surfaces[static_cast<std::size_t>(level - 1)], *this->offscreen);
            level = battle.darkLevel;
        }

        i32 frames = battle.darkFrames;

        if (frames > BattleState::DarkLevelFrames)
        {
            frames = 0;
            ++level;
            battle.darkLevel = level;

            if (level > BattleState::MaxDarkLevel)
            {
                level = BattleState::MaxDarkLevel;
                battle.darkLevel = level;
            }
        }

        battle.darkFrames = frames + 1;
    }

    if (battle.unk53BFEC != 1 || battle.unk53C030 != 0)
    {
        return;
    }

    if (level < 1)
    {
        battle.darkRed = 0;
        battle.darkGreen = 0;
        battle.darkBlue = 0;
        battle.darkLevel = 0;
        battle.unk4A7630 = -1;
        battle.unk53BF98 = 0;
        battle.unk53BF94 = 0;
        battle.darkenByTint = 0;
        battle.unk53BFEC = 0;
        return;
    }

    --level;
    battle.darkLevel = level;

    if (battle.unk4A7574 != -1 && battle.darkenByTint == 0)
    {
        // Level 0 reads the null pointer before the copies (0x005244C0): the blit fails and nothing is drawn.
        if (level >= 1)
        {
            CopySurface(*battle.surfaces[static_cast<std::size_t>(level - 1)], *this->offscreen);
        }

        return;
    }

    drawBackground(Painter::Copy);
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
        this->AudioPump();
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
    battle.unk4A7574 = battle.unk53BD0C;
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);
}
}  // namespace swd3
