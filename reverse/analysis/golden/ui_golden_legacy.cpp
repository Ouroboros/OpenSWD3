// Golden generator for the boxes and window frames: runs tests/golden/ui_scenarios.h through the legacy transliteration
// and writes "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "golden/ui_scenarios.h"
#include "pic/pixel_format.h"
#include "res/tsw.h"
#include "test.h"
#include "ui/ui_draw.h"

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

class LegacyUi : public golden::UiBackend
{
public:
    LegacyUi()
    {
        static Screen565 surface;
        DrawInit(&surface);
    }

    void Reset(golden::u32 seed, int left, int top, int right, int bottom) override
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
        SetClipRectClamped(left, top, right, bottom);
    }

    void Fill(int x, int y, int width, int height, int red, int green, int blue, int mode) override
    {
        FillBoxMode(x, y, width, height, red, green, blue, mode);
    }

    void Frame(int id, int left, int top, int right, int bottom, int alpha, int border) override
    {
        DrawWindowFrame(id, left, top, right, bottom, alpha, border);
    }

    const golden::u16* Screen() const override
    {
        return Visible();
    }
};

}  // namespace

TEST_CASE("golden: legacy ui drawing")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    std::snprintf(G.gameDir, sizeof(G.gameDir), "%s/", gameDir);
    LegacyUi backend;
    std::FILE* file = std::fopen(output, "w");
    golden::RunUi(
        backend,
        std::string(gameDir) + "/all_sys.tsw",
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
