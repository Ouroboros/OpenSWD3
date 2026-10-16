#include "menu/menu.h"

#include <span>
#include <string>

#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "pic/picture.h"
#include "text/text_input.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr int kFirstRoleFlag = 0x1E;
constexpr int kSwappedPagesFlag = 0x49;
constexpr int kHiddenPointerFlag = 9;
constexpr int kMenuSound = 0xBB;
constexpr int kSlideMaximum = 0x320;
constexpr int kTabGlowMaximum = 0x10;
constexpr int kTabGlowMinimum = 6;
constexpr std::string_view kLevelFormat = "%2d";  // swd32.exe 0x004A6ABC
constexpr std::string_view kRatioFormat = "%4d/%4d";

// A key slot acting once when it goes down.
bool Pressed(const ButtonState& button)
{
    return button.press != 0 && button.heldFrames == 1;
}

// A key slot acting when it goes down and every second frame after the seventh.
bool Repeating(const ButtonState& button, i32 parity)
{
    return button.press != 0 && (button.heldFrames == 1 || ((parity & 1) == 0 && button.heldFrames > 7));
}

}  // namespace

Menu::Menu(Game& game)
    : game(game)
{
}

Menu::~Menu() = default;

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
        this->unk4FC840.action = 1;
        const u32 menu = state & Game::MenuNumberMask;

        if (menu <= Game::ItemChoiceMenu)
        {
            owner.menuMode = 0;
            this->background.id = 0x232A;
            this->background.action = 0x34;
            this->unk4FBA18.id = 0x232A;
            this->unk4FB8D8.id = 0x232A;
            this->unk4FBA18.action = 0x1A;
            this->unk4FB8D8.action = 0x1B;

            for (std::size_t item = 0; item < this->items.size(); ++item)
            {
                this->items[item].id = 0x232A;
                this->items[item].action = static_cast<i32>(8 + item);
            }

            this->unk4FC82C = 0;
            this->frames = 0x1E;

            if ((state & Game::MenuOnItems) != 0)
            {
                this->unk4FC82C = 1;
                this->frames = 0x24;
                this->Open(0x24, 2);
                this->frames = 0x41;
                this->Call(this->pageTable[this->page]);
                this->beforeFrame = nullptr;
                // The original installs the items page's frame (0x004407F0), restored with that page.
                this->frame = &Menu::Nothing;
            }
            else
            {
                this->Open(0x1E, 1);
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
                item.id = 0x232B;
            }

            this->background.action = 0x4E;

            for (std::size_t item = 0; item < this->items.size(); ++item)
            {
                this->items[item].action = static_cast<i32>(0x2C + item);
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

        this->frames = 0x40;
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
    this->unk4FBECC = 5;
    this->page = static_cast<u16>((menuMode - 0x1E) / 6 + 0xB);
    this->mode = static_cast<u16>(menuMode);
    this->unk4FC200 = static_cast<u16>(menuMode);
    this->unk4FBAB8 = static_cast<u16>(menuMode);
    this->SetPage(menuCode);
    this->BuildTabs(this->unk4FBECC);
    owner.controls.buttons = {};
    this->tint = 0;
    const auto shadow = static_cast<Pixel>(owner.painter.MakeColor(6, 4, 3));

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

    this->unk4A6AA8 = 0xFFFF;
    int count = 0;

    for (int role = 0; role < Menu::Tabs; ++role)
    {
        MenuTab& tab = this->tabs[static_cast<std::size_t>(role)];
        tab.role = 0xFFFF;

        if (this->game.state.GameFlag(role + kFirstRoleFlag))
        {
            tab.role = static_cast<u16>(role);
            tab.nameAction = static_cast<u16>(count + 8);
            tab.iconAction = static_cast<u16>(count + 8);
            tab.frameAction = static_cast<u16>(count + 8);
            tab.state = count == selected ? 2 : 1;
            tab.stateCopy = tab.state;
            ++count;
        }
    }

    if (count < Menu::Tabs)
    {
        this->tabs[static_cast<std::size_t>(count)].state = this->tabs[static_cast<std::size_t>(count)].frameAct;
    }
}

// swd32.exe 0x0043B480: the handlers of page `menuCode`
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
        this->escapeDelay = 6;
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
            this->clicks = 1;
        }
        else if (leftButton.press > 1)
        {
            this->clicks = 2;
        }
    }

    const ButtonState& rightButton = keys.At(Button::MouseRight);

    if (rightButton.heldFrames == 1)
    {
        if (rightButton.press == 1)
        {
            this->clicks |= 4;
        }
        else if (rightButton.press > 1)
        {
            this->clicks |= 8;
        }
    }
}

