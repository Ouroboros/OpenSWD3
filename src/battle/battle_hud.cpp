#include "battle/battle_hud.h"

#include "audio/audio_manager.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "pic/picture.h"
#include "pic/picture_ops.h"
#include "random/random.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

constexpr int kPortraitAct = 0x2390;
constexpr int kArrowAct = 0x2392;
constexpr u32 kOutlineGreen = 0x07E007E0;
constexpr u32 kTintFlag = 0x10;
constexpr u32 kTranslucent = 0x14;
constexpr u32 kScaledGlow = 0x20;
constexpr u32 kRawGradient = 0x08;
constexpr u16 kDigitGlow4000 = 0x4000;
constexpr u16 kDigitGlow8000 = 0x8000;
constexpr u32 kDigitFont0 = 0x2359;
constexpr u32 kDigitFont1 = 0x2358;
constexpr u16 kCoinAct = 0x2329;
constexpr u32 kTimingPicture = 0x234F;
constexpr u32 kTallGauge = 0x241A;
constexpr int kScreenWidth = 0x280;
constexpr int kScreenHeight = 0x1E0;
constexpr int kVisibleRows = 7;

}  // namespace

BattleHud::BattleHud(Painter& painter, TswArchive& archive, ActArchive& acts, Random& random, AudioManager& audio)
    : painter(painter), archive(archive), acts(acts), random(random), audio(audio)
{
}

// Picture `frame` of `id` as the source and the last picture.
const Picture* BattleHud::Select(int id, int frame)
{
    const Picture* picture = this->archive.GetPicture(static_cast<u32>(id), static_cast<u32>(frame));
    this->lastPicture = picture;

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
    }

    return picture;
}

void BattleHud::Draw(const Picture* picture, int x, int y, int width, int height, u32 mode)
{
    if (picture != nullptr)
    {
        this->painter.DrawPicture(x, y, width, height, mode);
    }
}

// swd32.exe 0x00450270
int BattleHud::DrawPictureFrame0(int id, int x, int y)
{
    const Picture* picture = this->Select(id, 0);
    return picture != nullptr ? this->painter.DrawPicture(x, y, picture->width, picture->height, 0) : 0;
}

// swd32.exe 0x004502B0: portrait action `index` of act 0x2390, outlined in green when highlighted, darkened when `dim`
// is 1
int BattleHud::DrawPortrait(u16 index, int x, int y, int dim)
{
    ActState state;
    state.id = kPortraitAct;
    state.action = index;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->Select(state.frame, state.picture);

    if (picture == nullptr)
    {
        return 0;
    }

    if (this->portraitHighlight[index] == 1)
    {
        DrawArgument outline;
        outline.colorPair = kOutlineGreen;
        this->painter.DrawOutline(x - state.x, y - state.y, picture->width, picture->height, 0, outline);
        picture = this->lastPicture;
    }

    this->painter.DrawPicture(x - state.x, y - state.y, picture->width, picture->height, 0);

    if (dim != 1)
    {
        return dim;
    }

    DrawOptions& options = this->painter.Options();
    options.tintGreen = -10;
    options.tintBlue = -10;
    options.tintRed = 0;
    const Picture* current = this->lastPicture;
    return this->painter
        .DrawPicture(x - state.x, y - state.y, current->width, current->height, state.flags | kTintFlag);
}

// swd32.exe 0x00450400: the looping arrow (act 0x2392) of one of four slots
int BattleHud::DrawArrow(int x, int y, int slot)
{
    ActState& state = this->arrowActs[static_cast<std::size_t>(slot)];
    state.id = kArrowAct;
    state.action = 0;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->Select(state.frame, state.picture);

    if (picture == nullptr)
    {
        return 0;
    }

    return this->painter.DrawPicture(x - state.x, y - state.y, picture->width, picture->height, state.flags);
}

// swd32.exe 0x00450490: only positive widths are drawn
int BattleHud::DrawBar(int id, int frame, int x, int y, int width)
{
    const Picture* picture = this->Select(id, frame);

    if (picture != nullptr && width > 0)
    {
        return this->painter.DrawPicture(x, y, width, picture->height, 0);
    }

    return picture != nullptr ? 1 : 0;
}

