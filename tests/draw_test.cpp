#include <cstring>
#include <vector>

#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "pic/pixel_format.h"
#include "test.h"

using namespace swd3;

namespace {

alignas(16) std::uint16_t g_screen[640 * 480];

struct Screen565 : swd3test::StubSurface {
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = 1280;
        desc->lpSurface = g_screen;
        return DD_OK;
    }
    HRESULT Unlock(LPVOID) override { return DD_OK; }
    HRESULT GetPixelFormat(DDPIXELFORMAT* format) override {
        format->dwRBitMask = 0xF800;
        format->dwGBitMask = 0x7E0;
        format->dwBBitMask = 0x1F;
        return DD_OK;
    }
};

// RLE picture: magic 0xFFFF, flags 0x10 at +6, then lines (byte length header + codes), 0 terminator.
std::vector<std::uint16_t> rlePicture(const std::vector<std::vector<std::uint16_t>>& lines) {
    std::vector<std::uint16_t> data{0xFFFF, 0, 0, 0x10};
    for (const auto& line : lines) {
        data.push_back(static_cast<std::uint16_t>((line.size() + 1) * 2));
        data.insert(data.end(), line.begin(), line.end());
    }
    data.push_back(0);
    return data;
}

void setup(std::uint16_t fill) {
    static Screen565 surface;
    CHECK_EQ(DrawInit(&surface), 1);
    G.drawSurface = reinterpret_cast<std::uint8_t*>(g_screen);
    G.drawPalette = nullptr;
    G.waveMode = 0;
    for (auto& px : g_screen) {
        px = fill;
    }
}

// Restores the default pixel format (555, no-op converters) that the other test files expect.
struct RestoreFormat {
    ~RestoreFormat() { PixelFormatInit(nullptr); }
};

std::uint16_t at(int x, int y) { return g_screen[y * 640 + x]; }

int draw(std::vector<std::uint16_t>& picture, int x, int y, int width, int height, int flags, int arg = 0) {
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture.data());
    return DrawPicture(x, y, width, height, flags, arg);
}

}  // namespace

TEST_CASE("draw.rle copy, mirror and left clip") {
    RestoreFormat restore;
    auto picture = rlePicture({{0x0002, 0xA, 0xB, 0x4001, 0x0001, 0xC}, {0x0004, 0xD, 0xE, 0xF, 0x10}});
    setup(0x1111);
    draw(picture, 10, 5, 4, 2, 0);
    CHECK_EQ(at(9, 5), 0x1111);
    CHECK_EQ(at(10, 5), 0xA);
    CHECK_EQ(at(11, 5), 0xB);
    CHECK_EQ(at(12, 5), 0x1111);
    CHECK_EQ(at(13, 5), 0xC);
    CHECK_EQ(at(10, 6), 0xD);
    CHECK_EQ(at(13, 6), 0x10);
    CHECK_EQ(at(10, 7), 0x1111);

    setup(0x1111);
    draw(picture, 10, 5, 4, 2, 1);
    CHECK_EQ(at(13, 5), 0xA);
    CHECK_EQ(at(12, 5), 0xB);
    CHECK_EQ(at(11, 5), 0x1111);
    CHECK_EQ(at(10, 5), 0xC);
    CHECK_EQ(at(10, 6), 0x10);

    setup(0x1111);
    CHECK_EQ(SetClipRectClamped(11, 0, 640, 480), 1);
    draw(picture, 10, 5, 4, 2, 0);
    CHECK_EQ(at(10, 5), 0x1111);
    CHECK_EQ(at(11, 5), 0xB);
    CHECK_EQ(at(13, 5), 0xC);
    CHECK_EQ(at(10, 6), 0x1111);
    CHECK_EQ(at(11, 6), 0xE);
    CHECK_EQ(SetClipRectClamped(0, 0, 640, 480), 1);
}

TEST_CASE("draw.rle blend at G.drawAlpha and option reset") {
    RestoreFormat restore;
    auto picture = rlePicture({{0x0003, 0x8410, 0x8410, 0x8410}});
    setup(0);
    G.drawAlpha = 14;
    draw(picture, 20, 0, 3, 1, 0x14);
    // level 14: source for the 1/2, 1/4, 1/8 terms, destination (0) for 1/16
    CHECK_EQ(at(20, 0), 0x4208 + 0x2104 + 0x1082);
    CHECK_EQ(at(22, 0), 0x4208 + 0x2104 + 0x1082);
    CHECK_EQ(G.drawAlpha, 0);

    setup(0x0841);
    G.drawAlpha = 0;
    draw(picture, 20, 0, 3, 1, 0x14);
    CHECK_EQ(at(20, 0), 0x0841);
}

TEST_CASE("draw.alpha map: aligned literal pixel skipped at level 12") {
    RestoreFormat restore;
    std::vector<std::uint16_t> pixels(6, 0x1234);
    std::vector<std::uint16_t> line{0x0006};
    line.insert(line.end(), pixels.begin(), pixels.end());
    auto picture = rlePicture({line, line});
    std::uint8_t alpha[12];
    std::memset(alpha, 0x30, sizeof(alpha));
    setup(0);
    // x = 11: the destination is not 4-byte aligned, so the run starts with the alignment pixel
    draw(picture, 11, 3, 6, 2, 8, static_cast<int>(reinterpret_cast<std::uintptr_t>(alpha)));
    CHECK_EQ(at(11, 3), 0);
    CHECK_EQ(at(12, 3), 0x1234);
    CHECK_EQ(at(16, 3), 0x1234);
    setup(0);
    draw(picture, 10, 3, 6, 2, 8, static_cast<int>(reinterpret_cast<std::uintptr_t>(alpha)));
    CHECK_EQ(at(10, 3), 0x1234);
}

TEST_CASE("draw.fade starts one level below opaque on the first line") {
    RestoreFormat restore;
    auto picture = rlePicture({{0x0002, 0x8410, 0x8410}});
    setup(0);
    draw(picture, 0, 0, 2, 1, 0x1C);
    CHECK_EQ(at(0, 0), 0x4208 + 0x2104 + 0x1082);
}

TEST_CASE("draw.smear copies neighbours by the cycle counter") {
    RestoreFormat restore;
    auto picture = rlePicture({{0x0006, 1, 1, 1, 1, 1, 1}});
    setup(0);
    for (int x = 0; x < 640; ++x) {
        g_screen[x] = static_cast<std::uint16_t>(x);
        g_screen[640 + x] = static_cast<std::uint16_t>(0x1000 + x);
        g_screen[1280 + x] = static_cast<std::uint16_t>(0x2000 + x);
    }
    G.drawSmearCounter = 3;
    draw(picture, 100, 1, 6, 1, 0x30);
    // steps 4, 5..10: left neighbour once, then the pixel one row below
    CHECK_EQ(at(100, 1), 0x1000 + 99);
    CHECK_EQ(at(101, 1), 0x2000 + 101);
    CHECK_EQ(at(105, 1), 0x2000 + 105);
    CHECK_EQ(G.drawSmearCounter, 9);
}
