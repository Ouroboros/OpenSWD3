#include <cstring>
#include <vector>

#include "crt/crt.h"
#include "data/code_ptr.h"
#include "data/globals.h"
#include "pic/pixel_format.h"
#include "pic/rle.h"
#include "test.h"

using namespace swd3;

namespace {

struct RleKeys {
    RleKeys() {
        G.rleMagic = 0x5254;
        G.rleSkipColor16 = 0x0000;
        G.rleFillColor16 = 0x0821;
        G.rleSkipColor8 = 0;
        G.rleFillColor8 = 1;
    }
};

}  // namespace

TEST_CASE("pic.pixelConvert relocated to the no-op converter") {
    CHECK(codeAs<PixelConvertFn>(G.pixelConvert) == &PixelConvertNone);
    CHECK(codeAs<PixelConvertFn>(G.pixelConvertBack) == &PixelConvertNone);
}

TEST_CASE("pic.rle16 encode decode round trip") {
    RleKeys keys;
    const std::uint32_t w = 7;
    const std::uint32_t h = 3;
    std::uint16_t pixels[w * h] = {
        0x0000, 0x0000, 0x1234, 0x2345, 0x0821, 0x0821, 0x0000,
        0x7FFF, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
        0x0821, 0x0000, 0x0821, 0x0821, 0x0821, 0x4444, 0x0000,
    };
    std::uint32_t size = 0;
    auto* rle = static_cast<std::uint16_t*>(RleEncode(pixels, &size, w, h, 0x10));
    CHECK(rle != nullptr);
    CHECK_EQ(rle[0], 0x5254);
    CHECK_EQ(rle[1], w);
    CHECK_EQ(rle[2], h);
    CHECK_EQ(rle[3], 0x10);
    CHECK_EQ(rle[4] & 0x8000, 0);
    CHECK_EQ(rle[5], 0x8002);
    CHECK_EQ(rle[6], 2);
    std::uint32_t dw = 0, dh = 0, df = 0;
    auto* decoded = static_cast<std::uint16_t*>(RleDecode(rle, &dw, &dh, &df));
    CHECK_EQ(dw, w);
    CHECK_EQ(dh, h);
    CHECK_EQ(df, 0x10u);
    CHECK(std::memcmp(decoded, pixels, sizeof(pixels)) == 0);
    std::uint16_t cw = 0, ch = 0, cf = 0;
    CHECK_EQ(RleCheck16(rle, &cw, &ch, &cf), 1);
    CHECK_EQ(cw, w);
    CHECK_EQ(cf, 0x10);
    crt::free(decoded);
    crt::free(rle);
}

TEST_CASE("pic.rle8 encode decode round trip and 8 to 16 conversion") {
    RleKeys keys;
    const std::uint32_t w = 6;
    const std::uint32_t h = 2;
    std::uint8_t pixels[w * h] = {0, 0, 5, 6, 1, 7, 9, 1, 1, 0, 3, 4};
    std::uint32_t size = 0;
    auto* rle = static_cast<std::uint16_t*>(RleEncode(pixels, &size, w, h, 8));
    CHECK_EQ(rle[3], 8);
    std::uint32_t dw = 0, dh = 0, df = 0;
    auto* decoded = static_cast<std::uint8_t*>(RleDecode(rle, &dw, &dh, &df));
    CHECK_EQ(df, 8u);
    CHECK(std::memcmp(decoded, pixels, sizeof(pixels)) == 0);
    crt::free(decoded);

    auto* palette = static_cast<std::uint16_t*>(crt::malloc(512));
    for (int i = 0; i < 256; ++i) {
        palette[i] = static_cast<std::uint16_t>(0x100 + i);
    }
    Picture picture{};
    picture.pixels = rle;
    picture.palette = palette;
    RleConvert8To16(&picture);
    CHECK(picture.palette == nullptr);
    CHECK_EQ(picture.pixels[3], 0x10);
    std::uint32_t pw = 0, ph = 0, pf = 0;
    auto* converted = static_cast<std::uint16_t*>(RleDecode(picture.pixels, &pw, &ph, &pf));
    CHECK_EQ(pf, 0x10u);
    CHECK_EQ(converted[2], 0x105);
    CHECK_EQ(converted[5], 0x107);
    CHECK_EQ(converted[0], 0);
    CHECK_EQ(converted[4], 0x0821);
    // header 8 + two lines of 18 bytes + 2 for the empty next-line header + 2 for the terminator:
    // the original counts the terminator twice, so the copy carries 2 trailing bytes.
    CHECK_EQ(picture.size, 48u);
    crt::free(converted);
    crt::free(picture.pixels);
}
