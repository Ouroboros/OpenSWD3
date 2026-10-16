#include "game/game_state.h"

#include "game/direction.h"

namespace swd3
{

// swd32.exe 0x0040DB40
int ClickRegions::Count() const
{
    return static_cast<int>(this->regions.size());
}

// swd32.exe 0x0040DB60: the index of the first region strictly containing the pointer (the count when none does)
int ClickRegions::Find(int pointerX, int pointerY, ClickRegion*& found)
{
    int index = 0;
    found = nullptr;

    for (ClickRegion& region : this->regions)
    {
        if (static_cast<u32>(pointerX) > region.left && static_cast<u32>(pointerX) < region.right &&
            static_cast<u32>(pointerY) > region.top && static_cast<u32>(pointerY) < region.bottom)
        {
            found = &region;
            break;
        }

        ++index;
    }

    return index;
}

// swd32.exe 0x0040DBC0
void ClickRegions::Free()
{
    this->regions.clear();
    this->flags = 0;
    this->state = {};
}

// swd32.exe 0x0040DC50
bool GameState::GameFlag(int flag) const
{
    return (this->flags[static_cast<std::size_t>(flag >> 3)] & GameState::kBitMasks[flag & 7]) != 0;
}

// swd32.exe 0x0040DC80
u8 GameState::SetGameFlag(int flag)
{
    u8& byte = this->flags[static_cast<std::size_t>(flag >> 3)];
    byte = static_cast<u8>(byte | GameState::kBitMasks[flag & 7]);
    return byte;
}

// swd32.exe 0x0040DCB0
u8 GameState::ClearGameFlag(int flag)
{
    u8& byte = this->flags[static_cast<std::size_t>(flag >> 3)];
    byte = static_cast<u8>(byte & (0xFF - GameState::kBitMasks[flag & 7]));
    return byte;
}

// swd32.exe 0x0040DCE0
u8 GameState::SetGameFlagTo(int flag, int value)
{
    return value == 1 ? this->SetGameFlag(flag) : this->ClearGameFlag(flag);
}

// swd32.exe 0x0040DD10
void GameState::SetWalkSpeed(int speed)
{
    this->walkSpeed = speed;
}

// swd32.exe 0x0040DD20: always 1
int GameState::SetFrameInterval(int value)
{
    this->frameInterval = value;
    return 1;
}

// swd32.exe 0x0040DD30
void GameState::ClearFrameInterval()
{
    this->frameInterval = 0;
}

// swd32.exe 0x0040E030: the direction code from the centre of the controlled role towards (x, y)
int GameState::DirectionToPlayer(int x, int y) const
{
    const Role& role = this->Player();
    i32 distance = 0;
    const int angle = AngleBetween(role.x + 8 * role.act.width, role.y + 8 * role.act.height, x, y, distance);
    return AngleToDirection(angle);
}

Role& GameState::Player()
{
    return this->roles[static_cast<std::size_t>(this->playerRole)];
}

const Role& GameState::Player() const
{
    return this->roles[static_cast<std::size_t>(this->playerRole)];
}

}  // namespace swd3
