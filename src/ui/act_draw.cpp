#include "ui/act_draw.h"

#include "draw/painter.h"
#include "pic/picture.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

constexpr u32 kTranslucent = 0x14;
constexpr u32 kTinted = 4;
constexpr int kGlowLimit = -12;
constexpr int kGlowBase = -25;

}  // namespace

ActDrawer::ActDrawer(ActArchive& acts, TswArchive& archive, Painter& painter)
    : acts(acts), archive(archive), painter(painter)
{
}

// swd32.exe 0x0040EBF0: steps the script and draws its picture at (x, y) minus the script's anchor, with the
// translucency of 'SG'
void ActDrawer::Draw(ActState& state, int x, int y)
{
    if (this->acts.Run(state) == 0)
    {
        return;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return;
    }

    this->painter.SetSource(picture->data.data());
    this->painter.Options().alpha = state.signal;
    this->painter.DrawPicture(x - state.x, y - state.y, picture->width, picture->height, state.flags);
}

// swd32.exe 0x00449C30: the picture tinted darker by `level`, then while `level` is below -12 a trail of copies
// spreading to both sides, each one step brighter
void ActDrawer::DrawGlow(ActState& state, int x, int y, int level)
{
    if (this->acts.Run(state) == 0)
    {
        return;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return;
    }

    auto tint = [this](int steps)
    {
        DrawOptions& options = this->painter.Options();
        options.tintBlue = steps;
        options.tintGreen = steps;
        options.tintRed = steps;
    };

    this->painter.SetSource(picture->data.data());
    tint(kGlowBase - level);
    this->painter.DrawPicture(x - state.x, y - state.y, picture->width, picture->height, kTinted);
    int step = level;

    for (int offset = kGlowLimit - level; step < kGlowLimit; --offset)
    {
        tint(step);
        this->painter.DrawPicture(x + offset - state.x, y - state.y, picture->width, picture->height, kTinted);
        tint(step);
        this->painter.DrawPicture(x - offset / 2 - state.x, y - state.y, picture->width, picture->height, kTinted);
        ++step;
    }
}

// swd32.exe 0x004117F0: `value` in the digits of picture 0x2354 leftwards from `right` (each digit 2 pixels apart),
// then the label 0x245E before it, all translucent at `alpha`
void ActDrawer::DrawCounter(int right, int y, u32 value, int alpha)
{
    int x = right;

    do
    {
        const Picture* digit = this->archive.GetPicture(ActDrawer::kDigits, value % 10);

        if (digit != nullptr)
        {
            x += -2 - digit->width;
            this->painter.SetSource(digit->data.data());
            this->painter.Options().alpha = alpha;
            this->painter.DrawPicture(x, y + 1, digit->width, digit->height, kTranslucent);
        }

        value /= 10;
    } while (value != 0);

    const Picture* label = this->archive.GetPicture(ActDrawer::kCounterLabel, 0);

    if (label != nullptr)
    {
        this->painter.SetSource(label->data.data());
        this->painter.Options().alpha = alpha;
        this->painter.DrawPicture(x - 0x28, y - 8, label->width, label->height, kTranslucent);
    }
}

// swd32.exe 0x0043B080: translucent at `alpha`, keeping only the mirror bits of the script's mode
void ActDrawer::DrawGhost(ActState& state, int x, int y, int alpha)
{
    if (this->acts.Run(state) == 0)
    {
        return;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return;
    }

    this->painter.SetSource(picture->data.data());
    this->painter.Options().alpha = alpha;
    this->painter.DrawPicture(
        x - state.x,
        y - state.y,
        picture->width,
        picture->height,
        (state.flags & 0x80000003u) | kTranslucent
    );
}

// swd32.exe 0x0040ECC0: `mode` with the mirror bits of the script's mode, at the translucency of 'SG'
void ActDrawer::DrawWithMode(ActState& state, int x, int y, u32 mode)
{
    if (this->acts.Run(state) == 0)
    {
        return;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return;
    }

    this->painter.SetSource(picture->data.data());
    this->painter.Options().alpha = state.signal;
    this->painter
        .DrawPicture(x - state.x, y - state.y, picture->width, picture->height, mode | (state.flags & 0x80000003u));
}

}  // namespace swd3
