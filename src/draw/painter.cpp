#include "draw/painter.h"

#include <array>
#include <cstring>

#include "core/words.h"
#include "pic/picture.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

// The channel masks of the display formats, and where each channel starts.
constexpr u32 kRed555 = 0x7C00;
constexpr u32 kGreen555 = 0x3E0;
constexpr u32 kBlue5 = 0x1F;
constexpr u32 kRed565 = 0xF800;
constexpr u32 kGreen565 = 0x7E0;
constexpr u32 kGreenAt6 = 0x7C0;  // a 5-bit green at bits 6..10
constexpr u32 kBlue6 = 0x3F;
constexpr u32 kBlueAt1 = 0x3E;  // a 5-bit blue at bits 1..5
constexpr u32 kRed6 = 0xFC00;
constexpr int kRed555Shift = 10;
constexpr int kGreen555Shift = 5;
constexpr int kRed565Shift = 11;
constexpr int kGreenAt6Shift = 6;
constexpr int kBlueAt1Shift = 1;
constexpr std::size_t kChannels = 3;
constexpr int kChannelBits = 5;
constexpr u32 kChannelMask = (1u << kChannelBits) - 1;
constexpr int kDitherOffset = Display::Height;  // the dither phase counts from the screen height

u16 ReadPixel(const u8* bytes)
{
    u16 value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

// swd32.exe 0x004238F0: RGB555 to R5 G5 B5 A1
void ConvertTo5551(std::span<u16> pixels)
{
    for (u16& pixel : pixels)
    {
        pixel = static_cast<u16>(pixel << 1);
    }
}

// swd32.exe 0x00423920: RGB555 to RGB565 (green gets a zero low bit)
void ConvertTo565(std::span<u16> pixels)
{
    for (u16& pixel : pixels)
    {
        pixel = static_cast<u16>(((pixel & 0xFFE0u) << 1) + (pixel & 0x1Fu));
    }
}

// swd32.exe 0x00423950: RGB565 back to RGB555
void ConvertFrom565(std::span<u16> pixels)
{
    for (u16& pixel : pixels)
    {
        pixel = static_cast<u16>(((pixel >> 1) & 0xFFE0u) + (pixel & 0x1Fu));
    }
}

// swd32.exe 0x00423990: RGB555 to R6 G5 B5 (the low red bit left at 0)
void ConvertTo6551(std::span<u16> pixels)
{
    for (u16& pixel : pixels)
    {
        pixel = static_cast<u16>(((pixel & 0x7C00u) << 1) | (pixel & 0x3FFu));
    }
}

// The converters process at least the first pixel in the original; an empty span converts nothing here.
void Convert(Painter::Conversion conversion, std::span<u16> pixels)
{
    switch (conversion)
    {
        case Painter::Conversion::None:
            break;

        case Painter::Conversion::To5551:
            ConvertTo5551(pixels);
            break;

        case Painter::Conversion::To565:
            ConvertTo565(pixels);
            break;

        case Painter::Conversion::To6551:
            ConvertTo6551(pixels);
            break;
    }
}

// Only RGB565 has a reverse converter; the others keep the empty function.
void ConvertBack(Painter::Conversion conversion, std::span<u16> pixels)
{
    if (conversion == Painter::Conversion::To565)
    {
        ConvertFrom565(pixels);
    }
}

}  // namespace

u32 ColorChannel::Ramp(int index) const
{
    return this->ramp[static_cast<std::size_t>(index + ColorChannel::Levels)];
}

// Ready to draw once Init has set the format and the screen (the tables of 0x00416D90 are built here as well).
Painter::Painter()
{
    this->BuildBlitTables();
}

// swd32.exe 0x00416D30: DrawInit
bool Painter::Init(Surface& screen)
{
    if (!this->InitPixelFormat(&screen))
    {
        return false;
    }

    this->clipWidth = this->screenWidth;
    this->clipX = 0;
    this->clipY = 0;
    this->clipHeight = this->screenHeight;

    for (int y = 0; y < this->screenHeight && static_cast<std::size_t>(y) < this->rowOffsets.size(); ++y)
    {
        this->rowOffsets[static_cast<std::size_t>(y)] = y * this->screenPitch;
    }

    this->BuildBlitTables();
    return true;
}

