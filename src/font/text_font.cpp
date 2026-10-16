#include "font/text_font.h"

#include <climits>
#include <cstring>

#include "gfx/graphics.h"
#include "platform/display.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr int kGlyphSurfaceSize = 0x40;
constexpr int kMaxFontSize = 0x40;
constexpr int kCacheFull = 0x7CF;
constexpr int kHighBit = 0x80;  // the leftmost pixel of a glyph byte; a lead byte of Big5 from here
constexpr u32 kWhite = 0xFFFFFF;
constexpr int kBackgroundPad = 2;  // the background of the last character reaches past it

// The colour step per glyph row: -1 for kGradientDown, +1 for kGradientUp, else 0.
int RowStep(int style)
{
    if ((style & TextFont::GradientDown) != 0)
    {
        return -1;
    }

    return (style & TextFont::GradientUp) != 0 ? 1 : 0;
}

// A pixel pair as the original writes it with one 32-bit store: the low half first.
void PutPair(Pixel* target, std::ptrdiff_t index, Pixel color)
{
    target[index] = color;
    target[index + 1] = color;
}

}  // namespace

// swd32.exe 0x004350E0: 16x16 glyphs, no background, the screen as target
TextFont::TextFont(GlyphRasterizer& rasterizer)
    : rasterizer(rasterizer)
{
    this->SetBackground(TextFont::NoBackground);
    this->SetTarget(Display::Width, Display::Height);
}

// swd32.exe 0x00435130
TextFont::~TextFont()
{
    this->FreeGlyphPixels();
    this->FreeGlyphCache();
    this->targetRows.clear();
}

int TextFont::GlyphCount() const
{
    return this->glyphCount;
}

int TextFont::GlyphWidth() const
{
    return this->glyphWidth;
}

int TextFont::GlyphHeight() const
{
    return this->glyphHeight;
}

u16 TextFont::GlyphCode(int glyph) const
{
    return this->codes[static_cast<std::size_t>(glyph)];
}

u8* TextFont::GlyphBits(int glyph)
{
    return this->glyphs.data() + static_cast<std::ptrdiff_t>(glyph) * this->glyphHeight * this->glyphRowBytes;
}

const u8* TextFont::Bits(int glyph) const
{
    return this->glyphs.data() + static_cast<std::ptrdiff_t>(glyph) * this->glyphHeight * this->glyphRowBytes;
}

// The row table of the target; rows outside it (heap memory beyond the table in the original) count as row * width.
std::ptrdiff_t TextFont::RowOffset(int row) const
{
    if (row >= 0 && static_cast<std::size_t>(row) < this->targetRows.size())
    {
        return this->targetRows[static_cast<std::size_t>(row)];
    }

    return static_cast<std::ptrdiff_t>(row) * this->targetWidth;
}

// swd32.exe 0x00435160: targets the back buffer of `graphics` with 16x16 glyphs of the default Big5 font
int TextFont::Init(Graphics& graphics)
{
    this->graphics = &graphics;
    i32 pitch = 0;
    i32 height = 0;
    graphics.BackSize(pitch, height);
    this->SetTarget(pitch / static_cast<i32>(sizeof(Pixel)), height);
    this->SetGlyphSize(TextFont::DefaultGlyphSize, TextFont::DefaultGlyphSize);
    this->SetFont(TextFont::DefaultFace, this->glyphWidth);
    this->SetAdvance(this->glyphWidth);
    this->SetBackground(TextFont::NoBackground);
    this->SetShadow(0);
    this->codes.fill(0);
    this->glyphCount = 0;
    return 0;
}

// swd32.exe 0x004351F0
void TextFont::Shutdown()
{
    this->fontFace.clear();
    this->ReleaseGlyphSurface();
    this->FreeGlyphPixels();
    this->FreeGlyphCache();
    this->targetRows.clear();
}

// swd32.exe 0x00435240: an offscreen surface cleared to black
void TextFont::CreateGlyphSurface(int width, int height)
{
    this->ReleaseGlyphSurface();
    this->glyphSurface = this->graphics != nullptr ? this->graphics->CreateOffscreen(width, height)
                                                   : std::make_unique<Surface>(width, height);
    FillSurfaceColor(*this->glyphSurface, 0);
    this->surfaceHeight = this->glyphSurface->Height();
    this->surfacePitch = this->glyphSurface->Width();
}

