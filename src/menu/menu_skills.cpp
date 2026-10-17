#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <string>

#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/databases.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_list.h"
#include "org/database.h"
#include "platform/display.h"
#include "text/big5.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// SkillsPage::stage.
constexpr i32 kSkillList = 1;
constexpr i32 kChooseRole = 2;
constexpr i32 kDescribing = 5;
constexpr i32 kChooseTown = 0xF;
constexpr i32 kNoTownWarp = 0x11;  // the town warp cannot be used here
constexpr i32 kNoExitWarp = 0x12;
constexpr u16 kTownCode = 0xF;  // Menu::code with the town list open as a subpage (as the original tests)

constexpr int kRows = 0x18;  // the list's rows: two columns of twelve
constexpr int kColumns = 2;
constexpr int kTownRows = 8;
constexpr int kShownTabs = 3;  // 0x00444FB0
constexpr int kBaseTabs = 3;
constexpr int kFirstExtraTabFlag = 0x15;  // two flags a role add the fourth and fifth tab
constexpr int kTabFlagsPerRole = 2;
// swd32.exe 0x00499CDC: the skill kind (ObjectData::unk52) of each tab; kind 0 shows on every tab.
constexpr std::array<u16, 5> kTabKinds = {0x1C, 0x1B, 0x1F, 0x1D, 0x1E};
constexpr u16 kRoleMask = 3;
constexpr u16 kSpiritCost = 0x8000;  // ObjectData::unk40: the cost in spirit (bit 15) or stamina (bit 14)
constexpr u16 kSpiritMask = 0x7FFF;
constexpr u16 kStaminaCost = 0x4000;
constexpr u16 kStaminaMask = 0x3FFF;
constexpr u16 kCostBits = 0xC000;
constexpr u16 kUseMask = 0xF;  // ObjectData::usableBy's low nibble: 0 never, 1 in battle only, 2 and up also here
constexpr u16 kFieldUse = 2;
constexpr u16 kBattleOnly = 1;
constexpr u16 kExitWarp = 0x619;
constexpr u16 kTownWarp = 0x61E;
constexpr int kExitWarpBlocked = 0x4E;
constexpr int kTownWarpBlocked = 0x4D;
constexpr u32 kWholeParty = 1;  // SkillScript::flags: the skill works on every party role
constexpr int kTownFlags = 0x1388;
constexpr i16 kWarpEvent = 0x52;
constexpr u16 kWarpLeader = 1;
constexpr u16 kTownNameEnd = 0x5125;  // "%Q"
constexpr u16 kTableEnd = 0xFFFF;
constexpr std::size_t kTownNameBytes = 0x40;
constexpr std::size_t kTownsMost = 0x200;
constexpr std::size_t kTownWords = 3;
constexpr i32 kDescriptionSlide = 0x80;
constexpr i32 kDescriptionHidden = 0x1E0;
constexpr i32 kDescriptionUp = 0x168;
constexpr i32 kPageBackground = 0x45;
constexpr i32 kLeaveBackground = 0x36;
constexpr i32 kPressedOffset = -1;
constexpr int kMessageLines = 3;
constexpr std::size_t kMessageLineBytes = 0x10;

constexpr u16 kMoveSound = 0x2E;
constexpr u16 kRoleSound = 0x107;
constexpr u16 kUsedSound = 0x8B;
constexpr u16 kRefusedSound = 0x8C;
constexpr u16 kScrollUp = 3;
constexpr u16 kScrollDown = 0x30;
constexpr u16 kTownScrollUp = 0x300;
constexpr u16 kTownScrollDown = 0x3000;
constexpr int kNibbleShift = 4;
constexpr u16 kNibble = 0xF;

// The clicks (Menu::clicks).
constexpr u32 kLeftClicks = 3;
constexpr u32 kDoubleClick = 2;
constexpr u32 kRightClick = 4;
constexpr u32 kLeftOrRight = 5;

// The texts.
// swd32.exe 0x004A6F80..0x004A6FA0: the names of the tabs.
constexpr std::array<std::u8string_view, 5> kTabNames = {u8"魔法", u8"仙術", u8"絕技", u8"武術", u8"醫術"};
constexpr std::u8string_view kCannotHere = u8"此地無法使用";  // swd32.exe 0x004A6F64
constexpr std::u8string_view kEmptyName = u8"無";             // swd32.exe 0x0049F9FC
constexpr std::string_view kNameFormat = "%-14s";
constexpr std::string_view kCostFormat = "%3d";
// The element picture of a skill (SkillScript::unk10) in pictures of 0x241C (-1: none).
constexpr std::array<int, 12> kElementPictures = {8, 1, 2, 3, 4, -1, 5, 6, 7, -1, 9, 0xA};
constexpr u16 kElementPictureFile = 0x241C;

