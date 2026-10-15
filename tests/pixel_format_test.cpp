#include <cstring>

#include "data/code_ptr.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "pic/pixel_format.h"
#include "test.h"

using namespace swd3;

namespace {

struct Surface565 : swd3test::StubSurface {
    std::uint16_t pixels[4] = {};
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = 1280;
        desc->lpSurface = pixels;
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

}  // namespace

TEST_CASE("pixel_format.565 surface selects 555<->565 converters and builds tables") {
    Surface565 surface;
    CHECK_EQ(PixelFormatInit(&surface), 1);
    CHECK_EQ(G.screenWidth, 640);
    CHECK_EQ(G.screenPitch, 1280);
    CHECK_EQ(G.maskRed, 0xF800);
    CHECK_EQ(G.maskGreen, 0x7E0);
    CHECK(codeAs<PixelConvertFn>(G.pixelConvert) == &PixelConvert555To565);
    CHECK(codeAs<PixelConvertFn>(G.pixelConvertBack) == &PixelConvert565To555);
    CHECK_EQ(G.shiftRed, 11);
    CHECK_EQ(G.shiftGreen, 6);
    CHECK_EQ(G.redHalves[0], 0xF800);
    CHECK_EQ(G.greenHalves[0], 0x7C0);
    CHECK_EQ(G.redHalves[1], 0x7800);
    CHECK_EQ(G.halfMaskPairs[0], static_cast<std::int32_t>(0x7BCF7BCFu));
    CHECK_EQ(G.redSteps[0], 1 << 11);
    CHECK_EQ(G.rleFillColor16, 0x26B);
    CHECK_EQ(G.fillColorPair, static_cast<std::int32_t>(0x04CB04CBu));
    CHECK_EQ(G.rleMagic, 0xFFFF);
    CHECK_EQ(G.redRamp[2], 2 * G.redHalves[4]);
    CHECK_EQ(G.greenFill[31], 0x7C0);
    std::uint16_t px[2] = {0x7FFF, 0x0421};
    PixelConvert(px, 2);
    CHECK_EQ(px[0], 0xFFDF);
    CHECK_EQ(px[1], 0x0841);
    PixelConvertBack(px, 2);
    CHECK_EQ(px[0], 0x7FFF);
    CHECK_EQ(px[1], 0x0421);
    CHECK_EQ(MakeColor(31, 0, 0), 0xF800F800u);
}

TEST_CASE("pixel_format.null surface keeps 555 and converters process at least one pixel") {
    CHECK_EQ(PixelFormatInit(nullptr), 1);
    CHECK_EQ(G.maskRed, 0x7C00);
    CHECK(codeAs<PixelConvertFn>(G.pixelConvert) == &PixelConvertNone);
    CHECK_EQ(MakeColor(1, 2, 3), 0x04430443u);
    std::uint16_t px[2] = {0x0001, 0x0001};
    PixelConvert5551(px, 0);
    CHECK_EQ(px[0], 2);
    CHECK_EQ(px[1], 1);
}
