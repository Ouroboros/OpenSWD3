#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <string>

#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/databases.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_list.h"
#include "org/database.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// AltarPage::stage.
constexpr i32 kChoosing = 1;
constexpr i32 kAltars = 2;  // the two results shown, an altar to choose
constexpr i32 kMorphing = 3;
constexpr i32 kMade = 4;
constexpr i32 kNeedTwo = 5;  // "請將兩個欄位選擇物品"
constexpr i32 kAltarsAgain = 0xA;

constexpr int kTrialFlag = 0x1BB0;      // the trial version: the page only says so
constexpr int kEastAltarFlag = 0x1BA9;  // the east altar is open
constexpr int kLastTab = 2;
constexpr u32 kEastTooHigh = 1;  // AltarPage::tooHigh
constexpr u32 kWestTooHigh = 2;
constexpr int kTooHighLevels = 9;  // a result this many levels above the first role is refused
constexpr u16 kDefaultResult = 0x65;
constexpr u32 kKindMask = 0x0FFF7FFF;
constexpr u32 kKeptKind = 0x800;  // AltarPage::kinds: a monster of 0x65..0x1F4 without it is never a result
constexpr int kLastMonster = 0x1F4;
constexpr int kLevelBonus = 3;  // the east result aims three levels above the average
constexpr int kNoBest = 0x7FFFFFFF;
constexpr i32 kEastOnly = 1;  // AltarPage::altars: the monster is never the east result
constexpr i32 kWestOnly = 2;
constexpr std::size_t kRecipesAt = 0x1C2;  // in maps.dat table 0x18: four words a recipe, ended by 0xFFFF
constexpr u16 kRecipesEnd = 0xFFFF;
constexpr std::size_t kRecipeWords = 4;
constexpr std::size_t kMatrixSide = 0xF;  // the 15 x 15 result categories at the table's start
// The row of the matrix of each category value 0..20 (0 for the values without one).
constexpr std::array<std::size_t, 21> kMatrixOrder = {0, 1, 2, 3, 4,   5,   6,   7,   8,   9, 0,
                                                      0, 0, 0, 0, 0xE, 0xD, 0xC, 0xA, 0xB, 0};
constexpr int kLastObjectCategory = 9;  // TabAccepts: categories past it count only from 0xF to 0x13
constexpr int kFirstSpecialCategory = 0xF;
constexpr int kLastSpecialCategory = 0x13;

constexpr i32 kPageBackground = 0x3B;  // Menu::background actions
constexpr i32 kAltarsBackground = 0x39;
constexpr i32 kMorphBackground = 0x46;
constexpr i32 kFaceAct = 0x232A;
constexpr i32 kFaceLeaveAction = 3;
constexpr i32 kAltarAct = 0x232C;
constexpr i32 kMorphFrameAction = 0x4E;
constexpr i32 kPictureAction = 0x44;
constexpr u16 kOpenSound = 0x136;
constexpr u16 kMoveSound = 0x2E;
constexpr u16 kSwitchSound = 0x107;
constexpr u16 kRefusedSound = 0x8C;
constexpr u16 kMorphSound = 0xB7;
constexpr u16 kDoneSound = 0x208;

// The morph (AltarMorph): the counter runs from -40.
constexpr i32 kMorphStart = -0x28;
constexpr i32 kMorphShown = -0x23;  // from here the pictures morph
constexpr i32 kTintEnd = -0x1E;
constexpr i32 kSkipTo = 0x23;
constexpr i32 kSettleStart = 0x78;
constexpr u32 kMorphLast = 0x8C;
constexpr i32 kMorphDone = 0xC8;
constexpr i32 kMixStart = 0x32;
constexpr i32 kMixEnd = 0x6E;
constexpr i32 kMixPerFrame = 8;
constexpr i32 kMixBase = 0x190;
constexpr u32 kMixPairs = 0x3354;
constexpr i32 kSoundEnd = 0x50;
constexpr i32 kSoundMask = 7;
constexpr i32 kDrawRange = 0xB4;
constexpr i32 kDrawLast = 0x28;
constexpr i32 kStepSpan = 0x28;
constexpr std::array<int, 3> kSteps = {3, 5, 7};
constexpr int kSettle = 9;  // the offsets shrink to nine tenths a frame
constexpr int kTenths = 0xA;
constexpr int kPictureTop = 0xB4;
constexpr int kLeftPictureX = 0x8C;
constexpr int kRightPictureX = 0x17C;
constexpr int kTintStep = 6;

constexpr u16 kRolePagesCode = 2;  // Menu::code on a role page
constexpr u16 kScrollUp = 3;       // AltarPage::scrollArrows
constexpr u16 kScrollDown = 0x30;
constexpr u16 kScrollUpMask = 0xF;
constexpr u16 kScrollDownMask = 0xF0;
constexpr int kNibbleShift = 4;
constexpr int kSpiritItemA = 2;  // AltarResultVitals: the level times these
constexpr int kSpiritItemB = 3;
constexpr int kSpiritItemC = 5;
constexpr int kSpiritHealA = 2;
constexpr int kSpiritHealB = 4;
constexpr int kStaminaSkillA = 3;
constexpr int kStaminaSkillB = 5;
constexpr int kTintBase = 0x5A;

// The clicks (Menu::clicks).
constexpr u32 kLeftClick = 1;
constexpr u32 kDoubleClick = 2;
constexpr u32 kLeftClicks = 3;
constexpr u32 kRightClicks = 0xC;
constexpr u32 kAnyClick = 0xF;

// The mouse areas.
constexpr u32 kTabsTop = 0x32;
constexpr u32 kTabsBottom = 0x49;
constexpr u32 kTabsLeft = 8;
constexpr u32 kTabsRight = 0x74;
constexpr u32 kTabWidth = 0x24;
constexpr u32 kRowsTop = 0x50;
constexpr u32 kRowsBottom = 0x1D0;
constexpr u32 kRowsLeft = 2;
constexpr u32 kRowsRight = 0xBC;
constexpr u32 kRowHeight = 0x18;
constexpr u32 kSlotsTop = 0x46;
constexpr u32 kSlotsBottom = 0x151;
constexpr u32 kFirstSlotLeft = 0xFE;
constexpr u32 kFirstSlotRight = 0x189;
constexpr u32 kSecondSlotLeft = 0x1D1;
constexpr u32 kSecondSlotRight = 0x25B;
constexpr u32 kRunTop = 0x151;
constexpr u32 kRunBottom = 0x166;
constexpr u32 kRunLeft = 0x19C;
constexpr u32 kRunRight = 0x1C4;
constexpr u32 kBarLeft = 0xC6;
constexpr u32 kBarRight = 0xD6;
constexpr i32 kBarUpTop = 0x4C;
constexpr i32 kBarUpBottom = 0x5A;
constexpr i32 kBarDownTop = 0x1C6;
constexpr i32 kBarDownBottom = 0x1D4;
constexpr u32 kResultsTop = 0x28;
constexpr u32 kResultsBottom = 0x19F;
constexpr u32 kEastLeft = 0xC;
constexpr u32 kEastRight = 0x124;
constexpr u32 kWestLeft = 0x14C;
constexpr u32 kWestRight = 0x264;

