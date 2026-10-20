#include <algorithm>
#include <array>
#include <span>
#include <string>

#include "core/debug_log.h"
#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "map/field.h"
#include "menu/menu.h"
#include "menu/menu_list.h"
#include "platform/display.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// The codes of the page (Menu::code).
constexpr u16 kListCode = 2;
constexpr u16 kRoleCode = 3;         // the role an object is used on
constexpr u16 kDescriptionCode = 4;  // the description box
constexpr u16 kDiscardCode = 5;      // "discard it?"
constexpr u16 kFollowersCode = 0xA;  // the followers that can be called
constexpr u16 kFollowerCode = 0xB;   // "call it?" or "send it away?"
constexpr u16 kTownsCode = 0xF;      // the town warp
constexpr u16 kNoTownWarpCode = 0x11;
constexpr u16 kNoExitWarpCode = 0x12;
constexpr u16 kBestiaryCode = 0x1F4;  // the bestiary opened by its book

// The modes (Menu::mode): the three tabs.
constexpr u16 kUseMode = 0x1E;
constexpr u16 kSortMode = 0x1F;
constexpr u16 kDiscardMode = 0x20;
constexpr i32 kSortedGlow = 0x23;  // InventoryPage::glow counts on from here after sorting
constexpr i32 kSortedGlowEnd = 0x26;

// The objects with their own use.
constexpr u16 kSaveItem = 0x2D9;
constexpr u16 kBestiaryItem = 0x318;
constexpr u16 kTownWarpItem = 0x2B9;
constexpr u16 kExitWarpItem = 0x2B8;
constexpr u16 kOtherExitWarpItem = 0x2BA;
constexpr u16 kFollowerItem = 0x301;
constexpr u16 kHomeItem = 0x2DB;
constexpr int kTownWarpBlocked = GameState::FileNameA;
constexpr int kExitWarpBlocked = GameState::FileNameB;
constexpr int kHomeBlocked = GameState::Unk17;
constexpr int kNoWarpMap = 0x16;
constexpr i16 kWarpEvent = 0x52;
constexpr u16 kHomeEvent = 0x51;
constexpr u16 kWarpLeader = 1;
constexpr i32 kTownsShown = 8;
constexpr u32 kTileMask = 0xF;
constexpr int kTileShift = 4;

// The objects (ObjectData).
constexpr u16 kUseMask = 0xF;  // the low bits of usableBy: 0 not usable, 1 (red), 2 and up on a role
constexpr u16 kUsableMask = 6;
constexpr u16 kUsedOnRole = 2;
constexpr u16 kRedUse = 1;
constexpr u32 kKeptKind = 0x20;      // ObjectData::kind: cannot be discarded
constexpr u32 kNotSpentKind = 0x80;  // not used up
constexpr u32 kWholeParty = 1;       // SkillScript::flags: the object works on every party role
// The kinds of the categories 1..6 (swd32.exe 0x0049E328; category 0 lists the pending counts).
constexpr std::array<u32, 7> kCategoryKinds = {0x8000, 0x1003, 0x1C, 0x08000040, 0x700, 0x800, 0x20};
constexpr i32 kCategories = 7;
constexpr i32 kOwnKindCategory = 3;  // categories 3 and 6 take their kinds alone, the others not with theirs
constexpr i32 kLastCategory = 6;
constexpr u16 kHeadId = 0;  // the id the sorted insertion reads before the first node (0x004FD1EC, always 0)

// The followers.
constexpr u16 kFirstFollower = 0x47;
constexpr i32 kFollowerAct = 0x232B;
constexpr i32 kFollowerActionStep = 4;
constexpr i16 kGoneMap = 0x3E8;  // the map of a follower sent away
constexpr u32 kFollowerFlagsKept = 0x3FFF;
constexpr i32 kDirectionMask = 3;

// The clicks (Menu::clicks).
constexpr u32 kSingleClick = 1;
constexpr u32 kDoubleClick = 2;
constexpr u32 kLeftClicks = 3;
constexpr u32 kRightClick = 4;
constexpr u32 kRightClicks = 0xC;
constexpr u32 kAnyClicks = 0xF;

// The sounds.
constexpr int kMoveSound = 0x2E;
constexpr int kSortSound = 0x2D;
constexpr int kChosenSound = 0x8B;
constexpr int kRefusedSound = 0x8C;
constexpr int kSpentSound = 0xB8;

// The list.
constexpr int kItemRows = 0xD;
constexpr u16 kListDown = 0x30;  // InventoryPage::arrows: frames the list's arrows stay pressed (bits 0..3, 4..7)
constexpr u16 kListUp = 3;
constexpr u16 kTownsDown = 0x3000;  // the towns' (bits 8..11, 12..15)
constexpr u16 kTownsUp = 0x300;
constexpr u16 kNibble = 0xF;
constexpr i32 kDescriptionHidden = 0x1E0;  // Menu::descriptionTop
constexpr i32 kDescriptionOpen = 0x100;    // Menu::descriptionSlide
constexpr i32 kDescriptionClose = -0x100;
constexpr i32 kMenuBackground = 0x34;  // Menu::background actions
constexpr i32 kItemsBackground = 0x43;
constexpr i32 kBackgroundFrame = 4;  // the frame of the page that switches to it
constexpr i32 kGrowFrames = 5;       // the boxes grow in over five frames
constexpr i32 kBarFrame = 4;         // the scroll bar from this frame
constexpr i32 kPressedOffset = -1;   // Menu::buttonPress

// The mouse areas.
constexpr int kFollowersLeft = 0x138;
constexpr int kFollowersRight = 0x25C;
constexpr int kFollowersTop = 0x84;
constexpr int kFollowerHeight = 0x19;
constexpr int kChoicesLeft = 0x1B7;
constexpr int kChoicesRight = 0x23B;
constexpr int kChoicesTop = 0xBD;
constexpr int kChoicesBottom = 0xD6;
constexpr int kChoiceWidth = 0x42;
constexpr int kDiscardLeft = 0x1AC;
constexpr int kDiscardRight = 0x204;
constexpr int kDiscardTop = 0xBE;
constexpr int kDiscardBottom = 0xD6;
constexpr int kDiscardWidth = 0x2C;
constexpr int kItemHeight = 0x16;  // the row step of the boxes beside the list
constexpr int kButtonLeft = 0x216;
constexpr int kButtonRight = 0x260;
constexpr int kButtonTop = 0x1D0;
constexpr int kButtonBottom = 0x1E0;
constexpr int kModesLeft = 0xD8;
constexpr int kModesRight = 0x186;
constexpr int kModesTop = 0x3E;
constexpr int kModesBottom = 0x52;
constexpr int kModeWidth = 0x3C;
constexpr int kModeStep = 6;  // the tabs are picked in steps of 6 pixels, ten to a tab, the last two between them
constexpr int kModeSteps = 0xA;
constexpr int kModeGap = 8;
constexpr int kCategoriesLeft = 0xD8;
constexpr int kCategoriesRight = 0x1BA;
constexpr int kCategoriesTop = 0x60;
constexpr int kCategoriesBottom = 0x78;
constexpr int kCategoryShift = 5;
constexpr int kCategoryWidth = 0x21;
constexpr int kRolesLeft = 0x1B4;
constexpr int kRolesRight = 0x234;
constexpr int kRowsLeft = 0xD4;
constexpr int kRowsRight = 0x1CA;
constexpr int kRowsTop = 0x80;
constexpr int kRowsBottom = 0x1B4;
constexpr int kRowHeight = 0x18;
constexpr int kBarLeft = 0x1E0;
constexpr int kBarRight = 0x1F8;
constexpr int kUpArrowTop = 0x78;
constexpr int kUpArrowBottom = 0x88;
constexpr int kDownArrowTop = 0x1A6;
constexpr int kDownArrowBottom = 0x1B6;
constexpr int kTownBarLeft = 0x228;
constexpr int kTownBarRight = 0x236;
constexpr int kTownUpTop = 0xC2;
constexpr int kTownUpBottom = 0xD0;
constexpr int kTownDownTop = 0x18A;
constexpr int kTownDownBottom = 0x198;
constexpr int kTownsLeft = 0x190;
constexpr int kTownsRight = 0x222;
constexpr int kTownsTop = 0xC4;
constexpr int kTownsBottom = 0x18B;
constexpr int kTownHeight = 0x19;

// The texts.
constexpr std::array<std::u8string_view, 3> kModeLabels = {u8"使用", u8"整理", u8"丟棄"};  // swd32.exe 0x004A7030..
constexpr std::u8string_view kNoItems = u8"沒有物品";                                      // swd32.exe 0x004A6E4C
constexpr std::u8string_view kDiscardQuestion = u8"是否丟棄";                              // swd32.exe 0x004A700C
constexpr std::u8string_view kYesNo = u8"  是  否";                                        // swd32.exe 0x0049FA54
constexpr std::u8string_view kRecallQuestion = u8"使用中 要回收嗎?";                       // swd32.exe 0x004A6FF0
constexpr std::u8string_view kCallQuestion = u8"閒置中 要使用嗎?";                         // swd32.exe 0x004A6FDC
constexpr std::u8string_view kConfirmGiveUp = u8"確定  放棄";                              // swd32.exe 0x004A6FD0
constexpr std::u8string_view kCannotUseHere = u8"此地無法使用";                            // swd32.exe 0x004A6F64
constexpr std::u8string_view kBadObject = u8"!警告! 物[%d] 種[%x]錯誤 ";                   // swd32.exe 0x004A7038
constexpr std::string_view kNameFormat = " %-14s";
constexpr std::string_view kCountFormat = "%3d";
constexpr std::string_view kFollowerFormat = "%d   %s";
constexpr std::size_t kNoticeLines = 3;  // a refusal is drawn in three lines of sixteen bytes
constexpr std::size_t kNoticeLineBytes = 0x10;

