#include "core/fault.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "item/item_list.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr u16 kAimNone = 0x8000;  // GetWord29A2: the role aims at no enemy
constexpr i32 kBattleWon = 0x63;
constexpr i32 kBattleLost = 0x67;
constexpr i32 kTurnsRunning = 1;   // BattleState::result while no side has lost
constexpr u16 kBossReward = 0x14;  // unk53BF16 gained by an enemy of ObjectData::UnkKind20
constexpr u16 kEnemyReward = 3;
constexpr i16 kDefeatCount = 1;  // the bestiary count of a defeated enemy
constexpr std::size_t kLastTurnUnit = BattleState::RoleUnits - 1;
constexpr i32 kFieldFadeStep = 2;
constexpr i32 kFieldFadeEnd = -0x20;
constexpr u16 kThrownObject = 0x300;  // the object a thrown attack takes (given back unless it lands)
constexpr i32 kLostEnd = 2;           // unk53BDC8 when the party is down
constexpr i32 kLostQuietly = 0x68;    // a battle state that ends with unk53BDC8 0
constexpr i32 kPromptFrames = 0x12C;
constexpr i32 kWaitState1 = 1;  // the battle states of the wait pointer
constexpr i32 kWaitState2 = 2;
constexpr i32 kTurnState = 3;
constexpr i32 kWaitState4 = 4;
constexpr i32 kWaitState5 = 5;
constexpr i32 kWaitState8 = 8;
constexpr i32 kWaitState1B = 0x1B;
constexpr i32 kWaitState1E = 0x1E;
constexpr int kNoUnitPointer = 0x238C;
constexpr int kUnitPointer = 0x238D;
constexpr int kWaitPointer = 0x238E;
constexpr int kTurnPointer = 0x238F;
constexpr int kNoTurnPointer = 0x2393;
constexpr int kMarkEffect = 0x23A0;
constexpr i32 kListPage = 7;  // a back key scrolls a list up by this
constexpr u16 kBackSound = 0x2E;
constexpr i32 kListState2 = 2;
constexpr i32 kListState4 = 4;
constexpr i32 kListState1B = 0x1B;
constexpr i32 kGuardOrder = 5;  // the order of a role that guards in the auto battle

}  // namespace

// swd32.exe 0x00464CC0: role unit `unit` takes the turn (unk53BD50, its unit turn and SetTarget 1); while an enemy
// stands, unk4A754C picks one at random (1-based), going on past defeated ones for at most enemyCount tries.
void Game::StartRoleTurn(int unit)
{
    BattleState& battle = this->battleState;
    battle.unk53BD50 = unit;
    battle.unk53BCE8 = 1;
    battle.unk53BF60 = 1;
    battle.unk53AF38[static_cast<std::size_t>(unit)] = 1;
    this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].SetTarget(1);
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.unk53BF00 >= count)
    {
        return;
    }

    battle.unk4A754C = static_cast<i32>(this->random.Below(battle.enemyCount)) + 1;
    int tries = 0;

    while (this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].IsDefeated())
    {
        ++battle.unk4A754C;

        if (battle.unk4A754C > count)
        {
            battle.unk4A754C = 1;
        }

        if (++tries >= count)
        {
            break;
        }
    }
}

