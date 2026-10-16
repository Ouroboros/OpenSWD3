#include "font/text_font.h"

#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"
#include "draw/draw.h"
#include "gfx/graphics.h"

namespace swd3 {

using namespace w32;

namespace {

constexpr int kMaxGlyphs = 2000;
constexpr int kGlyphSurfaceSize = 0x40;
constexpr int kMaxFontSize = 0x40;

void put16(std::uint16_t* target, std::int32_t pixel, std::uint16_t color) {
    std::memcpy(reinterpret_cast<std::uint8_t*>(target) + 2 * static_cast<std::intptr_t>(pixel), &color, 2);
}

void put32(std::uint16_t* target, std::int32_t pixel, std::uint32_t pair) {
    std::memcpy(reinterpret_cast<std::uint8_t*>(target) + 2 * static_cast<std::intptr_t>(pixel), &pair, 4);
}

// Colour step per glyph row: -1 for kTextGradientDown, +1 for kTextGradientUp, else 0.
std::int16_t rowStep(int style) {
    if (style & kTextGradientDown) {
        return -1;
    }
    return (style & kTextGradientUp) ? 1 : 0;
}

}  // namespace

// swd32.exe 0x004350E0
TextFont::TextFont() {
    m_glyphPixels = nullptr;
    m_glyphSurface = nullptr;
    m_glyphs = nullptr;
    m_glyphHeight = 16;
    m_glyphWidth = 16;
    m_font = nullptr;
    SetBackground(kTextNoBackground);
    const int height = GetSystemMetrics(SM_CYSCREEN);
    SetTarget(GetSystemMetrics(SM_CXSCREEN), height);
}

// swd32.exe 0x00435130
TextFont::~TextFont() {
    FreeGlyphPixels();
    FreeGlyphCache();
    if (m_targetRows != nullptr) {
        crt::free(m_targetRows);
        m_targetRows = nullptr;
    }
}

// swd32.exe 0x00435160: targets the back buffer of `graphics`, 16x16 glyphs of the Big5 font at 0x0049FB74
int TextFont::Init(Graphics* graphics) {
    m_graphics = graphics;
    LONG pitch;
    LONG height;
    graphics->GetBackSize(&pitch, &height);
    SetTarget(pitch / 2, height);
    SetGlyphSize(16, 16);
    SetFont(G.defaultFontFace, m_glyphWidth);
    SetAdvance(m_glyphWidth);
    SetBackground(kTextNoBackground);
    SetShadow(0);
    std::memset(m_codes, 0, sizeof(m_codes));
    m_glyphCount = 0;
    return 0;
}

// swd32.exe 0x004351F0
void TextFont::Shutdown() {
    if (m_font != nullptr) {
        DeleteObject(m_font);
        m_font = nullptr;
    }
    ReleaseGlyphSurface();
    FreeGlyphPixels();
    FreeGlyphCache();
    if (m_targetRows != nullptr) {
        crt::free(m_targetRows);
        m_targetRows = nullptr;
    }
}

// swd32.exe 0x00435240
void TextFont::CreateGlyphSurface(int width, int height) {
    ReleaseGlyphSurface();
    m_glyphSurface = m_graphics->CreateOffscreen(width, height);
    FillSurfaceColor(m_glyphSurface, 0);
    DDSURFACEDESC2 desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(DDSURFACEDESC2);
    desc.dwFlags = DDSD_HEIGHT | DDSD_PITCH;
    m_glyphSurface->GetSurfaceDesc(&desc);
    m_surfaceHeight = static_cast<std::int32_t>(desc.dwHeight);
    m_surfacePitch = static_cast<std::int32_t>(static_cast<DWORD>(desc.lPitch) >> 1);
}

// swd32.exe 0x004352C0
void TextFont::ReleaseGlyphSurface() {
    if (m_glyphSurface != nullptr) {
        m_glyphSurface->Release();
        m_glyphSurface = nullptr;
    }
}

// swd32.exe 0x004352E0: clears the glyph surface and draws `text` at (0, 0) whatever x and y are; ReleaseDC is
// called even when GetDC failed (with an unset DC in the original, none here)
HRESULT TextFont::RenderString(HGDIOBJ font, int, int, const char* text, COLORREF color) {
    if (m_glyphSurface->IsLost() == DDERR_SURFACELOST) {
        m_glyphSurface->Restore();
    }
    FillSurfaceColor(m_glyphSurface, 0);
    HDC dc = nullptr;
    if (m_glyphSurface->GetDC(&dc) == 0) {
        HGDIOBJ previous = SelectObject(dc, font);
        SetBkColor(dc, 0);
        SetBkMode(dc, kBkOpaque);
        SetTextColor(dc, color);
        TextOutA(dc, 0, 0, "  ", 2);
        TextOutA(dc, 0, 0, text, static_cast<int>(std::strlen(text)));
        SelectObject(dc, previous);
    }
    return m_glyphSurface->ReleaseDC(dc);
}

// swd32.exe 0x004353C0
void TextFont::SetGlyphSize(int width, int height) {
    FreeGlyphPixels();
    CreateGlyphSurface(kGlyphSurfaceSize, kGlyphSurfaceSize);
    m_glyphPixels = static_cast<std::uint16_t*>(crt::malloc(static_cast<std::uint32_t>(2 * height * width)));
    if (m_glyphPixels != nullptr) {
        m_glyphWidth = width;
        m_glyphHeight = height;
        SetAdvance(width);
    }
}

// swd32.exe 0x00435410
void TextFont::FreeGlyphPixels() {
    if (m_glyphPixels != nullptr) {
        crt::free(m_glyphPixels);
        m_glyphPixels = nullptr;
    }
}

// swd32.exe 0x00435430
void TextFont::AllocGlyphCache() {
    FreeGlyphCache();
    m_glyphRowBytes = m_glyphWidth / 8;
    if (m_glyphWidth % 8 != 0) {
        m_glyphRowBytes = m_glyphWidth / 8 + 1;
    }
    const auto bytes = static_cast<std::uint32_t>(kMaxGlyphs * m_glyphRowBytes * m_glyphHeight);
    m_glyphs = static_cast<std::uint8_t*>(crt::malloc(bytes));
    if (m_glyphs != nullptr) {
        std::memset(m_codes, 0, sizeof(m_codes));
        m_glyphCount = 0;
        std::memset(m_glyphs, 0, kMaxGlyphs * m_glyphRowBytes * m_glyphHeight);
    }
}

// swd32.exe 0x004354D0
void TextFont::FreeGlyphCache() {
    if (m_glyphs != nullptr) {
        crt::free(m_glyphs);
        m_glyphs = nullptr;
    }
}

// swd32.exe 0x00435500: Big5 font of `size` pixels (at most 64), square glyphs, cache reset
void TextFont::SetFont(const char* faceName, int size) {
    if (size > kMaxFontSize) {
        size = kMaxFontSize;
    }
    if (m_font != nullptr) {
        DeleteObject(m_font);
        m_font = nullptr;
    }
    LOGFONTA font;
    font.lfHeight = -size;
    font.lfWidth = 0;
    font.lfEscapement = 0;
    font.lfOrientation = 0;
    font.lfWeight = 400;
    font.lfItalic = 0;
    font.lfUnderline = 0;
    font.lfStrikeOut = 0;
    font.lfCharSet = kChineseBig5Charset;
    font.lfOutPrecision = 0;
    font.lfClipPrecision = 0;
    font.lfQuality = 0;
    font.lfPitchAndFamily = 0;
    lstrcpyA(font.lfFaceName, faceName);
    m_font = CreateFontIndirectA(&font);
    SetGlyphSize(size, size);
    AllocGlyphCache();
}

// swd32.exe 0x004355B0: row table of a width x height target; the old table is freed but a failed allocation
// leaves the dangling pointer replaced by null
void TextFont::SetTarget(int width, int height) {
    if (m_targetRows != nullptr) {
        crt::free(m_targetRows);
    }
    m_targetRows = static_cast<std::int32_t*>(crt::malloc(static_cast<std::uint32_t>(4 * height)));
    if (m_targetRows == nullptr) {
        return;
    }
    m_targetWidth = width;
    m_targetHeight = height;
    m_targetBytes = 2 * height * width;
    std::int32_t offset = 0;
    for (int row = 0; row < height; ++row) {
        m_targetRows[row] = offset;
        offset += width;
    }
    SetClip(0, 0, width, height);
}

// swd32.exe 0x00435620
void TextFont::SetClip(int x, int y, int width, int height) {
    m_clipX = x;
    m_clipY = y;
    m_clipWidth = width;
    m_clipHeight = height;
}

// swd32.exe 0x00435650
void TextFont::SetAdvance(int advance) { m_advance = advance; }

// swd32.exe 0x00435660
void TextFont::SetBackground(std::uint16_t color) { m_background = color; }

// swd32.exe 0x00435670
void TextFont::SetShadow(std::uint16_t color) { m_shadow = color; }

// swd32.exe 0x00435680: one pixel per set bit; a column is drawn when clipX <= x < clipX + clipWidth - 1 (the
// last clip column is never drawn), rows when clipY <= y < clipY + clipHeight. Glyph index m_glyphCount is
// accepted (one past the cache).
void TextFont::DrawGlyph(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style) {
    const std::int32_t left = m_clipX;
    const std::int32_t top = m_clipY;
    const std::int32_t right = left + m_clipWidth;
    const std::int32_t bottom = top + m_clipHeight;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    for (int row = 0; row < m_glyphHeight; ++row, ++y) {
        if (y >= top && y < bottom) {
            const std::int32_t base = x + m_targetRows[y];
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - k && column + k + 1 < right) {
                        put16(target, base + 8 * byte + k, color);
                    }
                }
            }
        }
        color = static_cast<std::uint16_t>(color + step);
        bits += m_glyphRowBytes;
    }
}