// The drawing.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kGrayRed = 0xD;
constexpr u32 kGrayGreen = 0xD;
constexpr u32 kGrayBlue = 9;
constexpr u32 kRedRed = 0x18;
constexpr u32 kRedGreen = 0xA;
constexpr u32 kRedBlue = 0xB;
constexpr u32 kTargetRed = 0x1B;
constexpr u32 kTargetGreen = 0x15;
constexpr u32 kTargetBlue = 0xE;
constexpr int kDim = -8;
constexpr int kPanelStyle = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kOtherHighlightRed = 0xC;
constexpr int kOtherHighlightGreen = 5;
constexpr int kListBoxX = 0xD0;
constexpr int kListBoxY = 0x74;
constexpr int kListBoxWidth = 0x110;
constexpr int kListBoxGrowth = 0x136;
constexpr int kListBoxExtra = 0x10;
constexpr int kListBoxBlue = 0x60;
constexpr int kListFrameLeft = 0xD8;
constexpr int kListFrameTop = 0x7C;
constexpr int kListFrameRight = 0x1D8;
constexpr int kHelpBoxX = 0xD0;
constexpr int kHelpBoxY = 0x1BA;
constexpr int kHelpBoxWidth = 0x190;
constexpr int kHelpBoxHeight = 0x24;
constexpr int kHelpBoxBlue = 0x48;
constexpr int kHelpFrameLeft = 0xD8;
constexpr int kHelpFrameTop = 0x1C2;
constexpr int kHelpFrameRight = 0x258;
constexpr int kHelpFrameBottom = 0x1D6;
constexpr int kScreenWidth = Display::Width;
constexpr int kScreenHeight = Display::Height;
constexpr int kTabsClipGrowth = 0xB0;
constexpr int kTabsClipLeft = 0xD8;
constexpr int kTabX = 0xE0;
constexpr int kTabY = 0x3E;
constexpr int kTabStep = 0x3C;
constexpr i32 kInterfaceAct = Scene::InterfaceAct;
constexpr i32 kCategoryBarAction = 7;
constexpr int kCategoryBarX = 0xD4;
constexpr int kCategoryBarY = 0x64;
constexpr i32 kFirstCategoryAction = 0x3C;
constexpr int kCategoryIconX = 0xD4;
constexpr int kCategoryIconY = 0x61;
constexpr int kCategoryIconStep = 0x20;
constexpr int kListClipGrowth = 0x13A;
constexpr int kListClipTop = 0x7C;
constexpr i32 kPictureAction = 0x44;
constexpr int kPictureX = 0x204;
constexpr int kPictureY = 0x76;
constexpr int kHelpClipX = 0xD4;
constexpr int kHelpClipY = 0x1C2;
constexpr int kHelpClipWidth = 0x188;
constexpr int kHelpClipHeight = 0x19;
constexpr int kHelpTextX = 0xDA;
constexpr int kHelpTextY = 0x1C4;
constexpr i32 kButtonAction = 0x20;
constexpr int kButtonX = 0x21C;
constexpr int kButtonY = 0x1D4;
constexpr int kNameX = 0xDC;
constexpr int kNameY = 0x80;
constexpr int kCountX = 0x17A;
constexpr int kCountY = 0x85;
constexpr int kHighlightX = 0xD3;
constexpr int kHighlightWidth = 0x10A;
constexpr int kListBarX = 0x1E0;
constexpr int kListBarY = 0x88;
constexpr int kListBarHeight = 0x11E;
constexpr int kRoleBoxX = 0x1B4;
constexpr int kRoleBoxTop = 0x88;
constexpr int kRoleBoxLowest = 0x168;
constexpr int kRoleBoxWidth = 0x80;
constexpr int kRoleBoxHeight = 0x68;
constexpr int kRoleBoxRed = 0x10;
constexpr int kRoleBoxGreen = 0x10;
constexpr int kRoleBoxBlue = 0x80;
constexpr int kRoleBoxClick = 2;  // InventoryPage::unk4FD148 is the box's top and this
constexpr int kRoleFrameLeft = 0x1BC;
constexpr int kRoleFrameDown = 8;
constexpr int kRoleFrameRight = 0x22C;
constexpr int kRoleFrameBottom = 0x64;
constexpr int kRoleNameX = 0x1BC;
constexpr int kRoleNameDown = 8;
constexpr int kRoleHighlightX = 0x1B8;
constexpr int kRoleHighlightDown = 4;
constexpr int kRoleHighlightWidth = 0x7A;
constexpr int kRoleHighlightHeight = 0x14;
constexpr int kDiscardBoxX = 0x1A8;
constexpr int kDiscardBoxY = 0xA8;
constexpr int kDiscardBoxWidth = 0x68;
constexpr int kDiscardBoxHeight = 0x30;
constexpr int kDiscardHighlightHeight = 0x18;
constexpr int kDiscardAnswersY = 0xC0;
constexpr int kFollowersBoxX = 0x138;
constexpr int kFollowersBoxY = 0x7C;
constexpr int kFollowersBoxWidth = 0x124;
constexpr int kFollowersBoxExtra = 8;
constexpr int kFollowersShade = 6;
constexpr int kFollowersFrameLeft = 0x140;
constexpr int kFollowersFrameTop = 0x80;
constexpr int kFollowersFrameRight = 0x258;
constexpr int kFollowerFaceX = 0x154;
constexpr int kFollowerFaceY = 0x82;
constexpr int kFollowerTextX = 0x144;
constexpr int kFollowerTextY = 0x84;
constexpr int kFollowerHighlightX = 0x13C;
constexpr int kFollowerHighlightWidth = 0x120;
constexpr int kAskBoxX = 0x178;
constexpr int kAskBoxY = 0x9C;
constexpr int kAskBoxWidth = 0xDA;
constexpr int kAskBoxHeight = 0x3C;
constexpr int kAskShade = -0x18;
constexpr int kAskFrameLeft = 0x17C;
constexpr int kAskFrameTop = 0xA0;
constexpr int kAskFrameRight = 0x24E;
constexpr int kAskFrameBottom = 0xD4;
constexpr int kAskTextX = 0x180;
constexpr int kAskTextY = 0xA4;
constexpr int kAskAnswersX = 0x1C2;
constexpr int kAskAnswersY = 0xBD;
constexpr int kAskHighlightY = 0xBB;
constexpr int kTownsBoxX = 0x190;
constexpr int kTownsBoxY = 0xC4;
constexpr int kTownsBoxWidth = 0xA2;
constexpr int kTownsBoxHeight = 0xD0;
constexpr int kTownsBoxBlue = 4;
constexpr int kTownsX = 0x190;
constexpr int kTownsY = 0xC2;
constexpr int kTownHighlightX = 0x18B;
constexpr int kTownHighlightWidth = 0xAC;
constexpr int kTownsBarX = 0x226;
constexpr int kTownsBarY = 0xD0;
constexpr int kTownsBarHeight = 0xB8;
constexpr int kNoticeX = 0x12C;
constexpr int kNoticeY = 0xF0;
constexpr int kNoticeWidth = 0xB0;
constexpr int kNoticeHeight = 0x48;
constexpr int kNoticeLineHeight = 0x18;

// The first party role from `from` on, wrapping, whose tab is in use (stepping by `step`).
int NextRole(const std::array<MenuTab, Menu::Tabs>& tabs, int from, int step)
{
    int role = from;

    do
    {
        role += step;

        if (role >= Menu::Tabs)
        {
            role = 0;
        }

        if (role < 0)
        {
            role = Menu::Tabs - 1;
        }
    } while (tabs[static_cast<std::size_t>(role)].role == Menu::NoRole);

    return role;
}

}  // namespace

// The description of the chosen row of the items page (the original describes the node, 0x0043B9E0; none past the
// end, where it reads through a null node) (inline in the original)
void Menu::ItemsDescribe()
{
    const Game& owner = this->game;
    const ItemNode* chosen = ItemAfter(this->itemList.row + this->itemList.top, this->itemObjects);
    this->description = chosen != nullptr ? ObjectDescription(owner.databases.mapsData, chosen->id) : std::string();
}

// The view of the items list fitted after it changed (0x0043BCC0 with 13 rows) (inline in the original)
void Menu::ItemsFit()
{
    Game& owner = this->game;
    this->description =
        FitMenuList(this->itemList, this->itemObjects, kItemRows, owner.database, owner.databases.mapsData);
}

