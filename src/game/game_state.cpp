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

// swd32.exe 0x0040DD40
void MoveRecord::Clear()
{
    this->role = 0xFFFF;
    this->step = 0xFFFF;
    this->x = 0xFFFF;
    this->y = 0xFFFF;
    this->nextRole = 0xFFFF;
    this->unk0A = 0xFFFF;
    this->nextX = 0xFFFF;
    this->nextY = 0xFFFF;
    this->act = -1;
    this->action = -1;
    this->direction = -1;
    this->stepX = -1;
    this->stepY = -1;
    this->unk1A = 0xFF;
    this->mode = 0xFF;
    this->directions.fill(0xFF);
}

// swd32.exe 0x00425B50: frees the map buffers and the roles' buffers, clears the roles, fills the leader with 0xFF and
// clears the move records
void GameState::ClearWorld()
{
    this->map = MapHeader{};
    this->roleCount = 0;

    for (Role& role : this->roles)
    {
        role = Role{};
    }

    this->leader = Role{};
    this->leader.FillUnused();

    for (MoveRecord& move : this->moves)
    {
        move.Clear();
    }
}

// swd32.exe 0x0040F3B0: the buffers of the roles in use freed, every role cleared with its action playback reset
void GameState::ClearRoles()
{
    for (int index = 0; index <= this->roleCount && index < GameState::kRoles; ++index)
    {
        this->roles[static_cast<std::size_t>(index)].label.clear();
    }

    for (Role& role : this->roles)
    {
        role = Role{};
        ResetPlayback(role.act);
    }
}

// swd32.exe 0x0040C020: the first role in use with `id` that is not hidden (flag 0x10000000); 0xFFFFFFFF when none
u32 GameState::FindRole(u16 id) const
{
    if (this->roleCount == 0)
    {
        return 0xFFFFFFFF;
    }

    u32 index = 0;

    while (this->roles[index].id != id || (this->roles[index].flags & 0x10000000) != 0)
    {
        if (++index >= static_cast<u32>(this->roleCount) || index >= this->roles.size())
        {
            return 0xFFFFFFFF;
        }
    }

    return index;
}

// swd32.exe 0x0040C100
bool GameState::FindRole(u16 id, u32& index) const
{
    index = this->FindRole(id);
    return index != 0xFFFFFFFF;
}

// swd32.exe 0x0040C0D0: id 0xFFFE stands for the controlled role
bool GameState::FindRoleOrPlayer(u16 id, u32& index) const
{
    index = 0;

    if (id != 0xFFFE)
    {
        return this->FindRole(id, index);
    }

    index = static_cast<u32>(this->playerRole);
    return true;
}

}  // namespace swd3
