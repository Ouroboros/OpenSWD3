#include <algorithm>
#include <cstdlib>
#include <span>
#include <string>

#include "battle/battle_items.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu_list.h"
#include "menu/shop_constants.h"
#include "platform/display.h"
#include "save/save_slots.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// BattleState::shopStage.
constexpr i32 kShopChoice = 1;  // buy, sell or leave
constexpr i32 kShopList = 2;
constexpr i32 kShopTotal = 3;  // the total to confirm
constexpr i32 kShopEquip = 4;  // equip the equipment bought at once?
constexpr i32 kShopPoor = 0xA;

// BattleState::mode in the shop: bits 0..1 the choice (0 buy, 1 sell, 2 leave), bit 2 "放棄" on the total, bit 3 on
// the equip prompt.
constexpr i32 kChoiceMask = 3;
constexpr i32 kBuyChoice = 0;
constexpr i32 kSellChoice = 1;
constexpr i32 kLeaveChoice = 2;
constexpr i32 kTotalCancel = 4;
constexpr i32 kEquipCancel = 8;
constexpr i32 kAnswerBits = kTotalCancel | kEquipCancel;
constexpr i32 kAnswerSkip = 5;  // the equip prompt counts only without the sell and cancel bits

// BattleState::shopFlags.
constexpr u32 kListOpened = 2;
constexpr u32 kEquipmentBought = 4;
constexpr u32 kUnkShopFlag1 = 1;

constexpr i32 kPageRows = 0xD;
constexpr i16 kMaxCount = 0x63;
constexpr i16 kPressFrames = 2;
constexpr u16 kScrollUp = 3;  // BattleState::scrollArrows
constexpr u16 kScrollDown = 0x30;
constexpr u16 kScrollUpMask = 0xF;
constexpr u16 kScrollDownMask = 0xF0;
constexpr int kNibbleShift = 4;

constexpr u16 kMoveSound = 0xBF;
constexpr u16 kCountSound = 0xB9;
constexpr u16 kStageSound = 0xBB;
constexpr u16 kPoorSound = 0x8C;

constexpr i32 kFaceAct = 0x232A;  // the acts of the shop page
constexpr i32 kShopTitleAction = 0x16;
constexpr i32 kShopFrameAction = 0x32;
constexpr i32 kPlusAction = 0x1C;
constexpr i32 kMinusAction = 0x1D;
constexpr i32 kFaceAction = 5;
constexpr i32 kFaceChosenAction = 0xD;
constexpr i32 kFaceUnusableAction = 0x39;  // the first role cannot use the equipment

// The menu clicks (Menu::clicks).
constexpr u32 kLeftClicks = 3;
constexpr u32 kLeftClick = 1;
constexpr u32 kRightClick = 4;
constexpr u32 kAnyClick = 0xF;

constexpr i32 kShopRepeatDelay = 7;  // ShopKeys: held frames before a key repeats every second frame
constexpr u32 kKindTopBit = 0x8000;  // CompareEquipment clears it from the match
constexpr u32 kEquipmentKinds = 0x700;
constexpr i32 kPictureAction = 0x44;
constexpr i16 kFaceSlideMost = 0x10;
constexpr i16 kFaceSlideLeast = 8;
constexpr int kSellRate = 0x3C;  // percent of the price paid for what is sold
constexpr int kFullRate = 0x64;

// The colours of the shop's text.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kGainRed = 0x1F;
constexpr u32 kGainGreen = 0x1F;
constexpr u32 kGainBlue = 0x1F;
constexpr u32 kLossRed = 0x1A;
constexpr int kPanelStyle = 4;

// The texts.
constexpr std::u8string_view kGreeting = u8"客倌 您要買東西還是賣東西";  // swd32.exe 0x004A7490
constexpr std::u8string_view kChoices = u8"買   賣   離開";              // swd32.exe 0x004A7480
constexpr std::u8string_view kRun = u8"執行";                            // swd32.exe 0x004A7478
constexpr std::u8string_view kBuyTitle = u8"購買";                       // swd32.exe 0x004A7470
constexpr std::u8string_view kSellTitle = u8"賣出";                      // swd32.exe 0x004A7468
constexpr std::u8string_view kTotalFormat = u8"總價 %d 餘額 %d";         // swd32.exe 0x004A7454
constexpr std::u8string_view kSellTotal = u8"總共是 %d 銀幣 要賣嗎?";    // swd32.exe 0x004A7424
constexpr std::u8string_view kBuyTotal = u8"總共是 %d 銀幣 要買嗎?";     // swd32.exe 0x004A743C
constexpr std::u8string_view kTotalAnswers = u8"確定  放棄";             // swd32.exe 0x004A6FD0
constexpr std::u8string_view kEquipNow = u8"要馬上裝備嗎?";              // swd32.exe 0x004A7414
constexpr std::u8string_view kEquipAnswers = u8" 確定  放棄 ";           // swd32.exe 0x004A7404
constexpr std::u8string_view kTooPoor = u8"錢帶多一點再來吧";            // swd32.exe 0x004A73F0
constexpr std::u8string_view kAttackLabel = u8"攻";                      // swd32.exe 0x004A74C4
constexpr std::u8string_view kDefenseLabel = u8"防";                     // swd32.exe 0x004A74B0
constexpr std::u8string_view kAgilityLabel = u8"敏";                     // swd32.exe 0x004A74AC
constexpr std::string_view kNoChange = "----";
constexpr std::string_view kChangeFormat = "%c%d";
constexpr std::string_view kNameFormat = "%-14s";
constexpr std::string_view kPriceFormat = "%6d";
constexpr std::string_view kCountFormat = "%3d";