// swd32.exe 0x004358C0: each set bit paints itself and its right neighbour
void TextFont::DrawGlyphBold(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style) {
    const std::int32_t left = m_clipX;
    const std::int32_t top = m_clipY;
    const std::int32_t right = left + m_clipWidth;
    const std::int32_t bottom = top + m_clipHeight;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    for (int row = 0; row < m_glyphHeight; ++row, ++y) {
        if (y >= top && y < bottom) {
            const std::int32_t base = x + m_targetRows[y];
            const std::uint32_t pair = color | (static_cast<std::uint32_t>(color) << 16);
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - k && column + k + 2 < right) {
                        put32(target, base + 8 * byte + k, pair);
                    }
                }
            }
        }
        color = static_cast<std::uint16_t>(color + step);
        bits += m_glyphRowBytes;
    }
}

// swd32.exe 0x00435AF0: pixel in `color`, the pixel below and its right neighbour in the shadow colour; rows need
// the row below inside the clip
void TextFont::DrawGlyphShadow(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style) {
    const std::int32_t left = m_clipX;
    const std::int32_t right = left + m_clipWidth;
    const std::int32_t top = m_clipY;
    const std::int32_t bottom = top + m_clipHeight;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    const std::uint32_t shadow = m_shadow | (static_cast<std::uint32_t>(m_shadow) << 16);
    for (int row = 0; row < m_glyphHeight; ++row) {
        const std::int32_t line = y + row;
        if (line >= top && line + 1 < bottom) {
            const std::int32_t base = x + m_targetRows[line];
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                const std::int32_t pixel = base + 8 * byte;
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - k && column + k + 1 < right) {
                        put16(target, pixel + k, color);
                        put32(target, pixel + m_targetWidth + k, shadow);
                    }
                }
            }
        }
        color = static_cast<std::uint16_t>(color + step);
        bits += m_glyphRowBytes;
    }
}