// The texts.
constexpr std::u8string_view kTrialOnly = u8"試玩版無此功能";         // swd32.exe 0x004A6CF8
constexpr std::u8string_view kEastAltar = u8"東方祭壇";               // swd32.exe 0x004A6CC8
constexpr std::u8string_view kWestAltar = u8"西方祭壇";               // swd32.exe 0x004A6CBC
constexpr std::u8string_view kChooseAltar = u8"請選擇一個祭壇";       // swd32.exe 0x004A6CAC
constexpr std::u8string_view kChooseTwo = u8"請將兩個欄位選擇物品";   // swd32.exe 0x004A6C94
constexpr std::u8string_view kTooHighText = u8"等級不足 無法訂契約";  // swd32.exe 0x004A6D18
constexpr std::u8string_view kEast = u8"東";                          // swd32.exe 0x004A6D0C
constexpr std::u8string_view kWest = u8"西";                          // swd32.exe 0x004A6D08
constexpr std::u8string_view kAltarFormat = u8"%s方祭壇";             // swd32.exe 0x004A6CD4
constexpr std::u8string_view kSlotFormat = u8"%-8s  %2d級";           // swd32.exe 0x004A6CE0
constexpr std::u8string_view kResultLevelFormat = u8"%d 級";          // swd32.exe 0x004A6D74
constexpr std::u8string_view kHitPointsFormat = u8" 生命  %4d";       // swd32.exe 0x004A6D68
constexpr std::u8string_view kSpiritFormat = u8" 靈力  %4d";
constexpr std::u8string_view kStaminaFormat = u8" 體力  %4d";
constexpr std::u8string_view kAttackFormat = u8" 攻擊  %4d";
constexpr std::u8string_view kDefenseFormat = u8" 防禦  %4d";
constexpr std::u8string_view kAgilityFormat = u8" 敏捷  %4d";
constexpr std::string_view kRowFormat = "%-12s%3d";
constexpr std::string_view kNameFormat = "%-12s";

// The layout.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kGoldRed = 0x15;
constexpr u32 kGoldGreen = 0xF;
constexpr u32 kGoldBlue = 8;
constexpr u32 kDimRed = 0xA;
constexpr u32 kDimGreen = 7;
constexpr u32 kDimBlue = 4;
constexpr u32 kWarnRed = 0x18;
constexpr u32 kWarnGreen = 0xA;
constexpr u32 kWarnBlue = 0xB;
constexpr int kPanelStyle = 4;
constexpr int kTrialX = 0x118;
constexpr int kTrialY = 0xDC;
constexpr int kTrialCharWidth = 0xB;
constexpr int kTrialHeight = 0x16;
constexpr int kTrialBoxMode = 2;
constexpr int kAltarBoxMode = 4;
constexpr int kChosenBoxMode = 2;
constexpr i32 kRunLitAction = 0xF;
constexpr int kRunLitX = 0x198;
constexpr int kRunLitY = 0x150;
constexpr i32 kFirstTabAction = 0x14;
constexpr int kTabStepX = 0x1F;
constexpr int kTabX = 8;
constexpr int kTabY = 0x34;
constexpr i32 kFirstSlotAction = 0x10;
constexpr int kSlotStepX = 0xD8;
constexpr int kSlotX = 0xD0;
constexpr int kSlotStepY = 8;
constexpr int kSlotY = 0x21;
constexpr int kBarX = 0xC5;
constexpr int kBarY = 0x5C;
constexpr int kBarHeight = 0x168;
constexpr int kListTop = 0x4F;
constexpr int kListEnd = 0x1CF;
constexpr int kListX = 0x10;
constexpr int kHighlightX = 4;
constexpr int kHighlightWidth = 0xBE;
constexpr int kHighlightHeight = 0x18;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kSlotPictureStep = 0xD4;
constexpr int kSlotPictureX = 0x108;
constexpr int kSlotPictureY = 0x5E;
constexpr int kSlotBoxStep = 0xEC;
constexpr int kSlotBoxX = 0xF8;
constexpr int kSlotBoxY = 0x164;
constexpr int kSlotBoxWidth = 0x7E;
constexpr int kSlotBoxHeight = 0x24;
constexpr int kSlotNameY = 0x176;
constexpr int kAltarNameStep = 0x122;
constexpr int kAltarNameX = 0xF0;
constexpr int kAltarNameY = 0x1A9;
constexpr int kArrowX = 0x139;
constexpr u16 kUpArrow = 0x2463;  // the result's level at least the average
constexpr u16 kDownArrow = 0x2465;
constexpr int kEastResultX = 0x14;
constexpr int kWestResultX = 0x154;
constexpr int kResultY = 0x28;
constexpr u32 kChosenResult = 1;  // AltarDrawResult flags
constexpr int kTooHighShift = 0xC;
constexpr u32 kTooHighFlag = 0x1000;
constexpr int kPromptX = 0x101;
constexpr int kPromptY = 0x1B8;
constexpr int kPromptWidth = 0x7E;
constexpr int kPromptHeight = 0x12;
constexpr int kResultBoxWidth = 0x118;
constexpr int kResultBoxHeight = 0x177;
constexpr i32 kResultFaceAction = 0x17;
constexpr int kResultFaceX = 8;
constexpr int kResultFaceUp = 0x1E;
constexpr int kResultTitleX = 0x36;
constexpr int kResultTitleUp = 0xA;
constexpr int kResultPictureX = 0x10;
constexpr int kResultPictureY = 0x18;
constexpr int kResultPictureWidth = 0x78;
constexpr int kResultPictureHeight = 0xDC;
constexpr int kResultValuesX = 0x92;
constexpr int kResultLine = 0x19;
constexpr int kTooHighX = 0x40;
constexpr int kTooHighY = 0x120;
constexpr int kMadeX = 0x104;
constexpr int kMadeY = 0xB4;
constexpr int kNeedTwoCharWidth = 0xC;
constexpr int kNeedTwoY = 0xE4;
constexpr int kNeedTwoHeight = 0x18;

}  // namespace

// swd32.exe 0x0043D530: the altar page opened: the category, level, kind and altar of every object 0..0x4AF; the
// inventory's counts free to use, the objects of the first tab moved over, both slots empty; the morph tables made
void Menu::AltarEnter()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    altar.categories.assign(AltarPage::Objects, -1);
    altar.levels.assign(AltarPage::Objects, -1);
    altar.kinds.assign(AltarPage::Objects, ~0u);
    altar.altars.assign(AltarPage::Objects, -1);

    for (int id = 0; id < AltarPage::Objects; ++id)
    {
        ObjectRecord record;

        if (owner.database.LoadObject(record, static_cast<u16>(id)))
        {
            const auto at = static_cast<std::size_t>(id);
            altar.categories[at] = record.data.unk52;
            altar.levels[at] = record.data.level;
            altar.kinds[at] = record.data.kind;
            altar.altars[at] = static_cast<i8>(record.data.unk9B);
        }
    }

    altar.resultEast = ItemNode{};
    altar.resultWest = ItemNode{};

    for (ItemNode* node = owner.party.inventory.get(); node != nullptr; node = node->next.get())
    {
        node->unk06 = static_cast<i16>(node->count + node->pending);
    }

    this->background.id = kFaceAct;
    this->background.action = kPageBackground;
    ActState& panel = owner.battleMessages.panelAct;
    panel.id = BattleMessages::PanelAct;
    panel.action = 0;
    altar.top = 0;
    altar.row = 0;
    altar.stage = kChoosing;
    altar.slot = 0;
    altar.tab = 0;
    this->AltarFillTab();
    altar.firstShown = altar.objects.get();
    altar.count = CountItems(altar.objects);
    CountShown(altar.firstShown, altar.shown, AltarPage::Rows);
    altar.slotA.id = ItemNode::Empty;
    altar.slotB.id = ItemNode::Empty;
    owner.audio.PlaySound(kOpenSound, owner.config.soundVolume);
    altar.columnsA.assign(AltarPage::MorphColumns, 0);
    altar.columnsB.assign(AltarPage::MorphColumns, 0);
    altar.rowsA.assign(AltarPage::MorphRows, 0);
    altar.rowsB.assign(AltarPage::MorphRows, 0);
    altar.phase = 0;
    altar.scrollArrows = 0;
    this->code = kRolePagesCode;
}

