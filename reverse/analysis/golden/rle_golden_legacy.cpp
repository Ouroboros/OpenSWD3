// Golden generator for the RLE codec: runs tests/golden/rle_scenarios.h through the legacy transliteration and writes
// "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "codec/lzo.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "golden/rle_scenarios.h"
#include "pic/pixel_format.h"
#include "pic/rle.h"
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

void* Allocate(const void* data, std::size_t size)
{
    void* block = crt::malloc(static_cast<std::uint32_t>(size));
    std::memcpy(block, data, size);
    return block;
}

class LegacyRle : public golden::RleBackend
{
public:
    std::vector<std::uint8_t> Encode(const void* pixels, int width, int height, int format) override
    {
        std::uint32_t size = 0;
        void* data = RleEncode(pixels, &size, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
                               format);
        std::vector<std::uint8_t> out(static_cast<std::uint8_t*>(data), static_cast<std::uint8_t*>(data) + size);
        crt::free(data);
        return out;
    }

    std::vector<std::uint8_t> Decode(const std::vector<std::uint8_t>& rle) override
    {
        std::vector<std::uint8_t> copy = rle;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t format = 0;
        void* pixels = RleDecode(reinterpret_cast<const std::uint16_t*>(copy.data()), &width, &height, &format);

        if (pixels == nullptr)
        {
            return {};
        }

        const std::size_t size = static_cast<std::size_t>(width) * height * (format == 0x10 ? 2 : 1);
        std::vector<std::uint8_t> out(static_cast<std::uint8_t*>(pixels), static_cast<std::uint8_t*>(pixels) + size);
        crt::free(pixels);
        golden::Append32(out, width);
        golden::Append32(out, height);
        golden::Append32(out, format);
        return out;
    }

    std::vector<std::uint8_t> ConvertToDisplay(const std::vector<std::uint8_t>& rle) override
    {
        std::vector<std::uint8_t> copy = rle;
        std::uint16_t width = 0;
        std::uint16_t height = 0;
        std::uint16_t format = 0;
        const int result = RleCheck16(reinterpret_cast<std::uint16_t*>(copy.data()), &width, &height, &format);
        golden::Append32(copy, static_cast<std::uint32_t>(result));
        golden::Append32(copy, width);
        golden::Append32(copy, height);
        golden::Append32(copy, format);
        return copy;
    }

    std::vector<std::uint8_t> ConvertTo16(const std::vector<std::uint8_t>& rle,
                                          const std::vector<std::uint16_t>& palette) override
    {
        Picture picture{};
        picture.pixels = static_cast<std::uint16_t*>(Allocate(rle.data(), rle.size()));
        picture.palette = palette.empty() ? nullptr : static_cast<std::uint16_t*>(Allocate(palette.data(), 0x200));
        picture.size = static_cast<std::uint32_t>(rle.size());
        RleConvert8To16(&picture);
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(picture.pixels);
        std::vector<std::uint8_t> out(bytes, bytes + picture.size);
        crt::free(picture.pixels);
        crt::free(picture.palette);
        golden::Append32(out, picture.width);
        golden::Append32(out, picture.height);
        return out;
    }

    std::vector<std::uint8_t> ConvertTileTo16(const std::vector<std::uint8_t>& tile) override
    {
        auto* data = static_cast<std::uint16_t*>(Allocate(tile.data(), tile.size()));
        std::uint32_t size = static_cast<std::uint32_t>(tile.size());
        RleConvertTile8To16(&data, &size);
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
        std::vector<std::uint8_t> out(bytes, bytes + size);
        crt::free(data);
        return out;
    }
};

}  // namespace

TEST_CASE("golden: legacy rle codec")
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
    auto gameFile = [gameDir](const std::string& name)
    {
        return std::string(gameDir) + "/" + name;
    };
    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(gameFile, decompress);
    const std::vector<golden::PaletteFrame> palettes = golden::LoadGoldenPaletteFrames(gameFile, decompress);
    std::FILE* file = std::fopen(output, "w");
    LegacyRle backend;
    golden::RunRle(
        backend,
        frames,
        palettes,
        [file](const std::string& name, std::uint64_t hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    PixelFormatInit(nullptr);
    std::printf("  frames: %zu + %zu\n", frames.size(), palettes.size());
}
