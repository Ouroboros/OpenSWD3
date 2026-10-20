#include "draw/draw_placer.h"

namespace swd3
{

DrawPlacer* DrawPlacer::active = nullptr;

DrawPlacer* DrawPlacer::Active()
{
    return DrawPlacer::active;
}

DrawPlacer::Scope::Scope(DrawPlacer* placer)
    : outer(DrawPlacer::active)
{
    DrawPlacer::active = placer;
}

DrawPlacer::Scope::~Scope()
{
    DrawPlacer::active = this->outer;
}

DrawPlacer::Holding::Holding(int x, int y, const void* who)
{
    if (DrawPlacer::active != nullptr)
    {
        DrawPlacer::active->Hold(true, x, y, who);
    }
}

DrawPlacer::Holding::~Holding()
{
    if (DrawPlacer::active != nullptr)
    {
        DrawPlacer::active->Hold(false, 0, 0, nullptr);
    }
}

}  // namespace swd3
