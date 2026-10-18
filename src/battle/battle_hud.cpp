#include "battle/battle_hud.h"

#include "audio/audio_manager.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "pic/picture.h"
#include "pic/picture_ops.h"
#include "platform/display.h"
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
constexpr u16 kDigitGlow4000 = 0x4000;
constexpr u16 kDigitGlow8000 = 0x8000;
constexpr u32 kDigitFont0 = 0x2359;
constexpr u32 kDigitFont1 = 0x2358;
constexpr u16 kCoinAct = 0x2329;
constexpr u32 kTimingPicture = 0x234F;
constexpr u32 kTallGauge = 0x241A;
constexpr int kVisibleRows = 7;
constexpr int kDimTint = -10;  // the green and blue tint of a dimmed portrait
constexpr int kGaugeBack = 0;  // the frames of a gauge picture
constexpr int kGaugeFill = 1;
constexpr int kGaugeAltFill = 2;
constexpr int kWideFillExtra = 2;
constexpr int kDecimal = 10;
constexpr int kSmallDigits = 4;
constexpr int kSmallTopDivisor = 1000;
constexpr u32 kTopDivisor = 1000000000u;  // DrawNumber: ten digits
constexpr int kNumberDigitWidth = 0x10;
constexpr int kScrollPiece = 0x18;  // the actions of the scroll bar act
constexpr int kScrollThumb = 0x19;
constexpr int kScrollTop = 0x1A;
constexpr int kScrollBottom = 0x1B;
constexpr int kScrollStyle1Top = 0x1E;
constexpr int kScrollStyle2Bottom = 0x1F;
constexpr int kTopCapStyle = 1;  // DrawScrollBar styles
constexpr int kBottomCapStyle = 2;
constexpr int kLineWidth = 1;  // the timing marks and needle
constexpr int kTrackInset = 2;
constexpr int kThumbInset = 5;
constexpr int kMinThumb = 5;
constexpr int kScrollClipWidth = 0x20;
constexpr u32 kCoinSides = 2;
constexpr u16 kWinningSide = 1;
constexpr i32 kCoinAction = 2;
constexpr i32 kCoinWinAction = 3;  // the coin showing the winning side
constexpr int kCoinX = 0x104;      // a coin is drawn kCoinStep further right on the winning side
constexpr int kCoinStep = 0x32;
constexpr int kCoinY = 0xC8;
constexpr i32 kCoinSlowFrames = 0x19;  // the coin flips kCoinSlowdown times slower every kCoinSlowFrames frames
constexpr int kCoinSlowdown = 2;
constexpr u16 kCoinStopDelay = 0x40;
constexpr u16 kCoinSound = 0x2E;
constexpr u16 kCoinVolume = 1;
constexpr int kNeedleShift = 1;  // the needle moves a pixel every second frame
constexpr int kNeedleReach = 2;  // a mark is hit up to two pixels past it
constexpr int kNeedleEnd = 0x3D;
constexpr u16 kTimingDone = 0x8000;
constexpr int kGaugeTop = 0;  // the frames of the tall gauge
constexpr int kGaugeBottom = 1;
constexpr int kGaugeBody = 2;
constexpr int kGaugeShine = 3;
constexpr int kGaugeLevels = 6;
constexpr int kBodyInset = 4;
constexpr int kShineAlpha = 8;
constexpr int kShineX = 0xB;
constexpr int kShineY = 0x1F;
constexpr u16 kNotLoaded = 0xFFFF;
constexpr int kTileFill = 4;  // the frames of a tile box (DrawTileBox)
constexpr int kTileTopLeft = 0;
constexpr int kTileTop = 1;
constexpr int kTileTopRight = 2;
constexpr int kTileLeft = 3;
constexpr int kTileRight = 5;
constexpr int kTileBottomLeft = 6;
constexpr int kTileBottom = 7;
constexpr int kTileBottomRight = 8;
constexpr int kTileSize = 0x10;

}  // namespace

BattleHud::BattleHud(Painter& painter, TswArchive& archive, ActArchive& acts, Random& random, AudioManager& audio)
    : painter(painter),
      archive(archive),
      acts(acts),
      random(random),
      audio(audio)
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
    options.tintGreen = kDimTint;
    options.tintBlue = kDimTint;
    options.tintRed = 0;
    const Picture* current = this->lastPicture;
    return this->painter.DrawPicture(
        x - state.x,
        y - state.y,
        current->width,
        current->height,
        state.flags | Painter::TintUnder,
        PaletteArgument(*current)
    );
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

    return this->painter
        .DrawPicture(x - state.x, y - state.y, picture->width, picture->height, state.flags, PaletteArgument(*picture));
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
    const Picture* back = this->Select(id, kGaugeBack);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, kGaugeFill);

    if (front != nullptr && fill != 0)
    {
        return this->painter.DrawPicture(x, y, fill, front->height, 0);
    }

    return front != nullptr ? 1 : 0;
}

