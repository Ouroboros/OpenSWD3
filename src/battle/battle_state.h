#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
#include "res/act_state.h"

namespace swd3
{

struct ClickRegion;

// A record of 0x1C bytes, eighteen of them from 0x00524788 (cleared by the battle start): a unit with a kind of 1 or
// 2 (AddSlotRecord), the free records after the used ones.
struct BattleSlotRecord
{
    i32 unit = -1;        // +0x00 -1: a free record
    i32 targetCount = 0;  // +0x04 the units its order hits (BattleState::sweepTotal)
    u16 kind = 0;         // +0x08
    u16 unk0A = 0;        // +0x0A
    i32 atParty = 0;      // +0x0C 1: its order is at the party (PendingOrder::atParty)
    i32 unk10 = 0;        // +0x10
    i32 targetsAll = 0;   // +0x14 1: its order hits every unit of its side (PendingOrder::targetsAll)
    i32 allAtOnce = 0;    // +0x18 1: all of them at once (PendingOrder::allAtOnce)
};

// The order a party unit chose (0x14 bytes, ten at 0x00520E90), taken into its slot record by InsertSlotRecord.
struct PendingOrder
{
    i32 atParty = 0;     // +0x00 1: at the party (the role aimed at, or every role with targetsAll), else the enemies
    i32 targetsAll = 0;  // +0x04 1: every unit of the side (BattleUnit::TargetsAll)
    i32 allAtOnce = 0;   // +0x08 1: hit at once, not one after another (BattleUnit::AllAtOnce)
    u16 unk0C = 0;       // +0x0C to unk0A
    u16 unk0E = 0;
    i32 targetCount = 0;  // +0x10 the units it hits, counted as they are gathered (to targetCount)
};

// What an object bought in the shop changes for a party role (0x10 bytes, four of them at 0x004FD3C8): the attack,
// defence (each with its equipment bonus) and agility against the equipment worn, and whether the role can use it.
struct ShopComparison
{
    i32 attack = 0;   // +0x00
    i32 defense = 0;  // +0x04
    i32 agility = 0;  // +0x08
    i32 usable = 0;   // +0x0C
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
    static constexpr std::array<int, 4> PortraitX = {0x14A, 0xB8, 0x26, 0x1DC};  // 0x004A75A8 by party role
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
        UnkPanel4 = 4,        // an escort was sent back (ConfirmEscort)
        UnkPanel8 = 8,        // the battle ends with frameResult 0 even when lost (CheckBattleEnd)
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
    void ClearActing();
    u16 MenuKindWord(std::size_t index) const;

    i32 listBCount = 0;         // 0x004FD3B0 the rows of the list shown from listB, at most 13
    ItemNode* listB = nullptr;  // 0x004FD4C0 the first item shown (itemsHead itself until the list is made)
    i32 mode = 0;               // 0x004FD578 bits 0..1: the shop and sell modes of the item lists
    ItemNode itemsHead;         // 0x004FD580 the node heading the sorted item list (its count at 0x004FD588)

