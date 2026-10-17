#pragma once

// TextFont scenarios shared by the legacy golden generator and the TextFont test (see draw_scenarios.h). No glyphs
// are rasterized on Linux; after the cache is filled every cached slot gets a synthetic bitmap from its character
// code, so that both sides draw the same shapes.

#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "golden/draw_scenarios.h"
#include "text/big5.h"

namespace golden
{

class FontBackend
{
public:
    virtual ~FontBackend() = default;
    virtual void Reset(u32 seed) = 0;
    virtual void DrawText(int x, int y, std::string_view text, u16 color, int style) = 0;
    virtual void SetClip(int x, int y, int width, int height) = 0;
    virtual void SetBackground(u16 color) = 0;
    virtual void SetShadow(u16 color) = 0;
    virtual void SetAdvance(int advance) = 0;
    virtual void FillRect(int x, int y, int width, int height, u16 color) = 0;
    virtual int GlyphCount() = 0;
    virtual u16 GlyphCode(int slot) = 0;
    virtual u8* GlyphBits(int slot) = 0;
    virtual int GlyphBytes() = 0;
    virtual const u16* Screen() = 0;
};

constexpr int kFontTextX = 10;
constexpr int kFontTextY = 10;
constexpr u16 kFontWhite = 0xFFFF;
constexpr int kFontPlainStyle = 1;
constexpr int kFillerCodes = 120;  // characters drawn to fill the glyph cache
constexpr int kFillerLead = 0xA4;  // their lead bytes run over kFillerLeads values, the trail bytes over kFillerTrails
constexpr int kFillerLeads = 7;
constexpr int kFillerTrail = 0x40;
constexpr int kFillerTrailStep = 13;
constexpr int kFillerTrails = 0x3F;
constexpr u32 kGlyphSeed = 131;  // the synthetic glyph bits
constexpr u32 kGlyphMask = 0x7E;
constexpr std::array<u16, 2> kFontBackgrounds = {0xFFFE, 0x1234};
constexpr u16 kFontShadow = 0x0841;
constexpr int kFontAbove = 8;  // the first line starts above the clip rectangle
constexpr int kFontLeftOut = 5;
constexpr int kFontRightIn = 40;  // the second text of a line runs past the right edge
constexpr int kFontRightDrop = 3;
constexpr int kFontLineHeight = 21;
constexpr u16 kFontLeftColor = 0x7BEF;
constexpr u16 kFontRightColor = 0xF800;
constexpr int kFontAdvance = 22;
constexpr int kFontAdvanceX = 100;
constexpr int kFontAdvanceY = 100;
constexpr u16 kFontAdvanceColor = 0x07E0;
constexpr u32 kFontFirstSeed = 2;  // seed 1 is the cache filling screen

struct FontClip
{
    int x;
    int y;
    int width;
    int height;
};

struct FontFill
{
    int x;
    int y;
    int width;
    int height;
    u16 color;
};

constexpr std::array<int, 11> kFontStyles = {0x1, 0x2, 0x4, 0x8, 0x10, 0x81, 0x102, 0x84, 0x108, 0x190, 0x20};
constexpr std::array<FontClip, 3> kFontClips = {
    {{0, 0, kScreenWidth, kScreenHeight}, {50, 40, 300, 200}, {13, 7, 101, 33}}
};

constexpr std::array<FontFill, 3> kFontFills = {{
    {-5, -3, 30, 20, 0x1111},
    {620, 470, 40, 30, 0x2222},
    {300, 200, 0, 10, 0x3333},
}};

inline void RunFont(FontBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), kScreenBytes);
    };

    // Big5 pairs mixed with ASCII.
    const std::array<std::string, 5> texts = {
        swd3::Big5(u8"AB中文x"),
        swd3::Big5(u8"開始遊戲 swd3"),
        swd3::Big5(u8"一二三弋五六七八九十"),
        swd3::Big5(u8"Zz中"),
        swd3::Big5(u8"視軾跺是"),
    };

    backend.Reset(1);

    for (const std::string& text : texts)
    {
        backend.DrawText(kFontTextX, kFontTextY, text, kFontWhite, kFontPlainStyle);
    }

    for (int code = 0; code < kFillerCodes; ++code)
    {
        const std::string text = {
            static_cast<char>(kFillerLead + code % kFillerLeads),
            static_cast<char>(kFillerTrail + (code * kFillerTrailStep) % kFillerTrails)
        };

        backend.DrawText(0, 0, text, kFontWhite, kFontPlainStyle);
    }

    report("count", static_cast<u64>(backend.GlyphCount()));
    std::vector<u16> codes;

    for (int slot = 0; slot <= backend.GlyphCount(); ++slot)
    {
        const u16 code = backend.GlyphCode(slot);
        codes.push_back(code);
        u8* bits = backend.GlyphBits(slot);

        for (int i = 0; i < backend.GlyphBytes(); ++i)
        {
            bits[i] = static_cast<u8>(Mix32(code * kGlyphSeed + static_cast<u32>(i)) & kGlyphMask);
        }
    }

    report("codes", Hash(codes.data(), codes.size() * sizeof(u16)));
    u32 seed = kFontFirstSeed;

    for (const int style : kFontStyles)
    {
        for (const FontClip& clip : kFontClips)
        {
            for (std::size_t background = 0; background < kFontBackgrounds.size(); ++background)
            {
                backend.Reset(seed++);
                backend.SetClip(clip.x, clip.y, clip.width, clip.height);
                backend.SetBackground(kFontBackgrounds[background]);
                backend.SetShadow(kFontShadow);
                int y = clip.y - kFontAbove;

                for (const std::string& text : texts)
                {
                    backend.DrawText(clip.x - kFontLeftOut, y, text, kFontLeftColor, style);
                    backend
                        .DrawText(clip.x + clip.width - kFontRightIn, y + kFontRightDrop, text, kFontRightColor, style);
                    y += kFontLineHeight;
                }

                report(
                    "text/" + std::to_string(style) + "/" + std::to_string(clip.width) + "/" +
                        std::to_string(background),
                    screenHash()
                );
            }
        }
    }

    backend.Reset(seed++);
    backend.SetAdvance(kFontAdvance);
    backend.DrawText(kFontAdvanceX, kFontAdvanceY, texts[1], kFontAdvanceColor, kFontPlainStyle);

    for (const FontFill& fill : kFontFills)
    {
        backend.FillRect(fill.x, fill.y, fill.width, fill.height, fill.color);
    }

    report("advance-fill", screenHash());
}

}  // namespace golden