// The layout.
constexpr int kFrameInset = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr int kHighlightWidth = 0x30;
constexpr int kHighlightHeight = 0x18;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kFaceX = 0xC;
constexpr std::array<int, Party::Roles> kFaceTops = {0x48, 0xC0, 0x138, 0x1B0};
constexpr int kHelpX = 0x96;
constexpr int kHelpY = 0x1C0;
constexpr int kHelpWidth = 0x1E4;
constexpr int kHelpHeight = 0x1C;
constexpr int kHelpRight = 0x272;
constexpr int kHelpBottom = 0x1D8;
constexpr int kHelpTextClipTop = 0x1C2;
constexpr int kHelpTextClipRight = 0x1E6;
constexpr int kHelpTextClipBottom = 0x1DE;
constexpr int kHelpTextY = 0x1C5;
constexpr int kGoldBoxX = 0x1A4;
constexpr int kGoldBoxY = 0xA;
constexpr int kGoldBoxWidth = 0xC8;
constexpr int kGoldBoxHeight = 0x1C;
constexpr int kGoldBoxRight = 0x268;
constexpr int kGoldBoxBottom = 0x22;
constexpr int kGoldRight = 0x264;
constexpr int kGoldY = 0x12;
constexpr int kChoiceBoxX = 0xB0;
constexpr int kChoiceBoxY = 0x128;
constexpr int kChoiceBoxWidth = 0x130;
constexpr int kChoiceBoxHeight = 0x36;
constexpr int kChoicesX = 0x118;
constexpr int kChoicesY = 0x145;
constexpr int kChoiceWidthPixels = 0x37;
constexpr int kChoiceHighlightX = 0x114;
constexpr int kLeaveHighlightX = 0x182;
constexpr int kRunX = 0x1BC;
constexpr int kRunY = 0x197;
constexpr int kRunWidth = 0x28;
constexpr int kRunHeight = 0x10;
constexpr int kRunMode = 2;
constexpr u32 kRunLeft = 0x1B8;
constexpr u32 kRunRight = 0x1E0;
constexpr u32 kRunTop = 0x193;
constexpr u32 kRunBottom = 0x1AF;
constexpr int kTitleBoxX = 0x96;
constexpr int kTitleBoxY = 0xA;
constexpr int kTitleBoxWidth = 0x54;
constexpr int kTitleBoxHeight = 0x20;
constexpr int kTitleBoxRight = 0xE2;
constexpr int kTitleBoxBottom = 0x22;
constexpr int kTitleTextX = 0xA6;
constexpr int kListBoxY = 0x32;
constexpr int kListBoxWidth = 0x158;
constexpr int kListBoxHeight = 0x159;
constexpr int kListBoxRight = 0x1E6;
constexpr int kListBoxBottom = 0x183;
constexpr int kTitleClipRight = 0x180;
constexpr int kTitleClipLeft = 0x1A0;
constexpr int kTitleLeftX = 0x92;
constexpr int kTitleRightX = 0xB4;
constexpr int kTitleActY = 0x28;
constexpr int kFirstRowY = 0x40;
constexpr int kRowsEndY = 0x185;
constexpr int kNameX = 0x9E;
constexpr int kNameRaise = 4;
constexpr int kPriceX = 0x142;
constexpr int kChosenX = 0x1A0;
constexpr int kCarriedX = 0x1C2;
constexpr int kPlusX = 0x194;
constexpr int kMinusX = 0x184;
constexpr int kRowRaise = 6;
constexpr int kRowWidth = 0x154;
constexpr int kPictureX = 0x1FE;
constexpr int kPictureY = 0x5A;
constexpr int kTotalBoxY = 0x193;
constexpr int kTotalBoxWidth = 0x11C;
constexpr int kTotalBoxHeight = 0x1C;
constexpr int kTotalBoxRight = 0x1AA;
constexpr int kTotalBoxBottom = 0x1A7;
constexpr int kTotalTextX = 0x9C;
constexpr int kBarX = 0x1EE;
constexpr int kBarY = 0x42;
constexpr int kBarHeight = 0x134;
constexpr int kCompareTop = 0x35;
constexpr int kCompareStep = 0x78;
constexpr int kCompareLine = 0x14;
constexpr int kCompareLabelX = 0x44;
constexpr int kCompareValueX = 0x58;
constexpr int kPromptX = 0xEC;
constexpr int kPromptY = 0x182;
constexpr int kPromptHeight = 0x44;
constexpr int kPromptBottom = 0x1C2;
constexpr int kPromptShade = -16;
constexpr int kPromptCharWidth = 0xB;
constexpr int kPromptPad = 0x10;
constexpr int kPromptBoxPad = 0x18;
constexpr int kPromptTextX = 0xFA;
constexpr int kTotalTextY = 0x190;
constexpr int kTotalAnswersX = 0x15E;
constexpr int kAnswersTextY = 0x1A9;
constexpr int kAnswerStep = 0x42;
constexpr int kTotalHighlightX = 0x154;
constexpr int kEquipBoxWidth = 0xAE;
constexpr int kEquipBoxRight = 0x196;
constexpr int kPoorBoxWidth = 0xC6;
constexpr int kPoorBoxHeight = 0x26;
constexpr int kPoorBoxRight = 0x1AE;
constexpr int kPoorBoxBottom = 0x1A4;
constexpr int kPoorTextY = 0x18A;

// The mouse areas.
constexpr u32 kChoiceLeft = 0x118;
constexpr u32 kChoiceRight = 0x1B2;
constexpr u32 kChoiceTop = 0x144;
constexpr u32 kChoiceBottom = 0x15C;
constexpr u32 kChoiceWidth = 0x37;
constexpr u32 kTotalAnswersLeft = 0x153;
constexpr u32 kTotalAnswersRight = 0x1E2;
constexpr u32 kEquipAnswersLeft = 0xFA;
constexpr u32 kEquipAnswersRight = 0x17E;
constexpr u32 kAnswersTop = 0x1A8;
constexpr u32 kAnswersBottom = 0x1C0;
constexpr u32 kAnswerWidth = 0x42;
constexpr u32 kRowsLeft = 0x9E;
constexpr u32 kRowsRight = 0x1F4;
constexpr u32 kRowsTop = 0x3C;
constexpr u32 kRowsBottom = 0x181;
constexpr u32 kRowHeight = 0x19;
constexpr u32 kMinusLeft = 0x178;
constexpr u32 kMinusRight = 0x191;
constexpr u32 kPlusLeft = 0x193;
constexpr u32 kPlusRight = 0x1AC;
constexpr u32 kArrowsTop = 0x3D;
constexpr u32 kArrowsBottom = 0x50;
constexpr u32 kBarLeft = 0x1ED;
constexpr u32 kBarRight = 0x1FF;
constexpr i32 kBarUpTop = 0x31;
constexpr i32 kBarUpBottom = 0x43;
constexpr i32 kBarDownTop = 0x177;
constexpr i32 kBarDownBottom = 0x187;

}  // namespace

// swd32.exe 0x0044D920: the shop opened: the map drawn without the pointer, darkened and grey under it (two copies),
// the list empty with its head as the first item shown, the choice page
void Game::OpenShop()
{
    BattleState& shop = this->battleState;
    this->state.SetGameFlag(GameState::NoPointer);
    this->BeginMapCanvas();
    this->fieldView.Paint();
    this->EndMapCanvas();
    this->state.ClearGameFlag(GameState::NoPointer);
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);
    const std::size_t pixels = static_cast<std::size_t>(Display::Width) * Display::Height;
    shop.shopScreen.assign(this->drawSurface, this->drawSurface + pixels);
    this->painter.TintPixels(shop.shopScreen, kShopTintRed, kShopTintGreen, kShopTintBlue);
    this->painter.GrayPixels(shop.shopScreen);

    shop.shopScreenSaved = shop.shopScreen;
    shop.itemsHead = ItemNode{};
    shop.listB = &shop.itemsHead;
    shop.mode = 0;
    shop.listBCount = 0;
    shop.listRow = 0;
    shop.listTop = 0;
    shop.itemCount = 0;
    shop.lessPressed = 0;
    shop.morePressed = 0;
    shop.shopStage = kShopChoice;
    this->ResetShopActs();
    shop.faceSlides.fill(0);
    shop.shopFlags = 0;
}

// swd32.exe 0x0044DAA0: the acts of the shop page at their first actions: the four faces (acts 1, 2, 8, 0x11), the
// title, the frame and the plus and minus arrows
void Game::ResetShopActs()
{
    Menu& menu = this->menu;

    for (ActState& face : menu.items)
    {
        ResetPlayback(face);
    }

    ResetPlayback(menu.gauge);
    ResetPlayback(menu.unk4FBA18);
    ResetPlayback(menu.unk4FB8D8);
    ResetPlayback(menu.unk4FC840);
    ResetPlayback(this->battleState.shopTitle);

    for (std::size_t role = 0; role < menu.items.size(); ++role)
    {
        menu.items[role].action = kFaceAction;
        menu.items[role].id = SaveSlots::FrameActs[role];
        menu.items[role].direction = 0;
    }

    auto set = [](ActState& act, i32 action)
    {
        act.id = kFaceAct;
        act.action = action;
        act.direction = 0;
    };

    set(this->battleState.shopTitle, kShopTitleAction);
    set(menu.unk4FC840, kShopFrameAction);
    set(menu.unk4FBA18, kPlusAction);
    set(menu.unk4FB8D8, kMinusAction);
}