    // The shop (talk command ItemMenu, Game::ItemChoiceMenu).
    i32 shopStage = 0;    // 0x004FD414 0: closed, 1: buy, sell or leave, 2: the list, 3: the total, 4: equip now?, 0xA:
                          // too little money
    u32 shopFlags = 0;    // 0x004FD410 bit 1: the list was opened (its sound played), bit 2: equipment was bought
    i32 listTop = 0;      // 0x004FD420 the first item shown
    i32 listRow = 0;      // 0x004FD630 the row chosen
    i32 itemCount = 0;    // 0x004FD408 the items of the list
    i16 lessPressed = 0;  // 0x004FD3B4 frames the minus arrow of the row chosen shows pressed
    i16 morePressed = 0;  // 0x004FD40C the plus arrow
    std::array<i16, Party::Roles> faceSlides = {};              // 0x004FD3B8 the roles' faces slide out (8..16)
    std::array<ShopComparison, Party::Roles> comparisons = {};  // 0x004FD3C8
    u16 scrollArrows = 0;              // 0x004FD418 frames the scroll arrows show pressed (bits 0..3: up, 4..7: down)
    std::vector<u16> shopScreen;       // 0x004FD41C the darkened grey map under the shop
    std::vector<u16> shopScreenSaved;  // 0x004FD57C a copy of it
    ActState shopTitle;                // 0x004FD428 act 0x232A, action 0x16
    i32 gaugeFull = InitialGaugeFull;  // 0x004A74CC the full action gauge
    std::array<i16, 3> darkCopyTint = {-1, -1, -1};  // 0x004A7632 the darkRed, darkGreen, darkBlue of the copies
    i32 criticalAttack = 0;                          // 0x0053CEB8 1: a role's critical attack is under way (not evaded)
    u8 attacksSinceCritical = 0;                     // 0x0053CEB4 (low byte) the attacks since the last critical one
    i32 lastCriticalRole = 0;   // 0x0053CEBC the act of the role with the last rolled critical attack
    i32 fadingField = 0;        // 0x0053C030 1: an act started the field fade (the background waits for it)
    u8 evadeStreak = 0;         // 0x0053D028 failed evasion rolls in a row
    u8 evadeJustSucceeded = 0;  // 0x0053D029
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
    BattleFile file{&this->projector};    // 0x004FF5B8 battle.ffd
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
    std::array<i32, Party::Roles> roleListRows = {};     // 0x004FF578 by role: its roleListRow
    std::array<i32, RoleUnits> hitPointBars = {};        // 0x00520200 and its copy 0x00502914
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
    std::array<i32, Party::Roles> roleListScrolls =  // 0x0052544C by role: its roleListScroll
        {};  // 0x0052544C..0x00525458 per party role: the scroll of its list (ScrollListBack)
    std::array<PendingOrder, RoleUnits> pendingOrders = {};  // 0x00520E90
    i32 unk53BF80 = 0;     // cleared when a party unit's order is taken (InsertSlotRecord)
    i32 gathersParty = 0;  // 0x0053BFD0 1: a sweep order chosen at the party (cleared by any queued order or action)
    i16 unk53BF46 = 0;     // cleared when a stagger ends (HurtFrame)
    i16 unk53BF48 = 0;
    u16 unk53BF28 = 0;  // non-zero: an enemy does not choose a command with UnkAction1000 (ChooseCommand)
    std::array<i32, 0x12> unk4FF0BC = {};
    i32 unk502944 = 0;
    i32 unk502948 = 0;
    i32 unk50294C = 0;
    i32 unk502950 = 0;
    i32 unk5244D8 = 0;
    i32 unk5244DC = 0;
    std::array<i32, 0x12> slotDamage = {};  // 0x005242B0 per slot: the damage of the hit so far
    std::array<i32, 0x12> unk502984 = {};
    std::array<i32, 0x12> slotTargets = {};  // 0x00524420 per slot: the unit it hits (-1: none)
    std::array<i32, 0x12> unk53AE90 = {};
    std::array<i32, 0x7E> unk5244E8 = {};
    i32 unk53C048 = 0;
    i32 actingUnit = 0;      // 0x0053AE70 the unit acting now (-1: none)
    u16 listedCommands = 0;  // 0x0053BF22 the commands the menu lists beyond its fixed ones
    i32 unk53C4B0 = 0;
    std::array<BattleSlotRecord, Units> slotRecords;  // 0x00524788 the BattleSlotRecord of each slot
    u16 escortCount = 0;                              // 0x0053BF26 the high word of 0x0053BF24