// swd32.exe 0x004352C0
void TextFont::ReleaseGlyphSurface()
{
    this->glyphSurface.reset();
}

// swd32.exe 0x004352E0: clears the glyph surface and draws `text` at (0, 0) whatever x and y are
void TextFont::RenderString(int, int, std::string_view text, u32 color)
{
    FillSurfaceColor(*this->glyphSurface, 0);
    this->rasterizer.RenderText(*this->glyphSurface, this->fontFace, this->fontSize, text, color);
}

// swd32.exe 0x004353C0
void TextFont::SetGlyphSize(int width, int height)
{
    this->FreeGlyphPixels();
    this->CreateGlyphSurface(kGlyphSurfaceSize, kGlyphSurfaceSize);
    this->glyphPixels.assign(static_cast<std::size_t>(width) * height, 0);
    this->glyphWidth = width;
    this->glyphHeight = height;
    this->SetAdvance(width);
}

// swd32.exe 0x00435410
void TextFont::FreeGlyphPixels()
{
    this->glyphPixels.clear();
}

// swd32.exe 0x00435430
void TextFont::AllocGlyphCache()
{
    this->FreeGlyphCache();
    this->glyphRowBytes = this->glyphWidth / CHAR_BIT;

    if (this->glyphWidth % CHAR_BIT != 0)
    {
        this->glyphRowBytes = this->glyphWidth / CHAR_BIT + 1;
    }

    this->glyphs.assign(static_cast<std::size_t>(TextFont::MaxGlyphs) * this->glyphRowBytes * this->glyphHeight, 0);
    this->codes.fill(0);
    this->glyphCount = 0;
}

// swd32.exe 0x004354D0
void TextFont::FreeGlyphCache()
{
    this->glyphs.clear();
}

// swd32.exe 0x00435500: a Big5 font of `size` pixels (at most 64) with square glyphs; the cache restarts
void TextFont::SetFont(std::u8string_view face, int size)
{
    if (size > kMaxFontSize)
    {
        size = kMaxFontSize;
    }

    this->fontFace = Big5(face);
    this->fontSize = size;
    this->SetGlyphSize(size, size);
    this->AllocGlyphCache();
}

// swd32.exe 0x004355B0: the row table of a width x height target and a full clip rectangle
void TextFont::SetTarget(int width, int height)
{
    this->targetRows.assign(static_cast<std::size_t>(height), 0);
    this->targetWidth = width;
    this->targetHeight = height;
    this->targetBytes = static_cast<i32>(sizeof(Pixel)) * height * width;
    i32 offset = 0;

    for (i32& row : this->targetRows)
    {
        row = offset;
        offset += width;
    }

    this->SetClip(0, 0, width, height);
}

// swd32.exe 0x00435620
void TextFont::SetClip(int x, int y, int width, int height)
{
    this->clipX = x;
    this->clipY = y;
    this->clipWidth = width;
    this->clipHeight = height;
}

// swd32.exe 0x00435650
void TextFont::SetAdvance(int advance)
{
    this->advance = advance;
}

// swd32.exe 0x00435660
void TextFont::SetBackground(Pixel color)
{
    this->background = color;
}

// swd32.exe 0x00435670
void TextFont::SetShadow(Pixel color)
{
    this->shadow = color;
}

// swd32.exe 0x00435680: one pixel per set bit. A column is drawn when clipX <= x < clipX + clipWidth - 1 (the last
// clip column never is), a row when clipY <= y < clipY + clipHeight. Glyph index glyphCount is accepted.
void TextFont::DrawGlyph(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int left = this->clipX;
    const int top = this->clipY;
    const int right = left + this->clipWidth;
    const int bottom = top + this->clipHeight;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row, ++y)
    {
        if (y >= top && y < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(y);
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - k && column + k + 1 < right)
                    {
                        target[base + CHAR_BIT * byte + k] = color;
                    }
                }
            }
        }

        color = static_cast<Pixel>(color + step);
        bits += this->glyphRowBytes;
    }
}

