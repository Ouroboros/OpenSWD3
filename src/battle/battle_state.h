#pragma once

#include <array>
#include <memory>

#include "battle/battle_file.h"
#include "battle/battle_hud.h"
#include "battle/battle_unit.h"
#include "core/types.h"
#include "item/item_list.h"
#include "pic/picture.h"
#include "pic/projector.h"
#include "platform/surface.h"

namespace swd3
{

// A record of 0x1C bytes, eighteen of them from 0x00524788 (cleared by the battle start).
struct BattleSlotRecord
{
    i32 unk00 = -1;                // +0x00
    std::array<u8, 6> unk04 = {};  // +0x04
    u16 unk0A = 0;                 // +0x0A
    i32 unk0C = 0;                 // +0x0C
    i32 unk10 = 0;                 // +0x10
    i32 unk14 = 0;                 // +0x14
    i32 unk18 = 0;                 // +0x18
};

// The state of the battle and of the shop and item lists it shares with the menus.
class BattleState
{
public:
    static constexpr i32 InitialGaugeFull = 900;
    static constexpr int QueueRows = 4;
    static constexpr int QueueColumns = 10;

    void TakePendingAction();
    void SortDrawOrder();
    void ListEnemyOrder();

    i32 listBCount = 0;                           // 0x004FD3B0 entries of listB, at most 13
    ItemLink listB;                               // 0x004FD4C0
    i32 mode = 0;                                 // 0x004FD578 bits 0..1: the shop and sell modes of the item lists
    ItemLink items;                               // 0x004FD580 the sorted item list
    u16 itemCursor = 0;                           // 0x004FD588
    i32 gaugeFull = InitialGaugeFull;             // 0x004A74CC the full action gauge
    std::array<i16, 3> unk4A7632 = {-1, -1, -1};  // 0x004A7632
    i32 evadeDisabled = 0;                        // 0x0053CEB8
    u8 evadeStreak = 0;                           // 0x0053D028 failed evasion rolls in a row
    u8 evadeJustSucceeded = 0;                    // 0x0053D029
    std::array<std::unique_ptr<Surface>, 2> surfaces;            // 0x005244C4 screen sized off-screen surfaces
    std::array<i16, QueueRows * QueueColumns> actionQueue = {};  // 0x0052022C 4 rows of 10, taken by TakePendingAction
    i32 panelFlags = 0;                                          // 0x0053BC24 bit 5: the resistance panel
    i16 pendingActionRow = 0;                                    // 0x0053BF1E
    i16 pendingAction = 0;                                       // 0x0053BF20
    i32 paused = 0;                                              // 0x0053C00C messages do not slide while set
    std::array<char, 64> text = {};                              // 0x0053D02C the formatted text of the battle messages

