#include "battle/smooth/smooth_stage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <span>
#include <utility>

#include "battle/battle_hud.h"
#include "battle/battle_services.h"
#include "draw/painter.h"
#include "draw/weave.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3::smooth
{

namespace
{

constexpr int kSnapDistance = 0x80;  // a draw farther than this from its place before jumps
// Match: a move is come over in the time since the draw's last move, at most this long (a move after a longer stand: in
// the time since the frame before).
constexpr u64 kMostSpanNs = 140000000;

// What is left of `lag` coming nearer by `speed` a nanosecond for `gone` nanoseconds (none once it is there).
double LagAt(double lag, double speed, double gone)
{
    const double left = lag - speed * gone;
    return lag == 0.0 || (left > 0.0) != (lag > 0.0) ? 0.0 : left;
}

// The place on the screen of column (row) `offset` of the render scale's blocks of a draw at `place` shown `shift`
// from it: ceil((N x the place - offset) / N), in 64 bits (a place may lie far off).
int BlockPlace(int place, double shift, int scale, int offset)
{
    const i64 at = static_cast<i64>(std::round((place + shift) * scale)) - offset;
    const i64 quotient = at / scale;
    return static_cast<int>(at % scale != 0 && at > 0 ? quotient + 1 : quotient);
}

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

void Stage::Draw(Scene&& decided, int pointerX, int pointerY, u64 now)
{
    this->lastDrawNs = this->drawNs;
    this->drawNs = now;
    this->scene = std::move(decided);
    this->framePointer = {pointerX, pointerY};
    this->drawn.clear();
    this->moves.clear();
    this->measuring = true;
    this->Replay();
    this->measuring = false;
    this->Match();
}

void Stage::PaintBetween(u64 now, Surface& out, int mouseX, int mouseY)
{
    this->paintNs = now;
    this->mouse = {mouseX, mouseY};
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
    std::array<std::vector<i32>, MaxWeaveScale> columnPlaces;
    std::array<std::vector<i32>, MaxWeaveScale> rowPlaces;
    std::array<std::size_t, MaxWeaveScale> columnShown = {};
    std::array<std::size_t, MaxWeaveScale> rowShown = {};

    for (std::size_t at = 0; at < this->moves.size(); ++at)
    {
        const std::array<double, 2> shift = this->Shift(at);

        if (shift[0] == 0.0 && shift[1] == 0.0)
        {
            continue;
        }

        const std::array<int, 2>& place = this->drawn[at].second.at;

        for (std::size_t offset = 0; offset < blocks; ++offset)
        {
            const auto block = static_cast<int>(offset);
            columnPlaces[offset].push_back(BlockPlace(place[0], shift[0], this->scale, block));
            rowPlaces[offset].push_back(BlockPlace(place[1], shift[1], this->scale, block));
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
    this->drawNs = 0;
    this->lastDrawNs = 0;
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
        this->stepStill = step.still;
        this->stepPointer = step.pointer;
        this->stepOwner = step.owner;
        this->stepDraws.clear();
        step.draw(this->services);
    }
}

// Each draw of this frame takes the way back to its place before: in each code with as many draws as before, the
// pairs of a draw and a draw before are taken nearest first (within kSnapDistance), each draw once. Held draws (what
// stands at a place) are measured and moved by the places they show; others by where they are drawn, and only when
// the picture is the same (a new picture is a new pose, drawn where it is).
void Stage::Match()
{
    this->before = std::move(this->places);
    this->places.clear();
    std::map<Key, std::vector<std::size_t>> groups;

    for (std::size_t at = 0; at < this->drawn.size(); ++at)
    {
        const auto& [key, draw] = this->drawn[at];
        this->places[key].push_back(draw);
        groups[key].push_back(at);
    }

    this->moves.assign(this->drawn.size(), std::array<int, 2>{});

    auto point = [](const Drawn& draw, bool held) -> const std::array<int, 2>&
    {
        return held ? draw.stand : draw.at;
    };

    for (const auto& [key, draws] : groups)
    {
        const auto found = this->before.find(key);

        if (found == this->before.end() || found->second.size() != draws.size() ||
            this->drawn[draws.front()].second.still)
        {
            continue;
        }

        const std::vector<Drawn>& earlier = found->second;
        // (distance, draw, draw before)
        std::vector<std::array<int, 3>> pairs;

        for (std::size_t draw = 0; draw < draws.size(); ++draw)
        {
            const Drawn& now = this->drawn[draws[draw]].second;

            for (std::size_t at = 0; at < earlier.size(); ++at)
            {
                const Drawn& then = earlier[at];
                const bool held = now.held && then.held;

                if (!held && now.source != then.source)
                {
                    continue;
                }

                const std::array<int, 2>& from = point(then, held);
                const std::array<int, 2>& to = point(now, held);
                const int far = std::max(std::abs(from[0] - to[0]), std::abs(from[1] - to[1]));

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
        std::vector<bool> beforeTaken(earlier.size());

        for (const auto& [far, draw, at] : pairs)
        {
            if (drawTaken[static_cast<std::size_t>(draw)] || beforeTaken[static_cast<std::size_t>(at)])
            {
                continue;
            }

            drawTaken[static_cast<std::size_t>(draw)] = true;
            beforeTaken[static_cast<std::size_t>(at)] = true;
            const std::size_t index = draws[static_cast<std::size_t>(draw)];
            const Drawn& now = this->drawn[index].second;
            const Drawn& then = earlier[static_cast<std::size_t>(at)];
            const bool held = now.held && then.held;
            const std::array<int, 2>& from = point(then, held);
            const std::array<int, 2>& to = point(now, held);
            this->moves[index] = {from[0] - to[0], from[1] - to[1]};
            this->Follow(
                this->drawn[index].second,
                then,
                this->moves[index],
                this->places[key][static_cast<std::size_t>(draw)]
            );
        }
    }
}

// How `now` is shown after `then`, the same draw a frame before: what `then` had left to come at this frame, and its
// way back to the place before (`way`), which it comes over in the time since its last move (a move after a stand
// longer than kMostSpanNs, or its first, in the time since the frame before); with no move it goes on as it went.
// `kept` (its place for the next frame) takes the same.
void Stage::Follow(Drawn& now, const Drawn& then, const std::array<int, 2>& way, Drawn& kept)
{
    const bool moved = way[0] != 0 || way[1] != 0;
    const double gone = this->drawNs > then.since ? static_cast<double>(this->drawNs - then.since) : 0.0;
    u64 span = this->drawNs > this->lastDrawNs ? this->drawNs - this->lastDrawNs : 1;

    if (moved && then.moved != 0 && this->drawNs > then.moved && this->drawNs - then.moved <= kMostSpanNs)
    {
        span = this->drawNs - then.moved;
    }

    for (std::size_t axis = 0; axis < way.size(); ++axis)
    {
        const double lag = LagAt(then.lag[axis], then.speed[axis], gone) + way[axis];
        now.lag[axis] = lag;
        now.speed[axis] = moved ? lag / static_cast<double>(span) : then.speed[axis];
    }

    now.since = this->drawNs;
    now.moved = moved ? this->drawNs : then.moved;
    kept.lag = now.lag;
    kept.speed = now.speed;
    kept.since = now.since;
    kept.moved = now.moved;
}

void Stage::Hold(bool held, int x, int y, const void* who)
{
    this->holding = held;
    this->held = {x, y};
    this->holdingWho = held ? who : nullptr;
}

void Stage::Place(int& x, int& y, const void* site, const void* source)
{
    const void* who = this->holding ? this->holdingWho : this->stepOwner;
    const std::pair<const void*, const void*> code{site, who};
    auto counted = std::find_if(
        this->stepDraws.begin(),
        this->stepDraws.end(),
        [&code](const std::pair<std::pair<const void*, const void*>, u32>& entry)
        {
            return entry.first == code;
        }
    );

    if (counted == this->stepDraws.end())
    {
        counted = this->stepDraws.insert(this->stepDraws.end(), {code, 0});
    }

    const Key key{this->stepFile, this->stepLine, site, who, counted->second++};

    if (this->measuring)
    {
        this->drawn.emplace_back(
            key,
            Drawn{{x, y}, this->held, this->holding, this->stepStill, this->stepPointer, source}
        );
        return;
    }

    if (this->next >= this->moves.size())
    {
        return;
    }

    const std::array<double, 2> shift = this->Shift(this->next++);

    // A draw that does not move stays where it is drawn at any scale (its place may lie far off the screen).
    if (shift[0] == 0.0 && shift[1] == 0.0)
    {
        return;
    }

    if (this->scale == 1)
    {
        x += static_cast<i32>(std::round(shift[0]));
        y += static_cast<i32>(std::round(shift[1]));
        return;
    }

    x = BlockPlace(x, shift[0], this->scale, this->column);
    y = BlockPlace(y, shift[1], this->scale, this->row);
}

std::array<double, 2> Stage::Shift(std::size_t index) const
{
    if (this->drawn[index].second.pointer)
    {
        return {
            static_cast<double>(this->mouse[0] - this->framePointer[0]),
            static_cast<double>(this->mouse[1] - this->framePointer[1]),
        };
    }

    const Drawn& draw = this->drawn[index].second;
    const double gone = this->paintNs > draw.since ? static_cast<double>(this->paintNs - draw.since) : 0.0;
    return {LagAt(draw.lag[0], draw.speed[0], gone), LagAt(draw.lag[1], draw.speed[1], gone)};
}

}  // namespace swd3::smooth