// swd32.exe 0x0045ADF0: when `enemy` is unk4A7626 its turn is reset, and each role but `role` aiming at it turns to
// the first standing enemy other than it (unk53C010 set). With none and one enemy left, such a role's aim, command,
// target and turn are reset and the turn order is cleared.
void Game::RetargetRoles(int role, int enemy)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (battle.unk4A7626 != enemy)
    {
        return;
    }

    this->battleEnemies[static_cast<std::size_t>(enemy)].ResetTurn(services);

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        if (static_cast<int>(index) == role)
        {
            continue;
        }

        BattleRole& unit = this->battleRoles[index];
        const u16 aim = unit.GetWord29A2();

        if ((aim & kAimNone) != 0 || aim != battle.unk4A7626)
        {
            continue;
        }

        const auto count = static_cast<i32>(battle.enemyCount);
        bool turned = false;

        for (int other = 0; other < count; ++other)
        {
            if (battle.unk4A7626 != other && !this->battleEnemies[static_cast<std::size_t>(other)].IsDefeated())
            {
                unit.ClearWord29A2();
                this->battleEnemies[aim].DecreaseHits();
                unit.Retarget(static_cast<i16>(other));
                battle.unk53C010 = 1;
                turned = true;
                break;
            }
        }

        if (turned || battle.unk53BF00 + 1 != count)
        {
            continue;
        }

        unit.ClearWord29A2();
        this->battleEnemies[aim].DecreaseHits();
        unit.SetCommand(0);
        unit.SetTarget(0);
        unit.ResetTurn(services);
        battle.unk53BF64 = 0;
        battle.unk520DD0 = {};
        battle.unk53BD50 = 0;
        battle.currentUnit = 0;
        battle.unk53AE70 = -1;
        battle.WipeSlotRecords();
    }
}

// swd32.exe 0x0045AA00: one frame of the defeat of unit `index` (a role when `role` is 1, else an enemy, none for
// -1); 1 once it is over. An enemy adds its rewards (unk53BF14, unk53BF12, unk53BF16), its bestiary count and the
// defeated count; all down wins the battle, and a lone standing first enemy of a unk53BF28 battle ends the others. A
// role counts as defeated (a forced one as a defeated guest: once all are down their places go and the units are
// measured again), drops its command, slot record, turn and order; with no party role standing the battle is lost,
// else one with EffectFlag2 takes the next turn.
int Game::RemoveDefeated(int index, int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (role != 1)
    {
        if (index == -1)
        {
            return 0;
        }

        BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(index)];

        if (enemy.DefeatFrame(services) != 1)
        {
            return 0;
        }

        u16 first = 0;
        u16 second = 0;
        enemy.Unk62And8A(first, second);
        battle.unk53BF14 = static_cast<u16>(battle.unk53BF14 + first);
        battle.unk53BF12 = static_cast<u16>(battle.unk53BF12 + second);
        battle.unk5028FC = enemy.Data();
        const bool boss = (battle.unk5028FC->data.kind & ObjectData::UnkKind20) != 0;
        battle.unk53BF16 = static_cast<u16>(battle.unk53BF16 + (boss ? kBossReward : kEnemyReward));
        AddCounterCapped(this->party.fame.lists[Fame::FameList], static_cast<i16>(enemy.ActId()), kDefeatCount);
        battle.RemoveSlotRecord(index);
        const auto count = static_cast<i32>(battle.enemyCount);

        if (index >= 0 && index <= count)
        {
            ++battle.unk53BF00;
        }

        if (battle.unk53BF00 - battle.unk53BF02 >= count)
        {
            battle.result = kBattleWon;
            battle.autoBattle = 0;
            battle.unk53BFC4 = 1;
            battle.unk53BFC0 = 1;
        }

        if (battle.unk53BF28 != 0 && !this->battleEnemies[0].IsDefeated() &&
            static_cast<i32>(battle.enemyCount) - battle.unk53BF00 == 1)
        {
            battle.enemyCount = 1;
            battle.unk53BF00 = 0;
            battle.unk53BF28 = 0;
            this->MeasureUnits();
            battle.SortDrawOrder();
            battle.ListEnemyOrder();
        }

        return 1;
    }

    BattleRole& unit = this->battleRoles[static_cast<std::size_t>(index)];

    if (unit.DefeatFrame(services) != 1)
    {
        return 0;
    }

    if (unit.unk2B00 == 1)
    {
        ++battle.unk53BF01;

        if (battle.unk53BF01 >= battle.unk53BF26)
        {
            const u16 guests = battle.unk53BF26;

            // The original clears the places from the end of the list down, before it when there are fewer.
            for (u16 cleared = 0; cleared < guests && cleared < battle.roleCount; ++cleared)
            {
                battle.rolePlaces[battle.roleCount - 1 - cleared] = UnitPlacement{};
            }

            battle.unk53BF26 = 0;
            battle.roleCount -= guests;
        }

        this->MeasureUnits();
        battle.SortDrawOrder();
        battle.ListEnemyOrder();
    }
    else
    {
        ++battle.defeatedRoles;
    }

    unit.CancelCommand();
    const int unitId = index + BattleState::FirstRoleUnit;
    battle.RemoveSlotRecord(unitId);
    battle.unk524268[static_cast<std::size_t>(index)] = 0;

    if (battle.currentUnit == unitId)
    {
        for (u32 other = 0; other < battle.roleCount; ++other)
        {
            this->battleRoles[other].SetAC0(0);
        }

        for (u32 other = 0; other < battle.enemyCount; ++other)
        {
            this->battleEnemies[other].SetAC0(0);
        }

        battle.currentUnit = 0;
        battle.unk53BCE8 = 0;
        battle.unk4A754C = 1;
        battle.result = kTurnsRunning;
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BF74 = 0;
    }

    if (battle.unk53AE70 == unitId)
    {
        battle.unk53AE70 = -1;
        battle.unk53BF74 = 0;
    }

    for (std::size_t at = 0; static_cast<i32>(at) < static_cast<i32>(battle.roleCount); ++at)
    {
        if (battle.unk520DD0[at] != unitId)
        {
            continue;
        }

        for (std::size_t next = at; next < kLastTurnUnit; ++next)
        {
            battle.unk520DD0[next] = battle.unk520DD0[next + 1];
        }

        battle.unk520DD0[kLastTurnUnit] = 0;
        break;
    }

    if (battle.defeatedRoles >= battle.roleCount - battle.guestCount - battle.unk53BF26)
    {
        battle.WipeSlotRecords();
        battle.unk53AE70 = -1;
        battle.unk53BFC0 = 1;
        battle.unk53BFC4 = 1;
        battle.result = kBattleLost;
        return 1;
    }

    if (unit.EffectFlag2() == 1)
    {
        battle.unk4A754C = battle.unk53AE70 + 1;
        battle.unk53BD50 = unitId;
        unit.SetTarget(1);
        battle.unk53BCE8 = 1;
        battle.unk53AF38[static_cast<std::size_t>(unitId)] = 1;
        battle.unk53BF60 = 1;
        battle.pendingOrders[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)] = PendingOrder{};
    }

    return 1;
}