// swd32.exe 0x0043A610: the page drawn over its background (sliding in on the role pages), then the role tabs, the
// gold on page 1, the pointer, a tint, the corner notices, and the frame shown
void Menu::Present()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (this->frames == 0x41)
    {
        this->slide = kSlideMaximum;
    }

    const bool swapped = state.GameFlag(kSwappedPagesFlag);
    const bool doubling =
        (this->page == 0xF && this->code > 1 && swapped) || (this->page == 0xB && this->code >= 0x1F4);

    if (!doubling)
    {
        this->slide >>= 1;
    }

    owner.drawSurface = owner.painter.LockSurface(*owner.offscreen);
    owner.painter.SetTarget(owner.drawSurface);
    owner.painter.UnlockSurface(*owner.offscreen);

    if (this->code != Menu::SystemPage)
    {
        int offset = this->frames < 0x4B ? this->slide : 0;

        if (swapped && this->page == 0xF)
        {
            offset = 0;
        }

        owner.actDrawer.Draw(this->background, offset + 0xB4, 0);
    }

    if (this->code != 1)
    {
        this->Call(this->draw);

        if (owner.menuState == Game::NoMenu)
        {
            return;
        }
    }

    if (owner.menuMode == 0)
    {
        if (this->code != Menu::SystemPage && this->page != 0x11)
        {
            if (doubling)
            {
                if (this->slide == 0)
                {
                    this->slide = 1;
                }

                owner.actDrawer.Draw(this->unk4FC840, -this->slide, 0);
                this->DrawRoleTabs(this->slide);
                this->slide *= 2;

                if (this->slide > kSlideMaximum)
                {
                    this->slide = kSlideMaximum;
                }
            }
            else
            {
                this->DrawPageTitle();
                owner.actDrawer.Draw(this->unk4FC840, -this->slide, 0);
                this->DrawRoleTabs(this->slide);
            }
        }

        if (this->code == 1)
        {
            owner.actDrawer.DrawCounter(0x27C, 0x1CC, static_cast<u32>(owner.party.variables[0]), owner.counterAlpha);
        }

        owner.scene.pointerRole.act.action = 0xD;

        if (!state.GameFlag(kHiddenPointerFlag))
        {
            owner.RefreshCursor();
        }

        if (this->tint != 0)
        {
            owner.painter.TintPixels(std::span<u16>(owner.drawSurface, 0x4B000), this->tint, this->tint, this->tint);
        }

        owner.notices.Draw(owner.smallFont, owner.painter, state, owner.textColors);
        owner.ShowFrame();
    }

    this->lastPage = this->page;
    this->pointerX = state.pointerX;
    this->pointerY = state.pointerY;
}