// swd32.exe 0x0043D880: the altar page closed: the moved objects back in the inventory, the slots, results and tables
// freed, the main tabs back
void Menu::AltarLeave()
{
    AltarPage& altar = this->altar;
    this->background.id = kFaceAct;
    this->background.action = kAltarsBackground;
    this->face.id = kFaceAct;
    this->face.action = kFaceLeaveAction;
    this->AltarReturnObjects();
    altar.slotA = ItemNode{};
    altar.slotB = ItemNode{};
    altar.resultEast.record.description.reset();
    altar.resultWest.record.description.reset();
    FreeItemList(altar.objects);
    altar.resultEast = ItemNode{};
    altar.resultWest = ItemNode{};
    altar.categories.clear();
    altar.levels.clear();
    altar.kinds.clear();
    altar.altars.clear();
    altar.columnsA.clear();
    altar.columnsB.clear();
    altar.rowsA.clear();
    altar.rowsB.clear();
    this->code = Menu::MainCode;
}

// swd32.exe 0x0043F000: the objects of the tab moved over from the inventory (the page's empty entry when none)
void Menu::AltarFillTab()
{
    AltarPage& altar = this->altar;
    this->AltarReturnObjects();
    this->AltarTakeObjects(this->game.party.inventory, altar.tab);

    if (altar.objects == nullptr)
    {
        altar.objects = NewEmptyItem();
    }

    altar.count = CountItems(altar.objects);
    altar.top = 0;
    altar.row = 0;
    altar.firstShown = altar.objects.get();
    // The original counts from the list's head itself, one more than its objects.
    altar.shown = std::min(altar.count + 1, AltarPage::Rows);
}

// swd32.exe 0x0043F080: the page's objects back at the front of the inventory one by one (their order turns around);
// the empty entry is freed
void Menu::AltarReturnObjects()
{
    AltarPage& altar = this->altar;
    ItemLink& inventory = this->game.party.inventory;

    while (altar.objects != nullptr)
    {
        std::unique_ptr<ItemNode> node = std::move(altar.objects);
        altar.objects = std::move(node->next);

        if (node->id != ItemNode::Empty)
        {
            node->next = std::move(inventory);
            inventory = std::move(node);
        }
    }
}

// swd32.exe 0x0043F0D0: the objects of `source` the tab takes moved into the page's list in the order of their ids
void Menu::AltarTakeObjects(ItemLink& source, int tab)
{
    AltarPage& altar = this->altar;
    altar.objects.reset();
    ItemLink* link = &source;

    while (*link != nullptr)
    {
        if (!AltarTabTakes(**link, tab))
        {
            link = &(*link)->next;
            continue;
        }

        std::unique_ptr<ItemNode> node = std::move(*link);
        *link = std::move(node->next);
        InsertItemById(altar.objects, std::move(node));
    }
}

// swd32.exe 0x0043F160: the page's list sorted again by id
void Menu::AltarSortObjects()
{
    AltarPage& altar = this->altar;
    ItemLink sorted;

    while (altar.objects != nullptr)
    {
        std::unique_ptr<ItemNode> node = std::move(altar.objects);
        altar.objects = std::move(node->next);
        InsertItemById(sorted, std::move(node));
    }

    altar.objects = std::move(sorted);
}

// swd32.exe 0x0043F7C0: whether tab `tab` takes the object: categories 0..9 and 0xF..0x13 only, by kind (bits 15 and
// 28..31 ignored): tab 0 kinds 1, 2, 4, 8, 0x10 and 0x1000, tab 1 kinds 0x100, 0x200 and 0x400, tab 2 kind 0x800
bool AltarTabTakes(const ItemNode& node, int tab)
{
    const u16 category = node.record.data.unk52;

    if (category > kLastObjectCategory && (category < kFirstSpecialCategory || category > kLastSpecialCategory))
    {
        return false;
    }

    switch (node.record.data.kind & kKindMask)
    {
        case 1:
        case 2:
        case 4:
        case 8:
        case 0x10:
        case 0x1000:
            return tab == 0;

        case 0x100:
        case 0x200:
        case 0x400:
            return tab == 1;

        case 0x800:
            return tab == 2;

        default:
            return false;
    }
}

// swd32.exe 0x0043F880: on the list, the object of the row chosen put into the slot chosen; the list counted, sorted
// and fitted again
void Menu::AltarPick()
{
    AltarPage& altar = this->altar;

    if (altar.stage != kChoosing)
    {
        return;
    }

    if (altar.slot == 0)
    {
        this->AltarPut(altar.slotA, altar.top + altar.row);
    }
    else if (altar.slot == 1)
    {
        this->AltarPut(altar.slotB, altar.top + altar.row);
    }

    if (altar.objects == nullptr)
    {
        altar.objects = NewEmptyItem();
    }

    altar.count = CountItems(altar.objects);
    this->AltarSortObjects();
    SkipItems(altar.top, altar.objects, altar.firstShown);
    CountShown(altar.firstShown, altar.shown, AltarPage::Rows);
    FitListRow(altar.count, altar.top, altar.row, altar.shown);
}

// swd32.exe 0x0043F940: the object of list place `index` put into `slot` (one of it taken; with none left the slot
// empties); the object the slot held goes back to the list or the inventory (the original reads through a null node
// when it is in neither)
void Menu::AltarPut(ItemNode& slot, int index)
{
    AltarPage& altar = this->altar;
    ItemNode* chosen = ItemAfter(index, altar.objects);
    const bool listed = AltarTabTakes(slot, altar.tab);
    const u16 previous = slot.id;

    if (chosen != nullptr && chosen->unk06 != 0)
    {
        slot.id = chosen->id;
        slot.record = chosen->record;
        slot.count = 0;
        slot.pending = 0;
        slot.unk06 = 1;
        --chosen->unk06;
    }
    else
    {
        slot.record.description.reset();
        slot.id = ItemNode::Empty;
        slot.count = 0;
        slot.pending = 0;
        slot.unk06 = 1;
    }

    if (previous == ItemNode::Empty)
    {
        return;
    }

    ItemNode* back = FindItemOfKind(listed ? altar.objects : this->game.party.inventory, static_cast<i16>(previous));

    if (back != nullptr)
    {
        ++back->unk06;
    }
}