// swd32.exe 0x004504E0
int BattleHud::DrawPictureFrame(int id, int frame, int x, int y)
{
    const Picture* picture = this->Select(id, frame);
    return picture != nullptr ? this->painter.DrawPicture(x, y, picture->width, picture->height, 0) : 0;
}

// swd32.exe 0x00450530: frame 0 as the background, `fill` pixels of frame 1 over it
int BattleHud::DrawGauge(int id, int x, int y, int fill)
{
    const Picture* back = this->Select(id, 0);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, 1);

    if (front != nullptr && fill != 0)
    {
        return this->painter.DrawPicture(x, y, fill, front->height, 0);
    }

    return front != nullptr ? 1 : 0;
}

// swd32.exe 0x004505B0: as DrawGauge with frame 2
int BattleHud::DrawGaugeAlt(int id, int x, int y, int fill)
{
    const Picture* back = this->Select(id, 0);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, 2);

    if (front != nullptr && fill != 0)
    {
        this->painter.DrawPicture(x, y, fill, front->height, 0);
    }

    return 1;
}

// swd32.exe 0x00450630: frame 1 drawn fill + 2 pixels wide (also when fill is 0)
int BattleHud::DrawGaugeWide(int id, int x, int y, u16 fill)
{
    const Picture* back = this->Select(id, 0);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, 1);
    return front != nullptr ? this->painter.DrawPicture(x, y, fill + 2, front->height, 0) : 0;
}

// swd32.exe 0x004506B0: up to four decimal digits right aligned at x (frames of picture `font`)
int BattleHud::DrawSmallNumber(int font, int value, int x, int y)
{
    std::array<int, 4> digits = {};
    int count = 0;
    int divisor = 1000;
    this->numberX = x;
    this->numberY = y;
    this->numberStarted = 0;

    for (int& digit : digits)
    {
        this->numberValue = value;
        digit = value / divisor;
        value %= divisor;

        if (digit != 0 || this->numberStarted == 1)
        {
            this->numberStarted = 1;
            ++count;
        }

        divisor /= 10;
    }

    if (count == 0)
    {
        count = 1;
    }

    int result = 0;

    for (int i = 0; i < count; ++i)
    {
        const Picture* picture = this->Select(font, digits[static_cast<std::size_t>(3 - i)]);

        if (picture == nullptr)
        {
            continue;
        }

        result = this->painter.DrawPicture(x, y, picture->width, picture->height, 0);
        x -= this->lastPicture->width;
    }

    return result;
}

// swd32.exe 0x004507A0: a ten-digit number from x without leading zeros (the units digit always drawn)
int BattleHud::DrawNumber(i16 font, int value, int x, int y)
{
    this->numberFont = (this->numberFont & 0xFFFF) | (static_cast<u32>(static_cast<u16>(font)) << 16);
    this->numberValue = value;
    this->numberX = x;
    this->numberY = y;
    this->numberStarted = 0;
    this->numberX = this->DrawNumberDigit(1000000000u) + x;

    for (u32 divisor = 100000000u; divisor >= 10u; divisor /= 10)
    {
        this->numberX += this->DrawNumberDigit(divisor);
    }

    this->numberStarted = 1;
    const int width = this->DrawNumberDigit(1u);
    this->numberX += width;
    return width;
}

// swd32.exe 0x00450900: the digit for `divisor` 16 pixels left of numberX; its width (0 for a leading zero)
u16 BattleHud::DrawNumberDigit(u32 divisor)
{
    const u32 digit = static_cast<u32>(this->numberValue) / divisor;

    if (static_cast<u16>(digit) == 0 && this->numberStarted == 0)
    {
        return 0;
    }

    const Picture* picture = this->Select(static_cast<u16>(this->numberFont >> 16), static_cast<int>(digit));
    this->Draw(
        picture,
        this->numberX - 0x10,
        this->numberY,
        picture != nullptr ? picture->width : 0,
        picture != nullptr ? picture->height : 0,
        this->digitMode == kDigitGlow8000 ? kScaledGlow : 0
    );
    this->numberStarted = 1;
    this->numberValue = static_cast<i32>(static_cast<u32>(this->numberValue) - divisor * static_cast<u16>(digit));
    return this->lastPicture != nullptr ? this->lastPicture->width : 0;
}

