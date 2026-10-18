#include "game/game.h"
#include "menu/menu.h"

namespace swd3
{

namespace
{

constexpr int kSwappedPagesFlag = GameState::SwappedPages;
constexpr u16 kLastPage = 0xF;  // the main page's titles run from the first role page to here
constexpr u16 kLastSwappedPage = 0x10;
constexpr u16 kBestiaryPage = 0x11;         // keeps the background of the page before it
constexpr i32 kFirstPageBackground = 0x29;  // Menu::background shows page p with action p + this
constexpr i32 kAltarBackground = 0x38;      // the two the swapped pages exchange
constexpr i32 kOptionsBackground = 0x39;
constexpr int kTitleSound = 0x8B;
constexpr int kPageSound = 0xBB;
constexpr u32 kMouseSuppressed = 0xC;  // Controls::mouseSuppressed after a right click goes back

// The main page's titles (the mouse).
constexpr int kTitlesLeft = 0xDC;
constexpr int kTitlesRight = 0x258;
constexpr int kSwappedTitlesRight = 0x276;
constexpr int kTitleShift = 3;  // eight pixels a step, ten steps a title
constexpr int kSwappedTitleStep = 7;
constexpr int kTitleSteps = 0xA;
constexpr int kTitleGap = 8;  // the steps past this are the gap between titles
constexpr u32 kTitlesTop = 0xA;
constexpr u32 kTitlesBottom = 0x2A;
constexpr u32 kLeftClick = 1;
constexpr u32 kRightClicks = 0xC;
constexpr u16 kFirstClickedPage = 0xC;  // the title clicked counts from here (MainLeft steps it back)

}  // namespace

// The background of role page `page`; with the pages swapped (flag 0x49) the altar and the system tab exchange theirs
// (inline in the original)
void Menu::ShowPageBackground(int page)
{
    this->background.action = page + kFirstPageBackground;

    if (!this->game.state.GameFlag(kSwappedPagesFlag))
    {
        return;
    }

    if (this->background.action == kAltarBackground)
    {
        this->background.action = kOptionsBackground;
    }
    else if (this->background.action == kOptionsBackground)
    {
        this->background.action = kAltarBackground;
    }
}

// swd32.exe 0x00444FC0: the opening, leaving and drawing handlers of the role pages 0xB..0x11; with the pages swapped
// (flag 0x49) pages 0xF and 0x10 exchange the system tab and the altar
void Menu::FillPageTables()
{
    this->drawTable[FirstRolePage] = &Menu::ItemsDraw;
    this->drawTable[FirstRolePage + 1] = &Menu::EquipDraw;
    this->drawTable[FirstRolePage + 2] = &Menu::SkillsDraw;
    this->drawTable[FirstRolePage + 3] = &Menu::StatusDraw;
    this->drawTable[FirstRolePage + 4] = &Menu::OptionsDraw;
    this->drawTable[FirstRolePage + 5] = &Menu::AltarDraw;
    this->drawTable[FirstRolePage + 6] = &Menu::BestiaryDraw;
    this->enterTable[FirstRolePage] = &Menu::ItemsEnter;
    this->enterTable[FirstRolePage + 1] = &Menu::EquipEnter;
    this->enterTable[FirstRolePage + 2] = &Menu::SkillsEnter;
    this->enterTable[FirstRolePage + 3] = &Menu::StatusEnter;
    this->enterTable[FirstRolePage + 4] = &Menu::OptionsEnter;
    this->enterTable[FirstRolePage + 5] = &Menu::AltarEnter;
    this->enterTable[FirstRolePage + 6] = &Menu::BestiaryEnter;
    this->leaveTable[FirstRolePage] = &Menu::ItemsLeave;
    this->leaveTable[FirstRolePage + 1] = &Menu::EquipLeave;
    this->leaveTable[FirstRolePage + 2] = &Menu::SkillsLeave;
    this->leaveTable[FirstRolePage + 3] = &Menu::StatusLeave;
    this->leaveTable[FirstRolePage + 4] = &Menu::OptionsLeave;
    this->leaveTable[FirstRolePage + 5] = &Menu::AltarLeave;
    this->leaveTable[FirstRolePage + 6] = &Menu::BestiaryLeave;

    if (this->game.state.GameFlag(kSwappedPagesFlag))
    {
        this->drawTable[FirstRolePage + 5] = &Menu::OptionsDraw;
        this->drawTable[FirstRolePage + 4] = &Menu::AltarDraw;
        this->enterTable[FirstRolePage + 5] = &Menu::OptionsEnter;
        this->enterTable[FirstRolePage + 4] = &Menu::AltarEnter;
        this->leaveTable[FirstRolePage + 5] = &Menu::OptionsLeave;
        this->leaveTable[FirstRolePage + 4] = &Menu::AltarLeave;
    }
}

// swd32.exe 0x004450E0: the mouse on the role pages' titles (run before every frame of them): a click on a title opens
// its page (the page open first left, except the system tab with the pages swapped, which only the main page leaves);
// a right click on the main page closes the menu
void Menu::MainFrame()
{
    Game& owner = this->game;
    const int x = owner.state.pointerX;
    const auto y = static_cast<u32>(owner.state.pointerY);
    u32 step = static_cast<u32>(x - kTitlesLeft) >> kTitleShift;
    int right = kTitlesRight;
    u32 within = step % kTitleSteps;

    if (owner.state.GameFlag(kSwappedPagesFlag))
    {
        right = kSwappedTitlesRight;
        step = static_cast<u32>(x - kTitlesLeft) / kSwappedTitleStep;
        within = step % kTitleSteps;
    }

    if (static_cast<u32>(x) <= static_cast<u32>(kTitlesLeft) || static_cast<u32>(x) >= static_cast<u32>(right))
    {
        return;
    }

    if (y < kTitlesBottom && y > kTitlesTop && (this->clicks & kLeftClick) != 0 && within > 0 && within < kTitleGap)
    {
        if (!owner.state.GameFlag(kSwappedPagesFlag) || this->page != kLastPage)
        {
            if (this->code != MainCode)
            {
                this->Call(this->leaveTable[this->page]);
            }
        }
        else if (this->code != MainCode)
        {
            return;
        }

        this->page = static_cast<u16>(step / kTitleSteps + kFirstClickedPage);
        this->MainLeft();
        this->MainSelect();
        return;
    }

    if ((this->clicks & kRightClicks) != 0 && this->code == MainCode)
    {
        owner.controls.mouseSuppressed = static_cast<i32>(kMouseSuppressed);
        this->MainCancel();
    }
}

// swd32.exe 0x00445210: left on the main page: the title before (from the first role page on)
void Menu::MainLeft()
{
    Game& owner = this->game;
    u16 page = static_cast<u16>(this->page - 1);
    this->page = page;

    if (this->page <= FirstRolePage - 1)
    {
        page = FirstRolePage;
        this->page = FirstRolePage;
    }

    this->mode = static_cast<u16>(PageModeStep * page - (PageModeStep * FirstRolePage - FirstPageMode));
    this->unk4FC200 = this->mode;
    this->ShowPageBackground(page);
    owner.audio.PlaySound(kTitleSound, owner.config.soundVolume);
}

// swd32.exe 0x004452B0: right on the main page: the next title (up to page 0xF, 0x10 with the pages swapped)
void Menu::MainRight()
{
    Game& owner = this->game;
    const u16 last = owner.state.GameFlag(kSwappedPagesFlag) ? kLastSwappedPage : kLastPage;
    u16 page = static_cast<u16>(this->page + 1);
    this->page = page;

    if (this->page > last)
    {
        page = last;
        this->page = last;
    }

    this->mode = static_cast<u16>(PageModeStep * page - (PageModeStep * FirstRolePage - FirstPageMode));
    this->unk4FC200 = this->mode;
    this->ShowPageBackground(page);
    owner.audio.PlaySound(kTitleSound, owner.config.soundVolume);
}

// swd32.exe 0x00445360: Enter on the main page: the page of the title opens (its background, its handlers, its
// opening handler)
void Menu::MainSelect()
{
    Game& owner = this->game;
    this->code = PageCode;

    if (this->page != kBestiaryPage)
    {
        this->ShowPageBackground(this->page);
    }

    this->SetPage(this->code);
    this->Call(this->enterTable[this->page]);
    owner.audio.PlaySound(kPageSound, owner.config.soundVolume);
}

// swd32.exe 0x004453F0: Escape on the main page: the menu closes
void Menu::MainCancel()
{
    if (--this->code == 0)
    {
        this->game.menuState = Game::NoMenu;
    }

    this->SetPage(this->code);
}

// swd32.exe 0x00445420: the main page: the page of the title chosen drawn
void Menu::MainDraw()
{
    this->Call(this->drawTable[this->page]);
}

}  // namespace swd3