// swd32.exe 0x0043F1E0: the results of the two slots (none unless both hold an object left): a recipe of maps.dat
// table 0x18 names both; otherwise the east result is the monster of the category the matrix gives for (slot B, slot
// A) nearest above three levels over their average (below it for an odd category of slot B), the west result the one
// of (slot A, slot B) nearest the average (above or below by slot A's category), skipping the monsters 0x65..0x1F4
// without kind bit 0x800 and those barred from that altar. false when a slot is empty.
bool Menu::AltarCombine()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    const ItemNode& first = altar.slotA;
    const ItemNode& second = altar.slotB;
    altar.resultEast = ItemNode{};
    altar.resultWest = ItemNode{};
    altar.resultEast.id = ItemNode::Empty;
    altar.resultWest.id = ItemNode::Empty;

    if (first.id == ItemNode::Empty || first.unk06 == 0 || second.id == ItemNode::Empty || second.unk06 == 0)
    {
        return false;
    }

    auto setResult = [](ItemNode& result, u16 id)
    {
        result.id = id;
        result.count = 0;
        result.pending = 0;
        result.unk06 = 1;
    };

    setResult(altar.resultEast, kDefaultResult);
    setResult(altar.resultWest, kDefaultResult);
    const std::vector<u8>& maps = owner.databases.mapsData;
    const std::size_t table = owner.scene.unk4C97E4;

    for (std::size_t recipe = table + kRecipesAt;; recipe += kRecipeWords * sizeof(u16))
    {
        const u16 a = MapsWord(maps, recipe);

        if (a == kRecipesEnd)
        {
            break;
        }

        const u16 b = MapsWord(maps, recipe + sizeof(u16));

        if ((a == first.id && b == second.id) || (a == second.id && b == first.id))
        {
            setResult(altar.resultEast, MapsWord(maps, recipe + 2 * sizeof(u16)));
            setResult(altar.resultWest, MapsWord(maps, recipe + 3 * sizeof(u16)));
            return true;
        }
    }

    auto order = [](u16 category)
    {
        return category < kMatrixOrder.size() ? kMatrixOrder[category] : 0;
    };

    auto search = [&altar, &maps, table, &order](u16 row, u16 column, int target, bool below, i32 barred)
    {
        const u16 category = MapsWord(maps, table + sizeof(u16) * (order(column) + kMatrixSide * order(row)));
        int best = kNoBest;
        u16 found = 0;

        for (int id = kDefaultResult; id < AltarPage::Objects; ++id)
        {
            const auto at = static_cast<std::size_t>(id);

            if (altar.categories[at] != category)
            {
                continue;
            }

            const int distance = altar.levels[at] - target;
            bool better = false;

            if (below)
            {
                better = distance <= 0 && target - altar.levels[at] < best;
            }
            else
            {
                better = distance >= 0 && distance < best;
            }

            if (id <= kLastMonster && (altar.kinds[at] & kKeptKind) == 0)
            {
                better = false;
            }

            if (altar.altars[at] == barred || !better)
            {
                continue;
            }

            found = static_cast<u16>(id);
            best = below ? target - altar.levels[at] : distance;
        }

        return found;
    };

    const int average = (second.record.data.level + first.record.data.level) / 2;
    const u16 east = search(
        first.record.data.unk52,
        second.record.data.unk52,
        average + kLevelBonus,
        static_cast<u8>(second.record.data.unk52) % 2 != 0,
        kEastOnly
    );
    setResult(altar.resultEast, east != 0 ? east : kDefaultResult);
    owner.database.LoadObject(altar.resultEast.record, altar.resultEast.id);
    const u16 west = search(
        second.record.data.unk52,
        first.record.data.unk52,
        average,
        static_cast<u8>(first.record.data.unk52) % 2 != 0,
        kWestOnly
    );
    setResult(altar.resultWest, west != 0 ? west : kDefaultResult);
    owner.database.LoadObject(altar.resultWest.record, altar.resultWest.id);
    return true;
}

// The list moved (inline in the altar page's keys): the first row shown, the object picked into the slot, the
// results.
void Menu::AltarMoved(u16 arrows)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    SkipItems(altar.top, altar.objects, altar.firstShown);
    CountShown(altar.firstShown, altar.shown, AltarPage::Rows);
    this->AltarPick();
    this->AltarCombine();
    altar.scrollArrows = static_cast<u16>(altar.scrollArrows | arrows);
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// The east altar chosen when it is open and the result not too high (inline in the altar page's keys).
void Menu::AltarChooseEast(bool soundFirst)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;

    if (soundFirst && altar.chosen != 0)
    {
        owner.audio.PlaySound(kSwitchSound, owner.config.soundVolume);
    }

    if (owner.state.GameFlag(kEastAltarFlag) && (altar.tooHigh & kEastTooHigh) == 0)
    {
        if (!soundFirst && altar.chosen != 0)
        {
            owner.audio.PlaySound(kSwitchSound, owner.config.soundVolume);
        }

        altar.chosen = 0;
    }
}

// The west altar chosen unless its result is too high (inline in the altar page's keys).
void Menu::AltarChooseWest()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;

    if (altar.chosen != 1)
    {
        owner.audio.PlaySound(kSwitchSound, owner.config.soundVolume);
    }

    if ((altar.tooHigh & kWestTooHigh) == 0)
    {
        altar.chosen = 1;
    }
}

// swd32.exe 0x0043DD20: down: the next object (into the slot); on the altars the west one; during the morph its end
void Menu::AltarDown()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        ListDown(altar.count, altar.top, altar.row, altar.shown);
        this->AltarMoved(kScrollDown);
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseWest();
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// swd32.exe 0x0043DDF0: up: the object before; on the altars the east one
void Menu::AltarUp()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        ListUp(altar.top, altar.row);
        this->AltarMoved(kScrollUp);
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseEast(false);
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// swd32.exe 0x0043DED0: Page Down: a page of sixteen on; on the altars the west one
void Menu::AltarPageDown()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        ListPageDown(altar.count, altar.top, altar.row, altar.shown, AltarPage::Rows);
        this->AltarMoved(kScrollDown);
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseWest();
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// swd32.exe 0x0043DFA0: Page Up: a page of sixteen back; on the altars the east one
void Menu::AltarPageUp()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        ListPageUp(altar.top, altar.row, AltarPage::Rows);
        this->AltarMoved(kScrollUp);
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseEast(false);
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// The tab changed (inline in 0x0043E080 and 0x0043E170): its objects listed, the first one picked.
void Menu::AltarTabChanged()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    this->AltarFillTab();
    altar.top = 0;
    altar.row = 0;
    altar.shown = AltarPage::Rows;
    MenuList view{altar.count, altar.top, altar.row, altar.shown, altar.firstShown};
    this->description = FitMenuList(view, altar.objects, AltarPage::Rows, owner.database, owner.databases.mapsData);
    altar.count = view.count;
    altar.top = view.top;
    altar.row = view.row;
    altar.shown = view.shown;
    altar.firstShown = view.first;
    this->AltarPick();
    this->AltarCombine();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0043E080: left: the tab before (wrapping); on the altars the east one
void Menu::AltarLeft()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        if (--altar.tab < 0)
        {
            altar.tab = kLastTab;
        }

        this->AltarTabChanged();
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseEast(true);
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// swd32.exe 0x0043E170: right: the next tab (wrapping); on the altars the west one
void Menu::AltarRight()
{
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        if (++altar.tab > kLastTab)
        {
            altar.tab = 0;
        }

        this->AltarTabChanged();
    }
    else if (altar.stage == kAltars)
    {
        this->AltarChooseWest();
    }
    else if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// The other slot or altar (inline in 0x0043E250 and 0x0043E310).
void Menu::AltarSwitch(u16 slotSound)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;

    if (altar.stage == kChoosing)
    {
        if (++altar.slot > 1)
        {
            altar.slot = 0;
        }

        owner.audio.PlaySound(slotSound, owner.config.soundVolume);
        return;
    }

    if (altar.stage == kAltars)
    {
        if (++altar.chosen > 1)
        {
            altar.chosen = 0;
        }

        if (owner.state.GameFlag(kEastAltarFlag))
        {
            altar.chosen = 1;
        }

        if ((altar.tooHigh & kEastTooHigh) == 0)
        {
            altar.chosen = 1;
        }

        if ((altar.tooHigh & kWestTooHigh) == 0)
        {
            altar.chosen = 0;
        }

        owner.audio.PlaySound(kSwitchSound, owner.config.soundVolume);
        return;
    }

    if (altar.stage == kMorphing)
    {
        altar.counter = kMorphDone;
    }
}

