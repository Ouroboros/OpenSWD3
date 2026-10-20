#include "battle/smooth/smooth_stage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <span>
#include <utility>

#include "battle/battle_hud.h"
#include "battle/battle_services.h"
#include "core/divide.h"
#include "draw/painter.h"
#include "draw/weave.h"
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

Stage::Stage(BattleServices& services)
    : services(services)
{
}

void Stage::Draw(Scene&& decided)
{
    this->scene = std::move(decided);
    this->drawn.clear();
    this->moves.clear();
    this->measuring = true;
    this->Replay();
    this->measuring = false;
    this->Match();
}

void Stage::PaintBetween(double alpha, Surface& out)
{
    this->share = std::clamp(alpha, 0.0, 1.0);
    this->scale = std::clamp(out.Width() / Display::Width, 1, MaxWeaveScale);
    this->column = 0;
    this->row = 0;

    if (this->scale == 1)
    {
        this->DrawInto(out);
        return;
    }

    // Each screen of the blocks: column a and row b show a place p (in 1/N pixels) at ceil((p - a) / N). Columns that
    // place every moving draw alike (and rows alike) show the same screen, which is drawn once.
    const auto blocks = static_cast<std::size_t>(this->scale);
    const double back = 1.0 - this->share;
    std::array<std::vector<i32>, MaxWeaveScale> columnPlaces;
    std::array<std::vector<i32>, MaxWeaveScale> rowPlaces;
    std::array<std::size_t, MaxWeaveScale> columnShown = {};
    std::array<std::size_t, MaxWeaveScale> rowShown = {};

    for (std::size_t at = 0; at < this->moves.size(); ++at)
    {
        const std::array<int, 2>& move = this->moves[at];

        if (move[0] == 0 && move[1] == 0)
        {
            continue;
        }

        const std::array<int, 2>& place = this->drawn[at].second;
        const auto fineX = static_cast<i32>(std::lround((place[0] + move[0] * back) * this->scale));
        const auto fineY = static_cast<i32>(std::lround((place[1] + move[1] * back) * this->scale));

        for (std::size_t offset = 0; offset < blocks; ++offset)
        {
            columnPlaces[offset].push_back(CeilDivide(fineX - static_cast<i32>(offset), this->scale));
            rowPlaces[offset].push_back(CeilDivide(fineY - static_cast<i32>(offset), this->scale));
        }
    }

    for (std::size_t offset = 0; offset < blocks; ++offset)
    {
        columnShown[offset] = offset;
        rowShown[offset] = offset;

        for (std::size_t earlier = 0; earlier < offset; ++earlier)
        {
            if (columnShown[offset] == offset && columnPlaces[earlier] == columnPlaces[offset])
            {
                columnShown[offset] = earlier;
            }

            if (rowShown[offset] == offset && rowPlaces[earlier] == rowPlaces[offset])
            {
                rowShown[offset] = earlier;
            }
        }
    }

    this->parts.resize(blocks * blocks);

    for (std::size_t block = 0; block < blocks * blocks; ++block)
    {
        const std::size_t partRow = block / blocks;
        const std::size_t partColumn = block % blocks;

        if (rowShown[partRow] != partRow || columnShown[partColumn] != partColumn)
        {
            continue;
        }

        Surface& part = this->parts[block];

        if (part.Width() != Display::Width || part.Height() != Display::Height)
        {
            part.Resize(Display::Width, Display::Height);
        }

        this->column = static_cast<int>(partColumn);
        this->row = static_cast<int>(partRow);
        this->DrawInto(part);
    }

    std::array<const Surface*, MaxWeaveScale * MaxWeaveScale> shown = {};

    for (std::size_t block = 0; block < blocks * blocks; ++block)
    {
        shown[block] = &this->parts[rowShown[block / blocks] * blocks + columnShown[block % blocks]];
    }

    Weave(
        std::span<const Surface* const>(shown.data(), blocks * blocks),
        this->scale,
        std::span<Pixel>(out.Pixels(), static_cast<std::size_t>(out.Width()) * static_cast<std::size_t>(out.Height()))
    );
    this->scale = 1;
}

void Stage::Clear()
{
    this->scene = Scene{};
    this->before.clear();
    this->places.clear();
    this->drawn.clear();
    this->moves.clear();
}

void Stage::DrawInto(Surface& target)
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

void Stage::Replay()
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
void Stage::Match()
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

void Stage::Place(int& x, int& y)
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

    if (this->scale == 1)
    {
        x += static_cast<int>(std::lround(move[0] * back));
        y += static_cast<int>(std::lround(move[1] * back));
        return;
    }

    const auto fineX = static_cast<i32>(std::lround((x + move[0] * back) * this->scale));
    const auto fineY = static_cast<i32>(std::lround((y + move[1] * back) * this->scale));
    x = CeilDivide(fineX - this->column, this->scale);
    y = CeilDivide(fineY - this->row, this->scale);
}

}  // namespace swd3::smooth
