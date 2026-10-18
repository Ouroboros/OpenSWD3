#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "config/config.h"
#include "core/types.h"
#include "item/item_list.h"
#include "menu/menu_list.h"
#include "org/object_record.h"
#include "party/party.h"
#include "res/act_state.h"

namespace swd3
{

class Game;
class Menu;
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

// The altar page (role page 0x10; 0x004FCA88..0x004FCD20): two objects of the party in the two slots give two
// results, one at each altar; the altar chosen makes its result with a morph of the pictures.
struct AltarPage
{
    static constexpr int Objects = 0x4B0;
    static constexpr int Rows = 0x10;
    static constexpr std::size_t MorphColumns = 0x78;
    static constexpr std::size_t MorphRows = 0xDC;

    ItemNode resultEast;             // 0x004FCA88
    ItemNode resultWest;             // 0x004FCBA0
    ItemNode slotA;                  // 0x004FCBA8
    ItemNode slotB;                  // 0x004FCC58
    std::vector<i32> categories;     // 0x004FCAC4 by object id
    std::vector<i32> levels;         // 0x004FCAB0
    std::vector<u32> kinds;          // 0x004FCA90
    std::vector<i32> altars;         // 0x004FCACC 1: never the east result, 2: never the west one
    ItemLink objects;                // 0x004FCAE0 the objects of the tab, moved out of the inventory
    ItemNode* firstShown = nullptr;  // 0x004FCD10
    i32 count = 0;                   // 0x004FCAD8
    i32 top = 0;                     // 0x004FCAD0
    i32 row = 0;                     // 0x004FCBA4
    i32 shown = 0;                   // 0x004FCB98
    i32 tab = 0;                     // 0x004FCADC 0..2
    i32 slot = 0;                    // 0x004FCAB8 the slot an object goes into
    i32 stage = 0;                   // 0x004FCD20
    i32 chosen = 0;                  // 0x004FCA8C 0: the east altar, 1: the west one
    u32 tooHigh = 0;                 // 0x004FCAAC bit 0: the east result, bit 1: the west one
    i32 hover = 0;                   // 0x004FCAD4 the pointer is on "執行"
    i32 counter = 0;                 // 0x004FCAB4 the morph
    i32 phase = 0;                   // 0x004FCAC8
    u16 scrollArrows = 0;            // 0x004FCAA4
    std::array<i16, 4> vitals = {};  // 0x004FCD14 the spirit and stamina of the east result, then of the west one
    std::vector<i16> columnsA;       // 0x004FCD08 the morph's offsets
    std::vector<i16> columnsB;       // 0x004FCB9C
    std::vector<i16> rowsA;          // 0x004FCAA0
    std::vector<i16> rowsB;          // 0x004FCD0C
    std::vector<u16> frame;          // 0x004FCD1C the pictures of the morph
    std::vector<u16> result;         // 0x004FCB90
    std::vector<u16> pictureA;       // 0x004FCA9C
    std::vector<u16> pictureB;       // 0x004FCA98
};

bool AltarTabTakes(const ItemNode& node, int tab);
std::u8string_view CategoryName(u16 category);

// The figures of a role on the equipment page (0x14 dwords): the vitals and maxima, the attributes, the word at
// +0x20, the two equipment bonuses, then the slot's extra figures (percent, proficiency, fame).
using EquipStats = std::array<i32, 0x14>;

// The equipment page (role page 0xC; 0x004FCD28..0x004FCF90): the eleven slots of a role and the inventory's objects
// that fit the slot chosen, with the stats each would give.
struct EquipPage
{
    ItemLink unk4FCE20;                          // 0x004FCE20 where an object of another kind taken off goes
    ItemNode* inventoryHead = nullptr;           // 0x004FCED8 the inventory's first node when opened
    i32 openedAt = 0;                            // 0x004FCD28 Menu::frames when opened
    i32 slide = 0;                               // 0x004FCD2C the page slides in
    i32 unk4FCD30 = 0;                           // 0x004FCD30
    i32 shown = 0;                               // 0x004FCD34
    i32 slot = 0;                                // 0x004FCD38 0..10
    i32 row = 0;                                 // 0x004FCD3C
    i32 listX = 0;                               // 0x004FCD40
    i32 listY = 0;                               // 0x004FCD44
    void (Menu::*savedBeforeFrame)() = nullptr;  // 0x004FCD48
    RoleRecord worn;                             // 0x004FCD4C the record the stats are computed in
    i32 stage = 0;                               // 0x004FCD54 0: the slots, 1: the list, 5: a description, 0xF: refused
    ItemNode* described = nullptr;               // 0x004FCD58
    i32 unk4FCD5C = 0;                           // 0x004FCD5C
    ItemNode* firstShown = nullptr;              // 0x004FCD64
    u16 scrollArrows = 0;                        // 0x004FCD68
    ItemLink objects;                            // 0x004FCD70 the objects that fit the slot
    i32 top = 0;                                 // 0x004FCF88
    std::array<EquipStats, Party::Roles + 1> stats =
        {};                 // 0x004FCF8C the four roles, then the role with the object tried
    i32 count = 0;          // 0x004FCF90
    i32 previousStage = 0;  // 0x004FCED0 the stage a description returns to
};

// The skills page (role page 0xD; 0x004FCF98..0x004FD080): a role's skills of one tab in two columns, used on the
// party or on one role; the town and exit warps.
struct SkillsPage
{
    ItemLink list;                   // 0x004FCFC0 the skills of the tab, moved out of the role's list
    ItemNode* firstShown = nullptr;  // 0x004FCFBC
    i32 top = 0;                     // 0x004FCF98
    i32 roles = 0;                   // 0x004FCF9C the party roles
    i32 shown = 0;                   // 0x004FCFA0 at most 24
    i32 buttonOffset = 0;            // 0x004FCFA4 -1 while the description button is pressed
    i32 tab = 0;                     // 0x004FCFA8
    i32 target = 0;                  // 0x004FCFAC the role a skill is used on
    i32 tabs = 0;                    // 0x004FCFB0 the tabs of the target role (3..5)
    i32 descriptionSlide = 0;        // 0x004FCFB4
    i32 count = 0;                   // 0x004FCFB8
    i32 row = 0;                     // 0x004FD070
    i32 shownTabs = 0;               // 0x004FD074 always 3
    i32 stage = 0;                   // 0x004FD078
    i32 descriptionTop = 0;          // 0x004FD07C
    u16 scrollArrows = 0;            // 0x004FD080 the list's (bits 0..7) and the towns' (bits 8..15)
    SkillScript skill;               // 0x004BAB74 the skill loaded last
};

// A town of the town warp (0x004FB8D0 holds them, six bytes and the name).
struct MenuTown
{
    u16 map = 0;
    u16 x = 0;
    u16 y = 0;
    std::string name;
};

// The status page (role page 0xE; 0x004FD2D8 and 0x004FD2DC): a role's record with its equipment.
struct StatusPage
{
    RoleRecord bonus;  // 0x004FD2D8 what the equipment gives
    RoleRecord total;  // 0x004FD2DC the record with it
};

// The system tab (role page 0xF; 0x004FD2E0..0x004FD3A8): save, load, the notes, the settings with the key setup, quit.
struct OptionsPage
{
    i32 entry = 0;     // 0x004FD310 the tab: save, load, notes, settings, quit
    i32 stage = 0;     // 0x004FD2F8 0: the tabs, 1: a tab opened, 2: confirming, 5..7: the key setup, 8: display
    i32 row = 0;       // 0x004FD30C the row of the settings or of the quit box
    i32 locked = 0;    // 0x004FD3A8 the quit fade runs
    i32 choice = 0;    // 0x004FD3A4 the confirmation's choice, then the fade's frames from 0x1E
    i32 quitKind = 0;  // 0x004FD2EC 1: end the game, 2: restart
    i32 keyRow = 0;    // 0x004FD2F0 the key row (0x10..0x12: the buttons)
    std::array<i32, Config::Keys> keys = {};  // 0x004FD324 the keys being set
    i32 heldKey = 0;                          // 0x004FD314 a key down since the last one taken
    i32 swappedKey = 0;                       // 0x004FD2E8 the key a swap took from another row (kept, never read)
    std::vector<u16> notes;                   // 0x004FD318 the numbers of the notes
    i32 noteCount = 0;                        // 0x004FD320
    i32 noteTop = 0;                          // 0x004FD2F4
    i32 notesShown = 0;                       // 0x004FD31C
    i32 noteRow = 0;                          // 0x004FD2E4 (kept, never read)
    std::size_t noteAt = 0;                   // 0x004FD2FC (kept, never read)
    u32 arrows = 0;          // 0x004FD2E0 frames the scroll arrows stay pressed (bits 0..3 up, 4..7 down)
    i32 sampleLength = 0;    // 0x004FD300 characters of the speed sample shown
    i32 textSpeedShown = 0;  // 0x004FD304
    i32 sampleDelay = 0;     // 0x004FD308
    i32 displayRow = 0;      // the rebuild's display setup (stage 8): the row chosen
};

// The items page (role page 0xB; 0x004FD088..0x004FD164): the inventory of one category with its counts, used, sorted
// or discarded.
struct InventoryPage
{
    static constexpr i32 Unk4FC3D0 = 0x60;
    static constexpr i32 Unk4FC3DC = 2;
    static constexpr u16 FirstRowNumber = 0x64;

