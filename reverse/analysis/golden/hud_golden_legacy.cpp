// Golden generator for the battle screen helpers: runs tests/golden/hud_scenarios.h through the legacy transliteration
// and writes "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "battle/battle_draw.h"
#include "core/random.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "golden/hud_scenarios.h"
#include "pic/pixel_format.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "test.h"

using namespace swd3;

namespace
{

std::vector<std::uint16_t> g_buffer(3 * golden::kScreenWidth * golden::kScreenHeight);

std::uint16_t* Visible()
{
    return g_buffer.data() + golden::kScreenWidth * golden::kScreenHeight;
}

struct Screen565 : swd3test::StubSurface
{
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override
    {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = 1280;
        desc->lpSurface = Visible();
        return DD_OK;
    }

    HRESULT Unlock(LPVOID) override
    {
        return DD_OK;
    }

    HRESULT GetPixelFormat(DDPIXELFORMAT* format) override
    {
        format->dwRBitMask = 0xF800;
        format->dwGBitMask = 0x7E0;
        format->dwBBitMask = 0x1F;
        return DD_OK;
    }
};

class LegacyHud : public golden::HudBackend
{
public:
    LegacyHud()
    {
        static Screen565 surface;
        DrawInit(&surface);
        std::memset(this->animation, 0, sizeof(this->animation));
    }

    void Reset(golden::u32 seed) override
    {
        std::fill(g_buffer.begin(), g_buffer.end(), 0);

        for (int y = 0; y < golden::kScreenHeight; ++y)
        {
            for (int x = 0; x < golden::kScreenWidth; ++x)
            {
                Visible()[y * golden::kScreenWidth + x] = golden::Background(seed, x, y);
            }
        }

        G.drawSurface = reinterpret_cast<std::uint8_t*>(Visible());
        SetClipRectClamped(0, 0, golden::kScreenWidth, golden::kScreenHeight);
    }

    void SeedRandom(golden::u32 seed) override
    {
        R250Seed(seed);
    }

    int PictureFrame0(int id, int x, int y) override
    {
        return DrawPictureFrame0(id, x, y) != 0;
    }

    int Portrait(int index, int x, int y, int dim, bool highlight) override
    {
        G.portraitHighlight[index] = highlight ? 1 : 0;
        return DrawPortrait(static_cast<std::uint16_t>(index), x, y, dim) != 0;
    }

    int Arrow(int x, int y, int slot) override
    {
        return DrawArrow(x, y, slot) != 0;
    }

    int Bar(int id, int frame, int x, int y, int width) override
    {
        return DrawBar(id, frame, x, y, width) != 0;
    }

    int PictureFrame(int id, int frame, int x, int y) override
    {
        return DrawPictureFrame(id, frame, x, y) != 0;
    }

    int Gauge(int id, int x, int y, int fill) override
    {
        return DrawGauge(id, x, y, fill) != 0;
    }

    int GaugeAlt(int id, int x, int y, int fill) override
    {
        return DrawGaugeAlt(id, x, y, fill);
    }

    int GaugeWide(int id, int x, int y, int fill) override
    {
        return DrawGaugeWide(id, x, y, static_cast<std::uint16_t>(fill)) != 0;
    }

    int SmallNumber(int font, int value, int x, int y) override
    {
        return DrawSmallNumber(font, value, x, y) != 0;
    }

    int Number(int font, int value, int x, int y, int digitMode) override
    {
        G.digitMode = static_cast<std::uint16_t>(digitMode);
        return DrawNumber(static_cast<std::int16_t>(font), value, x, y);
    }

    int SlotAnimation(int id, int slot, int x, int y) override
    {
        return DrawSlotAnimation(id, slot, x, y) != 0;
    }

    int Fill(int x, int y, int width, int height, golden::u32 color) override
    {
        return FillBox(x, y, width, height, color) != 0;
    }

    int Effect(int id, int action, int x, int y, int mirror) override
    {
        return DrawEffectAt(id, action, x, y, mirror);
    }

    int ResetEffect() override
    {
        return ResetEffectAt();
    }

    int Cursor(int id, int x, int y) override
    {
        return DrawCursorAnimation(id, x, y) != 0;
    }

    int Digit(int font, int digit, int x, int y, int digitMode) override
    {
        G.digitMode = static_cast<std::uint16_t>(digitMode);
        return DrawDigit(font, digit, x, y);
    }

    int ScrollBar(int id, int x, int y, int rows, int offset, int style, int count, int first) override
    {
        G.scrollCount = count;
        G.scrollFirst = first;
        return DrawScrollBar(static_cast<std::uint16_t>(id), x, y, rows, offset, style) != 0;
    }

    int CoinToss() override
    {
        return DrawCoinToss();
    }

    int TimingBar(int x, int y) override
    {
        return DrawTimingBar(x, y);
    }

    int TallGauge(int x, int y, int level) override
    {
        return DrawTallGauge(x, y, level) != 0;
    }

    int AnimationLoad(int x, int y, int id, int divisor) override
    {
        return BattleAnimationLoad(this->animation, 0, x, y, static_cast<std::uint16_t>(id),
                                   static_cast<std::uint16_t>(divisor));
    }

    int AnimationDraw() override
    {
        return BattleAnimationDraw(this->animation);
    }

    int AnimationPlayShifted(int amount) override
    {
        return BattleAnimationPlayShifted(this->animation, 0, amount);
    }

    int AnimationFree() override
    {
        return BattleAnimationFree(this->animation);
    }

    std::vector<golden::i32> State() override
    {
        return {G.numberX,         G.numberValue,       G.scrollStep,   G.scrollThumbTop,
                G.scrollThumbBottom, G.coinFrames,       static_cast<golden::i32>(G.coinState),
                static_cast<golden::i32>(G.coinDelay), G.timingCounter, G.timingHit};
    }

    const golden::u16* Screen() const override
    {
        return Visible();
    }

private:
    std::uint8_t animation[0xC4];
};

}  // namespace

TEST_CASE("golden: legacy battle hud")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    std::snprintf(G.gameDir, sizeof(G.gameDir), "%s/", gameDir);
    LegacyHud backend;
    std::FILE* file = std::fopen(output, "w");
    golden::RunHud(
        backend,
        [file](const std::string& name, golden::u64 hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    TswCloseAll();
    PixelFormatInit(nullptr);
}
