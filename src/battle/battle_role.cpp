#include "battle/battle_role.h"

namespace swd3
{

// swd32.exe 0x0046E490
BattleRole::BattleRole() : ownedRecord(std::make_unique<RoleRecord>())
{
    this->battleRecord = this->ownedRecord.get();
}

}  // namespace swd3