// swd32.exe 0x004358C0: each set bit paints itself and its right neighbour
void TextFont::DrawGlyphBold(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int left = this->clipX;
    const int top = this->clipY;
    const int right = left + this->clipWidth;
    const int bottom = top + this->clipHeight;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row, ++y)
    {
        if (y >= top && y < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(y);
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - k && column + k + 2 < right)
                    {
                        PutPair(target, base + CHAR_BIT * byte + k, color);
                    }
                }
            }
        }

        color = static_cast<Pixel>(color + step);
        bits += this->glyphRowBytes;
    }
}

// swd32.exe 0x00435AF0: the pixel in `color`, the pixel below and its right neighbour in the shadow colour; a row
// needs the row below inside the clip
void TextFont::DrawGlyphShadow(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int left = this->clipX;
    const int right = left + this->clipWidth;
    const int top = this->clipY;
    const int bottom = top + this->clipHeight;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        const int line = y + row;

        if (line >= top && line + 1 < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(line);
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                const std::ptrdiff_t pixel = base + CHAR_BIT * byte;

                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - k && column + k + 1 < right)
                    {
                        target[pixel + k] = color;
                        PutPair(target, pixel + this->targetWidth + k, this->shadow);
                    }
                }
            }
        }

        color = static_cast<Pixel>(color + step);
        bits += this->glyphRowBytes;
    }
}

// swd32.exe 0x00435D80: a bold pixel pair in `color` and a shadow pair one row down and one pixel right
void TextFont::DrawGlyphBoldShadow(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int left = this->clipX;
    const int right = left + this->clipWidth;
    const int top = this->clipY;
    const int bottom = top + this->clipHeight;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        const int line = y + row;

        if (line >= top && line + 1 < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(line);
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                const std::ptrdiff_t pixel = base + CHAR_BIT * byte;

                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - k && column + k + 2 < right)
                    {
                        PutPair(target, pixel + k, color);
                        PutPair(target, pixel + this->targetWidth + k + 1, this->shadow);
                    }
                }
            }
        }

        color = static_cast<Pixel>(color + step);
        bits += this->glyphRowBytes;
    }
}

// swd32.exe 0x00436030: an outline in the shadow colour (above, a left pair, a pair below-left, below-right) for rows
// strictly below the clip top, then the glyph in the colour already advanced by every glyph row
void TextFont::DrawGlyphOutline(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int right = this->clipX + this->clipWidth;
    const int left = this->clipX;
    const int bottom = this->clipY + this->clipHeight;
    const int top = this->clipY;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        const int line = y + row;

        if (line > top && y + row + 1 < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(line);
            const std::ptrdiff_t width = this->targetWidth;
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                const std::ptrdiff_t pixel = base + CHAR_BIT * byte;

                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - (k + 1) && column + k + 1 < right)
                    {
                        target[pixel + k - width] = this->shadow;
                        PutPair(target, pixel + k - 1, this->shadow);
                        PutPair(target, pixel + width + k - 1, this->shadow);
                        target[pixel + width + k + 1] = this->shadow;
                    }
                }
            }
        }

        bits += this->glyphRowBytes;
        color = static_cast<Pixel>(color + step);
    }

    this->DrawGlyph(target, x, y, glyph, color, style);
}

