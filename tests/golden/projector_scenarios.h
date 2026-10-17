#pragma once

// Projector and picture operation scenarios shared by the legacy golden generator and the Projector test (see
// draw_scenarios.h). Where the original reads or writes outside a picture, the legacy backend pads its buffers (with
// the skip key, the fill key or zero words) so that it behaves like the new implementation's recorded deviation.

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

// The RLE fill and skip colours as the projector compares them (not converted to the display format).
constexpr u16 kFillKey = 0x026B;
constexpr u16 kSkipKey = 0x319F;

constexpr int kUnitScale = 1024;   // the scales are fixed point
constexpr int kRowExtraWords = 3;  // an uncompressed row: two header words, the pixels and an end word
constexpr u16 kRowEnd = 0x5A5A;
constexpr int kFullCircle = 360;
constexpr int kAngleStep = 7;
constexpr int kDirectionSteps = 40;
constexpr int kLineSteps = 60;
constexpr std::size_t kScaledFrames = 24;
constexpr std::size_t kScaledSeed = 31;
constexpr std::size_t kExplosionFrames = 8;
constexpr int kMinExplosionSide = 2;
constexpr std::size_t kExplosionSeed = 17;
constexpr std::size_t kHitFrames = 40;
constexpr int kHitMargin = 3;
constexpr int kHitStep = 2;
constexpr int kHitOriginX = 100;
constexpr int kHitOriginY = 50;
constexpr std::size_t kShiftFrames = 12;
constexpr int kShiftModes = 4;  // the first kVerticalShifts move rows
constexpr int kVerticalShifts = 2;
constexpr int kShiftPart = 3;
constexpr std::array<u16, 3> kFirstWords = {0x8010, 0x4010, 0x0010};

// RawPixels with its key colours replaced by kFillKey and kSkipKey.
inline std::vector<u16> KeyedPixels(const Frame& frame)
{
    std::vector<u16> pixels = RawPixels(frame);

    for (u16& pixel : pixels)
    {
        if (pixel == kFillColor)
        {
            pixel = kFillKey;
        }
        else if (pixel == kSkipColor)
        {
            pixel = kSkipKey;
        }
    }

    return pixels;
}

struct ScaledScenario
{
    int x = 0;
    int y = 0;
    int scaleX = kUnitScale;
    int scaleY = kUnitScale;
    u16 flags = 0;
    int angle = 0;
};

struct ExplosionScenario
{
    int x = 0;
    int y = 0;
    int targetLeft = 0;
    int targetWidth = 0;
    int targetTop = 0;
    int targetHeight = 0;
    u16 lifeBase = 0;
    u16 speed = 0;
    u16 bursts = 0;
    u16 divisor = 0;
    u16 flags = 0;
    u32 seed = 0;
    int frames = 0;
};

class ProjectorBackend
{
public:
    virtual ~ProjectorBackend() = default;
    // Fills the screen with the background of `seed` and clears the particle range.
    virtual void Reset(u32 seed) = 0;
    virtual void DrawScaled(const std::vector<u16>& pixels, int width, int height, const ScaledScenario& scenario) = 0;
    // Runs the explosion for scenario.frames frames after marking it started (the first call seeds rand with the time)
    // and seeding rand; returns per frame the result, the live particles, the opaque count, the bursts left and the
    // screen hash, and finally the hash of the picture pixels.
    virtual std::vector<u8> Explosion(
        const std::vector<u16>& pixels,
        int width,
        int height,
        const ExplosionScenario& scenario
    ) = 0;
    virtual int HitTest(const Frame& frame, int x, int y, int originX, int originY) = 0;
    virtual std::vector<u8> Shift(const std::vector<u8>& data, int mode, int amount) = 0;
    // dirX then dirY.
    virtual std::vector<i32> Directions() = 0;
    // The positions of `steps` direction steps from (0, 0) along `angle`.
    virtual std::vector<i32> Steps(int angle, int steps) = 0;
    // The positions and results of line steps from (fromX, fromY) to (toX, toY), at most `steps`.
    virtual std::vector<i32> LineSteps(int fromX, int fromY, int toX, int toY, int steps) = 0;
    virtual const u16* Screen() const = 0;
};

// The uncompressed picture ShiftPicture works on: 0xFFFF, width, height, 0, then per row 2 header words, the pixels
// and 1 word.
inline std::vector<u8> UncompressedPicture(const std::vector<u16>& pixels, int width, int height, u16 firstWord)
{
    std::vector<u16> words = {kRleMagic, static_cast<u16>(width), static_cast<u16>(height), 0};

    for (int y = 0; y < height; ++y)
    {
        words.push_back(y == 0 ? firstWord : static_cast<u16>(sizeof(u16) * (width + kRowExtraWords)));
        words.push_back(static_cast<u16>(y));

        for (int x = 0; x < width; ++x)
        {
            words.push_back(pixels[static_cast<std::size_t>(y) * width + x]);
        }

        words.push_back(kRowEnd);
    }

    std::vector<u8> data(words.size() * sizeof(u16));
    std::memcpy(data.data(), words.data(), data.size());
    return data;
}

