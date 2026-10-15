#include "draw/pixel_ops.h"

#include "draw/blit_tint.h"

namespace swd3 {

using namespace blit;

namespace {

std::uint8_t* bytes(std::uint16_t* pixels) { return reinterpret_cast<std::uint8_t*>(pixels); }

// Shared body of 0x00420600 / 0x004206F0: one channel of two pixels per step. Only the high pixel's overflow
// is detected with the channel's inverse mask (shifted up); a low pixel overflow is detected on the low word.
// When shifting down, a borrow from the low pixel propagates into the high pixel's channel. The count drops by
// 2 per step and the loop ends only at exactly 0, so an odd count runs past the buffer.
void shiftChannelPairs(std::uint16_t* pixels, std::uint32_t count, int delta, std::uint32_t mask,
                       std::uint32_t inverse, std::int32_t step) {
    const std::uint32_t highInverse = inverse << 16;
    const std::uint32_t mask2 = mask | (mask << 16);
    const std::uint32_t notMask2 = 0xFFFFFFFFu - mask2;
    std::uint8_t* p = bytes(pixels);
    if (delta >= 0) {
        std::uint32_t saturated = mask << 16;
        std::uint32_t offset = static_cast<std::uint32_t>(step * delta);
        offset |= offset << 16;
        do {
            const std::uint32_t d = u32(p);
            std::uint32_t v = (d & mask2) + offset;
            if (notMask2 & v & 0xFFFF) {
                v = (v & 0xFFFF0000u) | (mask & 0xFFFF);
            }
            if (highInverse & v) {
                saturated = (saturated & 0xFFFF0000u) | (v & 0xFFFF);
                v = saturated;
            }
            put32(p, (d & notMask2) | v);
            p += 4;
            count -= 2;
        } while (count != 0);
        return;
    }
    std::uint32_t offset = static_cast<std::uint32_t>(-(step * delta));
    offset |= offset << 16;
    do {
        const std::uint32_t d = u32(p);
        std::uint32_t v = (d & mask2) - offset;
        if (notMask2 & v & 0xFFFF) {
            v &= 0xFFFF0000u;
        }
        if (highInverse & v) {
            v &= 0xFFFF;
        }
        put32(p, (d & notMask2) | v);
        p += 4;
        count -= 2;
    } while (count != 0);
}

}  // namespace

// swd32.exe 0x00420490: adds red/green/blue steps to each pixel, saturating like the tint blitters
// (count 0 wraps around)
void TintPixels(std::uint16_t* pixels, std::uint32_t count, int red, int green, int blue) {
    Tint t{};
    t.redOffset = static_cast<std::uint32_t>(G.redHalves[4] * red);
    t.greenOffset = static_cast<std::uint32_t>(G.greenHalves[4] * green);
    t.blueOffset = static_cast<std::uint32_t>(G.blueHalves[4] * blue);
    t.redSaturate = red >= 0 ? static_cast<std::uint32_t>(G.redMaskCopy) : 0;
    t.greenSaturate = green >= 0 ? static_cast<std::uint32_t>(G.greenMaskCopy) : 0;
    t.blueSaturate = blue >= 0 ? static_cast<std::uint32_t>(G.blueMaskCopy) : 0;
    std::uint8_t* p = bytes(pixels);
    do {
        put16(p, tintStep(t, u16(p)));
        p += 2;
    } while (--count != 0);
}

// swd32.exe 0x00420560: moves the red channel by delta steps, saturating at the mask / 0
void ShiftRed(std::uint16_t* pixels, std::uint32_t count, int delta) {
    const auto mask = static_cast<std::uint32_t>(G.redMaskCopy);
    const std::uint32_t notMask = 0xFFFFFFFFu - mask;
    std::uint8_t* p = bytes(pixels);
    if (delta >= 0) {
        const auto offset = static_cast<std::uint32_t>(G.redHalves[4] * delta);
        do {
            const std::uint32_t d = u16(p);
            std::uint32_t r = (d & mask) + offset;
            if (notMask & r) {
                r = mask;
            }
            put16(p, (d & notMask) | r);
            p += 2;
        } while (--count != 0);
        return;
    }
    const auto offset = static_cast<std::uint32_t>(-(G.redHalves[4] * delta));
    do {
        const std::uint32_t d = u16(p);
        std::uint32_t r = (d & mask) - offset;
        if (notMask & r) {
            r = 0;
        }
        put16(p, (d & notMask) | r);
        p += 2;
    } while (--count != 0);
}

// swd32.exe 0x00420600: green channel, two pixels per step
void ShiftGreenPairs(std::uint16_t* pixels, std::uint32_t count, int delta) {
    shiftChannelPairs(pixels, count, delta, static_cast<std::uint32_t>(G.greenMaskCopy),
                      static_cast<std::uint32_t>(G.greenMaskInverse), G.greenHalves[4]);
}

// swd32.exe 0x004206F0: blue channel, two pixels per step
void ShiftBluePairs(std::uint16_t* pixels, std::uint32_t count, int delta) {
    shiftChannelPairs(pixels, count, delta, static_cast<std::uint32_t>(G.blueMaskCopy),
                      static_cast<std::uint32_t>(G.blueMaskInverse), G.blueHalves[4]);
}

// swd32.exe 0x004207E0: each pixel becomes the saturated sum of itself and the matching pixel of `other`,
// per channel through the ramp tables. Truncates the three channel masks to 16 bits first.
void MixPixels(const std::uint16_t* other, std::uint16_t* pixels, std::uint32_t count) {
    G.redMaskCopy &= 0xFFFF;
    G.greenMaskCopy &= 0xFFFF;
    G.blueMaskCopy &= 0xFFFF;
    const std::ptrdiff_t gap = reinterpret_cast<const std::uint8_t*>(other) - bytes(pixels);
    std::uint8_t* p = bytes(pixels);
    for (; count != 0; --count) {
        const std::uint32_t a = u16(p + gap);
        const std::uint32_t s = u16(p);
        const auto rm = static_cast<std::uint32_t>(G.redMaskCopy);
        const auto gm = static_cast<std::uint32_t>(G.greenMaskCopy);
        const auto bm = static_cast<std::uint32_t>(G.blueMaskCopy);
        const std::uint32_t b = ((bm & s) + (bm & a)) >> G.shiftBlue;
        const std::uint32_t g = ((gm & s) + (gm & a)) >> G.shiftGreen;
        const std::uint32_t r = ((rm & s) + (rm & a)) >> G.shiftRed;
        const auto* blue = reinterpret_cast<const std::uint8_t*>(&G.blueRamp[0]) + b * 4;
        const auto* green = reinterpret_cast<const std::uint8_t*>(&G.greenRamp[0]) + g * 4;
        const auto* redRamp = reinterpret_cast<const std::uint8_t*>(&G.redRamp[0]) + r * 4;
        put16(p, static_cast<std::uint16_t>(u16(blue) + u16(green) + u16(redRamp)));
        p += 2;
    }
}

// swd32.exe 0x00421FB0: grey ((r + g + b) / 4 in every channel), count 0 wraps around
void GrayPixels(std::uint16_t* pixels, std::uint32_t count) {
    std::uint8_t* p = bytes(pixels);
    do {
        const std::uint32_t d = u16(p);
        const std::uint32_t r = (d & static_cast<std::uint32_t>(G.redMaskCopy)) >> G.shiftRed;
        const std::uint32_t g = (d & static_cast<std::uint32_t>(G.greenMaskCopy)) >> G.shiftGreen;
        const std::uint32_t b = (d & static_cast<std::uint32_t>(G.blueMaskCopy)) >> G.shiftBlue;
        const std::uint32_t avg = (r + g + b) >> 2;
        put16(p, (avg << G.shiftRed) + (avg << G.shiftGreen) + (avg << G.shiftBlue));
        p += 2;
    } while (--count != 0);
}

}  // namespace swd3