// The view moved in the list (the first node and the rows shown again, the arrows pressed, the row described)
// (inline in the original)
void Menu::ItemsMoved(u16 arrows)
{
    Game& owner = this->game;
    SkipItems(this->itemList.top, this->itemObjects, this->itemList.first);
    CountShown(this->itemList.first, this->itemList.shown, kItemRows);
    this->itemsPage.arrows |= arrows;
    this->unk4FC200 = static_cast<u16>(this->itemList.row);
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    this->ItemsDescribe();
}

// swd32.exe 0x00445430: the items page opened on the first category in the use mode: its list taken from the
// inventory, the party roles counted, the chosen row described
void Menu::ItemsEnter()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;
    this->roleTab = Menu::NoTab;
    page.openedAt = this->frames - 1;
    ResetPlayback(this->buttonAct);
    this->buttonAct.id = kInterfaceAct;
    this->buttonAct.action = kCategoryBarAction;
    this->mode = kUseMode;
    page.category = 0;
    page.unk4FD160 = 0;
    this->descriptionSlide = 0;
    this->descriptionTop = kDescriptionHidden;
    this->itemList.first = nullptr;
    this->itemList.count = 0;
    this->itemList.top = 0;
    this->itemList.row = 0;
    this->ItemsFill();
    page.target = 0;
    page.roles = 0;

    for (int role = 0; role < Menu::Tabs; ++role)
    {
        if (owner.state.GameFlag(Party::FirstRoleFlag + role))
        {
            ++page.roles;
        }
    }

    this->ItemsDescribe();
    page.unk4FC3D4 = 0;
    page.unk4FC3D0 = InventoryPage::Unk4FC3D0;
    page.unk4FC3DC = InventoryPage::Unk4FC3DC;
    page.glow = this->mode;
    this->scrollBar = MenuScrollBar{};
    this->townBar = MenuScrollBar{};
    this->savedInventory.reset();
    this->savedItems.reset();
}

// swd32.exe 0x004455A0: the items page left: its list back in the inventory
void Menu::ItemsLeave()
{
    InventoryPage& page = this->itemsPage;
    this->ItemsReturn();
    page.category = 0;
    page.unk4FD160 = 0;
    this->itemList.first = nullptr;
    this->itemList.count = 0;
    this->itemList.top = 0;
    this->itemList.row = 0;
}

// swd32.exe 0x004455E0: the mouse on the items page: the followers and the follower box, the discard box, the
// description button, the mode tabs (a click on the sort tab sorts at once), the category icons, the role box, the rows
// (a click chooses, a double click uses), the list's and the towns' scroll bars (while the left button repeats), the
// towns; a right click goes back, on a refusal any click
void Menu::ItemsFrame()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;
    u16 code = this->code;

    if (code >= kBestiaryCode)
    {
        this->BestiaryFrame();
        return;
    }

    u32 clicks = this->clicks;
    this->buttonPress = 0;

    if ((code == kNoTownWarpCode || code == kNoExitWarpCode) && (clicks & kAnyClicks) != 0)
    {
        this->code = kListCode;
        return;
    }

    const int x = owner.state.pointerX;
    int y = owner.state.pointerY;
    bool boxes = true;  // the boxes of the codes are looked at (the original jumps past them to LABEL_56 otherwise)

    if (code == kFollowersCode)
    {
        if (x < kFollowersRight && x > kFollowersLeft && y < kFollowerHeight * page.followers + kFollowersTop &&
            y > kFollowersTop &&
            static_cast<int>(static_cast<u32>(y - kFollowersTop) / kFollowerHeight) <= page.followers)
        {
            page.followerRow = static_cast<i32>(static_cast<u32>(y - kFollowersTop) / kFollowerHeight);

            if ((clicks & kLeftClicks) != 0)
            {
                this->ItemsSelect();
                return;
            }
        }

        if ((clicks & kRightClick) == 0)
        {
            boxes = false;
        }
        else
        {
            this->ItemsCancel();
            code = this->code;
            clicks = this->clicks;
        }
    }

    if (boxes && code == kFollowerCode)
    {
        const int top = kFollowerHeight * page.followerRow;

        if (x < kChoicesRight && x > kChoicesLeft && y < top + kChoicesBottom && y > top + kChoicesTop)
        {
            page.unk4FD148 = (x - kChoicesLeft) / kChoiceWidth;

            if ((clicks & kLeftClicks) != 0)
            {
                this->ItemsSelect();
                return;
            }
        }

        if ((clicks & kRightClick) == 0)
        {
            boxes = false;
        }
        else
        {
            this->ItemsCancel();
            code = this->code;
            clicks = this->clicks;
        }
    }

    if (boxes && code == kDiscardCode)
    {
        const int top = kItemHeight * this->itemList.row;

        if (y < top + kDiscardBottom && y > top + kDiscardTop && static_cast<u32>(x) < kDiscardRight &&
            static_cast<u32>(x) > kDiscardLeft)
        {
            page.discardChoice = (x - kDiscardLeft) / kDiscardWidth;

            if ((clicks & kLeftClicks) != 0)
            {
                this->ItemsSelect();
                return;
            }

            if ((clicks & kRightClicks) != 0)
            {
                this->ItemsCancel();
                return;
            }
        }
    }
    else if (boxes && code == kListCode)
    {
        if (static_cast<u32>(y) < kButtonBottom && static_cast<u32>(y) > kButtonTop &&
            static_cast<u32>(x) < kButtonRight && static_cast<u32>(x) > kButtonLeft)
        {
            this->buttonPress = kPressedOffset;

            if ((clicks & kLeftClicks) != 0)
            {
                owner.input.PressKey(owner.config.keys[static_cast<std::size_t>(Button::F1)]);
                code = this->code;
                clicks = this->clicks;
            }
        }

        if (y < kModesBottom && y > kModesTop && x < kModesRight && x > kModesLeft && (clicks & kLeftClicks) != 0)
        {
            if ((x - kModesLeft) / kModeWidth == 1)
            {
                if ((clicks & kSingleClick) != 0)
                {
                    this->mode = kSortMode;
                    this->ItemsSelect();
                    this->mode = kUseMode;
                    owner.audio.PlaySound(kSortSound, owner.config.soundVolume);
                    page.glow = kSortMode;
                }
                else
                {
                    this->mode = kUseMode;
                }
            }
            else if ((clicks & kSingleClick) != 0)
            {
                const int step = (x - kModesLeft) / kModeStep;

                if (static_cast<u32>(step % kModeSteps) < kModeGap)
                {
                    this->mode = static_cast<u16>(step / kModeSteps + kUseMode);
                    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
                }
            }
            else
            {
                page.glow = this->mode;
            }

            return;
        }
    }

    if (code == kListCode)
    {
        if ((clicks & kLeftClicks) != 0 && y < kCategoriesBottom && y > kCategoriesTop && x < kCategoriesRight &&
            x > kCategoriesLeft && page.category != (x - kCategoriesLeft) >> kCategoryShift)
        {
            page.category = (x - kCategoriesLeft) / kCategoryWidth + 1;
            this->ItemsLeft();
            return;
        }

        if (y < kRowsBottom && y > kRowsTop && x < kRowsRight && x > kRowsLeft)
        {
            if ((clicks & kSingleClick) != 0)
            {
                const int row = (y - kRowsTop) / kRowHeight;

                if (row != this->itemList.row)
                {
                    if (row < this->itemList.shown)
                    {
                        this->itemList.row = row;
                        this->ItemsDescribe();
                        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
                    }

                    return;
                }
            }

            if ((clicks & kDoubleClick) != 0)
            {
                this->ItemsSelect();
                return;
            }
        }
    }
    else if (
        code == kRoleCode && y < page.unk4FD148 + kItemHeight * page.roles && y > page.unk4FD148 && x < kRolesRight &&
        x > kRolesLeft
    )
    {
        y -= page.unk4FD148;
        int role = 0;

        if ((clicks & kLeftClicks) != 0)
        {
            for (int step = y / kItemHeight; step > 0; --step)
            {
                int flag = role + Party::FirstRoleFlag;

                do
                {
                    ++role;
                    ++flag;
                } while (!owner.state.GameFlag(flag));
            }

            if (page.target != role)
            {
                page.target = role;
                return;
            }

            this->ItemsSelect();
            return;
        }

        if ((clicks & kRightClicks) != 0)
        {
            this->ItemsCancel();
            return;
        }
    }

    y = owner.state.pointerY;
    bool towns = false;  // LABEL_107 in the original

    if (owner.controls.Repeating(Button::MouseLeft))
    {
        if (this->code != kListCode)
        {
            towns = true;
        }
        else if (this->itemList.count > kItemRows && static_cast<u32>(x) < kBarRight && static_cast<u32>(x) > kBarLeft)
        {
            const MenuScrollBar& bar = this->scrollBar;

            if (y < kUpArrowBottom && y > kUpArrowTop)
            {
                this->ItemsUp();
            }

            if (y < kDownArrowBottom && y > kDownArrowTop)
            {
                this->ItemsDown();
            }

            if (y < bar.thumbTop && y > bar.top)
            {
                this->ItemsPageUp();
            }

            if (y < bar.bottom && y > bar.thumbBottom)
            {
                this->ItemsPageDown();
            }

            towns = true;
        }
    }

    if (towns && this->code == kTownsCode)
    {
        if (static_cast<i32>(this->towns.size()) <= kTownsShown || static_cast<u32>(x) >= kTownBarRight ||
            static_cast<u32>(x) <= kTownBarLeft)
        {
            if (static_cast<u32>(x) < kTownsRight && static_cast<u32>(x) > kTownsLeft &&
                static_cast<u32>(y) < kTownsBottom && static_cast<u32>(y) > kTownsTop)
            {
                int row = static_cast<int>(static_cast<u32>(y - kTownsTop) / kTownHeight);

                if (row >= this->townShown)
                {
                    row = this->townShown - 1;
                }

                if (row == this->townRow)
                {
                    if ((this->clicks & kDoubleClick) != 0)
                    {
                        this->ItemsSelect();
                    }

                    return;
                }

                if ((this->clicks & kLeftClicks) != 0)
                {
                    this->townRow = row;
                }
            }
        }
        else
        {
            const MenuScrollBar& bar = this->townBar;

            if (y < kTownUpBottom && y > kTownUpTop)
            {
                this->ItemsUp();
            }

            if (y < kTownDownBottom && y > kTownDownTop)
            {
                this->ItemsDown();
            }

            if (y < bar.thumbTop && y > bar.top)
            {
                this->ItemsPageUp();
            }

            if (y < bar.bottom && y > bar.thumbBottom)
            {
                this->ItemsPageDown();
            }
        }

        return;
    }

    if ((this->clicks & kRightClick) != 0)
    {
        this->ItemsCancel();
    }
}

