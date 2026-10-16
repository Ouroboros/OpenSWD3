#pragma once

#include <cstddef>

#include "core/types.h"

namespace swd3
{

class Game;
struct Role;

// The walking of the controlled role (0x004040B0..0x00405100): the free lanes beside it, sliding along walls, the
// roles and map events in front of it and the roles it touches.
class RoleControl
{
public:
    explicit RoleControl(Game& game);

    u32 FreeLanes(int x, int y, int length, int stride, int limit, int axis) const;
    void Slide(Role& role, int& stepX, int& stepY) const;
    u32 FindRoleAt(u32 x, u32 y) const;
    u32 FindTouchRoleAt(u32 x, u32 y) const;
    u32 StepEvent(int index, int stepX, int stepY, u32& role) const;
    u32 FindRoleAhead(int index, int stepX, int stepY) const;

private:
    u32 Cell(std::ptrdiff_t index) const;

    Game& game;
};

}  // namespace swd3