// swd32.exe 0x0043E250: Shift: the other slot; on the altars the other one
void Menu::AltarShift()
{
    this->AltarSwitch(kSwitchSound);
}

// swd32.exe 0x0043E310: End: as Shift, the slot changing with the move sound
void Menu::AltarEnd()
{
    this->AltarSwitch(kMoveSound);
}

// swd32.exe 0x0043E3D0: Space or Enter: with both slots set the altars are offered (a result more than nine levels
// above the first role refused there); the altar chosen starts the morph (a refused one sounds); during the morph it
// ends; once made, the two objects are spent and the result added (to the list or the inventory) and the list starts
// again. The trial version leaves.
void Menu::AltarSelect()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;

    switch (altar.stage)
    {
        case kChoosing:
        {
            if (owner.state.GameFlag(kTrialFlag))
            {
                this->AltarCancel();
                return;
            }

            if (!this->AltarCombine())
            {
                altar.stage = kNeedTwo;
                return;
            }

            this->AltarResultVitals();
            this->background.id = kFaceAct;
            ++altar.stage;
            this->background.action = kAltarsBackground;
            altar.chosen = owner.state.GameFlag(kEastAltarFlag) ? 0 : 1;
            altar.tooHigh = 0;
            const int level = owner.party.roles[0].level;

            if (owner.state.GameFlag(kEastAltarFlag) && altar.resultEast.record.data.level - level > kTooHighLevels)
            {
                altar.chosen = 1;
                altar.tooHigh |= kEastTooHigh;
            }

            if (altar.resultWest.record.data.level - level > kTooHighLevels)
            {
                altar.tooHigh |= kWestTooHigh;
            }

            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;
        }

        case kAltars:
            if ((altar.chosen == 0 && (altar.tooHigh & kEastTooHigh) != 0) ||
                (altar.chosen == 1 && (altar.tooHigh & kWestTooHigh) != 0))
            {
                owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
                return;
            }

            altar.stage = kMorphing;
            altar.counter = kMorphStart;
            this->AltarPrepareMorph();
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kMorphing:
            this->AltarFinishMorph();

            if (altar.counter < kMorphShown)
            {
                altar.counter = kSkipTo;
                this->background.id = kFaceAct;
                this->background.action = kMorphBackground;
            }

            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kMade:
            this->AltarSpend();
            return;

        case kNeedTwo:
        case kAltarsAgain:
            altar.stage = kChoosing;
            return;

        default:
            return;
    }
}

// The made result taken (inline in 0x0043E3D0, stage 4).
void Menu::AltarSpend()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    this->tint = 0;

    auto spend = [&owner, &altar](ItemNode& slot)
    {
        const bool listed = AltarTabTakes(slot, altar.tab);

        if (slot.id == ItemNode::Empty)
        {
            return;
        }

        AddItem(listed ? altar.objects : owner.party.inventory, slot.id, -1, ItemCountMode::Pending, owner.database);
        slot.record.description.reset();
        slot.id = ItemNode::Empty;
    };

    spend(altar.slotA);
    spend(altar.slotB);
    const ItemNode& result = altar.chosen != 0 ? altar.resultWest : altar.resultEast;
    const bool listed = AltarTabTakes(result, altar.tab);

    if (result.id != ItemNode::Empty)
    {
        ItemNode* added = AddItem(
            listed ? altar.objects : owner.party.inventory,
            result.id,
            1,
            ItemCountMode::PendingOnly,
            owner.database
        );

        if (added != nullptr)
        {
            ++added->unk06;
        }
    }

    altar.count = CountItems(altar.objects);
    SkipItems(altar.top, altar.objects, altar.firstShown);
    CountShown(altar.firstShown, altar.shown, AltarPage::Rows);
    MenuList view{altar.count, altar.top, altar.row, altar.shown, altar.firstShown};
    this->description = FitMenuList(view, altar.objects, AltarPage::Rows, owner.database, owner.databases.mapsData);
    altar.count = view.count;
    altar.top = view.top;
    altar.row = view.row;
    altar.shown = view.shown;
    altar.firstShown = view.first;
    altar.counter = 0;
    altar.chosen = 0;
    altar.stage = kChoosing;
    this->background.id = kFaceAct;
    this->background.action = kPageBackground;
}

// swd32.exe 0x0043E770: Escape: from the list the page closes (to the main tabs); from the altars back to the list;
// during the morph or once made as Space; from the two slots message back to the list
void Menu::AltarCancel()
{
    AltarPage& altar = this->altar;

    switch (altar.stage)
    {
        case kChoosing:
            if (--this->code == 0)
            {
                this->game.menuState = Game::NoMenu;
            }

            this->SetPage(this->code);
            this->AltarLeave();
            return;

        case kAltars:
            altar.stage = kChoosing;
            this->background.id = kFaceAct;
            this->background.action = kPageBackground;
            return;

        case kMorphing:
        case kMade:
            this->AltarSelect();
            return;

        case kNeedTwo:
            altar.stage = kChoosing;
            return;

        default:
            return;
    }
}

