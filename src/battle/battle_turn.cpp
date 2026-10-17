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

    if (battle.unk53BF00 - battle.unk53BF02 >= static_cast<i32>(battle.enemyCount) || battle.unk53CEAC == 1)
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
}  // namespace swd3