// swd32.exe 0x0045B280: the draw order with the acting unit (unk53AE70) drawn right after the unit it aims at (none
// while unk53C4C0, unk53AE7C or unk53AE80 is set or no unit acts): a role aims at an enemy, an enemy at a role.
void Game::SortActingDrawOrder()
{
    BattleState& battle = this->battleState;

    if (battle.unk53C4C0 == 1 || battle.unk53AE7C == 1 || battle.unk53AE80 == 1 || battle.unk53AE70 == -1)
    {
        return;
    }

    const i32 acting = battle.unk53AE70;
    i32 target = 0;

    if (acting >= BattleState::FirstRoleUnit)
    {
        const auto role = static_cast<std::size_t>(acting - BattleState::FirstRoleUnit);
        target = static_cast<i16>(this->battleRoles[role].GetWord29A2());

        // Never so here (the original returned above).
        if (battle.unk53AE7C == 1)
        {
            target += BattleState::FirstRoleUnit;
        }
    }
    else
    {
        target = static_cast<i16>(this->battleEnemies[static_cast<std::size_t>(acting)].GetWord29A2());

        if (battle.unk53AE80 == 0)
        {
            target += BattleState::FirstRoleUnit;
        }
    }

    battle.SortDrawOrderAround(acting, target);
}