// swd32.exe 0x00435D80: bold pixel pair in `color`, shadow pair one row down and one pixel right
void TextFont::DrawGlyphBoldShadow(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color,
                                   int style) {
    const std::int32_t left = m_clipX;
    const std::int32_t right = left + m_clipWidth;
    const std::int32_t top = m_clipY;
    const std::int32_t bottom = top + m_clipHeight;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    const std::uint32_t shadow = m_shadow | (static_cast<std::uint32_t>(m_shadow) << 16);
    for (int row = 0; row < m_glyphHeight; ++row) {
        const std::int32_t line = y + row;
        if (line >= top && line + 1 < bottom) {
            const std::int32_t base = x + m_targetRows[line];
            const std::uint32_t pair = color | (static_cast<std::uint32_t>(color) << 16);
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                const std::int32_t pixel = base + 8 * byte;
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - k && column + k + 2 < right) {
                        put32(target, pixel + k, pair);
                        put32(target, pixel + m_targetWidth + k + 1, shadow);
                    }
                }
            }
        }
        color = static_cast<std::uint16_t>(color + step);
        bits += m_glyphRowBytes;
    }
}

// swd32.exe 0x00436030: outline in the shadow colour (above, left pair, below-left pair, below-right) for rows
// strictly below the clip top, then the glyph with the colour already advanced by every glyph row
void TextFont::DrawGlyphOutline(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style) {
    const std::int32_t right = m_clipX + m_clipWidth;
    const std::int32_t left = m_clipX;
    const std::int32_t bottom = m_clipY + m_clipHeight;
    const std::int32_t top = m_clipY;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    const std::uint32_t shadow = m_shadow | (static_cast<std::uint32_t>(m_shadow) << 16);
    for (int row = 0; row < m_glyphHeight; ++row) {
        const std::int32_t line = y + row;
        if (line > top && y + row + 1 < bottom) {
            const std::int32_t base = x + m_targetRows[line];
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                const std::int32_t pixel = base + 8 * byte;
                const std::int32_t width = m_targetWidth;
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - (k + 1) && column + k + 1 < right) {
                        put16(target, pixel + k - width, m_shadow);
                        put32(target, pixel + k - 1, shadow);
                        put32(target, pixel + width + k - 1, shadow);
                        put16(target, pixel + width + k + 1, m_shadow);
                    }
                }
            }
        }
        bits += m_glyphRowBytes;
        color = static_cast<std::uint16_t>(color + step);
    }
    DrawGlyph(target, x, y, glyph, color, style);
}

