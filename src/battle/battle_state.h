#pragma once

#include <array>
#include <memory>
#include <string_view>

#include "battle/battle_file.h"
#include "battle/battle_hud.h"
#include "battle/battle_unit.h"
#include "core/types.h"
#include "game/color_fade.h"
#include "item/item_list.h"
#include "org/level.h"
#include "party/party.h"
#include "pic/picture.h"
#include "pic/projector.h"
#include "platform/surface.h"

namespace swd3
{

// A record of 0x1C bytes, eighteen of them from 0x00524788 (cleared by the battle start): a unit with a kind of 1 or
// 2 (AddSlotRecord), the free records after the used ones.
struct BattleSlotRecord
{
    i32 unit = -1;  // +0x00 -1: a free record
    i32 unk04 = 0;  // +0x04
    u16 kind = 0;   // +0x08
    u16 unk0A = 0;  // +0x0A
    i32 unk0C = 0;  // +0x0C
    i32 unk10 = 0;  // +0x10
    i32 unk14 = 0;  // +0x14
    i32 unk18 = 0;  // +0x18
};

// The order a party unit chose (0x14 bytes, ten at 0x00520E90), taken into its slot record by InsertSlotRecord.
struct PendingOrder
{
    i32 unk00 = 0;  // +0x00 to the record's unk0C
    i32 unk04 = 0;  // +0x04 to unk14
    i32 unk08 = 0;  // +0x08 to unk18
    u16 unk0C = 0;  // +0x0C to unk0A
    u16 unk0E = 0;
    i32 unk10 = 0;  // +0x10 to unk04
};

// The state of the battle and of the shop and item lists it shares with the menus.
class BattleState
{
public:
    static constexpr i32 InitialGaugeFull = 900;
    static constexpr int QueueRows = 4;
    static constexpr int QueueColumns = 10;
    static constexpr int Enemies = static_cast<int>(BattleFormation::MaxEnemies);
    static constexpr int RoleUnits = 10;               // the party roles and the guests
    static constexpr int Units = Enemies + RoleUnits;  // the enemies, then the roles
    static constexpr int FirstRoleUnit = Enemies;
    static constexpr int Guests = 8;
    static constexpr std::size_t LootSlots = 10;  // as BattleMessages::LootSlots
    static constexpr std::size_t TextBytes = 64;
    static constexpr u16 DefaultTimingWindow = 0x3C;
    static constexpr int FieldHeight = 0x180;  // the rows of the battle field above the panels
    static constexpr int BandRow = 0xC0;       // the grey band opens from this row
    static constexpr u16 BandFull = 0xC0;
    static constexpr u16 BandFastFrom = 0x14;  // below it the band doubles a frame, from it it grows by BandStep
    static constexpr u16 BandStep = 0x16;
    static constexpr i8 FlashStart = 0x10;
    static constexpr i8 FlashStep = 4;
    static constexpr i16 MaxDarkLevel = 2;
    static constexpr i32 DarkLevelFrames = 1;  // darkFrames past it step to the next level
    static constexpr std::array<u8, 10> QuakeSteps = {4, 3, 4, 3, 4, 3, 2, 3, 2, 3};  // 0x004A75EC by quakePhase
    static constexpr std::array<i32, 5> RoleCycle = {11, 2, 1, 0, 3};  // 0x004A796C the party roles by turn, from 1
    static constexpr std::array<i32, 4> PanelUnits = {10, 9, 8, 11};   // 0x004A7960 the role units by panel
    // 0x004A7534: the five commands of the menu (pointers in the original)
    static constexpr std::array<std::u8string_view, 5> FixedCommandNames =
        {u8"攻擊", u8"奇術", u8"物品", u8"絕招", u8"防禦"};
    static constexpr std::u8string_view SpiritPactName = u8"靈契";  // 0x004A79A0 the command of battle role 0
    // 0x004A74A0 [0x15..0x29]: the names of the accessory commands by kind (pointers in the original)
    static constexpr std::array<std::u8string_view, 21> AccessoryCommandNames = {
        u8"連擊", u8"陰陽", u8"偷竊", u8"群攻", u8"控制", u8"命中", u8"投擲", u8"隱形", u8"斷玉", u8"煉符", u8"鼠槌",
        u8"護罩", u8"分解", u8"氣合", u8"冥想", u8"運功", u8"攻擊", u8"奇術", u8"物品", u8"絕招", u8"防禦",
    };

