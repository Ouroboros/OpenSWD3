#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <string>

#include "core/debug_log.h"
#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/databases.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_list.h"
#include "org/fame.h"
#include "platform/display.h"
#include "text/big5.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr int kLastSlot = 0xA;          // the slots of a role shown: 0..10
constexpr int kRows = 0xA;              // the list's rows
constexpr std::size_t kTriedStats = 4;  // EquipPage::stats: the role chosen with the object tried
constexpr int kWeaponSlot = 0;
constexpr int kFirstMagicSlot = 7;
constexpr int kSecondMagicSlot = 8;
constexpr int kFirstGuardSlot = 9;
constexpr int kSecondGuardSlot = 0xA;
constexpr int kResistanceSlots = 9;  // the resistances show for the first nine slots
constexpr i32 kDescriptionStage = 5;
constexpr i32 kRefusedStage = 0xF;
constexpr i32 kNoStage = -1;
constexpr u32 kKindTopBit = 0x8000;
constexpr u16 kRoleMask = 3;
constexpr int kWordShift = 16;
constexpr u32 kWordMask = 0xFFFF;
constexpr i32 kDescriptionSlide = 0x100;
constexpr i32 kPressedOffset = -1;  // Menu::buttonPress while the description button is pressed

// EquipStats.
constexpr std::size_t kStatsRead = 0x11;
constexpr std::size_t kAttackStat = 6;
constexpr std::size_t kDefenseStat = 7;
constexpr std::size_t kAgilityStat = 9;
constexpr std::size_t kAttackBonusStat = 0xF;
constexpr std::size_t kDefenseBonusStat = 0x10;
constexpr std::size_t kPercentStat = 0x11;
constexpr std::size_t kProficiencyStat = 0x12;
constexpr std::size_t kFameStat = 0x13;
constexpr i32 kFamePercent = 5;

constexpr u16 kMoveSound = 0x2E;
constexpr u16 kRoleSound = 0x107;
constexpr u16 kRefusedSound = 0x8C;
constexpr u16 kScrollUp = 3;
constexpr u16 kScrollDown = 0x30;
constexpr u16 kScrollUpMask = 0xF;
constexpr u16 kScrollDownMask = 0xF0;
constexpr int kNibbleShift = 4;

// The clicks (Menu::clicks).
constexpr u32 kLeftClicks = 3;
constexpr u32 kRightClick = 4;
constexpr u32 kAnyClick = 0xF;

// The mouse areas.
constexpr u32 kSlotsTop = 0x68;
constexpr u32 kSlotsBottom = 0x19C;
constexpr u32 kSlotsLeft = 0xC8;
constexpr u32 kSlotsRight = 0x1B2;
constexpr u32 kSlotHeight = 0x1C;
constexpr i32 kListBottom = 0xD0;
constexpr i32 kListTopMargin = 0xC;
constexpr i32 kListRight = 0x25C;
constexpr i32 kListLeftMargin = 0x22;
constexpr u32 kListRowHeight = 0x15;
constexpr u32 kBarLeft = 0x262;
constexpr u32 kBarRight = 0x272;
constexpr i32 kBarUpTop = 0x66;
constexpr i32 kBarUpBottom = 0x74;
constexpr i32 kBarDownTop = 0x130;
constexpr i32 kBarDownBottom = 0x140;
constexpr u32 kDescribeTop = 0x1D0;
constexpr u32 kDescribeBottom = 0x1E0;
constexpr u32 kDescribeLeft = 0x216;
constexpr u32 kDescribeRight = 0x260;
constexpr i32 kFacesTop = 0xA;
constexpr i32 kFacesBottom = 0x1D4;
constexpr i32 kFacesLeft = 4;
constexpr i32 kFacesRight = 0xBC;
constexpr i32 kFaceHeight = 0x6E;

// The texts.
constexpr std::u8string_view kNoObjects = u8"沒有物品";                                  // swd32.exe 0x004A6E4C
constexpr std::u8string_view kEmptyName = u8"無";                                        // swd32.exe 0x0049F9FC
constexpr std::u8string_view kBareHanded = u8"無法空手戰鬥";                             // swd32.exe 0x004A6E18
constexpr std::u8string_view kElementsTop = u8"火       冰       風       土       毒";  // swd32.exe 0x004A6E80
constexpr std::u8string_view kElementsBottom = u8"光       暗       電       物";        // swd32.exe 0x004A6E60
constexpr std::u8string_view kWeakFormat = u8"損%2d0%%";                                 // swd32.exe 0x004A6E40
constexpr std::u8string_view kResistFormat = u8"抗%2d0%%";                               // swd32.exe 0x004A6E34
constexpr std::u8string_view kAbsorbFormat = u8"吸%2d0%%";                               // swd32.exe 0x004A6E28
constexpr std::string_view kNoResistance = "------";
constexpr std::u8string_view kAttackLabel = u8" 攻擊";   // swd32.exe 0x004A6F08
constexpr std::u8string_view kDefenseLabel = u8" 防禦";  // swd32.exe 0x004A6EF0
constexpr std::u8string_view kAgilityLabel = u8" 敏捷";  // swd32.exe 0x004A6EE8
constexpr std::u8string_view kSkillLabel = u8" 熟練";    // swd32.exe 0x004A6EE0
constexpr std::u8string_view kGrowthLabel = u8" 成長";   // swd32.exe 0x004A6ED8
constexpr std::u8string_view kKnownLabel = u8" 辨識";    // swd32.exe 0x004A6EC0
// The names of the slots (swd32.exe 0x004A6F1C..0x004A6F5C, built on the stack in this order).
constexpr std::array<std::u8string_view, kLastSlot + 1> kSlotNames = {
    u8"武器",
    u8"頭部",
    u8"身體",
    u8"手部",
    u8"足部",
    u8"飾品",
    u8"    ",
    u8"法寶",
    u8"    ",
    u8"護駕",
    u8"    ",
};

constexpr std::string_view kSlotFormat = "%-7s%-12s";
constexpr std::string_view kRowFormat = "%-12s %2d";
constexpr std::string_view kEmptyFormat = "%-12s";
constexpr std::string_view kStatFormat = "%-6s%5d";
constexpr std::string_view kNumberFormat = "%d";
constexpr std::string_view kNewValueFormat = "%-5d";
constexpr std::string_view kPercentFormat = "%-6s%5d%%";
constexpr std::string_view kGrowthFormat = "%-6s%5d/%-5d";

// The layout.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kBlueRed = 2;
constexpr u32 kBlueGreen = 0xE;
constexpr u32 kBlueBlue = 0x1D;
constexpr u32 kRedRed = 0x1C;
constexpr u32 kRedGreen = 2;
constexpr u32 kRedBlue = 2;
constexpr u32 kGreenRed = 2;
constexpr u32 kGreenGreen = 0x1C;
constexpr u32 kGreenBlue = 0xD;
constexpr u32 kGrey = 0x10;
constexpr std::size_t kTopColor =
    6;  // swd32.exe 0x0049E0E0: the low word of TextColors::primary[6] (converted at start)