// swd32.exe 0x0043A880: the title of the role page: glowing in (out on page 1's code) over 8..16 steps; once fully lit
// the plain picture (actions 0x14 and 0x15 or 0x28 and 0x29 swap with flag 0x49)
void Menu::DrawPageTitle()
{
    Game& owner = this->game;
    const bool swapped = owner.state.GameFlag(kSwappedPagesFlag);
    const int last = this->frames == 0x41 ? 0 : this->titleGlow;

    if (this->code == 1)
    {
        this->titleGlow = last + 1 > 0x10 ? 0x10 : last + 1;
    }
    else
    {
        this->titleGlow = last - 1 < 8 ? 8 : last - 1;
    }

    const int spacing = swapped ? 0x46 : 0x50;
    this->unk4FC790.direction = swapped ? 3 : 2;
    owner.actDrawer.DrawGhost(this->unk4FC790, 0, 9, this->titleGlow);
    this->unk4FC790.flags &= 0x80000003u;
    this->unk4FBEE0.id = 0x232A;
    const int x = spacing * (this->page - 0xB) + 0xDC;

    auto swap = [swapped](int action, int first)
    {
        if (!swapped)
        {
            return action;
        }

        return action == first ? first + 1 : action == first + 1 ? first : action;
    };

    if (this->titleGlow == 0x10)
    {
        this->unk4FBEE0.action = swap(this->page + 5, 0x14);
        owner.actDrawer.Draw(this->unk4FBEE0, x, 0xA);
        return;
    }

    this->unk4FBEE0.action = swap(this->page + 0x19, 0x28);

    if (owner.acts.Run(this->unk4FBEE0) == 0)
    {
        return;
    }

    const Picture* picture = owner.archive.GetPicture(this->unk4FBEE0.frame, this->unk4FBEE0.picture);

    if (picture == nullptr)
    {
        return;
    }

    owner.painter.SetSource(picture->data.data());
    DrawOptions& options = owner.painter.Options();
    options.tintBlue = this->titleGlow - 0x10;
    options.tintGreen = this->titleGlow - 0x10;
    options.tintRed = this->titleGlow - 0x10;
    owner.painter.DrawPicture(
        x - this->unk4FBEE0.x,
        0xA - this->unk4FBEE0.y,
        picture->width,
        picture->height,
        this->unk4FBEE0.flags
    );
}

// swd32.exe 0x0043AAA0: the tabs of the roles shifted left by `offset`: the selected one glowing up, with the bars of
// hit points, spirit and stamina, the name, the level with the experience still needed and the three ratios
void Menu::DrawRoleTabs(int offset)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const auto color = static_cast<Pixel>(painter.MakeColor(0x1D, 0x1B, 0x15));

    for (std::size_t index = 0; index < Menu::Tabs; ++index)
    {
        i8& glow = this->tabGlow[index];

        if (this->unk4FBECC == index)
        {
            glow = static_cast<i8>(glow + 3 > kTabGlowMaximum ? kTabGlowMaximum : glow + 3);
        }
        else
        {
            glow = static_cast<i8>(glow - 1 < kTabGlowMinimum ? kTabGlowMinimum : glow - 1);
        }

        if (this->unk4FBECC == 5 || this->code == 1)
        {
            glow = kTabGlowMaximum;
        }

        const MenuTab& tab = this->tabs[index];

        if (tab.role == 0xFFFF)
        {
            continue;
        }

        const RoleRecord& role = owner.party.roles[index];
        const int x = tab.x - offset;
        owner.actDrawer.DrawGhost(this->items[index], x, tab.y, glow);
        painter.SetClipClamped(0, 0, 0x4F * role.hitPoints / role.maxHitPoints + 0x64, 0x1E0);
        owner.actDrawer.DrawGhost(this->unk4FB810, 0x64 - offset, tab.y - 0x27, glow);
        painter.SetClipClamped(0, 0, 0x4F * role.spirit / role.maxSpirit + 0x64, 0x1E0);
        owner.actDrawer.DrawGhost(this->unk4FC450, 0x64 - offset, tab.y - 0x11, glow);
        painter.SetClipClamped(0, 0, 0x4F * role.stamina / role.maxStamina + 0x64, 0x1E0);
        owner.actDrawer.DrawGhost(this->unk4FC518, 0x64 - offset, tab.y + 5, glow);
        owner.mediumFont.DrawText(painter.Target(), x - 0x58, tab.y - 0x5E, owner.party.names[index].data(), color, 4);
        u32 needed = 0;
        owner.database.LoadLevelEntry(static_cast<int>(index) + 1, role.level + 1, needed);
        const std::string level =
            WinFormat(kLevelFormat, {static_cast<i32>(role.level), static_cast<i32>(needed) - role.unk00});
        owner.mediumFont.DrawText(painter.Target(), 0x5C - offset, tab.y - 0x4C, level, color, 4);
        const std::string hitPoints = WinFormat(kRatioFormat, {role.hitPoints, role.maxHitPoints});
        owner.smallFont.DrawText(painter.Target(), 0x6C - offset, tab.y - 0x34, hitPoints, color, 4);
        const std::string spirit = WinFormat(kRatioFormat, {role.spirit, role.maxSpirit});
        owner.smallFont.DrawText(painter.Target(), 0x6C - offset, tab.y - 0x1E, spirit, color, 4);
        const std::string stamina = WinFormat(kRatioFormat, {role.stamina, role.maxStamina});
        owner.smallFont.DrawText(painter.Target(), 0x6C - offset, tab.y - 8, stamina, color, 4);
        painter.SetClipClamped(0, 0, 0x280, 0x1E0);

        if (static_cast<i8>(role.unk24 >> 8) < 0)
        {
            owner.actDrawer.DrawWithMode(this->items[index], x, tab.y, 0x28);
        }
    }
}

