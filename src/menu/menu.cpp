#include "menu/menu.h"

#include <cmath>
#include <span>
#include <string>
#include <utility>

#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "pic/picture.h"
#include "platform/display.h"
#include "text/text_input.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr i32 kRepeatAfter = 7;  // frames a key slot is held before it repeats

constexpr int kFirstRoleFlag = GameState::InParty;
constexpr int kSwappedPagesFlag = GameState::SwappedPages;
constexpr int kHiddenPointerFlag = GameState::NoChoicePointer;
constexpr int kMenuSound = 0xBB;
constexpr int kSlideMaximum = 0x320;
constexpr int kTabGlowMaximum = 0x10;
constexpr int kTabGlowMinimum = 6;

// The modes of the role pages (Menu::mode) SetPage tells them by.
constexpr std::pair<u16, u16> kItemsModes = {0x1E, 0x20};
constexpr std::pair<u16, u16> kEquipmentModes = {0x24, 0x29};
constexpr std::pair<u16, u16> kSkillsModes = {0x2A, 0x2E};
constexpr std::pair<u16, u16> kStatusModes = {0x30, 0x34};
constexpr std::pair<u16, u16> kOptionsModes = {0x36, 0x3A};  // the altar's while the pages are swapped
constexpr std::pair<u16, u16> kAltarModes = {0x3C, 0x3E};    // the system tab's while the pages are swapped
constexpr std::pair<u16, u16> kBestiaryModes = {0x42, 0x47};
constexpr std::u8string_view kLevelFormat = u8"%2d 級/%d";  // swd32.exe 0x004A6ABC: the level, the experience needed
constexpr std::string_view kRatioFormat = "%4d/%4d";

// The actions of the interface act (Scene::InterfaceAct) and of the effect act in the menus.
constexpr i32 kTabsPanelAction = 1;
constexpr i32 kFaceAction = 3;
constexpr i32 kHitPointBarAction = 4;
constexpr i32 kSpiritBarAction = 5;
constexpr i32 kStaminaBarAction = 6;
constexpr i32 kFirstTabAction = 8;  // the animations of the four tabs
constexpr i32 kBarTrackAction = 0x18;
constexpr i32 kBarThumbAction = 0x19;
constexpr i32 kListUpArrowAction = 0x1A;
constexpr i32 kListDownArrowAction = 0x1B;
constexpr i32 kRoleBackgroundAction = 0x34;
constexpr i32 kSystemBackgroundAction = 0x4E;
constexpr i32 kFirstSystemItemAction = 0x2C;  // of the effect act: the four title entries

// Menu::frames: the frame counter of an opening menu.
constexpr i32 kOpeningFrame = 0x1E;
constexpr i32 kItemsOpeningFrame = 0x24;
constexpr i32 kSystemOpenedFrame = 0x40;
constexpr i32 kOpenedFrame = 0x41;  // the first frame shown: the tabs start sliding in
constexpr i32 kSlideFrames = 0x4B;  // the background slides until this frame
constexpr int kSlideGrowth = 2;     // sliding out, the tabs double their offset every frame
constexpr int kBackgroundX = 0xB4;

constexpr int kShadowRed = 6;  // the shadow of the menu fonts
constexpr int kShadowGreen = 4;
constexpr int kShadowBlue = 3;
constexpr i32 kEscapeDelay = 6;

// Menu::clicks.
constexpr u32 kLeftClick = 1;
constexpr u32 kLeftDoubleClick = 2;
constexpr u32 kRightClick = 4;
constexpr u32 kRightDoubleClick = 8;

// The title of a role page.
constexpr i32 kTitleLit = 0x10;  // the glow of a fully lit title
constexpr i32 kTitleDim = 8;
constexpr int kTitleSpacing = 0x50;
constexpr int kSwappedTitleSpacing = 0x46;
constexpr int kTitleX = 0xDC;
constexpr int kTitleY = 0xA;
constexpr int kTitleArrowY = 9;
constexpr i32 kLitTitleActions = 5;  // the lit title of page p is action p + kLitTitleActions
constexpr i32 kFirstLitTitleAction = 0x14;
constexpr i32 kGlowTitleActions = 0x19;
constexpr i32 kFirstGlowTitleAction = 0x28;