constexpr int kDim = -4;
constexpr int kPanelStyle = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr i32 kDescriptionHidden = 0x1E0;  // Menu::descriptionTop
constexpr i32 kDescriptionUp = 0x154;
constexpr int kDescriptionBoxX = 0xD8;
constexpr int kDescriptionBoxUp = 8;
constexpr int kDescriptionBoxWidth = 0x184;
constexpr int kDescriptionBoxBottom = 0x1E6;
constexpr int kDescriptionTextX = 0xDC;
constexpr int kDescriptionFrameRight = 0x254;
constexpr int kDescriptionFrameBottom = 0x1D6;
constexpr int kDescriptionLines = 5;
constexpr int kDescriptionWidth = 0x168;
constexpr i32 kSlideStart = 0x190;
constexpr i32 kListStartX = 0x1E8;
constexpr i32 kListStartY = 0x78;
constexpr i32 kFaceAct = 0x232A;
constexpr i32 kFirstFaceAction = 0x2D;
constexpr int kFirstFaceX = 0x14A;
constexpr int kSecondFaceX = 0x1AE;
constexpr int kThirdFaceX = 0x212;
constexpr int kFacesY = 0x3E;
constexpr int kNameBoxLeft = 0xD0;
constexpr int kNameBoxTop = 0x3C;
constexpr int kNameBoxRight = 0x13C;
constexpr int kNameBoxBottom = 0x50;
constexpr int kNameX = 0xD4;
constexpr int kNameY = 0x3D;
constexpr int kHelpBoxX = 0xD0;
constexpr int kHelpBoxY = 0x1B4;
constexpr int kHelpBoxWidth = 0x19E;
constexpr int kHelpBoxHeight = 0x1E;
constexpr int kHelpBoxMode = 2;
constexpr int kListBoxLeft = 0x24;
constexpr int kStatsBoxY = 0x152;
constexpr int kStatsBoxWidth = 0xAA;
constexpr int kStatsBoxHeight = 0x4E;
constexpr int kListBoxMode = 4;
constexpr int kSlotsBoxX = 0xC8;
constexpr int kSlotsBoxY = 0x60;
constexpr int kSlotsBoxWidth = 0xEA;
constexpr int kSlotsBoxHeight = 0x148;
constexpr int kSlotsFrameLeft = 0xD0;
constexpr int kSlotsFrameTop = 0x68;
constexpr int kSlotsFrameRight = 0x1AC;
constexpr int kSlotsFrameBottom = 0x1A0;
constexpr int kSlotsWidth = 0xFC;
constexpr int kListBoxUp = 0x10;
constexpr int kListBoxWidth = 0x96;
constexpr int kListBoxHeight = 0xD2;
constexpr i32 kPictureAction = 0x44;
constexpr i32 kButtonAction = 0x20;
constexpr int kButtonX = 0x21C;
constexpr int kButtonY = 0x1D4;
constexpr int kRowTextLeft = 0x1E;
constexpr int kRowTextUp = 0xE;
constexpr int kRowHighlightLeft = 0x29;
constexpr int kRowHighlightUp = 0xF;
constexpr int kRowHighlightWidth = 0xA0;
constexpr int kRowHighlightHeight = 0x14;
constexpr int kHighlightHeight = 0x18;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kBarOffsetX = 0x7A;
constexpr int kBarUp = 4;
constexpr int kBarHeight = 0xBA;
constexpr int kSecondBarOffsetX = 0xC4;
constexpr int kSecondBarDown = 9;
constexpr int kSecondBarHeight = 0xCA;
constexpr int kElementsX = 0xD8;
constexpr int kElementsY = 0x1B0;
constexpr int kElementLine = 0x14;
constexpr int kElementStep = 0x51;
constexpr int kElementValuesX = 0xF0;
constexpr int kElementValuesY = 0x1B5;
constexpr int kElementsPerLine = 5;
constexpr i32 kAbsorbFrom = -0xA;
constexpr int kRefusedX = 0xFE;
constexpr int kRefusedY = 0xE4;
constexpr int kRefusedWidth = 0x84;
constexpr int kRefusedHeight = 0x16;
constexpr int kSlotRowDown = 4;
constexpr int kSlotRowHeight = 0x1C;
constexpr int kSlotTextX = 5;
constexpr int kSlotTextDown = 2;
constexpr int kSlotHighlightNarrow = 0x16;
constexpr i32 kSlotPicturesAction = 0xD;
constexpr int kSlotPictureX = 0x38;
constexpr std::array<u16, 8> kSlotPictures = {3, 0, 1, 6, 4, 7, 5, 2};
constexpr std::array<int, 8> kSlotPictureY = {6, 0x22, 0x3E, 0x5A, 0x76, 0x92, 0xCA, 0x102};
constexpr int kStatsX = 0x1C8;
constexpr int kAttackY = 0x152;
constexpr int kDefenseY = 0x166;
constexpr int kAgilityY = 0x17A;
constexpr int kExtraY = 0x18E;
constexpr int kAttackTopX = 0x17C;
constexpr int kDefenseTopX = 0x1E0;
constexpr int kAgilityTopX = 0x244;
constexpr int kArrowX = 0x22E;
constexpr int kNewValueX = 0x23E;
constexpr u16 kUpArrow = 0x2463;  // the object tried raises the value
constexpr u16 kDownArrow = 0x2465;

}  // namespace

// swd32.exe 0x00440630: the equipment page opened (on the first role when no tab is raised): the list of the first
// slot, the stats, its description
void Menu::EquipEnter()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    page.takenOff = ItemLink();
    page.inventoryHead = owner.party.inventory.get();

    if (this->roleTab == Menu::NoTab)
    {
        this->roleTab = 0;
    }

    page.openedAt = this->frames;
    page.worn = RoleRecord{};
    page.shown = 0;
    page.row = 0;
    page.top = 0;
    page.count = 0;
    page.slot = 0;
    page.stage = 0;
    page.firstShown = nullptr;
    page.objects.reset();
    this->EquipFillList();
    this->gauge.id = 0;
    page.slide = 0;

    for (auto& stats : page.stats)
    {
        stats.fill(0);
    }

    this->EquipComputeStats();
    this->EquipDescribeSlot();
    this->descriptionSlide = 0;
    page.described = nullptr;
    page.scrollArrows = 0;
    this->descriptionTop = kDescriptionHidden;
    page.previousStage = kNoStage;
    page.listX = kListStartX;
    page.listY = kListStartY;
}