// swd32.exe 0x00445C90: down: the next row (the description box closed), the next party role, "no", the next
// follower, the next town
void Menu::ItemsDown()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryDown();
        return;
    }

    switch (this->code)
    {
        case kListCode:
            if (this->mode == kSortMode)
            {
                break;
            }

            this->descriptionTop = kDescriptionHidden;
            this->descriptionSlide = 0;
            ListDown(this->itemList.count, this->itemList.top, this->itemList.row, this->itemList.shown);
            this->ItemsMoved(kListDown);
            break;

        case kRoleCode:
            page.target = NextRole(this->tabs, page.target, 1);
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            break;

        case kDiscardCode:
            page.discardChoice = 1;
            break;

        case kFollowersCode:
            if (++page.followerRow >= page.followers)
            {
                page.followerRow = page.followers - 1;
            }

            break;

        case kFollowerCode:
            page.unk4FD148 = 1;
            break;

        case kTownsCode:
            ListDown(static_cast<i32>(this->towns.size()), this->townTop, this->townRow, this->townShown);
            page.arrows |= kTownsDown;
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// swd32.exe 0x00445E90: up: the row before (the description box closed), the party role before, "yes", the follower
// before, the town before
void Menu::ItemsUp()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryUp();
        return;
    }

    switch (this->code)
    {
        case kListCode:
            if (this->mode == kSortMode)
            {
                break;
            }

            this->descriptionTop = kDescriptionHidden;
            this->descriptionSlide = 0;
            ListUp(this->itemList.top, this->itemList.row);
            this->ItemsMoved(kListUp);
            break;

        case kRoleCode:
            page.target = NextRole(this->tabs, page.target, -1);
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            break;

        case kDiscardCode:
            page.discardChoice = 0;
            break;

        case kFollowersCode:
            if (--page.followerRow < 0)
            {
                page.followerRow = 0;
            }

            break;

        case kFollowerCode:
            page.unk4FD148 = 0;
            break;

        case kTownsCode:
            ListUp(this->townTop, this->townRow);
            page.arrows |= kTownsUp;
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// swd32.exe 0x00446090: Page Down: a page of 13 rows on, the last party role, "no", the last follower, a page of 8
// towns on
void Menu::ItemsPageDown()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryPageDown();
        return;
    }

    switch (this->code)
    {
        case kListCode:
            if (this->mode == kSortMode)
            {
                break;
            }

            ListPageDown(this->itemList.count, this->itemList.top, this->itemList.row, this->itemList.shown, kItemRows);
            this->ItemsMoved(kListDown);
            break;

        case kRoleCode:
            page.target = NextRole(this->tabs, Menu::Tabs, -1);
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            break;

        case kDiscardCode:
            page.discardChoice = 1;
            break;

        case kFollowersCode:
            page.followerRow = page.followers - 1;
            break;

        case kFollowerCode:
            page.unk4FD148 = 1;
            break;

        case kTownsCode:
            ListPageDown(
                static_cast<i32>(this->towns.size()),
                this->townTop,
                this->townRow,
                this->townShown,
                kTownsShown
            );
            page.arrows |= kTownsDown;
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// swd32.exe 0x00446260: Page Up: a page of 13 rows back, the first party role, "yes", the first follower, a page of 8
// towns back
void Menu::ItemsPageUp()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryPageUp();
        return;
    }

    switch (this->code)
    {
        case kListCode:
            if (this->mode == kSortMode)
            {
                break;
            }

            ListPageUp(this->itemList.top, this->itemList.row, kItemRows);
            this->ItemsMoved(kListUp);
            break;

        case kRoleCode:
            page.target = NextRole(this->tabs, -1, 1);
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            break;

        case kDiscardCode:
            page.discardChoice = 0;
            break;

        case kFollowersCode:
            page.followerRow = 0;
            break;

        case kFollowerCode:
            page.unk4FD148 = 0;
            break;

        case kTownsCode:
            ListPageUp(this->townTop, this->townRow, kTownsShown);
            page.arrows |= kTownsUp;
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// The list of the next category made (the description box closed, the row described) (inline in the original)
void Menu::ItemsCategory(int step)
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;
    this->descriptionTop = kDescriptionHidden;
    this->descriptionSlide = 0;
    this->ItemsReturn();

    if (step < 0)
    {
        if (page.category-- == 0)
        {
            page.category = kLastCategory;
        }
    }
    else if (++page.category == kCategories)
    {
        page.category = 0;
    }

    this->ItemsFill();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    this->ItemsDescribe();
}

// swd32.exe 0x00446420: left: the category before (from the first to the last), "yes", the follower before, the town
// before (the towns' arrows not pressed)
void Menu::ItemsLeft()
{
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryLeft();
        return;
    }

    if (this->code == kListCode)
    {
        if (this->mode == kSortMode)
        {
            page.glow = this->mode;
            return;
        }

        this->ItemsCategory(-1);
    }

    switch (this->code)
    {
        case kDiscardCode:
            page.discardChoice = 0;
            break;

        case kFollowersCode:
            if (--page.followerRow < 0)
            {
                page.followerRow = 0;
            }

            break;

        case kFollowerCode:
            page.unk4FD148 = 0;
            break;

        case kTownsCode:
            ListUp(this->townTop, this->townRow);
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// swd32.exe 0x00446550: right: the next category (past the last back to the first), "no", the next follower, the next
// town (the towns' arrows not pressed)
void Menu::ItemsRight()
{
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryRight();
        return;
    }

    if (this->code == kListCode)
    {
        if (this->mode == kSortMode)
        {
            page.glow = this->mode;
            return;
        }

        this->ItemsCategory(1);
    }

    switch (this->code)
    {
        case kDiscardCode:
            page.discardChoice = 1;
            break;

        case kFollowersCode:
            if (++page.followerRow >= page.followers)
            {
                page.followerRow = page.followers - 1;
            }

            break;

        case kFollowerCode:
            page.unk4FD148 = 1;
            break;

        case kTownsCode:
            ListDown(static_cast<i32>(this->towns.size()), this->townTop, this->townRow, this->townShown);
            break;

        default:
            break;
    }

    page.glow = this->mode;
}

// swd32.exe 0x00446680: Shift: on the bestiary its next category
void Menu::ItemsShift()
{
    if (this->code >= kBestiaryCode)
    {
        this->BestiaryShift();
        return;
    }

    this->itemsPage.glow = this->mode;
}

// swd32.exe 0x004466A0: End: the next mode tab (wrapping); on the bestiary its next category
void Menu::ItemsEnd()
{
    Game& owner = this->game;

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryShift();
        return;
    }

    if (this->code == kListCode)
    {
        if (++this->mode > kDiscardMode)
        {
            this->mode = kUseMode;
        }

        owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
    }

    this->itemsPage.glow = this->mode;
}

// The towns of the town warp freed (the original leaves their top at their count) (inline in the original)
void Menu::ItemsFreeTowns()
{
    this->townTop = static_cast<i32>(this->towns.size());
    this->towns.clear();
}

