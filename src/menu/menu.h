#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"
#include "item/item_list.h"
#include "menu/menu_list.h"
#include "res/act_state.h"

namespace swd3
{

class Game;
class TextInput;

// A role tab of the menus (0x1C bytes, four of them at 0x004A6A38): the role shown (0xFFFF: none), the box and the
// three animations of the tab with their actions.
struct MenuTab
{
    u16 role = 0;         // +0x00
    u16 unk02 = 0;        // +0x02
    u16 unk04 = 0;        // +0x04
    u16 x = 0;            // +0x06
    u16 y = 0;            // +0x08
    u16 unk0A = 0;        // +0x0A
    u16 state = 0;        // +0x0C 1, 2 for the selected tab
    u16 stateCopy = 0;    // +0x0E
    u16 frameAct = 0;     // +0x10
    u16 frameAction = 0;  // +0x12
    u16 iconAct = 0;      // +0x14
    u16 iconAction = 0;   // +0x16
    u16 nameAct = 0;      // +0x18
    u16 nameAction = 0;   // +0x1A
};

// The bestiary (role page 0x11; 0x004FC90C..0x004FCA80): the monsters of mon.dat 1..500 by category, with what the
// party knows of each from how many it defeated.
struct Bestiary
{
    static constexpr int Monsters = 0x1F4;
    static constexpr int Categories = 0xC;
    static constexpr std::size_t Listed = 0x40;
    static constexpr int Rows = 0xF;
    static constexpr std::size_t Lines = 0x10;

    ItemNode monster;                              // 0x004FC918 the monster chosen
    std::array<i8, Monsters + 1> categories = {};  // 0x004FCA78 by monster id (-1: none)
    std::array<u8, Monsters + 1> defeated = {};    // 0x004FCA7C by monster id
    std::array<i8, Listed> known = {};             // 0x004FC930 the defeats of each monster listed
    std::array<std::string, Lines> lines;          // 0x004FC934 the lines of the monster chosen
    std::array<std::string, Listed> names;         // the names of the monsters listed (empty: not defeated)
    std::array<u32, Listed> ids = {};              // 0x004FC91C the monsters listed, ended by 0
    std::size_t firstShown = 0;                    // 0x004FC920 the first listed shown (a pointer in the original)
    i32 top = 0;                                   // 0x004FC90C
    i32 category = 0;                              // 0x004FC910 the tab chosen
    i32 shown = 0;                                 // 0x004FC914
    i32 stamina = 0;                               // 0x004FC924
    i32 row = 0;                                   // 0x004FC928
    i32 spirit = 0;                                // 0x004FC92C
    i32 count = 0;                                 // 0x004FC974 the monsters listed
    u16 scrollArrows = 0;                          // 0x004FCA80
};

// The edges of a list's scroll bar as last drawn (DrawScrollBar), read by the clicks.
struct MenuScrollBar
{
    i32 top = 0;          // +0x00
    i32 thumbBottom = 0;  // +0x04
    i32 thumbTop = 0;     // +0x08
    i32 bottom = 0;       // +0x0C
};

// The menus (the state at 0x004FB000..0x004FDFFF): a page installs the handlers of the frame, the keys and the drawing,
// which the frame dispatches. Handler slots are kept separately: some pages leave a slot of the page before them.
class Menu
{
public:
    using Handler = void (Menu::*)();

    // The code of the system page (the title and the settings).
    static constexpr u16 SystemPage = 0xEA60;
    // The modes of the system page (Open on SystemPage).
    static constexpr int TitleMode = 0;
    static constexpr int GameModeA = 1;
    static constexpr int GameModeB = 2;
    static constexpr int NameEntryMode = 3;

