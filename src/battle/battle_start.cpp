#include <algorithm>
#include <array>

#include "battle/battle_items.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "pic/picture_ops.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr int kGuests = 8;
constexpr int kMaxGuests = 2;
// swd32.exe 0x004A7600: the story flags of the guests who may join a battle, and 0x004A7610 their mon.dat objects:
// 薇達, 麥爾斯, 慧彥, 張客, 慧榮, 夜行裝林明, 鎮邪 and 悟緣.
constexpr std::array<u16, kGuests> kGuestFlags = {34, 35, 38, 44, 45, 46, 47, 49};
constexpr std::array<u16, kGuests> kGuestActs = {3, 4, 10, 33, 34, 37, 38, 40};
// swd32.exe 0x004A75D8: the battle act of each party role (賽特, 妮可, 卡瑪, 李靖), dwords read as words.
constexpr std::array<u32, Party::Roles> kRoleActs = {1, 2, 8, 0x17};
constexpr i16 kGuestX = 0x2EE;
constexpr i16 kGuestY = 0x136;
constexpr i16 kPanelMirror = 0x270;
constexpr u16 kShortTimingWindow = 0x12;  // with game flag 0x1BB0
constexpr int kMediumAdvance = 0x10;
constexpr int kLightRed = 0x1F;  // the light and dark colours of the battle panels
constexpr int kLightGreen = 0x1D;
constexpr int kLightBlue = 0x17;
constexpr int kDarkRed = 0xF;
constexpr int kDarkGreen = 0xE;
constexpr int kDarkBlue = 0xB;
constexpr int kPointerX = Display::Width / 2;  // the pointer starts in the middle of the screen
constexpr int kPointerY = Display::Height / 2;
constexpr int kBarLength = 56;  // the pixels of a full gauge of the role panels
constexpr int kMaxEnemySteps = 6;
constexpr int kScenes = 4;
constexpr i32 kNoParty = 0x67;
constexpr std::u8string_view kBadBattle = u8"第%d號戰鬥場景資料錯誤!!";  // swd32.exe 0x004A76F0

struct Spot
{
    i16 x;
    i16 y;
};

// The places of the party by its size, and the offsets of the role panels from them.
constexpr std::array<std::array<Spot, 4>, 4> kPartySpots = {{
    {{{0x20F, 0x11F}, {0, 0}, {0, 0}, {0, 0}}},
    {{{0x1EA, 0x113}, {0x22B, 0x172}, {0, 0}, {0, 0}}},
    {{{0x1F8, 0x110}, {0x235, 0x161}, {0x1CE, 0xE0}, {0, 0}}},
    {{{0x20E, 0x12A}, {0x1F1, 0x115}, {0x1D0, 0xD9}, {0x24C, 0x167}}},
}};

constexpr std::array<Spot, 4> kPanelOffsets = {{{0xA, -0x91}, {5, -0xAA}, {0xA, -0x9B}, {-5, -0xA3}}};

// A gauge of `length` pixels for value out of maximum (the original divides as doubles and truncates with _ftol).
i32 Bar(int value, int maximum)
{
    return static_cast<i32>(crt::Ftol(static_cast<double>(value) / static_cast<double>(maximum) * kBarLength));
}

}  // namespace

// swd32.exe 0x00451940: the battle background: picture `picture` of all_map2.tsw turned to 16 bits and shifted right
// by a share of the screen per scene, and its animation when the battle has one
void Game::LoadBattleBackground(int picture, u16 animation, int x, int y, u16 scene)
{
    BattleState& battle = this->battleState;
    const std::string path = Concat({this->config.installDirectory, "all_map2.tsw"});
    this->battleHud.FreeAnimation(battle.backgroundAnimation);
    battle.background = Picture{};

    if (!this->archive.LoadFromFile(path, picture, 0, battle.background))
    {
        return;
    }

    this->codec.ConvertTo16(battle.background);
    ShiftPicture(battle.background.data, ShiftMode::Right, Display::Width / scene);

    if (battle.formation.animated != 0 && animation != 0)
    {
        this->battleHud.LoadAnimation(battle.backgroundAnimation, 0, x, y, animation, scene);
    }

    battle.unk4A7632.fill(-1);
}