    i32 openedAt = 0;         // 0x004FD088 the frame before the page opened
    i32 category = 0;         // 0x004FD094 0..6
    i32 unk4FD160 = 0;        // 0x004FD160 (only cleared)
    i32 roles = 0;            // 0x004FD09C the party roles
    i32 target = 0;           // 0x004FD0A0 the role an object is used on
    i32 followers = 0;        // 0x004FD090 the followers that can be called
    i32 followerRow = 0;      // 0x004FD0A4
    i32 followerPresent = 0;  // 0x004FD154 the follower chosen is called
    i32 discardChoice = 0;    // 0x004FD144 0: yes, 1: no
    i32 unk4FD148 = 0;        // 0x004FD148 the top of the role box below its first row, the follower box's choice
    i32 glow = 0;             // 0x004FD14C the mode tab drawn raised (0x23..0x26 after sorting)
    u16 arrows = 0;           // 0x004FD158 frames the arrows stay pressed: the list's (bits 0..7), the towns'
    u16 unk4FD1F0 = 0;        // 0x004FD1F0 (only cleared)
    i32 unk4FC3D0 = 0;        // 0x004FC3D0 (only set)
    i32 unk4FC3D4 = 0;        // 0x004FC3D4
    i32 unk4FC3DC = 0;        // 0x004FC3DC
    std::array<u16, 14> unk4A6A00 = {};  // 0x004A6A00 the rows shown numbered from 100, ended by 0 (never read)
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
    static constexpr u16 EquipmentPage = 0xC;
    static constexpr u16 UnkPageF = 0xF;    // drawn without sliding while the pages are swapped (flag 0x49)
    static constexpr u16 UnkPage11 = 0x11;  // drawn without the title and the tabs
    static constexpr std::size_t Pages = 0x12;
    static constexpr int FirstPageMode = 0x1E;
    static constexpr int PageModeStep = 6;
    static constexpr int EquipmentMode = FirstPageMode + PageModeStep;
    // The codes the role pages are opened with.
    static constexpr u16 MainCode = 1;          // the gold is shown and the title glows in
    static constexpr u16 PageCode = 2;          // a role page open
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
    void BestiaryShowFrom(int top);
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

