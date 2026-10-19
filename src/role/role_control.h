#pragma once

#include <array>
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
    void ControlPlayer();

    // swd32.exe 0x004993C0: the run toggle.
    static constexpr std::array<i32, 3> RunToggle = {1, 0, 0};
    // swd32.exe 0x004993CC and 0x004993EC: the tile in front of a role by its act direction.
    static constexpr std::array<i32, 8> AheadX = {0, 0, -1, 1, -1, 1, -1, 1};
    static constexpr std::array<i32, 8> AheadY = {-1, 1, 0, 0, -1, 1, 1, -1};

    i32 running = 0;      // 0x004BA428 the run toggle (key slot 16)
    i32 scrollLeft = 0;   // 0x004CB210 the camera can follow a step left
    i32 scrollRight = 0;  // 0x004CB20C
    i32 scrollUp = 0;     // 0x004CB21C
    i32 scrollDown = 0;   // 0x004CB218
    i32 stepLeft = 0;     // 0x004CB22C the role alone can step left
    i32 stepRight = 0;    // 0x004CB228
    i32 stepUp = 0;       // 0x004CB238
    i32 stepDown = 0;     // 0x004CB230
    i32 choiceMoved = 0;  // 0x004CB214
    i32 idleFrames = 0;   // 0x004CB234 frames standing (idle action 0x34 after 0x80)
    i32 unk4CB220 = 0;    // frames since a debug key

private:
    u32 Cell(std::ptrdiff_t index) const;
    void Approach(Role& player, Role& other, u32 index, bool walking);

    Game& game;
};

}  // namespace swd3