// The layout.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kNeverRed = 0xD;
constexpr u32 kNeverGreen = 0xD;
constexpr u32 kNeverBlue = 9;
constexpr u32 kBattleRed = 0x18;
constexpr u32 kBattleGreen = 0xA;
constexpr u32 kBattleBlue = 0xB;
constexpr u32 kRoleRed = 0x15;
constexpr u32 kRoleGreen = 0xF;
constexpr u32 kRoleBlue = 8;
constexpr u32 kRoleChosenRed = 0x19;
constexpr u32 kRoleChosenGreen = 0x13;
constexpr u32 kRoleChosenBlue = 0xC;
constexpr int kDim = -4;
constexpr int kPanelStyle = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr int kTabsBoxX = 0xD0;
constexpr int kTabsBoxY = 0x36;
constexpr int kTabWidth = 0x3C;
constexpr int kTabsBoxPad = 0x10;
constexpr int kTabsBoxHeight = 0x1E;
constexpr int kTabsShade = 0x10;
constexpr int kTabsBlue = 0x6C;
constexpr int kTabsFrameLeft = 0xD8;
constexpr int kTabsFrameTop = 0x3E;
constexpr int kTabsFrameBottom = 0x50;
constexpr int kHelpBoxX = 0xD0;
constexpr int kHelpBoxY = 0x1B5;
constexpr int kHelpBoxWidth = 0x190;
constexpr int kHelpBoxHeight = 0x24;
constexpr int kHelpFrameLeft = 0xD8;
constexpr int kHelpFrameTop = 0x1BD;
constexpr int kHelpFrameRight = 0x258;
constexpr int kHelpFrameBottom = 0x1D1;
constexpr int kTabTextX = 0xE0;
constexpr int kTabTextY = 0x3E;
constexpr int kListX = 0xF0;
constexpr int kListY = 0x72;
constexpr int kColumnWidth = 0xC6;
constexpr int kRowHeight = 0x19;
constexpr int kCostX = 0x178;
constexpr int kCostDown = 6;
constexpr int kElementColumn = 0xC8;
constexpr int kElementX = 0xDB;
constexpr int kElementDown = 1;
constexpr int kHighlightColumn = 0xC5;
constexpr int kHighlightX = 0xD3;
constexpr int kHighlightY = 0x70;
constexpr int kHighlightWidth = 0xC5;
constexpr int kHighlightHeight = 0x18;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kNoneX = 0xD8;
constexpr int kNoneY = 0x70;
constexpr Pixel kNoneColor = 0xFFFF;
constexpr i32 kButtonAct = 0x232A;
constexpr i32 kButtonAction = 0x20;
constexpr int kButtonX = 0x21C;
constexpr int kButtonY = 0x1D4;
constexpr int kBarX = 0x264;
constexpr int kBarY = 0x7A;
constexpr int kBarHeight = 0x118;
constexpr int kRolesBoxX = 0x154;
constexpr int kRolesBoxY = 0xD0;
constexpr int kRolesBoxWidth = 0x74;
constexpr int kRolesBoxPad = 0xC;
constexpr int kRolesBoxRed = 0xC0;
constexpr int kRolesBoxGreen = 0x40;
constexpr int kRolesBoxBlue = 0x40;
constexpr int kRolesFrameLeft = 0x15C;
constexpr int kRolesFrameTop = 0xD8;
constexpr int kRolesFrameRight = 0x1C4;
constexpr int kRoleTextX = 0x160;
constexpr int kRoleTextY = 0xDC;
constexpr int kRoleHighlightX = 0x157;
constexpr int kRoleHighlightY = 0xDA;
constexpr int kRoleHighlightWidth = 0x72;
constexpr int kRoleHighlightHeight = 0x14;
constexpr int kHelpTextClipX = 0xD4;
constexpr int kHelpTextClipY = 0x1BD;
constexpr int kHelpTextClipWidth = 0x188;
constexpr int kHelpTextClipHeight = 0x1E;
constexpr int kHelpTextX = 0xDA;
constexpr int kHelpTextY = 0x1C0;
constexpr int kTownsBoxX = 0x190;
constexpr int kTownsBoxY = 0xC4;
constexpr int kTownsBoxWidth = 0xA2;
constexpr int kTownsBoxHeight = 0xD0;
constexpr int kTownsBoxMode = 4;
constexpr int kTownsBoxBlue = 4;
constexpr int kTownsX = 0x190;
constexpr int kTownsY = 0xC2;
constexpr int kTownHighlightX = 0x18B;
constexpr int kTownHighlightWidth = 0xAC;
constexpr int kTownsBarX = 0x226;
constexpr int kTownsBarY = 0xD0;
constexpr int kTownsBarHeight = 0xB8;
constexpr int kMessageX = 0x12C;
constexpr int kMessageY = 0xF0;
constexpr int kMessageWidth = 0xB0;
constexpr int kMessageHeight = 0x48;
constexpr int kMessageBoxMode = 4;
constexpr int kMessageLineHeight = 0x18;
constexpr int kDescriptionBoxX = 0xD4;
constexpr int kDescriptionBoxUp = 8;
constexpr int kDescriptionBoxWidth = 0x188;
constexpr int kDescriptionBoxBottom = 0x1E6;
constexpr int kDescriptionTextX = 0xDC;
constexpr int kDescriptionFrameRight = 0x254;
constexpr int kDescriptionFrameBottom = 0x1D6;
constexpr int kDescriptionLines = 5;
constexpr int kDescriptionWidth = 0x168;

// The mouse areas.
constexpr u32 kTownsLeft = 0x190;
constexpr u32 kTownsRight = 0x226;
constexpr u32 kTownsTop = 0xC4;
constexpr u32 kTownsBottom = 0x18C;
constexpr u32 kTownsBottomInside = 0x18B;
constexpr u32 kTownsInnerRight = 0x222;
constexpr u32 kTownsBarLeft = 0x228;
constexpr u32 kTownsBarRight = 0x236;
constexpr i32 kTownsUpTop = 0xC2;
constexpr i32 kTownsUpBottom = 0xD0;
constexpr i32 kTownsDownTop = 0x18A;
constexpr i32 kTownsDownBottom = 0x198;
constexpr u32 kDescribeTop = 0x1D0;
constexpr u32 kDescribeBottom = 0x1E0;
constexpr u32 kDescribeLeft = 0x216;
constexpr u32 kDescribeRight = 0x260;
constexpr i32 kTabsTop = 0x3A;
constexpr i32 kTabsBottom = 0x56;
constexpr i32 kTabsLeft = 0xD4;
constexpr i32 kTabsRight = 0x1F6;
constexpr i32 kTabAreaWidth = 0x3A;
constexpr i32 kRowsTop = 0x72;
constexpr i32 kRowsBottom = 0x19E;
constexpr i32 kRowsLeft = 0xD4;
constexpr i32 kRowsRight = 0x25A;
constexpr i32 kRowsColumnWidth = 0xC5;
constexpr u32 kBarLeft = 0x264;
constexpr u32 kBarRight = 0x274;
constexpr i32 kBarUpTop = 0x6A;
constexpr i32 kBarUpBottom = 0x7A;
constexpr i32 kBarDownTop = 0x194;
constexpr i32 kBarDownBottom = 0x1A2;
constexpr i32 kRoleRowsTop = 0xD7;
constexpr i32 kRoleRowsBottom = 0x13B;
constexpr i32 kRoleRowsLeft = 0x158;
constexpr i32 kRoleRowsRight = 0x1C6;
constexpr i32 kFacesTop = 0xA;
constexpr u32 kFacesBottom = 0x1D4;
constexpr u32 kFacesLeft = 4;
constexpr u32 kFacesRight = 0xBC;
constexpr u32 kFaceHeight = 0x6E;

// The cost of `skill` paid by `role` when it can be: true when every cost it has was met (the costs met are taken
// even when another is not).
bool PayCost(RoleRecord& role, u16 cost)
{
    int costs = 0;
    int paid = 0;

    if ((cost & kSpiritCost) != 0)
    {
        ++costs;

        if (role.spirit >= (cost & kSpiritMask))
        {
            ++paid;
            role.spirit = static_cast<i16>(role.spirit - (cost & kSpiritMask));
        }
    }

    if ((cost & kStaminaCost) != 0)
    {
        ++costs;

        if (role.stamina >= (cost & kStaminaMask))
        {
            ++paid;
            role.stamina = static_cast<i16>(role.stamina - (cost & kStaminaMask));
        }
    }

    return paid == costs && costs != 0;
}

// Whether `role` can pay the cost (inline in 0x00443BD0).
bool CanPay(const RoleRecord& role, u16 cost)
{
    int costs = 0;
    int met = 0;

    if ((cost & kSpiritCost) != 0)
    {
        costs = 1;
        met = role.spirit >= (cost & kSpiritMask) ? 1 : 0;
    }

    if ((cost & kStaminaCost) != 0)
    {
        ++costs;

        if (role.stamina >= (cost & kStaminaMask))
        {
            ++met;
        }
    }

    return met == costs && costs != 0;
}