// swd32.exe 0x00440750: the equipment page left: the list's objects back in the inventory, every slot's count 1 again
void Menu::EquipLeave()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    this->EquipReturn();
    page.shown = 0;
    page.row = 0;
    page.top = 0;
    page.count = 0;
    page.slot = 0;
    page.stage = 0;
    page.firstShown = nullptr;

    for (std::unique_ptr<ItemNode>& slot : owner.party.equipment)
    {
        if (slot != nullptr)
        {
            slot->count = 1;
            slot->pending = 0;
        }
    }

    page.savedBeforeFrame = nullptr;
}

// The description of the object in the slot chosen (inline in the equipment page).
void Menu::EquipDescribeSlot()
{
    const ItemNode* worn = this->EquipSlotNode();
    this->description = worn != nullptr ? ObjectDescription(this->game.databases.mapsData, worn->id) : std::string();
}

// The description of the object of the row chosen (inline in the equipment page).
void Menu::EquipDescribeRow()
{
    const EquipPage& page = this->equip;
    const ItemNode* chosen = ItemAfter(page.top + page.row, page.objects);
    this->description =
        chosen != nullptr ? ObjectDescription(this->game.databases.mapsData, chosen->id) : std::string();
}

// The object in the slot chosen of the role chosen (inline in the equipment page).
ItemNode* Menu::EquipSlotNode()
{
    const auto at = static_cast<std::size_t>(Party::SlotsPerRole * this->roleTab + this->equip.slot);
    std::array<std::unique_ptr<ItemNode>, Party::Roles * Party::SlotsPerRole>& equipment = this->game.party.equipment;
    return at < equipment.size() ? equipment[at].get() : nullptr;
}

// swd32.exe 0x00441F70: the objects of `source` that fit slot `slot` and the role chosen can use moved into the page's
// list in the order of their ids
void Menu::EquipTake(ItemLink& source, int slot)
{
    EquipPage& page = this->equip;
    page.objects.reset();
    const u32 kind = Party::SlotKinds[static_cast<std::size_t>(std::clamp(slot, 0, kLastSlot))];
    const u16 role = static_cast<u16>(ObjectData::FirstRoleBit >> std::min<u16>(this->roleTab, Party::Roles - 1));
    ItemLink* link = &source;

    while (*link != nullptr)
    {
        const ItemNode& node = **link;

        if ((node.record.data.kind & kind & ~kKindTopBit) != kind || (node.record.data.usableBy & role) == 0)
        {
            link = &(*link)->next;
            continue;
        }

        std::unique_ptr<ItemNode> moved = std::move(*link);
        *link = std::move(moved->next);
        InsertItemById(page.objects, std::move(moved));
    }
}

// swd32.exe 0x00442020: the rows shown from the first one shown (at most ten)
void Menu::EquipCountShown()
{
    EquipPage& page = this->equip;
    page.shown = 0;

    for (const ItemNode* node = page.firstShown; node != nullptr && page.shown < kRows; node = node->next.get())
    {
        ++page.shown;
    }
}

// swd32.exe 0x00442050: the list of the slot chosen made from the inventory, the empty entry last when the slot holds
// an object (or nothing fits)
void Menu::EquipFillList()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    this->EquipTake(owner.party.inventory, page.slot);
    const ItemNode* worn = this->EquipSlotNode();

    if ((worn != nullptr && worn->id != ItemNode::Empty) || page.objects == nullptr)
    {
        ItemLink* last = &page.objects;

        while (*last != nullptr)
        {
            last = &(*last)->next;
        }

        *last = NewEmptyItem();
    }

    page.count = CountItems(page.objects);
    page.top = 0;
    page.row = 0;
    page.firstShown = page.objects.get();
    this->EquipCountShown();
}

// swd32.exe 0x004420F0: the list's objects back at the front of the inventory one by one (their order turns around);
// the empty entry freed
void Menu::EquipReturn()
{
    EquipPage& page = this->equip;
    ItemLink& inventory = this->game.party.inventory;

    while (page.objects != nullptr)
    {
        std::unique_ptr<ItemNode> node = std::move(page.objects);
        page.objects = std::move(node->next);

        if (node->id != ItemNode::Empty)
        {
            node->next = std::move(inventory);
            inventory = std::move(node);
        }
    }
}

// swd32.exe 0x004429B0: the stats of the four roles as they wear their equipment, then of the role chosen with the
// object tried, and the slot's extra figure
void Menu::EquipComputeStats()
{
    EquipPage& page = this->equip;

    for (int role = 0; role < Party::Roles; ++role)
    {
        this->EquipRoleStats(role, page.stats[static_cast<std::size_t>(role)]);
    }

    const ItemNode* tried = this->EquipTried();
    this->EquipTryStats(this->roleTab, page.slot, page.stats[kTriedStats], tried);
    this->EquipExtraStats(tried, page.stats[kTriedStats]);
}

// swd32.exe 0x00442A40: the object tried: the one in the slot, or the row chosen on the list
const ItemNode* Menu::EquipTried()
{
    const EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        return this->EquipSlotNode();
    }

    if (page.stage != 1)
    {
        return nullptr;
    }

    ItemNode* node = nullptr;
    SkipItems(page.top + page.row, page.objects, node);
    return node;
}

// swd32.exe 0x00442AA0: the stats of role `role` wearing its sixteen slots. The original starts from the record of the
// role chosen, not of `role`.
void Menu::EquipRoleStats(int role, EquipStats& stats)
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    page.worn = owner.party.roles[std::min<std::size_t>(this->roleTab, Party::Roles - 1)];

    for (int slot = 0; slot < Party::SlotsPerRole; ++slot)
    {
        const ItemNode* worn = owner.party.equipment[static_cast<std::size_t>(Party::SlotsPerRole * role + slot)].get();

        if (worn != nullptr)
        {
            ApplyObject(page.worn, worn->record.data, owner.database);
        }
    }

    this->EquipStatsOf(stats);
}

// swd32.exe 0x00442B10: the stats of the role chosen with `tried` in slot `slot` (nothing there without one), the
// equipment bonuses starting from 0
void Menu::EquipTryStats(int role, int slot, EquipStats& stats, const ItemNode* tried)
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    page.worn = owner.party.roles[std::min<std::size_t>(this->roleTab, Party::Roles - 1)];
    page.worn.attackBonus = 0;
    page.worn.defenseBonus = 0;
    stats.fill(0);

    for (int index = 0; index < Party::SlotsPerRole; ++index)
    {
        if (index == slot)
        {
            if (tried != nullptr)
            {
                ApplyObject(page.worn, tried->record.data, owner.database);
            }

            continue;
        }

        const ItemNode* worn =
            owner.party.equipment[static_cast<std::size_t>(Party::SlotsPerRole * role + index)].get();

        if (worn != nullptr)
        {
            ApplyObject(page.worn, worn->record.data, owner.database);
        }
    }

    this->EquipStatsOf(stats);
}

