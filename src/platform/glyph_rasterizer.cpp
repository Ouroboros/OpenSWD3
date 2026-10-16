#include "platform/glyph_rasterizer.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr char kMagic[] = "SWD3FONT";

u32 ReadU32(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;

    if (offset + 4 <= data.size())
    {
        std::memcpy(&value, data.data() + offset, 4);
    }

    return value;
}

// GDI's colour of a COLORREF on the RGB565 glyph surface.
Pixel DevicePixel(u32 color)
{
    const u32 red = color & 0xFF;
    const u32 green = (color >> 8) & 0xFF;
    const u32 blue = (color >> 16) & 0xFF;
    return static_cast<Pixel>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

}  // namespace

bool GlyphLibrary::Load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return false;
    }

    const std::vector<u8> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    if (data.size() < 16 || std::memcmp(data.data(), kMagic, 8) != 0 || ReadU32(data, 8) != 1)
    {
        return false;
    }

    const u32 count = ReadU32(data, 12);
    std::size_t at = 16;

    for (u32 s = 0; s < count; ++s)
    {
        Strike strike;
        strike.size = static_cast<int>(ReadU32(data, at));
        strike.stride = (strike.size + 7) / 8;
        const u32 glyphs = ReadU32(data, at + 4);
        at += 8;
        const std::size_t cellBytes = static_cast<std::size_t>(strike.size) * strike.stride;

        if (at + glyphs * (2 + cellBytes) > data.size())
        {
            return false;
        }

        strike.codes.resize(glyphs);
        strike.cells.resize(glyphs * cellBytes);

        for (u32 g = 0; g < glyphs; ++g)
        {
            strike.codes[g] = static_cast<u16>(data[at] | (data[at + 1] << 8));
            std::memcpy(strike.cells.data() + g * cellBytes, data.data() + at + 2, cellBytes);
            at += 2 + cellBytes;
        }

        this->strikes[strike.size] = std::move(strike);
    }

    return true;
}

bool GlyphLibrary::Loaded() const
{
    return !this->strikes.empty();
}

const u8* GlyphLibrary::Find(int size, u16 code) const
{
    const auto strike = this->strikes.find(size);

    if (strike == this->strikes.end())
    {
        return nullptr;
    }

    const std::vector<u16>& codes = strike->second.codes;
    const auto found = std::lower_bound(codes.begin(), codes.end(), code);

    if (found == codes.end() || *found != code)
    {
        return nullptr;
    }

    const std::size_t cellBytes = static_cast<std::size_t>(strike->second.size) * strike->second.stride;
    return strike->second.cells.data() + static_cast<std::size_t>(found - codes.begin()) * cellBytes;
}

LibraryGlyphRasterizer::LibraryGlyphRasterizer(const GlyphLibrary& library) : library(library)
{
}

// One character per call (the game rasterizes glyph by glyph); the cell is drawn at (0, 0) in `color`.
void LibraryGlyphRasterizer::RenderText(Surface& surface, std::string_view, int size, std::string_view text, u32 color)
{
    if (text.empty())
    {
        return;
    }

    const u16 code = static_cast<u16>(static_cast<u8>(text[0]) | (text.size() > 1 ? static_cast<u8>(text[1]) << 8 : 0));
    const u8* cell = this->library.Find(size, code);

    if (cell == nullptr)
    {
        return;
    }

    const Pixel pixel = DevicePixel(color);
    const int stride = (size + 7) / 8;

    for (int y = 0; y < size && y < surface.Height(); ++y)
    {
        for (int x = 0; x < size && x < surface.Width(); ++x)
        {
            if ((cell[y * stride + x / 8] & (0x80 >> (x % 8))) != 0)
            {
                surface.Row(y)[x] = pixel;
            }
        }
    }
}

std::string GlyphLibraryPath()
{
    const char* base = SDL_GetBasePath();
    return std::string(base != nullptr ? base : "") + "swd3font.bin";
}

}  // namespace swd3