// swd32.exe 0x0045BD10: one frame of the field fading to black: the frame's pixels shifted by the fade (unk53C4A8,
// also the painter's tint), 2 darker a frame; 1 once past -32 (the fade reset to 0).
int Game::FadeFieldOut()
{
    BattleState& battle = this->battleState;
    DrawOptions& options = this->painter.Options();
    options.tintRed = battle.unk53C4A8;
    options.tintGreen = battle.unk53C4A8;
    options.tintBlue = battle.unk53C4A8;
    const std::span<u16> frame(this->drawSurface, static_cast<std::size_t>(Display::Width * Display::Height));
    this->painter.ShiftRed(frame, battle.unk53C4A8);
    this->painter.ShiftGreenPairs(frame, options.tintGreen);
    this->painter.ShiftBluePairs(frame, options.tintBlue);
    battle.unk53C4A8 -= kFieldFadeStep;

    if (battle.unk53C4A8 > kFieldFadeEnd)
    {
        return 0;
    }

    battle.unk53C4A8 = 0;
    return 1;
}

// swd32.exe 0x0045E9C0: the objects held back in the battle given to the inventory: unk4FF2EA[1] and [2], and one
// kThrownObject for each still thrown (unk53C4B0); the counts and unk53BF26 are cleared.
void Game::ReturnBattleItems()
{
    BattleState& battle = this->battleState;

    for (std::size_t at = 1; at < battle.unk4FF2EA.size(); ++at)
    {
        if (battle.unk4FF2EA[at] != 0)
        {
            AddOneItem(this->party.inventory, battle.unk4FF2EA[at], 0, this->database);
        }
    }

    for (i32 thrown = 0; thrown < battle.unk53C4B0; ++thrown)
    {
        AddOneItem(this->party.inventory, kThrownObject, 0, this->database);
    }

    battle.unk53BF26 = 0;
    battle.unk53C4B0 = 0;
    battle.unk4FF2EA[1] = 0;
    battle.unk4FF2EA[2] = 0;
}

// swd32.exe 0x0045E580: with every party role down (unk53BF5C set) the field fades out (unk53BFE4), then the music
// stops, the objects return and unk53BDC8 becomes 2 (0 for a battle state of 0x68 or UnkPanel8); with every enemy
// down or unk53CEAC set it fades out too, the objects return and unk53BDC8 becomes 0.
void Game::CheckBattleEnd()
{
    BattleState& battle = this->battleState;

    if (battle.defeatedRoles >= battle.roleCount - battle.unk53BF26 - battle.guestCount)
    {
        battle.unk53BF5C = 1;

        if (battle.unk53BFE4 == 1 && this->FadeFieldOut() == 1)
        {
            this->audio.StopMusic();
            this->ReturnBattleItems();
            battle.unk53BDC8 = kLostEnd;

            if (battle.result == kLostQuietly)
            {
                battle.unk53BDC8 = 0;
            }

            if ((battle.panelFlags & BattleState::UnkPanel8) != 0)
            {
                battle.unk53BDC8 = 0;
            }
        }
    }

    if (battle.unk53BF00 - battle.unk53BF02 >= static_cast<i32>(battle.enemyCount) || this->fightTalk.unk53CEAC == 1)
    {
        battle.unk53BF5C = 1;

        if (battle.unk53BFE4 == 1 && this->FadeFieldOut() == 1)
        {
            this->ReturnBattleItems();
            battle.unk53BDC8 = 0;
        }
    }
}