// swd32.exe 0x0043DA30: the mouse on the altar page. On the list: a click on a tab chooses it, on a row picks it, on a
// slot chooses the slot, on "執行" selects (the pointer over it lights it); a held left button on the scroll bar
// scrolls. On the altars a click on the open east or west result chooses it (a double click on the chosen one
// selects). During the morph or once made any click selects; on the two slots message any click goes back. A right
// click goes back.
void Menu::AltarFrame()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);
    const u32 clicks = this->clicks;

    auto inside = [](u32 value, u32 low, u32 high)
    {
        return value < high && value > low;
    };

    altar.hover = 0;

    if (altar.stage == kChoosing)
    {
        if ((clicks & kLeftClicks) != 0 && inside(y, kTabsTop, kTabsBottom) && inside(x, kTabsLeft, kTabsRight))
        {
            altar.tab = static_cast<i32>((x - kTabsLeft) / kTabWidth) + 1;
            this->AltarLeft();
            return;
        }

        if ((clicks & kLeftClicks) != 0)
        {
            if (inside(y, kRowsTop, kRowsBottom) && inside(x, kRowsLeft, kRowsRight))
            {
                const auto row = static_cast<i32>((y - kRowsTop) / kRowHeight);

                if (row < altar.shown)
                {
                    altar.row = row + 1;
                    this->AltarUp();
                }

                return;
            }

            if (inside(y, kSlotsTop, kSlotsBottom) && inside(x, kFirstSlotLeft, kFirstSlotRight))
            {
                altar.slot = 1;
                this->AltarEnd();
                return;
            }

            if (inside(y, kSlotsTop, kSlotsBottom) && inside(x, kSecondSlotLeft, kSecondSlotRight))
            {
                altar.slot = 0;
                this->AltarEnd();
                return;
            }
        }

        altar.hover = 0;

        if (inside(y, kRunTop, kRunBottom) && inside(x, kRunLeft, kRunRight))
        {
            altar.hover = 1;

            if ((clicks & kLeftClicks) != 0)
            {
                this->AltarSelect();
                return;
            }
        }

        if (altar.count > AltarPage::Rows && owner.controls.Repeating(Button::MouseLeft) &&
            inside(x, kBarLeft, kBarRight))
        {
            const MenuScrollBar& bar = this->scrollBar;
            const i32 pointerY = owner.state.pointerY;

            if (pointerY < kBarUpBottom && pointerY > kBarUpTop)
            {
                this->AltarUp();
            }

            if (owner.state.pointerY < kBarDownBottom && owner.state.pointerY > kBarDownTop)
            {
                this->AltarDown();
            }

            if (owner.state.pointerY < bar.thumbTop && owner.state.pointerY > bar.top)
            {
                this->AltarPageUp();
            }

            if (owner.state.pointerY < bar.bottom && owner.state.pointerY > bar.thumbBottom)
            {
                this->AltarPageDown();
            }

            return;
        }
    }
    else if (altar.stage == kAltars)
    {
        const bool inResults = inside(y, kResultsTop, kResultsBottom);

        if (inResults && inside(x, kEastLeft, kEastRight) && owner.state.GameFlag(kEastAltarFlag) &&
            (altar.tooHigh & kEastTooHigh) == 0)
        {
            if ((clicks & kLeftClick) != 0)
            {
                this->AltarLeft();
            }

            if ((clicks & kDoubleClick) == 0)
            {
                if ((clicks & kRightClicks) != 0)
                {
                    this->AltarCancel();
                }

                return;
            }

            if (altar.chosen != 0)
            {
                this->AltarLeft();
                return;
            }

            this->AltarSelect();
            return;
        }

        if (inResults && inside(x, kWestLeft, kWestRight) && (altar.tooHigh & kWestTooHigh) == 0)
        {
            if ((clicks & kLeftClick) != 0)
            {
                this->AltarRight();
            }

            if ((clicks & kDoubleClick) == 0)
            {
                if ((clicks & kRightClicks) != 0)
                {
                    this->AltarCancel();
                }

                return;
            }

            if (altar.chosen != 1)
            {
                this->AltarRight();
                return;
            }

            this->AltarSelect();
            return;
        }
    }
    else if (altar.stage == kMorphing || altar.stage == kMade)
    {
        if ((clicks & kAnyClick) != 0)
        {
            this->AltarSelect();
        }

        return;
    }
    else
    {
        if (altar.stage == kNeedTwo && (clicks & kAnyClick) != 0)
        {
            this->AltarCancel();
        }

        return;
    }

    if ((clicks & kRightClicks) != 0)
    {
        this->AltarCancel();
    }
}

// swd32.exe 0x0043E800: the altar page. The trial version only says it lacks the page. On the list: the tab, the slot
// chosen, the scroll bar, the rows ("%-12s%3d": name and count free), the two slots (picture, category and level,
// name) and, with both results known, the altars that are open with an up or down arrow by the result's level against
// the average. On the altars the two results; the morph (darkening in, then the pictures run together); the result
// made; the two slots message.
void Menu::AltarDraw()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));

    if (owner.state.GameFlag(kTrialFlag))
    {
        const std::string text = Big5(kTrialOnly);
        this->DrawBox(
            kTrialX,
            kTrialY,
            kTrialCharWidth * static_cast<int>(text.size()),
            kTrialHeight,
            kTrialBoxMode,
            0,
            0,
            0
        );
        owner.largeFont.DrawText(owner.drawSurface, kTrialX, kTrialY, text, color, kPanelStyle);
        return;
    }

    if (altar.hover == 1)
    {
        this->face.id = kAltarAct;
        this->face.action = kRunLitAction;
        owner.actDrawer.Draw(this->face, kRunLitX, kRunLitY);
    }

    if (altar.stage == kChoosing || altar.stage == kNeedTwo)
    {
        this->AltarDrawList(color);
    }

    if (altar.stage == kAltars || altar.stage == kAltarsAgain)
    {
        this->AltarDrawResults(color);
    }

    if (altar.stage == kMorphing)
    {
        if (altar.counter > kMorphShown)
        {
            this->background.id = kFaceAct;
            this->background.action = kMorphBackground;
            this->tint = 0;

            if (altar.counter <= kTintEnd)
            {
                this->tint = 2 * (3 * altar.counter + kTintBase);
            }

            this->AltarMorph();

            if (++altar.counter > static_cast<i32>(kMorphLast))
            {
                altar.counter = kMorphDone;
                this->AltarFinishMorph();
            }
            else if (altar.counter >= kMorphDone)
            {
                this->AltarFinishMorph();
            }
        }
        else
        {
            this->AltarDrawResults(color);
            this->tint = kTintStep * (kMorphStart - altar.counter);

            if (altar.counter == kMorphShown)
            {
                this->background.id = kFaceAct;
                this->background.action = kMorphBackground;
            }

            ++altar.counter;
        }

        return;
    }

    if (altar.stage == kMade)
    {
        const ItemNode& result = altar.chosen == 1 ? altar.resultWest : altar.resultEast;
        this->gauge.id = result.record.data.unk50;
        this->gauge.action = kPictureAction;
        this->gauge.direction = 0;
        owner.actDrawer.Draw(this->gauge, kMadeX, kMadeY);
    }

    if (altar.stage == kNeedTwo)
    {
        const std::string text = Big5(kChooseTwo);
        const int width = kNeedTwoCharWidth * static_cast<int>(text.size());
        const int x = Display::Width / 2 - width / 2;
        this->DrawBox(x, kNeedTwoY, width, kNeedTwoHeight, kAltarBoxMode, 0, 0, 0);
        owner.largeFont.DrawText(owner.drawSurface, x, kNeedTwoY, text, color, kPanelStyle);
    }
}

