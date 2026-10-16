#pragma once

// Draw scenarios shared by the golden generator of the legacy implementation (reverse/analysis/golden/) and the
// Painter test: the same sequence of draws runs through both backends, and the hash of the visible screen after
// every scenario is compared. Header-only and independent of either implementation.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace golden
{

// The project integer types, repeated here because these headers are also built in the legacy tree.
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;

constexpr int kScreenWidth = 640;
constexpr int kScreenHeight = 480;

struct Frame
{
    std::vector<u8> rle;
    int width = 0;
    int height = 0;
};

// Decompresses `size` bytes into `output` (sized to the expected length); false on failure.
using Decompress = std::function<bool(const u8* input, std::size_t size, std::vector<u8>& output)>;

inline u32 ReadU32(std::ifstream& file, u32 offset)
{
    u32 value = 0;
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&value), 4);
    return value;
}

inline u16 ReadU16(std::ifstream& file, u32 offset)
{
    u16 value = 0;
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&value), 2);
    return value;
}

// 16-bit RLE frames of the given tsw entries (1-based), at most `perEntry` frames each.
inline std::vector<Frame>

LoadFrames(const std::string& path, const std::vector<int>& entries, int perEntry, const Decompress& decompress)
{
    std::vector<Frame> frames;
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return frames;
    }

    for (int entry : entries)
    {
        const u32 base = ReadU32(file, 0x1C + 0x2C * static_cast<u32>(entry - 1) + 0x18);

        if (base == 0 || ReadU16(file, base + 4) != 0xABCD)
        {
            continue;
        }

        const u16 count = ReadU16(file, base + 6);
        const u16 format = ReadU16(file, base + 8);
        const u32 table = base + 12 + (format == 8 ? 0x200 : 0);

        for (int index = 0; index < count && index < perEntry; ++index)
        {
            const u32 offset = ReadU32(file, table + 0x24 * index);
            const u32 packed = ReadU32(file, table + 0x24 * index + 4);
            const u32 size = ReadU32(file, table + 0x24 * index + 8);
            std::vector<u8> input(packed);
            file.seekg(base + offset);
            file.read(reinterpret_cast<char*>(input.data()), packed);
            Frame frame;
            frame.rle.assign(size + 64, 0);
            std::vector<u8> output(size);

            if (!decompress(input.data(), packed, output))
            {
                continue;
            }

            std::memcpy(frame.rle.data(), output.data(), size);
            u16 header[4] = {};
            std::memcpy(header, output.data(), 8);

            if (header[0] != 0xFFFF || (header[3] & 0x10) == 0)
            {
                continue;
            }

            frame.width = header[1];
            frame.height = header[2];
            frames.push_back(std::move(frame));
        }
    }

    return frames;
}

// The pictures of the golden scenarios: 16-bit entries of the character, system and item archives.
inline std::vector<Frame>

LoadGoldenFrames(const std::function<std::string(const std::string&)>& gameFile, const Decompress& decompress)
{
    std::vector<Frame> frames = LoadFrames(gameFile("all_char.tsw"), {7, 23, 33, 385}, 2, decompress);
    const std::vector<Frame> system = LoadFrames(gameFile("all_sys.tsw"), {14, 36, 99}, 1, decompress);
    const std::vector<Frame> items = LoadFrames(gameFile("all_item.tsw"), {1, 2}, 1, decompress);
    frames.insert(frames.end(), system.begin(), system.end());
    frames.insert(frames.end(), items.begin(), items.end());
    return frames;
}

struct DrawCall
{
    const void* source = nullptr;
    const u16* palette = nullptr;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    u32 mode = 0;
    int alpha = 0;
    int tintRed = 0;
    int tintGreen = 0;
    int tintBlue = 0;
    bool dropLines = false;
    int scaleHeight = 0;
    int shear = 0;
    int wave = 0;
    const u8* alphaMap = nullptr;
    u32 colorPair = 0;
};

enum class PixelOperation
{
    Tint,
    ShiftRed,
    ShiftGreenPairs,
    ShiftBluePairs,
    Mix,
    Gray,
    BlendPairs,
};