// swd32.exe 0x004509D0: the looping animation `id` of one of eight slots
int BattleHud::DrawSlotAnimation(int id, int slot, int x, int y)
{
    ActState& state = this->slotActs[static_cast<std::size_t>(slot)];
    state.id = id;
    state.action = 0;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return 0;
    }

    this->painter.SetSource(picture->data.data());
    return this->painter.DrawPicture(x, y, picture->width, picture->height, state.flags);
}

// swd32.exe 0x00450A50: the colour argument itself is the source picture: its first pixel blended in by the raw mode 8
int BattleHud::FillColor(int x, int y, int width, int height, u32 color)
{
    u32 slot = color;
    this->painter.SetSource(&slot);
    const int result = this->painter.DrawPicture(x, y, width, height, kRawGradient);
    this->painter.SetSource(nullptr);
    return result;
}

// swd32.exe 0x00450A80: one step of the shared effect animation (mirrored when `mirror` is 1); 1 once it reached its
// end
int BattleHud::DrawEffectAt(int id, int action, int x, int y, int mirror)
{
    ActState& state = this->effectAct;
    state.id = id;
    state.action = action;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return 0;
    }

    u32 flags = state.flags;
    i16 offset = static_cast<i16>(state.x);

    if (mirror == 1)
    {
        flags = (state.flags & 1) != 0 ? state.flags & ~1u : state.flags | 1;
        offset = static_cast<i16>(picture->width - state.x);
    }

    this->painter.SetSource(picture->data.data());
    this->painter.DrawPicture(x - offset, y - state.y, picture->width, picture->height, flags);
    return this->effectAct.waiting == 1 ? 1 : 0;
}

// swd32.exe 0x00450B40
int BattleHud::ResetEffectAt()
{
    this->effectAct = ActState{};
    return 0;
}

// swd32.exe 0x00450B60
int BattleHud::DrawCursorAnimation(int id, int x, int y)
{
    ActState& state = this->cursorAct;
    state.id = id;
    state.action = 0;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture == nullptr)
    {
        return 0;
    }

    this->painter.SetSource(picture->data.data());
    return this->painter.DrawPicture(x, y, picture->width, picture->height, this->cursorAct.flags);
}

// swd32.exe 0x00450BD0: a digit of font 0 (picture 0x2359) or 1 (0x2358); other fonts redraw the last picture
u16 BattleHud::DrawDigit(int font, int digit, int x, int y)
{
    const Picture* picture = this->lastPicture;

    if (font == 0 || font == 1)
    {
        picture = this->archive.GetPicture(font == 0 ? kDigitFont0 : kDigitFont1, static_cast<u32>(digit));
        this->lastPicture = picture;

        if (picture != nullptr)
        {
            this->painter.SetSource(picture->data.data());
        }
    }

    if (picture == nullptr)
    {
        return 0;
    }

    this->painter
        .DrawPicture(x, y, picture->width, picture->height, this->digitMode == kDigitGlow4000 ? kScaledGlow : 0);
    return this->lastPicture->width;
}

const Picture* BattleHud::RunScroll(u16 id, int action)
{
    ActState& state = this->scrollAct;
    state.id = id;
    state.action = action;

    if (this->acts.Run(state) == 0)
    {
        return nullptr;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
    }

    return picture;
}