// swd32.exe 0x00416D90: DrawTableInit. Modes without an entry stay empty; the original calls through the empty slot.
void Painter::BuildBlitTables()
{
    this->blitters.fill(nullptr);

    auto rle = [this](u32 mode, Blitter forward, Blitter mirrored)
    {
        this->blitters[mode] = forward;
        this->blitters[mode | Painter::Mirror] = mirrored;
    };

    auto rleFlippable = [this](u32 mode, Blitter forward, Blitter mirrored)
    {
        this->blitters[mode] = forward;
        this->blitters[mode | Painter::Mirror] = mirrored;
        this->blitters[mode | Painter::Flip] = forward;
        this->blitters[mode | Painter::Flip | Painter::Mirror] = mirrored;
    };

    rleFlippable(Painter::Copy, &Painter::BlitCopy, &Painter::BlitCopyMirrored);
    rleFlippable(Painter::Shadow, &Painter::BlitShadow, &Painter::BlitShadowMirrored);
    rleFlippable(Painter::Translucent, &Painter::BlitTranslucent, &Painter::BlitTranslucentMirrored);
    rleFlippable(Painter::Silhouette, &Painter::BlitSilhouette, &Painter::BlitSilhouetteMirrored);
    rleFlippable(Painter::GrayUnder, &Painter::BlitGray, &Painter::BlitGrayMirrored);
    rleFlippable(Painter::SmearUnder, &Painter::BlitSmear, &Painter::BlitSmearMirrored);
    rle(Painter::Additive, &Painter::BlitAdditive, &Painter::BlitAdditiveMirrored);
    rle(Painter::AlphaMap, &Painter::BlitAlphaMap, &Painter::BlitAlphaMapMirrored);
    rle(Painter::TintUnder, &Painter::BlitTint, &Painter::BlitTintMirrored);
    rle(Painter::EdgeMark, &Painter::BlitEdgeMark, &Painter::BlitEdgeMarkMirrored);
    this->blitters[Painter::FadeDown] = &Painter::BlitFadeDown;
    rle(Painter::ScaledGlow, &Painter::BlitScaledGlow, &Painter::BlitScaledGlowMirrored);
    rle(Painter::Subtractive, &Painter::BlitSubtractive, &Painter::BlitSubtractiveMirrored);

    constexpr u32 kRaw = Painter::RawBlitters;
    this->blitters[kRaw + Painter::RawCopy] = &Painter::BlitRawCopy;
    this->blitters[kRaw + (Painter::RawCopy | Painter::Mirror)] = &Painter::BlitRawCopyMirrored;
    this->blitters[kRaw + Painter::RawKeyed] = &Painter::BlitRawKeyed;
    this->blitters[kRaw + (Painter::RawKeyed | Painter::Mirror)] = &Painter::BlitRawKeyedMirrored;
    this->blitters[kRaw + Painter::RawGradient] = &Painter::BlitRawGradient;
    this->blitters[kRaw + Painter::RawTranslucent] = &Painter::BlitRawTranslucent;
}

