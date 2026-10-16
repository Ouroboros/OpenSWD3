#pragma once

#include <array>
#include <memory>
#include <span>

#include "battle/battle_unit.h"
#include "item/item_list.h"
#include "org/fame.h"
#include "party/party.h"

namespace swd3
{

// A party role in battle (0x2F34 bytes in the original, ten of them at 0x005029D0): a unit with a battle copy of the
// role's record and the fields past the unit.
class BattleRole : public BattleUnit
{
public:
    BattleRole();
    void SetSkills(ItemNode* list);
    void SetInventory(ItemLink* list);
    void ClearExtras();
    void Setup(RoleRecord& record, const RoleBattleBlock& block, const UnitPlacement& at, ErrorReporter& reporter);
    void SetupGuest(const UnitPlacement& at, const RoleRecord& base, Database& database, ErrorReporter& reporter);
    void
    ApplyEquipment(std::span<const std::unique_ptr<ItemNode>> slots, CounterList& proficiency, ErrorReporter& reporter);
    void ApplyAccessory(const ObjectRecord& accessory, CounterList& proficiency);

    std::unique_ptr<RoleRecord> ownedRecord;  // the 0x38 bytes at +0x00
    ObjectRecord unk2B24;                     // +0x2B24
    std::array<ActState, 5> unk2BC8;          // +0x2BC8
    ItemNode* skills = nullptr;               // +0x2EC0 the role's skill list (its "無" head)
    ItemNode* skillsShown = nullptr;          // +0x2EC4
    ItemLink* inventory = nullptr;            // +0x2EC8 the party's items
    ItemLink* inventoryShown = nullptr;       // +0x2ECC
    i32 unk2F0C = 0;
    u16 unk2F10 = 0;
    u16 unk2F12 = 0;
    u16 unk2F14 = 0;
    u16 unk2F16 = 0;
    u16 unk2F18 = 0;
    u16 weaponId = 0;  // +0x2F1A
    u16 unk2F1C = 0;
    u16 bonusA = 0;  // +0x2F1E the sum of the +0x34 words of the armour (slots 1..6 without kind)
    u16 bonusB = 0;  // +0x2F20 of their +0x36 words
    u16 bonusC = 0;  // +0x2F22 of their +0x38 words
    u16 unk2F24 = 0;
    u16 unk2F26 = 0;
};

}  // namespace swd3