inline void RunProjector(
    ProjectorBackend& backend,
    const std::vector<Frame>& frames,
    const std::function<void(const std::string&, u64)>& report
)
{
    auto screenHash = [&backend]
    {
        return Hash(backend.Screen(), kScreenBytes);
    };

    const std::vector<i32> directions = backend.Directions();
    report("directions", Hash(directions.data(), directions.size() * sizeof(i32)));

    for (int angle = 0; angle < kFullCircle; angle += kAngleStep)
    {
        const std::vector<i32> steps = backend.Steps(angle, kDirectionSteps);
        report("steps/" + std::to_string(angle), Hash(steps.data(), steps.size() * sizeof(i32)));
    }

    const std::array<std::array<int, 4>, 8> lines = {{
        {0, 0, 10, 3},
        {5, 5, -7, 20},
        {100, 50, 100, 10},
        {3, 4, 30, 4},
        {0, 0, 9, 9},
        {20, 0, 0, 13},
        {-5, -5, 40, 17},
        {7, 7, 7, 7},
    }};

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        const auto& line = lines[i];
        const std::vector<i32> steps = backend.LineSteps(line[0], line[1], line[2], line[3], kLineSteps);
        report("line/" + std::to_string(i), Hash(steps.data(), steps.size() * sizeof(i32)));
    }

    const std::array<ScaledScenario, 10> scaled = {{
        {100, 300, 1024, 1024, 0, 0},
        {200, 250, 512, 512, 0, 45},
        {-30, 400, 1536, 1024, 1, 90},
        {500, 100, 2048, 2048, 0x16, 200},
        {300, 470, 700, 1300, 0x17, 359},
        {600, 200, 1024, 600, 1, 10},
        {0, 0, 1024, 1024, 0, 0},
        {320, 240, 300, 300, 0x16, 135},
        {50, 479, 1100, 900, 1, 270},
        {610, 30, 800, 2000, 0x17, 5},
    }};

    const std::size_t scaledFrames = std::min(frames.size(), kScaledFrames);

    for (std::size_t f = 0; f < scaledFrames; ++f)
    {
        const std::vector<u16> pixels = KeyedPixels(frames[f]);

        for (std::size_t s = 0; s < scaled.size(); ++s)
        {
            backend.Reset(static_cast<u32>(f * kScaledSeed + s));
            backend.DrawScaled(pixels, frames[f].width, frames[f].height, scaled[s]);
            report("scaled/" + std::to_string(f) + "/" + std::to_string(s), screenHash());
        }
    }

    const std::array<ExplosionScenario, 6> explosions = {{
        {200, 150, 400, 100, 300, 80, 20, 3, 1, 1, 0, 11, 40},
        {100, 100, 20, 60, 20, 60, 10, 5, 3, 2, 0x40, 22, 60},
        {300, 200, 50, 500, 400, 50, 5, 2, 4, 1, 0x80, 33, 60},
        {250, 120, 500, 100, 50, 100, 8, 4, 2, 3, 0x81, 44, 50},
        {150, 220, 10, 600, 10, 400, 30, 6, 1, 1, 0x16, 55, 40},
        {320, 180, 300, 40, 200, 40, 12, 3, 5, 2, 0x57, 66, 80},
    }};

    const std::size_t explosionFrames = std::min(frames.size(), kExplosionFrames);

    for (std::size_t f = 0; f < explosionFrames; ++f)
    {
        const std::vector<u16> pixels = KeyedPixels(frames[f]);

        if (frames[f].width < kMinExplosionSide || frames[f].height < kMinExplosionSide)
        {
            continue;
        }

        for (std::size_t s = 0; s < explosions.size(); ++s)
        {
            backend.Reset(static_cast<u32>(f * kExplosionSeed + s));
            const std::vector<u8> result = backend.Explosion(pixels, frames[f].width, frames[f].height, explosions[s]);
            report("explosion/" + std::to_string(f) + "/" + std::to_string(s), Hash(result.data(), result.size()));
        }
    }

    const std::size_t hitFrames = std::min(frames.size(), kHitFrames);

    for (std::size_t f = 0; f < hitFrames; ++f)
    {
        std::vector<u8> hits;
        const Frame& frame = frames[f];

        for (int y = -kHitMargin; y < frame.height + kHitMargin; y += kHitStep)
        {
            for (int x = -kHitMargin; x < frame.width + kHitMargin; x += kHitStep)
            {
                hits.push_back(
                    static_cast<u8>(backend.HitTest(frame, x + kHitOriginX, y + kHitOriginY, kHitOriginX, kHitOriginY))
                );
            }
        }

        report("hit/" + std::to_string(f), Hash(hits.data(), hits.size()));
    }

    const std::size_t shiftFrames = std::min(frames.size(), kShiftFrames);

    for (std::size_t f = 0; f < shiftFrames; ++f)
    {
        const Frame& frame = frames[f];
        const std::vector<u16> pixels = KeyedPixels(frame);

        for (int mode = 0; mode < kShiftModes; ++mode)
        {
            const int limit = mode < kVerticalShifts ? frame.height : frame.width;

            for (const int amount : {0, 1, limit / kShiftPart, limit})
            {
                for (const u16 firstWord : kFirstWords)
                {
                    const std::vector<u8> shifted =
                        backend.Shift(UncompressedPicture(pixels, frame.width, frame.height, firstWord), mode, amount);
                    report(
                        "shift/" + std::to_string(f) + "/" + std::to_string(mode) + "/" + std::to_string(amount) + "/" +
                            std::to_string(firstWord),
                        Hash(shifted.data(), shifted.size())
                    );
                }
            }
        }
    }
}

}  // namespace golden