// swd32.exe 0x00442BC0: the figures of the record worked on: the vitals and their maxima, the attributes, the dword at
// +0x20 as a word, the two equipment bonuses
void Menu::EquipStatsOf(EquipStats& stats)
{
    const RoleRecord& record = this->equip.worn;
    const std::array<i32, kStatsRead> values = {
        record.hitPoints,
        record.spirit,
        record.stamina,
        record.maxHitPoints,
        record.maxSpirit,
        record.maxStamina,
        record.attack,
        record.defense,
        record.wisdom,
        record.agility,
        record.rapport,
        record.reaction,
        record.ailmentChance,
        record.evasion,
        static_cast<i16>(record.experience),
        static_cast<i16>(record.attackBonus),
        static_cast<i16>(record.defenseBonus),
    };

    std::copy(values.begin(), values.end(), stats.begin());
}

// swd32.exe 0x00442CA0: the slot's extra figure of `tried`: a weapon its percent (the first counter list), a slot 7 or
// 8 object its proficiency and maximum, a slot 9 or 10 object its fame; -1 for none
void Menu::EquipExtraStats(const ItemNode* tried, EquipStats& stats)
{
    Game& owner = this->game;
    const EquipPage& page = this->equip;
    const bool present = tried != nullptr && tried->id != ItemNode::Empty;
    stats[kPercentStat] = -1;

    if (page.slot == kWeaponSlot && present)
    {
        stats[kPercentStat] =
            static_cast<u16>(FindCounterPercent(owner.party.fame.lists[Fame::UnkList0], static_cast<i16>(tried->id)));
    }

    stats[kProficiencyStat] = -1;

    if ((page.slot == kFirstMagicSlot || page.slot == kSecondMagicSlot) && present)
    {
        u16 maximum = 0;
        u16 value = 0;
        GetProficiency(owner.party.fame.lists[Fame::ProficiencyList], tried->id, maximum, value, owner.database);
        stats[kProficiencyStat] = static_cast<i32>(value | static_cast<u32>(maximum) << kWordShift);
    }

    stats[kFameStat] = -1;

    if ((page.slot == kFirstGuardSlot || page.slot == kSecondGuardSlot) && present)
    {
        stats[kFameStat] =
            static_cast<u16>(FindCounterValue(owner.party.fame.lists[Fame::FameList], static_cast<i16>(tried->id)));
    }
}

// swd32.exe 0x00442D70: the maxima of the role chosen moved from what `removed` gives to what `added` gives
void Menu::EquipSwapVitals(const ItemNode& added, const ItemNode& removed)
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    RoleRecord& role = owner.party.roles[std::min<std::size_t>(this->roleTab, Party::Roles - 1)];
    page.worn = RoleRecord{};

    if (removed.id != ItemNode::Empty)
    {
        ApplyObject(page.worn, removed.record.data, owner.database);
    }

    role.maxHitPoints = static_cast<i16>(role.maxHitPoints - page.worn.maxHitPoints);
    role.maxSpirit = static_cast<i16>(role.maxSpirit - page.worn.maxSpirit);
    role.maxStamina = static_cast<i16>(role.maxStamina - page.worn.maxStamina);
    page.worn = RoleRecord{};

    if (added.id != ItemNode::Empty)
    {
        ApplyObject(page.worn, added.record.data, owner.database);
    }

    role.maxHitPoints = static_cast<i16>(role.maxHitPoints + page.worn.maxHitPoints);
    role.maxSpirit = static_cast<i16>(role.maxSpirit + page.worn.maxSpirit);
    role.maxStamina = static_cast<i16>(role.maxStamina + page.worn.maxStamina);
}

// The list moved (inline in the equipment page's keys): the first row shown, the description and stats of the row.
void Menu::EquipListMoved(u16 arrows)
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    SkipItems(page.top, page.objects, page.firstShown);
    CountShown(page.firstShown, page.shown, kRows);
    this->EquipDescribeRow();
    this->EquipComputeStats();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    page.scrollArrows = static_cast<u16>(page.scrollArrows | arrows);
}

// Another slot chosen (inline in the equipment page's keys): its list made again, its description and the stats.
void Menu::EquipSlotMoved()
{
    Game& owner = this->game;
    this->EquipFillList();
    this->EquipDescribeSlot();
    this->EquipComputeStats();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x00440B20: down: the next slot (wrapping), on the list the next row
void Menu::EquipDown()
{
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();

        if (++page.slot > kLastSlot)
        {
            page.slot = 0;
        }

        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        ListDown(page.count, page.top, page.row, page.shown);
        this->EquipListMoved(kScrollDown);
    }
}

// swd32.exe 0x00440C20: up: the slot before (wrapping), on the list the row before
void Menu::EquipUp()
{
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();

        if (--page.slot < 0)
        {
            page.slot = kLastSlot;
        }

        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        ListUp(page.top, page.row);
        this->EquipListMoved(kScrollUp);
    }
}

// swd32.exe 0x00440D20: Page Down: the last slot, on the list a page of ten on
void Menu::EquipPageDown()
{
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();
        page.slot = kLastSlot;
        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        ListPageDown(page.count, page.top, page.row, page.shown, kRows);
        this->EquipListMoved(kScrollDown);
    }
}

// swd32.exe 0x00440E10: Page Up: the first slot, on the list a page of ten back
void Menu::EquipPageUp()
{
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();
        page.slot = 0;
        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        ListPageUp(page.top, page.row, kRows);
        this->EquipListMoved(kScrollUp);
    }
}

// swd32.exe 0x00440F00: left: the slot before; on the list as up, then described again
void Menu::EquipLeft()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();

        if (--page.slot < 0)
        {
            page.slot = kLastSlot;
        }

        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        this->EquipUp();
        this->EquipDescribeRow();
        this->EquipComputeStats();
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    }
}

// swd32.exe 0x00440FB0: right: the next slot; on the list as down, then described again
void Menu::EquipRight()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;

    if (page.stage == 0)
    {
        this->EquipReturn();

        if (++page.slot > kLastSlot)
        {
            page.slot = 0;
        }

        this->EquipSlotMoved();
        return;
    }

    if (page.stage == 1)
    {
        this->EquipDown();
        this->EquipDescribeRow();
        this->EquipComputeStats();
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    }
}

// swd32.exe 0x00441060: Shift on the slots: the next role of the party
void Menu::EquipShift()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;

    if (page.stage != 0)
    {
        return;
    }

    u16 role = static_cast<u16>((this->roleTab + 1) & kRoleMask);

    while (this->tabs[role].role == Menu::NoRole)
    {
        role = static_cast<u16>((role + 1) & kRoleMask);
    }

    this->roleTab = role;
    this->EquipReturn();
    this->EquipFillList();
    FitListRow(page.count, page.top, page.row, page.shown);
    SkipItems(page.top, page.objects, page.firstShown);
    CountShown(page.firstShown, page.shown, kRows);
    this->EquipDescribeSlot();
    this->EquipComputeStats();
    owner.audio.PlaySound(kRoleSound, owner.config.soundVolume);
}