// The object of the chosen row used in the use mode (inline in 0x00446700): the save crystal opens the save slots,
// the bestiary book the bestiary, the town warp its towns, an exit warp warps out at once, the follower whistle the
// followers, the home charm sends the party back (none spent); another object works on every party role when its
// script says so (one spent), otherwise a role is chosen for it
void Menu::ItemsUse(ItemNode& node)
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if ((node.record.data.usableBy & kUsableMask) == 0)
    {
        owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
        this->ItemsFit();
        return;
    }

    switch (node.id)
    {
        case kSaveItem:
            this->ItemsSave();
            owner.menuMode = Menu::ItemSaveMode;
            owner.saveScreenPage = 1;
            owner.slotsLoad = 0;
            owner.storyState = Game::SaveSlotsState;
            owner.ResetSaveSlots();
            return;

        case kBestiaryItem:
            this->code = kBestiaryCode;
            this->BestiaryEnter();
            return;

        case kTownWarpItem:
            if (owner.state.GameFlag(kTownWarpBlocked) || owner.scene.townWarpAllowed != 1)
            {
                this->code = kNoTownWarpCode;
                owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
                return;
            }

            this->code = kTownsCode;
            this->ListTowns();
            this->townTop = 0;
            this->townShown = kTownsShown;
            this->townRow = 0;

            if (static_cast<i32>(this->towns.size()) < kTownsShown)
            {
                this->townShown = static_cast<i32>(this->towns.size());
            }

            return;

        case kExitWarpItem:
        case kOtherExitWarpItem:
        {
            const std::size_t warp = owner.state.GameFlag(kExitWarpBlocked) ? 0 : this->FindExitWarp(owner.currentMap);

            if (warp == 0)
            {
                this->code = kNoExitWarpCode;
                owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
                return;
            }

            const std::vector<u8>& maps = owner.databases.mapsData;
            AddItem(this->itemObjects, node.id, -1, ItemCountMode::Pending, owner.database);
            this->Warp(
                MapsWord(maps, warp),
                MapsWord(maps, warp + sizeof(u16)),
                MapsWord(maps, warp + 2 * sizeof(u16)),
                kWarpEvent
            );
            this->ItemsLeave();
            owner.menuState = Game::NoMenu;
            return;
        }

        case kFollowerItem:
        {
            page.followerRow = 0;
            page.followers = node.count + node.pending;

            if (const ItemNode* more = FindItemOfKind(owner.party.inventory, kFollowerItem); more != nullptr)
            {
                page.followers += more->pending + more->count;
            }

            this->code = kFollowersCode;
            return;
        }

        case kHomeItem:
        {
            if (owner.currentMap == kNoWarpMap || owner.state.GameFlag(kHomeBlocked))
            {
                owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
                return;
            }

            const Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
            Role& leader = owner.state.leader;
            leader.unk14 = 0;
            leader.unk20 = 0;
            leader.event = kHomeEvent;
            leader.id = kWarpLeader;
            leader.x = player.x;
            owner.scene.returnColumn = static_cast<i32>(static_cast<u32>(player.x) >> kTileShift);
            owner.scene.returnRow = static_cast<i32>(static_cast<u32>(player.y) >> kTileShift);
            owner.scene.returnMap = owner.currentMap;
            owner.scene.warpLeaderAct = player.act.id;
            leader.flags = player.flags;
            leader.y = player.y;
            std::fill_n(owner.drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height, u16{0});
            owner.ShowFrame();
            this->ItemsLeave();
            owner.menuState = Game::NoMenu;
            return;
        }

        default:
            break;
    }

    SkillScript& script = this->skills.skill;
    script = SkillScript{};

    if (!owner.database.LoadSkill(script, node.record.data.unk3E) || (script.flags & kWholeParty) == 0)
    {
        this->code = kRoleCode;
        owner.audio.PlaySound(kChosenSound, owner.config.soundVolume);
        this->ItemsFit();
        return;
    }

    for (int role = 0; role < Menu::Tabs; ++role)
    {
        if (owner.state.GameFlag(Party::FirstRoleFlag + role))
        {
            ApplyObject(owner.party.roles[static_cast<std::size_t>(role)], node.record.data, owner.database);
        }
    }

    AddItem(this->itemObjects, node.id, -1, ItemCountMode::Pending, owner.database);
    this->ItemsFit();
}

// swd32.exe 0x00446700: Enter: on the list the row is used (use mode), the list sorted (sort mode) or asked to be
// discarded (discard mode, unless it is kept); on the role box the object works on the role (spent unless marked so);
// the discard box, the followers (a follower is asked about), the follower box (called or sent away; calling closes
// the menu), the towns (the party warps there)
void Menu::ItemsSelect()
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;

    if (this->code >= kBestiaryCode)
    {
        return;
    }

    switch (this->code)
    {
        case kListCode:
        {
            ItemNode* chosen = ItemAfter(this->itemList.top + this->itemList.row, this->itemObjects);

            if (this->mode != kSortMode &&
                (this->itemList.count <= 0 || chosen == nullptr || chosen->id >= ItemNode::Empty))
            {
                return;
            }

            if (this->mode == kUseMode)
            {
                this->ItemsUse(*chosen);
                return;
            }

            if (this->mode == kSortMode)
            {
                this->ItemsReturn();
                MergePendingCounts(owner.party.inventory);
                this->mode = kUseMode;
                owner.audio.PlaySound(kSortSound, owner.config.soundVolume);
                this->ItemsFill();
            }
            else if (this->mode == kDiscardMode)
            {
                page.discardChoice = 1;

                if ((chosen->record.data.kind & kKeptKind) == 0)
                {
                    this->code = kDiscardCode;
                }
                else
                {
                    owner.audio.PlaySound(kRefusedSound, owner.config.soundVolume);
                }
            }

            this->ItemsFit();
            return;
        }

        case kRoleCode:
        {
            ItemNode* chosen = ItemAfter(this->itemList.top + this->itemList.row, this->itemObjects);

            if (this->mode == kUseMode)
            {
                if (chosen != nullptr && (chosen->record.data.usableBy & kUseMask) >= kUsedOnRole)
                {
                    ApplyObject(
                        owner.party.roles[static_cast<std::size_t>(std::clamp(page.target, 0, Menu::Tabs - 1))],
                        chosen->record.data,
                        owner.database
                    );

                    if ((chosen->record.data.kind & kNotSpentKind) == 0 &&
                        AddItem(this->itemObjects, chosen->id, -1, ItemCountMode::Pending, owner.database) == nullptr)
                    {
                        this->code = kListCode;
                    }

                    owner.audio.PlaySound(kChosenSound, owner.config.soundVolume);
                }
            }
            else if (this->mode == kDiscardMode && chosen != nullptr && chosen->id != ItemNode::Empty)
            {
                if ((chosen->record.data.kind & kKeptKind) != 0)
                {
                    owner.audio.PlaySound(kSpentSound, owner.config.soundVolume);
                }
                else
                {
                    AddItem(this->itemObjects, chosen->id, -1, ItemCountMode::Pending, owner.database);
                    owner.audio.PlaySound(kChosenSound, owner.config.soundVolume);
                }
            }

            this->ItemsFit();
            return;
        }

        case kDescriptionCode:
            this->ItemsCancel();
            return;

        case kDiscardCode:
            if (page.discardChoice == 0)
            {
                const ItemNode* chosen = ItemAfter(this->itemList.top + this->itemList.row, this->itemObjects);

                if (chosen != nullptr && chosen->id != ItemNode::Empty)
                {
                    AddItem(this->itemObjects, chosen->id, -1, ItemCountMode::Pending, owner.database);
                    owner.audio.PlaySound(kSpentSound, owner.config.soundVolume);
                    this->ItemsFit();
                }
            }

            this->code = kListCode;
            return;

        case kFollowersCode:
        {
            Role role;

            if (owner.LoadRoleRecord(role, static_cast<u16>(page.followerRow + kFirstFollower)) == GameState::NotFound)
            {
                // swd32.exe 0x00446E7A: 0x004A6FA8 + 0x004A6FB8
                DebugLog(u8"靈界指標問題 Maps找不到這個指標 [%d]", page.followerRow);
                this->code = kListCode;
            }
            else
            {
                page.followerPresent = role.act.action != 0 ? 1 : 0;
                ++this->code;
                page.unk4FD148 = 0;
            }

            return;
        }

        case kFollowerCode:
            this->code = kFollowersCode;

            if (page.unk4FD148 == 0 && this->ItemsCallFollower())
            {
                this->code = kListCode;

                if (page.followerPresent == 0)
                {
                    this->ItemsCancel();
                    owner.menuState = Game::NoMenu;
                }
            }

            return;

        case kTownsCode:
        {
            AddItem(this->itemObjects, kTownWarpItem, -1, ItemCountMode::Pending, owner.database);
            const auto index = static_cast<std::size_t>(this->townTop + this->townRow);

            if (index < this->towns.size())
            {
                const MenuTown town = this->towns[index];
                this->Warp(town.map, town.x, town.y, kWarpEvent);
            }

            this->ItemsFreeTowns();
            this->ItemsLeave();
            owner.menuState = Game::NoMenu;
            return;
        }

        case kNoTownWarpCode:
        case kNoExitWarpCode:
            this->code = kListCode;
            return;

        default:
            return;
    }
}

// swd32.exe 0x00446FE0: Escape: the description box closes, the list closes the page (back to the menu), the discard
// box answers "no", the followers and the towns close, the follower box goes back to the followers
void Menu::ItemsCancel()
{
    if (this->code >= kBestiaryCode)
    {
        this->BestiaryCancel();
        return;
    }

    switch (--this->code)
    {
        case kDescriptionCode - 1:
            this->code = kListCode;
            this->descriptionSlide = kDescriptionClose;
            return;

        case kListCode - 1:
            this->ItemsLeave();
            this->SetPage(this->code);
            this->background.action = kMenuBackground;
            return;

        case kDiscardCode - 1:
            this->itemsPage.discardChoice = 1;
            this->code = kListCode;
            return;

        case kFollowersCode - 1:
        case kNoTownWarpCode - 1:
        case kNoExitWarpCode - 1:
            this->code = kListCode;
            return;

        case kFollowerCode - 1:
            this->code = kFollowersCode;
            return;

        case kTownsCode - 1:
            this->ItemsFreeTowns();
            this->code = kListCode;
            return;

        default:
            this->itemsPage.glow = this->mode;
            return;
    }
}

