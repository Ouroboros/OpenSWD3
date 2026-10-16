#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

class Painter;

// The two 16-colour tables of the story text (COLORREF values in the executable), converted at startup to the display
// format with the colour in both halves.
struct TextColors
{
    static constexpr int Colors = 16;
    // swd32.exe 0x0049E0C8
    static constexpr std::array<u32, Colors> Primary = {
        0xFFFFFF,
        0x000000,
        0x0C31EC,
        0x0080FF,
        0x2C577B,
        0xFFE6E6,
        0xACCFE9,
        0x002CEC,
        0xFF0000,
        0x800000,
        0x606060,
        0x2C577B,
        0xE9C8C0,
        0xACCFE9,
        0x0D31EC,
        0x002CEC,
    };

    // swd32.exe 0x0049E108
    static constexpr std::array<u32, Colors> Secondary = {
        0x606060,
        0x808080,
        0x7DA9AE,
        0x003080,
        0x7DA9AE,
        0x2C4F24,
        0x334D64,
        0x2C4F24,
        0x800080,
        0xFF0000,
        0x606060,
        0x7DA9AE,
        0x2C4F24,
        0x334D64,
        0x7DA9AE,
        0x2C4F24,
    };

    void Convert(const Painter& painter);

    std::array<u32, Colors> primary = Primary;
    std::array<u32, Colors> secondary = Secondary;
};

}  // namespace swd3
