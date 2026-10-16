#pragma once

// TextFont scenarios shared by the legacy golden generator and the TextFont test (see draw_scenarios.h). No glyphs
// are rasterized on Linux; after the cache is filled every cached slot gets a synthetic bitmap from its character
// code, so that both sides draw the same shapes.

#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

class FontBackend
{
public:
    virtual ~FontBackend() = default;
    virtual void Reset(u32 seed) = 0;
    virtual void DrawText(int x, int y, const char* text, u16 color, int style) = 0;
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

inline void RunFont(FontBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), static_cast<std::size_t>(kScreenWidth) * kScreenHeight * 2);
    };

    // Big5 pairs mixed with ASCII.
    const char* texts[] = {
        "AB\244\244\244\345x",
        "\266}\251l\271C\300\270 swd3",
        "\244@\244G\244T\244|\244\255\244\273\244C\244K\244E\244Q",
        "Zz\244\244",
        "\265\370\270\375\270\366\254O",
    };

    backend.Reset(1);

    for (const char* text : texts)
    {
        backend.DrawText(10, 10, text, 0xFFFF, 1);
    }

    for (int code = 0; code < 120; ++code)
    {
        const char text[3] = {static_cast<char>(0xA4 + code % 7), static_cast<char>(0x40 + (code * 13) % 0x3F), 0};
        backend.DrawText(0, 0, text, 0xFFFF, 1);
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
            bits[i] = static_cast<u8>(Mix32(code * 131u + static_cast<u32>(i)) & 0x7E);
        }
    }

    report("codes", Hash(codes.data(), codes.size() * 2));
    const int styles[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x81, 0x102, 0x84, 0x108, 0x190, 0x20};

    const struct
    {
        int x;
        int y;
        int width;
        int height;
    } clips[] = {{0, 0, 640, 480}, {50, 40, 300, 200}, {13, 7, 101, 33}};

    u32 seed = 2;

    for (int style : styles)
    {
        for (const auto& clip : clips)
        {
            for (int background = 0; background < 2; ++background)
            {
                backend.Reset(seed++);
                backend.SetClip(clip.x, clip.y, clip.width, clip.height);
                backend.SetBackground(background == 0 ? 0xFFFE : 0x1234);
                backend.SetShadow(0x0841);
                int y = clip.y - 8;

                for (const char* text : texts)
                {
                    backend.DrawText(clip.x - 5, y, text, 0x7BEF, style);
                    backend.DrawText(clip.x + clip.width - 40, y + 3, text, 0xF800, style);
                    y += 21;
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
    backend.SetAdvance(22);
    backend.DrawText(100, 100, texts[1], 0x07E0, 1);
    backend.FillRect(-5, -3, 30, 20, 0x1111);
    backend.FillRect(620, 470, 40, 30, 0x2222);
    backend.FillRect(300, 200, 0, 10, 0x3333);
    report("advance-fill", screenHash());
}

}  // namespace golden
