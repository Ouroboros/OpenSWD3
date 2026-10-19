#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/types.h"
#include "draw/paint_record.h"
#include "platform/surface.h"

namespace swd3
{

class Game;

// The rebuild's smooth battle (deviations.md): the draws of a battle frame are recorded and drawn again between game
// frames, each draw made under a PaintRecord::Motion (the battle code reckoning where the next frame draws it by the
// rules of its own motion) that far along its way; the other draws stay. A frame is drawn again only when its record,
// drawn unmoved, gives the frame drawn.
class BattleSmooth
{
public:
    explicit BattleSmooth(Game& game);

    // The drawing of a battle frame starts (recorded with smooth motion), and is given up.
    void Begin();
    void Cancel();
    // The drawing ends and the record is checked: whether frames can be drawn between this one and the next.
    bool End();
    // The frame drawn again at `alpha` of the way to the next game frame.
    Surface& PaintBetween(double alpha);
    void LogTotals() const;

private:
    Game& game;
    PaintRecord record;
    std::vector<u16> scratch;
    std::vector<u16> frame;
    std::unique_ptr<Surface> screen;
    bool started = false;
    // The frames recorded, drawn again between game frames, and given up for a draw not recorded or a record that
    // does not give the frame drawn; the draws that moved.
    u32 recorded = 0;
    u32 smoothed = 0;
    u32 incomplete = 0;
    u32 mismatched = 0;
    u64 drawsMoving = 0;
    // The frames drawn between and the time they took (in all and the longest).
    u32 painted = 0;
    u64 paintNs = 0;
    u64 paintMostNs = 0;
};

}  // namespace swd3