// swd32.exe 0x0044DA40: the shop closed: the goods of talk command ItemMenu, the screen copies and the list freed
void Game::CloseShop()
{
    BattleState& shop = this->battleState;
    this->unk4B7928.clear();
    shop.shopScreen.clear();
    shop.shopScreenSaved.clear();
    FreeItemList(shop.itemsHead.next);
    shop.itemsHead = ItemNode{};
}

// swd32.exe 0x0044E9D0: Escape: from the choice the shop closes, from the list back to the choice (its screen and the
// list cleared), from the total to the list, from the equip prompt to the list
void Game::ShopBack()
{
    BattleState& shop = this->battleState;

    switch (shop.shopStage)
    {
        case kShopChoice:
            --shop.shopStage;
            this->CloseShop();
            this->menuState = Game::NoMenu;
            break;

        case kShopList:
            --shop.shopStage;
            FreeBattleItems(shop);
            shop.shopScreen = shop.shopScreenSaved;
            shop.shopFlags &= ~kListOpened;
            FreeItemList(shop.itemsHead.next);
            break;

        case kShopTotal:
            --shop.shopStage;
            break;

        case kShopEquip:
            shop.shopStage = kShopList;
            break;

        default:
            break;
    }
}

// The row chosen described and compared after a move (inline in the original, 0x0044DF00 and its neighbours).
void Game::ShopRowChanged(bool sound, u16 arrows)
{
    BattleState& shop = this->battleState;
    CountListB(shop);
    const ItemNode* chosen = ItemAfter(shop.listRow + shop.listTop, shop.itemsHead.next);
    this->menu.description =
        chosen != nullptr ? ObjectDescription(this->databases.mapsData, chosen->id) : std::string();

    if (sound)
    {
        this->audio.PlaySound(kMoveSound, this->config.soundVolume);
    }

    shop.scrollArrows = static_cast<u16>(shop.scrollArrows | arrows);
    this->CompareEquipment(ItemAt(shop.itemsHead.next, static_cast<u16>(shop.listTop + shop.listRow)));
}

// swd32.exe 0x0044DF00: down: on the choice the next choice (up to "離開"); on the list the next row, scrolling at the
// last row shown
void Game::ShopDown()
{
    BattleState& shop = this->battleState;

    if (shop.shopStage == kShopChoice)
    {
        shop.mode = (shop.mode & ~kChoiceMask) | std::min((shop.mode & kChoiceMask) + 1, kLeaveChoice);
        return;
    }

    if (shop.shopStage != kShopList)
    {
        return;
    }

    if (++shop.listRow >= shop.listBCount)
    {
        shop.listRow = 0;

        if (shop.listBCount >= 1)
        {
            shop.listRow = shop.listBCount - 1;
        }

        if (shop.listTop + shop.listBCount < shop.itemCount)
        {
            SkipItems(++shop.listTop, shop.itemsHead.next, shop.listB);
        }
    }

    this->ShopRowChanged(true, kScrollDown);
}

// swd32.exe 0x0044DFF0: up: on the choice the choice before; on the list the row before, scrolling at the first
void Game::ShopUp()
{
    BattleState& shop = this->battleState;

    if (shop.shopStage == kShopChoice)
    {
        shop.mode = (shop.mode & ~kChoiceMask) | std::max((shop.mode & kChoiceMask) - 1, 0);
        return;
    }

    if (shop.shopStage != kShopList)
    {
        return;
    }

    if (--shop.listRow < 0)
    {
        shop.listRow = 0;

        if (shop.listTop > 0)
        {
            SkipItems(--shop.listTop, shop.itemsHead.next, shop.listB);
        }
    }

    this->ShopRowChanged(true, kScrollUp);
}

// swd32.exe 0x0044E0C0: Page Down on the list: to the last row shown, from there a page of 13 further (unless that
// passes the end)
void Game::ShopPageDown()
{
    BattleState& shop = this->battleState;

    if (shop.shopStage != kShopList)
    {
        return;
    }

    i32 row = shop.listBCount - 1;

    if (shop.listRow != row)
    {
        shop.listRow = 0;

        if (shop.listBCount >= 1)
        {
            shop.listRow = shop.listBCount - 1;
        }

        return;
    }

    shop.listTop += kPageRows;

    if (shop.listTop < shop.itemCount)
    {
        SkipItems(shop.listTop, shop.itemsHead.next, shop.listB);
        CountListB(shop);
        row = shop.listRow;

        if (row > shop.listBCount - 1)
        {
            row = shop.listBCount - 1;
            shop.listRow = row;
        }
    }
    else
    {
        shop.listTop -= kPageRows;
        shop.listRow = row;
    }

    this->ShopRowChanged(true, kScrollDown);
}

// swd32.exe 0x0044E1B0: Page Up on the list: to the first row shown, from there a page of 13 back (to the start);
// it plays no sound and marks the down arrow
void Game::ShopPageUp()
{
    BattleState& shop = this->battleState;

    if (shop.shopStage != kShopList)
    {
        return;
    }

    if (shop.listRow != 0)
    {
        shop.listRow = 0;
        return;
    }

    shop.listTop -= kPageRows;

    if (shop.listTop >= 0)
    {
        SkipItems(shop.listTop, shop.itemsHead.next, shop.listB);
    }
    else
    {
        shop.listRow = 0;
        shop.listTop = 0;
        SkipItems(0, shop.itemsHead.next, shop.listB);
    }

    this->ShopRowChanged(false, kScrollDown);
}

// The item of the row chosen (inline in the original, 0x0043B9A0 into a local): nullptr past the end of the list.
ItemNode* Game::ShopChosenItem()
{
    BattleState& shop = this->battleState;
    ItemNode* item = nullptr;
    SkipItems(shop.listTop + shop.listRow, shop.itemsHead.next, item);
    return item;
}

// swd32.exe 0x0044E260: left or minus: the choice before; on the list one less of the row chosen (not below 0);
// "確定" on the total and on the equip prompt
void Game::ShopLess()
{
    BattleState& shop = this->battleState;

    switch (shop.shopStage)
    {
        case kShopChoice:
            shop.mode = (shop.mode & ~kChoiceMask) | std::max((shop.mode & kChoiceMask) - 1, 0);
            break;

        case kShopList:
            if (ItemNode* item = this->ShopChosenItem(); item != nullptr)
            {
                --item->unk06;

                if (item->unk06 > 0)
                {
                    this->audio.PlaySound(kCountSound, this->config.soundVolume);
                }
                else
                {
                    item->unk06 = 0;
                }
            }

            shop.lessPressed = kPressFrames;
            break;

        case kShopTotal:
            shop.mode &= ~kTotalCancel;
            break;

        case kShopEquip:
            shop.mode &= ~kEquipCancel;
            break;

        default:
            break;
    }
}