u16 TableWord(const std::vector<u8>& data, std::size_t at)
{
    return MapsWord(data, at);
}

}  // namespace

// swd32.exe 0x00442E40: the skills page opened (on the first role when no tab is raised): the first tab's list, the
// roles counted, the first skill described
void Menu::SkillsEnter()
{
    SkillsPage& page = this->skills;

    if (this->unk4FBECC == Menu::NoTab)
    {
        this->unk4FBECC = 0;
    }

    this->mode = static_cast<u16>(Menu::FirstPageMode + 2 * Menu::PageModeStep);
    page.target = 0;
    page.stage = kSkillList;
    page.tab = 0;
    this->SkillsFillList();
    this->SkillsCountTabs();
    page.roles = 0;

    for (const MenuTab& tab : this->tabs)
    {
        if (tab.role != Menu::NoRole)
        {
            ++page.roles;
        }
    }

    this->SkillsDescribe();
    page.buttonOffset = 0;
    page.descriptionSlide = 0;
    page.descriptionTop = kDescriptionHidden;
    page.skill = SkillScript{};
    page.shownTabs = kShownTabs;
    page.scrollArrows = 0;
    this->background.action = kPageBackground;
}

// swd32.exe 0x00442F10: the skills page left: the list's skills back with the role
void Menu::SkillsLeave()
{
    this->SkillsReturn();
    this->skills.stage = 0;
    this->background.action = kLeaveBackground;
}

// The description of the skill chosen (inline in the skills page).
void Menu::SkillsDescribe()
{
    const SkillsPage& page = this->skills;
    const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);
    this->description =
        chosen != nullptr ? ObjectDescription(this->game.databases.mapsData, chosen->id) : std::string();
}

// swd32.exe 0x00444DB0: the skills of `source` of the tab's kind (or of kind 0) moved into the page's list in the order
// of their ids
void Menu::SkillsTake(ItemLink& source, int tab)
{
    SkillsPage& page = this->skills;
    page.list.reset();
    const u16 kind = kTabKinds[static_cast<std::size_t>(std::clamp(tab, 0, static_cast<int>(kTabKinds.size()) - 1))];
    ItemLink* link = &source;

    while (*link != nullptr)
    {
        const u16 skillKind = (*link)->record.data.unk52;

        if (skillKind != kind && skillKind != 0)
        {
            link = &(*link)->next;
            continue;
        }

        std::unique_ptr<ItemNode> moved = std::move(*link);
        *link = std::move(moved->next);
        InsertItemById(page.list, std::move(moved));
    }
}

// swd32.exe 0x00444E50: the rows shown from the first one shown (at most 24)
void Menu::SkillsCountShown()
{
    SkillsPage& page = this->skills;
    page.shown = 0;

    for (const ItemNode* node = page.firstShown; node != nullptr && page.shown < kRows; node = node->next.get())
    {
        ++page.shown;
    }
}

// swd32.exe 0x00444E80: the list of the tab made from the role's skills (the empty entry when none)
void Menu::SkillsFillList()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    std::unique_ptr<ItemNode>& head = owner.party.skills[std::min<std::size_t>(this->unk4FBECC, Party::Roles - 1)];

    if (head != nullptr)
    {
        this->SkillsTake(head->next, page.tab);
    }

    if (page.list == nullptr)
    {
        page.list = NewEmptyItem();
    }

    page.count = CountItems(page.list);
    page.top = 0;
    page.row = 0;
    page.firstShown = page.list.get();
    this->SkillsCountShown();
}

// swd32.exe 0x00444F00: the list's skills back after the role's head one by one (their order turns around); the empty
// entry freed
void Menu::SkillsReturn()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    std::unique_ptr<ItemNode>& head = owner.party.skills[std::min<std::size_t>(this->unk4FBECC, Party::Roles - 1)];

    while (page.list != nullptr)
    {
        std::unique_ptr<ItemNode> node = std::move(page.list);
        page.list = std::move(node->next);

        if (node->id != ItemNode::Empty && head != nullptr)
        {
            node->next = std::move(head->next);
            head->next = std::move(node);
        }
    }
}

// swd32.exe 0x00444F60: the tabs of the target role: three, and one for each of its two extra flags
void Menu::SkillsCountTabs()
{
    SkillsPage& page = this->skills;
    page.tabs = kBaseTabs;

    if (this->game.state.GameFlag(kTabFlagsPerRole * page.target + kFirstExtraTabFlag))
    {
        ++page.tabs;
    }

    if (this->game.state.GameFlag(kTabFlagsPerRole * page.target + kFirstExtraTabFlag + 1))
    {
        ++page.tabs;
    }
}

// The list moved (inline in the skills page's keys): the first row shown again, the skill described.
void Menu::SkillsMoved(u16 arrows, bool sound)
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    SkipItems(page.top, page.list, page.firstShown);
    CountShown(page.firstShown, page.shown, kRows);
    this->SkillsDescribe();

    if (sound)
    {
        page.scrollArrows = arrows;
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    }
}

// The next party role among the tabs (inline in the skills page's keys on the role choice).
int Menu::SkillsNextRole(int role, int step) const
{
    int next = role;

    do
    {
        next += step;

        if (next >= Party::Roles)
        {
            next = 0;
        }

        if (next < 0)
        {
            next = Party::Roles - 1;
        }
    } while (this->tabs[static_cast<std::size_t>(next)].role == Menu::NoRole);

    return next;
}

// swd32.exe 0x00443450: down: on the list the row below (two columns), scrolling a line at the bottom; on the roles the
// next party role; on the towns the next town
void Menu::SkillsDown()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage == kSkillList)
    {
        page.row += kColumns;

        if (page.row >= page.shown)
        {
            if (page.shown + page.top < page.count)
            {
                page.top += kColumns;
            }

            page.row -= kColumns;
        }

        SkipItems(page.top, page.list, page.firstShown);
        CountShown(page.firstShown, page.shown, kRows);

        if (page.row >= page.shown)
        {
            page.row -= kColumns;
        }

        this->SkillsDescribe();
        page.scrollArrows = kScrollDown;
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
        return;
    }

    if (page.stage == kChooseRole)
    {
        page.target = this->SkillsNextRole(page.target, 1);
        return;
    }

    if (page.stage == kChooseTown)
    {
        ListDown(static_cast<i32>(this->towns.size()), this->townTop, this->townRow, this->townShown);
        page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollDown);
    }
}

// swd32.exe 0x00443570: up: on the list the row above, scrolling a line at the top; on the roles the role before
void Menu::SkillsUp()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage == kSkillList)
    {
        const i32 row = page.row - kColumns;
        page.row = row;

        if (page.row < 0)
        {
            page.row = row + kColumns;

            if (page.top > 0)
            {
                page.top -= kColumns;
            }
        }

        SkipItems(page.top, page.list, page.firstShown);
        CountShown(page.firstShown, page.shown, kRows);
        this->SkillsDescribe();
        page.scrollArrows = kScrollUp;
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
        return;
    }

    if (page.stage == kChooseRole)
    {
        page.target = this->SkillsNextRole(page.target, -1);
        return;
    }

    if (page.stage == kChooseTown)
    {
        ListUp(this->townTop, this->townRow);
        page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollUp);
    }
}

