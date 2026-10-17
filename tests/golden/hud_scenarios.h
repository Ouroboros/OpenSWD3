#pragma once

// Battle screen helper scenarios shared by the legacy golden generator and the BattleHud test (see draw_scenarios.h):
// one sequence of calls on one helper object; every step reports its result, the screen and the kept state.

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

class HudBackend
{
public:
    virtual ~HudBackend() = default;
    virtual void Reset(u32 seed) = 0;
    virtual void SeedRandom(u32 seed) = 0;
    virtual int PictureFrame0(int id, int x, int y) = 0;
    virtual int Portrait(int index, int x, int y, int dim, bool highlight) = 0;
    virtual int Arrow(int x, int y, int slot) = 0;
    virtual int Bar(int id, int frame, int x, int y, int width) = 0;
    virtual int PictureFrame(int id, int frame, int x, int y) = 0;
    virtual int Gauge(int id, int x, int y, int fill) = 0;
    virtual int GaugeAlt(int id, int x, int y, int fill) = 0;
    virtual int GaugeWide(int id, int x, int y, int fill) = 0;
    virtual int SmallNumber(int font, int value, int x, int y) = 0;
    virtual int Number(int font, int value, int x, int y, int digitMode) = 0;
    virtual int SlotAnimation(int id, int slot, int x, int y) = 0;
    virtual int Fill(int x, int y, int width, int height, u32 color) = 0;
    virtual int Effect(int id, int action, int x, int y, int mirror) = 0;
    virtual int ResetEffect() = 0;
    virtual int Cursor(int id, int x, int y) = 0;
    virtual int Digit(int font, int digit, int x, int y, int digitMode) = 0;
    virtual int ScrollBar(int id, int x, int y, int rows, int offset, int style, int count, int first) = 0;
    virtual int CoinToss() = 0;
    virtual int TimingBar(int x, int y) = 0;
    virtual int TallGauge(int x, int y, int level) = 0;
    virtual int AnimationLoad(int x, int y, int id, int divisor) = 0;
    virtual int AnimationDraw() = 0;
    virtual int AnimationPlayShifted(int amount) = 0;
    virtual int AnimationFree() = 0;
    // numberX, numberValue, scroll step, thumb top / bottom, coin frames / state / delay, timing counter / hit.
    virtual std::vector<i32> State() = 0;
    virtual const u16* Screen() const = 0;
};

// The positions and arguments of the HUD scenarios.
struct HudPoint
{
    int x;
    int y;
};

constexpr u32 kHudSeed = 1234;
constexpr std::array<int, 6> kHudGauges = {0x241F, 0x235F, 0x2352, 0x2350, 0x2354, 0x2355};
constexpr HudPoint kFrame0At = {30, 40};
constexpr int kFrame0Step = 7;  // frame 0 moves right with each gauge picture
constexpr HudPoint kFrame1At = {200, 60};
constexpr HudPoint kBarAt = {100, 300};
constexpr int kBarStep = 13;
constexpr int kBarBase = -10;
constexpr HudPoint kGaugeAt = {300, 100};
constexpr int kGaugeStep = 9;
constexpr HudPoint kGaugeAltAt = {300, 140};
constexpr int kGaugeAltStep = 11;
constexpr HudPoint kGaugeWideAt = {300, 180};
constexpr int kGaugeWideStep = 7;
constexpr std::array<int, 3> kNumberFonts = {0x2354, 0x2355, 0x2356};
constexpr std::array<int, 6> kSmallValues = {0, 7, 42, 999, 1234, 9999};
constexpr HudPoint kSmallAt = {400, 200};
constexpr std::array<int, 3> kNumberModes = {0, 0x4000, 0x8000};
constexpr std::array<int, 6> kNumberValues = {0, 5, 60, 1000000, 2147483647, -3};
constexpr int kNumberFont = 0x2356;
constexpr HudPoint kNumberAt = {100, 250};
constexpr int kDigitMask = 3;
constexpr HudPoint kDigit0At = {20, 20};
constexpr HudPoint kDigit1At = {40, 20};
constexpr int kLastDigitFont = 5;
constexpr int kLastDigit = 3;
constexpr HudPoint kLastDigitAt = {60, 20};
constexpr int kAnimationFrames = 30;
constexpr u32 kAnimationSeed = 500;
constexpr int kPortraits = 4;
constexpr HudPoint kPortraitAt = {320, 300};
constexpr int kPortraitHighlightEvery = 3;
constexpr int kPortraitFlagEvery = 5;
constexpr HudPoint kArrowAt = {100, 120};
constexpr int kArrows = 4;
constexpr std::array<int, 2> kSlotPictures = {0x238F, 0x2393};
constexpr int kSlotFrames = 8;
constexpr HudPoint kSlotAt = {200, 200};
constexpr int kCursorPicture = 0x2391;
constexpr HudPoint kCursorAt = {250, 250};
constexpr std::array<int, 2> kEffectPictures = {0x2394, 0x23A0};  // the first for the first kEffectSwitch frames
constexpr int kEffectSwitch = 15;
constexpr int kEffectFrames = 3;
constexpr HudPoint kEffectAt = {300, 300};
constexpr int kEffectFlags = 2;
constexpr std::array<u32, 3> kFillColors = {0x0000FFFFu, 0x12345678u, 0x7BEF7BEFu};
constexpr HudPoint kFillAt = {50, 60};
constexpr int kFillWidth = 100;
constexpr int kFillHeight = 40;
constexpr std::array<int, 3> kScrollCounts = {8, 12, 30};
constexpr std::array<int, 3> kScrollFirsts = {0, 3, 23};
constexpr int kScrollSeed = 100;
constexpr int kScrollPicture = 0x232A;
constexpr HudPoint kScrollAt = {0x190, 0x9E};
constexpr int kScrollRows = 4;
constexpr int kScrollSteps = 5;
constexpr int kScrollThumbs = 3;
constexpr int kCoinFrames = 400;
constexpr u32 kCoinSeeds = 7;
constexpr int kTimingFrames = 140;
constexpr u32 kTimingSeeds = 5;
constexpr HudPoint kTimingAt = {150, 300};
constexpr int kTallLevels = 7;
constexpr HudPoint kTallAt = {500, 100};