// swd32.exe 0x0044E330: right or plus: the next choice; on the list one more of the row chosen: selling up to the count
// carried, buying while the gold lasts (then the too little money page) up to 99 carried in all; "放棄" on the total
// and on the equip prompt
void Game::ShopMore()
{
    BattleState& shop = this->battleState;

    switch (shop.shopStage)
    {
        case kShopChoice:
            shop.mode = (shop.mode & ~kChoiceMask) | std::min((shop.mode & kChoiceMask) + 1, kLeaveChoice);
            break;

        case kShopList:
        {
            ItemNode* item = this->ShopChosenItem();

            if (item == nullptr)
            {
                break;
            }

            ++item->unk06;

            if ((shop.mode & kSellChoice) != 0)
            {
                const int carried = static_cast<u16>(item->pending) + static_cast<u16>(item->count);

                if (static_cast<u16>(item->unk06) <= carried)
                {
                    this->audio.PlaySound(kCountSound, this->config.soundVolume);
                }
                else
                {
                    item->unk06 = static_cast<i16>(carried);
                }

                shop.morePressed = kPressFrames;
                break;
            }

            if (static_cast<u32>(TotalItemValue(shop)) > static_cast<u32>(this->party.variables[0]))
            {
                --item->unk06;
                shop.shopStage = kShopPoor;
                this->audio.PlaySound(kPoorSound, this->config.soundVolume);
                break;
            }

            const int have = CountItemTotal(shop, this->party, this->state, item->id);

            if (static_cast<u16>(item->unk06) >= kMaxCount - have)
            {
                item->unk06 = static_cast<i16>(kMaxCount - have);
            }
            else
            {
                this->audio.PlaySound(kCountSound, this->config.soundVolume);
            }

            shop.morePressed = kPressFrames;
            break;
        }

        case kShopTotal:
            shop.mode |= kTotalCancel;
            break;

        case kShopEquip:
            shop.mode |= kEquipCancel;
            break;

        default:
            break;
    }
}

// swd32.exe 0x0044E4A0: Space, Enter or "執行": the choice opens the list (to buy: the goods of talk command ItemMenu,
// 99 of each; to sell: the inventory's objects with a price) or leaves; on the list with a total the total page; there
// "確定" buys (equipment asks to be equipped) or sells (an object sold out leaves the list) and "放棄" clears the
// counts; on the equip prompt "確定" opens the main menu on its items page
void Game::ShopConfirm()
{
    BattleState& shop = this->battleState;

    switch (shop.shopStage)
    {
        case kShopChoice:
            this->ShopOpenList();
            break;

        case kShopList:
            if (TotalItemValue(shop) > 0)
            {
                ++shop.shopStage;
                shop.mode &= ~kTotalCancel;
                this->audio.PlaySound(kStageSound, this->config.soundVolume);
            }

            break;

        case kShopTotal:
            this->ShopSettle();
            break;

        case kShopEquip:
            shop.shopStage = kShopChoice;

            if ((shop.mode & kAnswerSkip) == 0 && (shop.mode & kEquipCancel) == 0)
            {
                this->menuState = Game::MenuEntering | Game::MenuOnEquipment | Game::MainMenu;
                this->audio.PlaySound(kCountSound, this->config.soundVolume);
            }

            break;

        case kShopPoor:
            shop.shopStage = kShopList;
            break;

        default:
            break;
    }
}

// The list opened from the choice (inline in 0x0044E4A0, stage 1).
void Game::ShopOpenList()
{
    BattleState& shop = this->battleState;
    ++shop.shopStage;
    FreeItemList(shop.itemsHead.next);
    const i32 choice = shop.mode & kChoiceMask;

    if (choice == kSellChoice)
    {
        ItemLink copy = CopyItemList(this->party.inventory);
        SortBattleItems(shop, copy);
        FreeItemList(copy);

        for (ItemNode* item = shop.itemsHead.next.get(); item != nullptr; item = item->next.get())
        {
            item->unk06 = 0;
        }

        shop.shopFlags &= ~kUnkShopFlag1;

        if (shop.itemsHead.next == nullptr)
        {
            --shop.shopStage;
            return;
        }
    }
    else if (choice == kLeaveChoice)
    {
        shop.shopStage = kShopChoice;
        this->ShopBack();
        return;
    }
    else if (choice == kBuyChoice)
    {
        shop.mode = 0;
        shop.listBCount = 0;
        shop.listRow = 0;
        shop.listTop = 0;
        shop.itemCount = 0;
        ItemLink goods;
        const std::vector<u16>& ids = this->unk4B7928;

        for (std::size_t at = 0; at < ids.size() && ids[at] != 0; ++at)
        {
            if (AddItem(goods, ids[at], kMaxCount, ItemCountMode::Count, this->database) == nullptr)
            {
                // swd32.exe 0x0044E5CD: as the original, the id after the one that failed
                const u16 next = at + 1 < ids.size() ? ids[at + 1] : u16{0};
                DebugLog(u8"商店存貨設定 沒有定義物品號碼(%d)", next);  // 0x004A73E0 + 0x004A6BA0
            }
        }

        SortBattleItems(shop, goods);

        if (goods != nullptr)
        {
            DebugLog(u8"商店存貨設定 物品入庫發生錯誤");  // 0x004A73E0 + 0x004A73CC
        }

        const ItemNode* chosen = ItemAfter(shop.listTop + shop.listRow, shop.itemsHead.next);
        this->menu.description =
            chosen != nullptr ? ObjectDescription(this->databases.mapsData, chosen->id) : std::string();
    }

    shop.listTop = 0;
    shop.listRow = 0;
    shop.itemCount = CountItems(shop.itemsHead.next);
    shop.listB = shop.itemsHead.next.get();
    CountListB(shop);

    if ((shop.shopFlags & kListOpened) == 0)
    {
        this->audio.PlaySound(kStageSound, this->config.soundVolume);
        shop.shopFlags |= kListOpened;
    }

    this->CompareEquipment(ItemAt(shop.itemsHead.next, static_cast<u16>(shop.listTop + shop.listRow)));
}

