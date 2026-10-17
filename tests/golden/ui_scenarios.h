#pragma once

// Box and frame scenarios shared by the legacy golden generator and the UI drawing test (see draw_scenarios.h).

#include <array>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

class UiBackend
{
public:
    virtual ~UiBackend() = default;
    // Fills the screen with the background of `seed` and sets the clip rectangle.
    virtual void Reset(u32 seed, int left, int top, int right, int bottom) = 0;
    virtual void Fill(int x, int y, int width, int height, int red, int green, int blue, int mode) = 0;
    virtual void Frame(int id, int left, int top, int right, int bottom, int alpha, int border) = 0;
    virtual const u16* Screen() const = 0;
};

constexpr int kIndicesPerFile = 3000;  // the picture ids of a tsw file (all_sys.tsw is the fourth)
constexpr int kSysFirstId = 3 * kIndicesPerFile;
constexpr std::size_t kMaxFramePictures = 24;
constexpr u16 kFramePictureFrames = 9;  // a window frame: eight borders and the body
constexpr int kTaggedFrame = 0x234A;    // and the window frame with a name tag
constexpr u16 kTaggedFrameFrames = 10;
constexpr int kFillModes = 6;  // FillMode::Blend .. FillMode::Gradient
constexpr int kBlendMode = 0;
constexpr std::size_t kClipSeed = 1000;  // the background seed of a fill scenario
constexpr std::size_t kModeSeed = 100;
constexpr std::size_t kBoxSeed = 10;

struct Box
{
    int x;
    int y;
    int width;
    int height;
};

struct FillColor
{
    int red;
    int green;
    int blue;
};

struct FrameCase
{
    int left;
    int top;
    int right;
    int bottom;
    int alpha;
    int border;
};

constexpr ClipRect kFullScreen = {0, 0, kScreenWidth, kScreenHeight};
constexpr std::array<Box, 7> kBoxes = {{
    {10, 10, 100, 50},
    {-20, 30, 60, 40},
    {600, 450, 100, 100},
    {5, 5, 1, 10},
    {0, 0, kScreenWidth, kScreenHeight},
    {33, 77, 201, 123},
    {120, 90, 7, 1},
}};

constexpr std::array<FillColor, 4> kColors = {{{31, 0, 0}, {5, 10, 15}, {0, 31, 31}, {-4, 6, -2}}};
constexpr std::array<ClipRect, 2> kClips = {{kFullScreen, {100, 80, 400, 300}}};
constexpr std::array<FrameCase, 6> kFrames = {{
    {100, 100, 300, 200, 0, 8},
    {100, 100, 300, 200, 8, 8},
    {50, 60, 590, 420, 0, static_cast<int>(0x80000010)},
    {-10, -10, 100, 50, 4, 12},
    {500, 400, 700, 500, 0, 6},
    {200, 150, 230, 170, 12, 4},
}};

// Picture ids (kSysFirstId + entry) of the all_sys.tsw entries with at least kFramePictureFrames frames
// (kTaggedFrameFrames for kTaggedFrame), up to kMaxFramePictures.
inline std::vector<int> FramePictures(const std::string& sysPath)
{
    std::vector<int> ids;
    std::ifstream file(sysPath, std::ios::binary);

    for (int entry = 1; file && entry < kIndicesPerFile && ids.size() < kMaxFramePictures; ++entry)
    {
        const u32 base =
            ReadU32(file, kTswDirectoryStart + kTswDirectoryBytes * static_cast<u32>(entry - 1) + kTswOffsetAt);

        if (!file)
        {
            break;
        }

        if (base == 0 || ReadU16(file, base + kTswMagicAt) != kTswMagic)
        {
            continue;
        }

        const int id = kSysFirstId + entry;

        if (ReadU16(file, base + kTswCountAt) >= (id == kTaggedFrame ? kTaggedFrameFrames : kFramePictureFrames))
        {
            ids.push_back(id);
        }
    }

    return ids;
}

inline void

RunUi(UiBackend& backend, const std::string& sysPath, const std::function<void(const std::string&, u64)>& report)
{
    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), kScreenBytes);
    };

    for (std::size_t clip = 0; clip < kClips.size(); ++clip)
    {
        for (int mode = 0; mode < kFillModes; ++mode)
        {
            for (std::size_t box = 0; box < kBoxes.size(); ++box)
            {
                // The blend of a box 1 pixel wide runs away in the original (and in the legacy code).
                if (mode == kBlendMode && kBoxes[box].width == 1)
                {
                    continue;
                }

                for (std::size_t color = 0; color < kColors.size(); ++color)
                {
                    const ClipRect& c = kClips[clip];
                    const auto seed =
                        clip * kClipSeed + static_cast<std::size_t>(mode) * kModeSeed + box * kBoxSeed + color;
                    backend.Reset(static_cast<u32>(seed), c.left, c.top, c.right, c.bottom);
                    const Box& b = kBoxes[box];
                    const FillColor& k = kColors[color];
                    backend.Fill(b.x, b.y, b.width, b.height, k.red, k.green, k.blue, mode);
                    report(
                        "fill/" + std::to_string(clip) + "/" + std::to_string(mode) + "/" + std::to_string(box) + "/" +
                            std::to_string(color),
                        screenHash()
                    );
                }
            }
        }
    }

    for (const int id : FramePictures(sysPath))
    {
        for (std::size_t f = 0; f < kFrames.size(); ++f)
        {
            const FrameCase& r = kFrames[f];
            backend.Reset(
                static_cast<u32>(id + f),
                kFullScreen.left,
                kFullScreen.top,
                kFullScreen.right,
                kFullScreen.bottom
            );
            backend.Frame(id, r.left, r.top, r.right, r.bottom, r.alpha, r.border);
            report("frame/" + std::to_string(id) + "/" + std::to_string(f), screenHash());
        }
    }
}

}  // namespace golden
