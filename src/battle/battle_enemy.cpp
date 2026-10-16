#include "battle/battle_enemy.h"

#include <memory>

#include "org/database.h"

namespace swd3
{

// swd32.exe 0x00475560
BattleEnemy::BattleEnemy()
{
    this->ownedData = std::make_unique<ObjectRecord>();
    this->data = this->ownedData.get();
}

// swd32.exe 0x00475720: an enemy entering battle: its placement, its mon.dat object (a kind with bit 5 is 6 faster and
// 10 stronger), its maximum hit points from its hit points, its level byte, its act (the object's, else the
// placement's) and skill script; acts 0x1C and 0x2E get fixed hit points (41000 and 120000)
void BattleEnemy::Setup(const UnitPlacement& at, u16 id, Database& database)
{
    this->placement = at;
    this->savedPlacement = this->placement;
    this->hitPointsOverride = 0;
    database.LoadObject(*this->data, id);
    ObjectData& object = this->data->data;

    if ((object.kind & 0x20) != 0)
    {
        object.agility = static_cast<u16>(object.agility + 6);
        object.attack = static_cast<u16>(object.attack + 0xA);
    }

    this->kind = at.kind;
    object.maxHitPoints = object.hitPoints;
    this->unk2A93 = object.battleLevel;
    this->actId = object.unk50;

    if (this->actId == 0)
    {
        this->actId = at.actId;
    }

    this->unk2684 = 0;
    this->LoadCommand(database, object.skill);
    Database::FreeDescription(*this->data);

    if (this->actId == 0x1C)
    {
        this->hitPointsOverride = 0xA028;
        object.maxHitPoints = 0xA028;
    }

    if (this->actId == 0x2E)
    {
        this->hitPointsOverride = 0x1D4C0;
        object.maxHitPoints = 0x1D4C0;
    }
}

}  // namespace swd3