    // BattleState::mode bits.
    enum Mode : i32
    {
        SellMode = 1,  // prices at 60 percent
        UnkMode2 = 2,
        ListModes = SellMode | UnkMode2,
    };

    // BattleState::panelFlags bits.
    enum PanelFlag : i32
    {
        UnkPanel2 = 2,        // set when game flag 0xC9 is on at the battle start
        UnkPanel8 = 8,        // the battle ends with unk53BDC8 0 even when lost (CheckBattleEnd)
        UnkPanel100 = 0x100,  // the screen quakes all the time (QuakeFrame)
        ResistancePanel = 0x20,
        NoRetreat = 0x200,  // the party cannot retreat
    };

    // Bits of spellStage.
    enum SpellStage : u16
    {
        SpellOver = 0x1,     // the spell is over (MagicFrame ends)
        SpellCast = 0x8000,  // the magic act followed up: the spell takes effect
    };

    void TakePendingAction();
    void SortDrawOrder();
    void ListEnemyOrder();
    void AddSlotRecord(int kind, i32 unit);
    void RemoveSlotRecord(i32 unit);
    void InsertSlotRecord(int kind, i32 unit, int at);
    void StartFieldFade(int red, int green, int blue, int toRed, int toGreen, int toBlue, int frames);
    void WipeSlotRecords();
    void SortDrawOrderAround(i32 acting, i32 target);
    void SetReturnedItem(int index, u16 id);
    void TakeSlotRecordAt(std::size_t at, BattleSlotRecord& taken);

    i32 listBCount = 0;                              // 0x004FD3B0 entries of listB, at most 13
    ItemLink listB;                                  // 0x004FD4C0
    i32 mode = 0;                                    // 0x004FD578 bits 0..1: the shop and sell modes of the item lists
    ItemLink items;                                  // 0x004FD580 the sorted item list
    u16 itemCursor = 0;                              // 0x004FD588
    i32 gaugeFull = InitialGaugeFull;                // 0x004A74CC the full action gauge
    std::array<i16, 3> darkCopyTint = {-1, -1, -1};  // 0x004A7632 the darkRed, darkGreen, darkBlue of the copies
    i32 criticalAttack =
        0;  // 0x0053CEB8 1: a role's critical attack is under way (it cannot be evaded, acts add effects)
    u8 attacksSinceCritical = 0;  // 0x0053CEB4 (low byte) the attacks since the last critical one
    i32 lastCriticalRole = 0;     // 0x0053CEBC the act of the role with the last rolled critical attack
    i32 fadingField = 0;          // 0x0053C030 1: an act started the field fade (the background waits for it)
    u8 evadeStreak = 0;           // 0x0053D028 failed evasion rolls in a row
    u8 evadeJustSucceeded = 0;    // 0x0053D029
    std::array<std::unique_ptr<Surface>, 2> surfaces;            // 0x005244C4 screen sized off-screen surfaces
    std::array<i16, QueueRows * QueueColumns> actionQueue = {};  // 0x0052022C 4 rows of 10, taken by TakePendingAction
    i32 panelFlags = 0;                                          // 0x0053BC24 bit 5: the resistance panel
    i16 pendingActionRow = 0;                                    // 0x0053BF1E
    i16 pendingAction = 0;                                       // 0x0053BF20
    i32 paused = 0;                                              // 0x0053C00C messages do not slide while set
    std::array<char, TextBytes> text = {};                       // 0x0053D02C the formatted text of the battle messages