// swd32.exe 0x0045B0E0: the y of the enemies and of the roles (from 8)
void Game::MeasureUnits()
{
    BattleState& battle = this->battleState;
    battle.unitY.fill(0);
    battle.drawOrder.fill(0);
    u16 x = 0;
    u16 y = 0;

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        this->battleEnemies[enemy].GetDrawPosition(x, y);
        battle.unitY[enemy] = static_cast<i16>(y);
    }

    for (u32 role = 0; role < battle.roleCount; ++role)
    {
        this->battleRoles[role].GetDrawPosition(x, y);
        battle.unitY[BattleState::FirstRoleUnit + role] = static_cast<i16>(y);
    }
}

// swd32.exe 0x00451B10: a battle starts: the music stops, the battle state is cleared, the party (story flags 0x1E..)
// and up to two guests take their places, the formation of battle.ffd brings the enemies and the background, the
// role panels get their gauges, the units are put in drawing order and the enemies' gauges get a head start. A
// formation without enemies is reported and the battle does not start.
void Game::StartBattle(u16 number)
{
    BattleState& battle = this->battleState;
    GameState& state = this->state;
    battle.battle = number;
    this->audio.StopMusic();
    SetGaugeFull(battle, this->config.battleSpeed);
    battle.unk525470 = ActState{};
    battle.unk4FF168.fill(0);
    battle.unk524324.fill(0);
    battle.unk4FE5D4.fill(0);
    battle.actionQueue.fill(0);
    battle.unk4FF0B0 = 0;
    battle.unk4FE5CC = 0;
    battle.unk4FF0B4 = 0;
    battle.unk4FE5D0 = 0;
    battle.unk4FF0B8 = 0;
    this->battleMessages.messages.reset();
    battle.unk524268.fill(0);
    battle.unk524414 = 0;
    battle.unk52544C = 0;
    battle.unk520E90.fill(0);
    battle.unk4FF0BC.fill(0);
    battle.unk525450 = 0;
    battle.background = Picture{};
    battle.unk525454 = 0;
    battle.unk502944 = 0;
    battle.unk525458 = 0;
    battle.unk502948 = 0;
    battle.unk5244D8 = 0;
    battle.unk50294C = 0;
    battle.unk502950 = 0;
    battle.unk5244DC = 0;
    battle.unk5242B0.fill(0);
    battle.unk524418 = 0;
    battle.unk502984.fill(-1);
    battle.unk524420.fill(-1);
    battle.unk53AE90.fill(-1);
    battle.unk5244E8.fill(-1);
    battle.inParty.fill(0);
    battle.guestUsed.fill(0);
    battle.unk53C048 = 0;
    battle.unk53AE70 = -1;
    battle.unk53BF22 = 0;
    battle.unk53C4B0 = 0;

    for (BattleSlotRecord& record : battle.unk524788)
    {
        record.unk00 = -1;
        record.unk18 = 0;
        record.unk0C = 0;
        record.unk14 = 0;
        record.unk0A = 0;
    }

    battle.unk4FF578.fill(1);
    ResetPlayback(this->talk.endAct);
    this->talk.endAct.id = Scene::PointerAct;
    this->talk.endAct.action = Scene::UnkPointerC;
    this->graphics.SetClearColor(this->painter.FillColorPair());

    for (int role = 0; role < Party::Roles; ++role)
    {
        if (state.GameFlag(GameState::InParty + role))
        {
            battle.inParty[static_cast<std::size_t>(role)] = 1;
            ++battle.roleCount;
        }
    }

    u16 next = 0;

    for (u16 slot = 0; slot < battle.roleCount; ++slot)
    {
        while (next < Party::Roles && battle.inParty[next] == 0)
        {
            ++next;
        }

        battle.partyRoles[slot] = next;
        ++next;

        if (next >= Party::Roles)
        {
            break;
        }
    }

    if (state.GameFlag(GameState::UnkC9))
    {
        battle.panelFlags |= BattleState::UnkPanel2;
    }

    battle.timingWindow = BattleState::DefaultTimingWindow;

    if (state.GameFlag(GameState::Unk1BB0))
    {
        battle.timingWindow = kShortTimingWindow;
    }

    this->mediumFont.SetAdvance(kMediumAdvance);
    battle.lightColor = static_cast<u16>(this->painter.MakeColor(kLightRed, kLightGreen, kLightBlue));
    battle.darkColor = static_cast<u16>(this->painter.MakeColor(kDarkRed, kDarkGreen, kDarkBlue));
    battle.projector.Init(this->graphics);
    state.pointerX = kPointerX;
    battle.pointerX = kPointerX;
    state.pointerY = kPointerY;
    battle.pointerY = kPointerY;
    this->input.SetMousePosition(kPointerX, kPointerY);
    ReleaseBattleSurfaces(battle);
    CreateBattleSurfaces(battle, this->graphics);
    this->fightTalk.Load(this->config.installDirectory, number);
    const std::string path = Concat({this->config.installDirectory, "battle.ffd"});
    battle.file.ReadHeader(path);
    battle.file.ReadFormation(path, battle.formation, number, 0);
    battle.enemyCount = battle.formation.enemyCount;
    battle.animated = battle.formation.animated;

    if (battle.formation.enemyCount == 0)
    {
        ErrorLog(kBadBattle, number);
        return;
    }

    battle.scene = static_cast<u16>(this->random.Below(kScenes) + 1);
    this->LoadBattleBackground(
        battle.formation.background,
        battle.formation.animation,
        battle.formation.animationX,
        battle.formation.animationY,
        battle.scene
    );
    battle.unk525470 = ActState{};

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        BattleEnemy& unit = this->battleEnemies[enemy];
        unit.Reset(this->battleServices);
        battle.unk5246E0 = ObjectRecord{};
        UnitPlacement& place = battle.enemyPlaces[enemy];
        place.actId = static_cast<u16>(battle.formation.enemies[enemy]);
        place.x = static_cast<i16>(battle.formation.enemyX[enemy]);
        place.y = static_cast<i16>(battle.formation.enemyY[enemy]);
        place.kind = 0;

        if (battle.mirrored == 1)
        {
            unit.SetFacing(1);
            place.x = static_cast<i16>(Display::Width - place.x);
        }

        unit.Setup(place, static_cast<u16>(battle.formation.enemies[enemy]), this->database);

        if (battle.formation.guarding[enemy] == 1)
        {
            unit.SetGuarding(1);
        }
    }

    for (u32 role = 0; role < battle.roleCount && role < Party::Roles; ++role)
    {
        UnitPlacement& place = battle.rolePlaces[role];
        place.actId = static_cast<u16>(kRoleActs[static_cast<std::size_t>(battle.partyRoles[role])]);
        place.kind = BattleUnit::PartyRole;
    }

    if (battle.roleCount >= 1 && battle.roleCount <= Party::Roles)
    {
        const std::array<Spot, 4>& spots = kPartySpots[battle.roleCount - 1];

        for (u32 role = 0; role < battle.roleCount; ++role)
        {
            battle.rolePlaces[role].x = spots[role].x;
            battle.rolePlaces[role].y = spots[role].y;
        }
    }

    for (std::size_t panel = 0; panel < kPanelOffsets.size(); ++panel)
    {
        battle.panelPlaces[2 * panel] = battle.rolePlaces[panel].x + kPanelOffsets[panel].x;
        battle.panelPlaces[2 * panel + 1] = battle.rolePlaces[panel].y + kPanelOffsets[panel].y;
    }

    for (u32 role = 0; role < battle.roleCount && role < Party::Roles; ++role)
    {
        BattleRole& unit = this->battleRoles[role];
        const auto member = static_cast<std::size_t>(battle.partyRoles[role]);
        unit.Reset(this->battleServices);

        if (battle.mirrored == 1)
        {
            unit.SetFacing(1);
            battle.rolePlaces[role].x = static_cast<i16>(Display::Width - battle.rolePlaces[role].x);
            battle.panelPlaces[2 * role] = kPanelMirror - battle.panelPlaces[2 * role];
        }

        unit.Setup(this->party.roles[member], this->party.battleBlocks[member], battle.rolePlaces[role], this->errors);

        if (unit.IsDefeated())
        {
            ++battle.defeatedRoles;
        }
    }

    SortItems(this->party.inventory);
    SortSkills(this->party.skills);

    for (u32 role = 0; role < battle.roleCount && role < Party::Roles; ++role)
    {
        BattleRole& unit = this->battleRoles[role];
        const auto member = static_cast<std::size_t>(battle.partyRoles[role]);
        const auto slots = std::span<const std::unique_ptr<ItemNode>>(this->party.equipment)
                               .subspan(member * Party::SlotsPerRole, Party::SlotsPerRole);
        unit.ApplyEquipment(slots, this->party.fame.lists[Fame::ProficiencyList], this->errors);
        unit.SetSkills(this->party.skills[member].get());
        unit.SetInventory(&this->party.inventory);
        unit.SetDisplayName(this->party.names[member]);
    }

    for (u32 role = 0; role < battle.roleCount && role < Party::Roles; ++role)
    {
        BattleRole& unit = this->battleRoles[role];
        i32 hitPoints = 0;
        i32 maximum = 0;
        unit.GetHitPoints(hitPoints, maximum);
        battle.hitPointBars[role] = Bar(hitPoints, maximum);
        battle.hitPointBarsShown[role] = battle.hitPointBars[role];
        battle.hitPoints[role] = hitPoints;
        u16 spiritMaximum = 0;
        u16 spirit = 0;
        unit.GetSpirit(spiritMaximum, spirit);
        battle.spiritBars[role] = Bar(static_cast<i16>(spirit), static_cast<i16>(spiritMaximum));
        battle.spiritBarsShown[role] = battle.spiritBars[role];
        battle.spirit[role] = static_cast<i16>(spirit);
        u16 staminaMaximum = 0;
        u16 stamina = 0;
        unit.GetStamina(staminaMaximum, stamina);
        battle.staminaBars[role] = Bar(static_cast<i16>(stamina), static_cast<i16>(staminaMaximum));
        battle.staminaBarsShown[role] = battle.staminaBars[role];
        battle.stamina[role] = static_cast<i16>(stamina);
        const i32 block = unit.roleBlock != nullptr ? unit.roleBlock[0] : 0;
        battle.blockValues[role] = block;
        battle.blockValuesShown[role] = block;
    }

    int joining = 0;

    for (const u16 flag : kGuestFlags)
    {
        if (state.GameFlag(flag))
        {
            ++joining;
        }
    }

    battle.guestCount = static_cast<u16>(battle.guestCount + joining);

    auto addGuest = [this, &battle](std::size_t guest)
    {
        BattleRole& unit = this->battleRoles[battle.roleCount];
        UnitPlacement& place = battle.rolePlaces[battle.roleCount];
        place.actId = kGuestActs[guest];
        place.x = kGuestX;
        place.y = kGuestY;
        place.kind = BattleUnit::PartyRole;

        if (battle.mirrored == 1)
        {
            place.x = static_cast<i16>(Display::Width - place.x);
        }

        const auto* base = static_cast<const RoleRecord*>(this->battleRoles[0].SelectEntry(1));
        unit.SetupGuest(place, *base, this->database, this->errors);
        unit.SetB04(1);

        if (battle.mirrored == 0)
        {
            unit.SetFacing(1);
        }

        ++battle.roleCount;
    };

    if (battle.guestCount <= kMaxGuests)
    {
        battle.guestCount = 0;

        for (std::size_t guest = 0; guest < kGuests; ++guest)
        {
            if (!state.GameFlag(kGuestFlags[guest]))
            {
                continue;
            }

            addGuest(guest);
            const bool last = battle.guestCount == 1;
            ++battle.guestCount;

            if (last)
            {
                break;
            }
        }
    }
    else
    {
        battle.guestCount = 0;
        bool last = false;

        while (!last)
        {
            std::size_t guest = 0;

            do
            {
                do
                {
                    guest = static_cast<u16>(this->random.Below(kGuests));
                } while (!state.GameFlag(kGuestFlags[guest]));
            } while (battle.guestUsed[guest] == 1);

            addGuest(guest);
            last = battle.guestCount == 1;
            ++battle.guestCount;
            battle.guestUsed[guest] = 1;
        }
    }

    this->MeasureUnits();
    battle.SortDrawOrder();
    battle.ListEnemyOrder();

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        const u32 steps = this->random.Below(kMaxEnemySteps);

        for (u32 step = 0; step < steps; ++step)
        {
            this->battleEnemies[enemy].AdvanceGauge(this->battleServices, 0);
        }
    }

    for (u32 role = 0; role < battle.roleCount; ++role)
    {
        this->battleRoles[role].RandomizeGauge(this->battleServices);
    }

    const u32 fighting = battle.roleCount - battle.unk53BF26 - battle.guestCount;

    if (battle.defeatedRoles >= fighting)
    {
        battle.result = kNoParty;
    }
}

}  // namespace swd3