// swd32.exe 0x00423400: PixelFormatInit. Detects the converter of the display format; masks of an unknown format
// keep the converter and shifts of the previous initialization. Every channel table uses the masks as adjusted for
// the converter (RGB565 keeps a 5-bit green at bits 6..10), and the tint options are cleared.
bool Painter::InitPixelFormat(const Surface* screen)
{
    u32 redMask = kRed555;
    u32 greenMask = kGreen555;
    u32 blueMask = kBlue5;

    if (screen != nullptr)
    {
        this->screenWidth = screen->Width();
        this->screenPitch = screen->Width();
        this->screenHeight = screen->Height();
        redMask = Surface::RedMask;
        greenMask = Surface::GreenMask;
        blueMask = Surface::BlueMask;
    }

    this->red.mask = redMask;
    this->green.mask = greenMask;
    this->blue.mask = blueMask;
    this->displayRed = redMask;
    this->displayGreen = greenMask;
    this->displayBlue = blueMask;

    if (redMask == kRed555)
    {
        if (greenMask == kGreen555 && blueMask == kBlue5)
        {
            this->conversion = Conversion::None;
            this->conversionBack = Conversion::None;
            this->red.shift = kRed555Shift;
            this->green.shift = kGreen555Shift;
            this->blue.shift = 0;
        }
    }
    else if (redMask == kRed565)
    {
        if (greenMask == kGreenAt6)
        {
            if (blueMask == kBlue6)
            {
                blueMask = kBlueAt1;
                this->conversion = Conversion::To5551;
                this->red.shift = kRed565Shift;
                this->green.shift = kGreenAt6Shift;
                this->blue.shift = kBlueAt1Shift;
            }
        }
        else if (greenMask == kGreen565 && blueMask == kBlue5)
        {
            greenMask = kGreenAt6;
            this->conversion = Conversion::To565;
            this->conversionBack = Conversion::To565;
            this->red.shift = kRed565Shift;
            this->green.shift = kGreenAt6Shift;
            this->blue.shift = 0;
        }
    }
    else if (redMask == kRed6 && greenMask == kGreen555 && blueMask == kBlue5)
    {
        redMask = kRed565;
        this->conversion = Conversion::To6551;
        this->red.shift = kRed565Shift;
        this->green.shift = kGreen555Shift;
        this->blue.shift = 0;
    }

    this->greenMaskPair = MakeWords(LowWord(greenMask), LowWord(greenMask));
    const std::array<u32, kChannels> masks = {redMask, greenMask, blueMask};
    const std::array<ColorChannel*, kChannels> channels = {&this->red, &this->green, &this->blue};

    for (std::size_t i = 0; i < kChannels; ++i)
    {
        ColorChannel& channel = *channels[i];
        channel.mask = LowWord(masks[i]);
        channel.halves[0] = LowWord(masks[i]);
        channel.inverseMask = ~channel.mask;
    }

    for (std::size_t k = 0; k < ColorChannel::Shifts; ++k)
    {
        u32 pair = 0;

        for (std::size_t i = 0; i < kChannels; ++i)
        {
            ColorChannel& channel = *channels[i];
            channel.steps[k] = static_cast<u32>(k + 1) << channel.shift;
            channel.halves[k + 1] = (channel.halves[k] >> 1) & masks[i];
            pair |= channel.halves[k + 1];
        }

        this->halfMaskPairs[k] = MakeWords(LowWord(pair), LowWord(pair));
    }

    this->rleColors = RleColors{};
    u16 green = LowWord(this->greenMaskPair);
    u16 fill = this->rleColors.fill16;
    u16 skip = this->rleColors.skip16;
    this->ConvertPixels(std::span<u16>(&green, 1));
    this->ConvertPixels(std::span<u16>(&fill, 1));
    this->ConvertPixels(std::span<u16>(&skip, 1));
    this->greenMaskPair = MakeWords(HighWord(this->greenMaskPair), green);
    this->fillColorPair = MakeWords(fill, fill);
    this->skipColorPair = MakeWords(skip, skip);

    for (std::size_t i = 0; i < kChannels; ++i)
    {
        ColorChannel& channel = *channels[i];
        const u32 step = channel.halves[ColorChannel::TintHalf];

        for (int k = 0; k < ColorChannel::Levels; ++k)
        {
            channel.ramp[static_cast<std::size_t>(k)] = 0;
            channel.ramp[static_cast<std::size_t>(ColorChannel::Levels + k)] = step * static_cast<u32>(k);
            channel.ramp[static_cast<std::size_t>(2 * ColorChannel::Levels + k)] = masks[i];
        }
    }

    this->options.tintRed = 0;
    this->options.tintGreen = 0;
    this->options.tintBlue = 0;
    return true;
}

// swd32.exe 0x004233D0: the masks as reported by the display (before the converter adjustment)
void Painter::ColorMasks(u32& red, u32& green, u32& blue) const
{
    red = this->displayRed;
    green = this->displayGreen;
    blue = this->displayBlue;
}

// swd32.exe 0x004238B0: through the converter of the display format (0x004238F0, 0x00423920, 0x00423990)
void Painter::ConvertPixels(std::span<u16> pixels) const
{
    Convert(this->conversion, pixels);
}

// swd32.exe 0x004238D0: through the reverse converter (0x00423950 for RGB565, otherwise none)
void Painter::ConvertPixelsBack(std::span<u16> pixels) const
{
    ConvertBack(this->conversionBack, pixels);
}