    // Set by the battle start (0x00451B10).
    u16 spellStage = 0;                   // 0x0053C050 BattleState::SpellStage bits of the running spell
    u16 battle = 0;                       // 0x0053C052 the battle number
    BattleFile file;                      // 0x004FF5B8 battle.ffd
    BattleFormation formation;            // 0x004FF1E0
    Projector projector;                  // 0x0053B0B8
    Picture background;                   // 0x00502940 the picture of all_map2.tsw
    BattleAnimation backgroundAnimation;  // 0x004FDFA8
    u32 enemyCount = 0;                   // 0x0053BCE0
    u32 roleCount = 0;                    // 0x0053BCE4 the party and the guests in battle
    u16 animated = 0;                     // 0x0053BDDC the background is animated
    u16 scene = 0;                        // 0x0053BF10 1..4 at random
    std::array<i32, Party::Roles> partyRoles = {0, 1, 2, 3};  // 0x004A75C8 the party role of each battle role
    std::array<u8, Party::Roles> inParty = {};        // 0x00521398 the four roles are in the party (story flags 0x1E..)
    std::array<u8, Guests> guestUsed = {};            // 0x004FDF80
    u16 guestCount = 0;                               // 0x0053BF0C
    u8 defeatedRoles = 0;                             // 0x0053BEFF the party roles entering defeated
    i32 result = 0;                                   // 0x0053BCEC 0x67: the battle is lost before it starts
    i32 mirrored = 0;                                 // 0x0053C000 the sides are swapped
    u16 timingWindow = DefaultTimingWindow;           // 0x004A762A
    u16 lightColor = 0;                               // 0x004FF104
    u16 darkColor = 0;                                // 0x005240BC
    i32 pointerX = 0;                                 // 0x005028A0
    i32 pointerY = 0;                                 // 0x005028A4
    std::array<UnitPlacement, Enemies> enemyPlaces;   // 0x005213A0
    std::array<UnitPlacement, RoleUnits> rolePlaces;  // 0x0053AF70
    std::array<i32, 2 * Party::Roles> panelPlaces = {};  // 0x004FF558 the x and y of the four role panels
    std::array<i32, Party::Roles> unk4FF578 = {};
    std::array<i32, RoleUnits> hitPointBars = {};  // 0x00520200 and its copy 0x00502914
    std::array<i32, RoleUnits> hitPointBarsShown = {};
    std::array<i32, RoleUnits> hitPoints = {};   // 0x0052027C
    std::array<i32, RoleUnits> spiritBars = {};  // 0x005240C8 and its copy 0x00524288
    std::array<i32, RoleUnits> spiritBarsShown = {};
    std::array<i32, RoleUnits> spirit = {};       // 0x005201D8
    std::array<i32, RoleUnits> staminaBars = {};  // 0x00524164 and its copy 0x00524490
    std::array<i32, RoleUnits> staminaBarsShown = {};
    std::array<i32, RoleUnits> stamina = {};      // 0x00520DA8
    std::array<i32, RoleUnits> blockValues = {};  // 0x005242FC and its copy 0x0052418C
    std::array<i32, RoleUnits> blockValuesShown = {};
    std::array<i32, Units> unitY = {};         // 0x00520E40 the y of each unit
    std::array<i32, Units> drawOrder = {};     // 0x005214AC the units from the top down
    std::array<i32, Units> drawTaken = {};     // 0x00520D5C
    std::array<i32, Enemies> enemyOrder = {};  // 0x00520DF8 the enemies from the top down
    ActState unk525470;
    ObjectRecord unk5246E0;
    std::array<i32, 0x14> unk4FF168 = {};
    std::array<i32, 0x3C> unk524324 = {};
    std::array<i32, 0xA> unk4FE5D4 = {};
    std::array<std::u8string_view, 3> commandNames;  // 0x004FF0B0 the accessory commands of the menu (pointers)
    std::array<u16, 3> commandKinds = {};            // 0x004FE5CC their accessory kinds
    std::array<i32, 8> unk524268 = {};
    std::array<u8, 8> commandEnabled = {};  // 0x00524414 the five commands of the menu and the three accessory ones
    std::array<i32, Party::Roles> unk52544C = {};            // per party role: the scroll of its list (ScrollListBack)
    std::array<PendingOrder, RoleUnits> pendingOrders = {};  // 0x00520E90
    i32 unk53BF80 = 0;  // cleared when a party unit's order is taken (InsertSlotRecord)
    i32 unk53BFD0 = 0;
    i16 unk53BF46 = 0;  // cleared when a stagger ends (HurtFrame)
    i16 unk53BF48 = 0;
    u16 unk53BF28 = 0;  // non-zero: an enemy does not choose a command with UnkAction1000 (ChooseCommand)
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