// The role tabs.
constexpr int kTabRed = 0x1D;
constexpr int kTabGreen = 0x1B;
constexpr int kTabBlue = 0x15;
constexpr int kTabGlowRise = 3;
constexpr int kTabTextStyle = TextFont::BoldShadow;
constexpr int kBarX = 0x64;
constexpr int kBarLength = 0x4F;
constexpr int kHitPointBarUp = 0x27;
constexpr int kSpiritBarUp = 0x11;
constexpr int kStaminaBarDown = 5;
constexpr int kNameLeft = 0x58;
constexpr int kNameUp = 0x5E;
constexpr int kLevelX = 0x5C;
constexpr int kLevelUp = 0x4C;
constexpr int kRatioX = 0x6C;
constexpr int kHitPointsUp = 0x34;
constexpr int kSpiritUp = 0x1E;
constexpr int kStaminaUp = 8;
constexpr u16 kDefeatedRole = 0x8000;  // RoleRecord::condition bit 15: the tab is greyed

constexpr int kBoxMargin = 8;
constexpr int kBoxFrameThickness = 8;
constexpr int kBoxBorder = static_cast<int>(0x80000000u | kBoxFrameThickness);  // bit 31: the frame only

// A key slot acting once when it goes down.
bool Pressed(const ButtonState& button)
{
    return button.press != 0 && button.heldFrames == 1;
}

// A key slot acting when it goes down and every second frame after the seventh.
bool Repeating(const ButtonState& button, i32 parity)
{
    return button.press != 0 && (button.heldFrames == 1 || ((parity & 1) == 0 && button.heldFrames > kRepeatAfter));
}

}  // namespace

void Menu::Nothing()
{
}

void Menu::Call(Handler handler)
{
    if (handler != nullptr)
    {
        (this->*handler)();
    }
}

// swd32.exe 0x00439FD0: the menu of the frame. On entering (bit 31 of the menu state) the page opens: the role pages
// (menus 1..2), the system page in its title mode (3, 6), or modes 1 and 2 (4, 5); then the keys, the handlers and
// the drawing of the page run.
void Menu::Run()
{
    Game& owner = this->game;
    const u32 state = owner.menuState;

    if (static_cast<i32>(state) < 0)
    {
        this->pointerX = 0;
        this->pointerY = 0;
        this->tabsPanelAct.action = kTabsPanelAction;
        const u32 menu = state & Game::MenuNumberMask;

        if (menu <= Game::ItemChoiceMenu)
        {
            owner.menuMode = Menu::NoSlotsMode;
            this->background.id = Scene::InterfaceAct;
            this->background.action = kRoleBackgroundAction;
            this->upArrowAct.id = Scene::InterfaceAct;
            this->downArrowAct.id = Scene::InterfaceAct;
            this->upArrowAct.action = kListUpArrowAction;
            this->downArrowAct.action = kListDownArrowAction;

            for (std::size_t item = 0; item < this->items.size(); ++item)
            {
                this->items[item].id = Scene::InterfaceAct;
                this->items[item].action = static_cast<i32>(kFirstTabAction + item);
            }

            this->openedOnEquipment = 0;
            this->frames = kOpeningFrame;

            if ((state & Game::MenuOnEquipment) != 0)
            {
                this->openedOnEquipment = 1;
                this->frames = kItemsOpeningFrame;
                this->Open(Menu::EquipmentMode, Menu::PageCode);
                this->frames = kOpenedFrame;
                this->Call(this->enterTable[this->page]);
                this->beforeFrame = nullptr;
                this->frame = &Menu::EquipFrame;
            }
            else
            {
                this->Open(Menu::FirstPageMode, Menu::MainCode);
            }

            owner.audio.PlaySound(kMenuSound, owner.config.soundVolume);
        }

        if (menu == Game::TitleScreenMenu || menu == Game::NameEntryMenu)
        {
            owner.archive.CloseAll();

            for (ActState& item : this->items)
            {
                item = ActState{};
                ResetPlayback(item);
                item.id = Scene::EffectAct;
            }

            this->background.action = kSystemBackgroundAction;

            for (std::size_t item = 0; item < this->items.size(); ++item)
            {
                this->items[item].action = static_cast<i32>(kFirstSystemItemAction + item);
            }

            this->Open(menu == Game::NameEntryMenu ? Menu::NameEntryMode : Menu::TitleMode, Menu::SystemPage);
        }

        if (menu == Game::SystemMenuA)
        {
            owner.archive.CloseAll();
            this->Open(Menu::GameModeA, Menu::SystemPage);
        }

        if (menu == Game::SystemMenuB)
        {
            owner.archive.CloseAll();
            this->Open(Menu::GameModeB, Menu::SystemPage);
        }

        this->frames = kSystemOpenedFrame;
        this->flags = 0;
        owner.menuState &= Game::MenuEnteredMask;
    }

    ++this->frames;
    this->ReadMouse();
    this->Dispatch();

    if (owner.menuState != Game::NoMenu)
    {
        this->Present();
    }

    if (owner.menuState == Game::NoMenu)
    {
        this->flags &= ~2u;
    }
}