    // The background drawn each frame (0x00453580): a quake shift, the grey band of the battle start, a flash of the
    // field, and the field darkened (by darkened copies in `surfaces` or by tint steps) while unit darkenUnit acts.
    i32 shake = 0;       // 0x0053BD5C the shift of the background this frame, cleared once drawn
    i32 quaking = 0;     // 0x0053C008 1: the screen quakes (read by 0x00453200, shaken by 0x0045E7D0)
    i32 quakePhase = 0;  // 0x0053BD60 the step of the quake in its offset table (0x004A75EC)
    i32 unk53BF94 = 0;   // with unk53BF98: the field is darkened while unit darkenUnit acts
    i32 unk53BF98 = 0;
    i16 darkenUnit = -1;  // 0x004A7630 compared with actingUnit
    u16 unk53BF2C = 0;
    u16 grayBand = 0;            // 0x0053BF2E the half height of the grey band around BandRow
    i32 grayBandShown = 0;       // 0x0053BF30
    i32 flashing = 0;            // 0x0053BFCC the field flashes, flashLevel falling by FlashStep a frame
    u16 sweptUnits = 1;          // 0x004A75FC the units a sweep has reached
    i8 flashLevel = FlashStart;  // 0x004A75FE tint steps of the flash
    u8 panelGlow = 1;            // 0x004A75FF the highlight of the current role's panel; bit 7: falling
    i32 darkenByTint = 0;        // 0x0053C4A0 the field is darkened by tint steps instead of the darkened copies
    i16 darkLevel = 0;           // 0x0053BF44 0..MaxDarkLevel
    i16 darkRed = 0;             // 0x0053BF36 the tint steps of a darkened field per level, halved
    i16 darkGreen = 0;           // 0x0053BF38
    i16 darkBlue = 0;            // 0x0053BF3A
    i32 darkFrames = 0;          // 0x00520E88 frames counted to the next darker level
    i32 undarkening = 0;         // 0x0053BFEC the darkening is undone a level a frame
    i32 unk53C030 = 0;
    i32 darkenedAt = -1;  // 0x004A7574 unk53BD0C when the darkened copies were made
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
    i32 turnUnit = 0;    // 0x0053BD50 the role unit taking the turn
    i32 endFade = 0;     // 0x0053BFE4 1: the field fades out once the battle is decided (CheckBattleEnd)
    u8 enemiesGone = 0;  // 0x0053BF00 set to enemyCount by a retreat
    u8 unk53BF01 = 0;
    u8 unk53BF02 = 0;
    u8 unk53BF03 = 0;
    // The turns (0x00464CC0, 0x0045ADF0, 0x0045AA00).
    i32 turnCommand = 0;  // 0x0053BCE8 the order of the turn under way
    i32 unk53BF60 = 0;
    i32 roleActing = 0;  // 0x0053BF64 1: a party role is acting (RoleActFrame)
    i32 unk53BF74 = 0;
    i32 retargeted = 0;                         // 0x0053C010 set when a role turns to another enemy (RetargetRoles)
    std::array<i32, Units> turnTaken = {};      // 0x0053AF38 per unit, set when it takes the turn
    std::array<i32, RoleUnits> roleOrder = {};  // 0x00520DD0 role units in order, a defeated one taken out
    i32 aimTarget = 1;                          // 0x004A754C the enemy a role turn aims at, 1-based
    u16 unk4A7626 = 0xFFFF;
    // The rewards of the defeated enemies (RemoveDefeated).
    u16 unk53BF12 = 0;
    u16 unk53BF14 = 0;
    u16 unk53BF16 = 0;
    const ObjectRecord* lastDefeatedObject = nullptr;  // 0x005028FC the object of the last defeated enemy
    // The hit acts of the units (0x004582B0, 0x00458DE0, PlayHitAct, FadeHitAct).
    std::array<ActState, Units> hitActs;        // 0x005202A8 the hit act of each unit
    std::array<ActState, Units> followActs;     // 0x004FE600 the follow act of each unit
    std::array<ActState, Units> fadeActs;       // 0x00524980 the fading act of each unit
    std::array<i8, Units> fadeActLevels = {};   // 0x00521374 the fade of fadeActs
    std::array<u16, Units> walkLineCalls = {};  // 0x004FDF94 the calls of WalkLine
    // The draw order around the acting unit (SortDrawOrderAround).
    u8 unk53C4C0 = 0;
    i32 atParty = 0;  // 0x0053AE7C the acting unit's slot record: atParty
    i32 unk53AE80 = 0;
    i32 orderSorted = 0;  // 0x0053C040 set once the order is sorted around the acting unit
    // The push of a hit (Game::PushUnits).
    i16 hitPush = 0x1A4;       // 0x004A7638 the push, halved each frame
    u16 pushFrames = 0;        // 0x0053BDE8 frames since the push was halved
    i16 slideBackPicture = 0;  // 0x0053AE7A the hit act picture after which the units slide back
    i32 quakeStepFrames = 0;   // 0x0053BD68 frames of a quake step
    i32 unk53AEDC = 0;         // the digits of the extra damage number (BattleUnit::DrawDamageNumbers)
    i32 unk53AEE0 = 0;
    i32 unk53AEE4 = 0;
    // The turns of the units and the lists (NextEnemyTurn, NextRoleTurn, ScrollListBack).
    i32 actingEnemy = 0;  // 0x0053BCF4 the enemy taking the turn
    i32 orderStep = 1;    // 0x004A7550 1-based step through enemyOrder or RoleCycle
    i32 choiceReady = 0;  // 0x0053BFB8 1: a choice is marked (by a key or under the pointer); the confirm takes it
    i32 damageShown = 0;  // 0x0053BFD8 set when a unit's damage numbers are done (ApplyHits)
    i32 pressedScrollPart = 0;  // 0x0053BD9C the scroll part pressed this frame (the arrow keys press up and down)
    i32 pageListRow = 1;        // 0x004A7558 the row marked in the list of state 2 (1-based)
    i32 roleListRow = 1;        // 0x004A755C the row marked in the lists of states 4 and 0x1B (1-based)
    i32 listScroll = 0;         // 0x0053BD04 the scroll of a list
    i32 roleListScroll = 0;     // 0x0053BD08 the scroll of the lists of states 4 and 0x1B
    i32 listRole = 2;           // 0x004A7570 a party role
    u8 listEntries = 0;         // 0x0053BDF2 the entries of the list of state 2
    u16 roleListEntries = 0;    // 0x0053BDF4 the entries of the lists of states 4 and 0x1B
    i32 listCursor = 0;         // 0x0053BD14 the list entry under the cursor
    i32 specialCursor = 0;      // 0x0053BD10 the skill under the cursor of the 絕招 list
    u8 specialCount = 0;        // 0x0053BDF3 the skills of the 絕招 list
    i32 guardCursor = 0;        // 0x0053BCF8 the row under the cursor of the 佈置護駕 panel
    std::array<u16, LootSlots> lootIds = {};  // 0x004FF2F0 the objects of the loot list
    i32 accessoryGained = 0;                  // 0x0053C4C4 set when an accessory gains proficiency at the victory
    i32 endFrames = 0;                        // 0x0053BD20 frames the end messages have waited
    RoleRecord levelUpBefore;                 // 0x004FF108 the record of the role before its level-up
    i32 unk53C4C8 = 0;
    std::array<i16, 9> statRolls = {};  // 0x005214A4..0x00525466 the rises of a level-up still to count
    u16 levelUpFrames = 0;              // 0x0053BDEA the frames of the level-up stats (the high word of 0x0053BDE8)
    // The debug keys (Game::BattleDebugKeys) and the acting unit.
    i32 unk5244E0 = 0;
    i32 unk53AF68 = 0;
    i32 hitsAllRoles = 0;  // 0x0053BF68 1: the hit falls on all roles at once
    i32 unk53BD88 = 0;
    i32 unk53BD80 = 0;
    i32 snapNext = 0;           // 0x0053BFE8 1: the next frame is saved as a screenshot
    std::u8string battleMusic;  // 0x0053C198 the path of the battle music
    i32 unk53AF30 = 0;
    i32 unk53AF34 = 0;
    i32 sweepTotal = 0;  // 0x0053AE74 the units a sweep is to hit
    u16 unk53AE78 = 0;
    i32 targetsAll = 0;  // 0x0053AE84 the acting unit's slot record: targetsAll
    i32 allAtOnce = 0;   // 0x0053AE88 the acting unit's slot record: allAtOnce
    u16 unk53BF4A = 0;   // a unit's place read for the debug gauge and the menu
    u16 unk53BF4E = 0;
    i32 hoverScrollPart = 0;    // 0x0053BDA0 the part of the scroll bar under the pointer (1..4)
    i32 frameMilliseconds = 1;  // 0x004A75E8 the milliseconds of a frame (the debug overlay shows 1000 / it)
    // The pointer over the battle (Game::MouseHover).
    ClickRegion* hoverRegion = nullptr;  // 0x005244CC the click region under the pointer
    i32 menuX = 0;                       // 0x0053BD24 the place of the command menu
    i32 menuY = 0;                       // 0x0053BD28
    i32 hoverPageTab = -1;               // 0x004A7578 the page tab under the pointer (list of state 2)
    i32 hoverRoleTab = -1;               // 0x004A757C the role tab under the pointer (list of state 4)
    i32 thumbPressY = 0;                 // 0x0053BD6C the pointer's y when the thumb was pressed (only written)
    i32 thumbDragOffset = 0;             // 0x0053BD70 the thumb drawn off its place by this (always 0)
    i32 thumbDragged = 0;                // 0x0053BFF8 1: the thumb is dragged (never set, as in the original)
    i32 hoverUnit = 0;                   // 0x0053C4AC the unit aimed at under the pointer
    u16 unk53BF2A = 0;
    u16 unk4FE5D2 = 0;  // the word after commandKinds (read as an accessory kind by MouseHover)
    // The confirm key of the menu (Game::MenuConfirm).
    i32 timingPress = 0;  // 0x0053BDA4 the press of the timing bar (bit 0: Space down, bit 15: taken)
    i32 unk53BCDC = 0;
    i32 unk53C01C = 0;
    std::array<u16, Units> escortCalled = {};  // 0x0052137C per unit: the escort place it called
    u16 callingEscort = 0;                     // 0x0053BF24 the escort being called (its low word)
    i32 levelUpPending = 0;                    // 0x0053C038 a level-up to show at the end
    u16 unk53BF1C = 0;
    i32 timingBarShown = 0;       // 0x0053C024 1: the timing bar of command 0x1A runs (the confirm key waits)
    i32 victoryWithoutPanel = 0;  // 0x0053C004 1: the victory ends without its panel (BattleEndFrame; inferred)
    // The hit act of a slot (Game::HitTargetFrame).
    i32 rippleHits = 0;                             // 0x0053C03C 1: hits run the ripple instead of their pictures
    std::array<i32, Units> rippleFrames = {};       // 0x0053AEE8 per slot: the frames of the ripple
    i32 hitPictureX = 0;                            // 0x0053BDE0 the place the hit picture is drawn at
    i32 hitPictureY = 0;                            // 0x0053BDE4
    std::array<i32, Units> lastSpiritDamage = {};   // 0x005241B4 per slot: the spirit damage of the last hit
    std::array<i32, Units> lastStaminaDamage = {};  // 0x00524220 per slot: the stamina damage of the last hit
    std::array<i32, Units> hitFading = {};          // 0x0052411C per slot: 1 while the hit fades (FadeHitAct)
    i32 hitAllFrames = 0;  // 0x004FE5C8 the frames of a hit act on a whole side (Game::HitAllFrame)
    // The hits of the acting unit (Game::HitFrame).
    i32 hitLanded = 0;              // 0x0053BFA8 1 once a hit landed
    i32 sweepHit = 0;               // 0x0053BF7C 1 once a sweep hit a unit
    u16 sweepGap = 0;               // 0x0053BDEC frames between the units a sweep reaches
    u16 unk4FDF7C = 0;              // the attacker's word 29A2 of the last hit
    u16 lastRoleHit = 0xFFFF;       // 0x004A7620 the last role hit
    u16 lastEnemyHit = 0xFFFF;      // 0x004A7622 the last enemy hit
    u16 lastRoleDefeated = 0xFFFF;  // 0x004A7624 the last role defeated by a hit
    i32 enemySweeps = 0;            // 0x0053BF84 1: an enemy's hit sweeps
    i32 enemyHitsAll = 0;           // 0x0053BFF4 1: an enemy's hit falls on the party at once
    i32 enemyHitAct = 0;            // 0x0053BD40 the hit act of the acting enemy
    i32 sweepTargets = 0;           // 0x0053BD7C the units an enemy's sweep hits
    i32 sweepDefeated = 0;          // 0x0053BD84 the units an enemy's sweep defeated
    i32 enemyFaded = 0;             // 0x0053BFE0 1 once an enemy faded or left (Game::EnemyActionFrame)
    // The turn of an enemy (Game::EnemyFrame).
    std::array<u16, Enemies> enemyOrders = {};  // 0x005028AC the order of the fight talk for each enemy
    i32 enemyGaugeBoost = 0;                    // 0x0053C044 the boost of the enemies' gauges
    i32 heldEnemy = 0;                          // 0x0053BD58 the enemy (1-based) whose turn ended held
    i32 gatheringSweep = 0;                     // 0x0053BF8C 1 while an enemy gathers the targets of a sweep
    i32 enemyTargetPicked = 0;                  // 0x0053C014 1 once the acting enemy picked its target
    i32 enemyTarget = 0;                        // 0x0053BCFC the target of the acting enemy
    i32 enemyAimsAtEnemies = 0;                 // 0x0053BFD4 1: an ordered enemy aims at the enemies
    i32 unk53BF9C = 0;
    i32 enemiesHitAct = 0;  // 0x0053BDBC the hit act of the enemies (unk53AE90)
    i32 unk53BFDC = 0;
    // The commands of a role (Game::RoleActionFrame).
    i32 absorbSucceeded = 0;  // 0x0053C020 1: the absorb succeeded
    u16 usedUpItem = 0;       // 0x0053BF34 the item used up by the last command
    i32 unk53BDD4 = 0;
    i32 unk53BDD8 = 0;
    SkillScript accessoryEffect;  // 0x004FF140 the buff or ailment of an accessory command
    // The frame of a role (Game::RoleFrame).
    i32 aiTarget = 1;  // 0x004A7554 the enemy (1-based) an AI role aims at
    u16 unk5240C4 = 0;
    // The frame of the battle (Game::BattleFrame).
    u16 slotRecordFrames = 0;  // 0x0053BF1A frames since the last slot record was taken
    u16 snapNumber = 0;        // 0x0053C49A the number of the last snapshot
    // Cleared by Game::EndBattle (no other use found).
    std::array<i32, 8> unk524200 = {};
    std::array<i32, 8> unk52100C = {};
    std::array<i32, 0x90> unk4FF318 = {};
    std::array<i32, 0xA> unk4FF588 = {};
    i32 unk53AED8 = 0;
    i32 unk4A7580 = 0x60;  // the opening places of unk4A7580..unk4A7594 (exe data)
    i32 unk4A7584 = 0x180;
    i32 unk4A7588 = 0xF0;
    i32 unk4A758C = 0xFA;
    i32 unk4A7590 = 0xF2;
    i32 unk4A7594 = 0x64;
    i32 unk53BD4C = 0;
    i32 unk53BD74 = 0;
    i32 unk53BD78 = 0;
    i32 unk520FBC = 0;
    i32 unk520FC0 = 0;
    u16 unk520FC4 = 0;
    i32 unk53BF04 = 0;
    i32 unk53BF08 = 0;
    u16 unk5242F8 = 0;
    u16 unk4A7628 = 6;  // passed to DrawRolePanels, which takes no argument
    u8 unk53BDEE = 0;
    i32 unk53BF6C = 0;
    i32 unk53BF70 = 0;
    i32 unk53BF78 = 0;
    i32 unk53BF90 = 0;
    i32 unk53BFA0 = 0;
    i32 unk53BFA4 = 0;
    i32 unk53BFAC = 0;
    i32 unk53BFB4 = 0;
    i32 unk4A763C = 1;
    i32 unk53C028 = 0;
    i32 autoTarget = -1;        // 0x004A7564 the enemy the auto battle aims at (1-based), -1: none
    u16 anyStateRole = 0xFFFF;  // 0x004A7644 a role unit that may take the panel in any battle state (SelectRoleUnit)
    i32 menuCommand = 1;        // 0x004A7548 the command chosen in the menu (1-based)
    i32 listPage = 0;           // 0x0053BD18 the page of the list of state 2 (0..2)
    // The menu (MenuCancel).
    i32 unk53BFB0 = 0;
    // The rebuild (deviations.md): the unit the keys went to last frame; a new one spends the keys held.
    i32 keysUnit = 0;
    i32 listPanelFrames = 0;               // 0x0053BD90 the frames of the list panel act
    i32 menuGrow = 0;                      // 0x0053BD94 how far the command menu has opened
    i32 listRows = 0;                      // 0x0053BD98 the rows of the list open
    std::array<char, 0x14> listName = {};  // 0x0053C184 a name shown by the lists
    i32 unk53C49C = 0;
    i32 unk53BFC8 = 0;
    u16 controlledEnemy = 0xFFFF;  // 0x004A762C the enemy under control (0xFFFF: none)
    u16 talkRole = 0xFFFF;         // 0x004A762E the role with a battle talk (0xFFFF: none)
    i32 orderedSkill = 0;          // 0x0053BD44 the skill a controlled enemy is ordered to use
    i32 unk53BD48 = 0;
    i32 unk53C02C = 0;
    i32 unk53BF88 = 0;
    i32 unk53BFF0 = 0;
    i32 listReturnCommand = 0;  // 0x0053BCF0 the menu command a list returns to
    i32 specialRow = 1;         // 0x004A7560 the row of the 絕招 list (1-based)
    i32 controlRows = 2;        // 0x004A7568 the rows of the 控制 menu
    i32 controlCursor = 1;      // 0x004A756C its row under the cursor (1-based)
    // The proficiency filled in the battle (AnnounceMastery).
    ObjectRecord lastObject;   // 0x0053BC28 the object read last (stolen, or its proficiency filled)
    LearnedSkill lastLearned;  // 0x0053C154 the name of the accessory or of the skill learned, for the messages
    i32 learnedPending = 0;    // 0x0053BFFC 1: a proficiency filled or a special was learned, its message to show
    u8 levelUpRole = 0xFF;     // 0x004A7646 the role
    // The end of the battle (Game::CheckBattleEnd, Game::DrawBattlePrompt).
    i32 blackFade = 0;    // 0x0053C4A8 the fade of the field to black, 2 darker a frame
    i32 frameResult = 0;  // 0x0053BDC8 the result of the battle frame (1 runs on; the end CheckBattleEnd set)
    std::array<u16, 3> givenBack = {};  // 0x004FF2EA [1], [2]: objects given back at the end (set by 0x0045EC60)
    i32 promptFrames = 0;               // 0x0053BDA8 frames of the prompt
    i32 timerAttack = 0;  // 0x0053BDAC the timer attack of command 0x1F (bit 31: running, bit 0: a press to hit)
    i32 mouseMode = 0;    // 0x0053BFBC 1: the mouse is in use (MouseHover): the menus follow the pointer, the aim waits
    i32 turnPointer = 1;  // 0x004A7640 1: the pointer shows the turn mark
    i32 unk53BF58 = 0;
    i16 unk53BF50 = 0;
    i16 unk53BF52 = 0;
};

}  // namespace swd3