// swd32.exe 0x004505B0: as DrawGauge with frame 2
int BattleHud::DrawGaugeAlt(int id, int x, int y, int fill)
{
    const Picture* back = this->Select(id, kGaugeBack);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, kGaugeAltFill);

    if (front != nullptr && fill != 0)
    {
        this->painter.DrawPicture(x, y, fill, front->height, 0);
    }

    return 1;
}

// swd32.exe 0x00450630: frame 1 drawn fill + 2 pixels wide (also when fill is 0)
int BattleHud::DrawGaugeWide(int id, int x, int y, u16 fill)
{
    const Picture* back = this->Select(id, kGaugeBack);

    if (back != nullptr)
    {
        this->painter.DrawPicture(x, y, back->width, back->height, 0);
    }

    const Picture* front = this->Select(id, kGaugeFill);
    return front != nullptr ? this->painter.DrawPicture(x, y, fill + kWideFillExtra, front->height, 0) : 0;
}

// swd32.exe 0x004506B0: up to four decimal digits right aligned at x (frames of picture `font`)
int BattleHud::DrawSmallNumber(int font, int value, int x, int y)
{
    std::array<int, kSmallDigits> digits = {};
    int count = 0;
    int divisor = kSmallTopDivisor;
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

        divisor /= kDecimal;
    }

    if (count == 0)
    {
        count = 1;
    }

    int result = 0;

    for (int i = 0; i < count; ++i)
    {
        const Picture* picture = this->Select(font, digits[static_cast<std::size_t>(kSmallDigits - 1 - i)]);

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
    this->numberFont = MakeWords(static_cast<u16>(font), LowWord(this->numberFont));
    this->numberValue = value;
    this->numberX = x;
    this->numberY = y;
    this->numberStarted = 0;
    this->numberX = this->DrawNumberDigit(kTopDivisor) + x;

    for (u32 divisor = kTopDivisor / kDecimal; divisor >= kDecimal; divisor /= kDecimal)
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

    const Picture* picture = this->Select(HighWord(this->numberFont), static_cast<int>(digit));
    this->Draw(
        picture,
        this->numberX - kNumberDigitWidth,
        this->numberY,
        picture != nullptr ? picture->width : 0,
        picture != nullptr ? picture->height : 0,
        this->digitMode == kDigitGlow8000 ? Painter::ScaledGlow : 0
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
    const int result = this->painter.DrawPicture(x, y, width, height, Painter::RawGradient);
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
    i32 offset = state.x;

    if (mirror == 1)
    {
        flags = (state.flags & 1) != 0 ? state.flags & ~1u : state.flags | 1;
        // 0x00450AE2 mov dx / sub dx: only the low word becomes width - x, the high word of x stays
        offset = static_cast<i32>(
            (static_cast<u32>(state.x) & 0xFFFF0000u) | static_cast<u16>(picture->width - static_cast<u16>(state.x))
        );
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

    const u32 glow = this->digitMode == kDigitGlow4000 ? Painter::ScaledGlow : 0;
    this->painter.DrawPicture(x, y, picture->width, picture->height, glow);
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
    const Picture* cap = this->RunScroll(id, style == kTopCapStyle ? kScrollStyle1Top : kScrollTop);

    if (cap == nullptr)
    {
        return 0;
    }

    this->painter.DrawPicture(x, y, cap->width, cap->height, state.flags);
    const int capHeight = cap->height;
    const int trackTop = capHeight + y;
    state = ActState{};
    const Picture* piece = this->RunScroll(id, kScrollPiece);

    if (piece == nullptr)
    {
        return 0;
    }

    int row = 0;

    for (; row < rows; ++row)
    {
        const int rowY = y + capHeight + row * piece->height;
        this->painter.DrawPicture(x + kTrackInset, rowY, piece->width, piece->height, state.flags);
    }

    const u16 pieceHeight = piece->height;
    const int bottomOffset = row * pieceHeight + capHeight;
    const int track = rows * pieceHeight;
    state = ActState{};
    const Picture* thumb = this->RunScroll(id, kScrollThumb);

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

    if (thumbBottom - thumbTop < kMinThumb)
    {
        thumbBottom = thumbTop + kMinThumb;
    }

    if (this->scrollFirst + kVisibleRows >= this->scrollCount)
    {
        thumbBottom = trackTop + track;
    }

    this->scrollTrackTop = trackTop;
    this->scrollThumbTop = thumbTop;
    this->scrollThumbBottom = thumbBottom;
    this->scrollTrackBottom = trackTop + track;
    this->painter.SetClipClamped(x, y, x + kScrollClipWidth, thumbBottom);

    if (thumbLength > 0)
    {
        int drawn = 0;

        do
        {
            this->painter
                .DrawPicture(x + kThumbInset, drawn + this->scrollThumbY, thumb->width, thumb->height, state.flags);
            drawn += thumb->height;
        } while (drawn < thumbLength && thumb->height != 0);
    }

    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    state = ActState{};
    const Picture* bottom = this->RunScroll(id, style == kBottomCapStyle ? kScrollStyle2Bottom : kScrollBottom);

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
        if (HighWord(this->coinState) != 0)
        {
            this->coinState = 0;
        }
        else
        {
            this->coinState = MakeWords(HighWord(this->coinState), static_cast<u16>(this->random.Below(kCoinSides)));
        }
    }

    ActState& state = this->scrollAct;
    state.id = kCoinAct;
    state.action = LowWord(this->coinState) == kWinningSide ? kCoinWinAction : kCoinAction;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, 0);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
        this->painter.DrawPicture(
            kCoinStep * LowWord(this->coinState) + kCoinX,
            kCoinY,
            picture->width,
            picture->height,
            state.flags
        );
    }

    u16 delay = 0;

    if (++this->coinFrames % kCoinSlowFrames != 0)
    {
        delay = LowWord(this->coinDelay);
    }
    else
    {
        delay = static_cast<u16>(kCoinSlowdown * this->coinDelay);
        this->coinDelay = MakeWords(delay, delay);
    }

    if (delay < kCoinStopDelay)
    {
        auto countdown = HighWord(this->coinDelay);

        if (--countdown == 0)
        {
            countdown = delay;
            this->coinState = MakeWords(HighWord(this->coinState), LowWord(this->coinState) == 0 ? u16{1} : u16{0});
            this->audio.PlaySoundAt(kCoinSound, kCoinVolume);
        }

        this->coinDelay = MakeWords(countdown, LowWord(this->coinDelay));
        return 0;
    }

    this->coinDelay = BattleHud::InitialCoinDelay;
    this->coinFrames = 0;

    if (LowWord(this->coinState) == kWinningSide)
    {
        this->coinState = MakeWords(static_cast<u16>(HighWord(this->coinState) + 1), LowWord(this->coinState));
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

    for (int mark = 0; mark < static_cast<int>(BattleHud::TimingMarks.size()); ++mark)
    {
        const u16 position = BattleHud::TimingMarks[static_cast<std::size_t>(mark)];
        this->painter.SetClipClamped(x + position, y, x + position + kLineWidth, y + this->lastPicture->height);
        const Picture* line = this->Select(static_cast<int>(kTimingPicture), 1);
        this->Draw(line, x, y, line != nullptr ? line->width : 0, line != nullptr ? line->height : 0, 0);
        const int needle = (LowWord(this->timingCounter) >> kNeedleShift) + 1;

        if (needle >= position && needle <= position + kNeedleReach)
        {
            this->timingHit = static_cast<u16>(mark + 1);

            if (LowWord(this->numberFont) == mark + 1)
            {
                this->timingHit = 0;
            }
            else
            {
                this->numberFont = MakeWords(HighWord(this->numberFont), 0);
            }
        }
    }

    const int needle = LowWord(this->timingCounter) >> kNeedleShift;
    this->painter.SetClipClamped(needle + x + 1, y, needle + x + 1 + kLineWidth, y + this->lastPicture->height);
    const Picture* line = this->Select(static_cast<int>(kTimingPicture), 1);
    this->Draw(line, x, y, line != nullptr ? line->width : 0, line != nullptr ? line->height : 0, 0);
    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->timingCounter =
        static_cast<i32>(MakeWords(HighWord(this->timingCounter), static_cast<u16>(this->timingCounter + 1)));

    if ((LowWord(this->timingCounter) >> kNeedleShift) != kNeedleEnd)
    {
        return 0;
    }

    this->timingCounter = static_cast<i32>(MakeWords(HighWord(this->timingCounter), kTimingDone));
    return 1;
}