// swd32.exe 0x0043A2A0: opens page `menuCode` in `menuMode` (the role pages count from 0x1E in steps of 6): its
// handlers, the tabs, cleared keys and the shadow of the three fonts
void Menu::Open(int menuMode, u16 menuCode)
{
    Game& owner = this->game;
    this->code = menuCode;
    this->roleTab = Menu::NoTab;
    this->page = static_cast<u16>((menuMode - Menu::FirstPageMode) / Menu::PageModeStep + Menu::FirstRolePage);
    this->mode = static_cast<u16>(menuMode);
    this->unk4FC200 = static_cast<u16>(menuMode);
    this->unk4FBAB8 = static_cast<u16>(menuMode);
    this->SetPage(menuCode);
    this->BuildTabs(this->roleTab);
    owner.controls.buttons = {};
    this->tint = 0;
    const auto shadow = static_cast<Pixel>(owner.painter.MakeColor(kShadowRed, kShadowGreen, kShadowBlue));

    for (TextFont* font : {&owner.largeFont, &owner.mediumFont, &owner.smallFont})
    {
        font->SetShadow(shadow);
    }

    for (TextFont* font : {&owner.largeFont, &owner.mediumFont, &owner.smallFont})
    {
        font->SetBackground(TextFont::NoBackground);
    }
}

// swd32.exe 0x0043A380: a tab for every role in the party (flags 0x1E..0x21), their animations at action 8 + their
// place, the tab `selected` raised. The original then copies a word of the tab past the last one (beyond the four when
// all roles are present).
void Menu::BuildTabs(int selected)
{
    for (MenuTab& tab : this->tabs)
    {
        tab.role = 0;
        tab.unk0A = 0;
        tab.state = 0;
        tab.stateCopy = 0;
    }

    this->unk4A6AA8 = Menu::NoRole;
    int count = 0;

    for (int role = 0; role < Menu::Tabs; ++role)
    {
        MenuTab& tab = this->tabs[static_cast<std::size_t>(role)];
        tab.role = Menu::NoRole;

        if (this->game.state.GameFlag(role + kFirstRoleFlag))
        {
            tab.role = static_cast<u16>(role);
            tab.nameAction = static_cast<u16>(count + kFirstTabAction);
            tab.iconAction = static_cast<u16>(count + kFirstTabAction);
            tab.frameAction = static_cast<u16>(count + kFirstTabAction);
            tab.state = count == selected ? Menu::RaisedTab : Menu::PlainTab;
            tab.stateCopy = tab.state;
            ++count;
        }
    }

    if (count < Menu::Tabs)
    {
        this->tabs[static_cast<std::size_t>(count)].state = this->tabs[static_cast<std::size_t>(count)].frameAct;
    }
}