// swd32.exe 0x0043BAB0: a menu box: a filled box 8 pixels larger and the frame of act 0x233B
void Menu::DrawBox(int x, int y, int width, int height, int mode, int red, int green, int blue)
{
    Game& owner = this->game;
    FillBox(owner.painter, x - 8, y - 8, width + 0x10, height + 0x10, red, green, blue, static_cast<FillMode>(mode));
    ActState& panel = owner.battleMessages.panelAct;
    panel.id = 0x233B;
    panel.action = 0;
    owner.acts.Run(panel);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        x,
        y,
        x + width,
        y + height,
        0,
        static_cast<int>(0x80000008u)
    );
}

// swd32.exe 0x00439DE0: the animations of the menus at their first actions. The original first fills the handler
// tables of the role pages (0x00444FC0), restored with those pages.
void Menu::InitActs()
{
    Game& owner = this->game;
    this->flags = 0;
    ActState& panel = owner.battleMessages.panelAct;

    for (ActState* state :
         {&this->background,
          &this->unk4FC790,
          &this->unk4FC840,
          &this->unk4FB810,
          &this->unk4FC450,
          &this->unk4FC518,
          &this->unk4FC328,
          &this->unk4FC6E8,
          &this->unk4FBA18,
          &this->unk4FB8D8,
          &this->gauge,
          &this->items[0],
          &this->items[1],
          &this->items[2],
          &this->items[3],
          &this->unk4FBEE0,
          &this->face,
          &panel})
    {
        ResetPlayback(*state);
    }

    this->background.id = 0x232A;
    this->background.action = 0;
    this->unk4FC840.id = 0x232A;
    this->unk4FC840.action = 1;
    this->unk4FC790.id = 0x232A;
    this->unk4FC790.direction = owner.state.GameFlag(kSwappedPagesFlag) ? 3 : 2;

    for (ActState* state :
         {&this->unk4FB810,
          &this->unk4FC450,
          &this->unk4FC518,
          &this->unk4FC328,
          &this->unk4FC6E8,
          &this->unk4FBA18,
          &this->unk4FB8D8,
          &this->face})
    {
        state->id = 0x232A;
    }

    this->unk4FB810.action = 4;
    this->unk4FC450.action = 5;
    this->unk4FC518.action = 6;
    this->unk4FC328.action = 0x18;
    this->unk4FC6E8.action = 0x19;
    this->unk4FBA18.action = 0x1A;
    this->unk4FB8D8.action = 0x1B;

    for (std::size_t item = 0; item < this->items.size(); ++item)
    {
        this->items[item].id = 0x232B;
        this->items[item].action = static_cast<i32>(0x2C + item);
    }

    this->face.action = 3;
    panel.id = 0x233B;
    panel.action = 0;
}

}  // namespace swd3