// swd32.exe 0x00443670: Page Down: on the list to the last row (in the column), from there a page of 24 on; on the
// roles the last party role
void Menu::SkillsPageDown()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage != kSkillList)
    {
        if (page.stage == kChooseRole)
        {
            int role = Party::Roles;

            do
            {
                --role;
            } while (this->tabs[static_cast<std::size_t>(role)].role == Menu::NoRole);

            page.target = role;
        }
        else if (page.stage == kChooseTown)
        {
            i32 shown = this->townShown;
            ListPageDown(static_cast<i32>(this->towns.size()), this->townTop, this->townRow, shown, kTownRows);
            this->townShown = shown;
            page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollDown);
        }

        return;
    }

    const i32 lastPair = page.shown - ((page.shown & 1) == 0 ? 1 : 0);
    const i32 last = lastPair - 1;

    if (page.row < last)
    {
        const i32 column = page.row & 1;
        page.row = column + lastPair - 1;

        if (page.row >= page.shown)
        {
            page.row = column + lastPair - 3;
        }

        return;
    }

    page.top += kRows;

    if (page.top >= page.count)
    {
        page.top -= kRows;
        page.row = last;
    }
    else
    {
        SkipItems(page.top, page.list, page.firstShown);
        this->SkillsCountShown();

        if (page.row >= page.shown)
        {
            page.row = (page.row & 1) - ((page.shown & 1) == 0 ? 1 : 0) + page.shown - 1;

            if (page.row >= page.shown)
            {
                page.row -= kColumns;
            }

            return;
        }
    }

    this->SkillsDescribe();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    page.scrollArrows = kScrollDown;
}

// swd32.exe 0x004437C0: Page Up: on the list to the first line, from there a page of 24 back; on the roles the first
// party role
void Menu::SkillsPageUp()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage == kSkillList)
    {
        if (page.row > 1)
        {
            page.row &= 1;
            return;
        }

        page.top -= kRows;

        if (page.top >= 0)
        {
            SkipItems(page.top, page.list, page.firstShown);
            this->SkillsCountShown();
            page.row &= 1;
        }
        else
        {
            page.row &= 1;
            page.top = 0;
            SkipItems(0, page.list, page.firstShown);
            this->SkillsCountShown();
        }

        this->SkillsDescribe();
        page.scrollArrows = kScrollUp;
        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
        return;
    }

    if (page.stage == kChooseRole)
    {
        int role = 0;

        while (this->tabs[static_cast<std::size_t>(role)].role == Menu::NoRole)
        {
            ++role;
        }

        page.target = role;
        return;
    }

    if (page.stage == kChooseTown)
    {
        ListPageUp(this->townTop, this->townRow, kTownRows);
        page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollUp);
    }
}

// The other column of the list (inline in 0x004438E0 and 0x004439A0).
void Menu::SkillsOtherColumn(u16 arrows)
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    page.row += (page.row & 1) != 1 ? 1 : -1;

    if (page.row >= page.shown)
    {
        page.row = page.shown - 1;
    }

    this->SkillsDescribe();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    page.scrollArrows = arrows;
}

// swd32.exe 0x004438E0: left: the other column; on the roles the first party role; on the towns the town before
void Menu::SkillsLeft()
{
    SkillsPage& page = this->skills;

    if (page.stage == kSkillList)
    {
        this->SkillsOtherColumn(kScrollUp);
    }
    else if (page.stage == kChooseRole)
    {
        int role = 0;

        while (this->tabs[static_cast<std::size_t>(role)].role == Menu::NoRole)
        {
            ++role;
        }

        page.target = role;
    }
    else if (page.stage == kChooseTown)
    {
        ListUp(this->townTop, this->townRow);
        page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollUp);
    }
}

// swd32.exe 0x004439A0: right: the other column; on the roles the last party role; on the towns the next town
void Menu::SkillsRight()
{
    SkillsPage& page = this->skills;

    if (page.stage == kSkillList)
    {
        this->SkillsOtherColumn(kScrollDown);
    }
    else if (page.stage == kChooseRole)
    {
        int role = Party::Roles;

        do
        {
            --role;
        } while (this->tabs[static_cast<std::size_t>(role)].role == Menu::NoRole);

        page.target = role;
    }
    else if (page.stage == kChooseTown)
    {
        ListDown(static_cast<i32>(this->towns.size()), this->townTop, this->townRow, this->townShown);
        page.scrollArrows = static_cast<u16>(page.scrollArrows | kTownScrollDown);
    }
}

// swd32.exe 0x00443A60: Shift on the list: the next party role
void Menu::SkillsShift()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage != kSkillList)
    {
        return;
    }

    this->SkillsReturn();
    this->unk4FBECC = static_cast<u16>(this->SkillsNextRole(this->unk4FBECC, 1) & kRoleMask);
    this->SkillsCountTabs();
    this->SkillsFillList();
    FitListRow(page.count, page.top, page.row, page.shown);
    SkipItems(page.top, page.list, page.firstShown);
    CountShown(page.firstShown, page.shown, kRows);
    this->SkillsDescribe();
    page.shownTabs = kShownTabs;
    owner.audio.PlaySound(kRoleSound, owner.config.soundVolume);
}

// swd32.exe 0x00443B70: End on the list: the next of the three tabs
void Menu::SkillsEnd()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    if (page.stage != kSkillList)
    {
        return;
    }

    this->SkillsReturn();

    if (++page.tab == kShownTabs)
    {
        page.tab = 0;
    }

    this->SkillsFillList();
    this->SkillsDescribe();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x00443BD0: Space or Enter. On the list a skill usable here whose cost the role can pay is used: the exit
