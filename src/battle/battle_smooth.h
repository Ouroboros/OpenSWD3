#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "battle/battle_state.h"
#include "core/glide.h"
#include "core/types.h"
#include "draw/paint_record.h"
#include "platform/surface.h"

namespace swd3
{

class Game;

// The rebuild's smooth battle (deviations.md): the draws of a battle frame are recorded, each unit's under its tag
// (the unit plus one), and drawn again between game frames: the draws of a unit that moved slide with its glide, and
// a draw found again in the frame before (the same picture, text or box, the nearest) slides from where that one was.
// A frame is drawn again only when its record, drawn unmoved, gives the frame drawn.
class BattleSmooth
{
public:
    static constexpr std::size_t Tags = BattleState::FirstRoleUnit + BattleState::RoleUnits + 1;

    explicit BattleSmooth(Game& game);

    // The drawing of a battle frame starts (recorded with smooth motion), and is given up.
    void Begin();
    void Cancel();
    // The draws that follow are unit `unit`'s (a value of BattleState::drawOrder), or no unit's (-1).
    void Unit(int unit);
    // The drawing ends: the places of the units followed and the record checked; whether frames can be drawn
    // between this one and the next.
    bool End();
    // The frame drawn again at `alpha` of the way to the next game frame.
    Surface& PaintBetween(double alpha);
    void LogTotals() const;

private:
    void FollowUnits();
    void FollowDraws();

    Game& game;
    PaintRecord record;
    std::vector<u16> scratch;
    std::vector<u16> frame;
    std::unique_ptr<Surface> screen;
    std::array<Glide, Tags> glides{};
    std::array<u32, Tags> seen{};  // the game frame that last followed each unit
    std::array<std::array<int, 2>, Tags> unitOffsets{};
    std::vector<PaintRecord::Place> places;  // the draws of this frame and the one before
    std::vector<PaintRecord::Place> previous;
    u32 previousFrame = 0;
    std::vector<std::array<int, 2>> moves;    // how far each draw moved since the frame before
    std::vector<std::array<int, 2>> offsets;  // where each draw is drawn between, from where it was drawn
    u32 placesFrame = 0;
    bool placesValid = false;
    bool started = false;
    // The frames recorded, drawn again between game frames, and given up for a draw not recorded or a record that
    // does not give the frame drawn.
    u32 recorded = 0;
    u32 smoothed = 0;
    u32 incomplete = 0;
    u32 mismatched = 0;
    // The frames drawn between, those with a unit moved, and the time they took (in all and the longest).
    u32 painted = 0;
    u32 moved = 0;
    u32 framesMoving = 0;  // the game frames a unit moved in
    u64 paintNs = 0;
    u64 paintMostNs = 0;
};

}  // namespace swd3
