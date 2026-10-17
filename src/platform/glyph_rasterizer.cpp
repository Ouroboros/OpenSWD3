#include "platform/glyph_rasterizer.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <climits>
#include <cstring>

#include "platform/file_system.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr std::string_view kMagic = "SWD3FONT";  // the header: magic, version, strike count
constexpr std::size_t kVersionAt = 8;
constexpr u32 kVersion = 1;
constexpr std::size_t kStrikeCountAt = 12;
constexpr std::size_t kHeaderBytes = 16;
constexpr std::size_t kStrikeHeaderBytes = 8;  // a strike: size, glyph count, then (code, cell) pairs
constexpr u8 kLeftmostBit = 0x80;              // cell rows are packed from the left

u32 ReadU32(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;

    if (offset + sizeof(value) <= data.size())
    {
        std::memcpy(&value, data.data() + offset, sizeof(value));
    }

    return value;
}

}  // namespace

bool GlyphLibrary::Load(const std::string& path)
{
    std::vector<u8> data;

    if (!ReadSharedFile(path, data))
    {
        return false;
    }

    if (data.size() < kHeaderBytes || std::memcmp(data.data(), kMagic.data(), kMagic.size()) != 0 ||
        ReadU32(data, kVersionAt) != kVersion)
    {
        return false;
    }

    const u32 count = ReadU32(data, kStrikeCountAt);
    std::size_t at = kHeaderBytes;

    for (u32 strikeIndex = 0; strikeIndex < count; ++strikeIndex)
    {
        Strike strike;
        strike.size = static_cast<int>(ReadU32(data, at));
        strike.stride = (strike.size + CHAR_BIT - 1) / CHAR_BIT;
        const u32 glyphs = ReadU32(data, at + sizeof(u32));
        at += kStrikeHeaderBytes;
        const std::size_t cellBytes = static_cast<std::size_t>(strike.size) * strike.stride;

        if (at + glyphs * (sizeof(u16) + cellBytes) > data.size())
        {
            return false;
        }

        strike.codes.resize(glyphs);
        strike.cells.resize(glyphs * cellBytes);

        for (u32 glyph = 0; glyph < glyphs; ++glyph)
        {
            strike.codes[glyph] = static_cast<u16>(data[at] | (data[at + 1] << CHAR_BIT));
            std::memcpy(strike.cells.data() + glyph * cellBytes, data.data() + at + sizeof(u16), cellBytes);
            at += sizeof(u16) + cellBytes;
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

LibraryGlyphRasterizer::LibraryGlyphRasterizer(const GlyphLibrary& library)
    : library(library)
{
}

// One character per call (the game rasterizes glyph by glyph); the cell is drawn at (0, 0) in `color`.
void LibraryGlyphRasterizer::RenderText(Surface& surface, std::string_view, int size, std::string_view text, u32 color)
{
    if (text.empty())
    {
        return;
    }

    const u16 code =
        static_cast<u16>(static_cast<u8>(text[0]) | (text.size() > 1 ? static_cast<u8>(text[1]) << CHAR_BIT : 0));
    const u8* cell = this->library.Find(size, code);

    if (cell == nullptr)
    {
        return;
    }

    const Pixel pixel = DevicePixel(color);
    const int stride = (size + CHAR_BIT - 1) / CHAR_BIT;

    for (int y = 0; y < size && y < surface.Height(); ++y)
    {
        for (int x = 0; x < size && x < surface.Width(); ++x)
        {
            if ((cell[y * stride + x / CHAR_BIT] & (kLeftmostBit >> (x % CHAR_BIT))) != 0)
            {
                surface.Row(y)[x] = pixel;
            }
        }
    }
}

std::string GlyphLibraryPath()
{
    std::string path;

    if (const auto* base = SDL_GetBasePath(); base != nullptr)
    {
        path = base;
    }

    return path + "swd3font.bin";
}

}  // namespace swd3