// warp (0x619) leaves the dungeon, the town warp (0x61E) lists the towns, a skill for the whole party is paid and works
// on every party role, another asks for its role; a refused one sounds. On the roles the role chosen gets it if it is
// paid; on the towns the party warps there; the description and the messages close.
void Menu::SkillsSelect()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    RoleRecord& role = owner.party.roles[std::min<std::size_t>(this->unk4FBECC, Party::Roles - 1)];

    auto refuse = [&owner]
    {
        owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
    };

    switch (page.stage)
    {
        case kSkillList:
        {
            const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);

            if (chosen == nullptr || chosen->id == ItemNode::Empty)
            {
                return;
            }

            const ObjectData& data = chosen->record.data;

            if (static_cast<i16>(role.unk24) < 0 || (data.usableBy & kUseMask) < kFieldUse || !CanPay(role, data.unk40))
            {
                refuse();
                return;
            }

            if (chosen->id == kExitWarp)
            {
                const std::size_t warp =
                    owner.state.GameFlag(kExitWarpBlocked) ? 0 : this->FindExitWarp(owner.currentMap);

                if (warp == 0)
                {
                    page.stage = kNoExitWarp;
                    refuse();
                    return;
                }

                const std::vector<u8>& maps = owner.databases.mapsData;
                role.spirit = static_cast<i16>(role.spirit - (data.unk40 & kSpiritMask));
                this->Warp(
                    TableWord(maps, warp),
                    TableWord(maps, warp + sizeof(u16)),
                    TableWord(maps, warp + 2 * sizeof(u16)),
                    kWarpEvent
                );
                this->SkillsLeave();
                owner.menuState = Game::NoMenu;
                return;
            }

            if (chosen->id == kTownWarp)
            {
                if (owner.state.GameFlag(kTownWarpBlocked) || owner.scene.unk4ACDB4 != 1)
                {
                    page.stage = kNoTownWarp;
                    refuse();
                    return;
                }

                page.stage = kChooseTown;
                this->ListTowns();
                this->townTop = 0;
                this->townShown = std::min<i32>(kTownRows, static_cast<i32>(this->towns.size()));
                this->townRow = 0;
                return;
            }

            page.skill = SkillScript{};

            if (!owner.database.LoadSkill(page.skill, data.unk3E))
            {
                return;
            }

            if ((page.skill.flags & kWholeParty) == 0)
            {
                ++page.stage;
                this->code = static_cast<u16>(page.stage + 1);
                return;
            }

            PayCost(role, data.unk40);
            owner.audio.PlaySound(kUsedSound, owner.config.soundVolume);

            for (int member = 0; member < Party::Roles; ++member)
            {
                if (owner.state.GameFlag(Party::FirstRoleFlag + member))
                {
                    ApplyObject(owner.party.roles[static_cast<std::size_t>(member)], data, owner.database);
                }
            }

            return;
        }

        case kChooseRole:
        {
            const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);

            if (chosen == nullptr)
            {
                return;
            }

            if (!PayCost(role, chosen->record.data.unk40))
            {
                refuse();
                return;
            }

            if (owner.state.GameFlag(page.target + Party::FirstRoleFlag))
            {
                ApplyObject(
                    owner.party.roles[static_cast<std::size_t>(page.target)],
                    chosen->record.data,
                    owner.database
                );
                owner.audio.PlaySound(kUsedSound, owner.config.soundVolume);
            }

            return;
        }

        case kDescribing:
            page.stage = kSkillList;
            page.descriptionSlide = -kDescriptionSlide;
            return;

        case kChooseTown:
        {
            const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);
            role.spirit =
                static_cast<i16>(role.spirit - ((chosen != nullptr ? chosen->record.data.unk40 : 0) & kSpiritMask));
            const std::size_t index = static_cast<std::size_t>(this->townTop + this->townRow);

            if (index < this->towns.size())
            {
                const MenuTown& town = this->towns[index];
                this->Warp(town.map, town.x, town.y, kWarpEvent);
            }

            this->towns.clear();
            this->SkillsLeave();
            owner.menuState = Game::NoMenu;
            return;
        }

        case kNoTownWarp:
        case kNoExitWarp:
            page.stage = kSkillList;
            return;

        default:
            return;
    }
}

// swd32.exe 0x004441A0: Escape: from the list the page closes; from the roles back to the list; the description, the
// towns and the messages close
void Menu::SkillsCancel()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;

    switch (page.stage)
    {
        case kSkillList:
            if (--this->code == 0)
            {
                owner.menuState = Game::NoMenu;
            }

            this->SetPage(this->code);
            this->SkillsLeave();
            return;

        case kChooseRole:
            --this->code;
            page.stage = kSkillList;
            page.target = 0;
            return;

        case kDescribing:
            page.stage = kSkillList;
            page.descriptionSlide = -kDescriptionSlide;
            return;

        case kChooseTown:
            this->towns.clear();
            page.stage = kSkillList;
            return;

        case kNoTownWarp:
        case kNoExitWarp:
            page.stage = kSkillList;
            return;

        default:
            return;
    }
}