    // The role pages: `page` counts from FirstRolePage as the mode counts from FirstPageMode in PageModeStep steps.
    static constexpr u16 FirstRolePage = 0xB;
    static constexpr u16 ItemsPage = 0xC;
    static constexpr u16 UnkPageF = 0xF;    // drawn without sliding while the pages are swapped (flag 0x49)
    static constexpr u16 UnkPage11 = 0x11;  // drawn without the title and the tabs
    static constexpr std::size_t Pages = 0x12;
    static constexpr int FirstPageMode = 0x1E;
    static constexpr int PageModeStep = 6;
    static constexpr int ItemsPageMode = FirstPageMode + PageModeStep;
    // The codes the role pages are opened with.
    static constexpr u16 MainCode = 1;  // the gold is shown and the title glows in
    static constexpr u16 ItemsCode = 2;
    static constexpr u16 SubpageCodes = 0x1F4;  // codes from here slide the tabs out on the first page
    static constexpr u16 NoTab = 5;             // `unk4FBECC`: no tab raised
    static constexpr u16 NoRole = 0xFFFF;       // MenuTab::role
    static constexpr u16 PlainTab = 1;          // MenuTab::state
    static constexpr u16 RaisedTab = 2;
    static constexpr i8 InitialTabGlow = 12;
    // `Game::menuMode`: where the save slots were opened from.
    static constexpr int NoSlotsMode = 0;
    static constexpr int KeySaveMode = 1;     // F12 or Ctrl+S on the map
    static constexpr int KeyLoadMode = 2;     // Ctrl+L on the map
    static constexpr int TitleLoadMode = 3;   // loading from the title or the launcher
    static constexpr int ScriptSaveMode = 4;  // talk command SaveMenu
    static constexpr int ItemSaveMode = 5;    // the items page (0x004485F0)
    static constexpr int Tabs = 4;
    // swd32.exe 0x004A6A38: the tabs as the executable holds them.
    static constexpr std::array<MenuTab, Tabs> InitialTabs = {{
        {0, 0x17, 0x16, 0xB4, 0x6D, 0, 0, 0, 0x232B, 0, 0x232C, 0, 0x232A, 0},
        {0, 0x17, 0x82, 0xB4, 0xE0, 0, 0, 0, 0x232B, 0, 0x232C, 0, 0x232A, 0},
        {0, 0x17, 0xE7, 0xB4, 0x153, 0, 0, 0, 0x232B, 0, 0x232C, 0, 0x232A, 0},
        {0, 0x17, 0x150, 0xB4, 0x1C6, 0, 0, 0, 0x232B, 0, 0x232C, 0, 0x232A, 0},
    }};

    explicit Menu(Game& game);
    ~Menu();
    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;

    void InitActs();
    void Run();
    void Open(int menuMode, u16 menuCode);
    void BuildTabs(int selected);
    void SetPage(int menuCode);
    void Dispatch();
    void ReadMouse();
    void Present();
    void DrawBox(int x, int y, int width, int height, int mode, int red, int green, int blue);
    void DrawPageTitle();
    void DrawRoleTabs(int offset);
    void ItemsAfterSave(int saved);
    void DrawScrollBar(int x, int y, int height, int arrows, float from, float to, MenuScrollBar& bar);

    // The bestiary (role page 0x11).
    void BestiaryEnter();
    void BestiaryLeave();
    void BestiaryFrame();
    void BestiaryMoved(u16 arrows);
    void BestiaryDown();
    void BestiaryUp();
    void BestiaryPageDown();
    void BestiaryPageUp();
    void BestiaryLeft();
    void BestiaryRight();
    void BestiaryShift();
    void BestiaryCancel();
    void BestiaryDraw();
    void BestiaryList(int category);
    void BestiaryCountShown();
    void DrawBestiaryRow(int index, int row, Pixel color, bool chosen);
    void BestiaryChoose(u32 id);
    std::string KnownValue(std::u8string_view label, int known, int needed, int value, int most);
    void BestiaryLines(int index);
    void DrawBestiaryTabs();
    void OptionsLeave();
    bool DrawNameEntry();
    void ActOnEntry();
    void DrawSettings();

    // The system page (the title in mode 0, the settings).
    void SystemEnter();
    void SystemFrame();
    void SystemDown();
    void SystemUp();
    void SystemLast();
    void SystemFirst();
    void SystemLeft();
    void SystemRight();
    void SystemEnd();
    void SystemSelect();
    void SystemCancel();
    void SystemDraw();

    void Nothing();

    // The handler slots.
    Handler beforeFrame = nullptr;              // 0x004FB808 (none: not called)
    Handler frame = &Menu::Nothing;             // 0x004FC64C
    Handler down = &Menu::Nothing;              // 0x004FC828
    Handler up = &Menu::Nothing;                // 0x004FC4E8
    Handler pageDown = &Menu::Nothing;          // 0x004FBF78
    Handler pageUp = &Menu::Nothing;            // 0x004FBA10
    Handler left = &Menu::Nothing;              // 0x004FBAC0
    Handler right = &Menu::Nothing;             // 0x004FC788
    Handler cancel = &Menu::Nothing;            // 0x004FC838
    Handler shift = &Menu::Nothing;             // 0x004FC83C
    Handler end = &Menu::Nothing;               // 0x004FBF7C
    Handler select = &Menu::Nothing;            // 0x004FC3C0
    Handler draw = &Menu::Nothing;              // 0x004FBEC8
    std::array<Handler, Pages> pageTable = {};  // 0x004FC8B0 the role pages (entries 0xB..0x11)

