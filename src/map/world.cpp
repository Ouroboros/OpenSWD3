#include "map/world.h"

#include <cstring>

namespace swd3
{

// swd32.exe 0x0040DD40
void MoveRecord::Clear()
{
    this->unk000.fill(0xFF);
}

// swd32.exe 0x00425B50: frees the map buffers and the roles' buffers, clears the roles and the leader (to 0xFF) and
// the move records
void World::Clear()
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

}  // namespace swd3