// The list of the altar page with its slots (inline in 0x0043E800, stages 1 and 5).
void Menu::AltarDrawList(Pixel color)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    this->face.id = kAltarAct;
    this->face.action = altar.tab + kFirstTabAction;
    owner.actDrawer.Draw(this->face, kTabStepX * altar.tab + kTabX, kTabY);

    if (altar.slot <= 1)
    {
        this->face.action = altar.slot + kFirstSlotAction;
        owner.actDrawer.Draw(this->face, kSlotStepX * altar.slot + kSlotX, kSlotStepY * altar.slot + kSlotY);
    }

    if (altar.count > AltarPage::Rows)
    {
        int arrows = 0;

        if ((altar.scrollArrows & kScrollUpMask) != 0)
        {
            altar.scrollArrows =
                static_cast<u16>((altar.scrollArrows & ~kScrollUpMask) | ((altar.scrollArrows & kScrollUpMask) - 1));
            arrows = 1;
        }

        if ((altar.scrollArrows & kScrollDownMask) != 0)
        {
            const int down = (altar.scrollArrows & kScrollDownMask) >> kNibbleShift;
            arrows |= 2;
            altar.scrollArrows =
                static_cast<u16>((altar.scrollArrows & ~kScrollDownMask) | ((down - 1) << kNibbleShift));
        }

        const auto count = static_cast<double>(altar.count);
        const auto to = static_cast<float>(static_cast<double>(altar.top + altar.shown) / count);
        const auto from = static_cast<float>(static_cast<double>(altar.top) / count);
        this->DrawScrollBar(kBarX, kBarY, kBarHeight, arrows, from, to, this->scrollBar);
    }

    int rowY = kListTop;
    int row = 0;

    for (const ItemNode* node = altar.firstShown; node != nullptr && rowY < kListEnd;
         node = node->next.get(), ++row, rowY += static_cast<int>(kRowHeight))
    {
        const std::string text =
            WinFormat(kRowFormat, {ObjectName(node->record), static_cast<i32>(static_cast<u16>(node->unk06))});
        owner.largeFont.DrawText(owner.drawSurface, kListX, rowY + 1, text, color, kPanelStyle);

        if (altar.row == row)
        {
            FillBox(
                owner.painter,
                kHighlightX,
                rowY,
                kHighlightWidth,
                kHighlightHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }

    const int average = (altar.slotA.record.data.level + altar.slotB.record.data.level) / 2;

    for (int side = 0; side < 2; ++side)
    {
        const ItemNode& result = side == 1 ? altar.resultWest : altar.resultEast;
        const ItemNode& slot = side == 1 ? altar.slotB : altar.slotA;

        if (slot.id == ItemNode::Empty)
        {
            continue;
        }

        if (slot.record.data.unk50 != 0)
        {
            this->gauge.id = slot.record.data.unk50;
            this->gauge.action = kPictureAction;
            this->gauge.direction = 0;
            owner.actDrawer.Draw(this->gauge, kSlotPictureStep * side + kSlotPictureX, kSlotPictureY);
        }

        const int boxX = kSlotBoxStep * side + kSlotBoxX;
        this->DrawBox(boxX, kSlotBoxY, kSlotBoxWidth, kSlotBoxHeight, kAltarBoxMode, 0, 0, 0);
        const std::u8string_view category = CategoryName(slot.record.data.unk52);
        owner.mediumFont.DrawText(
            owner.drawSurface,
            boxX,
            kSlotBoxY,
            WinFormat(kSlotFormat, {category, static_cast<i32>(slot.record.data.level)}),
            color,
            kPanelStyle
        );
        owner.mediumFont.DrawText(
            owner.drawSurface,
            boxX,
            kSlotNameY,
            WinFormat(kNameFormat, {ObjectName(slot.record)}),
            color,
            kPanelStyle
        );

        if ((side == 1 || owner.state.GameFlag(kEastAltarFlag)) && altar.resultEast.id != ItemNode::Empty &&
            altar.resultWest.id != ItemNode::Empty)
        {
            owner.mediumFont.DrawText(
                owner.drawSurface,
                kAltarNameStep * side + kAltarNameX,
                kAltarNameY,
                WinFormat(kAltarFormat, {side == 0 ? kEast : kWest}),
                color,
                kPanelStyle
            );
            const u16 arrow = result.record.data.level >= average ? kUpArrow : kDownArrow;
            const Picture* picture = owner.archive.GetPicture(arrow, 0);

            if (picture != nullptr)
            {
                owner.painter.SetSource(picture->data.data());
                owner.painter
                    .DrawPicture(kAltarNameStep * side + kArrowX, kAltarNameY, picture->width, picture->height, 0);
            }
        }
    }
}

// The two results and the altar prompt (inline in 0x0043E800, stages 2, 3 and 0xA).
void Menu::AltarDrawResults(Pixel color)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;

    if (owner.state.GameFlag(kEastAltarFlag))
    {
        this->AltarDrawResult(
            altar.resultEast,
            kEastAltar,
            kEastResultX,
            kResultY,
            (altar.chosen == 0 ? kChosenResult : 0) | ((altar.tooHigh & kEastTooHigh) << kTooHighShift)
        );
    }

    this->AltarDrawResult(
        altar.resultWest,
        kWestAltar,
        kWestResultX,
        kResultY,
        (altar.chosen == 1 ? kChosenResult : 0) | ((altar.tooHigh & kWestTooHigh) << (kTooHighShift - 1))
    );
    this->DrawBox(kPromptX, kPromptY, kPromptWidth, kPromptHeight, kAltarBoxMode, 0, 0, 0);
    owner.mediumFont.DrawText(owner.drawSurface, kPromptX, kPromptY, kChooseAltar, color, kPanelStyle);
}

// swd32.exe 0x0043FA70: a result in its box at (x, y): the title with its face, the picture, the category, name,
// level and values (spirit and stamina of the side's result); darker and boxed over when not chosen or too high, then
// "等級不足 無法訂契約"
void Menu::AltarDrawResult(const ItemNode& result, std::u8string_view title, int x, int y, u32 flags)
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    const auto gold = static_cast<Pixel>(owner.painter.MakeColor(kGoldRed, kGoldGreen, kGoldBlue));
    Pixel text = gold;
    int mode = kChosenBoxMode;
    const bool dim = (flags & kChosenResult) == 0 || (flags & kTooHighFlag) != 0;

    if (dim)
    {
        text = static_cast<Pixel>(owner.painter.MakeColor(kDimRed, kDimGreen, kDimBlue));
        mode = kAltarBoxMode;
    }

    this->DrawBox(x, y, kResultBoxWidth, kResultBoxHeight, mode, 0, 0, 0);
    this->face.id = kAltarAct;
    this->face.action = kResultFaceAction;
    owner.actDrawer.Draw(this->face, x + kResultFaceX, y - kResultFaceUp);
    owner.mediumFont.DrawText(owner.drawSurface, x + kResultTitleX, y - kResultTitleUp, title, gold, kPanelStyle);
    const ObjectData& data = result.record.data;
    this->gauge.id = data.unk50;
    this->gauge.action = kPictureAction;
    this->gauge.direction = 0;
    owner.actDrawer.Draw(this->gauge, x + kResultPictureX, y + kResultPictureY);
    const int valuesX = x + kResultValuesX;
    const std::size_t side = x <= Display::Width / 2 ? 0 : 2;
    const std::array<std::string, 9> lines = {
        Big5(CategoryName(data.unk52)),
        ObjectName(result.record),
        WinFormat(kResultLevelFormat, {static_cast<i32>(data.level)}),
        WinFormat(kHitPointsFormat, {static_cast<i32>(data.hitPoints)}),
        WinFormat(kSpiritFormat, {static_cast<i32>(altar.vitals[side])}),
        WinFormat(kStaminaFormat, {static_cast<i32>(altar.vitals[side + 1])}),
        WinFormat(kAttackFormat, {static_cast<i32>(data.attack)}),
        WinFormat(kDefenseFormat, {static_cast<i32>(data.defense)}),
        WinFormat(kAgilityFormat, {static_cast<i32>(data.agility)}),
    };

    for (std::size_t line = 0; line < lines.size(); ++line)
    {
        owner.largeFont.DrawText(
            owner.drawSurface,
            valuesX,
            y + kResultPictureY + kResultLine * static_cast<int>(line),
            lines[line],
            text,
            kPanelStyle
        );
    }

    if (!dim)
    {
        return;
    }

    FillBox(
        owner.painter,
        x + kResultPictureX,
        y + kResultPictureY,
        kResultPictureWidth,
        kResultPictureHeight,
        0,
        0,
        0,
        FillMode::Quarter
    );

    if ((flags & kTooHighFlag) != 0)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            x + kTooHighX,
            y + kTooHighY,
            kTooHighText,
            static_cast<Pixel>(owner.painter.MakeColor(kWarnRed, kWarnGreen, kWarnBlue)),
            kPanelStyle
        );
    }
}

