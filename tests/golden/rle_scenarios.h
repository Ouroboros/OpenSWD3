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

constexpr std::size_t kDecodedTrailer = 3 * sizeof(u32);  // the width, height and format after the decoded pixels
constexpr std::array<int, 2> kCharPaletteEntries = {1, 40};
constexpr std::array<int, 3> kSystemPaletteEntries = {1, 5, 30};

struct PaletteFrame
{
    std::vector<u8> rle;
    std::vector<u16> palette;
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
        const u32 base =
            ReadU32(file, kTswDirectoryStart + kTswDirectoryBytes * static_cast<u32>(entry - 1) + kTswOffsetAt);

        if (base == 0 || ReadU16(file, base + kTswMagicAt) != kTswMagic ||
            ReadU16(file, base + kTswFormatAt) != kTswPaletteFormat)
        {
            continue;
        }

        std::vector<u16> palette(kPaletteColors);
        file.seekg(base + kTswHeaderBytes);
        file.read(reinterpret_cast<char*>(palette.data()), kTswPaletteBytes);
        const u16 count = ReadU16(file, base + kTswCountAt);
        const u32 table = base + kTswHeaderBytes + kTswPaletteBytes;

        for (int index = 0; index < count && index < perEntry; ++index)
        {
            const u32 offset = ReadU32(file, table + kTswFrameBytes * index);
            const u32 packed = ReadU32(file, table + kTswFrameBytes * index + kTswPackedAt);
            const u32 size = ReadU32(file, table + kTswFrameBytes * index + kTswSizeAt);
            std::vector<u8> input(packed);
            file.seekg(base + offset);
            file.read(reinterpret_cast<char*>(input.data()), packed);
            std::vector<u8> output(size);

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
    virtual std::vector<u8> Encode(const void* pixels, int width, int height, int format) = 0;
    // Decoded pixels followed by width, height and format as 32-bit values; empty when nothing is decoded.
    virtual std::vector<u8> Decode(const std::vector<u8>& rle) = 0;
    // The converted data followed by the result and the header fields.
    virtual std::vector<u8> ConvertToDisplay(const std::vector<u8>& rle) = 0;
    // The data of the converted picture followed by its width and height.
    virtual std::vector<u8> ConvertTo16(const std::vector<u8>& rle, const std::vector<u16>& palette) = 0;
    virtual std::vector<u8> ConvertTileTo16(const std::vector<u8>& tile) = 0;
};

inline void RunRle(
    RleBackend& backend,
    const std::vector<Frame>& frames,
    const std::vector<PaletteFrame>& palettes,
    const std::function<void(const std::string&, u64)>& report
)
{
    auto hash = [](const std::vector<u8>& data)
    {
        return Hash(data.data(), data.size());
    };

    for (std::size_t f = 0; f < frames.size(); ++f)
    {
        const std::string name = "rle16/" + std::to_string(f);
        const std::vector<u8> rle(frames[f].rle.begin(), frames[f].rle.end() - kRleSlack);
        const std::vector<u8> decoded = backend.Decode(rle);
        report(name + "/decode", hash(decoded));
        const std::vector<u8> encoded =
            backend.Encode(decoded.data(), frames[f].width, frames[f].height, kRleTrueColor);
        report(name + "/encode", hash(encoded));
        report(name + "/display", hash(backend.ConvertToDisplay(rle)));
        report(name + "/to16", hash(backend.ConvertTo16(rle, {})));
    }

    for (std::size_t f = 0; f < palettes.size(); ++f)
    {
        const std::string name = "rle8/" + std::to_string(f);
        const PaletteFrame& frame = palettes[f];
        const std::vector<u8> decoded = backend.Decode(frame.rle);
        report(name + "/decode", hash(decoded));

        if (decoded.size() >= kDecodedTrailer)
        {
            const std::size_t tail = decoded.size() - kDecodedTrailer;
            u32 width = 0;
            u32 height = 0;
            std::memcpy(&width, decoded.data() + tail, sizeof(width));
            std::memcpy(&height, decoded.data() + tail + sizeof(width), sizeof(height));
            report(
                name + "/encode",
                hash(backend.Encode(decoded.data(), static_cast<int>(width), static_cast<int>(height), kRleIndexed))
            );
        }

        report(name + "/display", hash(backend.ConvertToDisplay(frame.rle)));
        report(name + "/to16", hash(backend.ConvertTo16(frame.rle, frame.palette)));
        std::vector<u8> tile(kTswPaletteBytes);
        std::memcpy(tile.data(), frame.palette.data(), kTswPaletteBytes);
        tile.insert(tile.end(), frame.rle.begin(), frame.rle.end());
        report(name + "/tile", hash(backend.ConvertTileTo16(tile)));
    }
}

// The 8-bit pictures of the codec scenarios.
inline std::vector<PaletteFrame>

LoadGoldenPaletteFrames(const std::function<std::string(const std::string&)>& gameFile, const Decompress& decompress)
{
    std::vector<PaletteFrame> frames = LoadPaletteFrames(
        gameFile("all_char.tsw"),
        {kCharPaletteEntries.begin(), kCharPaletteEntries.end()},
        kCharFrames,
        decompress
    );
    const std::vector<PaletteFrame> system = LoadPaletteFrames(
        gameFile("all_sys.tsw"),
        {kSystemPaletteEntries.begin(), kSystemPaletteEntries.end()},
        1,
        decompress
    );
    frames.insert(frames.end(), system.begin(), system.end());
    return frames;
}

}  // namespace golden