// swd32.exe 0x00442F40: the mouse on the skills page: the messages close on a click or a right click; on the town list
// opened as a subpage a click chooses a town (on the chosen one: warps). On the list the description button presses
// the description key, a click on a tab chooses it, on a skill chooses it (a double click on the chosen one uses it);
// a held left button on the scroll bar scrolls, on the towns their bar or a town (a double click on the chosen one
// warps). On the roles a click chooses one (on the chosen one uses the skill); a click on a face changes the role; a
// right click goes back.
void Menu::SkillsFrame()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    const u32 clicks = this->clicks;
    const i32 x = owner.state.pointerX;
    const i32 y = owner.state.pointerY;
    const auto ux = static_cast<u32>(x);
    const auto uy = static_cast<u32>(y);
    page.buttonOffset = 0;

    if ((clicks & kLeftOrRight) != 0 && (page.stage == kNoTownWarp || page.stage == kNoExitWarp))
    {
        this->SkillsSelect();
        return;
    }

    if (this->code == kTownCode && x < static_cast<i32>(kTownsRight) && x > static_cast<i32>(kTownsLeft) &&
        y < static_cast<i32>(kTownsBottom) && y > static_cast<i32>(kTownsTop) && (clicks & kLeftClicks) != 0)
    {
        const auto row = static_cast<i32>((uy - kTownsTop) / static_cast<u32>(kRowHeight));

        if (this->townRow == row)
        {
            this->SkillsSelect();
        }
        else
        {
            this->townRow = row;
        }

        return;
    }

    if (page.stage == kSkillList)
    {
        bool rest = false;

        if (uy < kDescribeBottom && uy > kDescribeTop && ux < kDescribeRight && ux > kDescribeLeft)
        {
            page.buttonOffset = kPressedOffset;

            if ((clicks & kLeftClicks) == 0)
            {
                rest = true;
            }
            else
            {
                owner.input.PressKey(owner.config.keys[static_cast<std::size_t>(Button::F1)]);
            }
        }

        if (!rest && (clicks & kLeftClicks) != 0)
        {
            if (y < kTabsBottom && y > kTabsTop && x < kTabsRight && x > kTabsLeft)
            {
                const int tab = (x - kTabsLeft) / kTabAreaWidth;
                int tabs = kBaseTabs;

                if (owner.state.GameFlag(kTabFlagsPerRole * page.target + kFirstExtraTabFlag))
                {
                    tabs = kBaseTabs + 1;
                }

                if (owner.state.GameFlag(kTabFlagsPerRole * page.target + kFirstExtraTabFlag + 1))
                {
                    ++tabs;
                }

                if (tab < tabs)
                {
                    page.tab = tab - 1;
                    this->SkillsEnd();
                }

                return;
            }

            if (y < kRowsBottom && y > kRowsTop && x < kRowsRight && x > kRowsLeft)
            {
                const int row = (x - kRowsLeft) / kRowsColumnWidth + kColumns * ((y - kRowsTop) / kRowHeight);

                if (row >= page.shown)
                {
                    return;
                }

                if (row != page.row)
                {
                    page.row = row;
                    this->SkillsDescribe();
                    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
                    return;
                }

                if ((clicks & kDoubleClick) != 0)
                {
                    this->SkillsSelect();
                }

                return;
            }
        }
    }

    if (owner.controls.Repeating(Button::MouseLeft))
    {
        if (page.stage == kSkillList)
        {
            if (page.count > kRows && ux < kBarRight && ux > kBarLeft)
            {
                const MenuScrollBar& bar = this->scrollBar;

                if (y < kBarUpBottom && y > kBarUpTop)
                {
                    this->SkillsUp();
                }

                if (y < kBarDownBottom && y > kBarDownTop)
                {
                    this->SkillsDown();
                }

                if (y < bar.thumbTop && y > bar.top)
                {
                    this->SkillsPageUp();
                }

                if (y < bar.bottom && y > bar.thumbBottom)
                {
                    this->SkillsPageDown();
                }

                return;
            }
        }
        else if (page.stage == kChooseTown)
        {
            if (static_cast<i32>(this->towns.size()) <= kTownRows || ux >= kTownsBarRight || ux <= kTownsBarLeft)
            {
                if (ux >= kTownsInnerRight || ux <= kTownsLeft || uy >= kTownsBottomInside || uy <= kTownsTop)
                {
                    return;
                }

                i32 row = static_cast<i32>((uy - kTownsTop) / static_cast<u32>(kRowHeight));

                if (row >= this->townShown)
                {
                    row = this->townShown - 1;
                }

                if (row != this->townRow)
                {
                    if ((clicks & kLeftClicks) != 0)
                    {
                        this->townRow = row;
                    }

                    return;
                }

                if ((clicks & kDoubleClick) != 0)
                {
                    this->SkillsSelect();
                }

                return;
            }

            const MenuScrollBar& bar = this->townBar;

            if (y < kTownsUpBottom && y > kTownsUpTop)
            {
                this->SkillsUp();
            }

            if (y < kTownsDownBottom && y > kTownsDownTop)
            {
                this->SkillsDown();
            }

            if (y < bar.thumbTop && y > bar.top)
            {
                this->SkillsPageUp();
            }

            if (y < bar.bottom && y > bar.thumbBottom)
            {
                this->SkillsPageDown();
            }

            return;
        }
    }

    if ((clicks & kLeftClicks) != 0 && page.stage == kChooseRole && y < kRoleRowsBottom && y > kRoleRowsTop &&
        x < kRoleRowsRight && x > kRoleRowsLeft)
    {
        const auto row = static_cast<i32>((uy - static_cast<u32>(kRoleRowsTop)) / static_cast<u32>(kRowHeight));

        if (row < page.roles)
        {
            int role = 0;

            for (int step = 0; step < row; ++step)
            {
                do
                {
                    ++role;
                } while (!owner.state.GameFlag(role + Party::FirstRoleFlag));
            }

            if (page.target == role)
            {
                this->SkillsSelect();
            }

            page.target = role;
        }

        return;
    }

    if ((clicks & kLeftClicks) != 0 && page.stage == kSkillList && uy < kFacesBottom &&
        uy > static_cast<u32>(kFacesTop) && ux < kFacesRight && ux > kFacesLeft)
    {
        const auto face = static_cast<u16>((uy - static_cast<u32>(kFacesTop)) / kFaceHeight);

        if (owner.state.GameFlag(face + Party::FirstRoleFlag))
        {
            do
            {
                this->SkillsShift();
            } while (this->unk4FBECC != face);
        }

        return;
    }

    if ((clicks & kRightClick) != 0)
    {
        this->SkillsCancel();
    }
}