// The total settled (inline in 0x0044E4A0, stage 3).
void Game::ShopSettle()
{
    BattleState& shop = this->battleState;
    ++shop.shopStage;

    if ((shop.mode & kTotalCancel) != 0)
    {
        shop.shopStage = kShopList;

        for (ItemNode* item = shop.itemsHead.next.get(); item != nullptr; item = item->next.get())
        {
            item->unk06 = 0;
        }

        this->ShopAnswered();
        return;
    }

    if ((shop.mode & kChoiceMask) != kSellChoice)
    {
        this->party.variables[0] -= TotalItemValue(shop);
        shop.shopFlags &= ~kEquipmentBought;

        for (ItemNode* item = shop.itemsHead.next.get(); item != nullptr; item = item->next.get())
        {
            if (item->unk06 != 0)
            {
                for (const u32 kind : Party::SlotKinds)
                {
                    if ((item->record.data.kind & kind) == kind)
                    {
                        shop.shopFlags |= kEquipmentBought;
                    }
                }

                AddItem(this->party.inventory, item->id, item->unk06, ItemCountMode::Pending, this->database);
            }

            item->unk06 = 0;
        }

        if ((shop.shopFlags & kEquipmentBought) == 0)
        {
            shop.shopStage = kShopChoice;
        }

        this->ShopAnswered();
        return;
    }

    shop.shopStage = kShopList;
    this->party.variables[0] += TotalItemValue(shop);
    ItemLink* link = &shop.itemsHead.next;

    while (*link != nullptr)
    {
        ItemNode& item = **link;

        if (item.count + item.pending > 0 && item.unk06 != 0)
        {
            AddItem(
                this->party.inventory,
                item.id,
                static_cast<i16>(-item.unk06),
                ItemCountMode::Pending,
                this->database
            );

            if (FindItemOfKind(this->party.inventory, static_cast<i16>(item.id)) == nullptr)
            {
                *link = std::move(item.next);
                continue;
            }

            item.unk06 = 0;
        }

        link = &(*link)->next;
    }

    if (shop.itemsHead.next == nullptr)
    {
        return;
    }

    this->audio.PlaySound(kCountSound, this->config.soundVolume);
    shop.listTop = 0;
    shop.listRow = 0;
    shop.listB = shop.itemsHead.next.get();
    shop.itemCount = CountItems(shop.itemsHead.next);
    shop.listRow = 0;

    if (shop.listBCount <= 0)
    {
        shop.listRow = 0;

        if (shop.listBCount >= 1)
        {
            shop.listRow = shop.listBCount - 1;
        }

        if (shop.listTop + shop.listBCount < shop.itemCount)
        {
            SkipItems(++shop.listTop, shop.itemsHead.next, shop.listB);
        }
    }

    CountListB(shop);
    const ItemNode* chosen = ItemAfter(shop.listTop + shop.listRow, shop.itemsHead.next);
    this->menu.description =
        chosen != nullptr ? ObjectDescription(this->databases.mapsData, chosen->id) : std::string();
    shop.shopStage = kShopChoice;
    this->ShopAnswered();
}

// The answers of the total and the equip prompt cleared (inline in 0x0044E4A0).
void Game::ShopAnswered()
{
    this->battleState.mode &= ~kAnswerBits;
    this->audio.PlaySound(kStageSound, this->config.soundVolume);
}

// swd32.exe 0x0044F920: the keys of the shop: Escape or Right Ctrl back; the arrows and the page keys (held: every
// second frame after seven); Space or Enter confirm. Left and Right read the held frames of Page Up for the repeat.
void Game::ShopKeys()
{
    auto pressedOnce = [this](Button button)
    {
        const ButtonState& state = this->controls.At(button);
        return state.press == 1 && state.heldFrames == 1;
    };

    auto repeating = [this](Button button, Button parity)
    {
        const ButtonState& state = this->controls.At(button);
        const i32 held = this->controls.At(parity).heldFrames;
        return state.press != 0 && (state.heldFrames == 1 || ((held & 1) == 0 && state.heldFrames > kShopRepeatDelay));
    };

    if (pressedOnce(Button::Escape) || pressedOnce(Button::RightControl))
    {
        this->ShopBack();
    }
    else if (repeating(Button::Down, Button::Down))
    {
        this->ShopDown();
    }
    else if (repeating(Button::Up, Button::Up))
    {
        this->ShopUp();
    }
    else if (repeating(Button::PageDown, Button::PageDown))
    {
        this->ShopPageDown();
    }
    else if (repeating(Button::PageUp, Button::PageUp))
    {
        this->ShopPageUp();
    }
    else if (repeating(Button::Left, Button::PageUp))
    {
        this->ShopLess();
    }
    else if (repeating(Button::Right, Button::PageUp))
    {
        this->ShopMore();
    }
    else
    {
        const ButtonState& space = this->controls.At(Button::Space);
        const ButtonState& enter = this->controls.At(Button::Enter);

        if ((space.press != 0 && space.heldFrames == 1) || (enter.press != 0 && enter.heldFrames == 1))
        {
            this->ShopConfirm();
        }
    }
}

// swd32.exe 0x0044DBC0: the mouse in the shop: on the choice a choice under the pointer (a click confirms); on the list
// a click on a row chooses it, on its minus or plus arrow changes its count, on "執行" confirms; a held left button on
// the scroll bar's arrows or beside its thumb scrolls; on the prompts a click on an answer picks and confirms it; a
// right click goes back; any click leaves the too little money page
void Game::ShopMouse()
{
    BattleState& shop = this->battleState;
    this->menu.ReadMouse();
    const auto x = static_cast<u32>(this->state.pointerX);
    const auto y = static_cast<u32>(this->state.pointerY);
    const u32 clicks = this->menu.clicks;

    auto inside = [](u32 value, u32 low, u32 high)
    {
        return value < high && value > low;
    };

    auto back = [this, clicks]
    {
        if ((clicks & kRightClick) != 0)
        {
            this->ShopBack();
        }
    };

    auto answers = [this, clicks, x, y, &inside, &back](u32 left, u32 right)
    {
        if (!inside(x, left, right) || !inside(y, kAnswersTop, kAnswersBottom))
        {
            back();
            return;
        }

        if ((x - left) / kAnswerWidth != 0)
        {
            this->ShopMore();
        }
        else
        {
            this->ShopLess();
        }

        if ((clicks & kLeftClicks) != 0)
        {
            this->ShopConfirm();
        }
    };

    switch (shop.shopStage)
    {
        case kShopChoice:
            if (inside(x, kChoiceLeft, kChoiceRight) && inside(y, kChoiceTop, kChoiceBottom))
            {
                shop.mode = (shop.mode & ~kChoiceMask) | static_cast<i32>((x - kChoiceLeft) / kChoiceWidth);

                if ((clicks & kLeftClick) != 0)
                {
                    this->ShopConfirm();
                }
            }

            back();
            return;

        case kShopList:
            this->ShopListMouse();
            return;

        case kShopTotal:
            answers(kTotalAnswersLeft, kTotalAnswersRight);
            return;

        case kShopEquip:
            answers(kEquipAnswersLeft, kEquipAnswersRight);
            return;

        case kShopPoor:
            if ((clicks & kAnyClick) != 0)
            {
                shop.shopStage = kShopList;
            }

            return;

        default:
            return;
    }
}