inline void RunHud(HudBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    int step = 0;

    auto record = [&](const std::string& name, int result)
    {
        std::vector<i32> state = backend.State();
        state.push_back(result);
        const u64 screen = Hash(backend.Screen(), kScreenBytes);
        state.push_back(static_cast<i32>(screen));
        state.push_back(static_cast<i32>(screen >> (CHAR_BIT * sizeof(u32))));
        report(std::to_string(step++) + "/" + name, Hash(state.data(), state.size() * sizeof(i32)));
    };

    backend.SeedRandom(kHudSeed);

    for (std::size_t g = 0; g < kHudGauges.size(); ++g)
    {
        const int picture = kHudGauges[g];
        const int index = static_cast<int>(g);
        backend.Reset(static_cast<u32>(g));
        record("frame0", backend.PictureFrame0(picture, kFrame0At.x + kFrame0Step * index, kFrame0At.y));
        record("frame1", backend.PictureFrame(picture, 1, kFrame1At.x, kFrame1At.y));
        record("bar", backend.Bar(picture, 1, kBarAt.x, kBarAt.y, index * kBarStep + kBarBase));
        record("gauge", backend.Gauge(picture, kGaugeAt.x, kGaugeAt.y, index * kGaugeStep));
        record("gauge-alt", backend.GaugeAlt(picture, kGaugeAltAt.x, kGaugeAltAt.y, index * kGaugeAltStep));
        record("gauge-wide", backend.GaugeWide(picture, kGaugeWideAt.x, kGaugeWideAt.y, index * kGaugeWideStep));
    }

    for (const int font : kNumberFonts)
    {
        for (const int value : kSmallValues)
        {
            backend.Reset(static_cast<u32>(font + value));
            record("small/" + std::to_string(value), backend.SmallNumber(font, value, kSmallAt.x, kSmallAt.y));
        }
    }

    for (const int mode : kNumberModes)
    {
        for (const int value : kNumberValues)
        {
            backend.Reset(static_cast<u32>(mode + value));
            record(
                "number/" + std::to_string(value),
                backend.Number(kNumberFont, value, kNumberAt.x, kNumberAt.y, mode)
            );
            record("digit0", backend.Digit(0, value & kDigitMask, kDigit0At.x, kDigit0At.y, mode));
            record("digit1", backend.Digit(1, 0, kDigit1At.x, kDigit1At.y, mode));
            record("digit-last", backend.Digit(kLastDigitFont, kLastDigit, kLastDigitAt.x, kLastDigitAt.y, mode));
        }
    }

    for (int frame = 0; frame < kAnimationFrames; ++frame)
    {
        backend.Reset(kAnimationSeed + static_cast<u32>(frame));
        record(
            "portrait",
            backend.Portrait(
                frame % kPortraits,
                kPortraitAt.x,
                kPortraitAt.y,
                frame % kPortraitHighlightEvery == 0 ? 1 : 0,
                frame % kPortraitFlagEvery == 0
            )
        );
        record("arrow", backend.Arrow(kArrowAt.x + frame, kArrowAt.y, frame % kArrows));
        record(
            "slot",
            backend.SlotAnimation(
                kSlotPictures[static_cast<std::size_t>(frame % kSlotPictures.size())],
                frame % kSlotFrames,
                kSlotAt.x,
                kSlotAt.y
            )
        );
        record("cursor", backend.Cursor(kCursorPicture, kCursorAt.x, kCursorAt.y));
        record(
            "effect",
            backend.Effect(
                kEffectPictures[frame < kEffectSwitch ? 0 : 1],
                frame % kEffectFrames,
                kEffectAt.x,
                kEffectAt.y,
                frame % kEffectFlags
            )
        );
    }

    record("effect-reset", backend.ResetEffect());

    for (const u32 color : kFillColors)
    {
        backend.Reset(color);
        record("fill", backend.Fill(kFillAt.x, kFillAt.y, kFillWidth, kFillHeight, color));
    }

    for (const int count : kScrollCounts)
    {
        for (const int first : kScrollFirsts)
        {
            backend.Reset(static_cast<u32>(count * kScrollSeed + first));
            record(
                "scroll",
                backend.ScrollBar(
                    kScrollPicture,
                    kScrollAt.x,
                    kScrollAt.y,
                    kScrollRows,
                    first % kScrollSteps,
                    (count + first) % kScrollThumbs,
                    count,
                    first
                )
            );
        }
    }

    for (int frame = 0; frame < kCoinFrames; ++frame)
    {
        backend.Reset(static_cast<u32>(frame) % kCoinSeeds);
        record("coin", backend.CoinToss());
    }

    for (int frame = 0; frame < kTimingFrames; ++frame)
    {
        backend.Reset(static_cast<u32>(frame) % kTimingSeeds);
        record("timing", backend.TimingBar(kTimingAt.x, kTimingAt.y));
    }

    for (int level = 0; level <= kTallLevels; ++level)
    {
        backend.Reset(static_cast<u32>(level));
        record("tall", backend.TallGauge(kTallAt.x, kTallAt.y, level));
    }
}

}  // namespace golden
