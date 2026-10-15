#include "pic/pixel_format.h"

#include <cstring>

#include "data/code_ptr.h"
#include "data/globals.h"

namespace swd3 {

// swd32.exe 0x004233D0
void GetColorMasks(std::int32_t* red, std::int32_t* green, std::int32_t* blue) {
    *red = G.maskRed;
    *green = G.maskGreen;
    *blue = G.maskBlue;
}

// swd32.exe 0x00423400
int PixelFormatInit(w32::IDirectDrawSurface4* surface) {
    std::uint32_t red;
    std::uint32_t green;
    std::uint32_t blue;
    if (surface) {
        w32::DDSURFACEDESC2 desc;
        std::memset(&desc, 0, sizeof(desc));
        desc.dwSize = 0x7C;
        if (surface->Lock(nullptr, &desc, w32::DDLOCK_WAIT, nullptr) != 0) {
            return 0;
        }
        surface->Unlock(desc.lpSurface);
        G.screenWidth = static_cast<std::int32_t>(desc.dwWidth);
        G.screenPitch = desc.lPitch;
        G.screenHeight = static_cast<std::int32_t>(desc.dwHeight);
        w32::DDPIXELFORMAT format;
        format.dwSize = 0x20;
        format.dwFlags = w32::DDPF_RGB;
        surface->GetPixelFormat(&format);
        blue = format.dwBBitMask;
        green = format.dwGBitMask;
        red = format.dwRBitMask;
    } else {
        red = 0x7C00;
        green = 0x3E0;
        blue = 0x1F;
    }
    G.maskRed = static_cast<std::int32_t>(red);
    G.maskGreen = static_cast<std::int32_t>(green);
    G.maskBlue = static_cast<std::int32_t>(blue);
    if (red == 0x7C00) {
        if (green == 0x3E0 && blue == 0x1F) {
            G.pixelConvert = codeOf(&PixelConvertNone);
            G.pixelConvertBack = codeOf(&PixelConvertNone);
            G.shiftRed = 10;
            G.shiftGreen = 5;
            G.shiftBlue = 0;
        }
    } else if (red == 0xF800) {
        if (green == 0x7C0) {
            if (blue == 0x3F) {
                blue = 0x3E;
                G.pixelConvert = codeOf(&PixelConvert5551);
                G.shiftRed = 11;
                G.shiftGreen = 6;
                G.shiftBlue = 1;
            }
        } else if (green == 0x7E0 && blue == 0x1F) {
            green = 0x7C0;
            G.pixelConvert = codeOf(&PixelConvert555To565);
            G.pixelConvertBack = codeOf(&PixelConvert565To555);
            G.shiftRed = 11;
            G.shiftGreen = 6;
            G.shiftBlue = 0;
        }
    } else if (red == 0xFC00 && green == 0x3E0 && blue == 0x1F) {
        red = 0xF800;
        G.pixelConvert = codeOf(&PixelConvert6551);
        G.shiftRed = 11;
        G.shiftGreen = 5;
        G.shiftBlue = 0;
    }

    G.greenMaskPair = static_cast<std::int32_t>((green << 16) | green);
    const std::uint32_t red16 = red & 0xFFFF;
    G.redHalves[0] = static_cast<std::int32_t>(red16);
    G.redMaskCopy = static_cast<std::int32_t>(red16);
    const std::uint32_t green16 = green & 0xFFFF;
    G.greenHalves[0] = static_cast<std::int32_t>(green16);
    G.greenMaskCopy = static_cast<std::int32_t>(green16);
    const std::uint32_t blue16 = blue & 0xFFFF;
    G.redMaskInverse = static_cast<std::int32_t>(0xFFFFFFFFu - red16);
    G.greenMaskInverse = static_cast<std::int32_t>(0xFFFFFFFFu - green16);
    G.blueHalves[0] = static_cast<std::int32_t>(blue16);
    G.blueMaskCopy = static_cast<std::int32_t>(blue16);
    G.blueMaskInverse = static_cast<std::int32_t>(0xFFFFFFFFu - blue16);

    for (int k = 0; k < 5; ++k) {
        const std::uint32_t step = static_cast<std::uint32_t>(k + 1);
        G.redSteps[k] = static_cast<std::int32_t>(step << G.shiftRed);
        G.greenSteps[k] = static_cast<std::int32_t>(step << G.shiftGreen);
        G.blueSteps[k] = static_cast<std::int32_t>(step << G.shiftBlue);
        const std::uint32_t b = (static_cast<std::uint32_t>(G.blueHalves[k]) >> 1) & blue;
        const std::uint32_t r = (static_cast<std::uint32_t>(G.redHalves[k]) >> 1) & red;
        const std::uint32_t g = (static_cast<std::uint32_t>(G.greenHalves[k]) >> 1) & green;
        G.redHalves[k + 1] = static_cast<std::int32_t>(r);
        G.greenHalves[k + 1] = static_cast<std::int32_t>(g);
        G.blueHalves[k + 1] = static_cast<std::int32_t>(b);
        const std::uint32_t mask = b | g | r;
        G.halfMaskPairs[k] = static_cast<std::int32_t>((mask << 16) | mask);
    }

    G.rleFillColor16 = 0x26B;
    G.fillColorPair = 0x26B;
    G.rleSkipColor16 = 0x319F;
    G.skipColorPair = 0x319F;
    PixelConvert(reinterpret_cast<std::uint16_t*>(&G.greenMaskPair), 1);
    PixelConvert(reinterpret_cast<std::uint16_t*>(&G.fillColorPair), 1);
    PixelConvert(reinterpret_cast<std::uint16_t*>(&G.skipColorPair), 1);
    const std::uint32_t fill = static_cast<std::uint32_t>(G.fillColorPair) & 0xFFFF;
    const std::uint32_t skip = static_cast<std::uint32_t>(G.skipColorPair) & 0xFFFF;
    G.fillColorPair = static_cast<std::int32_t>(fill | (fill << 16));
    G.skipColorPair = static_cast<std::int32_t>(skip | (skip << 16));
    const std::int32_t greenStep = G.greenHalves[4];
    std::memset(G.redZero, 0, sizeof(G.redZero));
    std::memset(G.greenZero, 0, sizeof(G.greenZero));
    const std::int32_t redStep = G.redHalves[4];
    std::memset(G.blueZero, 0, sizeof(G.blueZero));
    const std::int32_t blueStep = G.blueHalves[4];
    G.rleMagic = 0xFFFF;
    G.rleFillColor8 = 1;
    G.dword_4CD778 = 2;
    G.rleSkipColor8 = 3;
    G.dword_4CD304 = 0;
    G.dword_4CD30C = 0;
    G.dword_4CD71C = 0;
    std::int32_t r = 0;
    std::int32_t g = 0;
    std::int32_t b = 0;
    for (int i = 0; i < 32; ++i) {
        G.redRamp[i] = r;
        G.greenRamp[i] = g;
        G.blueRamp[i] = b;
        r += redStep;
        g += greenStep;
        b += blueStep;
    }
    for (std::int32_t& v : G.blueFill) {
        v = static_cast<std::int32_t>(blue);
    }
    for (std::int32_t& v : G.greenFill) {
        v = static_cast<std::int32_t>(green);
    }
    for (std::int32_t& v : G.redFill) {
        v = static_cast<std::int32_t>(red);
    }
    return 1;
}

// swd32.exe 0x004238B0: call through the converter selected for the display pixel format
void PixelConvert(std::uint16_t* pixels, int count) { codeAs<PixelConvertFn>(G.pixelConvert)(pixels, count); }

// swd32.exe 0x004238D0
void PixelConvertBack(std::uint16_t* pixels, int count) { codeAs<PixelConvertFn>(G.pixelConvertBack)(pixels, count); }

// swd32.exe 0x004238F0 (processes at least one pixel)
void PixelConvert5551(std::uint16_t* pixels, int count) {
    do {
        *pixels = static_cast<std::uint16_t>(*pixels << 1);
        ++pixels;
    } while (--count > 0);
}

// swd32.exe 0x00423920 (processes at least one pixel)
void PixelConvert555To565(std::uint16_t* pixels, int count) {
    do {
        const std::uint32_t v = *pixels;
        *pixels = static_cast<std::uint16_t>(((v & 0xFFE0) << 1) + (v & 0x1F));
        ++pixels;
    } while (--count > 0);
}

// swd32.exe 0x00423950 (processes at least one pixel)
void PixelConvert565To555(std::uint16_t* pixels, int count) {
    do {
        const std::uint16_t v = *pixels;
        *pixels = static_cast<std::uint16_t>(((v >> 1) & 0xFFE0) + (v & 0x1F));
        ++pixels;
    } while (--count > 0);
}

// swd32.exe 0x00423990 (processes at least one pixel)
void PixelConvert6551(std::uint16_t* pixels, int count) {
    do {
        const std::uint16_t v = *pixels;
        *pixels = static_cast<std::uint16_t>(((v & 0x7C00) << 1) | (v & 0x3FF));
        ++pixels;
    } while (--count > 0);
}

// swd32.exe 0x004239D0: 5-bit components -> display colour, duplicated in both halves of the result
std::uint32_t MakeColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) {
    std::uint32_t color = ((((red & 0x1F) << 5) | (green & 0x1F)) << 5) | (blue & 0x1F);
    color |= color << 16;
    PixelConvert(reinterpret_cast<std::uint16_t*>(&color), 2);
    return color;
}

// swd32.exe 0x0044A240 (nullsub_1) as stored in G.pixelConvert / G.pixelConvertBack
void PixelConvertNone(std::uint16_t*, int) {}

}  // namespace swd3