// swd32.exe 0x004512B0: the vertical gauge of picture 0x241A showing `level` sixths of its body
int BattleHud::DrawTallGauge(int x, int y, int level)
{
    const Picture* body = this->archive.GetPicture(kTallGauge, kGaugeBody);
    this->lastPicture = body;

    if (body == nullptr)
    {
        return 0;
    }

    const int shown = level * (body->height / kGaugeLevels);
    const Picture* top = this->Select(static_cast<int>(kTallGauge), kGaugeTop);

    if (top == nullptr)
    {
        return 0;
    }

    this->painter.DrawPicture(x, y, top->width, top->height, 0);
    const int bodyTop = this->lastPicture->height + y;

    if (level < kGaugeLevels)
    {
        this->painter.SetClipClamped(x, bodyTop, x + this->lastPicture->width, shown + bodyTop);
    }

    body = this->Select(static_cast<int>(kTallGauge), kGaugeBody);
    this->Draw(body, x + kBodyInset, bodyTop, body != nullptr ? body->width : 0, body != nullptr ? body->height : 0, 0);
    const Picture* shine = this->Select(static_cast<int>(kTallGauge), kGaugeShine);
    this->painter.Options().alpha = kShineAlpha;
    this->Draw(
        shine,
        x + kShineX,
        bodyTop + kShineY,
        shine != nullptr ? shine->width : 0,
        shine != nullptr ? shine->height : 0,
        Painter::Translucent
    );
    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    const Picture* bottom = this->Select(static_cast<int>(kTallGauge), kGaugeBottom);
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
        state.picture < BattleAnimation::Pictures ? animation.pictures[state.picture].get() : nullptr;

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
    std::array<u16, BattleAnimation::Pictures> loaded = {};
    loaded.fill(kNotLoaded);
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

        if (frame < BattleAnimation::Pictures && loaded[frame] == kNotLoaded)
        {
            animation.pictures[frame] = this->archive.LoadPictureDirect(state.frame, state.picture);
            loaded[frame] = frame;

            if (animation.pictures[frame] != nullptr)
            {
                ShiftPicture(animation.pictures[frame]->data, ShiftMode::Right, Display::Width / divisor);
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
    std::array<u16, BattleAnimation::Pictures> shown = {};
    shown.fill(kNotLoaded);

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

        if (frame < BattleAnimation::Pictures && shown[frame] == kNotLoaded)
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

// swd32.exe 0x0044FFE0: a box of `columns` x `rows` tiles of picture `id` around a filled area (`fill`, from frame 4,
// 16 pixels a tile with a border tile each side): frames 0..2 the top (corner, edge repeated, corner), 3 and 5 the
// sides of each row, 6..8 the bottom. A missing frame is read through a null pointer in the original; it is not drawn
// and takes no room here.
void BattleHud::DrawTileBox(int id, int x, int y, int columns, int rows, i8 fill)
{
    auto width = [](const Picture* picture)
    {
        return picture != nullptr ? static_cast<int>(picture->width) : 0;
    };

    auto height = [](const Picture* picture)
    {
        return picture != nullptr ? static_cast<int>(picture->height) : 0;
    };

    auto draw = [this](const Picture* picture, int left, int top)
    {
        if (picture != nullptr)
        {
            this->painter.DrawPicture(left, top, picture->width, picture->height, 0, PaletteArgument(*picture));
        }
    };

    this->Select(id, kTileFill);
    this->FillColor(x, y, kTileSize * (columns + 2), kTileSize * (rows + 2), static_cast<u32>(static_cast<i32>(fill)));
    const Picture* corner = this->Select(id, kTileTopLeft);
    draw(corner, x, y);
    int left = x + width(corner);
    const Picture* edge = this->Select(id, kTileTop);

    for (int column = 0; column < columns; ++column)
    {
        draw(edge, left, y);
        left += width(edge);
    }

    const Picture* topRight = this->Select(id, kTileTopRight);
    draw(topRight, left, y);
    int top = y + height(topRight);

    for (int row = 0; row < rows; ++row)
    {
        const Picture* side = this->Select(id, kTileLeft);
        draw(side, x, top);
        const int rightX = x + (columns + 1) * width(side);
        const Picture* right = this->Select(id, kTileRight);
        draw(right, rightX, top);
        top += height(right);
    }

    const Picture* bottomLeft = this->Select(id, kTileBottomLeft);
    draw(bottomLeft, x, top);
    left = x + width(bottomLeft);
    const Picture* bottom = this->Select(id, kTileBottom);

    for (int column = 0; column < columns; ++column)
    {
        draw(bottom, left, top);
        left += width(bottom);
    }

    draw(this->Select(id, kTileBottomRight), left, top);
}
}  // namespace swd3