class Backend
{
public:
    virtual ~Backend() = default;
    // A fresh screen filled with the background pattern of `seed`, the clip rectangle on the whole screen.
    virtual void Reset(u32 seed) = 0;
    virtual void SetClip(int left, int top, int right, int bottom) = 0;
    virtual int Draw(const DrawCall& call) = 0;
    virtual void Outline(const DrawCall& call) = 0;
    virtual void Zoom(
        const void* source,
        int x,
        int y,
        int width,
        int height,
        bool mirrored,
        int anchorX,
        int anchorY,
        int zoomX,
        int zoomY
    ) = 0;
    // kind 0: 16-bit, 1: 16-bit keyed, 2: palette, 3: palette keyed
    virtual void Tile(int kind, const void* source, const u16* palette, int x, int y) = 0;
    virtual void
    Pixels(PixelOperation operation, u16* pixels, std::size_t count, const u16* other, int a, int b, int c) = 0;
    virtual u32 MakeColor(u32 red, u32 green, u32 blue) = 0;
    virtual const u16* Screen() const = 0;
};

inline void Append32(std::vector<u8>& out, u32 value)
{
    for (int i = 0; i < 4; ++i)
    {
        out.push_back(static_cast<u8>(value >> (8 * i)));
    }
}

inline u64 Hash(const void* data, std::size_t bytes)
{
    u64 hash = 1469598103934665603ull;
    const auto* p = static_cast<const u8*>(data);

    for (std::size_t i = 0; i < bytes; ++i)
    {
        hash ^= p[i];
        hash *= 1099511628211ull;
    }

    return hash;
}

inline u32 Mix32(u32 value)
{
    value ^= value >> 16;
    value *= 0x7FEB352Du;
    value ^= value >> 15;
    value *= 0x846CA68Bu;
    value ^= value >> 16;
    return value;
}

// The background pattern of a scenario (also used by the backends' Reset).
inline u16 Background(u32 seed, int x, int y)
{
    return static_cast<u16>(Mix32(seed * 0x9E3779B9u + static_cast<u32>(y * kScreenWidth + x)));
}

// A raw 16-bit picture from an RLE frame: literal pixels, fill runs 0x04CB and skip runs 0x631F (the converted RLE
// fill and skip colours of an RGB565 display).
inline std::vector<u16> RawPixels(const Frame& frame)
{
    std::vector<u16> pixels(static_cast<std::size_t>(frame.width) * frame.height, 0x631F);
    const u8* line = frame.rle.data() + 8;

    for (int y = 0; y < frame.height; ++y)
    {
        u16 header = 0;
        std::memcpy(&header, line, 2);

        if (header == 0)
        {
            break;
        }

        const u8* code = line + 2;
        int x = 0;

        while (x < frame.width)
        {
            u16 value = 0;
            std::memcpy(&value, code, 2);
            const int count = value & 0x3FFF;
            code += 2;

            if (count == 0)
            {
                break;
            }

            for (int i = 0; i < count && x + i < frame.width; ++i)
            {
                u16 pixel = (value & 0x8000) != 0 ? 0x631F : 0x04CB;

                if ((value & 0xC000) == 0)
                {
                    std::memcpy(&pixel, code + 2 * i, 2);
                }

                pixels[static_cast<std::size_t>(y) * frame.width + x + i] = pixel;
            }

            if ((value & 0xC000) == 0)
            {
                code += 2 * count;
            }

            x += count;
        }

        line += header & 0x7FFF;
    }

    return pixels;
}

inline void