// swd32.exe 0x0043B480: the handlers of page `menuCode`: the system page; the main page (the page tables filled, its
// titles' mouse run before every frame); a role page by the mode (items, equipment, skills, status, the system tab and
// the altar exchanged while the pages are swapped, the bestiary). A slot a page does not set keeps the handler of the
// page before it.
void Menu::SetPage(int menuCode)
{
    if (menuCode == Menu::SystemPage)
    {
        this->SystemEnter();
        this->beforeFrame = nullptr;
        this->frame = &Menu::SystemFrame;
        this->down = &Menu::SystemDown;
        this->up = &Menu::SystemUp;
        this->left = &Menu::SystemLeft;
        this->right = &Menu::SystemRight;
        this->pageDown = &Menu::SystemLast;
        this->pageUp = &Menu::SystemFirst;
        this->shift = &Menu::Nothing;
        this->end = &Menu::SystemEnd;
        this->select = &Menu::SystemSelect;
        this->cancel = &Menu::SystemCancel;
        this->draw = &Menu::SystemDraw;
        return;
    }

    if (menuCode == Menu::MainCode)
    {
        this->FillPageTables();
        this->beforeFrame = &Menu::MainFrame;
        this->frame = &Menu::Nothing;
        this->down = &Menu::Nothing;
        this->up = &Menu::Nothing;
        this->left = &Menu::MainLeft;
        this->right = &Menu::MainRight;
        this->pageDown = &Menu::Nothing;
        this->pageUp = &Menu::Nothing;
        this->shift = &Menu::Nothing;
        this->end = &Menu::Nothing;
        this->select = &Menu::MainSelect;
        this->cancel = &Menu::MainCancel;
        this->draw = &Menu::MainDraw;
        return;
    }

    if (menuCode != Menu::PageCode)
    {
        return;
    }

    const bool swapped = this->game.state.GameFlag(kSwappedPagesFlag);
    const u16 mode = this->mode;

    if (mode >= kItemsModes.first && mode <= kItemsModes.second)
    {
        this->frame = &Menu::ItemsFrame;
        this->down = &Menu::ItemsDown;
        this->up = &Menu::ItemsUp;
        this->left = &Menu::ItemsLeft;
        this->right = &Menu::ItemsRight;
        this->pageDown = &Menu::ItemsPageDown;
        this->pageUp = &Menu::ItemsPageUp;
        this->shift = &Menu::ItemsShift;
        this->end = &Menu::ItemsEnd;
        this->select = &Menu::ItemsSelect;
        this->cancel = &Menu::ItemsCancel;
        this->draw = &Menu::ItemsDraw;
    }
    else if (mode >= kEquipmentModes.first && mode <= kEquipmentModes.second)
    {
        this->frame = &Menu::EquipFrame;
        this->down = &Menu::EquipDown;
        this->up = &Menu::EquipUp;
        this->left = &Menu::EquipLeft;
        this->right = &Menu::EquipRight;
        this->pageDown = &Menu::EquipPageDown;
        this->pageUp = &Menu::EquipPageUp;
        this->shift = &Menu::EquipShift;
        this->end = &Menu::Nothing;
        this->select = &Menu::EquipSelect;
        this->cancel = &Menu::EquipCancel;
        this->draw = &Menu::EquipDraw;
    }
    else if (mode >= kSkillsModes.first && mode <= kSkillsModes.second)
    {
        this->frame = &Menu::SkillsFrame;
        this->down = &Menu::SkillsDown;
        this->up = &Menu::SkillsUp;
        this->left = &Menu::SkillsLeft;
        this->right = &Menu::SkillsRight;
        this->pageDown = &Menu::SkillsPageDown;
        this->pageUp = &Menu::SkillsPageUp;
        this->shift = &Menu::SkillsShift;
        this->end = &Menu::SkillsEnd;
        this->select = &Menu::SkillsSelect;
        this->cancel = &Menu::SkillsCancel;
        this->draw = &Menu::SkillsDraw;
    }
    else if (mode >= kStatusModes.first && mode <= kStatusModes.second)
    {
        this->frame = &Menu::StatusFrame;
        this->down = &Menu::StatusNext;
        this->up = &Menu::StatusPrevious;
        this->left = &Menu::StatusPrevious;
        this->right = &Menu::StatusNext;
        this->pageDown = &Menu::Nothing;
        this->pageUp = &Menu::Nothing;
        this->shift = &Menu::StatusNext;
        this->select = &Menu::Nothing;
        this->cancel = &Menu::StatusCancel;
        this->draw = &Menu::StatusDraw;
    }
    else if (
        (mode >= kOptionsModes.first && mode <= kOptionsModes.second && !swapped) ||
        (mode >= kAltarModes.first && mode <= kAltarModes.second && swapped)
    )
    {
        this->frame = &Menu::OptionsFrame;
        this->down = &Menu::OptionsDown;
        this->up = &Menu::OptionsUp;
        this->left = &Menu::OptionsLeft;
        this->right = &Menu::OptionsRight;
        this->pageDown = &Menu::OptionsPageDown;
        this->pageUp = &Menu::OptionsPageUp;
        this->shift = &Menu::Nothing;
        this->select = &Menu::OptionsSelect;
        this->cancel = &Menu::OptionsCancel;
        this->draw = &Menu::OptionsDraw;
    }
    else if (
        (mode >= kOptionsModes.first && mode <= kOptionsModes.second) ||
        (mode >= kAltarModes.first && mode <= kAltarModes.second)
    )
    {
        this->frame = &Menu::AltarFrame;
        this->down = &Menu::AltarDown;
        this->up = &Menu::AltarUp;
        this->left = &Menu::AltarLeft;
        this->right = &Menu::AltarRight;
        this->pageDown = &Menu::AltarPageDown;
        this->pageUp = &Menu::AltarPageUp;
        this->shift = &Menu::AltarShift;
        this->select = &Menu::AltarSelect;
        this->cancel = &Menu::AltarCancel;
        this->end = &Menu::AltarEnd;
        this->draw = &Menu::AltarDraw;
    }
    else if (mode >= kBestiaryModes.first && mode <= kBestiaryModes.second)
    {
        this->frame = &Menu::BestiaryFrame;
        this->down = &Menu::BestiaryDown;
        this->up = &Menu::BestiaryUp;
        this->left = &Menu::BestiaryLeft;
        this->right = &Menu::BestiaryRight;
        this->pageDown = &Menu::BestiaryPageDown;
        this->pageUp = &Menu::BestiaryPageUp;
        this->shift = &Menu::BestiaryShift;
        this->select = &Menu::Nothing;
        this->cancel = &Menu::BestiaryCancel;
        this->draw = &Menu::BestiaryDraw;
    }
}