// swd32.exe 0x00450C50: the scroll bar of act `id`: the top cap (action 0x1A, 0x1E with style 1), `rows` track pieces
// (0x18), the thumb (0x19) sized 7 / scrollCount of the track and placed by scrollFirst, the bottom cap (0x1B, 0x1F
// with style 2)
int BattleHud::DrawScrollBar(u16 id, int x, int y, int rows, int offset, int style)
{
    ActState& state = this->scrollAct;
    const Picture* cap = this->RunScroll(id, style == 1 ? 0x1E : 0x1A);

    if (cap == nullptr)
    {
        return 0;
    }

    this->painter.DrawPicture(x, y, cap->width, cap->height, state.flags);
    const int capHeight = cap->height;
    const int trackTop = capHeight + y;
    state = ActState{};
    const Picture* piece = this->RunScroll(id, 0x18);

    if (piece == nullptr)
    {
        return 0;
    }

    int row = 0;

    for (; row < rows; ++row)
    {
        this->painter.DrawPicture(x + 2, y + capHeight + row * piece->height, piece->width, piece->height, state.flags);
    }

    const u16 pieceHeight = piece->height;
    const int bottomOffset = row * pieceHeight + capHeight;
    const int track = rows * pieceHeight;
    state = ActState{};
    const Picture* thumb = this->RunScroll(id, 0x19);

    if (thumb == nullptr)
    {
        return 0;
    }

    const auto thumbLength = static_cast<i32>(crt::Ftol(
        static_cast<double>(kVisibleRows) / static_cast<double>(this->scrollCount) * static_cast<double>(track)
    ));
    this->scrollStep = (track - thumbLength) / (this->scrollCount - kVisibleRows);
    const int thumbTop = offset + this->scrollStep * this->scrollFirst + trackTop;
    int thumbBottom = offset + trackTop + this->scrollStep * this->scrollFirst + thumbLength;
    this->scrollThumbY = thumbTop;

    if (thumbBottom - thumbTop < 5)
    {
        thumbBottom = thumbTop + 5;
    }

    if (this->scrollFirst + kVisibleRows >= this->scrollCount)
    {
        thumbBottom = trackTop + track;
    }

    this->scrollTrackTop = trackTop;
    this->scrollThumbTop = thumbTop;
    this->scrollThumbBottom = thumbBottom;
    this->scrollTrackBottom = trackTop + track;
    this->painter.SetClipClamped(x, y, x + 0x20, thumbBottom);

    if (thumbLength > 0)
    {
        int drawn = 0;

        do
        {
            this->painter.DrawPicture(x + 5, drawn + this->scrollThumbY, thumb->width, thumb->height, state.flags);
            drawn += thumb->height;
        } while (drawn < thumbLength && thumb->height != 0);
    }

    this->painter.SetClipClamped(0, 0, kScreenWidth, kScreenHeight);
    state = ActState{};
    const Picture* bottom = this->RunScroll(id, style == 2 ? 0x1F : 0x1B);

    if (bottom == nullptr)
    {
        return 0;
    }

    return this->painter.DrawPicture(x, y + bottomOffset, bottom->width, bottom->height, state.flags);
}

// swd32.exe 0x00450F90: the coin of act 0x2329 flips between two positions, slowing down every 25 frames; 1 once the
// delay reaches 64 (a coin ending on side 1 counts in the high word of coinState)
int BattleHud::DrawCoinToss()
{
    if (this->coinFrames == 0)
    {
        if ((this->coinState >> 16) != 0)
        {
            this->coinState = 0;
        }
        else
        {
            this->coinState = (this->coinState & 0xFFFF0000u) | static_cast<u16>(this->random.Below(2));
        }
    }

    ActState& state = this->scrollAct;
    state.id = kCoinAct;
    state.action = static_cast<u16>(this->coinState) == 1 ? 3 : 2;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, 0);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
        this->painter.DrawPicture(
            0x32 * static_cast<u16>(this->coinState) + 0x104,
            0xC8,
            picture->width,
            picture->height,
            state.flags
        );
    }

    u16 delay = 0;

    if (++this->coinFrames % 0x19 != 0)
    {
        delay = static_cast<u16>(this->coinDelay);
    }
    else
    {
        delay = static_cast<u16>(2 * this->coinDelay);
        this->coinDelay = (static_cast<u32>(delay) << 16) | delay;
    }

    if (delay < 0x40)
    {
        auto countdown = static_cast<u16>(this->coinDelay >> 16);

        if (--countdown == 0)
        {
            countdown = delay;
            this->coinState = (this->coinState & 0xFFFF0000u) | (static_cast<u16>(this->coinState) == 0 ? 1u : 0u);
            this->audio.PlaySoundAt(0x2E, 1);
        }

        this->coinDelay = (this->coinDelay & 0xFFFF) | (static_cast<u32>(countdown) << 16);
        return 0;
    }

    this->coinDelay = BattleHud::kInitialCoinDelay;
    this->coinFrames = 0;

    if (static_cast<u16>(this->coinState) == 1)
    {
        this->coinState += 0x10000;
    }

    state = ActState{};
    return 1;
}

