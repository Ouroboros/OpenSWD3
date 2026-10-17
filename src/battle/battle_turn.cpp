#include "game/game.h"

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
}  // namespace swd3
