#include "draw/painter.h"

#include <cstring>

#include "platform/surface.h"

namespace swd3
{

namespace
{

u16 ReadPixel(const u8* bytes)
{
    u16 value = 0;
    std::memcpy(&value, bytes, 2);
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
    return this->ramp[static_cast<std::size_t>(index + 32)];
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
        this->blitters[mode | draw_mode::kMirror] = mirrored;
    };

    auto rleFlippable = [this](u32 mode, Blitter forward, Blitter mirrored)
    {
        this->blitters[mode] = forward;
        this->blitters[mode | draw_mode::kMirror] = mirrored;
        this->blitters[mode | draw_mode::kFlip] = forward;
        this->blitters[mode | draw_mode::kFlip | draw_mode::kMirror] = mirrored;
    };

    rleFlippable(draw_mode::kCopy, &Painter::BlitCopy, &Painter::BlitCopyMirrored);
    rleFlippable(draw_mode::kShadow, &Painter::BlitShadow, &Painter::BlitShadowMirrored);
    rleFlippable(draw_mode::kTranslucent, &Painter::BlitTranslucent, &Painter::BlitTranslucentMirrored);
    rleFlippable(draw_mode::kSilhouette, &Painter::BlitSilhouette, &Painter::BlitSilhouetteMirrored);
    rleFlippable(draw_mode::kGray, &Painter::BlitGray, &Painter::BlitGrayMirrored);
    rleFlippable(draw_mode::kSmear, &Painter::BlitSmear, &Painter::BlitSmearMirrored);
    rle(draw_mode::kAdditive, &Painter::BlitAdditive, &Painter::BlitAdditiveMirrored);
    rle(draw_mode::kAlphaMap, &Painter::BlitAlphaMap, &Painter::BlitAlphaMapMirrored);
    rle(draw_mode::kTint, &Painter::BlitTint, &Painter::BlitTintMirrored);
    rle(draw_mode::kEdgeMark, &Painter::BlitEdgeMark, &Painter::BlitEdgeMarkMirrored);
    this->blitters[draw_mode::kFadeDown] = &Painter::BlitFadeDown;
    rle(draw_mode::kScaledGlow, &Painter::BlitScaledGlow, &Painter::BlitScaledGlowMirrored);
    rle(draw_mode::kSubtractive, &Painter::BlitSubtractive, &Painter::BlitSubtractiveMirrored);

