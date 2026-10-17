#include "talk/text_colors.h"

#include <span>

#include "core/words.h"
#include "draw/painter.h"

namespace swd3
{

namespace
{

// A COLORREF (0x00BBGGRR) folded to RGB555: the top five bits of red to bits 10..14, of green to 5..9 and of blue to
// 0..4 (blue is first moved beside green).
constexpr u32 kRedBits = 0xF8;
constexpr int kRedShift = 7;
constexpr u32 kGreenBits = 0xF800;
constexpr int kBlueFold = 0xD;
constexpr u32 kBlueBits = 0x7C0;
constexpr int kGreenBlueShift = 6;

// A COLORREF to RGB555, then to the display format, in both halves.
u32 ConvertColor(const Painter& painter, u32 color)
{
    u32 packed = ((color & kRedBits) << kRedShift) |
                 (((color & kGreenBits) | ((color >> kBlueFold) & kBlueBits)) >> kGreenBlueShift);
    auto low = static_cast<u16>(packed);
    painter.ConvertPixels(std::span<u16>(&low, 1));
    packed = low;
    return MakeWords(static_cast<u16>(packed), static_cast<u16>(packed));
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
