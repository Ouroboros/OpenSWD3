#include "talk/text_colors.h"

#include <span>

#include "draw/painter.h"

namespace swd3
{

namespace
{

// A COLORREF to RGB555, then to the display format, in both halves.
u32 ConvertColor(const Painter& painter, u32 color)
{
    u32 packed = ((color & 0xF8) << 7) | (((color & 0xF800) | ((color >> 0xD) & 0x7C0)) >> 6);
    auto low = static_cast<u16>(packed);
    painter.ConvertPixels(std::span<u16>(&low, 1));
    packed = low;
    return packed | packed << 16;
}

}  // namespace

// Part of the draw kernel start (0x00424B90): the colour loop
void TextColors::Convert(const Painter& painter)
{
    for (int index = 0; index < TextColors::Colors; ++index)
    {
        const auto at = static_cast<std::size_t>(index);
        this->primary[at] = ConvertColor(painter, this->primary[at]);
        this->secondary[at] = ConvertColor(painter, this->secondary[at]);
    }
}

}  // namespace swd3