// swd32.exe 0x00436410: the wider outline of the bold glyph, then the bold glyph in the advanced colour
void TextFont::DrawGlyphBoldOutline(Pixel* target, int x, int y, int glyph, Pixel color, int style)
{
    const int right = this->clipX + this->clipWidth;
    const int left = this->clipX;
    const int bottom = this->clipY + this->clipHeight;
    const int top = this->clipY;

    if (glyph > this->glyphCount)
    {
        return;
    }

    const u8* bits = this->Bits(glyph);
    const int step = RowStep(style);

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        const int line = y + row;

        if (line > top && y + row + 1 < bottom)
        {
            const std::ptrdiff_t base = x + this->RowOffset(line);
            const std::ptrdiff_t width = this->targetWidth;
            int column = x;

            for (int byte = 0; byte < this->glyphRowBytes; ++byte, column += CHAR_BIT)
            {
                const std::ptrdiff_t pixel = base + CHAR_BIT * byte;

                for (int k = 0; k < CHAR_BIT; ++k)
                {
                    if ((bits[byte] & (kHighBit >> k)) != 0 && column >= left - (k + 1) && column + k + 2 < right)
                    {
                        target[pixel + k - width] = this->shadow;
                        target[pixel + k - 1] = this->shadow;
                        target[pixel + k + 2] = this->shadow;
                        PutPair(target, pixel + width + k - 1, this->shadow);
                        PutPair(target, pixel + width + k + 1, this->shadow);
                    }
                }
            }
        }

        bits += this->glyphRowBytes;
        color = static_cast<Pixel>(color + step);
    }

    this->DrawGlyphBold(target, x, y, glyph, color, style);
}

// swd32.exe 0x00436840: copies the glyph area of the glyph surface
void TextFont::CaptureGlyphSurface()
{
    const Pixel* source = this->glyphSurface->Pixels();

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        std::memcpy(
            this->glyphPixels.data() + static_cast<std::ptrdiff_t>(row) * this->glyphWidth,
            source + static_cast<std::ptrdiff_t>(row) * this->surfacePitch,
            sizeof(Pixel) * static_cast<std::size_t>(this->glyphWidth)
        );
    }
}

// swd32.exe 0x004368D0: draws `text` in white and ORs its non-black pixels into the 1-bit `bitmap`
void TextFont::RasterizeGlyph(std::string_view text, u8* bitmap)
{
    this->RenderString(0, 0, text, kWhite);
    this->CaptureGlyphSurface();

    for (int row = 0; row < this->glyphHeight; ++row)
    {
        for (int byte = 0; byte < this->glyphRowBytes; ++byte)
        {
            int mask = kHighBit;
            int column = CHAR_BIT * byte;

            for (int bit = 0; bit < CHAR_BIT && column < this->glyphWidth; ++bit, ++column)
            {
                if (this->glyphPixels[static_cast<std::size_t>(this->glyphWidth * row + column)] != 0)
                {
                    bitmap[this->glyphRowBytes * row + byte] |= static_cast<u8>(mask);
                }

                mask >>= 1;
            }
        }
    }
}

// swd32.exe 0x00436980: binary search of the cache; -1 when missing
int TextFont::FindGlyph(u16 code) const
{
    int low = 0;
    int high = this->glyphCount - 1;

    while (low <= high)
    {
        const int middle = (high + low) >> 1;
        const u16 found = this->codes[static_cast<std::size_t>(middle)];

        if (code < found)
        {
            high = middle - 1;
        }
        else if (code <= found)
        {
            return middle;
        }
        else
        {
            low = middle + 1;
        }
    }

    return -1;
}

// swd32.exe 0x004369C0: makes room for `code` at its sorted position (the caller counts it). With an empty cache the
// bitmap of slot 0 is not cleared.
int TextFont::InsertGlyph(u16 code)
{
    const std::size_t glyphBytes = static_cast<std::size_t>(this->glyphRowBytes) * this->glyphHeight;
    const int count = this->glyphCount;

    if (count <= 0)
    {
        this->codes[0] = code;
        return 0;
    }

    int position = 0;

    while (code > this->codes[static_cast<std::size_t>(position)])
    {
        if (++position >= count)
        {
            break;
        }
    }

    for (int index = count; index > position; --index)
    {
        this->codes[static_cast<std::size_t>(index)] = this->codes[static_cast<std::size_t>(index - 1)];
        std::memcpy(
            this->glyphs.data() + glyphBytes * index,
            this->glyphs.data() + glyphBytes * (index - 1),
            glyphBytes
        );
    }

    this->codes[static_cast<std::size_t>(position)] = code;
    std::memset(this->glyphs.data() + glyphBytes * position, 0, glyphBytes);
    return position;
}