    // The altar page (role page 0x10).
    void AltarEnter();
    void AltarLeave();
    void AltarFillTab();
    void AltarReturnObjects();
    void AltarTakeObjects(ItemLink& source, int tab);
    void AltarSortObjects();
    void AltarPick();
    void AltarPut(ItemNode& slot, int index);
    bool AltarCombine();
    void AltarMoved(u16 arrows);
    void AltarChooseEast(bool soundFirst);
    void AltarChooseWest();
    void AltarDown();
    void AltarUp();
    void AltarPageDown();
    void AltarPageUp();
    void AltarTabChanged();
    void AltarLeft();
    void AltarRight();
    void AltarSwitch(u16 slotSound);
    void AltarShift();
    void AltarEnd();
    void AltarSelect();
    void AltarSpend();
    void AltarCancel();
    void AltarFrame();
    void AltarDraw();
    void AltarDrawList(Pixel color);
    void AltarDrawResults(Pixel color);
    void AltarDrawResult(const ItemNode& result, std::u8string_view title, int x, int y, u32 flags);
    void AltarPrepareMorph();
    void AltarMorph();
    void AltarFinishMorph();
    void AltarResultVitals();

    // The equipment page (role page 0xC).
    void EquipEnter();
    void EquipLeave();
    void EquipDescribeSlot();
    void EquipDescribeRow();
    ItemNode* EquipSlotNode();
    void EquipTake(ItemLink& source, int slot);
    void EquipCountShown();
    void EquipFillList();
    void EquipReturn();
    void EquipComputeStats();
    const ItemNode* EquipTried();
    void EquipRoleStats(int role, EquipStats& stats);
    void EquipTryStats(int role, int slot, EquipStats& stats, const ItemNode* tried);
    void EquipStatsOf(EquipStats& stats);
    void EquipExtraStats(const ItemNode* tried, EquipStats& stats);
    void EquipSwapVitals(const ItemNode& added, const ItemNode& removed);
    void EquipListMoved(u16 arrows);
    void EquipSlotMoved();
    void EquipDown();
    void EquipUp();
    void EquipPageDown();
    void EquipPageUp();
    void EquipLeft();
    void EquipRight();
    void EquipShift();
    void EquipSelect();
    void EquipPutOn();
    void EquipCancel();
    void EquipFrame();
    void EquipDraw();
    void EquipDrawSlots(int x, int y, int slide, int width, int chosen);
    void EquipDrawPicture(u16 file, u16 picture, int x, int y);
    void EquipDrawStats(int slot, int role);
    void DrawDescriptionBox(std::string_view text);