// swd32.exe 0x004442B0: the skills page: the description key opens the description of the skill chosen; the tabs, the
// two columns of skills (greyed when never usable, reddish in battle only; their cost; their element), the scroll bar,
// the roles to choose, the help line, the towns with their bar, the messages, the description box
void Menu::SkillsDraw()
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    const auto never = static_cast<Pixel>(owner.painter.MakeColor(kNeverRed, kNeverGreen, kNeverBlue));
    const auto battleOnly = static_cast<Pixel>(owner.painter.MakeColor(kBattleRed, kBattleGreen, kBattleBlue));
    ActState& panel = owner.battleMessages.panelAct;

    if (page.stage == kSkillList && owner.input.IsKeyDown(owner.config.keys[static_cast<std::size_t>(Button::F1)]) != 0)
    {
        const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);

        if (chosen != nullptr && chosen->id != ItemNode::Empty)
        {
            page.descriptionSlide = kDescriptionSlide;
            page.stage = kDescribing;
        }
    }

    const ItemNode* described = nullptr;
    owner.acts.Run(panel);
    FillBox(
        owner.painter,
        kTabsBoxX,
        kTabsBoxY,
        kTabWidth * page.shownTabs + kTabsBoxPad,
        kTabsBoxHeight,
        kTabsShade,
        kTabsShade,
        kTabsBlue,
        FillMode::Quarter
    );
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kTabsFrameLeft,
        kTabsFrameTop,
        kTabWidth * page.shownTabs + kTabsFrameLeft,
        kTabsFrameBottom,
        0,
        kFrameBorder
    );
    FillBox(owner.painter, kHelpBoxX, kHelpBoxY, kHelpBoxWidth, kHelpBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kHelpFrameLeft,
        kHelpFrameTop,
        kHelpFrameRight,
        kHelpFrameBottom,
        0,
        kFrameBorder
    );

    for (int tab = 0, x = 0; tab < kShownTabs; ++tab, x += kTabWidth)
    {
        Pixel tabColor = color;
        owner.painter.TintPixels(std::span<u16>(&tabColor, 1), kDim, kDim, kDim);
        int shift = 0;

        if (page.tab == tab)
        {
            shift = -1;
            tabColor = color;
        }

        owner.mediumFont.DrawText(
            owner.drawSurface,
            x + shift + kTabTextX,
            shift + kTabTextY,
            kTabNames[static_cast<std::size_t>(tab)],
            tabColor,
            kPanelStyle
        );
    }

    if (page.list != nullptr)
    {
        const ItemNode* node = page.firstShown;

        for (int row = 0; row < page.shown && node != nullptr; ++row, node = node->next.get())
        {
            Pixel rowColor = color;
            int shift = 0;

            if (row == page.row)
            {
                described = node;
                shift = -1;
            }

            const u16 use = node->record.data.usableBy & kUseMask;

            if (use == kBattleOnly)
            {
                shift = 0;
                rowColor = battleOnly;
            }

            if (use == 0)
            {
                shift = 0;
                rowColor = never;
            }

            if (page.stage > kSkillList)
            {
                owner.painter.TintPixels(std::span<u16>(&rowColor, 1), kDim, kDim, kDim);
            }

            const int column = row & 1;
            const int line = kRowHeight * (row / kColumns);
            owner.largeFont.DrawText(
                owner.drawSurface,
                shift + kColumnWidth * column + kListX,
                shift + line + kListY,
                WinFormat(kNameFormat, {ObjectName(node->record)}),
                rowColor,
                kPanelStyle
            );

            if (page.tab <= 1)
            {
                const u16 cost = node->record.data.unk40;
                std::string text;

                if ((cost & kSpiritCost) != 0)
                {
                    text = WinFormat(kCostFormat, {static_cast<i32>(cost & kSpiritMask)});
                }

                if ((cost & kStaminaCost) != 0)
                {
                    text = WinFormat(kCostFormat, {static_cast<i32>(cost & kStaminaMask)});
                }

                if ((cost & kCostBits) != 0)
                {
                    owner.mediumFont.DrawText(
                        owner.drawSurface,
                        shift + kColumnWidth * column + kCostX,
                        shift + line + kListY + kCostDown,
                        text,
                        rowColor,
                        kPanelStyle
                    );
                }
            }

            this->unk4FD0A8.id = kButtonAct;
            this->unk4FD0A8.action = kButtonAction;
            owner.actDrawer.Draw(this->unk4FD0A8, page.buttonOffset + kButtonX, page.buttonOffset + kButtonY);

            if (page.tab <= 1 && node->id != ItemNode::Empty)
            {
                SkillScript skill;
                owner.database.LoadSkill(skill, node->record.data.unk3E);
                const auto element = static_cast<std::size_t>(static_cast<u16>(skill.unk10));
                const int picture = element < kElementPictures.size() ? kElementPictures[element] - 1 : -1;

                if (picture >= 0)
                {
                    owner.actDrawer.DrawPicture(
                        kElementPictureFile,
                        static_cast<u16>(picture),
                        shift + kElementColumn * column + kElementX,
                        shift + line + kListY + kElementDown
                    );
                }

                page.skill = skill;
            }

            if (row == page.row && page.stage == kSkillList)
            {
                FillBox(
                    owner.painter,
                    kHighlightColumn * column + kHighlightX,
                    line + kHighlightY,
                    kHighlightWidth,
                    kHighlightHeight,
                    kHighlightRed,
                    kHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }
        }
    }
    else
    {
        owner.largeFont.DrawText(owner.drawSurface, kNoneX, kNoneY, kEmptyName, kNoneColor, kPanelStyle);
    }

    if (page.count > kRows)
    {
        int arrows = 0;

        if ((page.scrollArrows & kNibble) != 0)
        {
            page.scrollArrows = static_cast<u16>((page.scrollArrows & ~kNibble) | ((page.scrollArrows & kNibble) - 1));
            arrows = 1;
        }

        if ((page.scrollArrows & (kNibble << kNibbleShift)) != 0)
        {
            const int down = (page.scrollArrows >> kNibbleShift) & kNibble;
            arrows |= 2;
            page.scrollArrows =
                static_cast<u16>((page.scrollArrows & ~(kNibble << kNibbleShift)) | ((down - 1) << kNibbleShift));
        }

        const auto count = static_cast<double>(page.count);
        const auto to = static_cast<float>(static_cast<double>(page.top + page.shown) / count);
        const auto from = static_cast<float>(static_cast<double>(page.top) / count);
        this->DrawScrollBar(kBarX, kBarY, kBarHeight, arrows, from, to, this->scrollBar);
    }

    if (page.stage == kChooseRole)
    {
        FillBox(
            owner.painter,
            kRolesBoxX,
            kRolesBoxY,
            kRolesBoxWidth,
            kRowHeight * page.roles + kRolesBoxPad,
            kRolesBoxRed,
            kRolesBoxGreen,
            kRolesBoxBlue,
            FillMode::Eighth
        );
        DrawWindowFrame(
            owner.painter,
            owner.archive,
            panel.frame,
            kRolesFrameLeft,
            kRolesFrameTop,
            kRolesFrameRight,
            kRowHeight * page.roles + kRolesFrameTop,
            0,
            kFrameBorder
        );

        for (int member = 0, line = 0; member < Party::Roles; ++member)
        {
            if (this->tabs[static_cast<std::size_t>(member)].role == Menu::NoRole)
            {
                continue;
            }

            Pixel roleColor = static_cast<Pixel>(owner.painter.MakeColor(kRoleRed, kRoleGreen, kRoleBlue));
            int shift = 0;

            if (member == page.target)
            {
                roleColor =
                    static_cast<Pixel>(owner.painter.MakeColor(kRoleChosenRed, kRoleChosenGreen, kRoleChosenBlue));
                shift = -1;
            }

            const Party::Name& name = owner.party.names[static_cast<std::size_t>(member)];
            owner.mediumFont.DrawText(
                owner.drawSurface,
                shift + kRoleTextX,
                shift + line + kRoleTextY,
                std::string_view(name.data(), strnlen(name.data(), name.size())),
                roleColor,
                kPanelStyle
            );

            if (member == page.target)
            {
                FillBox(
                    owner.painter,
                    kRoleHighlightX,
                    line + kRoleHighlightY,
                    kRoleHighlightWidth,
                    kRoleHighlightHeight,
                    kHighlightRed,
                    kHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }

            line += kRowHeight;
        }
    }

    if (described != nullptr && described->id != ItemNode::Empty)
    {
        owner.mediumFont.SetClip(kHelpTextClipX, kHelpTextClipY, kHelpTextClipWidth, kHelpTextClipHeight);
        owner.mediumFont.DrawText(owner.drawSurface, kHelpTextX, kHelpTextY, this->description, color, kPanelStyle);
        owner.mediumFont.SetClip(0, 0, Display::Width, Display::Height);
    }

    if (page.stage == kChooseTown)
    {
        this->SkillsDrawTowns(color);
    }

    if (page.stage == kNoTownWarp || page.stage == kNoExitWarp)
    {
        const std::optional<std::string>& message =
            page.stage == kNoTownWarp ? owner.scene.unk4B751C : owner.scene.unk4B74F4;
        std::string text = message.value_or(Big5(kCannotHere));
        text.resize(kMessageLines * kMessageLineBytes, '\0');
        this->DrawBox(kMessageX, kMessageY, kMessageWidth, kMessageHeight, kMessageBoxMode, 0, 0, 0);

        for (int line = 0; line < kMessageLines; ++line)
        {
            const char* start = text.data() + kMessageLineBytes * static_cast<std::size_t>(line);
            owner.largeFont.DrawText(
                owner.drawSurface,
                kMessageX,
                kMessageY + kMessageLineHeight * line,
                std::string_view(start, strnlen(start, kMessageLineBytes)),
                color,
                kPanelStyle
            );
        }
    }

    if (page.descriptionSlide != 0 || page.descriptionTop == kDescriptionUp)
    {
        const int half = page.descriptionSlide >> 1;
        page.descriptionSlide = half;
        page.descriptionTop -= half;

        if (half <= 0)
        {
            if (page.descriptionTop > kDescriptionHidden)
            {
                page.descriptionTop = kDescriptionHidden;
                page.descriptionSlide = 0;
            }
        }
        else if (page.descriptionTop < kDescriptionUp)
        {
            page.descriptionTop = kDescriptionUp;
            page.descriptionSlide = 0;
        }

        const ItemNode* chosen = ItemAfter(page.top + page.row, page.list);
        FillBox(
            owner.painter,
            kDescriptionBoxX,
            page.descriptionTop - kDescriptionBoxUp,
            kDescriptionBoxWidth,
            kDescriptionBoxBottom - page.descriptionTop,
            kTabsShade,
            kTabsShade,
            kTabsBlue,
            FillMode::Eighth
        );
        DrawWindowFrame(
            owner.painter,
            owner.archive,
            panel.frame,
            kDescriptionTextX,
            page.descriptionTop,
            kDescriptionFrameRight,
            kDescriptionFrameBottom,
            0,
            kFrameBorder
        );
        owner.DrawWrappedText(
            chosen != nullptr ? chosen->record.description.value_or(std::string()) : std::string(),
            kDescriptionTextX,
            page.descriptionTop,
            kDescriptionLines,
            kDescriptionWidth
        );
    }
}

// The towns of the town warp with their bar (inline in 0x004442B0).
void Menu::SkillsDrawTowns(Pixel color)
{
    Game& owner = this->game;
    SkillsPage& page = this->skills;
    this->DrawBox(kTownsBoxX, kTownsBoxY, kTownsBoxWidth, kTownsBoxHeight, kTownsBoxMode, 0, 0, kTownsBoxBlue);

    for (int row = 0, y = kTownsY; row < this->townShown; ++row, y += kRowHeight)
    {
        const auto index = static_cast<std::size_t>(row + this->townTop);

        if (index + 1 > this->towns.size())
        {
            break;
        }

        owner.largeFont.DrawText(owner.drawSurface, kTownsX, y + 2, this->towns[index].name, color, kPanelStyle);

        if (row == this->townRow)
        {
            FillBox(
                owner.painter,
                kTownHighlightX,
                y,
                kTownHighlightWidth,
                kHighlightHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }

    if (static_cast<i32>(this->towns.size()) <= this->townShown)
    {
        return;
    }

    int arrows = 0;
    const u16 upMask = kNibble << (2 * kNibbleShift);
    const u16 downMask = kNibble << (3 * kNibbleShift);

    if ((page.scrollArrows & upMask) != 0)
    {
        // As the original, the low byte (the list's arrows) comes out 0xFF.
        page.scrollArrows = static_cast<u16>((page.scrollArrows & ~upMask) | ((page.scrollArrows & upMask) - 1));
        arrows = 1;
    }

    if ((page.scrollArrows & downMask) != 0)
    {
        page.scrollArrows = static_cast<u16>(page.scrollArrows - (1u << (3 * kNibbleShift)));
        arrows |= 2;
    }

    const auto count = static_cast<double>(this->towns.size());
    const auto to = static_cast<float>(static_cast<double>(this->townShown + this->townTop) / count);
    const auto from = static_cast<float>(static_cast<double>(this->townTop) / count);
    this->DrawScrollBar(kTownsBarX, kTownsBarY, kTownsBarHeight, arrows, from, to, this->townBar);
}

// swd32.exe 0x0043BE40: the exit warp of map `map` in maps.dat table 0x16 (its map, x and y, then the maps it serves,
// each list ended by 0xFFFF); 0 when the map has none
std::size_t Menu::FindExitWarp(int map) const
{
    const std::vector<u8>& maps = this->game.databases.mapsData;
    std::size_t entry = MapsTable(maps, MapsDataTable::ExitWarps);

    if (TableWord(maps, entry) == kTableEnd)
    {
        return 0;
    }

    while (entry + sizeof(u16) <= maps.size())
    {
        std::size_t at = entry + kTownWords * sizeof(u16);

        for (u16 word = TableWord(maps, at); word != kTableEnd; word = TableWord(maps, at))
        {
            if (word == static_cast<u16>(map))
            {
                return entry;
            }

            at += sizeof(u16);
        }

        entry = at + sizeof(u16);

        if (TableWord(maps, entry) == kTableEnd)
        {
            return 0;
        }
    }

    return 0;
}

// swd32.exe 0x0043BE90: the towns of maps.dat table 0x17 the party may warp to: each a name ended by %Q, its map, x
// and y, then the flags (from 0x1388) any of which opens it, ended by 0xFFFF (names past 0x40 bytes are cut, where the
// original writes past its buffer)
void Menu::ListTowns()
{
    const Game& owner = this->game;
    const std::vector<u8>& maps = owner.databases.mapsData;
    this->towns.clear();
    std::size_t at = MapsTable(maps, MapsDataTable::Towns);

    if (TableWord(maps, at) == kTableEnd)
    {
        return;
    }

    std::size_t flags = at;

    do
    {
        std::string name;

        while (at + 1 < maps.size() && TableWord(maps, at) != kTownNameEnd)
        {
            if (name.size() < kTownNameBytes)
            {
                name.push_back(static_cast<char>(maps[at]));
            }

            ++at;
        }

        const std::size_t place = at + sizeof(u16);
        flags = place + kTownWords * sizeof(u16);
        bool open = false;
        u16 flag = TableWord(maps, flags);

        if (flag != kTableEnd)
        {
            do
            {
                if (owner.state.GameFlag(flag + kTownFlags))
                {
                    open = true;
                }

                flag = TableWord(maps, flags + sizeof(u16));
                flags += sizeof(u16);
            } while (flag != kTableEnd);

            if (open && this->towns.size() < kTownsMost)
            {
                this->towns.push_back(
                    {TableWord(maps, place),
                     TableWord(maps, place + sizeof(u16)),
                     TableWord(maps, place + 2 * sizeof(u16)),
                     name}
                );
            }
        }

        at = flags + sizeof(u16);
    } while (flags + 2 * sizeof(u16) <= maps.size() && TableWord(maps, flags + sizeof(u16)) != kTableEnd);
}

// swd32.exe 0x0043BFC0: the party warped to (map, x, y): the screen cleared, the map changed, the quake stopped, the
// leader set to run event `event` from the controlled role's place
void Menu::Warp(u16 map, u16 x, u16 y, i16 event)
{
    Game& owner = this->game;
    std::fill_n(owner.drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height, u16{0});
    owner.ShowFrame();
    const Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
    owner.ChangeMap(map, x, y, player.act.id, 0, 1, 1);
    owner.scene.shake.fill(Scene::ShakeEnd);
    Role& leader = owner.state.leader;
    leader.event = static_cast<u16>(event);
    leader.unk14 = 0;
    leader.unk20 = 0;
    leader.id = kWarpLeader;
    leader.flags = player.flags;
    leader.x = player.x;
    leader.y = player.y;
}

}  // namespace swd3
