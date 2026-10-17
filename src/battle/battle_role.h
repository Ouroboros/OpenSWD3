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
    Steal = 0x17,        // 神偷面具, 乾坤袋
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
    // The equipment slots of a role (Party::SlotsPerRole).
    static constexpr std::size_t WeaponSlot = 0;
    static constexpr std::size_t FirstAccessorySlot = 7;
    static constexpr std::size_t SecondAccessorySlot = 8;
    static constexpr std::size_t ArmourSlots = 7;  // slots below it give the bonus words
    static constexpr std::size_t NoResistanceSlotA = 9;
    static constexpr std::size_t NoResistanceSlotB = 0xA;

    static constexpr std::size_t ExtraActs = 5;

    BattleRole();
    void SetSkills(ItemNode* list);
    void SetInventory(ItemLink* list);
    void ClearExtras();
    void Setup(RoleRecord& record, const RoleBattleBlock& block, const UnitPlacement& at, ErrorReporter& reporter);
    void SetupEscort(const UnitPlacement& at, Database& database, ErrorReporter& reporter);
    void Strike(BattleServices& services, BattleUnit& target, int evadable);
    int AttackFrame(BattleServices& services, BattleUnit& target, int slot);
    void LoadItemAction(Database& database, u16 id, u32& act);
    void StartAbsorb(BattleServices& services, BattleUnit& source);
    void FlyExtraAct(BattleServices& services, i32 id, i32 action, int slot, i16 x, i16 y, int steps);
    int AbsorbFrame(BattleServices& services);
    int ThrowAct(BattleServices& services, const BattleUnit& target);
    int CatchAct(BattleServices& services, const BattleUnit& target);
    int GlowAct(BattleServices& services, int x, int y);
    int PoseAtTarget(BattleServices& services, const BattleUnit& target);
    u16 Steal(BattleServices& services, const BattleUnit& target);
    u16 AttackPose(BattleServices& services);
    static int CanTarget(const BattleUnit& target);
    int CanAbsorb(BattleServices& services, const BattleUnit& target) const;
    int DirectAttack(BattleServices& services, BattleUnit& target);
    int CastAt(BattleServices& services, const BattleUnit& target, i32 spell);
    void SetupGuest(const UnitPlacement& at, const RoleRecord& base, Database& database, ErrorReporter& reporter);
    void
    ApplyEquipment(std::span<const std::unique_ptr<ItemNode>> slots, CounterList& proficiency, ErrorReporter& reporter);
    void ApplyAccessory(const ObjectRecord& accessory, CounterList& proficiency);
    void ShowAllSkills();
    void ShowAllItems();
    void PayStamina();
    void TakeCommand2F18();

    std::unique_ptr<RoleRecord> ownedRecord;     // the 0x38 bytes at +0x00
    ObjectRecord unk2B24;                        // +0x2B24
    std::array<ActState, ExtraActs> extraActs;   // +0x2BC8 the sparks of an absorb (FlyExtraAct)
    ItemNode* skills = nullptr;                  // +0x2EC0 the first skill shown in the list
    ItemNode* skillsHead = nullptr;              // +0x2EC4 the start of the list (its "無" head)
    ItemLink* inventory = nullptr;               // +0x2EC8 the first item shown
    ItemLink* inventoryHead = nullptr;           // +0x2ECC the party's items
    std::array<i32, ExtraActs> extraSteps = {};  // +0x2EF8 the line steps of each extra act this frame
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
    i16 absorbFrames = 0;  // +0x2F26 the frames of the running absorb (AbsorbFrame)
    u8 critical = 0;       // +0x2F30 1: the next attack is critical (set by 0x0046FF00)
};

}  // namespace swd3
