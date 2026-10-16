#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>

#include "core/types.h"
#include "font/text_font.h"
#include "gfx/graphics.h"
#include "golden/font_scenarios.h"
#include "platform/display.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u64;
using swd3::u8;

namespace
{

class NullRasterizer : public swd3::GlyphRasterizer
{
public:
    void RenderText(swd3::Surface&, std::string_view, int, std::string_view, u32) override
    {
    }
};

class FontBackend : public golden::FontBackend
{
public:
    explicit FontBackend(swd3::TextFont& font)
        : font(font)
    {
    }

    void Reset(u32 seed) override
    {
        this->screen = std::make_unique<swd3::Surface>(golden::kScreenWidth, golden::kScreenHeight);

        for (int y = 0; y < golden::kScreenHeight; ++y)
        {
            for (int x = 0; x < golden::kScreenWidth; ++x)
            {
                this->screen->Row(y)[x] = golden::Background(seed, x, y);
            }
        }

        this->font.SetClip(0, 0, golden::kScreenWidth, golden::kScreenHeight);
        this->font.SetBackground(swd3::TextFont::kNoBackground);
        this->font.SetAdvance(16);
    }

    void DrawText(int x, int y, std::string_view text, u16 color, int style) override
    {
        this->font.DrawText(this->screen->Pixels(), x, y, text, color, style);
    }

    void SetClip(int x, int y, int width, int height) override
    {
        this->font.SetClip(x, y, width, height);
    }

    void SetBackground(u16 color) override
    {
        this->font.SetBackground(color);
    }

    void SetShadow(u16 color) override
    {
        this->font.SetShadow(color);
    }

    void SetAdvance(int advance) override
    {
        this->font.SetAdvance(advance);
    }

    void FillRect(int x, int y, int width, int height, u16 color) override
    {
        this->font.FillRect(this->screen->Pixels(), x, y, width, height, color);
    }

    int GlyphCount() override
    {
        return this->font.GlyphCount();
    }

    u16 GlyphCode(int slot) override
    {
        return this->font.GlyphCode(slot);
    }

    u8* GlyphBits(int slot) override
    {
        return this->font.GlyphBits(slot);
    }

    int GlyphBytes() override
    {
        return ((this->font.GlyphWidth() + 7) / 8) * this->font.GlyphHeight();
    }

    const u16* Screen() override
    {
        return this->screen->Pixels();
    }

private:
    swd3::TextFont& font;
    std::unique_ptr<swd3::Surface> screen;
};

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> entries;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t space = line.rfind(' ');

        if (space != std::string::npos)
        {
            entries[line.substr(0, space)] = line.substr(space + 1);
        }
    }

    return entries;
}

}  // namespace

TEST_CASE("text font: matches the golden results of the legacy renderer")
{
    const std::map<std::string, std::string> golden = LoadGolden(SWD3_SOURCE_DIR "/tests/golden/font_golden.txt");

    if (golden.empty())
    {
        std::printf("  (golden file not found, skipped)\n");
        return;
    }

    swd3::Display display;
    swd3test::RecordingReporter errors;
    swd3::Graphics graphics(display, errors);
    graphics.Init(swd3::Graphics::kFullScreen, 640, 480, 16);
    NullRasterizer rasterizer;
    swd3::TextFont font(rasterizer);
    font.Init(graphics);
    FontBackend backend(font);
    int compared = 0;
    int mismatches = 0;
    golden::RunFont(
        backend,
        [&](const std::string& name, u64 hash)
        {
            char text[17];
            std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
            const auto found = golden.find(name);
            ++compared;

            if (found == golden.end() || found->second != text)
            {
                if (++mismatches <= 20)
                {
                    std::printf("  mismatch: %s\n", name.c_str());
                }
            }
        }
    );
    std::printf("  compared %d scenarios\n", compared);
    CHECK_EQ(mismatches, 0);
    CHECK_EQ(compared, static_cast<int>(golden.size()));
}