    // The skills page (role page 0xD).
    void SkillsEnter();
    void SkillsLeave();
    void SkillsDescribe();
    void SkillsTake(ItemLink& source, int tab);
    void SkillsCountShown();
    void SkillsFillList();
    void SkillsReturn();
    void SkillsCountTabs();
    void SkillsMoved(u16 arrows, bool sound);
    int SkillsNextRole(int role, int step) const;
    void SkillsDown();
    void SkillsUp();
    void SkillsPageDown();
    void SkillsPageUp();
    void SkillsOtherColumn(u16 arrows);
    void SkillsLeft();
    void SkillsRight();
    void SkillsShift();
    void SkillsEnd();
    void SkillsSelect();
    void SkillsCancel();
    void SkillsFrame();
    void SkillsDraw();
    void SkillsDrawTowns(Pixel color);
    std::size_t FindExitWarp(int map) const;
    void ListTowns();
    int SkillsShownTabs(int role) const;
    void Warp(u16 map, u16 x, u16 y, i16 event);

    // The status page (role page 0xE).
    void StatusEnter();
    void StatusLeave();
    void StatusCompute();
    void StatusFrame();
    void StatusNext();
    void StatusPrevious();
    void StatusCancel();
    void StatusDrawBonus(int bonus, int x, int y, int value);
    void StatusDraw();

    // The system tab (role page 0xF).
    // The key slot of each row of the key setup (swd32.exe 0x004A7210).
    static constexpr std::array<u16, 16> KeyRowSlots = {4, 6, 3, 5, 0xC, 1, 9, 0, 0xA, 2, 0x13, 7, 8, 0x10, 0x11, 0x12};
    void OptionsEnter();
    void CountNotes();
    void FindNote(int index);
    void DrawNote(int index, int x, int y);
    void OptionsFrame();
    void OptionsDown();
    void OptionsUp();
    void OptionsPageDown();
    void OptionsPageUp();
    void OptionsLeft();
    void OptionsRight();
    void OptionsSelect();
    void OptionsCancel();
    void OptionsDrawSettings(Pixel color);
    void OptionsDrawKeys(Pixel color);
    void OptionsDrawDisplay(Pixel color);
    void OptionsStepDisplay(i32 row, bool up, bool wrap);
    void OptionsDraw();

