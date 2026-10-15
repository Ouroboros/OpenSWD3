#pragma once

#include "draw/blit.h"

// Tint options G.drawOption71C / G.drawOption30C / G.drawOption304: signed steps added to the red / green /
// blue channel, saturating to the channel mask (positive step) or to 0 (negative step).
namespace swd3::blit {

struct Tint {
    std::uint32_t redOffset;
    std::uint32_t greenOffset;
    std::uint32_t blueOffset;
    std::uint32_t redSaturate;
    std::uint32_t greenSaturate;
    std::uint32_t blueSaturate;
};

inline bool tinted() { return G.drawOption71C != 0 || G.drawOption30C != 0 || G.drawOption304 != 0; }

inline Tint makeTint() {
    Tint t{};
    t.redOffset = static_cast<std::uint32_t>(G.redHalves[4] * G.drawOption71C);
    t.greenOffset = static_cast<std::uint32_t>(G.greenHalves[4] * G.drawOption30C);
    t.blueOffset = static_cast<std::uint32_t>(G.blueHalves[4] * G.drawOption304);
    t.redSaturate = G.drawOption71C >= 0 ? static_cast<std::uint32_t>(G.redMaskCopy) : 0;
    t.greenSaturate = G.drawOption30C >= 0 ? static_cast<std::uint32_t>(G.greenMaskCopy) : 0;
    t.blueSaturate = G.drawOption304 >= 0 ? static_cast<std::uint32_t>(G.blueMaskCopy) : 0;
    return t;
}

// One tint step applied to a pixel (no source).
inline std::uint32_t tintStep(const Tint& t, std::uint32_t d) {
    std::uint32_t r = (d & static_cast<std::uint32_t>(G.redMaskCopy)) + t.redOffset;
    if (static_cast<std::uint32_t>(G.redMaskInverse) & r) {
        r = t.redSaturate;
    }
    std::uint32_t g = (d & static_cast<std::uint32_t>(G.greenMaskCopy)) + t.greenOffset;
    if (static_cast<std::uint32_t>(G.greenMaskInverse) & g) {
        g = t.greenSaturate;
    }
    std::uint32_t b = (d & static_cast<std::uint32_t>(G.blueMaskCopy)) + t.blueOffset;
    if (static_cast<std::uint32_t>(G.blueMaskInverse) & b) {
        b = t.blueSaturate;
    }
    return r | g | b;
}

}  // namespace swd3::blit