// swd32.exe 0x00451100: the timing bar (picture 0x234F) with three marks and a moving needle; timingHit is the mark
// (1-based) under the needle unless it equals the low word of numberFont, which is cleared otherwise. 1 when the needle
// reaches the end.
int BattleHud::DrawTimingBar(int x, int y)
{
    const Picture* back = this->Select(static_cast<int>(kTimingPicture), 0);

    if (back == nullptr)
    {
        return 0;
    }

    this->painter.DrawPicture(x, y, back->width, back->height, 0);
    this->timingHit = 0;

    for (int mark = 0; mark < 3; ++mark)
    {
        const u16 position = BattleHud::kTimingMarks[static_cast<std::size_t>(mark)];
        this->painter.SetClipClamped(x + position, y, x + position + 1, y + this->lastPicture->height);
        const Picture* line = this->Select(static_cast<int>(kTimingPicture), 1);
        this->Draw(line, x, y, line != nullptr ? line->width : 0, line != nullptr ? line->height : 0, 0);
        const int needle = (static_cast<u16>(this->timingCounter) >> 1) + 1;

        if (needle >= position && needle <= position + 2)
        {
            this->timingHit = static_cast<u16>(mark + 1);

            if (static_cast<u16>(this->numberFont) == mark + 1)
            {
                this->timingHit = 0;
            }
            else
            {
                this->numberFont &= 0xFFFF0000u;
            }
        }
    }

    const int needle = static_cast<u16>(this->timingCounter) >> 1;
    this->painter.SetClipClamped(needle + x + 1, y, needle + x + 2, y + this->lastPicture->height);
    const Picture* line = this->Select(static_cast<int>(kTimingPicture), 1);
    this->Draw(line, x, y, line != nullptr ? line->width : 0, line != nullptr ? line->height : 0, 0);
    this->painter.SetClipClamped(0, 0, kScreenWidth, kScreenHeight);
    this->timingCounter = static_cast<i32>(
        (static_cast<u32>(this->timingCounter) & 0xFFFF0000u) | static_cast<u16>(this->timingCounter + 1)
    );

    if ((static_cast<u16>(this->timingCounter) >> 1) != 0x3D)
    {
        return 0;
    }

    this->timingCounter = static_cast<i32>((static_cast<u32>(this->timingCounter) & 0xFFFF0000u) | 0x8000);
    return 1;
}

// swd32.exe 0x004512B0: the vertical gauge of picture 0x241A showing `level` sixths of its body
int BattleHud::DrawTallGauge(int x, int y, int level)
{
    const Picture* body = this->archive.GetPicture(kTallGauge, 2);
    this->lastPicture = body;

    if (body == nullptr)
    {
        return 0;
    }

    const int shown = level * (body->height / 6);
    const Picture* top = this->Select(static_cast<int>(kTallGauge), 0);

    if (top == nullptr)
    {
        return 0;
    }

    this->painter.DrawPicture(x, y, top->width, top->height, 0);
    const int bodyTop = this->lastPicture->height + y;

    if (level < 6)
    {
        this->painter.SetClipClamped(x, bodyTop, x + this->lastPicture->width, shown + bodyTop);
    }

    body = this->Select(static_cast<int>(kTallGauge), 2);
    this->Draw(body, x + 4, bodyTop, body != nullptr ? body->width : 0, body != nullptr ? body->height : 0, 0);
    const Picture* shine = this->Select(static_cast<int>(kTallGauge), 3);
    this->painter.Options().alpha = 8;
    this->Draw(
        shine,
        x + 0xB,
        bodyTop + 0x1F,
        shine != nullptr ? shine->width : 0,
        shine != nullptr ? shine->height : 0,
        kTranslucent
    );
    this->painter.SetClipClamped(0, 0, kScreenWidth, kScreenHeight);
    const Picture* bottom = this->Select(static_cast<int>(kTallGauge), 1);
    return bottom != nullptr ? this->painter.DrawPicture(x, bodyTop + shown, bottom->width, bottom->height, 0) : 0;
}

