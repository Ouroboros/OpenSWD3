#include "battle/battle_smooth.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <span>

#include "core/log.h"
#include "game/game.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr u32 kLoggedFailures = 5;  // the first frames given up are logged with the reason
constexpr u64 kNanosecondsPerMicrosecond = 1000;

}  // namespace

BattleSmooth::BattleSmooth(Game& owner)
    : game(owner)
{
}

void BattleSmooth::Begin()
{
    Game& owner = this->game;

    if (!owner.smoothMotion || owner.drawSurface == nullptr)
    {
        return;
    }

    const auto pixels = static_cast<std::size_t>(owner.painter.ScreenPitch()) * Display::Height;
    this->record.Start(owner.drawSurface, pixels);
    this->started = true;
}

void BattleSmooth::Cancel()
{
    this->record.Stop();
    this->started = false;
}

bool BattleSmooth::End()
{
    Game& owner = this->game;
    this->record.Stop();

    if (!this->started)
    {
        return false;
    }

    this->started = false;
    ++this->recorded;
    this->drawsMoving += this->record.Moving();

    if (!this->record.Complete())
    {
        if (++this->incomplete <= kLoggedFailures)
        {
            LogInfo(u8"smooth battle: frame %d has a draw that cannot be recorded", static_cast<i32>(owner.gameFrames));
        }

        return false;
    }

    const int pitch = owner.painter.ScreenPitch();
    const std::span<const u16> drawn(owner.drawSurface, static_cast<std::size_t>(pitch) * Display::Height);

    if (!this->record.Matches(owner.painter, drawn, this->scratch, pitch))
    {
        if (++this->mismatched <= kLoggedFailures)
        {
            int left = pitch;
            int top = Display::Height;
            int right = -1;
            int bottom = -1;
            int count = 0;

            for (std::size_t at = 0; at < drawn.size(); ++at)
            {
                if (drawn[at] != this->scratch[at])
                {
                    const int x = static_cast<int>(at % static_cast<std::size_t>(pitch));
                    const int y = static_cast<int>(at / static_cast<std::size_t>(pitch));
                    left = std::min(left, x);
                    top = std::min(top, y);
                    right = std::max(right, x);
                    bottom = std::max(bottom, y);
                    ++count;
                }
            }

            LogInfo(
                u8"smooth battle: frame %d is not given by its %d draws (%d pixels in %d,%d-%d,%d)",
                static_cast<i32>(owner.gameFrames),
                static_cast<i32>(this->record.Draws()),
                count,
                left,
                top,
                right,
                bottom
            );
        }

        return false;
    }

    ++this->smoothed;
    return true;
}

Surface& BattleSmooth::PaintBetween(double alpha)
{
    Game& owner = this->game;
    const auto start = std::chrono::steady_clock::now();
    const double share = std::clamp(alpha, 0.0, 1.0);

    if (this->screen == nullptr)
    {
        this->screen = std::make_unique<Surface>(Display::Width, Display::Height);
    }

    const int pitch = owner.painter.ScreenPitch();
    this->frame.resize(static_cast<std::size_t>(pitch) * Display::Height);
    this->record.Replay(owner.painter, this->frame, pitch, share);

    for (int row = 0; row < Display::Height; ++row)
    {
        const u16* line = this->frame.data() + static_cast<std::ptrdiff_t>(row) * pitch;
        std::copy_n(line, Display::Width, this->screen->Row(row));
    }

    const auto took = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()
    );
    ++this->painted;
    this->paintNs += took;
    this->paintMostNs = std::max(this->paintMostNs, took);
    return *this->screen;
}

void BattleSmooth::LogTotals() const
{
    if (this->recorded == 0)
    {
        return;
    }

    LogInfo(
        u8"smooth battle: %d frames recorded, %d drawn between, %d with a draw not recorded, %d not matching",
        static_cast<i32>(this->recorded),
        static_cast<i32>(this->smoothed),
        static_cast<i32>(this->incomplete),
        static_cast<i32>(this->mismatched)
    );
    LogInfo(u8"smooth battle: %d draws moved", static_cast<i32>(this->drawsMoving));

    if (this->painted != 0)
    {
        LogInfo(
            u8"smooth battle: %d frames drawn between in %d us on average, %d us at most",
            static_cast<i32>(this->painted),
            static_cast<i32>(this->paintNs / this->painted / kNanosecondsPerMicrosecond),
            static_cast<i32>(this->paintMostNs / kNanosecondsPerMicrosecond)
        );
    }
}

}  // namespace swd3
