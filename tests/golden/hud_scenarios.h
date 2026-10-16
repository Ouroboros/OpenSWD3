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

inline void RunHud(HudBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    int step = 0;

    auto record = [&](const std::string& name, int result)
    {
        std::vector<i32> state = backend.State();
        state.push_back(result);
        const u64 screen = Hash(backend.Screen(), static_cast<std::size_t>(kScreenWidth) * kScreenHeight * 2);
        state.push_back(static_cast<i32>(screen));
        state.push_back(static_cast<i32>(screen >> 32));
        report(std::to_string(step++) + "/" + name, Hash(state.data(), state.size() * 4));
    };

    backend.SeedRandom(1234);
    const std::array<int, 6> gauges = {0x241F, 0x235F, 0x2352, 0x2350, 0x2354, 0x2355};

    for (std::size_t g = 0; g < gauges.size(); ++g)
    {
        backend.Reset(static_cast<u32>(g));
        record("frame0", backend.PictureFrame0(gauges[g], 30 + 7 * static_cast<int>(g), 40));
        record("frame1", backend.PictureFrame(gauges[g], 1, 200, 60));
        record("bar", backend.Bar(gauges[g], 1, 100, 300, static_cast<int>(g) * 13 - 10));
        record("gauge", backend.Gauge(gauges[g], 300, 100, static_cast<int>(g) * 9));
        record("gauge-alt", backend.GaugeAlt(gauges[g], 300, 140, static_cast<int>(g) * 11));
        record("gauge-wide", backend.GaugeWide(gauges[g], 300, 180, static_cast<int>(g) * 7));
    }

    for (const int font : {0x2354, 0x2355, 0x2356})
    {
        for (const int value : {0, 7, 42, 999, 1234, 9999})
        {
            backend.Reset(static_cast<u32>(font + value));
            record("small/" + std::to_string(value), backend.SmallNumber(font, value, 400, 200));
        }
    }

    for (const int mode : {0, 0x4000, 0x8000})
    {
        for (const int value : {0, 5, 60, 1000000, 2147483647, -3})
        {
            backend.Reset(static_cast<u32>(mode + value));
            record("number/" + std::to_string(value), backend.Number(0x2356, value, 100, 250, mode));
            record("digit0", backend.Digit(0, value & 3, 20, 20, mode));
            record("digit1", backend.Digit(1, 0, 40, 20, mode));
            record("digit-last", backend.Digit(5, 3, 60, 20, mode));
        }
    }

    for (int frame = 0; frame < 30; ++frame)
    {
        backend.Reset(static_cast<u32>(500 + frame));
        record("portrait", backend.Portrait(frame % 4, 320, 300, frame % 3 == 0 ? 1 : 0, frame % 5 == 0));
        record("arrow", backend.Arrow(100 + frame, 120, frame % 4));
        record("slot", backend.SlotAnimation(frame % 2 == 0 ? 0x238F : 0x2393, frame % 8, 200, 200));
        record("cursor", backend.Cursor(0x2391, 250, 250));
        record("effect", backend.Effect(frame < 15 ? 0x2394 : 0x23A0, frame % 3, 300, 300, frame % 2));
    }

    record("effect-reset", backend.ResetEffect());

    for (const u32 color : {0x0000FFFFu, 0x12345678u, 0x7BEF7BEFu})
    {
        backend.Reset(color);
        record("fill", backend.Fill(50, 60, 100, 40, color));
    }

    for (int count : {8, 12, 30})
    {
        for (int first : {0, 3, 23})
        {
            backend.Reset(static_cast<u32>(count * 100 + first));
            record("scroll", backend.ScrollBar(0x232A, 0x190, 0x9E, 4, first % 5, (count + first) % 3, count, first));
        }
    }

    for (int frame = 0; frame < 400; ++frame)
    {
        backend.Reset(static_cast<u32>(frame % 7));
        record("coin", backend.CoinToss());
    }

    for (int frame = 0; frame < 140; ++frame)
    {
        backend.Reset(static_cast<u32>(frame % 5));
        record("timing", backend.TimingBar(150, 300));
    }

    for (int level = 0; level <= 7; ++level)
    {
        backend.Reset(static_cast<u32>(level));
        record("tall", backend.TallGauge(500, 100, level));
    }
}

}  // namespace golden