// swd32.exe 0x004239D0: 5-bit components to a display colour, in both halves of the result
u32 Painter::MakeColor(u32 red, u32 green, u32 blue) const
{
    const auto color = static_cast<u16>(
        (((red & kChannelMask) << kChannelBits | (green & kChannelMask)) << kChannelBits) | (blue & kChannelMask)
    );
    std::array<u16, 2> halves = {color, color};
    this->ConvertPixels(halves);
    return MakeWords(halves[1], halves[0]);
}

const RleColors& Painter::Rle() const
{
    return this->rleColors;
}

const ColorChannel& Painter::Red() const
{
    return this->red;
}

const ColorChannel& Painter::Green() const
{
    return this->green;
}

const ColorChannel& Painter::Blue() const
{
    return this->blue;
}

// swd32.exe 0x00416F10: the surface memory; its pitch is kept for the callers that step through it
u16* Painter::LockSurface(Surface& surface)
{
    this->surfacePitch = surface.Width();
    return surface.Pixels();
}

// swd32.exe 0x00416F60
void Painter::UnlockSurface(Surface& surface)
{
    static_cast<void>(surface);
}

void Painter::SetTarget(u16* pixels)
{
    this->target = pixels;
}

u16* Painter::Target() const
{
    return this->target;
}

// swd32.exe 0x00416F80: rejected when it leaves the screen
bool Painter::SetClip(int left, int top, int right, int bottom)
{
    if (left < 0 || top < 0 || right > this->screenWidth || bottom > this->screenHeight)
    {
        return false;
    }

    this->clipX = left;
    this->clipY = top;
    this->clipWidth = right - left;
    this->clipHeight = bottom - top;
    return true;
}

std::array<int, 4> Painter::ClipRect() const
{
    return {this->clipX, this->clipY, this->clipWidth, this->clipHeight};
}

int Painter::ScreenPitch() const
{
    return this->screenPitch;
}

u32 Painter::HalfMaskPair(int index) const
{
    return this->halfMaskPairs[static_cast<std::size_t>(index)];
}

// swd32.exe 0x00416FF0: cut to the screen
void Painter::SetClipClamped(int left, int top, int right, int bottom)
{
    if (left < 0)
    {
        left = 0;
    }

    if (top < 0)
    {
        top = 0;
    }

    if (right > this->screenWidth)
    {
        right = this->screenWidth;
    }

    if (bottom > this->screenHeight)
    {
        bottom = this->screenHeight;
    }

    this->clipX = left;
    this->clipHeight = bottom - top;
    this->clipY = top;
    this->clipWidth = right - left;
}

void Painter::SetSource(const void* data)
{
    this->source = static_cast<const u8*>(data);
}

const void* Painter::Source() const
{
    return this->source;
}

void Painter::SetPalette(const u16* palette)
{
    this->palette = palette;
}

DrawOptions& Painter::Options()
{
    return this->options;
}

void Painter::SetWave(int mode)
{
    this->waveMode = mode;
}

int Painter::WavePhase() const
{
    return this->wavePhase;
}

void Painter::SetWavePhase(int phase)
{
    this->wavePhase = phase;
}

void Painter::SetZoom(int anchorX, int anchorY, int zoomX, int zoomY)
{
    this->zoomAnchorX = anchorX;
    this->zoomAnchorY = anchorY;
    this->zoomX = zoomX;
    this->zoomY = zoomY;
}

// The original indexes its row table directly; indices outside the table (negative or from 1024 on) count as 0 here.
std::ptrdiff_t Painter::RowOffset(int y) const
{
    if (y < 0 || static_cast<std::size_t>(y) >= this->rowOffsets.size())
    {
        return 0;
    }

    return this->rowOffsets[static_cast<std::size_t>(y)];
}

u16* Painter::Pixel(int x, int y) const
{
    return this->target + this->RowOffset(y) + x;
}

// swd32.exe 0x00417050: the picture at the four diagonal neighbours as a silhouette
void Painter::DrawOutline(int x, int y, int width, int height, u32 mode, const DrawArgument& argument)
{
    mode |= Painter::Silhouette;
    this->DrawPicture(x + 1, y + 1, width, height, mode, argument);
    this->DrawPicture(x - 1, y - 1, width, height, mode, argument);
    this->DrawPicture(x - 1, y + 1, width, height, mode, argument);
    this->DrawPicture(x + 1, y - 1, width, height, mode, argument);
}

