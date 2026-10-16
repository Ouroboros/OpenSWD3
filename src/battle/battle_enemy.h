#pragma once

#include "battle/battle_unit.h"

namespace swd3
{

// An enemy in battle (0x2B28 bytes in the original, eight of them at 0x00525508): a unit with its own copy of the
// monster's object.
class BattleEnemy : public BattleUnit
{
public:
    // ObjectData::kind bit of the named characters and bosses (all with their own act).
    static constexpr u32 kNamedKind = 0x20;
    // ObjectData::kind bit of the machines (蟹精魔機, 羯忍幻機, ...).
    static constexpr u32 kMachineKind = 0x800;
    static constexpr u16 kSmallMachineTypes = 0x15;
    static constexpr u16 kNamedAgility = 6;
    static constexpr u16 kNamedAttack = 0xA;
    // The acts of 撒旦 (mon.dat 0x1C) and 蚩尤 (0x2E) and the hit points they are given.
    static constexpr u16 kSatanAct = 0x1C;
    static constexpr i32 kSatanHitPoints = 41000;
    static constexpr u16 kChiyouAct = 0x2E;
    static constexpr i32 kChiyouHitPoints = 120000;

    void Setup(const UnitPlacement& at, u16 id, Database& database);
    void Reload(u16 id, Database& database);
    BattleEnemy();
};

}  // namespace swd3