    u16 code = 0;                  // 0x004FC900 the code the menu was opened with
    u16 page = 0;                  // 0x004FC3C4 (low word) the page index
    u16 mode = 0;                  // 0x004FB8A8
    u16 unk4FC200 = 0;             // 0x004FC200
    u16 unk4FBAB8 = 0;             // 0x004FBAB8
    u16 unk4FBECC = 0;             // 0x004FBECC (low word)
    u16 unk4FC82C = 0;             // 0x004FC82C
    u16 unk4A6AA8 = 0;             // 0x004A6AA8
    i32 tint = 0;                  // 0x004FC320 a screen tint while non-zero
    i32 frames = 0;                // 0x004FC784
    u32 flags = 0;                 // 0x004FBED8
    i32 pointerX = 0;              // 0x004FBAC4 the pointer of the last frame
    i32 pointerY = 0;              // 0x004FBED0
    i32 escapeDelay = 0;           // 0x004FC908 frames before RightControl acts as Escape again
    u32 clicks = 0;                // 0x004FB8AC bits 0..1: a left click (single, double), bits 2..3: a right click
    i32 slide = 0;                 // 0x004FC3C8
    i32 lastPage = FirstRolePage;  // 0x004A6AB0
    i32 titleGlow = 0;             // 0x004FC780
    std::array<i8, Tabs> tabGlow = {InitialTabGlow, InitialTabGlow, InitialTabGlow, InitialTabGlow};  // 0x004A6AAC
    std::array<MenuTab, Tabs> tabs = InitialTabs;                                                     // 0x004A6A38

    ActState background;               // 0x004FB970
    ActState hitPointBar;              // 0x004FB810 the bars of the role tabs
    ActState unk4FB8D8;                // 0x004FB8D8
    ActState unk4FBA18;                // 0x004FBA18
    std::array<ActState, Tabs> items;  // 0x004FBFA0 the four entries of a page
    ActState unk4FBEE0;                // 0x004FBEE0
    ActState face;                     // 0x004FC208 the face of the name entry
    ActState unk4FC328;                // 0x004FC328
    ActState spiritBar;                // 0x004FC450
    ActState staminaBar;               // 0x004FC518
    ActState gauge;                    // 0x004FC650 a step of the settings bars
    ActState unk4FC6E8;                // 0x004FC6E8
    ActState unk4FC790;                // 0x004FC790
    ActState unk4FC840;                // 0x004FC840

    // The system page (0x004FD29C..0x004FD2D4).
    i32 systemClick = 0;           // 0x004FD29C 1: the confirm button clicked, 2: a right click
    i32 nameReply = 0;             // 0x004FD2A0 frames of the face's reply to a name
    i32 textSpeedShown = 0;        // 0x004FD2A4
    i32 systemState = 0;           // 0x004FD2AC 1: the title entries, 2: chosen (5: the settings, 100: in game)
    std::vector<u16> savedScreen;  // 0x004FD2B0 the frame under the page in game
    i32 systemCursor = 0;          // 0x004FD2B4 the title entry
    std::array<i32, 4> glow = {};  // 0x004FD2B8 the glow of the four entries
    i32 systemRow = 0;             // 0x004FD2C8 the settings row, the fade and name entry steps otherwise
    i32 sampleLength = 0;          // 0x004FD2CC characters of the text speed sample shown
    std::vector<u8> enteredNames;  // 0x004FD2D0 the two names typed (0x20 bytes)
    i32 sampleDelay = 0;           // 0x004FD2D4

    // The items page.
    MenuList itemList;        // 0x004FD08C (see MenuList)
    MenuScrollBar scrollBar;  // 0x004FC4F0
    Bestiary bestiary;
    ItemLink itemObjects;        // 0x004FD1E8 the objects of the items page
    ItemLink savedInventory;     // 0x004FD15C the inventory kept while the items page saves
    ItemLink savedItems;         // 0x004FD140 `itemObjects` kept with it
    std::string description;     // 0x004FC2A0 the description of the chosen object
    std::vector<u16> unk4FD318;  // 0x004FD318 the options page's list

private:
    void Call(Handler handler);

    Game& game;
};

}  // namespace swd3