// swd32.exe 0x0045E660: the pointer prompt of the battle state for its first 300 frames (always while unk53BDAC is
// negative): kWaitPointer in states 1, 2, 4, 5, 8, 0x1B and 0x1E; in state 3 (with unk53BFBC) kTurnPointer or
// kNoTurnPointer by unk4A7640; otherwise kUnitPointer with a current unit, else kNoUnitPointer, or with a negative
// unk53BDAC the effect kMarkEffect at (unk53BF50, unk53BF52) (unk53BF58 cleared once it ends).
void Game::DrawBattlePrompt()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;
    ++battle.unk53BDA8;

    if (battle.unk53BDA8 >= kPromptFrames && battle.unk53BDAC >= 0)
    {
        return;
    }

    switch (battle.result)
    {
        case kWaitState1:
        case kWaitState2:
        case kWaitState4:
        case kWaitState5:
        case kWaitState8:
        case kWaitState1B:
        case kWaitState1E:
            hud.DrawEffectAt(kWaitPointer, 0, x, y, 0);
            break;

        case kTurnState:
            if (battle.unk53BFBC == 1)
            {
                hud.DrawEffectAt(battle.unk4A7640 != 0 ? kTurnPointer : kNoTurnPointer, 0, x, y, 0);
            }

            break;

        default:
            if (battle.unk53BDAC >= 0)
            {
                hud.DrawEffectAt(battle.currentUnit != 0 ? kUnitPointer : kNoUnitPointer, 0, x, y, 0);
            }
            else if (
                hud.DrawEffectAt(
                    kMarkEffect,
                    battle.unk53BF58,
                    battle.unk53BF50,
                    battle.unk53BF52,
                    battle.mirrored != 0 ? 1 : 0
                ) == 1
            )
            {
                battle.unk53BF58 = 0;
            }

            break;
    }
}

// swd32.exe 0x0045D2F0: one frame of the field fade (while any step is set): `counting` counts its frames down; while
// frames are left (not negative) the levels move by the steps, otherwise the steps take the fade's end levels (the
// original copies them into the steps, not the levels). The field rows of the frame are tinted by the levels.
void Game::FieldFadeFrame(int counting)
{
    ColorFade& fade = this->battleState.fieldFade;

    if (fade.stepRed == 0 && fade.stepGreen == 0 && fade.stepBlue == 0)
    {
        return;
    }

    bool moving = true;

    if (counting != 0)
    {
        if (fade.frames < 0)
        {
            moving = false;
        }
        else
        {
            --fade.frames;
        }
    }

    if (moving && fade.frames >= 0)
    {
        fade.red += fade.stepRed;
        fade.green += fade.stepGreen;
        fade.blue += fade.stepBlue;
    }
    else
    {
        fade.stepRed = fade.startRed;
        fade.stepGreen = fade.startGreen;
        fade.stepBlue = fade.startBlue;
    }

    const auto blue = static_cast<int>(crt::Ftol(fade.blue));
    const auto green = static_cast<int>(crt::Ftol(fade.green));
    const auto red = static_cast<int>(crt::Ftol(fade.red));
    const std::span<u16> field(this->drawSurface, static_cast<std::size_t>(Display::Width * BattleState::FieldHeight));
    this->painter.TintPixels(field, red, green, blue);
}

