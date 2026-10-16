#pragma once

// RLE codec scenarios shared by the legacy golden generator and the RleCodec test (see draw_scenarios.h).

#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

struct PaletteFrame
{
    std::vector<std::uint8_t> rle;
    std::vector<std::uint16_t> palette;
};

// 8-bit RLE frames of the given tsw entries with the palette of their entry.
inline std::vector<PaletteFrame>

LoadPaletteFrames(const std::string& path, const std::vector<int>& entries, int perEntry, const Decompress& decompress)
{
    std::vector<PaletteFrame> frames;
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return frames;
    }

    for (int entry : entries)
    {
        const std::uint32_t base = ReadU32(file, 0x1C + 0x2C * static_cast<std::uint32_t>(entry - 1) + 0x18);

        if (base == 0 || ReadU16(file, base + 4) != 0xABCD || ReadU16(file, base + 8) != 8)
        {
            continue;
        }

        std::vector<std::uint16_t> palette(256);
        file.seekg(base + 12);
        file.read(reinterpret_cast<char*>(palette.data()), 0x200);
        const std::uint16_t count = ReadU16(file, base + 6);
        const std::uint32_t table = base + 12 + 0x200;

        for (int index = 0; index < count && index < perEntry; ++index)
        {
            const std::uint32_t offset = ReadU32(file, table + 0x24 * index);
            const std::uint32_t packed = ReadU32(file, table + 0x24 * index + 4);
            const std::uint32_t size = ReadU32(file, table + 0x24 * index + 8);
            std::vector<std::uint8_t> input(packed);
            file.seekg(base + offset);
            file.read(reinterpret_cast<char*>(input.data()), packed);
            std::vector<std::uint8_t> output(size);

            if (!decompress(input.data(), packed, output))
            {
                continue;
            }

            frames.push_back({std::move(output), palette});
        }
    }

    return frames;
}

class RleBackend
{
public:
    virtual ~RleBackend() = default;
    // The encoded bytes (including the bytes the size covers).
    virtual std::vector<std::uint8_t> Encode(const void* pixels, int width, int height, int format) = 0;
    // Decoded pixels followed by width, height and format as 32-bit values; empty when nothing is decoded.
    virtual std::vector<std::uint8_t> Decode(const std::vector<std::uint8_t>& rle) = 0;
    // The converted data followed by the result and the header fields.
    virtual std::vector<std::uint8_t> ConvertToDisplay(const std::vector<std::uint8_t>& rle) = 0;
    // The data of the converted picture followed by its width and height.
    virtual std::vector<std::uint8_t>
    ConvertTo16(const std::vector<std::uint8_t>& rle, const std::vector<std::uint16_t>& palette) = 0;
    virtual std::vector<std::uint8_t> ConvertTileTo16(const std::vector<std::uint8_t>& tile) = 0;
};

inline void Append32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    }
}

inline void RunRle(
    RleBackend& backend,
    const std::vector<Frame>& frames,
    const std::vector<PaletteFrame>& palettes,
    const std::function<void(const std::string&, std::uint64_t)>& report
)
{
    auto hash = [](const std::vector<std::uint8_t>& data)
    {
        return Hash(data.data(), data.size());
    };

    for (std::size_t f = 0; f < frames.size(); ++f)
    {
        const std::string name = "rle16/" + std::to_string(f);
        const std::vector<std::uint8_t> rle(frames[f].rle.begin(), frames[f].rle.end() - 64);
        const std::vector<std::uint8_t> decoded = backend.Decode(rle);
        report(name + "/decode", hash(decoded));
        const std::vector<std::uint8_t> encoded =
            backend.Encode(decoded.data(), frames[f].width, frames[f].height, 0x10);
        report(name + "/encode", hash(encoded));
        report(name + "/display", hash(backend.ConvertToDisplay(rle)));
        report(name + "/to16", hash(backend.ConvertTo16(rle, {})));
    }

    for (std::size_t f = 0; f < palettes.size(); ++f)
    {
        const std::string name = "rle8/" + std::to_string(f);
        const PaletteFrame& frame = palettes[f];
        const std::vector<std::uint8_t> decoded = backend.Decode(frame.rle);
        report(name + "/decode", hash(decoded));

        if (decoded.size() >= 12)
        {
            const std::size_t tail = decoded.size() - 12;
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            std::memcpy(&width, decoded.data() + tail, 4);
            std::memcpy(&height, decoded.data() + tail + 4, 4);
            report(
                name + "/encode",
                hash(backend.Encode(decoded.data(), static_cast<int>(width), static_cast<int>(height), 8))
            );
        }

        report(name + "/display", hash(backend.ConvertToDisplay(frame.rle)));
        report(name + "/to16", hash(backend.ConvertTo16(frame.rle, frame.palette)));
        std::vector<std::uint8_t> tile(0x200);
        std::memcpy(tile.data(), frame.palette.data(), 0x200);
        tile.insert(tile.end(), frame.rle.begin(), frame.rle.end());
        report(name + "/tile", hash(backend.ConvertTileTo16(tile)));
    }
}

// The 8-bit pictures of the codec scenarios.
inline std::vector<PaletteFrame>

LoadGoldenPaletteFrames(const std::function<std::string(const std::string&)>& gameFile, const Decompress& decompress)
{
    std::vector<PaletteFrame> frames = LoadPaletteFrames(gameFile("all_char.tsw"), {1, 40}, 2, decompress);
    const std::vector<PaletteFrame> system = LoadPaletteFrames(gameFile("all_sys.tsw"), {1, 5, 30}, 1, decompress);
    frames.insert(frames.end(), system.begin(), system.end());
    return frames;
}

}  // namespace golden
