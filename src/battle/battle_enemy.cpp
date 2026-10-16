#include "battle/battle_enemy.h"

#include <memory>

namespace swd3
{

// swd32.exe 0x00475560
BattleEnemy::BattleEnemy()
{
    this->ownedData = std::make_unique<ObjectRecord>();
    this->data = this->ownedData.get();
}

}  // namespace swd3
