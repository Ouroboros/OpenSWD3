#pragma once

#include <memory>

#include "battle/battle_unit.h"
#include "party/party.h"

namespace swd3
{

// A party role in battle (0x2F34 bytes in the original, ten of them at 0x005029D0): a unit with a battle copy of the
// role's record and the fields past the unit.
class BattleRole : public BattleUnit
{
public:
    BattleRole();

    std::unique_ptr<RoleRecord> ownedRecord;  // the 0x38 bytes at +0x00
    u16 unk2F18 = 0;
    u16 unk2F26 = 0;
};

}  // namespace swd3
