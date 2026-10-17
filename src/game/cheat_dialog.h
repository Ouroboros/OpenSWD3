#pragma once

#include <array>
#include <string>
#include <string_view>

#include "core/types.h"
#include "item/item_list.h"

struct SDL_Window;

namespace swd3
{

class Database;
class Game;
class Party;

// The cheat dialog of the debug keys (F4; dialog 0x6A of swd32.exe): ten tabs (the items, the skills of the four
// roles, the attributes of the four roles, the variables) with a list and three fields to add, top up, delete or set
// entries. On Windows it is the original's dialog from swd32.exe's resources; elsewhere it cannot be shown. Also the
// name dialog (0x70) the launcher's start command would open.
class CheatDialog
{
public:
    // The tabs (CheatDialog::tab).
    static constexpr int ItemsTab = 0;
    static constexpr int FirstSkillsTab = 1;
    static constexpr int FirstAttributesTab = 5;
    static constexpr int VariablesTab = 9;
    static constexpr int Tabs = 10;
    // The labels of the attributes (swd32.exe 0x0049FC14, eight bytes each).
    static constexpr std::array<std::u8string_view, 17> AttributeNames = {
        u8"生命",
        u8"魔法",
        u8"體力",
        u8"大生命",
        u8"大魔法",
        u8"大體力",
        u8"力量",
        u8"耐力",
        u8"智慧",
        u8"速度",
        u8"默契",
        u8"反應力",
        u8"運氣",
        u8"閃躲",
        u8"經驗",
        u8"總經驗",
        u8"等級",
    };

    explicit CheatDialog(Game& game);

    // DialogBoxParamA(0x6A): the dialog until it is closed; its result (0 when it could not be shown).
    int Run();
    // DialogBoxParamA(0x70): the name of the first role asked for; 1 when one was given.
    int RunNameDialog();

    // The value column of a row (part of 0x00410490 and 0x00410600): "" without a value, the value alone without a
    // maximum (its percent sign written and then overwritten), "value/maximum" over a maximum of 1, the text before it
    // (the id) for a maximum of 1.
    static std::string ValueText(i32 value, i32 maximum, const std::string& before);
    // The counter shown for inventory object `node` (part of 0x00410730): its percent, its proficiency and maximum
    // or its counter by its kind; -1 for none.
    void ObjectCounter(const ItemNode& node, i32& value, i32& maximum) const;
    // The counters of an inventory object set from the value field (part of 0x0040F890).
    void SetObjectCounter(const ItemNode& node, u16 id, i32 value);
    // The list of the tab (the inventory or a role's skills after its "無" head); nullptr for the other tabs.
    ItemLink* TabList() const;
    SDL_Window* Window() const;

    Game& game;
    Party& party;
    Database& database;
    i32& running;
    const std::u8string& installDirectory;
    int tab = ItemsTab;  // 0x004CC2C8
};

}  // namespace swd3
