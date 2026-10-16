// Golden generator for TextFont: runs tests/golden/font_scenarios.h through the legacy transliteration and writes
// "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "data/globals.h"
#include "font/text_font.h"
#include "gfx/graphics.h"
#include "golden/font_scenarios.h"
#include "test.h"

using namespace swd3;

namespace
{

class LegacyFont : public golden::FontBackend
{
public:
    explicit LegacyFont(TextFont& font) : font(font)
    {
    }

    void Reset(std::uint32_t seed) override
    {
        std::fill(this->buffer.begin(), this->buffer.end(), 0);

        for (int y = 0; y < golden::kScreenHeight; ++y)
        {
            for (int x = 0; x < golden::kScreenWidth; ++x)
            {
                this->Visible()[y * golden::kScreenWidth + x] = golden::Background(seed, x, y);
            }
        }

        this->font.SetClip(0, 0, golden::kScreenWidth, golden::kScreenHeight);
        this->font.SetBackground(kTextNoBackground);
        this->font.SetAdvance(16);
    }

    void DrawText(int x, int y, std::string_view text, std::uint16_t color, int style) override
    {
        this->font.DrawText(this->Visible(), x, y, std::string(text).c_str(), color, style);
    }

    void SetClip(int x, int y, int width, int height) override
    {
        this->font.SetClip(x, y, width, height);
    }

    void SetBackground(std::uint16_t color) override
    {
        this->font.SetBackground(color);
    }

    void SetShadow(std::uint16_t color) override
    {
        this->font.SetShadow(color);
    }

    void SetAdvance(int advance) override
    {
        this->font.SetAdvance(advance);
    }

    void FillRect(int x, int y, int width, int height, std::uint16_t color) override
    {
        this->font.FillRect(this->Visible(), x, y, width, height, color);
    }

    int GlyphCount() override
    {
        return this->font.m_glyphCount;
    }

    std::uint16_t GlyphCode(int slot) override
    {
        return this->font.m_codes[slot];
    }

    std::uint8_t* GlyphBits(int slot) override
    {
        return this->font.m_glyphs + slot * this->GlyphBytes();
    }

    int GlyphBytes() override
    {
        return this->font.m_glyphRowBytes * this->font.m_glyphHeight;
    }

    const std::uint16_t* Screen() override
    {
        return this->Visible();
    }

private:
    std::uint16_t* Visible()
    {
        return this->buffer.data() + golden::kScreenWidth * golden::kScreenHeight;
    }

    TextFont& font;
    std::vector<std::uint16_t> buffer = std::vector<std::uint16_t>(golden::kScreenWidth * golden::kScreenHeight * 3);
};

}  // namespace

TEST_CASE("golden: legacy text font")
{
    const char* output = std::getenv("GOLDEN_OUT");

    if (output == nullptr)
    {
        std::printf("  (GOLDEN_OUT not set, skipped)\n");
        return;
    }

    G.graphics.Init(nullptr, 0x4E22, 640, 480, 16);
    G.textFont_4AB998.Init(&G.graphics);
    std::FILE* file = std::fopen(output, "w");
    LegacyFont backend(G.textFont_4AB998);
    golden::RunFont(
        backend,
        [file](const std::string& name, std::uint64_t hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
}
