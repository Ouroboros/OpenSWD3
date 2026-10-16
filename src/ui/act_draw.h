#pragma once

#include "core/types.h"

namespace swd3
{

class ActArchive;
class Painter;
class TswArchive;
struct ActState;

// One frame of an action script drawn at a point: the shared helpers of the screens and menus.
class ActDrawer
{
public:
    static constexpr int GoldX = 0x27C;  // the gold counter at the bottom right
    static constexpr int GoldY = 0x1CC;

    static constexpr u16 Digits = 0x2354;
    static constexpr u16 CounterLabel = 0x245E;

    ActDrawer(ActArchive& acts, TswArchive& archive, Painter& painter);

    void Draw(ActState& state, int x, int y);
    void DrawGlow(ActState& state, int x, int y, int level);
    void DrawCounter(int right, int y, u32 value, int alpha);
    void DrawGhost(ActState& state, int x, int y, int alpha);
    void DrawWithMode(ActState& state, int x, int y, u32 mode);

private:
    ActArchive& acts;
    TswArchive& archive;
    Painter& painter;
};

}  // namespace swd3
