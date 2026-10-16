// Golden generator for the tsw archives: runs tests/golden/tsw_scenarios.h through the legacy transliteration and
// writes "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "crt/crt.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "golden/tsw_scenarios.h"
#include "pic/pixel_format.h"
#include "res/tsw.h"
#include "test.h"

using namespace swd3;

namespace
{

std::vector<std::uint16_t> g_screen(640 * 480);

struct Screen565 : swd3test::StubSurface
{
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override
    {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = 1280;
        desc->lpSurface = g_screen.data();
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

golden::PictureResult Result(const Picture* picture)
{
    golden::PictureResult result;

    if (picture == nullptr)
    {
        return result;
    }

    result.present = true;

    if (picture->pixels != nullptr)
    {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(picture->pixels);
        result.data.assign(bytes, bytes + picture->size);
    }

    result.width = picture->width;
    result.height = picture->height;
    result.paletteEntries = picture->palette != nullptr ? 256 : 0;
    return result;
}

class LegacyTsw : public golden::TswBackend
{
public:
    void SetCacheLimit(int bytes) override
    {
        TswSetCacheLimit(bytes);
    }

    golden::PictureResult Get(std::uint32_t id, std::uint32_t frame) override
    {
        return Result(TswGetPicture(id, frame));
    }

    golden::PictureResult Find(std::uint32_t id, std::uint32_t frame) override
    {
        return Result(TswFindPicture(id, frame));
    }

    golden::PictureResult Direct(std::uint32_t id, std::uint32_t frame) override
    {
        Picture* picture = TswLoadPictureDirect(id, frame);
        golden::PictureResult result = Result(picture);
        crt::free(picture->pixels);
        crt::free(picture);
        return result;
    }

    int CacheBytes() override
    {
        return G.tswCacheBytes;
    }

    void CloseAll() override
    {
        TswCloseAll();
    }
};

}  // namespace

TEST_CASE("golden: legacy tsw archives")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    static Screen565 surface;
    DrawInit(&surface);
    std::snprintf(G.gameDir, sizeof(G.gameDir), "%s/", gameDir);
    std::FILE* file = std::fopen(output, "w");
    LegacyTsw backend;
    golden::RunTsw(
        backend,
        [file](const std::string& name, std::uint64_t hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    TswCloseAll();
    PixelFormatInit(nullptr);
}
