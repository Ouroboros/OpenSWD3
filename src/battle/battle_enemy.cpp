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

    if ((object.kind & BattleEnemy::kNamedKind) != 0)
    {
        object.agility = static_cast<u16>(object.agility + BattleEnemy::kNamedAgility);
        object.attack = static_cast<u16>(object.attack + BattleEnemy::kNamedAttack);
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

    if (this->actId == BattleEnemy::kSatanAct)
    {
        this->hitPointsOverride = BattleEnemy::kSatanHitPoints;
        object.maxHitPoints = BattleEnemy::kSatanHitPoints;
    }

    if (this->actId == BattleEnemy::kChiyouAct)
    {
        this->hitPointsOverride = BattleEnemy::kChiyouHitPoints;
        object.maxHitPoints = BattleEnemy::kChiyouHitPoints;
    }
}

}  // namespace swd3