    // The items page (role page 0xB).
    void ItemsDescribe();
    void ItemsFit();
    void ItemsMoved(u16 arrows);
    void ItemsEnter();
    void ItemsLeave();
    void ItemsFrame();
    void ItemsDown();
    void ItemsUp();
    void ItemsPageDown();
    void ItemsPageUp();
    void ItemsCategory(int step);
    void ItemsLeft();
    void ItemsRight();
    void ItemsShift();
    void ItemsEnd();
    void ItemsFreeTowns();
    void ItemsUse(ItemNode& node);
    void ItemsSelect();
    void ItemsCancel();
    void ItemsDrawNotice(Pixel color);
    void ItemsDrawFollowers(Pixel color, std::string& line);
    void ItemsDrawTowns(Pixel color);
    void ItemsDraw();
    void ItemsCountShown();
    void ItemsTake(int category);
    void ItemsFill();
    void ItemsReturn();
    bool ItemsCallFollower();
    void ItemsSave();

    // The main page (the titles of the role pages) and the role page tables.
    void ShowPageBackground(int page);
    void FillPageTables();
    void MainFrame();
    void MainLeft();
    void MainRight();
    void MainSelect();
    void MainCancel();
    void MainDraw();
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
    Handler beforeFrame = nullptr;               // 0x004FB808 (none: not called)
    Handler frame = &Menu::Nothing;              // 0x004FC64C
    Handler down = &Menu::Nothing;               // 0x004FC828
    Handler up = &Menu::Nothing;                 // 0x004FC4E8
    Handler pageDown = &Menu::Nothing;           // 0x004FBF78
    Handler pageUp = &Menu::Nothing;             // 0x004FBA10
    Handler left = &Menu::Nothing;               // 0x004FBAC0
    Handler right = &Menu::Nothing;              // 0x004FC788
    Handler cancel = &Menu::Nothing;             // 0x004FC838
    Handler shift = &Menu::Nothing;              // 0x004FC83C
    Handler end = &Menu::Nothing;                // 0x004FBF7C
    Handler select = &Menu::Nothing;             // 0x004FC3C0
    Handler draw = &Menu::Nothing;               // 0x004FBEC8
    std::array<Handler, Pages> enterTable = {};  // 0x004FC8B0 the role pages opened (entries 0xB..0x11)
    std::array<Handler, Pages> leaveTable = {};  // 0x004FB884 left
    std::array<Handler, Pages> drawTable = {};   // 0x004FBF54 drawn

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
    AltarPage altar;
    EquipPage equip;
    SkillsPage skills;
    StatusPage status;
    std::vector<MenuTown> towns;  // 0x004FB8D0 (its count at 0x004FC834)
    i32 townTop = 0;              // 0x004FBED4
    i32 townRow = 0;              // 0x004FC510
    i32 townShown = 0;            // 0x004FB8D4
    MenuScrollBar townBar;        // 0x004FC500
    MenuScrollBar unk4FBAB4;      // 0x004FBAB4 a second bar of the equipment list (only drawn)
    i32 descriptionSlide = 0;     // 0x004FC648 the description box slides (up while positive)
    i32 descriptionTop = 0x1E0;   // 0x004FC830 its top
    i32 unk4FC904 = 0;            // 0x004FC904 -1 while the description button is pressed
    ActState unk4FD0A8;           // 0x004FD0A8 the description button
    ItemLink itemObjects;         // 0x004FD1E8 the objects of the items page
    ItemLink savedInventory;      // 0x004FD15C the inventory kept while the items page saves
    ItemLink savedItems;          // 0x004FD140 `itemObjects` kept with it
    std::string description;      // 0x004FC2A0 the description of the chosen object
    OptionsPage options;
    InventoryPage itemsPage;

private:
    void Call(Handler handler);

    Game& game;
};

}  // namespace swd3