// swd32.exe 0x00441160: Space or Enter: on the slots the list opens; on the list the row is put on (the weapon slot
// refuses to be emptied: "無法空手戰鬥"): the maxima change, the object leaves the list and the one taken off joins it
// (one of another kind goes to a list of its own and is lost); with the description open it closes; the refusal
// closes
void Menu::EquipSelect()
{
    EquipPage& page = this->equip;

    switch (page.stage)
    {
        case 0:
            ++page.stage;
            this->EquipDescribeRow();
            this->EquipComputeStats();
            return;

        case 1:
            this->EquipPutOn();
            return;

        case kDescriptionStage:
        {
            const Handler saved = page.savedBeforeFrame;
            this->descriptionTop = kDescriptionHidden;
            page.described = nullptr;
            this->descriptionSlide = 0;
            page.savedBeforeFrame = nullptr;
            page.stage = page.previousStage;
            this->beforeFrame = saved;
            return;
        }

        case kRefusedStage:
            page.stage = 1;
            return;

        default:
            return;
    }
}

// The row chosen put on (inline in 0x00441160, stage 1).
void Menu::EquipPutOn()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    ItemNode* chosen = ItemAt(page.objects, static_cast<u16>(page.top + page.row));
    ItemNode* worn = this->EquipSlotNode();

    if (chosen == nullptr || worn == nullptr || (page.slot == kWeaponSlot && chosen->id == ItemNode::Empty))
    {
        page.stage = kRefusedStage;
        owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
        return;
    }

    ItemNode removed;
    removed.id = worn->id;
    removed.unk06 = worn->unk06;
    removed.count = worn->count;
    removed.pending = worn->pending;
    removed.record = worn->record;
    this->EquipSwapVitals(*chosen, removed);
    worn->id = chosen->id;
    worn->record = chosen->record;
    worn->count = 1;
    worn->pending = 0;
    worn->unk06 = 0;
    worn->next.reset();
    const u16 putOn = chosen->id;
    AddItem(page.objects, putOn, -1, ItemCountMode::Pending, owner.database);
    const u32 kind = Party::SlotKinds[static_cast<std::size_t>(page.slot)];

    if ((removed.record.data.kind & kind) == kind || removed.id == ItemNode::Empty)
    {
        AddItem(page.objects, removed.id, 1, ItemCountMode::Count, owner.database);
        AddItem(page.objects, ItemNode::Empty, -1, ItemCountMode::Pending, owner.database);
        ItemLink leftover = std::move(page.objects);
        this->EquipTake(leftover, page.slot);

        if (worn->id != ItemNode::Empty || page.objects == nullptr)
        {
            bool noEmpty = true;
            ItemLink* last = &page.objects;

            while (*last != nullptr)
            {
                if ((*last)->id == ItemNode::Empty)
                {
                    noEmpty = false;
                }

                last = &(*last)->next;
            }

            if (noEmpty)
            {
                AddItem(*last, ItemNode::Empty, 1, ItemCountMode::Pending, owner.database);
            }
        }
    }
    else
    {
        AddItem(page.takenOff, removed.id, 1, ItemCountMode::Count, owner.database);
        DebugLog(u8"裝備道具歸還 卸下了種類不符的裝備(%d)", removed.id);  // 0x004A6DEC + 0x004A6DFC
    }

    page.count = CountItems(page.objects);
    SkipItems(page.top, page.objects, page.firstShown);
    CountShown(page.firstShown, page.shown, kRows);
    FitListRow(page.count, page.top, page.row, page.shown);
    this->EquipDescribeSlot();
    --page.stage;
    this->EquipComputeStats();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x00441590: Escape: from the slots the page closes (back to the shop when it opened the menu); from the
// list back to the slots; with the description open it closes; the refusal closes
void Menu::EquipCancel()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;

    switch (page.stage)
    {
        case 0:
            if (--this->code == 0)
            {
                owner.menuState = Game::NoMenu;
            }

            if ((this->openedOnEquipment & 1) != 0)
            {
                owner.menuState = Game::MenuActsReset | Game::ItemChoiceMenu;
            }

            this->SetPage(this->code);
            this->EquipReturn();
            this->EquipLeave();
            return;

        case 1:
            --page.stage;
            this->EquipDescribeSlot();
            this->EquipComputeStats();
            return;

        case kDescriptionStage:
            this->descriptionTop = kDescriptionHidden;
            page.described = nullptr;
            this->descriptionSlide = 0;
            page.stage = page.previousStage;
            this->beforeFrame = page.savedBeforeFrame;
            page.savedBeforeFrame = nullptr;
            return;

        case kRefusedStage:
            this->EquipSelect();
            return;

        default:
            return;
    }
}

// swd32.exe 0x004407F0: the mouse on the equipment page: a click on a slot chooses it (from the list, the list closes
// first); on the list a click on another row chooses it, on the chosen one puts it on (from the slots it opens the
// list); a held left button on the scroll bar (from the slots: opens the list); the description button presses the
// description key; a click on a role's face changes the role; a right click goes back. The refusal closes on any
// click; with the description open only a right click counts.
void Menu::EquipFrame()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    const u32 clicks = this->clicks;
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    auto inside = [](u32 value, u32 low, u32 high)
    {
        return value < high && value > low;
    };

    auto back = [this, clicks]
    {
        if ((clicks & kRightClick) != 0)
        {
            this->EquipCancel();
        }
    };

    if (page.stage == kRefusedStage)
    {
        if ((clicks & kAnyClick) != 0)
        {
            this->EquipSelect();
            return;
        }
    }
    else if (page.stage == kDescriptionStage)
    {
        back();
        return;
    }

    if ((clicks & kLeftClicks) != 0 && inside(y, kSlotsTop, kSlotsBottom) && inside(x, kSlotsLeft, kSlotsRight))
    {
        const auto slot = static_cast<i32>((y - kSlotsTop) / kSlotHeight);

        if (page.stage == 1)
        {
            this->EquipCancel();
            page.slot = slot - 1;
            this->EquipDown();
        }
        else if (page.slot != slot)
        {
            page.slot = slot - 1;
            this->EquipDown();
        }

        return;
    }

    const i32 pointerX = owner.state.pointerX;
    const i32 pointerY = owner.state.pointerY;

    if ((clicks & kLeftClicks) != 0 && pointerY < page.listY + kListBottom && pointerY > page.listY - kListTopMargin &&
        pointerX < kListRight && pointerX > page.listX - kListLeftMargin)
    {
        const auto row = static_cast<i32>(static_cast<u32>(pointerY - page.listY + kListTopMargin) / kListRowHeight);

        if (page.stage != 0)
        {
            if (row == page.row)
            {
                this->EquipSelect();
            }
            else
            {
                if (row < page.shown)
                {
                    page.row = row;
                }

                const ItemNode* chosen = ItemAfter(page.row + page.top, page.objects);
                this->description =
                    chosen != nullptr ? ObjectDescription(owner.databases.mapsData, chosen->id) : std::string();
            }
        }
        else
        {
            this->EquipSelect();

            if (row < page.shown)
            {
                page.row = row;
            }
        }

        if (page.stage != kRefusedStage)
        {
            this->EquipComputeStats();
        }

        return;
    }

    if (owner.controls.Repeating(Button::MouseLeft) && page.count > kRows && inside(x, kBarLeft, kBarRight))
    {
        if (page.stage == 0)
        {
            this->EquipSelect();
            return;
        }

        if (page.stage == 1)
        {
            const MenuScrollBar& bar = this->scrollBar;

            if (pointerY < kBarUpBottom && pointerY > kBarUpTop)
            {
                this->EquipUp();
                return;
            }

            if (pointerY < kBarDownBottom && pointerY > kBarDownTop)
            {
                this->EquipDown();
                return;
            }

            if (pointerY < bar.thumbTop && pointerY > bar.top)
            {
                this->EquipPageUp();
                return;
            }

            if (pointerY < bar.bottom && pointerY > bar.thumbBottom)
            {
                this->EquipPageDown();
                return;
            }
        }
    }

    this->buttonPress = 0;

    if (inside(y, kDescribeTop, kDescribeBottom) && inside(x, kDescribeLeft, kDescribeRight))
    {
        this->buttonPress = kPressedOffset;

        if ((clicks & kLeftClicks) == 0)
        {
            back();
            return;
        }

        owner.input.PressKey(owner.config.keys[static_cast<std::size_t>(Button::F1)]);
    }

    if ((clicks & kLeftClicks) != 0 && pointerY < kFacesBottom && pointerY > kFacesTop && pointerX < kFacesRight &&
        pointerX > kFacesLeft)
    {
        if (page.stage == 1)
        {
            this->EquipCancel();
        }

        if (page.stage == 0)
        {
            const int face = (owner.state.pointerY - kFacesTop) / kFaceHeight;

            if (owner.state.GameFlag(face + Party::FirstRoleFlag))
            {
                this->roleTab = static_cast<u16>(face - 1);
                this->EquipShift();
            }
        }

        return;
    }

    back();
}