// swd32.exe 0x0045EC80: while no unit acts, a side that is all down or turned to stone (UnkStatus4) loses: the party
// (forced units left out; the count against the roles less the guests, in 8 bits) unless unk53BFE4, the stone roles
// counted as defeated and the battle state 0x67; then, unless unk53C048, the enemies, counted as defeated, unk53C048
// set, the auto battle off and the battle state 0x63. 1 when a side lost.
int Game::CheckStoneSides()
{
    BattleState& battle = this->battleState;

    if (battle.unk53AE70 != -1)
    {
        return 0;
    }

    u8 stoneRoles = 0;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (role.unk2B00 != 1 && role.unk2B04 != 1 && role.HasEffectBits(BattleUnit::UnkStatus4) == 1)
        {
            ++stoneRoles;
        }
    }

    const auto partyRoles = static_cast<u8>(
        static_cast<u8>(battle.roleCount) - static_cast<u8>(battle.unk53BF26) - static_cast<u8>(battle.guestCount)
    );

    if (stoneRoles != 0 && battle.defeatedRoles + stoneRoles >= partyRoles && battle.unk53BFE4 == 0)
    {
        battle.defeatedRoles = static_cast<u8>(battle.defeatedRoles + stoneRoles);
        battle.result = kBattleLost;
        return 1;
    }

    if (battle.unk53C048 != 0)
    {
        return 0;
    }

    u8 stoneEnemies = 0;

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        if (this->battleEnemies[index].HasEffectBits(BattleUnit::UnkStatus4) == 1)
        {
            ++stoneEnemies;
        }
    }

    if (stoneEnemies == 0 || battle.unk53BF00 + stoneEnemies < static_cast<i32>(battle.enemyCount) ||
        battle.unk53BFE4 != 0)
    {
        return 0;
    }

    battle.unk53BF00 = static_cast<u8>(battle.unk53BF00 + stoneEnemies);
    battle.unk53C048 = 1;
    battle.autoBattle = 0;
    battle.result = kBattleWon;
    return 1;
}

// swd32.exe 0x0045F020: the next slot record taken into `taken`: the records of held roles (IsHeldOrHolding) at the
// front are passed over; a used record taken is removed (the later ones move up, the free ones after the used ones
// cleared). The original also takes unit 7 for a role (the one before the first) and reads past the records when all
// are held; both stop the search here.
void Game::TakeSlotRecord(BattleSlotRecord& taken)
{
    BattleState& battle = this->battleState;
    std::size_t at = 0;

    while (at < battle.unk524788.size())
    {
        const i32 unit = battle.unk524788[at].unit;

        if (unit < BattleState::FirstRoleUnit ||
            this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsHeldOrHolding() != 1)
        {
            break;
        }

        ++at;
    }

    battle.TakeSlotRecordAt(at, taken);
}

// swd32.exe 0x0045A980: one frame of the damage numbers of role `index` (`role` 1) or enemy `index`; 1 once they are
// done (the original tests unk53BF80 first, both ways alike).
int Game::DamageNumbersFrame(int index, int role)
{
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(index);
    BattleUnit& unit =
        role == 1 ? static_cast<BattleUnit&>(this->battleRoles[at]) : static_cast<BattleUnit&>(this->battleEnemies[at]);
    return unit.DrawDamageNumbers(services) == 1 ? 1 : 0;
}

// swd32.exe 0x0045EB40: the units in draw order show their guard text and their damage numbers (once done unk53BFD8
// is set and their unk524420 entry cleared: a role by its role index, so a role and the enemy of the same index share
// it), then take their pending damage: a unit that does has its unk502984 entry set to its index and the slot record
// of that index removed (a role's index, as in the original).
void Game::ApplyHits()
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto units = static_cast<std::size_t>(battle.roleCount + battle.enemyCount);

    for (std::size_t order = 0; order < units; ++order)
    {
        const i32 unitId = battle.drawOrder[order];
        const bool isRole = unitId >= BattleState::FirstRoleUnit;
        const int index = isRole ? unitId - BattleState::FirstRoleUnit : unitId;
        const auto at = static_cast<std::size_t>(index);
        BattleUnit& unit = isRole ? static_cast<BattleUnit&>(this->battleRoles[at])
                                  : static_cast<BattleUnit&>(this->battleEnemies[at]);
        unit.DrawGuardText(services);

        if (this->DamageNumbersFrame(index, isRole ? 1 : 0) == 1)
        {
            battle.unk53BFD8 = 1;
            battle.unk524420[at] = -1;
        }

        if (!unit.ApplyPendingDamage(services))
        {
            continue;
        }

        battle.unk502984[at] = index;
        battle.RemoveSlotRecord(index);
    }
}