// swd32.exe 0x00436410: wider outline for the bold glyph, then DrawGlyphBold with the advanced colour
void TextFont::DrawGlyphBoldOutline(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color,
                                    int style) {
    const std::int32_t right = m_clipX + m_clipWidth;
    const std::int32_t left = m_clipX;
    const std::int32_t bottom = m_clipY + m_clipHeight;
    const std::int32_t top = m_clipY;
    if (glyph > m_glyphCount) {
        return;
    }
    const std::uint8_t* bits = m_glyphs + glyph * m_glyphHeight * m_glyphRowBytes;
    const std::int16_t step = rowStep(style);
    const std::uint32_t shadow = 0x10001u * m_shadow;
    for (int row = 0; row < m_glyphHeight; ++row) {
        const std::int32_t line = y + row;
        if (line > top && y + row + 1 < bottom) {
            const std::int32_t base = x + m_targetRows[line];
            std::int32_t column = x;
            for (int byte = 0; byte < m_glyphRowBytes; ++byte, column += 8) {
                const std::uint8_t b = bits[byte];
                const std::int32_t pixel = base + 8 * byte;
                const std::int32_t width = m_targetWidth;
                for (int k = 0; k < 8; ++k) {
                    if ((b & (0x80 >> k)) && column >= left - (k + 1) && column + k + 2 < right) {
                        put16(target, pixel + k - width, m_shadow);
                        put16(target, pixel + k - 1, m_shadow);
                        put16(target, pixel + k + 2, m_shadow);
                        put32(target, pixel + width + k - 1, shadow);
                        put32(target, pixel + width + k + 1, shadow);
                    }
                }
            }
        }
        bits += m_glyphRowBytes;
        color = static_cast<std::uint16_t>(color + step);
    }
    DrawGlyphBold(target, x, y, glyph, color, style);
}

// swd32.exe 0x00436840: copies the glyph area of the glyph surface into m_glyphPixels
HRESULT TextFont::CaptureGlyphSurface() {
    std::int32_t source = 0;
    std::int32_t dest = 0;
    std::uint8_t* pixels = LockSurface(m_glyphSurface);
    for (int row = 0; row < m_glyphHeight; ++row) {
        std::memcpy(reinterpret_cast<std::uint8_t*>(m_glyphPixels) + 2 * dest, pixels + 2 * source,
                    static_cast<std::size_t>(2 * m_glyphWidth));
        source += m_surfacePitch;
        dest += m_glyphWidth;
    }
    return UnlockSurface(m_glyphSurface, pixels);
}

// swd32.exe 0x004368D0: draws `text` in white with GDI and ORs its non-black pixels into the 1-bit `bitmap`
void TextFont::RasterizeGlyph(const char* text, std::uint8_t* bitmap) {
    RenderString(m_font, 0, 0, text, 0xFFFFFF);
    CaptureGlyphSurface();
    for (int row = 0; row < m_glyphHeight; ++row) {
        for (int byte = 0; byte < m_glyphRowBytes; ++byte) {
            int mask = 0x80;
            int column = 8 * byte;
            for (int bit = 0; bit < 8; ++bit, ++column) {
                if (column >= m_glyphWidth) {
                    break;
                }
                if (m_glyphPixels[m_glyphWidth * row + column] != 0) {
                    bitmap[m_glyphRowBytes * row + byte] |= static_cast<std::uint8_t>(mask);
                }
                mask >>= 1;
            }
        }
    }
}

// swd32.exe 0x00436980: binary search of the cache
int TextFont::FindGlyph(std::uint16_t code) {
    int low = 0;
    int high = m_glyphCount - 1;
    while (low <= high) {
        const int middle = (high + low) >> 1;
        const std::uint16_t found = m_codes[middle];
        if (code < found) {
            high = middle - 1;
        } else if (code <= found) {
            return middle;
        } else {
            low = middle + 1;
        }
    }
    return -1;
}