// swd32.exe 0x00441680: the equipment page. The description key on the slots or the list opens the description of the
// object (the page's keys held). The page slides in: the role's faces and name, the slots, the list of the slot (the
// row chosen with its picture; "沒有物品" when empty) with its scroll bar, the stats, and for a worn object or a row
// tried in the first nine slots its nine resistances; the description box; the refusal "無法空手戰鬥".
void Menu::EquipDraw()
{
    Game& owner = this->game;
    EquipPage& page = this->equip;
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    const auto blue = static_cast<Pixel>(owner.painter.MakeColor(kBlueRed, kBlueGreen, kBlueBlue));
    const auto red = static_cast<Pixel>(owner.painter.MakeColor(kRedRed, kRedGreen, kRedBlue));
    const auto green = static_cast<Pixel>(owner.painter.MakeColor(kGreenRed, kGreenGreen, kGreenBlue));
    ActState& panel = owner.battleMessages.panelAct;

    if (static_cast<u32>(page.stage) <= 1 &&
        owner.input.IsKeyDown(owner.config.keys[static_cast<std::size_t>(Button::F1)]) != 0)
    {
        if (page.stage == 1)
        {
            page.previousStage = 1;
            page.described = page.firstShown;

            for (int step = 0; step < page.row && page.described != nullptr; ++step)
            {
                page.described = page.described->next.get();
            }
        }
        else
        {
            page.previousStage = 0;
            page.described = this->EquipSlotNode();
        }

        if (page.described != nullptr && page.described->id != ItemNode::Empty)
        {
            this->descriptionSlide = kDescriptionSlide;
            page.stage = kDescriptionStage;
            page.savedBeforeFrame = this->beforeFrame;
            this->beforeFrame = nullptr;
        }
    }

    if (page.openedAt == this->frames)
    {
        page.slide = kSlideStart;
        page.listX = kListStartX;
        page.listY = kListStartY;
        page.unk4FCD30 = 0;
        page.unk4FCD5C = page.stage;
    }

    const i32 slideBefore = page.slide;
    page.slide >>= 1;
    this->Move(EquipSlideValue, page.slide, slideBefore, page.slide);
    const int slide = this->Shown(EquipSlideValue, page.slide);
    owner.acts.Run(panel);
    this->face.id = kFaceAct;
    this->face.direction = 0;
    this->face.action = kFirstFaceAction;
    owner.actDrawer.Draw(this->face, kFirstFaceX, kFacesY - slide);
    this->face.action = kFirstFaceAction + 1;
    owner.actDrawer.Draw(this->face, kSecondFaceX, kFacesY - slide);
    this->face.action = kFirstFaceAction + 2;
    owner.actDrawer.Draw(this->face, slide + kThirdFaceX, kFacesY - slide);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kNameBoxLeft - slide,
        kNameBoxTop - slide,
        kNameBoxRight - slide,
        kNameBoxBottom - slide,
        0,
        kFrameBorder
    );
    const Party::Name& name = owner.party.names[std::min<std::size_t>(this->roleTab, Party::Roles - 1)];
    owner.mediumFont.DrawText(
        owner.drawSurface,
        kNameX - slide,
        kNameY - slide,
        std::string_view(name.data(), strnlen(name.data(), name.size())),
        color,
        kPanelStyle
    );
    this->DrawBox(kHelpBoxX, slide + kHelpBoxY, kHelpBoxWidth, slide + kHelpBoxHeight, kHelpBoxMode, 0, 0, 0);
    this->DrawBox(
        page.listX + slide - kListBoxLeft,
        kStatsBoxY,
        kStatsBoxWidth,
        kStatsBoxHeight,
        kListBoxMode,
        0,
        0,
        0
    );
    FillBox(owner.painter, kSlotsBoxX - slide, kSlotsBoxY, kSlotsBoxWidth, kSlotsBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kSlotsFrameLeft - slide,
        kSlotsFrameTop,
        kSlotsFrameRight - slide,
        kSlotsFrameBottom,
        0,
        kFrameBorder
    );
    this->EquipDrawSlots(kSlotsFrameLeft, kSlotsFrameTop, slide, kSlotsWidth, page.slot);
    const ItemNode* shownObject = this->EquipSlotNode();
    owner.acts.Run(panel);
    this->DrawBox(
        page.listX + slide - kListBoxLeft,
        page.listY - kListBoxUp,
        kListBoxWidth,
        kListBoxHeight,
        kListBoxMode,
        0,
        0,
        0
    );
    this->gauge.id = 0;

    if (page.objects == nullptr)
    {
        owner.largeFont.DrawText(owner.drawSurface, slide + page.listX, page.listY, kNoObjects, color, kPanelStyle);
    }
    else
    {
        const ItemNode* node = page.firstShown;

        for (int row = 0, offset = 0; row < page.shown && node != nullptr;
             ++row, offset += kListRowHeight, node = node->next.get())
        {
            owner.largeFont.SetBackground(TextFont::NoBackground);
            Pixel rowColor = color;
            const bool open = page.stage == 1 || (page.stage == kDescriptionStage && page.previousStage == 1);

            if (page.stage != 1)
            {
                owner.painter.TintPixels(std::span<u16>(&rowColor, 1), kDim, kDim, kDim);
            }

            if ((page.stage == 1 || page.stage == kDescriptionStage) && row == page.row)
            {
                if (open)
                {
                    rowColor = color;

                    if (page.stage != 1)
                    {
                        owner.painter.TintPixels(std::span<u16>(&rowColor, 1), kDim, kDim, kDim);
                    }
                }

                if (node->id != ItemNode::Empty)
                {
                    if (node->record.data.act != 0)
                    {
                        this->gauge.action = kPictureAction;
                        this->gauge.id = node->record.data.act;
                        this->gauge.direction = 0;
                    }

                    shownObject = node;
                    this->buttonAct.id = kFaceAct;
                    this->buttonAct.action = kButtonAction;
                    owner.actDrawer.Draw(this->buttonAct, this->buttonPress + kButtonX, this->buttonPress + kButtonY);
                }
            }

            const std::string text =
                node->id == ItemNode::Empty
                    ? WinFormat(kEmptyFormat, {Big5(kEmptyName)})
                    : WinFormat(kRowFormat, {ObjectName(node->record), node->count + node->pending});
            owner.mediumFont.DrawText(
                owner.drawSurface,
                page.listX + slide - kRowTextLeft,
                page.listY + offset - kRowTextUp,
                text,
                rowColor,
                kPanelStyle
            );

            if (row == page.row && open)
            {
                FillBox(
                    owner.painter,
                    page.listX + slide - kRowHighlightLeft,
                    page.listY + offset - kRowHighlightUp,
                    kRowHighlightWidth,
                    kRowHighlightHeight,
                    kHighlightRed,
                    kHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }
        }
    }

    if (page.count > page.shown)
    {
        int arrows = 0;

        if ((page.scrollArrows & kScrollUpMask) != 0)
        {
            page.scrollArrows =
                static_cast<u16>((page.scrollArrows & ~kScrollUpMask) | ((page.scrollArrows & kScrollUpMask) - 1));
            arrows = 1;
        }

        if ((page.scrollArrows & kScrollDownMask) != 0)
        {
            const int down = (page.scrollArrows & kScrollDownMask) >> kNibbleShift;
            arrows |= 2;
            page.scrollArrows = static_cast<u16>((page.scrollArrows & ~kScrollDownMask) | ((down - 1) << kNibbleShift));
        }

        const auto count = static_cast<double>(page.count);
        const auto to = static_cast<float>(static_cast<double>(page.shown + page.top) / count);
        const auto from = static_cast<float>(static_cast<double>(page.top) / count);
        this->DrawScrollBar(
            page.listX + slide + kBarOffsetX,
            page.listY - kBarUp,
            kBarHeight,
            arrows,
            from,
            to,
            this->scrollBar
        );
    }

    this->EquipDrawStats(page.slot, this->roleTab);

    if (page.count > kRows)
    {
        const auto count = static_cast<double>(page.count);
        const auto to = static_cast<float>(static_cast<double>(page.shown + page.top) / count);
        const auto from = static_cast<float>(static_cast<double>(page.top) / count);
        this->DrawScrollBar(
            page.listX + slide + kSecondBarOffsetX,
            page.listY + kSecondBarDown,
            kSecondBarHeight,
            0,
            from,
            to,
            this->unk4FBAB4
        );
    }

    if (shownObject != nullptr && shownObject->id != ItemNode::Empty && page.slot < kResistanceSlots)
    {
        owner.mediumFont.DrawText(owner.drawSurface, kElementsX, slide + kElementsY, kElementsTop, color, kPanelStyle);
        owner.mediumFont.DrawText(
            owner.drawSurface,
            kElementsX,
            slide + kElementsY + kElementLine,
            kElementsBottom,
            color,
            kPanelStyle
        );
        const auto grey = static_cast<Pixel>(owner.painter.MakeColor(kGrey, kGrey, kGrey));
        Pixel shade = green;

        for (std::size_t element = 0; element < shownObject->record.data.resistances.size(); ++element)
        {
            const auto value = static_cast<i8>(shownObject->record.data.resistances[element]);
            std::string text;

            if (value == 0)
            {
                text = std::string(kNoResistance);
                shade = grey;
            }

            if (value > 0)
            {
                text = WinFormat(kWeakFormat, {static_cast<i32>(value)});
                shade = red;
            }

            if (value < 0 && value > kAbsorbFrom)
            {
                text = WinFormat(kResistFormat, {-static_cast<i32>(value)});
                shade = blue;
            }

            if (value <= kAbsorbFrom)
            {
                text = WinFormat(kAbsorbFormat, {kAbsorbFrom - static_cast<i32>(value)});
                shade = green;
            }

            const auto index = static_cast<int>(element);
            owner.smallFont.DrawText(
                owner.drawSurface,
                kElementStep * (index % kElementsPerLine) + kElementValuesX,
                slide + kElementLine * (index / kElementsPerLine) + kElementValuesY,
                text,
                shade,
                kPanelStyle
            );
        }
    }

    if (page.described != nullptr)
    {
        this->DrawDescriptionBox(page.described->record.description.value_or(std::string()));
    }

    if (page.stage == kRefusedStage)
    {
        this->DrawBox(kRefusedX, kRefusedY, kRefusedWidth, kRefusedHeight, kListBoxMode, 0, 0, 0);
        owner.largeFont.DrawText(owner.drawSurface, kRefusedX, kRefusedY, kBareHanded, color, kPanelStyle);
    }
}