// swd32.exe 0x0043A470: the handlers of the frame and of the keys going down (the cursor keys repeat). The repeats of
// Left and Right test the parity of PageUp's held frames.
void Menu::Dispatch()
{
    Controls& keys = this->game.controls;
    this->Call(this->beforeFrame);
    this->Call(this->frame);

    if (Pressed(keys.At(Button::Escape)))
    {
        this->Call(this->cancel);
    }

    if (this->escapeDelay == 0 && Pressed(keys.At(Button::RightControl)))
    {
        this->escapeDelay = kEscapeDelay;
        this->Call(this->cancel);
    }

    if (--this->escapeDelay < 0)
    {
        this->escapeDelay = 0;
    }

    if (Pressed(keys.At(Button::RightShift)))
    {
        this->Call(this->shift);
    }

    if (Pressed(keys.At(Button::End)))
    {
        this->Call(this->end);
    }

    const ButtonState& downKey = keys.At(Button::Down);

    if (Repeating(downKey, downKey.heldFrames))
    {
        this->Call(this->down);
    }

    const ButtonState& upKey = keys.At(Button::Up);

    if (Repeating(upKey, upKey.heldFrames))
    {
        this->Call(this->up);
    }

    const ButtonState& pageDownKey = keys.At(Button::PageDown);

    if (Repeating(pageDownKey, pageDownKey.heldFrames))
    {
        this->Call(this->pageDown);
    }

    const ButtonState& pageUpKey = keys.At(Button::PageUp);

    if (Repeating(pageUpKey, pageUpKey.heldFrames))
    {
        this->Call(this->pageUp);
    }

    if (Repeating(keys.At(Button::Left), keys.At(Button::PageUp).heldFrames))
    {
        this->Call(this->left);
    }

    if (Repeating(keys.At(Button::Right), keys.At(Button::PageUp).heldFrames))
    {
        this->Call(this->right);
    }

    if (Pressed(keys.At(Button::Space)) || Pressed(keys.At(Button::Enter)))
    {
        this->Call(this->select);
    }
}

// swd32.exe 0x0043BA40: the clicks of the frame: bits 0..1 a single or double left click, bits 2..3 the right button
void Menu::ReadMouse()
{
    const Controls& keys = this->game.controls;
    this->clicks = 0;
    const ButtonState& leftButton = keys.At(Button::MouseLeft);

    if (leftButton.heldFrames == 1)
    {
        if (leftButton.press == 1)
        {
            this->clicks = kLeftClick;
        }
        else if (leftButton.press > 1)
        {
            this->clicks = kLeftDoubleClick;
        }
    }

    const ButtonState& rightButton = keys.At(Button::MouseRight);

    if (rightButton.heldFrames == 1)
    {
        if (rightButton.press == 1)
        {
            this->clicks |= kRightClick;
        }
        else if (rightButton.press > 1)
        {
            this->clicks |= kRightDoubleClick;
        }
    }
}

// swd32.exe 0x0043A610: the page drawn over its background (sliding in on the role pages), then the role tabs, the
// gold on page 1, the pointer, a tint, the corner notices, and the frame shown
void Menu::Present()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    // The rebuild's continuous motion: the main menu of the map only (the title and the system menus as they were).
    const bool smooth = (owner.menuState & Game::MenuNumberMask) == Game::MainMenu;

    if (smooth)
    {
        this->KeepFrame();
    }

    owner.drawSurface = owner.painter.LockSurface(*owner.offscreen);
    owner.painter.SetTarget(owner.drawSurface);
    owner.painter.UnlockSurface(*owner.offscreen);

    if (!this->PaintPage(false))
    {
        return;
    }

    if (owner.menuMode == Menu::NoSlotsMode && smooth)
    {
        this->FrameDrawn();
        owner.ShowMenuFrame();
    }
    else if (owner.menuMode == Menu::NoSlotsMode)
    {
        owner.ShowFrame();
    }

    this->lastPage = this->page;
    this->pointerX = state.pointerX;
    this->pointerY = state.pointerY;
}