// swd32.exe 0x004170E0: clips the picture against the clip rectangle and runs the blitter of the mode. Vertical
// scaling (DrawOptions::scaleHeight) and shear take the clipping path of the sheared effects; every option is reset
// afterwards.
int Painter::DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument)
{
    if (this->source == nullptr)
    {
        this->options = DrawOptions{};
        return 0;
    }

    int result = 0;
    Placement& place = this->placement;
    // Only the scaled path sets the scale direction and only the sheared path the shear direction: both carry over to
    // the next draws, where an unscaled shadow then repeats source lines after a scaled-up one.
    const bool scaleUp = place.scaleUp;
    const int shearDirection = place.shearDirection;
    place = Placement{};
    place.scaleUp = scaleUp;
    place.shearDirection = shearDirection;
    int bottomSkip = 0;
    int destY = y;
    int destX = x;
    bool visible = true;

    if (ReadPixel(this->source) == this->rleColors.magic && this->palette == nullptr)
    {
        mode |= Painter::RleSource;
    }

    if ((mode & Painter::EffectMask) == Painter::Translucent)
    {
        if (this->options.alpha <= 0)
        {
            visible = false;
        }
        else if (this->options.alpha > Painter::MaxAlpha)
        {
            mode &= Painter::RleSource | Painter::Mirror | Painter::Flip;
        }
    }

    int rows = this->options.scaleHeight;
    bool sheared = false;

    if (visible && rows == 0)
    {
        rows = height;
        place.rows = rows;

        if (this->clipY > y)
        {
            rows = y - this->clipY + height;
            place.rows = rows;
            visible = rows > 0;
            place.skipTop = height - rows;
            destY = this->clipY;
        }

        if (visible && rows + destY >= this->clipHeight + this->clipY)
        {
            bottomSkip = destY - this->clipHeight - this->clipY + height;
            rows = this->clipHeight - destY + this->clipY;
            place.skipBottom = bottomSkip;
            place.rows = rows;
            visible = rows > 0;
        }

        sheared = this->options.shear != 0;

        if (visible && !sheared)
        {
            int columns = width;
            place.width = width;

            if (this->clipX > x)
            {
                columns = x - this->clipX + width;
                place.width = columns;
                visible = columns > 0;
                place.skipLeft = width - columns;
                destX = this->clipX;
            }

            if (visible && columns + destX >= this->clipWidth + this->clipX)
            {
                columns = this->clipWidth - destX + this->clipX;
                place.width = columns;
                visible = columns > 0;
            }
        }
    }
    else if (visible)
    {
        place.scaleUp = false;

        if (rows > height)
        {
            place.scaleFraction = static_cast<u32>((height << Painter::FractionBits) / rows);
            place.scaleUp = true;
        }
        else
        {
            place.scaleFraction =
                static_cast<u32>((height << Painter::FractionBits) / rows - (1 << Painter::FractionBits));
        }

        place.rows = rows;

        if (this->clipY > y)
        {
            rows = rows - this->clipY + y - 1;
            place.rows = rows;
            visible = rows > 0;
            place.skipTop = this->clipY - y;
            destY = this->clipY;
        }

        if (visible && rows + destY >= this->clipHeight + this->clipY)
        {
            bottomSkip = rows - this->clipHeight - this->clipY + destY;
            rows = this->clipHeight - destY + this->clipY - 1;
            place.skipBottom = bottomSkip;
            place.rows = rows;
            visible = rows > 0;
        }

        if (this->options.shear == 0)
        {
            this->options.shear = 1;
        }

        sheared = true;
    }

    if (visible && sheared)
    {
        const int shear = this->options.shear;
        place.shearDirection = shear > 0 ? -1 : 1;
        place.width = width;
        const int magnitude = shear < 0 ? -shear : shear;
        const int span = this->options.scaleHeight != 0 ? this->options.scaleHeight : height;
        place.shearStep = (magnitude << Painter::FractionBits) / span;
        int columns = width;

        if (this->clipX > x)
        {
            columns = x - this->clipX + width;
            place.width = columns;
            visible = columns > 0;
            place.skipLeft = this->clipX - x;
            destX = this->clipX;
        }

        if (visible && columns + destX >= this->clipWidth + this->clipX)
        {
            columns = this->clipWidth - destX + this->clipX;
            place.width = columns;
            visible = columns > 0;
        }
    }

    if (visible)
    {
        u16* dest = this->Pixel(destX, destY);
        u16* destEnd = dest + this->RowOffset(place.rows);
        place.dest = dest;
        place.pitch = this->screenPitch;
        int skip = place.skipTop;

        if ((mode & Painter::Flip) != 0)
        {
            place.skipTop = bottomSkip;
            skip = bottomSkip;
            place.pitch = -this->screenPitch;
            place.dest = destEnd;
        }

        const bool rle = (mode & Painter::RleSource) != 0;

        if (!rle && (mode & Painter::Mirror) != 0)
        {
            place.skipLeft = width - place.skipLeft - 1;
        }

        if (!rle)
        {
            const std::size_t pixelSize = this->palette == nullptr ? sizeof(u16) : sizeof(u8);
            place.sourceLine = this->source + (static_cast<std::size_t>(skip) * width + place.skipLeft) * pixelSize;
        }

        place.ditherPhase = (destY + kDitherOffset) % Painter::DitherPhases;
        const u32 index = LowWord(mode) + (rle ? 0 : Painter::RawBlitters);
        result = this->Dispatch(index, destX, destY, width, height, argument);
    }

    this->options = DrawOptions{};
    return result;
}