Run(Backend& backend, const std::vector<Frame>& frames, const std::function<void(const std::string&, u64)>& reportHash)
{
    // GOLDEN_DUMP=<scenario name prefix>: the screen after that scenario is written to <GOLDEN_DUMP_FILE>.
    const char* dumpName = std::getenv("GOLDEN_DUMP");
    const char* dumpFile = std::getenv("GOLDEN_DUMP_FILE");

    auto report = [&](const std::string& name, u64 hash)
    {
        if (dumpName != nullptr && dumpFile != nullptr && name.rfind(dumpName, 0) == 0)
        {
            std::ofstream out(dumpFile, std::ios::binary);
            out.write(reinterpret_cast<const char*>(backend.Screen()), kScreenWidth * kScreenHeight * 2);
        }

        reportHash(name, hash);
    };

    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), static_cast<std::size_t>(kScreenWidth) * kScreenHeight * 2);
    };

    struct Variant
    {
        const char* name;
        u32 mode;
        int alpha;
        int tintRed;
        int tintGreen;
        int tintBlue;
        bool dropLines;
        int scaleHeight;
        int shear;
        int wave;
    };

    const Variant variants[] = {
        {"copy", 0x00, 0, 0, 0, 0, false, 0, 0, 0},
        {"copy-mirror", 0x01, 0, 0, 0, 0, false, 0, 0, 0},
        {"copy-flip", 0x02, 0, 0, 0, 0, false, 0, 0, 0},
        {"copy-flip-mirror", 0x03, 0, 0, 0, 0, false, 0, 0, 0},
        {"copy-wave", 0x00, 0, 0, 0, 0, false, 0, 0, 3},
        {"copy-mirror-wave", 0x01, 0, 0, 0, 0, false, 0, 0, 7},
        {"additive", 0x04, 0, 0, 0, 0, false, 0, 0, 0},
        {"additive-tint", 0x04, 0, 3, -2, 5, false, 0, 0, 0},
        {"additive-drop", 0x04, 0, 0, 0, 0, true, 0, 0, 0},
        {"additive-mirror", 0x05, 0, 0, 0, 0, false, 0, 0, 0},
        {"additive-mirror-tint", 0x05, 0, -4, 6, 2, true, 0, 0, 0},
        {"alphamap", 0x08, 0, 0, 0, 0, false, 0, 0, 0},
        {"shadow", 0x0C, 0, -8, -8, -8, false, 0, 0, 0},
        {"shadow-shear", 0x0C, 0, -6, -6, -6, false, 0, 40, 0},
        {"shadow-scaled", 0x0C, 0, -6, -6, -6, false, 30, -25, 0},
        {"shadow-scaled-up", 0x0C, 0, -6, -6, -6, false, 200, 15, 0},
        {"shadow-mirror", 0x0D, 0, -5, -5, -5, false, 40, 20, 0},
        {"shadow-flip", 0x0E, 0, -5, -5, -5, false, 40, 20, 0},
        {"tint", 0x10, 0, 4, 0, -3, false, 0, 0, 0},
        {"tint-none", 0x10, 0, 0, 0, 0, false, 0, 0, 0},
        {"tint-mirror", 0x11, 0, -7, 2, 1, false, 0, 0, 2},
        {"tint-wave", 0x10, 0, 2, 2, 2, false, 0, 0, 2},
        {"translucent-1", 0x14, 1, 0, 0, 0, false, 0, 0, 0},
        {"translucent-7", 0x14, 7, 0, 0, 0, false, 0, 0, 0},
        {"translucent-12", 0x15, 12, 0, 0, 0, false, 0, 0, 0},
        {"translucent-flip", 0x16, 9, 0, 0, 0, false, 0, 0, 0},
        {"translucent-0", 0x14, 0, 0, 0, 0, false, 0, 0, 0},
        {"translucent-20", 0x15, 20, 0, 0, 0, false, 0, 0, 0},
        {"edge", 0x18, 0, 0, 0, 0, false, 0, 0, 0},
        {"edge-mirror", 0x19, 0, 0, 0, 0, false, 0, 0, 0},
        {"fade", 0x1C, 0, 0, 0, 0, false, 0, 0, 0},
        {"glow-scaled", 0x20, 0, 2, 1, 3, false, 50, 0, 0},
        {"glow-scaled-up", 0x20, 0, 0, 0, 0, false, 300, 0, 0},
        {"glow-mirror", 0x21, 0, 1, 1, 1, false, 70, 0, 0},
        {"silhouette", 0x24, 0, 0, 0, 0, false, 0, 0, 0},
        {"silhouette-mirror", 0x25, 0, 0, 0, 0, false, 0, 0, 0},
        {"silhouette-flip", 0x26, 0, 0, 0, 0, false, 0, 0, 0},
        {"gray", 0x28, 0, 1, 0, 0, false, 0, 0, 0},
        {"gray-none", 0x28, 0, 0, 0, 0, false, 0, 0, 0},
        {"gray-mirror", 0x29, 0, 0, 1, 0, false, 0, 0, 0},
        {"subtract", 0x2C, 0, 2, 0, 1, false, 0, 0, 0},
        {"subtract-drop", 0x2C, 0, 0, 0, 0, true, 0, 0, 0},
        {"subtract-mirror", 0x2D, 0, 3, -1, 0, false, 0, 0, 0},
        {"smear", 0x30, 0, 0, 0, 0, false, 0, 0, 0},
        {"smear-mirror", 0x31, 0, 0, 0, 0, false, 0, 0, 0},
        {"smear-flip", 0x32, 0, 0, 0, 0, false, 0, 0, 0},
    };

    struct Position
    {
        int dx;
        int dy;
    };

    u32 seed = 1;

    for (std::size_t f = 0; f < frames.size(); ++f)
    {
        const Frame& frame = frames[f];
        std::vector<u8> alphaMap(static_cast<std::size_t>(frame.width) * frame.height);

        for (std::size_t i = 0; i < alphaMap.size(); ++i)
        {
            alphaMap[i] = static_cast<u8>((Mix32(static_cast<u32>(i) + 77u) % 16) * 4);
        }

        const Position positions[] = {
            {200, 150},
            {-frame.width / 3, 100},
            {kScreenWidth - frame.width * 2 / 3, 260},
            {300, -frame.height / 2},
            {120, kScreenHeight - frame.height / 2},
            {-frame.width / 4, -frame.height / 4},
        };

        for (const Variant& variant : variants)
        {
            for (int clip = 0; clip < 2; ++clip)
            {
                backend.Reset(seed++);

                if (clip == 1)
                {
                    backend.SetClip(90, 70, 530, 410);
                }

                int result = 0;

                for (const Position& position : positions)
                {
                    DrawCall call;
                    call.source = frame.rle.data();
                    call.x = position.dx;
                    call.y = position.dy;
                    call.width = frame.width;
                    call.height = frame.height;
                    call.mode = variant.mode;
                    call.alpha = variant.alpha;
                    call.tintRed = variant.tintRed;
                    call.tintGreen = variant.tintGreen;
                    call.tintBlue = variant.tintBlue;
                    call.dropLines = variant.dropLines;
                    call.scaleHeight = variant.scaleHeight;
                    call.shear = variant.shear;
                    call.wave = variant.wave;
                    call.alphaMap = alphaMap.data();
                    call.colorPair = 0x7BEF1234u;
                    result += backend.Draw(call);
                }

                report(
                    "rle/" + std::to_string(f) + "/" + variant.name + "/clip" + std::to_string(clip) + " result " +
                        std::to_string(result),
                    screenHash()
                );
            }
        }

        backend.Reset(seed++);
        DrawCall outline;
        outline.source = frame.rle.data();
        outline.x = 150;
        outline.y = 120;
        outline.width = frame.width;
        outline.height = frame.height;
        outline.colorPair = 0x001F001Fu;
        backend.Outline(outline);
        report("outline/" + std::to_string(f), screenHash());

        const std::vector<u16> raw = RawPixels(frame);
        std::array<u16, 256> palette{};

        for (std::size_t i = 0; i < palette.size(); ++i)
        {
            palette[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * 31u));
        }

        std::vector<u8> indexed(raw.size());

        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            indexed[i] = static_cast<u8>(raw[i] % 7 == 0 ? 1 : raw[i] & 0xFF);
        }

        const u32 rawModes[] = {0x00, 0x01, 0x04, 0x05, 0x08, 0x14};

        for (u32 mode : rawModes)
        {
            for (int kind = 0; kind < 2; ++kind)
            {
                if (kind == 1 && (mode == 0x01 || mode == 0x05 || mode == 0x14 || mode == 0x08))
                {
                    continue;
                }

                backend.Reset(seed++);
                backend.SetClip(40, 30, 600, 450);
                int result = 0;

                for (const Position& position : positions)
                {
                    DrawCall call;
                    call.source = kind == 0 ? static_cast<const void*>(raw.data()) : indexed.data();
                    call.palette = kind == 0 ? nullptr : palette.data();
                    call.x = position.dx;
                    call.y = position.dy;
                    call.width = frame.width;
                    call.height = frame.height;
                    call.mode = mode;
                    call.alpha = 6;
                    result += backend.Draw(call);
                }

                report(
                    "raw/" + std::to_string(f) + "/" + std::to_string(mode) + "/" + std::to_string(kind) + " result " +
                        std::to_string(result),
                    screenHash()
                );
            }
        }

        const int zooms[][4] = {
            {0x28, 0x48, 0x600, 0x400},
            {10, 20, 0x200, 0x300},
            {0, 0, 0x400, 0x400},
            {frame.width / 2, frame.height, 0x900, 0x700}
        };

        for (const auto& zoom : zooms)
        {
            for (int mirrored = 0; mirrored < 2; ++mirrored)
            {
                backend.Reset(seed++);
                backend.SetClip(60, 50, 580, 430);

                for (const Position& position : positions)
                {
                    backend.Zoom(
                        frame.rle.data(),
                        position.dx,
                        position.dy,
                        frame.width,
                        frame.height,
                        mirrored != 0,
                        zoom[0],
                        zoom[1],
                        zoom[2],
                        zoom[3]
                    );
                }

                report(
                    "zoom/" + std::to_string(f) + "/" + std::to_string(zoom[2]) + "x" + std::to_string(zoom[3]) + "/" +
                        std::to_string(mirrored),
                    screenHash()
                );
            }
        }
    }

    std::vector<u16> tile16(256);
    std::vector<u8> tile8(256);
    std::array<u16, 256> palette{};

    for (int i = 0; i < 256; ++i)
    {
        tile16[static_cast<std::size_t>(i)] = static_cast<u16>(i % 5 == 0 ? 0x04CB : Mix32(i + 5u));
        tile8[static_cast<std::size_t>(i)] = static_cast<u8>(i % 6 == 0 ? 1 : Mix32(i + 9u));
        palette[static_cast<std::size_t>(i)] = static_cast<u16>(Mix32(static_cast<u32>(i) * 13u));
    }

    for (int kind = 0; kind < 4; ++kind)
    {
        backend.Reset(seed++);
        const void* source = kind < 2 ? static_cast<const void*>(tile16.data()) : tile8.data();
        backend.Tile(kind, source, palette.data(), 33, 47);
        backend.Tile(kind, source, palette.data(), 600, 400);
        report("tile/" + std::to_string(kind), screenHash());
    }

    std::vector<u16> pixels(1000);
    std::vector<u16> other(1000);

    const struct
    {
        PixelOperation operation;
        int a;
        int b;
        int c;
        const char* name;
    } operations[] = {

        {PixelOperation::Tint, 3, -2, 7, "tint"},
        {PixelOperation::Tint, -9, 4, -1, "tint-negative"},
        {PixelOperation::ShiftRed, 5, 0, 0, "red-up"},
        {PixelOperation::ShiftRed, -6, 0, 0, "red-down"},
        {PixelOperation::ShiftGreenPairs, 4, 0, 0, "green-up"},
        {PixelOperation::ShiftGreenPairs, -3, 0, 0, "green-down"},
        {PixelOperation::ShiftBluePairs, 9, 0, 0, "blue-up"},
        {PixelOperation::ShiftBluePairs, -2, 0, 0, "blue-down"},
        {PixelOperation::Mix, 0, 0, 0, "mix"},
        {PixelOperation::Gray, 0, 0, 0, "gray"},
        {PixelOperation::BlendPairs, 0x1234ABCD, 0, 0, "blend-pairs"},
    };

    for (const auto& operation : operations)
    {
        for (std::size_t i = 0; i < pixels.size(); ++i)
        {
            pixels[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * 3u + 1u));
            other[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * 7u + 2u));
        }

        backend.Pixels(
            operation.operation,
            pixels.data(),
            pixels.size(),
            other.data(),
            operation.a,
            operation.b,
            operation.c
        );
        report(std::string("pixels/") + operation.name, Hash(pixels.data(), pixels.size() * 2));
    }

    std::vector<u32> colors;

    for (u32 red = 0; red < 32; red += 3)
    {
        for (u32 green = 0; green < 32; green += 5)
        {
            for (u32 blue = 0; blue < 32; blue += 7)
            {
                colors.push_back(backend.MakeColor(red, green, blue));
            }
        }
    }

    report("make-color", Hash(colors.data(), colors.size() * 4));
}

}  // namespace golden
