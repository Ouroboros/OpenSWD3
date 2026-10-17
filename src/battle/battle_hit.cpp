#include "game/game.h"

namespace swd3
{

namespace
{

constexpr int kHitPanCentre = 0x140;  // the sound pans right from the middle of the field
constexpr int kHitPan = 0x10;
constexpr i8 kHitFadeEnd = -0x20;
constexpr i8 kHitFadeStep = 4;

}  // namespace

// swd32.exe 0x004599B0: one frame of act `act` on `unit` (unit `index`, its hit act unk5202A8); 1 once the act waits
// (cleared) or ends (the follow act unk4FE600 cleared instead). A quaking act quakes the screen. The picture is read
// straight from its file each frame, mirrored with the sides, at the unit's offset from its origin (or its draw
// place), with the act's sound panned by the side.
int Game::PlayHitAct(BattleUnit& unit, u32 act, int index)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];

    if ((hit.target & ActState::Quakes) != 0)
    {
        battle.quakePhase = 0;
        battle.quaking = 1;
    }

    if (hit.waiting == 0)
    {
        hit.id = act;
        hit.action = 0;
        hit.hold = 0;

        if (battle.paused == 1)
        {
            hit.hold = 1;
        }

        if (this->acts.Run(hit) == 0)
        {
            battle.unk4FE600[at] = ActState{};
            battle.unk4FF0BC[at] = 0;
            return 1;
        }

        const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(hit.frame, hit.picture);
        u32 flags = hit.flags;
        int originX = hit.x;

        // A picture that does not load is read through a null pointer in the original.
        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());

            if (battle.mirrored == 1)
            {
                flags ^= Painter::Mirror;
                originX = picture->width - originX;
            }
        }

        u16 offsetX = 0;
        u16 offsetY = 0;
        u16 x = 0;
        u16 y = 0;
        unit.GetOffset(offsetX, offsetY);

        if (offsetX != 0 && offsetY != 0)
        {
            unit.GetOrigin(x, y);
            x = static_cast<u16>(x + offsetX);
            y = static_cast<u16>(y + offsetY);
        }
        else
        {
            unit.GetDrawPosition(x, y);
        }

        y = static_cast<u16>(y - static_cast<u16>(hit.y));
        x = static_cast<u16>(x - originX);
        this->audio.PlaySound(hit.view, this->config.soundVolume);
        this->audio.SetSoundPan(hit.view, originX + static_cast<i16>(x) >= kHitPanCentre ? kHitPan : -kHitPan);
        hit.view = 0;

        if (picture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(x),
                static_cast<i16>(y),
                picture->width,
                picture->height,
                flags,
                PaletteArgument(*picture)
            );
        }
    }

    if (hit.waiting != 1)
    {
        return 0;
    }

    hit = ActState{};
    return 1;
}

// swd32.exe 0x00459BF0: one frame of act `act` (action `action`) fading on `unit` (unit `index`, its act
// unk524980): 1 at once without an act, or once the fade (unk521374, darker by kHitFadeStep a frame) is past
// kHitFadeEnd (reset to 0); 0 when the act ends.
int Game::FadeHitAct(BattleUnit& unit, u32 act, i32 action, int index)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);

    if (act == 0)
    {
        return 1;
    }

    i8& fade = battle.unk521374[at];

    if (fade <= kHitFadeEnd)
    {
        fade = 0;
        return 1;
    }

    u16 x = 0;
    u16 y = 0;
    unit.GetDrawPosition(x, y);
    ActState& effect = battle.unk524980[at];
    effect.id = act;
    effect.action = action;
    effect.hold = 0;

    if (battle.paused == 1)
    {
        effect.hold = 1;
    }

    if (this->acts.Run(effect) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(effect.frame, effect.picture);

    // A missing picture is read through a null pointer in the original.
    if (picture != nullptr)
    {
        painter.SetSource(picture->data.data());
        DrawOptions& options = painter.Options();
        options.tintRed = fade;
        options.tintGreen = fade;
        options.tintBlue = fade;
        painter.DrawPicture(
            static_cast<i16>(x) - effect.x,
            static_cast<i16>(y) - effect.y,
            picture->width,
            picture->height,
            effect.flags,
            PaletteArgument(*picture)
        );
    }

    fade = static_cast<i8>(fade - kHitFadeStep);
    return 0;
}

// swd32.exe 0x0045D810: the point unit `index` reaches walking the line from (fromX, fromY) to (toX, toY), `speed`
// steps more each call (unk4FDF94 counts the calls); 1 once it reaches toX (the count reset).
int Game::WalkLine(i32& x, i32& y, int fromX, int fromY, int toX, int toY, int speed, int index)
{
    BattleState& battle = this->battleState;
    u16& calls = battle.unk4FDF94[static_cast<std::size_t>(index)];
    LineWalker walker;
    walker.fromX = fromX;
    walker.fromY = fromY;
    walker.toX = toX;
    walker.toY = toY;
    ++calls;
    const int steps = speed * calls;
    bool reached = false;

    for (int step = 0; step < steps; ++step)
    {
        LineStep(walker);

        if (walker.fromX + walker.x == walker.toX)
        {
            reached = true;
            break;
        }
    }

    x = walker.fromX + walker.x;
    y = walker.fromY + walker.y;

    if (!reached)
    {
        return 0;
    }

    calls = 0;
    return 1;
}

}  // namespace swd3
