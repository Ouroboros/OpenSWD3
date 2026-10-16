#pragma once

#include "battle/battle_unit.h"

namespace swd3
{

// An enemy in battle (0x2B28 bytes in the original, eight of them at 0x00525508): a unit with its own copy of the
// monster's object.
class BattleEnemy : public BattleUnit
{
public:
    void Setup(const UnitPlacement& at, u16 id, Database& database);
    BattleEnemy();
};

}  // namespace swd3