// swd32.exe 0x00465090: the next enemy takes its turn: enemy unk53BCF4 (0 past the enemies); a defeated one passes
// on through unk4A7550 (1-based in enemyOrder, wrapping) for at most enemyCount tries, after which the battle state
// is 1. The enemy is marked (SetAC0 1) and aimed at (unk4A754C, 1-based).
void Game::NextEnemyTurn()
{
    BattleState& battle = this->battleState;
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.unk53BCF4 >= count)
    {
        battle.unk53BCF4 = 0;
    }

    if (this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].IsDefeated())
    {
        int tries = 0;

        while (true)
        {
            if (++battle.unk4A7550 > count)
            {
                battle.unk4A7550 = 1;
            }

            battle.unk53BCF4 = battle.enemyOrder[static_cast<std::size_t>(battle.unk4A7550 - 1)];

            if (++tries >= count)
            {
                battle.result = kTurnsRunning;
                break;
            }

            if (!this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].IsDefeated())
            {
                break;
            }
        }
    }

    this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].SetAC0(1);
    battle.unk53BFB8 = 1;
    battle.unk4A754C = battle.unk53BCF4 + 1;
}

// swd32.exe 0x00465170: the party role whose turn follows: unk4A7550 steps (1-based, wrapping over the party roles)
// through RoleCycle until it names the current unit's role, which unk4A754C then aims at (1-based). The original
// loops for ever when the cycle never names it.
void Game::NextRoleTurn()
{
    BattleState& battle = this->battleState;
    const auto roles = static_cast<i32>(battle.roleCount - battle.unk53BF26 - battle.guestCount);
    i32 at = battle.unk4A7550;
    i32 role = 0;
    int steps = 0;

    do
    {
        if (++at > roles)
        {
            at = 1;
        }

        if (++steps > static_cast<int>(BattleState::RoleCycle.size()) ||
            at >= static_cast<i32>(BattleState::RoleCycle.size()))
        {
            Hang();
        }

        role = BattleState::RoleCycle[static_cast<std::size_t>(at)];
    } while (role != battle.currentUnit - BattleState::FirstRoleUnit);

    battle.unk4A7550 = at;
    battle.unk4A754C = role + 1;
    battle.unk53BCF4 = 0;
    battle.unk53BFB8 = 1;
}

// swd32.exe 0x00461900: the back key in a list (kBackSound): battle state 2 scrolls unk53BD04 up by kListPage, states
// 4 and 0x1B unk53BD08 (state 4 also keeps unk4A755C and the scroll for role unk4A7570); each first only marks its
// list (unk4A7558, unk4A755C) unless unk53BD9C.
void Game::ScrollListBack()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto scroll = [](i32& top)
    {
        top -= kListPage;

        if (top < 0)
        {
            top = 0;
        }
    };

    if (battle.result == kListState2)
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BD9C == 0 && battle.unk4A7558 != 1)
        {
            battle.unk4A7558 = 1;
            return;
        }

        scroll(battle.unk53BD04);
        battle.unk53BFB8 = 1;
        return;
    }

    if (battle.result != kListState4)
    {
        if (battle.result != kListState1B)
        {
            return;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BD9C == 0 && battle.unk4A755C != 1)
        {
            battle.unk4A755C = 1;
            return;
        }

        scroll(battle.unk53BD08);
        battle.unk53BFB8 = 1;
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.unk4A755C != 1)
    {
        battle.unk4A755C = 1;
        return;
    }

    scroll(battle.unk53BD08);
    battle.unk53BFB8 = 1;
    const auto role = static_cast<std::size_t>(battle.unk4A7570);
    battle.unk4FF578[role] = battle.unk4A755C;
    battle.unk52544C[role] = battle.unk53BD08;
}