// swd32.exe 0x004425C0: the eleven slots of the role chosen ("%-7s%-12s": the slot's name and the object; the slot
// chosen highlighted, the others darker on the list) with the slot pictures of act 0x232A action 0xD
void Menu::EquipDrawSlots(int x, int y, int slide, int width, int chosen)
{
    Game& owner = this->game;
    const EquipPage& page = this->equip;
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    int rowY = y + kSlotRowDown;

    for (int slot = 0; slot <= kLastSlot; ++slot, rowY += kSlotRowHeight)
    {
        int shift = 0;
        Pixel rowColor = color;
        const ItemNode* worn =
            owner.party.equipment[static_cast<std::size_t>(Party::SlotsPerRole * this->roleTab + slot)].get();
        bool tint = page.stage > 0;

        if (chosen == slot)
        {
            shift = -1;

            if (page.stage == 0)
            {
                if (worn == nullptr || worn->id == ItemNode::Empty)
                {
                    tint = false;
                }
                else
                {
                    this->buttonAct.id = kFaceAct;
                    this->buttonAct.action = kButtonAction;
                    owner.actDrawer.Draw(this->buttonAct, this->buttonPress + kButtonX, this->buttonPress + kButtonY);
                }
            }
        }

        if (tint)
        {
            owner.painter.TintPixels(std::span<u16>(&rowColor, 1), kDim, kDim, kDim);
        }

        const std::string text = WinFormat(
            kSlotFormat,
            {Big5(kSlotNames[static_cast<std::size_t>(slot)]),
             worn != nullptr ? ObjectName(worn->record) : std::string()}
        );
        owner.largeFont.DrawText(
            owner.drawSurface,
            shift - slide + x + kSlotTextX,
            rowY + kSlotTextDown,
            text,
            rowColor,
            kPanelStyle
        );

        if (chosen == slot)
        {
            FillBox(
                owner.painter,
                x - slide - kSlotTextX,
                rowY,
                width - kSlotHighlightNarrow,
                kHighlightHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }

    this->gauge.id = kFaceAct;
    this->gauge.action = kSlotPicturesAction;
    owner.acts.Run(this->gauge);

    for (std::size_t picture = 0; picture < kSlotPictures.size(); ++picture)
    {
        this->EquipDrawPicture(
            this->gauge.frame,
            kSlotPictures[picture],
            x - slide + kSlotPictureX,
            y + kSlotPictureY[picture]
        );
    }

    this->gauge.id = 0;
    this->gauge.action = kPictureAction;
}

// swd32.exe 0x00442130: the attack, defence and agility of role `role` (with their equipment bonuses), each also over
// the faces; where the object tried changes one, an arrow and the new value; then the slot's extra figure (a weapon's
// skill, a magic's growth, a guard's recognition)
void Menu::EquipDrawStats(int slot, int role)
{
    Game& owner = this->game;
    const EquipPage& page = this->equip;
    const EquipStats& now = page.stats[static_cast<std::size_t>(std::clamp(role, 0, Party::Roles - 1))];
    const EquipStats& tried = page.stats[kTriedStats];
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    const int slide = this->Shown(EquipSlideValue, page.slide);

    auto line = [&](std::u8string_view label, int value, int changed, int lineY, int topX)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            slide + kStatsX,
            lineY,
            WinFormat(kStatFormat, {label, value}),
            color,
            kPanelStyle
        );
        owner.largeFont.DrawText(
            owner.drawSurface,
            topX,
            kFacesY - slide,
            WinFormat(kNumberFormat, {value}),
            static_cast<Pixel>(owner.textColors.primary[kTopColor]),
            kPanelStyle
        );

        if (value == changed)
        {
            return;
        }

        owner.actDrawer.DrawPicture(value - changed > 0 ? kDownArrow : kUpArrow, 0, slide + kArrowX, lineY);
        owner.mediumFont.DrawText(
            owner.drawSurface,
            slide + kNewValueX,
            lineY,
            WinFormat(kNewValueFormat, {changed}),
            color,
            kPanelStyle
        );
    };

    line(
        kAttackLabel,
        now[kAttackStat] + now[kAttackBonusStat],
        tried[kAttackBonusStat] + tried[kAttackStat],
        kAttackY,
        kAttackTopX
    );
    line(
        kDefenseLabel,
        now[kDefenseBonusStat] + now[kDefenseStat],
        tried[kDefenseStat] + tried[kDefenseBonusStat],
        kDefenseY,
        kDefenseTopX
    );
    line(kAgilityLabel, now[kAgilityStat], tried[kAgilityStat], kAgilityY, slide + kAgilityTopX);

    if (slot == kWeaponSlot)
    {
        if (tried[kPercentStat] != -1)
        {
            owner.mediumFont.DrawText(
                owner.drawSurface,
                slide + kStatsX,
                kExtraY,
                WinFormat(kPercentFormat, {kSkillLabel, tried[kPercentStat]}),
                color,
                kPanelStyle
            );
        }

        return;
    }

    if ((slot == kFirstMagicSlot || slot == kSecondMagicSlot) && tried[kProficiencyStat] != -1)
    {
        const auto value = static_cast<u32>(tried[kProficiencyStat]);
        owner.mediumFont.DrawText(
            owner.drawSurface,
            slide + kStatsX,
            kExtraY,
            WinFormat(
                kGrowthFormat,
                {kGrowthLabel, static_cast<i32>(value & kWordMask), static_cast<i32>(value >> kWordShift)}
            ),
            color,
            kPanelStyle
        );
    }

    if ((slot == kFirstGuardSlot || slot == kSecondGuardSlot) && tried[kFameStat] != -1)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            slide + kStatsX,
            kExtraY,
            WinFormat(kPercentFormat, {kKnownLabel, kFamePercent * tried[kFameStat]}),
            color,
            kPanelStyle
        );
    }
}