// swd32.exe 0x00451540: one step of the animation with the picture loaded for its frame; its waiting flag
int BattleHud::DrawAnimation(BattleAnimation& animation)
{
    if (animation.id == 0)
    {
        return 0;
    }

    ActState& state = animation.act;
    state.id = animation.id;
    state.action = 0;
    this->acts.Run(state);
    const Picture* picture =
        state.picture < BattleAnimation::kPictures ? animation.pictures[state.picture].get() : nullptr;

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
        this->painter.SetWave(animation.wave);
        this->painter
            .DrawPicture(animation.x - state.x, animation.y - state.y, picture->width, picture->height, state.flags);
        this->painter.SetWave(0);
    }

    return state.waiting;
}

// swd32.exe 0x00451730
int BattleHud::FreeAnimation(BattleAnimation& animation)
{
    for (std::unique_ptr<Picture>& picture : animation.pictures)
    {
        picture.reset();
    }

    animation.id = 0;
    animation.wave = 0;
    animation.act = ActState{};
    return 0;
}

// swd32.exe 0x00451420: loads every frame picture of act `id` (run to its end) into the animation, each shifted right
// by 640 / `divisor`; the wave mode of 'WT' is kept. `unused` is ignored. Always 0.
int BattleHud::LoadAnimation(BattleAnimation& animation, int, int x, int y, u16 id, u16 divisor)
{
    std::array<u16, BattleAnimation::kPictures> loaded = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    ActState& state = animation.act;
    animation.id = id;
    animation.x = x;
    animation.y = y;
    state.id = id;
    state.action = 0;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    for (;;)
    {
        if (state.wait != 0)
        {
            animation.wave = state.wait;
        }

        const u16 frame = state.picture;

        if (frame < BattleAnimation::kPictures && loaded[frame] == 0xFFFF)
        {
            animation.pictures[frame] = this->archive.LoadPictureDirect(state.frame, state.picture);
            loaded[frame] = frame;

            if (animation.pictures[frame] != nullptr)
            {
                ShiftPicture(animation.pictures[frame]->data, ShiftMode::Right, kScreenWidth / divisor);
            }
        }

        if (state.position == 0)
        {
            break;
        }

        state.action = 0;
        state.id = animation.id;

        if (this->acts.Run(state) == 0)
        {
            return 0;
        }
    }

    state = ActState{};
    return 0;
}

// swd32.exe 0x004515E0: plays the loaded animation to its end in one call, every new frame shifted by `amount` (right
// when positive, left when negative) and drawn; 1 when it ended, 0 without an animation. `unused` is ignored.
int BattleHud::PlayAnimationShifted(BattleAnimation& animation, int, int amount)
{
    std::array<u16, BattleAnimation::kPictures> shown = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    if (animation.id == 0)
    {
        return 0;
    }

    ActState& state = animation.act;
    state = ActState{};
    state.action = 0;
    state.id = animation.id;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    for (;;)
    {
        const u16 frame = state.picture;

        if (frame < BattleAnimation::kPictures && shown[frame] == 0xFFFF)
        {
            shown[frame] = frame;
            Picture* picture = animation.pictures[frame].get();

            if (picture != nullptr)
            {
                if (amount > 0)
                {
                    ShiftPicture(picture->data, ShiftMode::Right, amount);
                }
                else if (amount < 0)
                {
                    ShiftPicture(picture->data, ShiftMode::Left, -amount);
                }

                this->painter.SetSource(picture->data.data());
                this->painter.DrawPicture(
                    animation.x - state.x,
                    animation.y - state.y,
                    picture->width,
                    picture->height,
                    state.flags
                );
            }
        }

        const bool ended = state.position == 0;
        state.delay = 0;
        state.frameDelay = 0;

        if (ended)
        {
            break;
        }

        state.action = 0;
        state.id = animation.id;

        if (this->acts.Run(state) == 0)
        {
            return 0;
        }
    }

    state = ActState{};
    return 1;
}

}  // namespace swd3