// The mouse on the list (inline in 0x0044DBC0, stage 2).
void Game::ShopListMouse()
{
    BattleState& shop = this->battleState;
    const u32 clicks = this->menu.clicks;

    auto inside = [](u32 value, u32 low, u32 high)
    {
        return value < high && value > low;
    };

    if ((clicks & kLeftClicks) == 0)
    {
        if ((clicks & kRightClick) != 0)
        {
            this->ShopBack();
            return;
        }

        const auto x = static_cast<u32>(this->state.pointerX);

        if (this->controls.At(Button::MouseLeft).press == 0 || shop.itemCount <= kPageRows ||
            !inside(x, kBarLeft, kBarRight))
        {
            return;
        }

        const i32 y = this->state.pointerY;

        if (y < kBarUpBottom && y > kBarUpTop)
        {
            this->ShopUp();
        }

        if (this->state.pointerY<kBarDownBottom&& this->state.pointerY> kBarDownTop)
        {
            this->ShopDown();
        }

        const MenuScrollBar& bar = this->menu.scrollBar;

        if (this->state.pointerY<bar.thumbTop&& this->state.pointerY> bar.top)
        {
            this->ShopPageUp();
        }

        if (this->state.pointerY<bar.bottom&& this->state.pointerY> bar.thumbBottom)
        {
            this->ShopPageDown();
        }

        return;
    }

    u32 x = static_cast<u32>(this->state.pointerX);
    u32 y = static_cast<u32>(this->state.pointerY);

    if (inside(x, kRowsLeft, kRowsRight) && inside(y, kRowsTop, kRowsBottom))
    {
        const u32 row = (y - kRowsTop) / kRowHeight;

        if (row != static_cast<u32>(shop.listRow))
        {
            this->audio.PlaySound(kMoveSound, this->config.soundVolume);
            shop.listRow = static_cast<i32>(row) - 1;
            this->ShopDown();
        }
        else
        {
            const u32 rowTop = kRowHeight * row;

            if (inside(x, kMinusLeft, kMinusRight) && inside(y, rowTop + kArrowsTop, rowTop + kArrowsBottom))
            {
                this->ShopLess();
                x = static_cast<u32>(this->state.pointerX);
                y = static_cast<u32>(this->state.pointerY);
            }

            if ((this->menu.clicks & kLeftClicks) != 0 && inside(x, kPlusLeft, kPlusRight) &&
                inside(y, rowTop + kArrowsTop, rowTop + kArrowsBottom))
            {
                this->ShopMore();
            }
        }

        x = static_cast<u32>(this->state.pointerX);
        y = static_cast<u32>(this->state.pointerY);
    }

    if (inside(x, kRunLeft, kRunRight) && inside(y, kRunTop, kRunBottom))
    {
        this->ShopConfirm();
    }
}

// swd32.exe 0x0044FAF0: for each party role the attack, defence and agility `item` would give against its equipment:
// the role's record with all sixteen slots worn against one with the first eleven worn and `item` in the slots its
// kind fits (so the last five slots count against it); `usable` whether the role can use it
void Game::CompareEquipment(const ItemNode* item)
{
    BattleState& shop = this->battleState;

    for (int role = 0; role < Party::Roles; ++role)
    {
        if (!this->state.GameFlag(Party::FirstRoleFlag + role))
        {
            continue;
        }

        ShopComparison& comparison = shop.comparisons[static_cast<std::size_t>(role)];
        const auto index = static_cast<std::size_t>(role);
        comparison.usable = 1;

        if (item == nullptr || (item->record.data.usableBy & (ObjectData::FirstRoleBit >> role)) == 0)
        {
            comparison.usable = 0;
        }

        const std::span<const std::unique_ptr<ItemNode>> slots(
            this->party.equipment.data() + index * Party::SlotsPerRole,
            Party::SlotsPerRole
        );
        RoleRecord worn = this->party.roles[index];

        for (const std::unique_ptr<ItemNode>& slot : slots)
        {
            if (slot != nullptr)
            {
                ApplyObject(worn, slot->record.data, this->database);
            }
        }

        RoleRecord trying = this->party.roles[index];

        for (std::size_t slot = 0; slot < Party::SlotKinds.size(); ++slot)
        {
            const u32 kind = Party::SlotKinds[slot];
            const u32 fits = item != nullptr ? (kind & item->record.data.kind) & ~kKindTopBit : 0;

            if (item != nullptr && fits == kind)
            {
                ApplyObject(trying, item->record.data, this->database);
            }
            else if (slots[slot] != nullptr)
            {
                ApplyObject(trying, slots[slot]->record.data, this->database);
            }
        }

        comparison.attack =
            static_cast<i16>(trying.attack - worn.attack) + static_cast<i16>(trying.attackBonus - worn.attackBonus);
        comparison.defense =
            static_cast<i16>(trying.defense - worn.defense) + static_cast<i16>(trying.defenseBonus - worn.defenseBonus);
        comparison.agility = static_cast<i16>(trying.agility - worn.agility);
    }
}

// swd32.exe 0x0044FD30: beside each party role's face its changes of attack (攻), defence (防) and agility (敏):
// "+n" in white, "-n" in red, "----" when it cannot use the object or nothing changes
void Game::DrawEquipmentCompare()
{
    const auto color = static_cast<Pixel>(this->painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    const auto gain = static_cast<Pixel>(this->painter.MakeColor(kGainRed, kGainGreen, kGainBlue));
    const auto loss = static_cast<Pixel>(this->painter.MakeColor(kLossRed, 0, 0));

    for (int role = 0; role < Party::Roles; ++role)
    {
        if (!this->state.GameFlag(Party::FirstRoleFlag + role))
        {
            continue;
        }

        const ShopComparison& comparison = this->battleState.comparisons[static_cast<std::size_t>(role)];
        const int y = kCompareTop + kCompareStep * role;
        const std::array<std::pair<std::u8string_view, i32>, 3> lines = {{
            {kAttackLabel, comparison.attack},
            {kDefenseLabel, comparison.defense},
            {kAgilityLabel, comparison.agility},
        }};

        for (std::size_t line = 0; line < lines.size(); ++line)
        {
            const int lineY = y + kCompareLine * (static_cast<int>(line) - 1);
            const auto& [label, change] = lines[line];
            this->mediumFont.DrawText(this->drawSurface, kCompareLabelX, lineY, label, color, kPanelStyle);
            Pixel shade = color;
            std::string text(kNoChange);

            if (comparison.usable != 0 && change != 0)
            {
                shade = change < 0 ? loss : gain;
                text = WinFormat(kChangeFormat, {static_cast<i32>(change <= 0 ? '-' : '+'), std::abs(change)});
            }

            this->mediumFont.DrawText(this->drawSurface, kCompareValueX, lineY, text, shade, kPanelStyle);
        }
    }
}

// swd32.exe 0x0044EA60: a frame of the shop (Game::ItemChoiceMenu): opened when entered, its acts reset on bit 29, the
// mouse and the keys; then, while it stays open, the page over the grey map: the faces and the help box, the gold, the
// choice; the list with each object's price, count chosen, count carried and arrows (the row chosen with its
// description, picture and comparisons, the faces of the roles that cannot wear it lowered), the total and the balance
// and the scroll bar; the total, equip and too little money prompts; the pointer
void Game::ShopFrame()
{
    Menu& menu = this->menu;

    if ((this->menuState & Game::MenuEntering) != 0)
    {
        this->menuState &= ~Game::MenuEntering;
        this->OpenShop();
    }

    if ((this->menuState & Game::MenuActsReset) != 0)
    {
        this->ResetShopActs();
        this->menuState &= Game::MenuNumberMask;
    }

    this->ShopMouse();
    this->ShopKeys();

    if (this->menuState == Game::NoMenu ||
        this->menuState == (Game::MenuEntering | Game::MenuOnEquipment | Game::MainMenu))
    {
        return;
    }

    menu.KeepFrame();
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);
    this->DrawShop();
    menu.FrameDrawn();
    this->ShowMenuFrame();
}