// swd32.exe 0x0043BD70: the description box sliding up (Menu::descriptionSlide positive) or down from under the screen
// to its place and the text in it, while it moves or stays up
void Menu::DrawDescriptionBox(std::string_view text)
{
    Game& owner = this->game;

    if (this->descriptionSlide == 0 && this->descriptionTop != kDescriptionUp)
    {
        return;
    }

    const i32 topBefore = this->descriptionTop;
    const int half = this->descriptionSlide >> 1;
    this->descriptionSlide = half;
    int top = this->descriptionTop - half;
    this->descriptionTop = top;

    if (half <= 0)
    {
        if (top > kDescriptionHidden)
        {
            top = kDescriptionHidden;
            this->descriptionTop = top;
            this->descriptionSlide = 0;
        }
    }
    else if (top < kDescriptionUp)
    {
        top = kDescriptionUp;
        this->descriptionTop = top;
        this->descriptionSlide = 0;
    }

    this->Move(DescriptionTopValue, this->descriptionTop, topBefore, this->descriptionTop);
    top = this->Shown(DescriptionTopValue, this->descriptionTop);
    FillBox(
        owner.painter,
        kDescriptionBoxX,
        top - kDescriptionBoxUp,
        kDescriptionBoxWidth,
        kDescriptionBoxBottom - top,
        0,
        0,
        0,
        FillMode::Eighth
    );
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        owner.battleMessages.panelAct.frame,
        kDescriptionTextX,
        top,
        kDescriptionFrameRight,
        kDescriptionFrameBottom,
        0,
        kFrameBorder
    );
    owner.DrawWrappedText(text, kDescriptionTextX, top, kDescriptionLines, kDescriptionWidth);
}

// swd32.exe 0x00442960: picture `picture` of `file` at (x, y), opaque, as ActDrawer::DrawPicture (the original also
// keeps the picture's record at 0x004FCD60, never read)
void Menu::EquipDrawPicture(u16 file, u16 picture, int x, int y)
{
    this->game.actDrawer.DrawPicture(file, picture, x, y);
}

}  // namespace swd3