// The drawing of Present on the target set; `replay`: painted again between frames (the notices kept as they are).
bool Menu::PaintPage(bool replay)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const i32 slideBefore = this->frames == kOpenedFrame ? kSlideMaximum : this->slide;
    this->slide = slideBefore;

    const bool swapped = state.GameFlag(kSwappedPagesFlag);
    const bool doubling = (this->page == Menu::UnkPageF && this->code > Menu::MainCode && swapped) ||
                          (this->page == Menu::FirstRolePage && this->code >= Menu::SubpageCodes);

    if (!doubling)
    {
        this->slide >>= 1;
        this->Move(SlideValue, this->slide, slideBefore, this->slide);
    }

    if (this->code != Menu::SystemPage)
    {
        int offset = this->frames < kSlideFrames ? this->Shown(SlideValue, this->slide) : 0;

        if (swapped && this->page == Menu::UnkPageF)
        {
            offset = 0;
        }

        owner.actDrawer.Draw(this->background, offset + kBackgroundX, 0);
    }

    if (this->code != Menu::MainCode)
    {
        this->Call(this->draw);

        if (owner.menuState == Game::NoMenu)
        {
            return false;
        }
    }

    if (owner.menuMode == Menu::NoSlotsMode)
    {
        if (this->code != Menu::SystemPage && this->page != Menu::UnkPage11)
        {
            if (doubling)
            {
                if (this->slide == 0)
                {
                    this->slide = 1;
                }

                const i32 drawn = this->slide;
                const i32 shown = this->Shown(SlideValue, drawn);
                owner.actDrawer.Draw(this->tabsPanelAct, -shown, 0);
                this->DrawRoleTabs(shown);
                this->slide *= kSlideGrowth;

                if (this->slide > kSlideMaximum)
                {
                    this->slide = kSlideMaximum;
                }

                this->Move(SlideValue, drawn, drawn, this->slide);
            }
            else
            {
                const i32 shown = this->Shown(SlideValue, this->slide);
                this->DrawPageTitle();
                owner.actDrawer.Draw(this->tabsPanelAct, -shown, 0);
                this->DrawRoleTabs(shown);
            }
        }

        if (this->code == Menu::MainCode)
        {
            owner.actDrawer.DrawCounter(
                ActDrawer::GoldX,
                ActDrawer::GoldY,
                static_cast<u32>(owner.party.variables[Party::Gold]),
                owner.counterAlpha
            );
        }

        owner.scene.pointerRole.act.action = Scene::ChoicePointer;

        if (!state.GameFlag(kHiddenPointerFlag))
        {
            owner.RefreshCursor();
        }

        if (this->tint != 0)
        {
            owner.painter
                .TintPixels(std::span<u16>(owner.drawSurface, Display::Pixels), this->tint, this->tint, this->tint);
        }

        owner.notices.Draw(owner.smallFont, owner.painter, state, owner.textColors, !replay);
    }

    return true;
}