// swd32.exe 0x00436AD0: a Big5 string; single-byte characters advance by half of the advance. A glyph missing from the
// cache is rasterized into it; once the cache would reach 1999 entries the count stays and the slot after the last
// entry is cleared.
void TextFont::DrawText(Pixel* target, int x, int y, std::string_view text, Pixel color, int style)
{
    int offset = 0;
    const int last = static_cast<int>(text.size()) - 1;

    for (int index = 0; index <= last; ++index)
    {
        int half = 0;
        this->character[0] = text[static_cast<std::size_t>(index)];

        if (static_cast<u8>(this->character[0]) < kHighBit)
        {
            this->character[1] = 0;
            half = this->advance >> 1;
        }
        else
        {
            ++index;
            this->character[1] = index <= last ? text[static_cast<std::size_t>(index)] : '\0';
        }

        u16 code = 0;
        std::memcpy(&code, this->character.data(), sizeof(code));
        int glyph = this->FindGlyph(code);
        const bool cached = glyph >= 0;

        if (!cached)
        {
            glyph = this->InsertGlyph(code);
            this->RasterizeGlyph(
                std::string_view(this->character.data(), this->character[1] == '\0' ? 1 : this->character.size()),
                this->GlyphBits(glyph)
            );
        }

        if (this->background != TextFont::NoBackground)
        {
            if (index == last)
            {
                this->FillRect(
                    target,
                    x + offset,
                    y,
                    this->glyphWidth - half + kBackgroundPad,
                    this->glyphHeight,
                    this->background
                );
            }
            else
            {
                this->FillRect(target, x + offset, y, this->advance, this->glyphHeight, this->background);
            }
        }

        if ((style & TextFont::Plain) != 0)
        {
            this->DrawGlyph(target, x + offset, y, glyph, color, style);
        }
        else if ((style & TextFont::Shadow) != 0)
        {
            this->DrawGlyphShadow(target, x + offset, y, glyph, color, style);
        }
        else if ((style & TextFont::BoldShadow) != 0)
        {
            this->DrawGlyphBoldShadow(target, x + offset, y, glyph, color, style);
        }
        else if ((style & TextFont::Outline) != 0)
        {
            this->DrawGlyphOutline(target, x + offset, y, glyph, color, style);
        }
        else if ((style & TextFont::BoldOutline) != 0)
        {
            this->DrawGlyphBoldOutline(target, x + offset, y, glyph, color, style);
        }

        offset += this->advance - half;

        if (!cached)
        {
            const int count = this->glyphCount + 1;
            this->glyphCount = count;

            if (count >= kCacheFull)
            {
                this->glyphCount = count - 1;
                std::memset(
                    this->GlyphBits(count - 1),
                    0,
                    static_cast<std::size_t>(this->glyphHeight) * this->glyphRowBytes
                );
            }
        }
    }
}

// swd32.exe 0x00436EA0: a rectangle clamped to the target; the bottom clamp is tested on the unclamped top
void TextFont::FillRect(Pixel* target, int x, int y, int width, int height, Pixel color)
{
    int left = x;
    int right = x + width;
    const int unclampedBottom = y + height;
    int bottom = y + height;

    if (x < 0)
    {
        left = 0;
    }

    if (right >= this->targetWidth)
    {
        right = this->targetWidth;
    }

    if (y < 0)
    {
        y = 0;
    }

    if (unclampedBottom >= this->targetHeight)
    {
        bottom = this->targetHeight;
    }

    std::size_t rowBytes = 0;

    if (left < right)
    {
        rowBytes = sizeof(Pixel) * static_cast<std::size_t>(right - left);

        for (int column = left; column < right; ++column)
        {
            target[column + this->RowOffset(y)] = color;
        }
    }

    for (int row = y + 1; row < bottom; ++row)
    {
        std::memmove(target + left + this->RowOffset(row), target + left + this->RowOffset(y), rowBytes);
    }
}

// A u8 text, drawn as its Big5.
void TextFont::DrawText(Pixel* target, int x, int y, std::u8string_view text, Pixel color, int style)
{
    this->DrawText(target, x, y, Big5(text), color, style);
}

}  // namespace swd3