// swd32.exe 0x00461A30: the forward key in a list (kBackSound): battle state 2 scrolls unk53BD04 down by kListPage
// within unk53BDF2 entries (nothing below a page), states 4 and 0x1B unk53BD08 within unk53BDF4; each first only
// marks the last row of its page (unk4A7558, unk4A755C, at most the entries) unless unk53BD9C. A list shorter than a
// page goes back to 0 with the mark on its end. State 4, and the first mark of state 0x1B (the original shares that
// code), keep the mark and the scroll for role unk4A7570.
void Game::ScrollListForward()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto keepForRole = [&battle](i32 mark)
    {
        const auto role = static_cast<std::size_t>(battle.unk4A7570);
        battle.unk53BFB8 = 1;
        battle.unk4FF578[role] = mark;
        battle.unk52544C[role] = battle.unk53BD08;
    };

    auto markPageEnd = [&battle, &keepForRole]
    {
        battle.unk4A755C = std::min<i32>(kListPage, battle.unk53BDF4);
        keepForRole(battle.unk4A755C);
    };

    if (battle.result == kListState2)
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BDF2 < kListPage)
        {
            return;
        }

        if (battle.unk53BD9C == 0 && battle.unk4A7558 != kListPage)
        {
            battle.unk4A7558 = kListPage;
            return;
        }

        battle.unk53BD04 += kListPage;

        if (battle.unk53BD04 + kListPage > battle.unk53BDF2)
        {
            battle.unk53BD04 = battle.unk53BDF2 - kListPage;
        }

        if (battle.unk53BD04 < 0)
        {
            battle.unk53BD04 = 0;
            battle.unk4A7558 = battle.unk53BDF2;
        }

        battle.unk53BFB8 = 1;
        return;
    }

    if (battle.result != kListState4 && battle.result != kListState1B)
    {
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.unk4A755C != kListPage)
    {
        markPageEnd();
        return;
    }

    battle.unk53BD08 += kListPage;

    if (battle.unk53BD08 + kListPage > battle.unk53BDF4)
    {
        battle.unk53BD08 = battle.unk53BDF4 - kListPage;
    }

    if (battle.unk53BD08 < 0)
    {
        battle.unk53BD08 = 0;
        battle.unk4A755C = battle.unk53BDF4;
    }

    if (battle.result == kListState1B)
    {
        battle.unk53BFB8 = 1;
        return;
    }

    keepForRole(battle.unk4A755C);
}

// swd32.exe 0x0045D490: the auto battle takes the current role's turn (unless in battle state 3): with no remembered
// enemy (unk4A7564) the state becomes 3; otherwise the role acts on it (unk4A754C), or guards with MarkBit1 (its
// order taken as 1); a defeated enemy gives way to the first standing one, and with none the auto battle ends.
void Game::AutoBattleTurn()
{
    BattleState& battle = this->battleState;

    if (battle.autoBattle != 1 || battle.currentUnit == 0 || battle.result == kTurnState)
    {
        return;
    }

    battle.unk53BCE8 = 1;
    battle.unk53AF38[static_cast<std::size_t>(battle.currentUnit)] = 1;

    if (battle.unk4A7564 == -1)
    {
        battle.unk53BFB8 = 1;
        battle.result = kTurnState;
        return;
    }

    battle.unk53BD50 = battle.currentUnit;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)];
    role.SetTarget(1);
    battle.unk4A754C = battle.unk4A7564;
    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;

    if (role.MarkBit1() == 1)
    {
        battle.unk53BCE8 = kGuardOrder;
        battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = kGuardOrder;
        role.ResetAfterGuard();
        role.SetTarget(1);
        battle.pendingOrders[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)].unk00 = 1;
        return;
    }

    if (!this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].IsDefeated())
    {
        return;
    }

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        if (!this->battleEnemies[enemy].IsDefeated())
        {
            battle.unk4A754C = static_cast<i32>(enemy) + 1;
            return;
        }
    }

    role.SetTarget(0);
    battle.unk53BCE8 = 0;
    battle.unk4A754C = 1;
    battle.unk4A7564 = -1;
    battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = 0;
    battle.autoBattle = 0;
}
}  // namespace swd3