    // The background drawn each frame (0x00453580): a quake shift, the grey band of the battle start, a flash of the
    // field, and the field darkened (by darkened copies in `surfaces` or by tint steps) while unit unk4A7630 acts.
    i32 shake = 0;       // 0x0053BD5C the shift of the background this frame, cleared once drawn
    i32 quaking = 0;     // 0x0053C008 1: the screen quakes (read by 0x00453200, shaken by 0x0045E7D0)
    i32 quakePhase = 0;  // 0x0053BD60 the step of the quake in its offset table (0x004A75EC)
    i32 unk53BF94 = 0;   // with unk53BF98: the field is darkened while unit unk4A7630 acts
    i32 unk53BF98 = 0;
    i16 unk4A7630 = -1;  // compared with unk53AE70
    u16 unk53BF2C = 0;
    u16 grayBand = 0;       // 0x0053BF2E the half height of the grey band around BandRow
    i32 grayBandShown = 0;  // 0x0053BF30
    i32 flashing = 0;       // 0x0053BFCC the field flashes, flashLevel falling by FlashStep a frame
    u16 unk4A75FC = 1;
    i8 flashLevel = FlashStart;  // 0x004A75FE tint steps of the flash
    u8 panelGlow = 1;            // 0x004A75FF the highlight of the current role's panel; bit 7: falling
    i32 darkenByTint = 0;        // 0x0053C4A0 the field is darkened by tint steps instead of the darkened copies
    i16 darkLevel = 0;           // 0x0053BF44 0..MaxDarkLevel
    i16 darkRed = 0;             // 0x0053BF36 the tint steps of a darkened field per level, halved
    i16 darkGreen = 0;           // 0x0053BF38
    i16 darkBlue = 0;            // 0x0053BF3A
    i32 darkFrames = 0;          // 0x00520E88 frames counted to the next darker level
    i32 unk53BFEC = 0;           // the darkening is undone a level a frame
    i32 unk53C030 = 0;
    i32 unk4A7574 = -1;  // unk53BD0C when the darkened copies were made
    i32 unk53BD0C = 0;

    // The role panels (0x00459D10): the gauges move a step a frame towards their values and the numbers roll.
    i32 currentUnit = 0;                               // 0x0053BD54 the unit whose panel is highlighted
    i32 glowFrames = 0;                                // 0x0053BD64 frames to the next step of panelGlow
    i8 listGlow = 0;                                   // 0x0053BDF1 the glow of the last listed unit; bit 7: on
    std::array<i32, RoleUnits> nameFlashes = {};       // 0x00520F58
    std::array<i32, RoleUnits> hitPointsRolling = {};  // 0x004FF1B8 the change still rolling in the shown number
    std::array<i32, RoleUnits> hitPointSteps = {};     // 0x00520E18
    std::array<i32, RoleUnits> spiritRolling = {};     // 0x0052102C
    std::array<i32, RoleUnits> spiritSteps = {};       // 0x0053AE48
    std::array<i32, RoleUnits> staminaRolling = {};    // 0x00520FCC
    std::array<i32, RoleUnits> staminaSteps = {};      // 0x005240F0
    i32 autoBattle = 0;                                // 0x0053C018
    i32 autoBoxX = 0;                                  // 0x0053BDD0 the "自動攻擊" box slides in
    i32 autoBoxStep = 0;                               // 0x0053BDCC
    // 0x00520FB8 frames; levels 0x004FDFA4, 0x00521394, 0x00525468; targets 0x004FDF8C, 0x00521388, 0x00525448;
    // steps 0x0052151C, 0x00525430, 0x00520D58.
    ColorFade fieldFade;

