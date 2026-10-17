#pragma once

// Draw scenarios shared by the golden generator of the legacy implementation (reverse/analysis/golden/) and the
// Painter test: the same sequence of draws runs through both backends, and the hash of the visible screen after
// every scenario is compared. Header-only and independent of either implementation.

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace golden
{

// The project integer types, repeated here because these headers are also built in the legacy tree.
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// The tiles of DrawTile16, DrawTile16Keyed, DrawTile8 and DrawTile8Keyed.
enum TileKind : int
{
    Tile16,
    Tile16Keyed,
    TilePalette,
    TilePaletteKeyed,
    TileKinds,
};

using i32 = std::int32_t;

constexpr int kScreenWidth = 640;
constexpr int kScreenHeight = 480;

constexpr std::size_t kScreenBytes = static_cast<std::size_t>(kScreenWidth) * kScreenHeight * sizeof(u16);

// The tsw layout (as TswArchive reads it): the directory, an entry header and its frame records.
constexpr u32 kTswDirectoryStart = 0x1C;
constexpr u32 kTswDirectoryBytes = 0x2C;
constexpr u32 kTswOffsetAt = 0x18;
constexpr u32 kTswMagicAt = 4;
constexpr u16 kTswMagic = 0xABCD;
constexpr u32 kTswCountAt = 6;
constexpr u32 kTswFormatAt = 8;
constexpr u32 kTswHeaderBytes = 12;
constexpr u16 kTswPaletteFormat = 8;
constexpr u32 kTswPaletteBytes = 0x200;
constexpr u32 kTswFrameBytes = 0x24;
constexpr u32 kTswPackedAt = 4;
constexpr u32 kTswSizeAt = 8;
constexpr std::size_t kRleSlack = 64;  // room past the end of a frame

// The RLE format (as RleHeader describes it).
constexpr u16 kRleMagic = 0xFFFF;
constexpr std::size_t kRleHeaderWords = 4;
constexpr std::size_t kRleWidthWord = 1;
constexpr std::size_t kRleHeightWord = 2;
constexpr std::size_t kRleFormatWord = 3;
constexpr u16 kRleTrueColor = 0x10;
constexpr u16 kRleLengthMask = 0x7FFF;
constexpr u16 kRleCountMask = 0x3FFF;
constexpr u16 kRleSkipRun = 0x8000;
constexpr u16 kRleKindMask = 0xC000;
constexpr u16 kFillColor = 0x04CB;  // the RLE fill and skip colours converted to RGB565
constexpr u16 kSkipColor = 0x631F;

// The hashes: FNV-1a over bytes, and a 32-bit mixer for the generated inputs.
constexpr u64 kFnvOffset = 1469598103934665603ull;
constexpr u64 kFnvPrime = 1099511628211ull;
constexpr int kMixOuterShift = 16;
constexpr int kMixInnerShift = 15;
constexpr u32 kMixFirst = 0x7FEB352Du;
constexpr u32 kMixSecond = 0x846CA68Bu;
constexpr u32 kGoldenRatio = 0x9E3779B9u;

// The pictures of the scenarios: tsw entries and the frames taken from each.
constexpr std::array<int, 4> kCharEntries = {7, 23, 33, 385};
constexpr int kCharFrames = 2;
constexpr std::array<int, 3> kSystemEntries = {14, 36, 99};
constexpr std::array<int, 2> kItemEntries = {1, 2};

// The draws of the scenarios.
struct ClipRect
{
    int left;
    int top;
    int right;
    int bottom;
};

struct Place
{
    int x;
    int y;
};

struct Zoom
{
    int anchorX;
    int anchorY;
    int zoomX;
    int zoomY;
};

constexpr u32 kAlphaSeed = 77;  // the alpha map: a level of kAlphaLevels steps of kAlphaStep per pixel
constexpr u32 kAlphaLevels = 16;
constexpr u32 kAlphaStep = 4;
constexpr int kClipCases = 2;  // each RLE draw without and with kRleClip
constexpr ClipRect kRleClip = {90, 70, 530, 410};
constexpr ClipRect kRawClip = {40, 30, 600, 450};
constexpr ClipRect kZoomClip = {60, 50, 580, 430};
constexpr u32 kColorPair = 0x7BEF1234u;
constexpr int kOutlineX = 150;
constexpr int kOutlineY = 120;
constexpr u32 kOutlineColor = 0x001F001Fu;
constexpr std::size_t kPaletteColors = 256;
constexpr u32 kFramePaletteSeed = 31;
constexpr u16 kKeyEvery = 7;  // every pixel of a multiple of it becomes the key colour 1 in the indexed picture
constexpr u16 kByteMask = 0xFF;
constexpr int kRawKinds = 2;  // a raw picture drawn as 16-bit pixels and as palette indices
constexpr int kIndexedKind = 1;
constexpr std::array<u32, 4> kRawOnlyModes = {0x01, 0x05, 0x14, 0x08};  // modes without an indexed form
constexpr int kRawAlpha = 6;
constexpr int kTilePixels = 16 * 16;
constexpr int kTileFillEvery = 5;
constexpr u32 kTileSeed = 5;
constexpr int kTileKeyEvery = 6;
constexpr u32 kTile8Seed = 9;
constexpr u32 kTilePaletteSeed = 13;
constexpr std::array<Place, 2> kTilePlaces = {{{33, 47}, {600, 400}}};
constexpr std::size_t kPixelCount = 1000;
constexpr u32 kPixelStep = 3;
constexpr u32 kPixelSeed = 1;
constexpr u32 kOtherStep = 7;
constexpr u32 kOtherSeed = 2;
constexpr u32 kLevels = 32;  // MakeColor over five-bit levels in steps
constexpr u32 kRedStep = 3;
constexpr u32 kGreenStep = 5;
constexpr u32 kBlueStep = 7;

struct Frame
{
    std::vector<u8> rle;
    int width = 0;
    int height = 0;
};

// Decompresses `size` bytes into `output` (sized to the expected length); false on failure.
using Decompress = std::function<bool(const u8* input, std::size_t size, std::vector<u8>& output)>;

// An environment variable, empty when it is not set.
inline std::string Environment(const std::string& name)
{
    const auto* value = std::getenv(name.c_str());
    return value != nullptr ? std::string(value) : std::string();
}

inline u32 ReadU32(std::ifstream& file, u32 offset)
{
    u32 value = 0;
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

inline u16 ReadU16(std::ifstream& file, u32 offset)
{
    u16 value = 0;
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
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
        const u32 base =
            ReadU32(file, kTswDirectoryStart + kTswDirectoryBytes * static_cast<u32>(entry - 1) + kTswOffsetAt);

        if (base == 0 || ReadU16(file, base + kTswMagicAt) != kTswMagic)
        {
            continue;
        }

        const u16 count = ReadU16(file, base + kTswCountAt);
        const u16 format = ReadU16(file, base + kTswFormatAt);
        const u32 table = base + kTswHeaderBytes + (format == kTswPaletteFormat ? kTswPaletteBytes : 0);

        for (int index = 0; index < count && index < perEntry; ++index)
        {
            const u32 offset = ReadU32(file, table + kTswFrameBytes * index);
            const u32 packed = ReadU32(file, table + kTswFrameBytes * index + kTswPackedAt);
            const u32 size = ReadU32(file, table + kTswFrameBytes * index + kTswSizeAt);
            std::vector<u8> input(packed);
            file.seekg(base + offset);
            file.read(reinterpret_cast<char*>(input.data()), packed);
            Frame frame;
            frame.rle.assign(size + kRleSlack, 0);
            std::vector<u8> output(size);

            if (!decompress(input.data(), packed, output))
            {
                continue;
            }

            std::memcpy(frame.rle.data(), output.data(), size);
            u16 header[kRleHeaderWords] = {};
            std::memcpy(header, output.data(), sizeof(header));

            if (header[0] != kRleMagic || (header[kRleFormatWord] & kRleTrueColor) == 0)
            {
                continue;
            }

            frame.width = header[kRleWidthWord];
            frame.height = header[kRleHeightWord];
            frames.push_back(std::move(frame));
        }
    }

    return frames;
}

// The pictures of the golden scenarios: 16-bit entries of the character, system and item archives.
inline std::vector<Frame>

LoadGoldenFrames(const std::function<std::string(const std::string&)>& gameFile, const Decompress& decompress)
{
    std::vector<Frame> frames =
        LoadFrames(gameFile("all_char.tsw"), {kCharEntries.begin(), kCharEntries.end()}, kCharFrames, decompress);
    const std::vector<Frame> system =
        LoadFrames(gameFile("all_sys.tsw"), {kSystemEntries.begin(), kSystemEntries.end()}, 1, decompress);
    const std::vector<Frame> items =
        LoadFrames(gameFile("all_item.tsw"), {kItemEntries.begin(), kItemEntries.end()}, 1, decompress);
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
    // kind: a TileKind
    virtual void Tile(int kind, const void* source, const u16* palette, int x, int y) = 0;
    virtual void
    Pixels(PixelOperation operation, u16* pixels, std::size_t count, const u16* other, int a, int b, int c) = 0;
    virtual u32 MakeColor(u32 red, u32 green, u32 blue) = 0;
    virtual const u16* Screen() const = 0;
};

inline void Append32(std::vector<u8>& out, u32 value)
{
    for (std::size_t i = 0; i < sizeof(value); ++i)
    {
        out.push_back(static_cast<u8>(value >> (CHAR_BIT * i)));
    }
}

inline u64 Hash(const void* data, std::size_t bytes)
{
    u64 hash = kFnvOffset;
    const auto* p = static_cast<const u8*>(data);

    for (std::size_t i = 0; i < bytes; ++i)
    {
        hash ^= p[i];
        hash *= kFnvPrime;
    }

    return hash;
}

inline u32 Mix32(u32 value)
{
    value ^= value >> kMixOuterShift;
    value *= kMixFirst;
    value ^= value >> kMixInnerShift;
    value *= kMixSecond;
    value ^= value >> kMixOuterShift;
    return value;
}

// The background pattern of a scenario (also used by the backends' Reset).
inline u16 Background(u32 seed, int x, int y)
{
    return static_cast<u16>(Mix32(seed * kGoldenRatio + static_cast<u32>(y * kScreenWidth + x)));
}

// A raw 16-bit picture from an RLE frame: literal pixels, fill runs 0x04CB and skip runs 0x631F (the converted RLE
// fill and skip colours of an RGB565 display).
inline std::vector<u16> RawPixels(const Frame& frame)
{
    std::vector<u16> pixels(static_cast<std::size_t>(frame.width) * frame.height, kSkipColor);
    const u8* line = frame.rle.data() + kRleHeaderWords * sizeof(u16);

    for (int y = 0; y < frame.height; ++y)
    {
        u16 header = 0;
        std::memcpy(&header, line, sizeof(header));

        if (header == 0)
        {
            break;
        }

        const u8* code = line + sizeof(u16);
        int x = 0;

        while (x < frame.width)
        {
            u16 value = 0;
            std::memcpy(&value, code, sizeof(value));
            const int count = value & kRleCountMask;
            code += sizeof(u16);

            if (count == 0)
            {
                break;
            }

            for (int i = 0; i < count && x + i < frame.width; ++i)
            {
                u16 pixel = (value & kRleSkipRun) != 0 ? kSkipColor : kFillColor;

                if ((value & kRleKindMask) == 0)
                {
                    std::memcpy(&pixel, code + sizeof(u16) * i, sizeof(pixel));
                }

                pixels[static_cast<std::size_t>(y) * frame.width + x + i] = pixel;
            }

            if ((value & kRleKindMask) == 0)
            {
                code += sizeof(u16) * count;
            }

            x += count;
        }

        line += header & kRleLengthMask;
    }

    return pixels;
}

inline void

Run(Backend& backend, const std::vector<Frame>& frames, const std::function<void(const std::string&, u64)>& reportHash)
{
    // GOLDEN_DUMP=<scenario name prefix>: the screen after that scenario is written to <GOLDEN_DUMP_FILE>.
    const std::string dumpName = Environment("GOLDEN_DUMP");
    const std::string dumpFile = Environment("GOLDEN_DUMP_FILE");

    auto report = [&](const std::string& name, u64 hash)
    {
        if (!dumpName.empty() && !dumpFile.empty() && name.rfind(dumpName, 0) == 0)
        {
            std::ofstream out(dumpFile, std::ios::binary);
            out.write(reinterpret_cast<const char*>(backend.Screen()), kScreenBytes);
        }

        reportHash(name, hash);
    };

    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), kScreenBytes);
    };

    struct Variant
    {
        std::string_view name;
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
            alphaMap[i] = static_cast<u8>((Mix32(static_cast<u32>(i) + kAlphaSeed) % kAlphaLevels) * kAlphaStep);
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
            for (int clip = 0; clip < kClipCases; ++clip)
            {
                backend.Reset(seed++);

                if (clip == 1)
                {
                    backend.SetClip(kRleClip.left, kRleClip.top, kRleClip.right, kRleClip.bottom);
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
                    call.colorPair = kColorPair;
                    result += backend.Draw(call);
                }

                report(
                    "rle/" + std::to_string(f) + "/" + std::string(variant.name) + "/clip" + std::to_string(clip) +
                        " result " + std::to_string(result),
                    screenHash()
                );
            }
        }

        backend.Reset(seed++);
        DrawCall outline;
        outline.source = frame.rle.data();
        outline.x = kOutlineX;
        outline.y = kOutlineY;
        outline.width = frame.width;
        outline.height = frame.height;
        outline.colorPair = kOutlineColor;
        backend.Outline(outline);
        report("outline/" + std::to_string(f), screenHash());

        const std::vector<u16> raw = RawPixels(frame);
        std::array<u16, kPaletteColors> palette{};

        for (std::size_t i = 0; i < palette.size(); ++i)
        {
            palette[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * kFramePaletteSeed));
        }

        std::vector<u8> indexed(raw.size());

        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            indexed[i] = static_cast<u8>(raw[i] % kKeyEvery == 0 ? 1 : raw[i] & kByteMask);
        }

        const u32 rawModes[] = {0x00, 0x01, 0x04, 0x05, 0x08, 0x14};

        for (u32 mode : rawModes)
        {
            for (int kind = 0; kind < kRawKinds; ++kind)
            {
                if (kind == kIndexedKind &&
                    std::find(kRawOnlyModes.begin(), kRawOnlyModes.end(), mode) != kRawOnlyModes.end())
                {
                    continue;
                }

                backend.Reset(seed++);
                backend.SetClip(kRawClip.left, kRawClip.top, kRawClip.right, kRawClip.bottom);
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
                    call.alpha = kRawAlpha;
                    result += backend.Draw(call);
                }

                report(
                    "raw/" + std::to_string(f) + "/" + std::to_string(mode) + "/" + std::to_string(kind) + " result " +
                        std::to_string(result),
                    screenHash()
                );
            }
        }

        const Zoom zooms[] = {
            {0x28, 0x48, 0x600, 0x400},
            {10, 20, 0x200, 0x300},
            {0, 0, 0x400, 0x400},
            {frame.width / 2, frame.height, 0x900, 0x700}
        };

        for (const auto& zoom : zooms)
        {
            for (const int mirrored : {0, 1})
            {
                backend.Reset(seed++);
                backend.SetClip(kZoomClip.left, kZoomClip.top, kZoomClip.right, kZoomClip.bottom);

                for (const Position& position : positions)
                {
                    backend.Zoom(
                        frame.rle.data(),
                        position.dx,
                        position.dy,
                        frame.width,
                        frame.height,
                        mirrored != 0,
                        zoom.anchorX,
                        zoom.anchorY,
                        zoom.zoomX,
                        zoom.zoomY
                    );
                }

                report(
                    "zoom/" + std::to_string(f) + "/" + std::to_string(zoom.zoomX) + "x" + std::to_string(zoom.zoomY) +
                        "/" + std::to_string(mirrored),
                    screenHash()
                );
            }
        }
    }

    std::vector<u16> tile16(kTilePixels);
    std::vector<u8> tile8(kTilePixels);
    std::array<u16, kPaletteColors> palette{};

    for (int i = 0; i < kTilePixels; ++i)
    {
        tile16[static_cast<std::size_t>(i)] =
            static_cast<u16>(i % kTileFillEvery == 0 ? kFillColor : Mix32(i + kTileSeed));
        tile8[static_cast<std::size_t>(i)] = static_cast<u8>(i % kTileKeyEvery == 0 ? 1 : Mix32(i + kTile8Seed));
        palette[static_cast<std::size_t>(i)] = static_cast<u16>(Mix32(static_cast<u32>(i) * kTilePaletteSeed));
    }

    for (int kind = 0; kind < TileKinds; ++kind)
    {
        backend.Reset(seed++);
        const void* source = kind < TilePalette ? static_cast<const void*>(tile16.data()) : tile8.data();

        for (const Place& place : kTilePlaces)
        {
            backend.Tile(kind, source, palette.data(), place.x, place.y);
        }

        report("tile/" + std::to_string(kind), screenHash());
    }

    std::vector<u16> pixels(kPixelCount);
    std::vector<u16> other(kPixelCount);

    const struct
    {
        PixelOperation operation;
        int a;
        int b;
        int c;
        std::string_view name;
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
            pixels[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * kPixelStep + kPixelSeed));
            other[i] = static_cast<u16>(Mix32(static_cast<u32>(i) * kOtherStep + kOtherSeed));
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
        report("pixels/" + std::string(operation.name), Hash(pixels.data(), pixels.size() * sizeof(u16)));
    }

    std::vector<u32> colors;

    for (u32 red = 0; red < kLevels; red += kRedStep)
    {
        for (u32 green = 0; green < kLevels; green += kGreenStep)
        {
            for (u32 blue = 0; blue < kLevels; blue += kBlueStep)
            {
                colors.push_back(backend.MakeColor(red, green, blue));
            }
        }
    }

    report("make-color", Hash(colors.data(), colors.size() * sizeof(u32)));
}

}  // namespace golden
