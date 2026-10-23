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

// The kinds of the accessories (ObjectData::actionKind), named after the mon.dat items of each kind.
enum class AccessoryKind : u16
{
    RepeatAttack = 0x15,  // 武聖手鐲, 姜公膽: the attack repeats once
    Unk16 = 0x16,         // no item has it
    Steal = 0x17,         // 神偷面具, 乾坤袋
    Unk18 = 0x18,         // 武神魂玉
    Unk19 = 0x19,         // no item has it
    Unk1A = 0x1A,         // no item has it
    Unk1B = 0x1B,         // 暗殺寶錄, 御夫錄
    Unk1C = 0x1C,         // 硃砂辟魔玉
    Unk1D = 0x1D,         // no item has it
    Unk1E = 0x1E,         // 硃砂神筆
    Unk1F = 0x1F,         // 鼠槌
    Unk20 = 0x20,         // 金鐘罩秘笈
    Unk21 = 0x21,         // 庖丁菜刀
    Unk22 = 0x22,         // no item has it
    Unk23 = 0x23,         // 夜叉肩帶
    Unk24 = 0x24,         // no item has it
    Unk32 = 0x32,         // no item has it
    ElementOrb = 0x33,    // 地炎玉, 水靈珠, 青龍水珠, ...: an element's resistance lowered by the proficiency
    AttackRing = 0x34,    // 薩登荊環, 星君眼
    BladeReturn = 0x35,   // 返刃玉
    Unk36 = 0x36,         // no item has it
    SpiritDrain = 0x37,   // 吸魔壺
    LifeDrain = 0x38,     // 吸血鬼指環
    SpiritFan = 0x39,     // 靈虛扇
};

// A party role in battle (0x2F34 bytes in the original, ten of them at 0x005029D0): a unit with a battle copy of the
// role's record and the fields past the unit.
class BattleRole : public BattleUnit
{
public:
    // The equipment slots of a role (Party::SlotsPerRole).
    // 0x004A7BD4: the element icon of a skill by its script's resistance (0xFFFF: none)
    static constexpr std::array<u16, 12> SkillElements = {8, 1, 2, 3, 4, 0xFFFF, 5, 6, 7, 0xFFFF, 9, 10};
    static constexpr std::u8string_view NotEnoughItems = u8"物品不夠!!";      // 0x004A7CD0
    static constexpr std::u8string_view NotEnoughSpirit = u8"靈力值不夠!!";   // 0x004A7CC0
    static constexpr std::u8string_view NotEnoughStamina = u8"體力值不夠!!";  // 0x004A7CB0
    static constexpr std::size_t WeaponSlot = 0;
    static constexpr std::size_t FirstAccessorySlot = 7;
    static constexpr std::size_t SecondAccessorySlot = 8;
    static constexpr std::size_t ArmourSlots = 7;  // slots below it give the bonus words
    static constexpr std::size_t NoResistanceSlotA = 9;
    static constexpr std::size_t NoResistanceSlotB = 0xA;

    static constexpr std::size_t ExtraActs = 5;
    // The magic act is MagicActs entries after the unit's act (inferred: the second half of the 3000-entry archive).
    static constexpr i32 MagicActs = 1500;
    // SwarmSpellFrame: where the swarm acts start around the role, by their number (0x004A7C04, 0x004A7C14).
    static constexpr std::array<i16, 8> SwarmOffsetX = {0, 0, -120, 120, -70, 70, -70, 70};
    static constexpr std::array<i16, 8> SwarmOffsetY = {-100, 100, 0, 0, -50, -50, 50, 50};

    BattleRole();
    ~BattleRole();
    void Release();
    void SetSkills(ItemNode* list);
    void SetInventory(ItemLink* list);
    void ClearExtras();
    void Setup(RoleRecord& record, RoleBattleBlock& block, const UnitPlacement& at, ErrorReporter& reporter);
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
    int MagicFrame(BattleServices& services, const BattleUnit& target);
    int BeamSpellFrame(BattleServices& services, BattleUnit& target);
    int ChargeAttackFrame(BattleServices& services, BattleUnit& target, int slot);
    int ChargeOnTarget(BattleServices& services, BattleUnit& target);
    int SwarmSpellFrame(BattleServices& services, BattleUnit& target);
    int BoomerangFrame(BattleServices& services, BattleUnit& target);
    int LeapSpellFrame(BattleServices& services, BattleUnit& target);
    u16 UseSelectedItem();
    void CancelCommand();
    void AddFury(BattleServices& services, int damage);
    int DrainShare(BattleServices& services, i32& amount);
    u16 Unk1EProficiency(BattleServices& services);
    void CountUsableObjects(u16& count);
    int ListedObject(int index, std::string& name, u16& count);
    ItemNode* StepItem();
    static u32 CategoryKinds(int category, int categories);
    ItemNode* StepSkill();
    void CountSkills(int category, int type, u8& count);
    static int ObjectsLeft(const ItemNode& node);
    void CountObjects(int category, u16& count);
    int ListObject(int category, int index, std::string& name, u16& flags, u16& count);
    int ThrowableObject(int throwable, int index, std::string& name, u16& count);
    u16 ListSkill(Database& database, int category, int type, int index, std::string& name, u16& cost);
    int StepWornCounters(CounterList& list, u16 amount);
    u16 SelectSkill(Database& database, int category, int type, int index, u32& act);
    int CanPaySkill(BattleServices& services, int category, int index);
    int SelectObject(Database& database, ErrorReporter& reporter, int category, int index, u32& act, u16& kind);
    void Regenerate(BattleServices& services);
    void UseAccessorySkill(BattleServices& services, u16 kind);
    void RollCritical(BattleServices& services);
    void PrepareCommand(BattleServices& services, Database& database);
    void FinishCommand();
    int BeatsLevel(BattleServices& services, u16 level) const;
    int StepBack(BattleServices& services, int mode);
    void SetupGuest(const UnitPlacement& at, const RoleRecord& base, Database& database, ErrorReporter& reporter);
    void ApplyEquipment(
        std::span<const std::unique_ptr<ItemNode>> slots,
        CounterList& proficiency,
        ErrorReporter& reporter
    );
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
    ItemLink* inventory = nullptr;               // +0x2EC8 the first item shown (the next of inventoryNode)
    ItemNode* inventoryNode = nullptr;           // the item at +0x2EC8 itself; nullptr: the list head
    ItemLink* inventoryHead = nullptr;           // +0x2ECC the party's items
    ItemNode* selectedItem = nullptr;            // +0x2ED0 the item chosen to use
    std::array<i32, ExtraActs> extraSteps = {};  // +0x2EF8 the line steps of each extra act this frame
    i32 unk2F0C = 0;
    u16 unk2F10 = 0;
    u16 staminaCost = 0;  // +0x2F12 the stamina the command takes when it runs
    u16 usedItem = 0;     // +0x2F14 the id of the item used (UseSelectedItem; GlowAct plays it as its act)
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
