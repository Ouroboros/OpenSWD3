#include "battle/smooth/smooth_play.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "battle/battle_hud.h"
#include "battle/battle_services.h"
#include "draw/painter.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3::smooth
{

namespace
{

constexpr int kSnapDistance = 0x80;  // a draw farther than this from its place before jumps

// The HUD's number and digit state, which the draws of a frame drawn again set as they draw.
struct HudKept
{
    i32 numberX = 0;
    i32 numberY = 0;
    i32 numberStarted = 0;
    i32 numberValue = 0;
    const Picture* lastPicture = nullptr;
    u32 numberFont = 0;

    explicit HudKept(const BattleHud& hud)
        : numberX(hud.numberX),
          numberY(hud.numberY),
          numberStarted(hud.numberStarted),
          numberValue(hud.numberValue),
          lastPicture(hud.lastPicture),
          numberFont(hud.numberFont)
    {
    }

    void Restore(BattleHud& hud) const
    {
        hud.numberX = this->numberX;
        hud.numberY = this->numberY;
        hud.numberStarted = this->numberStarted;
        hud.numberValue = this->numberValue;
        hud.lastPicture = this->lastPicture;
        hud.numberFont = this->numberFont;
    }
};

}  // namespace

BattlePlayer::BattlePlayer(BattleServices& services)
    : services(services)
{
}

void BattlePlayer::Show(Scene&& decided)
{
    this->scene = std::move(decided);

    if (this->screen == nullptr)
    {
        this->screen = std::make_unique<Surface>(Display::Width, Display::Height);
    }

    this->drawn.clear();
    this->moves.clear();
    this->measuring = true;
    this->DrawInto(*this->screen);
    this->measuring = false;
    this->Match();
    this->share = 0.0;
    this->Replay();
}

Surface& BattlePlayer::PaintBetween(double alpha)
{
    if (this->screen == nullptr)
    {
        this->screen = std::make_unique<Surface>(Display::Width, Display::Height);
    }

    this->share = std::clamp(alpha, 0.0, 1.0);
    this->DrawInto(*this->screen);
    return *this->screen;
}

void BattlePlayer::Clear()
{
    this->scene = Scene{};
    this->before.clear();
    this->places.clear();
    this->drawn.clear();
    this->moves.clear();
}

void BattlePlayer::DrawInto(Surface& target)
{
    BattleServices& battle = this->services;
    Painter& painter = battle.painter;
    const Painter::DrawState kept = painter.SaveDrawState();
    const HudKept hud(battle.hud);
    u16* const frame = battle.screen;
    const int pitch = painter.ScreenPitch();

    for (int row = 0; row < Display::Height; ++row)
    {
        std::copy_n(frame + static_cast<std::ptrdiff_t>(row) * pitch, Display::Width, target.Row(row));
    }

    battle.screen = painter.LockSurface(target);
    painter.SetTarget(battle.screen);
    painter.UnlockSurface(target);
    this->Replay();
    battle.screen = frame;
    painter.RestoreDrawState(kept);
    hud.Restore(battle.hud);
}

void BattlePlayer::Replay()
{
    const DrawPlacer::Scope placing(this);
    this->next = 0;

    for (const Scene::Step& step : this->scene.steps)
    {
        this->stepFile = step.file;
        this->stepLine = step.line;
        this->stepDraws = 0;
        step.draw(this->services);
    }
}

// Each draw of this frame takes the way back to its place before: in each code with as many draws as before, the
// pairs of a draw and a place before are taken nearest first (within kSnapDistance), each draw and place once.
void BattlePlayer::Match()
{
    this->before = std::move(this->places);
    this->places.clear();
    std::map<Key, std::vector<std::size_t>> groups;

    for (std::size_t at = 0; at < this->drawn.size(); ++at)
    {
        const auto& [key, place] = this->drawn[at];
        this->places[key].push_back(place);
        groups[key].push_back(at);
    }

    this->moves.assign(this->drawn.size(), std::array<int, 2>{});

    for (const auto& [key, draws] : groups)
    {
        const auto found = this->before.find(key);

        if (found == this->before.end() || found->second.size() != draws.size())
        {
            continue;
        }

        const std::vector<std::array<int, 2>>& places = found->second;
        // (distance, draw, place before)
        std::vector<std::array<int, 3>> pairs;

        for (std::size_t draw = 0; draw < draws.size(); ++draw)
        {
            const std::array<int, 2>& place = this->drawn[draws[draw]].second;

            for (std::size_t at = 0; at < places.size(); ++at)
            {
                const int far = std::max(std::abs(places[at][0] - place[0]), std::abs(places[at][1] - place[1]));

                if (far <= kSnapDistance)
                {
                    pairs.push_back({far, static_cast<int>(draw), static_cast<int>(at)});
                }
            }
        }

        std::stable_sort(
            pairs.begin(),
            pairs.end(),
            [](const std::array<int, 3>& left, const std::array<int, 3>& right)
            {
                return left[0] < right[0];
            }
        );
        std::vector<bool> drawTaken(draws.size());
        std::vector<bool> placeTaken(places.size());

        for (const auto& [far, draw, at] : pairs)
        {
            if (drawTaken[static_cast<std::size_t>(draw)] || placeTaken[static_cast<std::size_t>(at)])
            {
                continue;
            }

            drawTaken[static_cast<std::size_t>(draw)] = true;
            placeTaken[static_cast<std::size_t>(at)] = true;
            const std::size_t index = draws[static_cast<std::size_t>(draw)];
            const std::array<int, 2>& place = this->drawn[index].second;
            this->moves[index] = {
                places[static_cast<std::size_t>(at)][0] - place[0],
                places[static_cast<std::size_t>(at)][1] - place[1]
            };
        }
    }
}

void BattlePlayer::Place(int& x, int& y)
{
    const Key key{this->stepFile, this->stepLine, this->stepDraws++};

    if (this->measuring)
    {
        this->drawn.emplace_back(key, std::array<int, 2>{x, y});
        return;
    }

    if (this->next >= this->moves.size())
    {
        return;
    }

    const std::array<int, 2>& move = this->moves[this->next++];
    const double back = 1.0 - this->share;
    x += static_cast<int>(std::lround(move[0] * back));
    y += static_cast<int>(std::lround(move[1] * back));
}

}  // namespace swd3::smooth
