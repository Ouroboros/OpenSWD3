#include "draw/blit_tint.h"
#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

namespace {

std::uint32_t addPixel(std::uint32_t s, std::uint32_t d) {
    const auto rm = static_cast<std::uint32_t>(G.redMaskCopy);
    const auto gm = static_cast<std::uint32_t>(G.greenMaskCopy);
    const auto bm = static_cast<std::uint32_t>(G.blueMaskCopy);
    return addChannel(s, d, rm, static_cast<std::uint32_t>(G.redMaskInverse)) |
           addChannel(s, d, gm, static_cast<std::uint32_t>(G.greenMaskInverse)) |
           addChannel(s, d, bm, static_cast<std::uint32_t>(G.blueMaskInverse));
}

std::uint32_t tintChannel(std::uint32_t s, std::uint32_t d, std::uint32_t mask, std::uint32_t inverse,
                          std::uint32_t offset, std::uint32_t saturate) {
    std::uint32_t v = (s & mask) + offset;
    if (inverse & v) {
        v = saturate;
    }
    v += d & mask;
    if (inverse & v) {
        v = mask;
    }
    return v;
}

std::uint32_t tintPixel(const Tint& t, std::uint32_t s, std::uint32_t d) {
    return tintChannel(s, d, static_cast<std::uint32_t>(G.redMaskCopy), static_cast<std::uint32_t>(G.redMaskInverse),
                       t.redOffset, t.redSaturate) |
           tintChannel(s, d, static_cast<std::uint32_t>(G.greenMaskCopy),
                       static_cast<std::uint32_t>(G.greenMaskInverse), t.greenOffset, t.greenSaturate) |
           tintChannel(s, d, static_cast<std::uint32_t>(G.blueMaskCopy), static_cast<std::uint32_t>(G.blueMaskInverse),
                       t.blueOffset, t.blueSaturate);
}

}  // namespace

// swd32.exe 0x00418840: additive blend, optional tint (G.drawOption71C/30C/304) and line dropping
// (G.drawOption300)
int BlitRle_418840(int, int, int, int, int) {
    const WalkOptions options{false, true, true};
    if (!tinted()) {
        walkForward(options, [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                put16(dest, addPixel(u16(src), u16(dest)));
                dest += 2;
                src += 2;
            } while (--n != 0);
        });
        return 0;
    }
    const Tint t = makeTint();
    walkForward(options, [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        do {
            put16(dest, tintPixel(t, u16(src), u16(dest)));
            dest += 2;
            src += 2;
        } while (--n != 0);
    });
    return 0;
}

// swd32.exe 0x00418EB0: mirrored 0x00418840. The original's untinted path jumps into the tinted loop for a
// literal run clipped on both sides, using uninitialised tint offsets; with zero offsets that loop computes
// the same additive result, which is what is done here.
int BlitRle_418EB0(int, int, int width, int, int) {
    const WalkOptions options{false, true, true};
    if (!tinted()) {
        walkMirror(options, width, [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                put16(dest - 2, addPixel(u16(src), u16(dest - 2)));
                dest -= 2;
                src += 2;
            } while (--n != 0);
        });
        return 0;
    }
    const Tint t = makeTint();
    walkMirror(options, width, [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        do {
            put16(dest - 2, tintPixel(t, u16(src), u16(dest - 2)));
            dest -= 2;
            src += 2;
        } while (--n != 0);
    });
    return 0;
}

namespace {

// Forward subtractive channel: the tint offset is added to the source channel.
std::uint32_t subChannelSource(std::uint32_t s, std::uint32_t d, std::uint32_t mask, std::uint32_t inverse,
                               std::uint32_t offset, std::uint32_t saturate) {
    std::uint32_t sub = (s & mask) + offset;
    if (inverse & sub) {
        sub = saturate;
    }
    std::uint32_t v = (d & mask) - sub;
    if (v & 0x80000000u) {
        v = 0;
    }
    return v & mask;
}

// Mirrored subtractive channel: the tint offset is added to the destination channel instead.
std::uint32_t subChannelDest(std::uint32_t s, std::uint32_t d, std::uint32_t mask, std::uint32_t inverse,
                             std::uint32_t offset, std::uint32_t saturate) {
    std::uint32_t v = (d & mask) + offset;
    if (inverse & v) {
        v = saturate;
    }
    v -= s & mask;
    if (v & 0x80000000u) {
        v = 0;
    }
    return v & mask;
}

template <typename Channel>
std::uint32_t subPixel(Channel channel, const Tint& t, std::uint32_t s, std::uint32_t d) {
    return channel(s, d, static_cast<std::uint32_t>(G.redMaskCopy), static_cast<std::uint32_t>(G.redMaskInverse),
                   t.redOffset, t.redSaturate) |
           channel(s, d, static_cast<std::uint32_t>(G.greenMaskCopy), static_cast<std::uint32_t>(G.greenMaskInverse),
                   t.greenOffset, t.greenSaturate) |
           channel(s, d, static_cast<std::uint32_t>(G.blueMaskCopy), static_cast<std::uint32_t>(G.blueMaskInverse),
                   t.blueOffset, t.blueSaturate);
}

}  // namespace

// swd32.exe 0x00422030: subtractive blend (dest - (src + tint)), line dropping by G.drawOption300
int BlitRle_422030(int, int, int, int, int) {
    const Tint t = makeTint();
    walkForward(WalkOptions{false, true, true}, [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        do {
            put16(dest, subPixel(subChannelSource, t, u16(src), u16(dest)));
            dest += 2;
            src += 2;
        } while (--n != 0);
    });
    return 0;
}

// swd32.exe 0x004223A0: mirrored 0x00422030, computing (dest + tint) - src
int BlitRle_4223A0(int, int, int width, int, int) {
    const Tint t = makeTint();
    walkMirror(WalkOptions{false, true, true}, width,
               [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
                   do {
                       put16(dest - 2, subPixel(subChannelDest, t, u16(src), u16(dest - 2)));
                       dest -= 2;
                       src += 2;
                   } while (--n != 0);
               });
    return 0;
}

// swd32.exe 0x004208D0: vertically scaled additive blend with tint; both run kinds are transparent. The
// original never initialises its scale remainder (stack garbage below 0x400 on the first line); it starts
// at 0 here.
int BlitRle_4208D0(int, int, int, int, int) {
    const Tint t = makeTint();
    walk3Forward(
        Walk3Options{0x84, true, false, false, true},
        [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                put16(dest, tintPixel(t, u16(src), u16(dest)));
                dest += 2;
                src += 2;
            } while (--n != 0);
        },
        [](std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; },
        [](std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; });
    return 0;
}

// swd32.exe 0x00420D70: mirrored 0x004208D0 (same uninitialised remainder)
int BlitRle_420D70(int, int, int width, int, int) {
    const Tint t = makeTint();
    walk3Mirror(
        Walk3Options{0x84, true, false, false, true}, width,
        [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                put16(dest - 2, tintPixel(t, u16(src), u16(dest - 2)));
                dest -= 2;
                src += 2;
            } while (--n != 0);
        },
        [](std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; },
        [](std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; });
    return 0;
}

}  // namespace swd3