    // The damage of the hit being dealt (0x004539B0, 0x0045D690).
    i32 hitDamage = 0;     // 0x0052441C
    i16 spiritDamage = 0;  // 0x0053B0B0
    u16 unk4FDF78 = 0;
    i16 staminaDamage = 0;     // 0x004FDF7A
    i32 lastDamage = 0;        // 0x0053AE8C the damage of the last hit with the proficiency bonus
    i16 proficiencyBonus = 0;  // 0x00521520 the weapon proficiency step of the last strike

    // The state of the turns (0x004539B0, 0x00456680 and the commands).
    i32 unk53BFC0 = 0;
    i32 unk53BFC4 = 0;
    i32 unk53BF5C = 0;
    i32 unk53BD50 = 0;
    i32 unk53BFE4 = 0;
    u8 unk53BF00 = 0;  // set to enemyCount by a retreat
    u8 unk53BF01 = 0;
    u8 unk53BF02 = 0;
    u8 unk53BF03 = 0;
    // The turns (0x00464CC0, 0x0045ADF0, 0x0045AA00).
    i32 unk53BCE8 = 0;
    i32 unk53BF60 = 0;
    i32 unk53BF64 = 0;
    i32 unk53BF74 = 0;
    i32 unk53C010 = 0;                          // set when a role turns to another enemy (RetargetRoles)
    std::array<i32, Units> unk53AF38 = {};      // per unit, set when it takes the turn
    std::array<i32, RoleUnits> unk520DD0 = {};  // role units in order, a defeated one taken out
    i32 unk4A754C = 1;                          // the enemy a role turn aims at, 1-based
    u16 unk4A7626 = 0xFFFF;
    // The rewards of the defeated enemies (RemoveDefeated).
    u16 unk53BF12 = 0;
    u16 unk53BF14 = 0;
    u16 unk53BF16 = 0;
    const ObjectRecord* unk5028FC = nullptr;  // the object of the last defeated enemy
    // The hit acts of the units (0x004582B0, 0x00458DE0, PlayHitAct, FadeHitAct).
    std::array<ActState, Units> unk5202A8;  // the hit act of each unit
    std::array<ActState, Units> unk4FE600;  // the follow act of each unit
    std::array<ActState, Units> unk524980;  // the fading act of each unit
    std::array<i8, Units> unk521374 = {};   // the fade of unk524980
    std::array<u16, Units> unk4FDF94 = {};  // the calls of WalkLine
    // The draw order around the acting unit (SortDrawOrderAround).
    u8 unk53C4C0 = 0;
    i32 unk53AE7C = 0;
    i32 unk53AE80 = 0;
    i32 unk53C040 = 0;  // set once the order is sorted around the acting unit
    // The push of a hit (Game::PushUnits).
    i16 unk4A7638 = 0x1A4;  // the push, halved each frame
    u16 unk53BDE8 = 0;      // frames since the push was halved
    i16 unk53AE7A = 0;      // the hit act picture after which the units slide back
    i32 unk53BD68 = 0;      // frames of a quake step
    i32 unk53AEDC = 0;      // the digits of the extra damage number (BattleUnit::DrawDamageNumbers)
    i32 unk53AEE0 = 0;
    i32 unk53AEE4 = 0;
    // The turns of the units and the lists (NextEnemyTurn, NextRoleTurn, ScrollListBack).
    i32 unk53BCF4 = 0;  // the enemy taking the turn
    i32 unk4A7550 = 1;  // 1-based step through enemyOrder or RoleCycle
    i32 unk53BFB8 = 0;
    i32 unk53BFD8 = 0;  // set when a unit's damage numbers are done (ApplyHits)
    i32 unk53BD9C = 0;
    i32 unk4A7558 = 1;
    i32 unk4A755C = 1;
    i32 unk53BD04 = 0;  // the scroll of a list
    i32 unk53BD08 = 0;
    i32 unk4A7570 = 2;                        // a party role
    u8 unk53BDF2 = 0;                         // the entries of the list of state 2
    u16 unk53BDF4 = 0;                        // the entries of the lists of states 4 and 0x1B
    i32 unk53BD14 = 0;                        // the list entry under the cursor
    i32 unk53BD10 = 0;                        // the skill under the cursor of the 絕招 list
    u8 unk53BDF3 = 0;                         // the skills of the 絕招 list
    i32 unk53BCF8 = 0;                        // the row under the cursor of the 佈置護駕 panel
    std::array<u16, LootSlots> lootIds = {};  // 0x004FF2F0 the objects of the loot list
    i32 unk53C4C4 = 0;                        // set when an accessory gains proficiency at the victory
    i32 unk53BD20 = 0;
    RoleRecord unk4FF108;  // the record of the role before its level-up
    i32 unk53C4C8 = 0;
    std::array<i16, 9> statRolls = {};  // 0x005214A4..0x00525466 the rises of a level-up still to count
    u16 unk53BDEA = 0;                  // the frames of the level-up stats (the high word of 0x0053BDE8)
    i32 unk4A7564 = -1;                 // the enemy the auto battle aims at (1-based), -1: none
    u16 unk4A7644 = 0xFFFF;             // a role unit that may take the panel in any battle state (SelectRoleUnit)
    i32 unk4A7548 = 1;                  // the command chosen in the menu (1-based)
    i32 unk53BD18 = 0;                  // the page of the list of state 2 (0..2)
    // The menu (MenuCancel).
    i32 unk53BFB0 = 0;
    i32 unk53BD90 = 0;
    i32 unk53BD94 = 0;
    i32 unk53BD98 = 0;
    std::array<char, 0x14> unk53C184 = {};  // a name shown by the lists
    i32 unk53C49C = 0;
    i32 unk53BFC8 = 0;
    u16 unk4A762C = 0xFFFF;
    u16 unk4A762E = 0xFFFF;  // the role with a battle talk (0xFFFF: none)
    i32 unk53BD44 = 0;
    i32 unk53BD48 = 0;
    i32 unk53C02C = 0;
    i32 unk53BF88 = 0;
    i32 unk53BFF0 = 0;
    i32 unk53BCF0 = 0;  // the menu command a list returns to
    i32 unk4A7560 = 1;
    i32 unk4A7568 = 2;
    i32 unk4A756C = 1;
    // The proficiency filled in the battle (AnnounceMastery).
    ObjectRecord unk53BC28;
    LearnedSkill lastLearned;  // 0x0053C154 the name of the accessory or of the skill learned, for the messages
    i32 unk53BFFC = 0;
    u8 unk4A7646 = 0xFF;  // the role
    // The end of the battle (Game::CheckBattleEnd, Game::DrawBattlePrompt).
    i32 unk53C4A8 = 0;  // the fade of the field to black, 2 darker a frame
    i32 unk53BDC8 = 0;
    std::array<u16, 3> unk4FF2EA = {};  // [1], [2]: objects given back when the battle ends (set by 0x0045EC60)
    i32 unk53BDA8 = 0;                  // frames of the prompt
    i32 unk53BDAC = 0;
    i32 unk53BFBC = 0;
    i32 unk4A7640 = 1;
    i32 unk53BF58 = 0;
    i16 unk53BF50 = 0;
    i16 unk53BF52 = 0;
};

}  // namespace swd3
