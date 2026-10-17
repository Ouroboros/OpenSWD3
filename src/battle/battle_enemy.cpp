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

// swd32.exe 0x00475590: the destructor: the enemy's mon.dat object freed (then the unit's, BattleUnit::~BattleUnit)
// swd32.exe 0x004983B0 (EH unwind: the unit part, by BattleUnit::~BattleUnit)
// swd32.exe 0x004983B8 (EH handler of 0x00475590)
BattleEnemy::~BattleEnemy()
{
    this->Release();
}

// swd32.exe 0x00476A60: the enemy's mon.dat object freed
void BattleEnemy::Release()
{
    this->ownedData.reset();
    this->data = nullptr;
}

// swd32.exe 0x00475720: an enemy entering battle: its placement, its mon.dat object (a named character or boss is
// faster and stronger), its maximum hit points from its hit points, its level byte, its act (the object's, else the
// placement's) and skill script; 撒旦 and 蚩尤 get fixed hit points
void BattleEnemy::Setup(const UnitPlacement& at, u16 id, Database& database)
{
    this->placement = at;
    this->savedPlacement = this->placement;
    this->hitPointsOverride = 0;
    database.LoadObject(*this->data, id);
    ObjectData& object = this->data->data;

    if ((object.kind & BattleEnemy::NamedKind) != 0)
    {
        object.agility = static_cast<u16>(object.agility + BattleEnemy::NamedAgility);
        object.attack = static_cast<u16>(object.attack + BattleEnemy::NamedAttack);
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

    if (this->actId == BattleEnemy::SatanAct)
    {
        this->hitPointsOverride = BattleEnemy::SatanHitPoints;
        object.maxHitPoints = BattleEnemy::SatanHitPoints;
    }

    if (this->actId == BattleEnemy::ChiyouAct)
    {
        this->hitPointsOverride = BattleEnemy::ChiyouHitPoints;
        object.maxHitPoints = BattleEnemy::ChiyouHitPoints;
    }
}

// swd32.exe 0x00475820: the enemy turned into object `id` (its hit points full, its level byte and skill script)
void BattleEnemy::Reload(u16 id, Database& database)
{
    database.LoadObject(*this->data, id);
    ObjectData& object = this->data->data;
    object.maxHitPoints = object.hitPoints;
    this->unk2A93 = object.battleLevel;
    this->LoadCommand(database, object.skill);
    Database::FreeDescription(*this->data);
}

}  // namespace swd3
