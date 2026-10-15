#pragma once

#include <cstdint>
#include <cstring>

#include "data/globals.h"

// Blit routines of the drawing kernel (0x00417950-0x00423400). They take their state from the globals set up
// by DrawPicture (G.draw*, G.clip*, G.wave*) and are dispatched through G.drawRleTable / G.drawRawTable.
namespace swd3 {

int BlitRaw_417950(int x, int y, int width, int height, int arg);
int BlitRaw_417EC0(int x, int y, int width, int height, int arg);
int BlitRle_418350(int x, int y, int width, int height, int arg);
int BlitRle_4185C0(int x, int y, int width, int height, int arg);
int BlitRle_418840(int x, int y, int width, int height, int arg);
int BlitRle_418EB0(int x, int y, int width, int height, int arg);
int BlitRle_419570(int x, int y, int width, int height, int arg);
int BlitRle_41A3B0(int x, int y, int width, int height, int arg);
int BlitRle_41B280(int x, int y, int width, int height, int arg);
int BlitRle_41B620(int x, int y, int width, int height, int arg);
int BlitRle_41B9F0(int x, int y, int width, int height, int arg);
int BlitRle_41CCF0(int x, int y, int width, int height, int arg);
int BlitRle_41D010(int x, int y, int width, int height, int arg);
int BlitRle_41D340(int x, int y, int width, int height, int arg);
int BlitRle_41E5C0(int x, int y, int width, int height, int arg);
int BlitRle_41F8D0(int x, int y, int width, int height, int arg);
int BlitRle_41FEA0(int x, int y, int width, int height, int arg);
int BlitRle_4208D0(int x, int y, int width, int height, int arg);
int BlitRle_420D70(int x, int y, int width, int height, int arg);
int BlitRle_421230(int x, int y, int width, int height, int arg);
int BlitRle_421540(int x, int y, int width, int height, int arg);
int BlitRle_421850(int x, int y, int width, int height, int arg);
int BlitRle_421BE0(int x, int y, int width, int height, int arg);
int BlitRle_422030(int x, int y, int width, int height, int arg);
int BlitRle_4223A0(int x, int y, int width, int height, int arg);
int BlitRle_422730(int x, int y, int width, int height, int arg);
int BlitRle_4229C0(int x, int y, int width, int height, int arg);

namespace blit {

inline std::uint16_t u16(const std::uint8_t* p) {
    std::uint16_t v;
    std::memcpy(&v, p, 2);
    return v;
}

inline std::uint32_t u32(const std::uint8_t* p) {
    std::uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

inline void put16(std::uint8_t* p, std::uint32_t v) {
    const auto w = static_cast<std::uint16_t>(v);
    std::memcpy(p, &w, 2);
}

inline void put32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, 4); }

inline std::int32_t waveAt(std::int32_t byteOffset) {
    std::int32_t v;
    std::memcpy(&v, reinterpret_cast<const std::uint8_t*>(G.waveOffsets) + byteOffset, 4);
    return v;
}

// Wave setup at the start of the line-based RLE blitters (two routines use a row stride of 0x528).
inline void waveBegin(std::int32_t stride = 0x84) {
    if (G.waveMode != 0) {
        G.waveStart = (G.waveMode - 1) * stride;
        G.wavePos = G.waveStart + G.wavePhase;
        G.waveEnd = G.waveStart + 0x84;
    }
}

// Per-line destination shift of the wave effect.
inline std::uint8_t* waveLine(std::uint8_t* dest) {
    if (G.waveMode != 0) {
        std::int32_t pos = G.wavePos + 4;
        if (static_cast<std::uint32_t>(pos) >= static_cast<std::uint32_t>(G.waveEnd)) {
            pos = G.waveStart;
        }
        G.wavePos = pos;
        dest += waveAt(pos);
        dest += waveAt(pos);
    }
    return dest;
}

inline void waveAdvancePhase() {
    if (G.waveMode != 0) {
        G.wavePhase += 4;
        if (static_cast<std::uint32_t>(G.wavePhase) >= 0x84u) {
            G.wavePhase = 0;
        }
    }
}

// Skips `count` RLE lines starting at the first line header.
inline const std::uint8_t* skipLines(const std::uint8_t* src, std::int32_t count) {
    for (std::int32_t i = 0; i != count; ++i) {
        src += u16(src) & 0x7FFF;
    }
    return src;
}

// Level L (0..15): each of the four terms 1/2, 1/4, 1/8, 1/16 comes from the source when the matching bit
// (8, 4, 2, 1) of L is set, otherwise from the destination. Level 0 leaves the destination untouched and
// level 15 copies the source. Computed on 16-bit pixels; the 32-bit pair arithmetic of the original gives the
// same low half because the halved masks clear the bits shifted in from the neighbouring pixel.
inline void blendPixel(std::uint8_t* dest, std::uint32_t source, int level) {
    if (level == 0) {
        return;
    }
    if (level == 15) {
        put16(dest, source);
        return;
    }
    const std::uint32_t d = u16(dest);
    const std::uint32_t t1 = ((level & 8) ? source : d) >> 1;
    const std::uint32_t t2 = ((level & 4) ? source : d) >> 2;
    const std::uint32_t t3 = ((level & 2) ? source : d) >> 3;
    const std::uint32_t t4 = ((level & 1) ? source : d) >> 4;
    const std::uint32_t v = (t1 & static_cast<std::uint32_t>(G.halfMaskPairs[0])) +
                            (t2 & static_cast<std::uint32_t>(G.halfMaskPairs[1])) +
                            (t3 & static_cast<std::uint32_t>(G.halfMaskPairs[2])) +
                            (t4 & static_cast<std::uint32_t>(G.halfMaskPairs[3]));
    put16(dest, v);
}

}  // namespace blit
}  // namespace swd3
