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

// Picture ids (9000 + entry) of the all_sys.tsw entries with at least 9 frames (10 for 0x234A), up to 24.
inline std::vector<int> FramePictures(const std::string& sysPath)
{
    std::vector<int> ids;
    std::ifstream file(sysPath, std::ios::binary);

    for (int entry = 1; file && entry < 3000 && ids.size() < 24; ++entry)
    {
        const u32 base = ReadU32(file, 0x1C + 0x2C * static_cast<u32>(entry - 1) + 0x18);

        if (!file)
        {
            break;
        }

        if (base == 0 || ReadU16(file, base + 4) != 0xABCD)
        {
            continue;
        }

        const int id = 9000 + entry;

        if (ReadU16(file, base + 6) >= (id == 0x234A ? 10 : 9))
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
        return Hash(backend.Screen(), static_cast<std::size_t>(kScreenWidth) * kScreenHeight * 2);
    };

    const std::array<std::array<int, 4>, 7> boxes = {{
        {10, 10, 100, 50},
        {-20, 30, 60, 40},
        {600, 450, 100, 100},
        {5, 5, 1, 10},
        {0, 0, 640, 480},
        {33, 77, 201, 123},
        {120, 90, 7, 1},
    }};

    const std::array<std::array<int, 3>, 4> colors = {{{31, 0, 0}, {5, 10, 15}, {0, 31, 31}, {-4, 6, -2}}};
    const std::array<std::array<int, 4>, 2> clips = {{{0, 0, 640, 480}, {100, 80, 400, 300}}};

    for (std::size_t clip = 0; clip < clips.size(); ++clip)
    {
        for (int mode = 0; mode <= 5; ++mode)
        {
            for (std::size_t box = 0; box < boxes.size(); ++box)
            {
                // The blend of a box 1 pixel wide runs away in the original (and in the legacy code).
                if (mode == 0 && boxes[box][2] == 1)
                {
                    continue;
                }

                for (std::size_t color = 0; color < colors.size(); ++color)
                {
                    const auto& c = clips[clip];
                    backend
                        .Reset(static_cast<u32>(clip * 1000 + mode * 100 + box * 10 + color), c[0], c[1], c[2], c[3]);
                    const auto& b = boxes[box];
                    const auto& k = colors[color];
                    backend.Fill(b[0], b[1], b[2], b[3], k[0], k[1], k[2], mode);
                    report(
                        "fill/" + std::to_string(clip) + "/" + std::to_string(mode) + "/" + std::to_string(box) + "/" +
                            std::to_string(color),
                        screenHash()
                    );
                }
            }
        }
    }

    const std::array<std::array<int, 6>, 6> frames = {{
        {100, 100, 300, 200, 0, 8},
        {100, 100, 300, 200, 8, 8},
        {50, 60, 590, 420, 0, static_cast<int>(0x80000010)},
        {-10, -10, 100, 50, 4, 12},
        {500, 400, 700, 500, 0, 6},
        {200, 150, 230, 170, 12, 4},
    }};

    for (const int id : FramePictures(sysPath))
    {
        for (std::size_t f = 0; f < frames.size(); ++f)
        {
            const auto& r = frames[f];
            backend.Reset(static_cast<u32>(id + f), 0, 0, 640, 480);
            backend.Frame(id, r[0], r[1], r[2], r[3], r[4], r[5]);
            report("frame/" + std::to_string(id) + "/" + std::to_string(f), screenHash());
        }
    }
}

}  // namespace golden
