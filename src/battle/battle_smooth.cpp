#include "battle/battle_smooth.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <span>
#include <unordered_map>

#include "core/log.h"
#include "game/game.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr u32 kLoggedFailures = 5;  // the first frames given up are logged with the reason

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

void BattleSmooth::Unit(int unit)
{
    this->record.SetTag(unit < 0 ? PaintRecord::NoTag : unit + 1);
}

void BattleSmooth::FollowUnits()
{
    Game& owner = this->game;
    const BattleState& battle = owner.battleState;
    const u32 frameNumber = owner.gameFrames;

    bool moving = false;

    auto follow = [this, frameNumber, &moving](std::size_t tag, const UnitPlacement& place)
    {
        const bool continuous = this->placesValid && this->seen[tag] + 1 == frameNumber;
        Glide& glide = this->glides[tag];
        moving = moving || (continuous && (glide.toX != place.x || glide.toY != place.y));
        glide.Follow(place.x, place.y, frameNumber, continuous, 0);
        this->seen[tag] = frameNumber;
    };

    for (u32 enemy = 0; enemy < battle.enemyCount && enemy < BattleState::Enemies; ++enemy)
    {
        follow(enemy + 1, owner.battleEnemies[enemy].placement);
    }

    for (u32 role = 0; role < battle.roleCount && role < BattleState::RoleUnits; ++role)
    {
        follow(BattleState::FirstRoleUnit + role + 1, owner.battleRoles[role].placement);
    }

    this->framesMoving += moving ? 1 : 0;
    this->placesFrame = frameNumber;
    this->placesValid = true;
}

// Each draw of this frame paired with the nearest unpaired draw of the frame before with its key, if that is within
// a glide's reach.
void BattleSmooth::FollowDraws()
{
    const u32 frameNumber = this->game.gameFrames;
    this->record.Places(this->places);
    this->moves.assign(this->places.size(), {0, 0});

    if (this->previousFrame + 1 == frameNumber && !this->previous.empty())
    {
        std::unordered_multimap<u64, std::size_t> before;

        for (std::size_t index = 0; index < this->previous.size(); ++index)
        {
            if (this->previous[index].key != 0)
            {
                before.emplace(this->previous[index].key, index);
            }
        }

        std::vector<bool> taken(this->previous.size(), false);

        for (std::size_t index = 0; index < this->places.size(); ++index)
        {
            const PaintRecord::Place& place = this->places[index];

            if (place.key == 0)
            {
                continue;
            }

            const auto [first, last] = before.equal_range(place.key);
            std::size_t nearest = this->previous.size();
            int distance = Glide::SnapDistance + 1;

            for (auto candidate = first; candidate != last; ++candidate)
            {
                const PaintRecord::Place& was = this->previous[candidate->second];
                const int apart = std::max(std::abs(place.x - was.x), std::abs(place.y - was.y));

                if (!taken[candidate->second] && apart < distance)
                {
                    distance = apart;
                    nearest = candidate->second;
                }
            }

            if (nearest != this->previous.size())
            {
                taken[nearest] = true;
                this->moves[index] = {place.x - this->previous[nearest].x, place.y - this->previous[nearest].y};
            }
        }
    }

    std::swap(this->previous, this->places);
    this->previousFrame = frameNumber;
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
    this->FollowUnits();
    this->FollowDraws();

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
            int drawnBase = 0;
            int replayBase = 0;
            const std::span<const u16> base = this->record.Base();

            for (std::size_t at = 0; at < drawn.size(); ++at)
            {
                if (drawn[at] != this->scratch[at])
                {
                    drawnBase += drawn[at] == base[at] ? 1 : 0;
                    replayBase += this->scratch[at] == base[at] ? 1 : 0;
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
                u8"smooth battle: frame %d is not given by its %d draws (%d pixels in %d,%d-%d,%d; base %d %d)",
                static_cast<i32>(owner.gameFrames),
                static_cast<i32>(this->record.Draws()),
                count,
                left,
                top,
                right,
                bottom,
                drawnBase,
                replayBase
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
    const double time = static_cast<double>(this->placesFrame) + alpha;
    bool shifted = false;

    for (std::size_t tag = 0; tag < Tags; ++tag)
    {
        const Glide& glide = this->glides[tag];
        const std::array<double, 2> shown = glide.Shown(time);
        this->unitOffsets[tag] = {
            static_cast<int>(std::lround(shown[0])) - glide.toX,
            static_cast<int>(std::lround(shown[1])) - glide.toY,
        };
    }

    const double behind = 1.0 - std::clamp(alpha, 0.0, 1.0);
    this->offsets.resize(this->moves.size());

    for (std::size_t index = 0; index < this->moves.size(); ++index)
    {
        const int tag = this->previous[index].tag;
        std::array<int, 2> offset = {0, 0};

        if (tag > PaintRecord::NoTag && static_cast<std::size_t>(tag) < Tags)
        {
            offset = this->unitOffsets[static_cast<std::size_t>(tag)];
        }

        if (offset[0] == 0 && offset[1] == 0)
        {
            offset = {
                static_cast<int>(std::lround(-this->moves[index][0] * behind)),
                static_cast<int>(std::lround(-this->moves[index][1] * behind)),
            };
        }

        this->offsets[index] = offset;
        shifted = shifted || offset[0] != 0 || offset[1] != 0;
    }

    if (this->screen == nullptr)
    {
        this->screen = std::make_unique<Surface>(Display::Width, Display::Height);
    }

    const int pitch = owner.painter.ScreenPitch();
    this->frame.resize(static_cast<std::size_t>(pitch) * Display::Height);
    this->record.Replay(owner.painter, this->frame, pitch, this->offsets);

    for (int row = 0; row < Display::Height; ++row)
    {
        const u16* line = this->frame.data() + static_cast<std::ptrdiff_t>(row) * pitch;
        std::copy_n(line, Display::Width, this->screen->Row(row));
    }

    const auto took = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()
    );
    ++this->painted;
    this->moved += shifted ? 1 : 0;
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

    if (this->painted != 0)
    {
        constexpr u64 kNanosecondsPerMicrosecond = 1000;
        LogInfo(
            u8"smooth battle: units moved in %d game frames; %d frames drawn between (%d with a unit moved) in %d us "
            u8"on "
            u8"average, %d us at most",
            static_cast<i32>(this->framesMoving),
            static_cast<i32>(this->painted),
            static_cast<i32>(this->moved),
            static_cast<i32>(this->paintNs / this->painted / kNanosecondsPerMicrosecond),
            static_cast<i32>(this->paintMostNs / kNanosecondsPerMicrosecond)
        );
    }
}

}  // namespace swd3
