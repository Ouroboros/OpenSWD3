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

}  // namespace swd3