    constexpr u32 kRaw = 0x80;
    this->blitters[kRaw + draw_mode::kRawCopy] = &Painter::BlitRawCopy;
    this->blitters[kRaw + (draw_mode::kRawCopy | draw_mode::kMirror)] = &Painter::BlitRawCopyMirrored;
    this->blitters[kRaw + draw_mode::kRawKeyed] = &Painter::BlitRawKeyed;
    this->blitters[kRaw + (draw_mode::kRawKeyed | draw_mode::kMirror)] = &Painter::BlitRawKeyedMirrored;
    this->blitters[kRaw + draw_mode::kRawGradient] = &Painter::BlitRawGradient;
    this->blitters[kRaw + draw_mode::kRawTranslucent] = &Painter::BlitRawTranslucent;
}

// swd32.exe 0x00423400: PixelFormatInit. Detects the converter of the display format; masks of an unknown format
// keep the converter and shifts of the previous initialization. Every channel table uses the masks as adjusted for
// the converter (RGB565 keeps a 5-bit green at bits 6..10), and the tint options are cleared.
bool Painter::InitPixelFormat(const Surface* screen)
{
    u32 redMask = 0x7C00;
    u32 greenMask = 0x3E0;
    u32 blueMask = 0x1F;

    if (screen != nullptr)
    {
        this->screenWidth = screen->Width();
        this->screenPitch = screen->Width();
        this->screenHeight = screen->Height();
        redMask = Surface::kRedMask;
        greenMask = Surface::kGreenMask;
        blueMask = Surface::kBlueMask;
    }

    this->red.mask = redMask;
    this->green.mask = greenMask;
    this->blue.mask = blueMask;
    this->displayRed = redMask;
    this->displayGreen = greenMask;
    this->displayBlue = blueMask;

    if (redMask == 0x7C00)
    {
        if (greenMask == 0x3E0 && blueMask == 0x1F)
        {
            this->conversion = Conversion::None;
            this->conversionBack = Conversion::None;
            this->red.shift = 10;
            this->green.shift = 5;
            this->blue.shift = 0;
        }
    }
    else if (redMask == 0xF800)
    {
        if (greenMask == 0x7C0)
        {
            if (blueMask == 0x3F)
            {
                blueMask = 0x3E;
                this->conversion = Conversion::To5551;
                this->red.shift = 11;
                this->green.shift = 6;
                this->blue.shift = 1;
            }
        }
        else if (greenMask == 0x7E0 && blueMask == 0x1F)
        {
            greenMask = 0x7C0;
            this->conversion = Conversion::To565;
            this->conversionBack = Conversion::To565;
            this->red.shift = 11;
            this->green.shift = 6;
            this->blue.shift = 0;
        }
    }
    else if (redMask == 0xFC00 && greenMask == 0x3E0 && blueMask == 0x1F)
    {
        redMask = 0xF800;
        this->conversion = Conversion::To6551;
        this->red.shift = 11;
        this->green.shift = 5;
        this->blue.shift = 0;
    }

    this->greenMaskPair = (greenMask << 16) | greenMask;
    const u32 masks[3] = {redMask, greenMask, blueMask};
    ColorChannel* channels[3] = {&this->red, &this->green, &this->blue};

    for (int i = 0; i < 3; ++i)
    {
        ColorChannel& channel = *channels[i];
        channel.mask = masks[i] & 0xFFFF;
        channel.halves[0] = masks[i] & 0xFFFF;
        channel.inverseMask = 0xFFFFFFFFu - channel.mask;
    }

    for (int k = 0; k < 5; ++k)
    {
        u32 pair = 0;

        for (int i = 0; i < 3; ++i)
        {
            ColorChannel& channel = *channels[i];
            channel.steps[k] = static_cast<u32>(k + 1) << channel.shift;
            channel.halves[k + 1] = (channel.halves[k] >> 1) & masks[i];
            pair |= channel.halves[k + 1];
        }

        this->halfMaskPairs[k] = (pair << 16) | pair;
    }

    this->rleColors = RleColors{};
    u16 converted[3] = {
        static_cast<u16>(this->greenMaskPair),
        this->rleColors.fill16,
        this->rleColors.skip16,
    };

    this->ConvertPixels(std::span<u16>(converted, 1));
    this->ConvertPixels(std::span<u16>(converted + 1, 1));
    this->ConvertPixels(std::span<u16>(converted + 2, 1));
    this->greenMaskPair = (this->greenMaskPair & 0xFFFF0000u) | converted[0];
    this->fillColorPair = converted[1] | (static_cast<u32>(converted[1]) << 16);
    this->skipColorPair = converted[2] | (static_cast<u32>(converted[2]) << 16);

    for (int i = 0; i < 3; ++i)
    {
        ColorChannel& channel = *channels[i];
        const u32 step = channel.halves[4];

        for (int k = 0; k < 32; ++k)
        {
            channel.ramp[static_cast<std::size_t>(k)] = 0;
            channel.ramp[static_cast<std::size_t>(32 + k)] = step * static_cast<u32>(k);
            channel.ramp[static_cast<std::size_t>(64 + k)] = masks[i];
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
    const auto color = static_cast<u16>((((red & 0x1F) << 5 | (green & 0x1F)) << 5) | (blue & 0x1F));
    u16 halves[2] = {color, color};
    this->ConvertPixels(halves);
    return halves[0] | (static_cast<u32>(halves[1]) << 16);
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
    mode |= draw_mode::kSilhouette;
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

    if (ReadPixel(this->source) == 0xFFFF && this->palette == nullptr)
    {
        mode |= draw_mode::kRle;
    }

    if ((mode & draw_mode::kEffectMask) == draw_mode::kTranslucent)
    {
        if (this->options.alpha <= 0)
        {
            visible = false;
        }
        else if (this->options.alpha > 0xF)
        {
            mode &= draw_mode::kRle | draw_mode::kMirror | draw_mode::kFlip;
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
            place.scaleFraction = static_cast<u32>((height << 10) / rows);
            place.scaleUp = true;
        }
        else
        {
            place.scaleFraction = static_cast<u32>((height << 10) / rows - 0x400);
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
        place.shearStep = (magnitude << 10) / span;
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

        if ((mode & draw_mode::kFlip) != 0)
        {
            place.skipTop = bottomSkip;
            skip = bottomSkip;
            place.pitch = -this->screenPitch;
            place.dest = destEnd;
        }

        const bool rle = (mode & draw_mode::kRle) != 0;

        if (!rle && (mode & draw_mode::kMirror) != 0)
        {
            place.skipLeft = width - place.skipLeft - 1;
        }

        if (!rle)
        {
            const std::size_t pixelSize = this->palette == nullptr ? 2 : 1;
            place.sourceLine = this->source + (static_cast<std::size_t>(skip) * width + place.skipLeft) * pixelSize;
        }

        place.ditherPhase = (destY + 0x1E0) % 3;
        const u32 index = (mode & 0xFFFF) + (rle ? 0 : 0x80);
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

    for (int row = 0; row < 16; ++row)
    {
        std::memcpy(dest, tile, 32);
        tile += 32;
        dest += this->screenPitch;
    }
}

// swd32.exe 0x00417530: 16x16 tile of 16-bit pixels, the fill colour transparent
void Painter::DrawTile16Keyed(int x, int y)
{
    u16* dest = this->Pixel(x, y);
    const u8* tile = this->source;
    const u16 key = static_cast<u16>(this->fillColorPair);

    for (int row = 0; row < 16; ++row)
    {
        for (int column = 0; column < 16; ++column)
        {
            const u16 pixel = ReadPixel(tile);
            tile += 2;

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

    for (int row = 0; row < 16; ++row)
    {
        for (int column = 0; column < 16; ++column)
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

    for (int row = 0; row < 16; ++row)
    {
        for (int column = 0; column < 16; ++column)
        {
            const u8 index = *tile++;

            if (index != 1)
            {
                dest[column] = this->palette[index];
            }
        }

        dest += this->screenPitch;
    }
}

}  // namespace swd3