// swd32.exe 0x0043A880: the title of the role page: glowing in (out on page 1's code) over 8..16 steps; once fully lit
// the plain picture (actions 0x14 and 0x15 or 0x28 and 0x29 swap with flag 0x49)
void Menu::DrawPageTitle()
{
    Game& owner = this->game;
    const bool swapped = owner.state.GameFlag(kSwappedPagesFlag);
    const int last = this->frames == kOpenedFrame ? 0 : this->titleGlow;

    if (this->code == Menu::MainCode)
    {
        this->titleGlow = last + 1 > kTitleLit ? kTitleLit : last + 1;
    }
    else
    {
        this->titleGlow = last - 1 < kTitleDim ? kTitleDim : last - 1;
    }

    this->Move(TitleGlowValue, this->titleGlow, last, this->titleGlow);
    const double fineGlow = this->ShownFine(TitleGlowValue, this->titleGlow);
    const i32 glow = static_cast<i32>(std::round(fineGlow));
    const auto arrowGlow = static_cast<i32>(std::floor(fineGlow));
    const int spacing = swapped ? kSwappedTitleSpacing : kTitleSpacing;
    this->titleArrowAct.direction = swapped ? ActState::Right : ActState::Left;
    owner.actDrawer.DrawGhost(this->titleArrowAct, 0, kTitleArrowY, arrowGlow, fineGlow - arrowGlow);
    this->titleArrowAct.flags &= Painter::RleSource | Painter::Mirror | Painter::Flip;
    this->titleAct.id = Scene::InterfaceAct;
    const int x = spacing * (this->page - Menu::FirstRolePage) + kTitleX;

    auto swap = [swapped](int action, int first)
    {
        if (!swapped)
        {
            return action;
        }

        return action == first ? first + 1 : action == first + 1 ? first : action;
    };

    if (this->titleGlow == kTitleLit)
    {
        this->titleAct.action = swap(this->page + kLitTitleActions, kFirstLitTitleAction);
        owner.actDrawer.Draw(this->titleAct, x, kTitleY);
        return;
    }

    this->titleAct.action = swap(this->page + kGlowTitleActions, kFirstGlowTitleAction);

    if (owner.acts.Run(this->titleAct) == 0)
    {
        return;
    }

    const Picture* picture = owner.archive.GetPicture(this->titleAct.frame, this->titleAct.picture);

    if (picture == nullptr)
    {
        return;
    }

    owner.painter.SetSource(picture->data.data());
    DrawOptions& options = owner.painter.Options();
    options.tintBlue = glow - kTitleLit;
    options.tintGreen = glow - kTitleLit;
    options.tintRed = glow - kTitleLit;
    owner.painter.DrawPicture(
        x - this->titleAct.x,
        kTitleY - this->titleAct.y,
        picture->width,
        picture->height,
        this->titleAct.flags
    );
}

// swd32.exe 0x0043AAA0: the tabs of the roles shifted left by `offset`: the selected one glowing up, with the bars of
// hit points, spirit and stamina, the name, the level with the experience still needed and the three ratios
void Menu::DrawRoleTabs(int offset)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const auto color = static_cast<Pixel>(painter.MakeColor(kTabRed, kTabGreen, kTabBlue));

    for (std::size_t index = 0; index < Menu::Tabs; ++index)
    {
        i8& glow = this->tabGlow[index];
        const i8 glowBefore = glow;

        if (this->roleTab == index)
        {
            glow = static_cast<i8>(glow + kTabGlowRise > kTabGlowMaximum ? kTabGlowMaximum : glow + kTabGlowRise);
        }
        else
        {
            glow = static_cast<i8>(glow - 1 < kTabGlowMinimum ? kTabGlowMinimum : glow - 1);
        }

        if (this->roleTab == Menu::NoTab || this->code == 1)
        {
            glow = kTabGlowMaximum;
        }

        this->Move(TabGlowValue + index, glow, glowBefore, glow);
        const double fineGlow = this->ShownFine(TabGlowValue + index, glow);
        const auto shownGlow = static_cast<i32>(std::floor(fineGlow));
        const double glowFraction = fineGlow - shownGlow;

        const MenuTab& tab = this->tabs[index];

        if (tab.role == Menu::NoRole)
        {
            continue;
        }

        const RoleRecord& role = owner.party.roles[index];
        const int x = tab.x - offset;
        owner.actDrawer.DrawGhost(this->items[index], x, tab.y, shownGlow, glowFraction);
        painter.SetClipClamped(0, 0, kBarLength * role.hitPoints / role.maxHitPoints + kBarX, Display::Height);
        owner.actDrawer.DrawGhost(this->hitPointBar, kBarX - offset, tab.y - kHitPointBarUp, shownGlow, glowFraction);
        painter.SetClipClamped(0, 0, kBarLength * role.spirit / role.maxSpirit + kBarX, Display::Height);
        owner.actDrawer.DrawGhost(this->spiritBar, kBarX - offset, tab.y - kSpiritBarUp, shownGlow, glowFraction);
        painter.SetClipClamped(0, 0, kBarLength * role.stamina / role.maxStamina + kBarX, Display::Height);
        owner.actDrawer.DrawGhost(this->staminaBar, kBarX - offset, tab.y + kStaminaBarDown, shownGlow, glowFraction);
        owner.mediumFont.DrawText(
            painter.Target(),
            x - kNameLeft,
            tab.y - kNameUp,
            owner.party.names[index].data(),
            color,
            kTabTextStyle
        );
        u32 needed = 0;
        owner.database.LoadLevelEntry(static_cast<int>(index) + 1, role.level + 1, needed);
        const std::string level =
            WinFormat(kLevelFormat, {static_cast<i32>(role.level), static_cast<i32>(needed) - role.totalExperience});
        owner.mediumFont.DrawText(painter.Target(), kLevelX - offset, tab.y - kLevelUp, level, color, kTabTextStyle);
        const std::string hitPoints = WinFormat(kRatioFormat, {role.hitPoints, role.maxHitPoints});
        owner.smallFont
            .DrawText(painter.Target(), kRatioX - offset, tab.y - kHitPointsUp, hitPoints, color, kTabTextStyle);
        const std::string spirit = WinFormat(kRatioFormat, {role.spirit, role.maxSpirit});
        owner.smallFont.DrawText(painter.Target(), kRatioX - offset, tab.y - kSpiritUp, spirit, color, kTabTextStyle);
        const std::string stamina = WinFormat(kRatioFormat, {role.stamina, role.maxStamina});
        owner.smallFont.DrawText(painter.Target(), kRatioX - offset, tab.y - kStaminaUp, stamina, color, kTabTextStyle);
        painter.SetClipClamped(0, 0, Display::Width, Display::Height);

        if ((role.condition & kDefeatedRole) != 0)
        {
            owner.actDrawer.DrawWithMode(this->items[index], x, tab.y, Painter::GrayUnder);
        }
    }
}