void Game::DrawShop()
{
    BattleState& shop = this->battleState;
    Menu& menu = this->menu;
    const auto color = static_cast<Pixel>(this->painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    std::copy(shop.shopScreen.begin(), shop.shopScreen.end(), this->drawSurface);
    ActState& panel = this->battleMessages.panelAct;

    auto box = [this, &panel](int x, int y, int width, int height, FillMode mode, int shade)
    {
        FillBox(this->painter, x, y, width, height, shade, shade, shade, mode);
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            x + kFrameInset,
            y + kFrameInset,
            x + width - kFrameInset,
            y + height - kFrameInset,
            0,
            kFrameBorder
        );
    };

    auto highlight = [this](int x, int y, int width)
    {
        FillBox(this->painter, x, y, width, kHighlightHeight, kHighlightRed, kHighlightGreen, 0, FillMode::Gradient);
    };

    if (shop.shopStage > kShopChoice)
    {
        this->actDrawer.Draw(menu.unk4FC840, 0, 0);

        for (int role = 0; role < Party::Roles; ++role)
        {
            if (this->state.GameFlag(Party::FirstRoleFlag + role))
            {
                const auto index = static_cast<std::size_t>(role);
                this->actDrawer.Draw(
                    menu.items[index],
                    menu.Shown(Menu::FaceSlideValue + index, shop.faceSlides[index]) + kFaceX,
                    kFaceTops[index]
                );
            }
        }

        FillBox(this->painter, kHelpX, kHelpY, kHelpWidth, kHelpHeight, 0, 0, 0, FillMode::Quarter);
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            kHelpX + kFrameInset,
            kHelpY + kFrameInset,
            kHelpRight,
            kHelpBottom,
            0,
            kFrameBorder
        );
    }

    FillBox(this->painter, kGoldBoxX, kGoldBoxY, kGoldBoxWidth, kGoldBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        this->painter,
        this->archive,
        panel.frame,
        kGoldBoxX + kFrameInset,
        kGoldBoxY + kFrameInset,
        kGoldBoxRight,
        kGoldBoxBottom,
        0,
        kFrameBorder
    );
    this->actDrawer.DrawCounter(kGoldRight, kGoldY, static_cast<u32>(this->party.variables[0]), this->counterAlpha);

    if (shop.shopStage == kShopChoice)
    {
        box(kChoiceBoxX, kChoiceBoxY, kChoiceBoxWidth, kChoiceBoxHeight, FillMode::Eighth, 0);
        this->largeFont.DrawText(
            this->drawSurface,
            kChoiceBoxX + kFrameInset,
            kChoiceBoxY + kFrameInset,
            kGreeting,
            color,
            kPanelStyle
        );
        int x = kChoiceWidthPixels * (shop.mode & kSellChoice) + kChoiceHighlightX;

        if ((shop.mode & kChoiceMask) == kLeaveChoice)
        {
            x = kLeaveHighlightX;
        }

        this->largeFont.DrawText(this->drawSurface, kChoicesX, kChoicesY, kChoices, color, kPanelStyle);
        highlight(x, kChoicesY - 1, kHighlightWidth);
    }

    if (shop.shopStage >= kShopList)
    {
        this->DrawShopList(color);
    }

    if (shop.shopStage == kShopTotal)
    {
        const std::string text =
            WinFormat((shop.mode & kSellChoice) != 0 ? kSellTotal : kBuyTotal, {TotalItemValue(shop)});
        const auto length = static_cast<int>(text.size());
        const int width = kPromptCharWidth * length + kPromptPad;
        FillBox(
            this->painter,
            kPromptX,
            kPromptY,
            kPromptCharWidth * length + kPromptBoxPad,
            kPromptHeight,
            kPromptShade,
            kPromptShade,
            kPromptShade,
            FillMode::Eighth
        );
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            kPromptX + kFrameInset,
            kPromptY + kFrameInset,
            width + kPromptX + kFrameInset,
            kPromptBottom,
            0,
            kFrameBorder
        );
        this->largeFont.DrawText(this->drawSurface, kPromptTextX, kTotalTextY, text, color, kPanelStyle);
        this->largeFont.DrawText(this->drawSurface, kTotalAnswersX, kAnswersTextY, kTotalAnswers, color, kPanelStyle);
        highlight(
            kAnswerStep * ((shop.mode & kTotalCancel) == kTotalCancel ? 1 : 0) + kTotalHighlightX,
            kAnswersTextY - 2,
            kAnswerStep
        );
    }

    if (shop.shopStage == kShopEquip)
    {
        FillBox(
            this->painter,
            kPromptX,
            kPromptY,
            kEquipBoxWidth,
            kPromptHeight,
            kPromptShade,
            kPromptShade,
            kPromptShade,
            FillMode::Eighth
        );
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            kPromptX + kFrameInset,
            kPromptY + kFrameInset,
            kEquipBoxRight,
            kPromptBottom,
            0,
            kFrameBorder
        );
        this->largeFont
            .DrawText(this->drawSurface, kPromptTextX, kPromptY + kFrameInset, kEquipNow, color, kPanelStyle);
        this->largeFont.DrawText(this->drawSurface, kPromptTextX, kAnswersTextY, kEquipAnswers, color, kPanelStyle);
        highlight(
            kAnswerStep * ((shop.mode & kEquipCancel) == kEquipCancel ? 1 : 0) + kPromptTextX,
            kAnswersTextY - 2,
            kAnswerStep
        );
    }

    if (shop.shopStage == kShopPoor)
    {
        FillBox(
            this->painter,
            kPromptX,
            kPromptY,
            kPoorBoxWidth,
            kPoorBoxHeight,
            kPromptShade,
            kPromptShade,
            kPromptShade,
            FillMode::Eighth
        );
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            kPromptX + kFrameInset,
            kPromptY + kFrameInset,
            kPoorBoxRight,
            kPoorBoxBottom,
            0,
            kFrameBorder
        );
        this->largeFont.DrawText(this->drawSurface, kPromptTextX, kPoorTextY, kTooPoor, color, kPanelStyle);
    }

    this->scene.pointerRole.act.action = Scene::ChoicePointer;
    this->RefreshCursor();
}

