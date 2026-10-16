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

// The kinds of the accessories (ObjectData::unk48), named after the mon.dat items of each kind.
enum class AccessoryKind : u16
{
    Unk32 = 0x32,        // no item has it
    ElementOrb = 0x33,   // 地炎玉, 水靈珠, 青龍水珠, ...: an element's resistance lowered by the proficiency
    AttackRing = 0x34,   // 薩登荊環, 星君眼
    BladeReturn = 0x35,  // 返刃玉
    Unk36 = 0x36,        // no item has it
    SpiritDrain = 0x37,  // 吸魔壺
    LifeDrain = 0x38,    // 吸血鬼指環
    SpiritFan = 0x39,    // 靈虛扇
};

// A party role in battle (0x2F34 bytes in the original, ten of them at 0x005029D0): a unit with a battle copy of the
// role's record and the fields past the unit.
class BattleRole : public BattleUnit
{
public:
    // The equipment slots of a role (Party::kSlotsPerRole).
    static constexpr std::size_t kWeaponSlot = 0;
    static constexpr std::size_t kFirstAccessorySlot = 7;
    static constexpr std::size_t kSecondAccessorySlot = 8;
    static constexpr std::size_t kArmourSlots = 7;  // slots below it give the bonus words
    static constexpr std::size_t kNoResistanceSlotA = 9;
    static constexpr std::size_t kNoResistanceSlotB = 0xA;

    BattleRole();
    void SetSkills(ItemNode* list);
    void SetInventory(ItemLink* list);
    void ClearExtras();
    void Setup(RoleRecord& record, const RoleBattleBlock& block, const UnitPlacement& at, ErrorReporter& reporter);
    void SetupGuest(const UnitPlacement& at, const RoleRecord& base, Database& database, ErrorReporter& reporter);
    void
    ApplyEquipment(std::span<const std::unique_ptr<ItemNode>> slots, CounterList& proficiency, ErrorReporter& reporter);
    void ApplyAccessory(const ObjectRecord& accessory, CounterList& proficiency);
    void ShowAllSkills();
    void ShowAllItems();
    void PayStamina();
    void TakeCommand2F18();

    std::unique_ptr<RoleRecord> ownedRecord;  // the 0x38 bytes at +0x00
    ObjectRecord unk2B24;                     // +0x2B24
    std::array<ActState, 5> unk2BC8;          // +0x2BC8
    ItemNode* skills = nullptr;               // +0x2EC0 the first skill shown in the list
    ItemNode* skillsHead = nullptr;           // +0x2EC4 the start of the list (its "無" head)
    ItemLink* inventory = nullptr;            // +0x2EC8 the first item shown
    ItemLink* inventoryHead = nullptr;        // +0x2ECC the party's items
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
