#include "ui/act_draw.h"

#include "core/debug_log.h"
#include "draw/painter.h"
#include "pic/picture.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

constexpr u32 kTranslucent = Painter::Translucent;
constexpr u32 kTinted = Painter::Additive;
constexpr int kGlowLimit = -12;
constexpr int kGlowBase = -25;
constexpr u32 kDecimal = 10;
constexpr int kDigitGap = 2;
constexpr int kLabelLeft = 0x28;
constexpr int kLabelUp = 8;

}  // namespace

ActDrawer::ActDrawer(ActArchive& acts, TswArchive& archive, Painter& painter)
    : acts(acts),
      archive(archive),
      painter(painter)
{
}

// swd32.exe 0x0040EBF0: steps the script and draws its picture at (x, y) minus the script's anchor, with the
// translucency of 'SG'
void ActDrawer::Draw(ActState& state, int x, int y)
{
    if (this->acts.Run(state) == 0)
    {
        DebugLog(
            u8"Act Err(Misc::DrawActData) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
            state.id,
            state.action,
            state.direction
        );
        return;
    }

    this->DrawShown(state, x, y);
}

// The rebuild's smooth motion: the picture `state` last ran to drawn at (x, y) as Draw draws it, the act not stepped.
void ActDrawer::DrawShown(const ActState& state, int x, int y)
{
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
        DebugLog(
            u8"Act Err(Menu::DrawActAlpha) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
            state.id,
            state.action,
            state.direction
        );  // 0x004A6B1C + 0x0049FD0C
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
        const Picture* digit = this->archive.GetPicture(ActDrawer::Digits, value % kDecimal);

        if (digit != nullptr)
        {
            x += -kDigitGap - digit->width;
            this->painter.SetSource(digit->data.data());
            this->painter.Options().alpha = alpha;
            this->painter.DrawPicture(x, y + 1, digit->width, digit->height, kTranslucent);
        }

        value /= kDecimal;
    } while (value != 0);

    const Picture* label = this->archive.GetPicture(ActDrawer::CounterLabel, 0);

    if (label != nullptr)
    {
        this->painter.SetSource(label->data.data());
        this->painter.Options().alpha = alpha;
        this->painter.DrawPicture(x - kLabelLeft, y - kLabelUp, label->width, label->height, kTranslucent);
    }
}

// swd32.exe 0x0043B080: translucent at `alpha`, keeping only the mirror bits of the script's mode
void ActDrawer::DrawGhost(ActState& state, int x, int y, int alpha, double alphaFraction)
{
    if (this->acts.Run(state) == 0)
    {
        DebugLog(
            u8"Act Err(Menu::DrawActGhost) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
            state.id,
            state.action,
            state.direction
        );  // 0x0049FD0C
        return;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return;
    }

    this->painter.SetSource(picture->data.data());
    this->painter.Options().alpha = alpha;
    this->painter.Options().alphaFraction = alphaFraction;
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
        DebugLog(
            u8"Act Err(Misc::DrawActData) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
            state.id,
            state.action,
            state.direction
        );
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

// swd32.exe 0x0040EC80: picture `picture` of `file` at (x, y), opaque
void ActDrawer::DrawPicture(u16 file, u16 picture, int x, int y)
{
    const Picture* source = this->archive.GetPicture(file, picture);

    if (source == nullptr)
    {
        return;
    }

    this->painter.SetSource(source->data.data());
    this->painter.DrawPicture(x, y, source->width, source->height, 0);
}

}  // namespace swd3