    // Set by the battle start (0x00451B10).
    static constexpr int Units = 0x12;  // eight enemies, then the party from 8
    static constexpr int FirstRoleUnit = 8;
    u16 battle = 0;                                // 0x0053C052 the battle number
    BattleFile file;                               // 0x004FF5B8 battle.ffd
    BattleFormation formation;                     // 0x004FF1E0
    Projector projector;                           // 0x0053B0B8
    Picture background;                            // 0x00502940 the picture of all_map2.tsw
    BattleAnimation backgroundAnimation;           // 0x004FDFA8
    u32 enemyCount = 0;                            // 0x0053BCE0
    u32 roleCount = 0;                             // 0x0053BCE4 the party and the guests in battle
    u16 animated = 0;                              // 0x0053BDDC the background is animated
    u16 scene = 0;                                 // 0x0053BF10 1..4 at random
    std::array<i32, 4> partyRoles = {0, 1, 2, 3};  // 0x004A75C8 the party role of each battle role
    std::array<u8, 4> inParty = {};                // 0x00521398 the four roles are in the party (story flags 0x1E..)
    std::array<u8, 8> guestUsed = {};              // 0x004FDF80
    u16 guestCount = 0;                            // 0x0053BF0C
    u8 defeatedRoles = 0;                          // 0x0053BEFF the party roles entering defeated
    i32 result = 0;                                // 0x0053BCEC 0x67: the battle is lost before it starts
    i32 mirrored = 0;                              // 0x0053C000 the sides are swapped
    u16 timingWindow = 0x3C;                       // 0x004A762A
    u16 lightColor = 0;                            // 0x004FF104
    u16 darkColor = 0;                             // 0x005240BC
    i32 pointerX = 0;                              // 0x005028A0
    i32 pointerY = 0;                              // 0x005028A4
    std::array<UnitPlacement, 8> enemyPlaces;      // 0x005213A0
    std::array<UnitPlacement, 10> rolePlaces;      // 0x0053AF70
    std::array<i32, 8> panelPlaces = {};           // 0x004FF558 the x and y of the four role panels
    std::array<i32, 4> unk4FF578 = {};
    std::array<i32, 10> hitPointBars = {};  // 0x00520200 and its copy 0x00502914
    std::array<i32, 10> hitPointBarsShown = {};
    std::array<i32, 10> hitPoints = {};   // 0x0052027C
    std::array<i32, 10> spiritBars = {};  // 0x005240C8 and its copy 0x00524288
    std::array<i32, 10> spiritBarsShown = {};
    std::array<i32, 10> spirit = {};       // 0x005201D8
    std::array<i32, 10> staminaBars = {};  // 0x00524164 and its copy 0x00524490
    std::array<i32, 10> staminaBarsShown = {};
    std::array<i32, 10> stamina = {};      // 0x00520DA8
    std::array<i32, 10> blockValues = {};  // 0x005242FC and its copy 0x0052418C
    std::array<i32, 10> blockValuesShown = {};
    std::array<i32, Units> unitY = {};      // 0x00520E40 the y of each unit
    std::array<i32, Units> drawOrder = {};  // 0x005214AC the units from the top down
    std::array<i32, Units> drawTaken = {};  // 0x00520D5C
    std::array<i32, 8> enemyOrder = {};     // 0x00520DF8 the enemies from the top down
    ActState unk525470;
    ObjectRecord unk5246E0;
    std::array<i32, 0x14> unk4FF168 = {};
    std::array<i32, 0x3C> unk524324 = {};
    std::array<i32, 0xA> unk4FE5D4 = {};
    i32 unk4FF0B0 = 0;
    i32 unk4FE5CC = 0;
    i32 unk4FF0B4 = 0;
    u16 unk4FE5D0 = 0;
    i32 unk4FF0B8 = 0;
    std::array<i32, 8> unk524268 = {};
    i32 unk524414 = 0;
    i32 unk52544C = 0;
    std::array<i32, 0x32> unk520E90 = {};
    std::array<i32, 0x12> unk4FF0BC = {};
    i32 unk525450 = 0;
    i32 unk525454 = 0;
    i32 unk525458 = 0;
    i32 unk502944 = 0;
    i32 unk502948 = 0;
    i32 unk50294C = 0;
    i32 unk502950 = 0;
    i32 unk5244D8 = 0;
    i32 unk5244DC = 0;
    std::array<i32, 0x12> unk5242B0 = {};
    i32 unk524418 = 0;
    std::array<i32, 0x12> unk502984 = {};
    std::array<i32, 0x12> unk524420 = {};
    std::array<i32, 0x12> unk53AE90 = {};
    std::array<i32, 0x7E> unk5244E8 = {};
    i32 unk53C048 = 0;
    i32 unk53AE70 = 0;
    u16 unk53BF22 = 0;
    i32 unk53C4B0 = 0;
    std::array<BattleSlotRecord, Units> unk524788;
    u16 unk53BF26 = 0;  // the high word of 0x0053BF24
};

}  // namespace swd3