// The refusal of a warp in three lines of sixteen bytes: the text a talk command gave, "此地無法使用" without one
// (part of 0x00447100)
void Menu::ItemsDrawNotice(Pixel color)
{
    Game& owner = this->game;
    const std::optional<std::string>& given =
        this->code == kNoTownWarpCode ? owner.scene.noTownWarpText : owner.scene.noExitWarpText;
    const std::string text = given.value_or(Big5(kCannotUseHere));
    this->DrawBox(kNoticeX, kNoticeY, kNoticeWidth, kNoticeHeight, static_cast<int>(FillMode::Eighth), 0, 0, 0);

    for (std::size_t line = 0; line < kNoticeLines; ++line)
    {
        std::string part = line * kNoticeLineBytes < text.size()
                               ? text.substr(line * kNoticeLineBytes, kNoticeLineBytes)
                               : std::string();
        part.resize(std::min(part.size(), part.find('\0')));
        owner.largeFont.DrawText(
            owner.drawSurface,
            kNoticeX,
            kNoticeY + kNoticeLineHeight * static_cast<int>(line),
            part,
            color,
            kPanelStyle
        );
    }
}

// The followers with their maps, and the follower box (part of 0x00447100). A follower missing from the table shows
// the text drawn before it.
void Menu::ItemsDrawFollowers(Pixel color, std::string& line)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const InventoryPage& page = this->itemsPage;
    ActState& panel = owner.battleMessages.panelAct;
    this->gauge.id = kFollowerAct;
    FillBox(
        painter,
        kFollowersBoxX,
        kFollowersBoxY,
        kFollowersBoxWidth,
        kFollowerHeight * page.followers + kFollowersBoxExtra,
        kFollowersShade,
        kFollowersShade,
        kFollowersShade,
        FillMode::Eighth
    );
    DrawWindowFrame(
        painter,
        owner.archive,
        panel.frame,
        kFollowersFrameLeft,
        kFollowersFrameTop,
        kFollowersFrameRight,
        kFollowerHeight * page.followers + kFollowersFrameTop,
        0,
        kFrameBorder
    );

    // One record for all of them, as the original: a follower missing from the table keeps the one before it (the
    // original's first is uninitialised memory).
    Role role;

    for (int follower = 0, offset = 0; follower < page.followers; ++follower, offset += kFollowerHeight)
    {
        const int shift = page.followerRow == follower ? -1 : 0;
        const u32 map = owner.LoadRoleRecord(role, static_cast<u16>(follower + kFirstFollower));
        owner.MapName(static_cast<int>(map), line);
        this->gauge.action = role.act.action == 0 ? 1 : 0;
        owner.acts.Run(this->gauge);
        owner.actDrawer.Draw(this->gauge, shift + kFollowerFaceX, shift + offset + kFollowerFaceY);
        owner.largeFont.DrawText(
            owner.drawSurface,
            shift + kFollowerTextX,
            shift + offset + kFollowerTextY,
            WinFormat(kFollowerFormat, {follower, line}),
            color,
            kPanelStyle
        );

        if (page.followerRow == follower)
        {
            FillBox(
                painter,
                kFollowerHighlightX,
                offset + kFollowerFaceY,
                kFollowerHighlightWidth,
                kRowHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }

    if (this->code != kFollowerCode)
    {
        return;
    }

    const int top = kFollowerHeight * page.followerRow;
    FillBox(
        painter,
        kAskBoxX,
        top + kAskBoxY,
        kAskBoxWidth,
        kAskBoxHeight,
        kAskShade,
        kAskShade,
        kAskShade,
        FillMode::Tint
    );
    DrawWindowFrame(
        painter,
        owner.archive,
        panel.frame,
        kAskFrameLeft,
        top + kAskFrameTop,
        kAskFrameRight,
        top + kAskFrameBottom,
        0,
        kFrameBorder
    );
    line = Big5(page.followerPresent != 0 ? kRecallQuestion : kCallQuestion);
    owner.largeFont.DrawText(owner.drawSurface, kAskTextX, top + kAskTextY, line, color, kPanelStyle);
    owner.largeFont.DrawText(owner.drawSurface, kAskAnswersX, top + kAskAnswersY, kConfirmGiveUp, color, kPanelStyle);
    FillBox(
        painter,
        (page.unk4FD148 != 0 ? kChoiceWidth : 0) + kChoicesLeft,
        top + kAskHighlightY,
        kChoiceWidth,
        kRowHeight,
        kHighlightRed,
        kHighlightGreen,
        0,
        FillMode::Gradient
    );
}

// The towns of the town warp with their bar (part of 0x00447100)
void Menu::ItemsDrawTowns(Pixel color)
{
    Game& owner = this->game;
    InventoryPage& page = this->itemsPage;
    this->DrawBox(
        kTownsBoxX,
        kTownsBoxY,
        kTownsBoxWidth,
        kTownsBoxHeight,
        static_cast<int>(FillMode::Eighth),
        0,
        0,
        kTownsBoxBlue
    );

    for (int row = 0, y = kTownsY; row < this->townShown; ++row, y += kTownHeight)
    {
        const auto index = static_cast<std::size_t>(this->townTop + row);

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
                kRowHeight,
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
    const u32 upMask = static_cast<u32>(kNibble) << 8;
    const u32 downMask = static_cast<u32>(kNibble) << 0xC;

    if ((page.arrows & upMask) != 0)
    {
        // As the original, the low byte (the list's arrows) comes out 0xFF.
        page.arrows = static_cast<u16>((page.arrows & ~upMask) | ((page.arrows & upMask) - 1));
        arrows = 1;
    }

    if ((page.arrows & downMask) != 0)
    {
        page.arrows = static_cast<u16>(page.arrows - (1u << 0xC));
        arrows |= 2;
    }

    const auto count = static_cast<double>(this->towns.size());
    const auto to = static_cast<float>(static_cast<double>(this->townShown + this->townTop) / count);
    const auto from = static_cast<float>(static_cast<double>(this->townTop) / count);
    this->DrawScrollBar(kTownsBarX, kTownsBarY, kTownsBarHeight, arrows, from, to, this->townBar);
}

// swd32.exe 0x00447100: the items page (nothing while the save slots are shown; the bestiary when its book opened it).
// The description key on the list opens the description box. The boxes grow in over five frames: the mode tabs, the
// category icons, the rows (the chosen one with its picture, its description below and the description button; the
// unusable grey, the red ones red) with the scroll bar, the role box, the discard box, the description box, the
// followers, the towns, a refusal.
void Menu::ItemsDraw()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    InventoryPage& page = this->itemsPage;
    ActState& panel = owner.battleMessages.panelAct;
    const int elapsed = this->frames - page.openedAt;

    if (owner.storyState != 0)
    {
        return;
    }

    const auto color = static_cast<Pixel>(painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    Pixel textColor = color;
    const auto gray = static_cast<Pixel>(painter.MakeColor(kGrayRed, kGrayGreen, kGrayBlue));
    const auto red = static_cast<Pixel>(painter.MakeColor(kRedRed, kRedGreen, kRedBlue));

    if (this->code >= kBestiaryCode)
    {
        this->BestiaryDraw();
        return;
    }

    if (this->code == kListCode && owner.input.IsKeyDown(owner.config.keys[static_cast<std::size_t>(Button::F1)]) != 0)
    {
        ItemNode* first = nullptr;
        SkipItems(this->itemList.top, this->itemObjects, first);

        if (first != nullptr && first->id != ItemNode::Empty)
        {
            this->descriptionSlide = kDescriptionOpen;
            this->code = kDescriptionCode;
        }
    }

    owner.acts.Run(panel);
    // The rebuild's continuous motion: the boxes grow on to where this frame grew them (in Menu::GrowSteps a frame).
    constexpr int kGrowParts = kGrowFrames * Menu::GrowSteps;
    const int growth = std::min(elapsed, kGrowFrames) * Menu::GrowSteps;

    if (elapsed >= 1 && elapsed <= kGrowFrames)
    {
        this->Move(ItemsGrowValue, growth, growth - Menu::GrowSteps, growth);
    }

    const int grownParts = this->Shown(ItemsGrowValue, growth);
    int grown = kGrowParts;

    if (grownParts < kGrowParts)
    {
        FillBox(
            painter,
            kListBoxX,
            kListBoxY,
            kListBoxWidth,
            kListBoxGrowth * grownParts / kGrowParts + kListBoxExtra,
            0,
            0,
            kListBoxBlue,
            FillMode::Quarter
        );
        DrawWindowFrame(
            painter,
            owner.archive,
            panel.frame,
            kListFrameLeft,
            kListFrameTop,
            kListFrameRight,
            kListBoxGrowth * grownParts / kGrowParts + kListFrameTop,
            0,
            kFrameBorder
        );
        FillBox(painter, kHelpBoxX, kHelpBoxY, kHelpBoxWidth, kHelpBoxHeight, 0, 0, kHelpBoxBlue, FillMode::Quarter);
        DrawWindowFrame(
            painter,
            owner.archive,
            panel.frame,
            kHelpFrameLeft,
            kHelpFrameTop,
            kHelpFrameRight,
            kHelpFrameBottom,
            0,
            kFrameBorder
        );
        grown = grownParts;
    }

    owner.mediumFont.SetClip(0, 0, kTabsClipGrowth * grown / kGrowParts + kTabsClipLeft, kScreenHeight);

    for (int tab = 0; tab < static_cast<int>(kModeLabels.size()); ++tab)
    {
        int raise = 0;
        Pixel tabColor = color;
        painter.TintPixels(std::span<u16>(&tabColor, 1), kDim, kDim, kDim);

        if (page.glow == tab + kUseMode && this->page == Menu::FirstRolePage)
        {
            tabColor = color;
            raise = -1;
        }

        if (page.glow >= kSortedGlow && tab == 1)
        {
            tabColor = color;
            raise = -1;
        }

        owner.mediumFont.DrawText(
            owner.drawSurface,
            raise + kTabStep * tab + kTabX,
            raise + kTabY,
            kModeLabels[static_cast<std::size_t>(tab)],
            tabColor,
            kPanelStyle
        );
    }

    if (page.glow == kSortMode || page.glow >= kSortedGlow)
    {
        const i32 glow = page.glow == kSortMode ? kSortedGlow : page.glow;
        page.glow = glow + 1;

        if (glow + 1 > kSortedGlowEnd)
        {
            page.glow = this->mode;
        }
    }

    owner.mediumFont.SetClip(0, 0, kScreenWidth, kScreenHeight);
    this->buttonAct.id = kInterfaceAct;
    this->buttonAct.action = kCategoryBarAction;
    owner.actDrawer.Draw(this->buttonAct, kCategoryBarX, kCategoryBarY);
    this->buttonAct.id = kInterfaceAct;
    this->buttonAct.action = page.category + kFirstCategoryAction;
    owner.actDrawer.Draw(this->buttonAct, kCategoryIconStep * page.category + kCategoryIconX, kCategoryIconY);
    owner.largeFont
        .SetClip(0, 0, kScreenWidth, kListClipGrowth * std::min(grownParts, kGrowParts) / kGrowParts + kListClipTop);
    std::string line;  // the original's command line buffer, written by the rows and read by the followers

    if (this->itemObjects != nullptr)
    {
        const ItemNode* node = this->itemList.first;

        for (int row = 0, offset = 0; row < this->itemList.shown && node != nullptr;
             ++row, offset += kRowHeight, node = node->next.get())
        {
            int shift = 0;

            if (row == this->itemList.row && node->id != ItemNode::Empty)
            {
                if (node->record.data.unk50 != 0)
                {
                    this->gauge.id = node->record.data.unk50;
                    this->gauge.action = kPictureAction;
                    owner.actDrawer.Draw(this->gauge, kPictureX, kPictureY);
                }

                owner.mediumFont.SetClip(kHelpClipX, kHelpClipY, kHelpClipWidth, kHelpClipHeight);
                owner.mediumFont
                    .DrawText(owner.drawSurface, kHelpTextX, kHelpTextY, this->description, color, kPanelStyle);
                owner.mediumFont.SetClip(0, 0, kScreenWidth, kScreenHeight);
                this->buttonAct.id = kInterfaceAct;
                this->buttonAct.action = kButtonAction;
                owner.actDrawer.Draw(this->buttonAct, this->buttonPress + kButtonX, this->buttonPress + kButtonY);
                shift = -1;
            }

            Pixel rowColor = color;
            const u16 use = node->record.data.usableBy & kUseMask;

            if (use == 0)
            {
                shift = 0;
                rowColor = gray;
            }

            if (use == kRedUse)
            {
                shift = 0;
                rowColor = red;
            }

            if (this->code > kListCode)
            {
                painter.TintPixels(std::span<u16>(&rowColor, 1), kDim, kDim, kDim);
            }

            const i32 count = page.category == 0 ? node->pending : node->count;
            line = WinFormat(kNameFormat, {ObjectName(node->record)});
            owner.largeFont
                .DrawText(owner.drawSurface, shift + kNameX, offset + shift + kNameY, line, rowColor, kPanelStyle);

            if (node->id != ItemNode::Empty)
            {
                owner.mediumFont.DrawText(
                    owner.drawSurface,
                    shift + kCountX,
                    offset + shift + kCountY,
                    WinFormat(kCountFormat, {count}),
                    rowColor,
                    kPanelStyle
                );
            }

            if (row == this->itemList.row && this->mode != kSortMode)
            {
                const bool open = this->code == kListCode;
                FillBox(
                    painter,
                    kHighlightX,
                    offset + kNameY,
                    kHighlightWidth,
                    kRowHeight,
                    open ? kHighlightRed : kOtherHighlightRed,
                    open ? kHighlightGreen : kOtherHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }
        }
    }
    else
    {
        line = Big5(kNoItems);
        owner.largeFont.DrawText(owner.drawSurface, kNameX, kNameY, line, color, kPanelStyle);
    }

    owner.largeFont.SetClip(0, 0, kScreenWidth, kScreenHeight);

    if (elapsed >= kBarFrame)
    {
        int arrows = 0;

        if ((page.arrows & kNibble) != 0)
        {
            arrows = 1;
            page.arrows = static_cast<u16>((page.arrows & ~kNibble) | ((page.arrows & kNibble) - 1));
        }

        if ((page.arrows & (kNibble << 4)) != 0)
        {
            arrows |= 2;
            page.arrows = static_cast<u16>(page.arrows - (1u << 4));
        }

        if (this->itemList.count > kItemRows)
        {
            const auto count = static_cast<double>(this->itemList.count);
            const auto to = static_cast<float>(static_cast<double>(this->itemList.top + this->itemList.shown) / count);
            const auto from = static_cast<float>(static_cast<double>(this->itemList.top) / count);
            this->DrawScrollBar(kListBarX, kListBarY, kListBarHeight, arrows, from, to, this->scrollBar);
        }
    }

    if (this->code == kRoleCode && this->mode == kUseMode)
    {
        int top = kItemHeight * this->itemList.row + kRoleBoxTop;

        if (top > kRoleBoxLowest)
        {
            top = kRoleBoxLowest;
        }

        FillBox(
            painter,
            kRoleBoxX,
            top,
            kRoleBoxWidth,
            kRoleBoxHeight,
            kRoleBoxRed,
            kRoleBoxGreen,
            kRoleBoxBlue,
            FillMode::Eighth
        );
        page.unk4FD148 = top + kRoleBoxClick;
        owner.acts.Run(panel);
        DrawWindowFrame(
            painter,
            owner.archive,
            panel.frame,
            kRoleFrameLeft,
            top + kRoleFrameDown,
            kRoleFrameRight,
            top + kRoleFrameBottom,
            0,
            kFrameBorder
        );
        int y = top;

        for (int role = 0; role < Menu::Tabs; ++role)
        {
            if (this->tabs[static_cast<std::size_t>(role)].role == Menu::NoRole)
            {
                continue;
            }

            int shift = 0;
            owner.mediumFont.SetBackground(TextFont::NoBackground);
            textColor = gray;

            if (role == page.target)
            {
                if (this->code == kRoleCode)
                {
                    shift = -1;
                }

                textColor = static_cast<Pixel>(painter.MakeColor(kTargetRed, kTargetGreen, kTargetBlue));
            }

            const Party::Name& name = owner.party.names[static_cast<std::size_t>(role)];
            owner.mediumFont.DrawText(
                owner.drawSurface,
                shift + kRoleNameX,
                y + shift + kRoleNameDown,
                std::string_view(name.data(), strnlen(name.data(), name.size())),
                textColor,
                kPanelStyle
            );

            if (role == page.target && this->code == kRoleCode)
            {
                FillBox(
                    painter,
                    shift + kRoleHighlightX,
                    y + kRoleHighlightDown,
                    kRoleHighlightWidth,
                    kRoleHighlightHeight,
                    kHighlightRed,
                    kHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }

            y += kItemHeight;
        }
    }

    if (this->code == kDiscardCode)
    {
        const int top = kItemHeight * this->itemList.row;
        this->DrawBox(
            kDiscardBoxX,
            top + kDiscardBoxY,
            kDiscardBoxWidth,
            kDiscardBoxHeight,
            static_cast<int>(FillMode::Eighth),
            0,
            0,
            0
        );
        FillBox(
            painter,
            kDiscardWidth * page.discardChoice + kDiscardLeft,
            top + kDiscardTop,
            kDiscardWidth,
            kDiscardHighlightHeight,
            kHighlightRed,
            kHighlightGreen,
            0,
            FillMode::Gradient
        );
        owner.largeFont
            .DrawText(owner.drawSurface, kDiscardLeft, top + kDiscardBoxY, kDiscardQuestion, textColor, kPanelStyle);
        owner.largeFont
            .DrawText(owner.drawSurface, kDiscardLeft, top + kDiscardAnswersY, kYesNo, textColor, kPanelStyle);
    }

    if (const ItemNode* chosen = ItemAfter(this->itemList.top + this->itemList.row, this->itemObjects);
        chosen != nullptr && chosen->id != ItemNode::Empty)
    {
        this->DrawDescriptionBox(chosen->record.description.value_or(std::string()));
    }

    if (this->code == kFollowersCode || this->code == kFollowerCode)
    {
        this->ItemsDrawFollowers(textColor, line);
    }

    if (this->code == kTownsCode)
    {
        this->ItemsDrawTowns(textColor);
    }

    if (this->code == kNoTownWarpCode || this->code == kNoExitWarpCode)
    {
        this->ItemsDrawNotice(textColor);
    }

    if (elapsed == kBackgroundFrame)
    {
        this->background.action = kItemsBackground;
    }
}

// swd32.exe 0x00447FD0: the rows shown from the first one shown (at most 13); the original also numbers them from 100
// in a table at 0x004A6A00, ended by 0, that nothing reads
void Menu::ItemsCountShown()
{
    InventoryPage& page = this->itemsPage;

    for (std::size_t row = 0; row < kItemRows; ++row)
    {
        page.rowNumbers[row] = static_cast<u16>(InventoryPage::FirstRowNumber + row);
    }

    this->itemList.shown = 0;

    for (const ItemNode* node = this->itemList.first; node != nullptr && this->itemList.shown < kItemRows;
         node = node->next.get())
    {
        ++this->itemList.shown;
    }

    page.rowNumbers[static_cast<std::size_t>(this->itemList.shown)] = 0;
}

// swd32.exe 0x00448020: the objects of category `category` copied from the inventory into the page's list, sorted by
// id: category 0 takes the pending counts, the others the counts of the objects of their kinds (categories 3 and 6
// their kinds alone, the others without those two); an object without a kind is taken in any category (reported when
// the errors are). The inventory keeps the other count.
void Menu::ItemsTake(int category)
{
    Game& owner = this->game;
    this->itemObjects.reset();
    this->itemsPage.unk4FD1F0 = 0;

    for (ItemNode* source = owner.party.inventory.get(); source != nullptr; source = source->next.get())
    {
        auto copy = std::make_unique<ItemNode>();
        copy->id = source->id;
        copy->unk06 = source->unk06;
        copy->count = source->count;
        copy->pending = source->pending;
        copy->record.data = source->record.data;
        copy->record.description = source->record.description.value_or(std::string());
        const u32 kind = copy->record.data.kind;
        bool take = false;

        if (category == 0)
        {
            take = copy->pending != 0;
        }
        else if (copy->count != 0)
        {
            if (category == kOwnKindCategory || category == kLastCategory)
            {
                take = (kind & kCategoryKinds[static_cast<std::size_t>(category)]) != 0;
            }
            else if ((kCategoryKinds[static_cast<std::size_t>(category)] & kind) != 0)
            {
                take = ((kCategoryKinds[kLastCategory] + kCategoryKinds[kOwnKindCategory]) & kind) == 0;
            }
        }

        if (source->record.data.kind == 0)
        {
            if (owner.state.GameFlag(GameState::DebugReports))
            {
                DebugLog(kBadObject, source->id, source->record.data.kind);
            }

            take = true;
        }

        if (!take)
        {
            continue;
        }

        if (category != 0)
        {
            copy->pending = 0;
            source->count = 0;
        }
        else
        {
            copy->count = 0;
            source->pending = 0;
        }

        const u16 id = copy->id;
        u16 before = kHeadId;
        ItemLink* link = &this->itemObjects;

        while (*link != nullptr && ((*link)->id < id || before >= id))
        {
            before = (*link)->id;
            link = &(*link)->next;
        }

        copy->next = std::move(*link);
        *link = std::move(copy);
    }
}

// swd32.exe 0x00448230: the page's list made for its category: the objects left without counts dropped from the
// inventory, the empty entry for an empty list, the view at its start
void Menu::ItemsFill()
{
    Game& owner = this->game;
    this->ItemsTake(this->itemsPage.category);
    ItemLink* link = &owner.party.inventory;

    while (*link != nullptr)
    {
        if ((*link)->count != 0 || (*link)->pending != 0)
        {
            link = &(*link)->next;
        }
        else
        {
            *link = std::move((*link)->next);
        }
    }

    if (this->itemObjects == nullptr)
    {
        this->itemObjects = NewEmptyItem();
    }

    this->itemList.count = CountItems(this->itemObjects);
    this->itemList.top = 0;
    this->itemList.row = 0;
    this->itemList.first = this->itemObjects.get();
    this->ItemsCountShown();
}

// swd32.exe 0x004482E0: the page's list back in the inventory (counts and pending counts added to its entries)
void Menu::ItemsReturn()
{
    Game& owner = this->game;

    while (this->itemObjects != nullptr)
    {
        std::unique_ptr<ItemNode> node = std::move(this->itemObjects);
        this->itemObjects = std::move(node->next);

        if (node->id == ItemNode::Empty)
        {
            continue;
        }

        if (node->count > 0)
        {
            AddItem(owner.party.inventory, node->id, node->count, ItemCountMode::Count, owner.database);
        }

        if (node->pending > 0)
        {
            AddItem(owner.party.inventory, node->id, node->pending, ItemCountMode::PendingOnly, owner.database);
        }
    }
}

// swd32.exe 0x00448360: the chosen follower (role 0x47 on) sent away (its record moved to map 1000, its roles on the
// map removed; false) or called to the controlled role's tile (its action from its place in the list, the controlled
// role's direction; its record on this map, placed on the map, replacing its role there; true)
bool Menu::ItemsCallFollower()
{
    Game& owner = this->game;
    GameState& game = owner.state;
    const InventoryPage& page = this->itemsPage;
    Role role;
    ResetPlayback(role.act);
    owner.LoadRoleRecord(role, static_cast<u16>(page.followerRow + kFirstFollower));
    role.act.id = kFollowerAct;
    const Role& player = game.roles[static_cast<std::size_t>(game.playerRole)];

    if (role.act.action != 0)
    {
        role.act.action = 0;
        role.act.direction = 0;
        role.x = static_cast<i32>(static_cast<u32>(player.x) & ~kTileMask);
        role.flags |= Role::Visible;
        role.y = static_cast<i32>(static_cast<u32>(player.y) & ~kTileMask);
        owner.UpdateRoleRecord(
            role.id,
            static_cast<i16>(role.act.id),
            static_cast<i16>(role.act.action),
            static_cast<i16>(role.act.direction),
            0,
            0,
            Game::KeepValue,
            Game::KeepValue,
            0,
            Game::KeepValue,
            kGoneMap
        );

        for (u32 index = 1; index < game.roleCount; ++index)
        {
            Role& placed = game.roles[index];

            if (placed.id == role.id)
            {
                placed.flags &= kFollowerFlagsKept;
                owner.field.ClearRoleCells(placed, owner.partyLeaderId);
                placed.flags |= Role::Removed;
                owner.field.RemoveRole(placed.id, static_cast<int>(placed.flags & Role::LayerMask), 0, 1);
            }
        }

        return false;
    }

    role.act.action = kFollowerActionStep * page.followerRow + kFollowerActionStep;
    role.act.direction = player.act.direction & kDirectionMask;
    role.x = static_cast<i32>(static_cast<u32>(player.x) & ~kTileMask);
    role.flags |= Role::Visible;
    role.y = static_cast<i32>(static_cast<u32>(player.y) & ~kTileMask);
    owner.UpdateRoleRecord(
        role.id,
        static_cast<i16>(role.act.id),
        static_cast<i16>(role.act.action),
        static_cast<i16>(role.act.direction),
        0,
        0,
        Game::KeepValue,
        Game::KeepValue,
        0,
        Game::KeepValue,
        static_cast<i16>(owner.currentMap)
    );
    owner.InitRole(role);

    for (u32 index = 1; index < game.roleCount; ++index)
    {
        if (game.roles[index].id == role.id)
        {
            game.roles[index] = role;
            owner.field.InsertRole(game.roles[index], static_cast<int>(game.roles[index].flags & Role::LayerMask));
            return true;
        }
    }

    if (game.roleCount < GameState::Roles)
    {
        game.roles[game.roleCount] = role;
        owner.field.InsertRole(
            game.roles[game.roleCount],
            static_cast<int>(game.roles[game.roleCount].flags & Role::LayerMask)
        );
        ++game.roleCount;
    }

    return true;
}

// swd32.exe 0x004485F0: the save crystal used: the inventory and the page's list kept (the list back in the
// inventory meanwhile), the save slots opened in the items page's mode
void Menu::ItemsSave()
{
    Game& owner = this->game;
    this->savedInventory = CopyItemList(owner.party.inventory);
    this->savedItems = CopyItemList(this->itemObjects);
    this->ItemsReturn();
    owner.menuMode = Menu::ItemSaveMode;
    owner.saveScreenPage = 1;
    owner.slotsLoad = 0;
    owner.storyState = Game::SaveSlotsState;
    owner.ResetSaveSlots();
}

}  // namespace swd3