// swd32.exe 0x004369C0: makes room for `code` at its sorted position (the caller counts it). With an empty cache
// the bitmap at index 0 is not cleared.
int TextFont::InsertGlyph(std::uint16_t code) {
    const std::uint32_t glyphBytes = static_cast<std::uint32_t>(m_glyphRowBytes * m_glyphHeight);
    const int count = m_glyphCount;
    if (count <= 0) {
        m_codes[0] = code;
        return 0;
    }
    int position = 0;
    while (code > m_codes[position]) {
        if (++position >= count) {
            break;
        }
    }
    for (int index = count; index > position; --index) {
        m_codes[index] = m_codes[index - 1];
        std::memcpy(m_glyphs + glyphBytes * index, m_glyphs + glyphBytes * (index - 1), glyphBytes);
    }
    m_codes[position] = code;
    std::memset(m_glyphs + position * glyphBytes, 0, glyphBytes);
    return position;
}

// swd32.exe 0x00436AD0: draws a Big5 string; single-byte characters advance by half of m_advance. Glyphs missing
// from the cache are rasterized and inserted; when the cache reaches 1999 entries the count stays and the slot
// after the last entry is cleared.
void TextFont::DrawText(std::uint16_t* target, int x, int y, const char* text, std::uint16_t color, int style) {
    std::int32_t offset = 0;
    const int last = static_cast<int>(std::strlen(text)) - 1;
    for (int index = 0; index <= last; ++index) {
        std::int32_t half;
        m_character[0] = text[index];
        if (static_cast<std::uint8_t>(m_character[0]) < 0x80) {
            m_character[1] = 0;
            half = m_advance >> 1;
        } else {
            ++index;
            m_character[1] = text[index];
            half = 0;
        }
        std::uint16_t code;
        std::memcpy(&code, m_character, 2);
        int glyph = FindGlyph(code);
        const bool cached = glyph >= 0;
        if (!cached) {
            glyph = InsertGlyph(code);
            RasterizeGlyph(m_character, m_glyphs + m_glyphRowBytes * glyph * m_glyphHeight);
        }
        if (m_background != kTextNoBackground) {
            if (index == last) {
                FillRect(target, x + offset, y, m_glyphWidth - half + 2, m_glyphHeight, m_background);
            } else {
                FillRect(target, x + offset, y, m_advance, m_glyphHeight, m_background);
            }
        }
        if (style & kTextPlain) {
            DrawGlyph(target, x + offset, y, glyph, color, style);
        } else if (style & kTextShadow) {
            DrawGlyphShadow(target, x + offset, y, glyph, color, style);
        } else if (style & kTextBoldShadow) {
            DrawGlyphBoldShadow(target, x + offset, y, glyph, color, style);
        } else if (style & kTextOutline) {
            DrawGlyphOutline(target, x + offset, y, glyph, color, style);
        } else if (style & kTextBoldOutline) {
            DrawGlyphBoldOutline(target, x + offset, y, glyph, color, style);
        }
        offset += m_advance - half;
        if (!cached) {
            const std::int32_t count = m_glyphCount + 1;
            m_glyphCount = count;
            if (count >= 0x7CF) {
                m_glyphCount = count - 1;
                std::memset(m_glyphs + m_glyphHeight * m_glyphRowBytes * (count - 1), 0,
                            static_cast<std::size_t>(m_glyphHeight * m_glyphRowBytes));
            }
        }
    }
}

// swd32.exe 0x00436EA0: fills a rectangle clamped to the target; the bottom clamp is tested on the unclamped top
void TextFont::FillRect(std::uint16_t* target, int x, int y, int width, int height, std::uint16_t color) {
    std::int32_t left = x;
    std::int32_t right = x + width;
    const std::int32_t unclampedBottom = y + height;
    std::int32_t bottom = y + height;
    if (x < 0) {
        left = 0;
    }
    if (right >= m_targetWidth) {
        right = m_targetWidth;
    }
    if (y < 0) {
        y = 0;
    }
    if (unclampedBottom >= m_targetHeight) {
        bottom = m_targetHeight;
    }
    std::uint32_t rowBytes = 0;
    if (left < right) {
        rowBytes = static_cast<std::uint32_t>(2 * (right - left));
        for (std::int32_t column = left; column < right; ++column) {
            put16(target, column + m_targetRows[y], color);
        }
    }
    for (std::int32_t row = y + 1; row < bottom; ++row) {
        std::memcpy(reinterpret_cast<std::uint8_t*>(target) + 2 * (left + m_targetRows[row]),
                    reinterpret_cast<std::uint8_t*>(target) + 2 * (left + m_targetRows[y]), rowBytes);
    }
}

}  // namespace swd3