// Modes without a blitter (and indices beyond the tables) draw nothing; the original calls through the empty or
// out-of-range table entry.
int Painter::Dispatch(u32 index, int x, int y, int width, int height, const DrawArgument& argument)
{
    if (index >= this->blitters.size() || this->blitters[index] == nullptr)
    {
        return 0;
    }

    return (this->*this->blitters[index])(x, y, width, height, argument);
}

// swd32.exe 0x004174D0: 16x16 tile of 16-bit pixels, not clipped
void Painter::DrawTile16(int x, int y)
{
    u16* dest = this->Pixel(x, y);
    const u8* tile = this->source;

    for (int row = 0; row < Painter::TileSide; ++row)
    {
        std::memcpy(dest, tile, Painter::TileSide * sizeof(u16));
        tile += Painter::TileSide * sizeof(u16);
        dest += this->screenPitch;
    }
}

// swd32.exe 0x00417530: 16x16 tile of 16-bit pixels, the fill colour transparent
void Painter::DrawTile16Keyed(int x, int y)
{
    u16* dest = this->Pixel(x, y);
    const u8* tile = this->source;
    const u16 key = static_cast<u16>(this->fillColorPair);

    for (int row = 0; row < Painter::TileSide; ++row)
    {
        for (int column = 0; column < Painter::TileSide; ++column)
        {
            const u16 pixel = ReadPixel(tile);
            tile += sizeof(u16);

            if (pixel != key)
            {
                dest[column] = pixel;
            }
        }

        dest += this->screenPitch;
    }
}

// swd32.exe 0x004175B0: 16x16 tile of palette indices
void Painter::DrawTile8(int x, int y)
{
    u16* dest = this->Pixel(x, y);
    const u8* tile = this->source;

    for (int row = 0; row < Painter::TileSide; ++row)
    {
        for (int column = 0; column < Painter::TileSide; ++column)
        {
            dest[column] = this->palette[*tile++];
        }

        dest += this->screenPitch;
    }
}

// swd32.exe 0x00417650: 16x16 tile of palette indices, index 1 transparent
void Painter::DrawTile8Keyed(int x, int y)
{
    u16* dest = this->Pixel(x, y);
    const u8* tile = this->source;

    for (int row = 0; row < Painter::TileSide; ++row)
    {
        for (int column = 0; column < Painter::TileSide; ++column)
        {
            const u8 index = *tile++;

            if (index != Painter::KeyIndex)
            {
                dest[column] = this->palette[index];
            }
        }

        dest += this->screenPitch;
    }
}

u32 Painter::FillColorPair() const
{
    return this->fillColorPair;
}

// The palette of a picture as the draw argument of an effect (the original passes the palette pointer).
DrawArgument PaletteArgument(const Picture& picture)
{
    DrawArgument argument;

    if (!picture.palette.empty())
    {
        argument.alphaMap = reinterpret_cast<const u8*>(picture.palette.data());
    }

    if (picture.palette.size() >= 2)
    {
        argument.colorPair = MakeWords(picture.palette[0], picture.palette[1]);
    }

    return argument;
}
}  // namespace swd3