// The list page of the shop (inline in 0x0044EA60, stages from 2).
void Game::DrawShopList(Pixel color)
{
    BattleState& shop = this->battleState;
    Menu& menu = this->menu;
    ActState& panel = this->battleMessages.panelAct;
    menu.DrawBox(kRunX, kRunY, kRunWidth, kRunHeight, kRunMode, 0, 0, 0);
    this->mediumFont.DrawText(this->drawSurface, kRunX + 2, kRunY, kRun, color, kPanelStyle);
    const auto x = static_cast<u32>(this->state.pointerX);
    const auto y = static_cast<u32>(this->state.pointerY);

    if (x < kRunRight && x > kRunLeft && y < kRunBottom && y > kRunTop && TotalItemValue(shop) > 0)
    {
        FillBox(
            this->painter,
            kRunLeft,
            kRunTop,
            kHighlightWidth,
            kHighlightHeight,
            kHighlightRed,
            kHighlightGreen,
            0,
            FillMode::Gradient
        );
    }

    FillBox(this->painter, kTitleBoxX, kTitleBoxY, kTitleBoxWidth, kTitleBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        this->painter,
        this->archive,
        panel.frame,
        kTitleBoxX + kFrameInset,
        kTitleBoxY + kFrameInset,
        kTitleBoxRight,
        kTitleBoxBottom,
        0,
        kFrameBorder
    );
    this->largeFont.DrawText(
        this->drawSurface,
        kTitleTextX,
        kTitleBoxY + kFrameInset,
        (shop.mode & kSellChoice) == kSellChoice ? kSellTitle : kBuyTitle,
        color,
        kPanelStyle
    );
    FillBox(this->painter, kTitleBoxX, kListBoxY, kListBoxWidth, kListBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        this->painter,
        this->archive,
        panel.frame,
        kTitleBoxX + kFrameInset,
        kListBoxY + kFrameInset,
        kListBoxRight,
        kListBoxBottom,
        0,
        kFrameBorder
    );
    this->painter.SetClipClamped(0, 0, kTitleClipRight, Display::Height);
    this->actDrawer.Draw(shop.shopTitle, kTitleLeftX, kTitleActY);
    this->painter.SetClipClamped(kTitleClipLeft, 0, Display::Width, Display::Height);
    this->actDrawer.Draw(shop.shopTitle, kTitleRightX, kTitleActY);
    this->painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    int row = 0;
    int rowY = kFirstRowY;

    for (ItemNode* item = shop.listB; item != nullptr && rowY < kRowsEndY;
         item = item->next.get(), rowY += kRowHeight, ++row)
    {
        const bool chosen = row == shop.listRow;

        if (chosen)
        {
            this->DrawShopChosen(*item, color);
        }

        const int rate = (shop.mode & kSellChoice) == kSellChoice ? kSellRate : kFullRate;
        const int have = CountItemTotal(shop, this->party, this->state, item->id);
        this->largeFont.DrawText(
            this->drawSurface,
            kNameX,
            rowY - kNameRaise,
            WinFormat(kNameFormat, {std::string(ObjectName(item->record))}),
            color,
            kPanelStyle
        );
        this->mediumFont.DrawText(
            this->drawSurface,
            kPriceX,
            rowY,
            WinFormat(kPriceFormat, {rate * item->record.data.price / kFullRate}),
            color,
            kPanelStyle
        );
        this->mediumFont.DrawText(
            this->drawSurface,
            kChosenX,
            rowY,
            WinFormat(kCountFormat, {static_cast<i32>(static_cast<u16>(item->unk06))}),
            color,
            kPanelStyle
        );
        this->mediumFont
            .DrawText(this->drawSurface, kCarriedX, rowY, WinFormat(kCountFormat, {have}), color, kPanelStyle);
        menu.unk4FBA18.id = kFaceAct;
        menu.unk4FBA18.action = kPlusAction;
        menu.unk4FBA18.direction = 0;
        menu.unk4FB8D8.id = kFaceAct;
        menu.unk4FB8D8.action = kMinusAction;
        menu.unk4FB8D8.direction = 0;

        auto press = [chosen](i16& frames)
        {
            if (!chosen)
            {
                return 0;
            }

            if (--frames >= 0)
            {
                return 1;
            }

            frames = 0;
            return 0;
        };

        const int plus = press(shop.morePressed);
        this->actDrawer.Draw(menu.unk4FBA18, plus + kPlusX, rowY + plus - 2);
        const int minus = press(shop.lessPressed);
        this->actDrawer.Draw(menu.unk4FB8D8, minus + kMinusX, rowY + minus - 2);

        if (chosen && item->id != ItemNode::Empty)
        {
            FillBox(
                this->painter,
                kTitleBoxX,
                rowY - kRowRaise,
                kRowWidth,
                kHighlightHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }

    FillBox(this->painter, kTitleBoxX, kTotalBoxY, kTotalBoxWidth, kTotalBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        this->painter,
        this->archive,
        panel.frame,
        kTitleBoxX + kFrameInset,
        kTotalBoxY + kFrameInset,
        kTotalBoxRight,
        kTotalBoxBottom,
        0,
        kFrameBorder
    );
    const int total = TotalItemValue(shop);
    const int spent = (shop.mode & kChoiceMask) == kSellChoice ? -total : total;
    this->mediumFont.DrawText(
        this->drawSurface,
        kTotalTextX,
        kTotalBoxY + kFrameInset,
        WinFormat(kTotalFormat, {total, this->party.variables[0] - spent}),
        color,
        kPanelStyle
    );

    if (shop.itemCount <= kPageRows)
    {
        return;
    }

    int arrows = 0;

    if ((shop.scrollArrows & kScrollUpMask) != 0)
    {
        arrows = 1;
        shop.scrollArrows =
            static_cast<u16>((shop.scrollArrows & ~kScrollUpMask) | ((shop.scrollArrows & kScrollUpMask) - 1));
    }

    if ((shop.scrollArrows & kScrollDownMask) != 0)
    {
        arrows |= 2;
        const int down = (shop.scrollArrows & kScrollDownMask) >> kNibbleShift;
        shop.scrollArrows = static_cast<u16>((shop.scrollArrows & ~kScrollDownMask) | ((down - 1) << kNibbleShift));
    }

    const auto count = static_cast<double>(shop.itemCount);
    const auto to = static_cast<float>(static_cast<double>(shop.listTop + shop.listBCount) / count);
    const auto from = static_cast<float>(static_cast<double>(shop.listTop) / count);
    menu.DrawScrollBar(kBarX, kBarY, kBarHeight, arrows, from, to, menu.scrollBar);
}

// The row chosen in the shop list (inline in 0x0044EA60): its comparisons, picture and description; the faces raised
// (lowered for the roles that cannot wear an equipment) and chosen.
void Game::DrawShopChosen(const ItemNode& item, Pixel color)
{
    BattleState& shop = this->battleState;
    Menu& menu = this->menu;

    if (item.id != ItemNode::Empty)
    {
        this->DrawEquipmentCompare();

        if (item.record.data.unk50 != 0)
        {
            menu.gauge.id = item.record.data.unk50;
            menu.gauge.action = kPictureAction;
            menu.gauge.direction = 0;
            this->actDrawer.Draw(menu.gauge, kPictureX, kPictureY);
        }

        this->mediumFont.SetClip(kHelpX, kHelpTextClipTop, kHelpTextClipRight, kHelpTextClipBottom);
        this->mediumFont
            .DrawText(this->drawSurface, kHelpX + kFrameInset, kHelpTextY, menu.description, color, kPanelStyle);
        this->mediumFont.SetClip(0, 0, Display::Width, Display::Height);
    }

    for (ActState& face : menu.items)
    {
        face.action = kFaceChosenAction;
    }

    // The rebuild's continuous motion: the faces drawn this frame slide to where the next frame draws them.
    const std::array<i16, Party::Roles> drawn = shop.faceSlides;

    auto move = [&menu, &shop, &drawn]()
    {
        for (std::size_t role = 0; role < drawn.size(); ++role)
        {
            menu.Move(Menu::FaceSlideValue + role, drawn[role], drawn[role], shop.faceSlides[role]);
        }
    };

    for (i16& slide : shop.faceSlides)
    {
        slide = std::min<i16>(static_cast<i16>(slide + 1), kFaceSlideMost);
    }

    if ((item.record.data.kind & kEquipmentKinds) == 0)
    {
        move();
        return;
    }

    const u16 usable = item.record.data.usableBy;

    for (int role = 0; role < Party::Roles; ++role)
    {
        if ((usable & (ObjectData::FirstRoleBit >> role)) != 0)
        {
            continue;
        }

        i16& slide = shop.faceSlides[static_cast<std::size_t>(role)];
        slide = std::max<i16>(static_cast<i16>(slide - 2), kFaceSlideLeast);
        menu.items[static_cast<std::size_t>(role)].action = role == 0 ? kFaceUnusableAction : kFaceAction;
    }

    move();
}

}  // namespace swd3
