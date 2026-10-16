// Golden generator for the drawing kernel: runs tests/golden/draw_scenarios.h through the legacy transliteration
// (git tag legacy-transliteration) and writes "name hash" lines to $GOLDEN_OUT. Built as an extra test file of the
// legacy tree (see reverse/analysis/golden/README.md).

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "codec/lzo.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "draw/pixel_ops.h"
#include "draw/zoom.h"
#include "golden/draw_scenarios.h"
#include "pic/pixel_format.h"
#include "test.h"

using namespace swd3;

namespace
{

std::vector<std::uint16_t> g_buffer(golden::kScreenWidth * golden::kScreenHeight * 3);

std::uint16_t* Visible()
{
    return g_buffer.data() + golden::kScreenWidth * golden::kScreenHeight;
}

struct Screen565 : swd3test::StubSurface
{
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override
    {
        desc->dwWidth = golden::kScreenWidth;
        desc->dwHeight = golden::kScreenHeight;
        desc->lPitch = golden::kScreenWidth * 2;
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

class LegacyBackend : public golden::Backend
{
public:
    LegacyBackend()
    {
        static Screen565 surface;
        DrawInit(&surface);
        G.drawPalette = nullptr;
        G.waveMode = 0;
    }

    void Reset(std::uint32_t seed) override
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

    void SetClip(int left, int top, int right, int bottom) override
    {
        SetClipRectClamped(left, top, right, bottom);
    }

    int Draw(const golden::DrawCall& call) override
    {
        this->Prepare(call);
        int argument = 0;

        if ((call.mode & 0xFFFC) == 0x08)
        {
            argument = static_cast<int>(reinterpret_cast<std::uintptr_t>(call.alphaMap));
        }
        else if ((call.mode & 0xFFFC) == 0x24)
        {
            this->color = call.colorPair;
            argument = static_cast<int>(reinterpret_cast<std::uintptr_t>(&this->color));
        }

        return DrawPicture(call.x, call.y, call.width, call.height, static_cast<int>(call.mode), argument);
    }

    void Outline(const golden::DrawCall& call) override
    {
        this->Prepare(call);
        this->color = call.colorPair;
        DrawOutline(
            call.x,
            call.y,
            call.width,
            call.height,
            static_cast<int>(call.mode),
            static_cast<int>(reinterpret_cast<std::uintptr_t>(&this->color))
        );
    }

    void Zoom(const void* source, int x, int y, int width, int height, bool mirrored, int anchorX, int anchorY,
              int zoomX, int zoomY) override
    {
        G.drawSource = static_cast<std::uint8_t*>(const_cast<void*>(source));
        G.zoomAnchorX = anchorX;
        G.zoomAnchorY = anchorY;
        G.zoomX = zoomX;
        G.zoomY = zoomY;

        if (mirrored)
        {
            DrawZoomedMirror(x, y, width, height);
        }
        else
        {
            DrawZoomed(x, y, width, height);
        }
    }

    void Tile(int kind, const void* source, const std::uint16_t* palette, int x, int y) override
    {
        G.drawSource = static_cast<std::uint8_t*>(const_cast<void*>(source));
        G.drawPalette = const_cast<std::uint16_t*>(palette);

        switch (kind)
        {
            case 0:
                DrawTile16(x, y);
                break;

            case 1:
                DrawTile16Keyed(x, y);
                break;

            case 2:
                DrawTile8(x, y);
                break;

            default:
                DrawTile8Keyed(x, y);
                break;
        }

        G.drawPalette = nullptr;
    }

    void Pixels(golden::PixelOperation operation, std::uint16_t* pixels, std::size_t count,
                const std::uint16_t* other, int a, int b, int c) override
    {
        const auto n = static_cast<std::uint32_t>(count);

        switch (operation)
        {
            case golden::PixelOperation::Tint:
                TintPixels(pixels, n, a, b, c);
                break;

            case golden::PixelOperation::ShiftRed:
                ShiftRed(pixels, n, a);
                break;

            case golden::PixelOperation::ShiftGreenPairs:
                ShiftGreenPairs(pixels, n, a);
                break;

            case golden::PixelOperation::ShiftBluePairs:
                ShiftBluePairs(pixels, n, a);
                break;

            case golden::PixelOperation::Mix:
                MixPixels(other, pixels, n);
                break;

            case golden::PixelOperation::Gray:
                GrayPixels(pixels, n);
                break;

            case golden::PixelOperation::BlendPairs:
                BlendPairs(reinterpret_cast<std::uint32_t*>(pixels), static_cast<std::uint32_t>(a), n);
                break;
        }
    }

    std::uint32_t MakeColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) override
    {
        return swd3::MakeColor(red, green, blue);
    }

    const std::uint16_t* Screen() const override
    {
        return Visible();
    }

private:
    void Prepare(const golden::DrawCall& call)
    {
        G.drawSource = static_cast<std::uint8_t*>(const_cast<void*>(call.source));
        G.drawPalette = const_cast<std::uint16_t*>(call.palette);
        G.drawAlpha = call.alpha;
        G.drawOption71C = call.tintRed;
        G.drawOption30C = call.tintGreen;
        G.drawOption304 = call.tintBlue;
        G.drawOption300 = call.dropLines ? 1 : 0;
        G.drawScaleHeight = call.scaleHeight;
        G.drawShear = call.shear;
        G.waveMode = call.wave;
    }

    std::uint32_t color = 0;
};

}  // namespace

TEST_CASE("golden: legacy drawing kernel")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    auto decompress = [](const std::uint8_t* input, std::size_t size, std::vector<std::uint8_t>& out)
    {
        std::vector<std::uint8_t> buffer(out.size() + 0x10000);
        std::uint32_t length = 0;
        const int status = Lzo1xDecompress(input, static_cast<std::uint32_t>(size), buffer.data(), &length);

        if (status != 0 || length != out.size())
        {
            return false;
        }

        std::copy_n(buffer.begin(), out.size(), out.begin());
        return true;
    };
    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(
        [gameDir](const std::string& name)
        {
            return std::string(gameDir) + "/" + name;
        },
        decompress
    );
    std::FILE* file = std::fopen(output, "w");
    LegacyBackend backend;
    golden::Run(
        backend,
        frames,
        [file](const std::string& name, std::uint64_t hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    PixelFormatInit(nullptr);
    std::printf("  frames: %zu\n", frames.size());
}