// swd32.exe 0x0043BAB0: a menu box: a filled box 8 pixels larger and the frame of act 0x233B
void Menu::DrawBox(int x, int y, int width, int height, int mode, int red, int green, int blue)
{
    Game& owner = this->game;
    FillBox(
        owner.painter,
        x - kBoxMargin,
        y - kBoxMargin,
        width + 2 * kBoxMargin,
        height + 2 * kBoxMargin,
        red,
        green,
        blue,
        static_cast<FillMode>(mode)
    );
    ActState& panel = owner.battleMessages.panelAct;
    panel.id = BattleMessages::PanelAct;
    panel.action = 0;
    owner.acts.Run(panel);
    DrawWindowFrame(owner.painter, owner.archive, panel.frame, x, y, x + width, y + height, 0, kBoxBorder);
}

// swd32.exe 0x00439DE0: the handler tables of the role pages filled and the animations of the menus at their first
// actions
void Menu::InitActs()
{
    Game& owner = this->game;
    this->flags = 0;
    this->FillPageTables();
    ActState& panel = owner.battleMessages.panelAct;

    for (ActState* state :
         {&this->background,
          &this->titleArrowAct,
          &this->tabsPanelAct,
          &this->hitPointBar,
          &this->spiritBar,
          &this->staminaBar,
          &this->barTrackAct,
          &this->barThumbAct,
          &this->upArrowAct,
          &this->downArrowAct,
          &this->gauge,
          &this->items[0],
          &this->items[1],
          &this->items[2],
          &this->items[3],
          &this->titleAct,
          &this->face,
          &panel})
    {
        ResetPlayback(*state);
    }

    this->background.id = Scene::InterfaceAct;
    this->background.action = 0;
    this->tabsPanelAct.id = Scene::InterfaceAct;
    this->tabsPanelAct.action = kTabsPanelAction;
    this->titleArrowAct.id = Scene::InterfaceAct;
    this->titleArrowAct.direction = owner.state.GameFlag(kSwappedPagesFlag) ? ActState::Right : ActState::Left;

    for (ActState* state :
         {&this->hitPointBar,
          &this->spiritBar,
          &this->staminaBar,
          &this->barTrackAct,
          &this->barThumbAct,
          &this->upArrowAct,
          &this->downArrowAct,
          &this->face})
    {
        state->id = Scene::InterfaceAct;
    }

    this->hitPointBar.action = kHitPointBarAction;
    this->spiritBar.action = kSpiritBarAction;
    this->staminaBar.action = kStaminaBarAction;
    this->barTrackAct.action = kBarTrackAction;
    this->barThumbAct.action = kBarThumbAction;
    this->upArrowAct.action = kListUpArrowAction;
    this->downArrowAct.action = kListDownArrowAction;

    for (std::size_t item = 0; item < this->items.size(); ++item)
    {
        this->items[item].id = Scene::EffectAct;
        this->items[item].action = static_cast<i32>(kFirstSystemItemAction + item);
    }

    this->face.action = kFaceAction;
    panel.id = BattleMessages::PanelAct;
    panel.action = 0;
}

}  // namespace swd3