// swd32.exe 0x0043FDE0: the morph prepared: its offsets cleared, the frame picture and the pictures of the result
// chosen and of the two slots unpacked
void Menu::AltarPrepareMorph()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    std::fill(altar.columnsA.begin(), altar.columnsA.end(), i16{0});
    std::fill(altar.columnsB.begin(), altar.columnsB.end(), i16{0});
    std::fill(altar.rowsA.begin(), altar.rowsA.end(), i16{0});
    std::fill(altar.rowsB.begin(), altar.rowsB.end(), i16{0});

    auto unpack = [this, &owner](i32 act, i32 action, std::vector<u16>& pixels)
    {
        this->gauge.id = act;
        this->gauge.action = action;
        this->gauge.direction = 0;

        if (owner.acts.Run(this->gauge) == 0)
        {
            return false;
        }

        const Picture* picture = owner.archive.GetPicture(this->gauge.frame, this->gauge.picture);
        pixels.clear();

        if (picture != nullptr)
        {
            const std::optional<RawPicture> raw = owner.codec.Decode(picture->data);

            if (raw.has_value())
            {
                pixels.assign(raw->pixels.size() / sizeof(u16), 0);
                std::memcpy(pixels.data(), raw->pixels.data(), pixels.size() * sizeof(u16));
            }
        }

        return true;
    };

    if (!unpack(kAltarAct, kMorphFrameAction, altar.frame))
    {
        return;
    }

    const ItemNode& result = altar.chosen != 0 ? altar.resultWest : altar.resultEast;

    if (!unpack(result.record.data.unk50, kPictureAction, altar.result))
    {
        return;
    }

    if (!unpack(altar.slotA.record.data.unk50, kPictureAction, altar.pictureA))
    {
        return;
    }

    unpack(altar.slotB.record.data.unk50, kPictureAction, altar.pictureB);
}

// swd32.exe 0x004400A0: a frame of the morph. Up to 0x8C: before 0x78 the columns and rows wander (more as the counter
// grows), pixel pairs of the result spread into both slot pictures from 0x32 (all of it at 0x6E, with its sound; the
// stirring sound before 0x50); from 0x78 the wandering settles. The two slot pictures are drawn displaced, closing in
// from the sides. The original reads its row table past either end and writes past the screen for large offsets; here
// those pixels are left out.
void Menu::AltarMorph()
{
    Game& owner = this->game;
    AltarPage& altar = this->altar;
    const i32 counter = altar.counter;
    const int pitch = owner.painter.ScreenPitch();

    auto settle = [](std::vector<i16>& values)
    {
        for (i16& value : values)
        {
            value = static_cast<i16>(static_cast<i16>(kSettle * value) / kTenths);
        }
    };

    if (static_cast<u32>(counter) <= kMorphLast)
    {
        if (counter >= kSettleStart)
        {
            settle(altar.columnsA);
            settle(altar.columnsB);
            settle(altar.rowsA);
            settle(altar.rowsB);
        }
        else
        {
            if (counter >= kMixStart && counter < kMixEnd && kMixPerFrame * counter - kMixBase > 0)
            {
                for (int pair = kMixPerFrame * counter - kMixBase; pair > 0; --pair)
                {
                    const auto at = static_cast<std::size_t>(2 * owner.random.Below(kMixPairs));

                    for (std::size_t half = 0; half < 2; ++half)
                    {
                        const u16 pixel = at + half < altar.result.size() ? altar.result[at + half] : u16{0};

                        if (at + half < altar.pictureA.size())
                        {
                            altar.pictureA[at + half] = pixel;
                        }

                        if (at + half < altar.pictureB.size())
                        {
                            altar.pictureB[at + half] = pixel;
                        }
                    }
                }
            }

            if (counter < kSoundEnd && (counter & kSoundMask) != kSoundMask)
            {
                owner.audio.PlaySound(kMorphSound, owner.config.soundVolume);
            }

            if (counter == kMixEnd)
            {
                owner.audio.PlaySound(kDoneSound, owner.config.soundVolume);
                altar.pictureA = altar.result;
                altar.pictureB = altar.result;
            }

            const int step = kSteps[static_cast<std::size_t>(std::clamp(counter / kStepSpan, 0, 2))];
            const int half = step / 2;

            for (std::size_t column = 0; column < altar.columnsA.size(); ++column)
            {
                const auto change =
                    static_cast<i16>(static_cast<int>(owner.random.Below(static_cast<u32>(step))) - half);
                altar.columnsA[column] = static_cast<i16>(altar.columnsA[column] + change);
                altar.columnsB[column] = static_cast<i16>(altar.columnsB[column] - change);
            }

            for (std::size_t row = 0; row < altar.rowsA.size(); ++row)
            {
                const auto change =
                    static_cast<i16>(static_cast<int>(owner.random.Below(static_cast<u32>(step))) - half);
                altar.rowsA[row] = static_cast<i16>(altar.rowsA[row] + change);
                altar.rowsB[row] = static_cast<i16>(altar.rowsB[row] - change);
            }
        }
    }

    if (kDrawRange - counter < kDrawLast)
    {
        return;
    }

    const i32 shift = std::clamp(counter, 0, kSettleStart);
    const std::size_t screen = static_cast<std::size_t>(pitch) * Display::Height;

    auto draw = [&](const std::vector<u16>& picture, std::ptrdiff_t base)
    {
        std::size_t pixel = 0;

        for (std::size_t row = 0; row < altar.rowsA.size(); ++row, base += pitch)
        {
            for (std::size_t column = 0; column < altar.columnsA.size(); ++column, ++pixel)
            {
                const std::ptrdiff_t at = base + static_cast<std::ptrdiff_t>(column) + altar.rowsA[row] -
                                          static_cast<std::ptrdiff_t>(altar.columnsA[column]) * pitch;

                if (at >= 0 && static_cast<std::size_t>(at) < screen && pixel < picture.size())
                {
                    owner.drawSurface[at] = picture[pixel];
                }
            }
        }
    };

    draw(altar.pictureA, static_cast<std::ptrdiff_t>(kPictureTop) * pitch + shift + kLeftPictureX);
    draw(altar.pictureB, static_cast<std::ptrdiff_t>(kPictureTop) * pitch - shift + kRightPictureX);

    if (++altar.phase >= static_cast<i32>(AltarPage::MorphColumns))
    {
        altar.phase = 0;
    }
}

// swd32.exe 0x004405C0: the morph over: the result made (stage 4), the pictures freed
void Menu::AltarFinishMorph()
{
    AltarPage& altar = this->altar;
    this->tint = 0;
    altar.stage = kMade;
    altar.result.clear();
    altar.frame.clear();
    altar.pictureA.clear();
    altar.pictureB.clear();
    altar.phase = 0;
}

// swd32.exe 0x004404D0: the spirit and stamina of the two results from their levels and the objects and skills they
// carry (as the bestiary's)
void Menu::AltarResultVitals()
{
    AltarPage& altar = this->altar;
    altar.vitals.fill(0);

    for (std::size_t side = 0; side < 2; ++side)
    {
        const ObjectData& data = (side == 0 ? altar.resultEast : altar.resultWest).record.data;
        const int level = data.level;
        i16& spirit = altar.vitals[2 * side];
        i16& stamina = altar.vitals[2 * side + 1];

        auto add = [level](i16& value, u16 present, int times)
        {
            if (present != 0)
            {
                value = static_cast<i16>(value + times * level);
            }
        };

        add(spirit, data.itemA, kSpiritItemA);
        add(spirit, data.itemB, kSpiritItemB);
        add(spirit, data.itemC, kSpiritItemC);
        add(spirit, data.healItemA, kSpiritHealA);
        add(spirit, data.healItemB, kSpiritHealB);
        add(stamina, data.skillA, kStaminaSkillA);
        add(stamina, data.skillB, kStaminaSkillB);
    }
}

}  // namespace swd3
